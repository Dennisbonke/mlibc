
#include <spawn.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <sched.h>
#include <signal.h>
#include <sys/wait.h>

#include <bits/ensure.h>
#include <mlibc/debug.hpp>

/*
 * This posix_spawn implementation is taken from musl
 */

static unsigned long handler_set[NSIG / (8 * sizeof(long))];

void __get_handler_set(sigset_t *set) {
	memcpy(set, handler_set, sizeof handler_set);
}

struct args {
	int p[2];
	sigset_t oldmask;
	const char *path;
	const posix_spawn_file_actions_t *fa;
	const posix_spawnattr_t *__restrict attr;
	char *const *argv, *const *envp;
};

static int child(void *args_vp) {
	int i, ret;
	struct sigaction sa = {0};
	struct args *args = (struct args *)args_vp;
	int p = args->p[1];
	const posix_spawn_file_actions_t *fa = args->fa;
	const posix_spawnattr_t *__restrict attr = args->attr;
	sigset_t hset;

	int (*exec)(const char *, char *const *, char *const *) =
		(int (*)())attr->__fn ? (int (*)())attr->__fn : (int (*)())execve;

	close(args->p[0]);
	mlibc::infoLogger() << "mlibc: posix_spawn: child enter!" << frg::endlog;

	/* All signal dispositions must be either SIG_DFL or SIG_IGN
	 * before signals are unblocked. Otherwise a signal handler
	 * from the parent might get run in the child while sharing
	 * memory, with unpredictable and dangerous results. To
	 * reduce overhead, sigaction has tracked for us which signals
	 * potentially have a signal handler. */
	// TODO NOT REPLACED YET!
	__get_handler_set(&hset);
	mlibc::infoLogger() << "mlibc: posix_spawn: child 1!" << frg::endlog;
	for(i = 1; i < NSIG; i++) {
		if((attr->__flags & POSIX_SPAWN_SETSIGDEF) && sigismember(&attr->__def, i)) {
			sa.sa_handler = SIG_DFL;
		} else if(sigismember(&hset, i)) {
			if (i - 32 < 3U) {
				sa.sa_handler = SIG_IGN;
			} else {
				// TODO, maybe use sigaction?
				//__libc_sigaction(i, 0, &sa);
				sigaction(i, 0, &sa);
				if(sa.sa_handler == SIG_IGN)
					continue;
				sa.sa_handler = SIG_DFL;
			}
		} else {
			continue;
		}
		// TODO, maybe use sigaction?
		//__libc_sigaction(i, &sa, 0);
		sigaction(i, &sa, 0);
	}
	mlibc::infoLogger() << "mlibc: posix_spawn: child 2!" << frg::endlog;

	if(attr->__flags & POSIX_SPAWN_SETSID) {
		// TODO, replace with setsid?
		//if((ret=__syscall(SYS_setsid)) < 0)
		if((ret = setsid()) < 0)
			goto fail;
	}

	if(attr->__flags & POSIX_SPAWN_SETPGROUP) {
		mlibc::infoLogger() << "mlibc: REEEEE SETPGID NOT IMPEMENTED, IGNORING!" << frg::endlog;
		// TODO, replace with setpgid?
		//if((ret = __syscall(SYS_setpgid, 0, attr->__pgrp)))
		//if((ret = setpgid(0, attr->__pgrp)))
		//	goto fail;
	}
	mlibc::infoLogger() << "mlibc: posix_spawn: child 3!" << frg::endlog;

	/* Use syscalls directly because the library functions attempt
	 * to do a multi-threaded synchronized id-change, which would
	 * trash the parent's state. */
	if(attr->__flags & POSIX_SPAWN_RESETIDS) {
		// TODO, replace with non syscalls
		/*if((ret = __syscall(SYS_setgid, __syscall(SYS_getgid))) ||
		    (ret = __syscall(SYS_setuid, __syscall(SYS_getuid))) )*/
		if((ret = setgid(getgid())) || (ret = setuid(getuid())) )
			goto fail;
	}

	if(fa && fa->__actions) {
		struct fdop *op;
		int fd;
		for(op = (struct fdop *)fa->__actions; op->next; op = op->next);
		for(; op; op = op->prev) {
			/* It's possible that a file operation would clobber
			 * the pipe fd used for synchronizing with the
			 * parent. To avoid that, we dup the pipe onto
			 * an unoccupied fd. */
			if(op->fd == p) {
				// TODO, replace with dup
				ret = dup(p);
				if(ret < 0)
					goto fail;
				// TODO, replace with close
				close(p);
				p = ret;
			}
			switch(op->cmd) {
			case FDOP_CLOSE:
				// TODO: replace with close
				close(op->fd);
				break;
			case FDOP_DUP2:
				fd = op->srcfd;
				if(fd == p) {
					ret = -EBADF;
					goto fail;
				}
				if(fd != op->fd) {
					// TODO, replace with dup2
					if((ret = dup2(fd, op->fd)) < 0)
						goto fail;
				} else {
					// TODO, replace with fcntl
					ret = fcntl(fd, F_GETFD);
					ret = fcntl(fd, F_SETFD, ret & ~FD_CLOEXEC);
					if(ret < 0)
						goto fail;
				}
				break;
			case FDOP_OPEN:
				// TODO, replace with open
				fd = open(op->path, op->oflag, op->mode);
				if((ret = fd) < 0)
					goto fail;
				if(fd != op->fd) {
					// TODO, replace with dup2
					if((ret = dup2(fd, op->fd)) < 0)
						goto fail;
					// TODO, replace with close
					close(fd);
				}
				break;
			case FDOP_CHDIR:
				// TODO, replace with chdir
				ret = chdir(op->path);
				if(ret < 0)
					goto fail;
				break;
			case FDOP_FCHDIR:
				// TODO, replace with fchdir
				ret = fchdir(op->fd);
				if(ret < 0)
					goto fail;
				break;
			}
		}
	}
	mlibc::infoLogger() << "mlibc: posix_spawn: child 4!" << frg::endlog;

	/* Close-on-exec flag may have been lost if we moved the pipe
	 * to a different fd. We don't use F_DUPFD_CLOEXEC above because
	 * it would fail on older kernels and atomicity is not needed --
	 * in this process there are no threads or signal handlers. */
	// TODOm replace with fcntl
	fcntl(p, F_SETFD, FD_CLOEXEC);

	pthread_sigmask(SIG_SETMASK, (attr->__flags & POSIX_SPAWN_SETSIGMASK)
		? &attr->__mask : &args->oldmask, 0);

	/*int (*exec)(const char *, char *const *, char *const *) =
		attr->__fn ? (int (*)())attr->__fn : execve;*/
	mlibc::infoLogger() << "mlibc: posix_spawn: child 5!" << frg::endlog;

	exec(args->path, args->argv, args->envp);
	ret = -errno;
	mlibc::infoLogger() << "mlibc: posix_spawn: child 6!" << frg::endlog;

fail:
	/* Since sizeof errno < PIPE_BUF, the write is atomic. */
	ret = -ret;
	// TODO, replace with write
	if(ret)
		while(write(p, &ret, sizeof ret) < 0);
	mlibc::infoLogger() << "mlibc: posix_spawn: child exit!" << frg::endlog;
	_exit(127);
}

int posix_spawn(pid_t *__restrict res, const char *__restrict path,
		const posix_spawn_file_actions_t *file_actions,
		const posix_spawnattr_t *__restrict attrs,
		char *const argv[], char *const envp[]) {
	mlibc::infoLogger() << "mlibc: posix_spawn enter!" << frg::endlog;
	pid_t pid;
	char stack[1024 + PATH_MAX];
	int ec=0, cs;
	struct args args;

	//pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &cs);

	args.path = path;
	args.fa = file_actions;
	args.attr = attrs ? attrs : &(const posix_spawnattr_t){0};
	args.argv = argv;
	args.envp = envp;
	mlibc::infoLogger() << "mlibc: posix_spawn 1!" << frg::endlog;
	pthread_sigmask(SIG_BLOCK, SIGALL_SET, &args.oldmask);
	mlibc::infoLogger() << "mlibc: posix_spawn 2!" << frg::endlog;

	/* The lock guards both against seeing a SIGABRT disposition change
	 * by abort and against leaking the pipe fd to fork-without-exec. */
	//LOCK(__abort_lock);

	if(pipe2(args.p, O_CLOEXEC)) {
		//UNLOCK(__abort_lock);
		ec = errno;
		goto fail;
	}
	mlibc::infoLogger() << "mlibc: posix_spawn 3!" << frg::endlog;

	//pid = clone(child, stack + sizeof stack, CLONE_VM | CLONE_VFORK | SIGCHLD, &args);
	pid = fork();
	if(!pid) {
		child(&args);
	}
	close(args.p[1]);
	mlibc::infoLogger() << "mlibc: posix_spawn 4!" << frg::endlog;
	//UNLOCK(__abort_lock);

	if(pid > 0) {
		if(read(args.p[0], &ec, sizeof ec) != sizeof ec)
			ec = 0;
		else
			waitpid(pid, &(int){0}, 0);
	} else {
		ec = -pid;
	}
	mlibc::infoLogger() << "mlibc: posix_spawn 5!" << frg::endlog;

	close(args.p[0]);

	if(!ec && res)
		*res = pid;
	mlibc::infoLogger() << "mlibc: posix_spawn 6!" << frg::endlog;

fail:
	pthread_sigmask(SIG_SETMASK, &args.oldmask, 0);
	//pthread_setcancelstate(cs, 0);
	mlibc::infoLogger() << "mlibc: posix_spawn exit!" << frg::endlog;

	return ec;
}

int posix_spawnattr_init(posix_spawnattr_t *attr) {
	__ensure(!"Not implemented");
	__builtin_unreachable();
}

int posix_spawnattr_destroy(posix_spawnattr_t *attr) {
	__ensure(!"Not implemented");
	__builtin_unreachable();
}

int posix_spawnattr_setflags(posix_spawnattr_t *attr, short flags) {
	__ensure(!"Not implemented");
	__builtin_unreachable();
}

int posix_spawnattr_setsigdefault(posix_spawnattr_t *__restrict attr,
		const sigset_t *__restrict sigdefault) {
	__ensure(!"Not implemented");
	__builtin_unreachable();
}

int posix_spawnattr_setschedparam(posix_spawnattr_t *__restrict attr,
		const struct sched_param *__restrict schedparam) {
	__ensure(!"Not implemented");
	__builtin_unreachable();
}

int posix_spawnattr_setschedpolicy(posix_spawnattr_t *attr, int schedpolicy) {
	__ensure(!"Not implemented");
	__builtin_unreachable();
}

int posix_spawnattr_setsigmask(posix_spawnattr_t *__restrict attr,
		const sigset_t *__restrict sigmask) {
	__ensure(!"Not implemented");
	__builtin_unreachable();
}

int posix_spawnattr_setpgroup(posix_spawnattr_t *attr, pid_t pgroup) {
	__ensure(!"Not implemented");
	__builtin_unreachable();
}

int posix_spawn_file_actions_init(posix_spawn_file_actions_t *file_actions) {
	__ensure(!"Not implemented");
	__builtin_unreachable();
}

int posix_spawn_file_actions_destroy(posix_spawn_file_actions_t *file_actions) {
	__ensure(!"Not implemented");
	__builtin_unreachable();
}

int posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t *file_actions,
		int fildes, int newfildes) {
	__ensure(!"Not implemented");
	__builtin_unreachable();
}

int posix_spawn_file_actions_addclose(posix_spawn_file_actions_t *file_actions,
		int fildes) {
	__ensure(!"Not implemented");
	__builtin_unreachable();
}

int posix_spawn_file_actions_addopen(posix_spawn_file_actions_t *__restrict file_actions,
		int fildes, const char *__restrict path, int oflag, mode_t mode) {
	__ensure(!"Not implemented");
	__builtin_unreachable();
}

int posix_spawnp(pid_t *__restrict pid, const char *__restrict file,
		const posix_spawn_file_actions_t *file_actions,
		const posix_spawnattr_t *__restrict attrp,
		char *const argv[], char *const envp[]) {
	__ensure(!"Not implemented");
	__builtin_unreachable();
}

