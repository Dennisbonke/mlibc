#include <bits/ensure.h>
#include <errno.h>
#include <frg/mutex.hpp>
#include <frg/spinlock.hpp>
#include <mlibc/debug.hpp>
#include <paths.h>
#include <stddef.h>
#include <stdlib.h>
#include <utmpx.h>

#include <mlibc/posix-sysdeps.hpp>

namespace {

constexpr const char *defaultUtmpxPath = _PATH_UTMPX;

const char *utmpxPath = defaultUtmpxPath;
frg::ticket_spinlock utmpxMutex;

frg::optional<int> utmpxFd = frg::null_opt;

utmpx returned;

}

void updwtmpx(const char *file, const struct utmpx *ut) {
	int fd;
	int err = mlibc::sys_open(file, O_WRONLY | O_LARGEFILE | O_CLOEXEC, 0644, &fd);
	if (err)
		return;

	off_t discard;
	err = mlibc::sys_seek(fd, 0, SEEK_END, &discard);
	if (err) {
		mlibc::sys_close(fd);
		return;
	}

	ssize_t written = 0;
	err = mlibc::sys_write(fd, ut, sizeof(struct utmpx), &written);
	if (err || written != sizeof(struct utmpx)) {
		mlibc::sys_close(fd);
		return;
	}

	mlibc::sys_close(fd);
}

void endutxent(void) {
	frg::unique_lock lock{utmpxMutex};

	if(utmpxFd) {
		mlibc::sys_close(utmpxFd.value());
		utmpxFd = frg::null_opt;
	}
}

void setutxent(void) {
	frg::unique_lock lock{utmpxMutex};

	if(!utmpxFd) {
		int fd;
		int err = mlibc::sys_open(utmpxPath, O_RDWR | O_CREAT | O_CLOEXEC, 0644, &fd);
		if(err) {
			mlibc::infoLogger() << "\e[31mmlibc: setutxent() failed to open " << utmpxPath << ": "
			                    << strerror(err) << "\e[39m" << frg::endlog;
			utmpxFd = frg::null_opt;
		} else {
			utmpxFd = fd;
		}
	} else {
		off_t discard;
		mlibc::sys_seek(utmpxFd.value(), 0, SEEK_SET, &discard);
	}
}

struct utmpx *getutxent(void) {
	frg::unique_lock lock{utmpxMutex};

	if (!utmpxFd) {
		setutxent();
		if (!utmpxFd)
			return NULL;
	}

	ssize_t read = 0;
	int err = mlibc::sys_read(utmpxFd.value(), &returned, sizeof(struct utmpx), &read);

	if (err || read != sizeof(struct utmpx))
		return NULL;

	return &returned;
}

struct utmpx *pututxline(const struct utmpx *entry) {
	__ensure(utmpxFd);
	frg::unique_lock lock{utmpxMutex};

	size_t progress = 0;
	uint8_t *ptr = (uint8_t *) entry;

	off_t discard;
	int err = mlibc::sys_seek(utmpxFd.value(), 0, SEEK_END, &discard);
	__ensure(!err);

	while(progress < sizeof(*entry)) {
		ssize_t written = 0;
		if(mlibc::sys_write(utmpxFd.value(), ptr + progress, sizeof(*entry) - progress, &written))
			return nullptr;
		progress += written;
	}

	return (struct utmpx *) entry;
}

int utmpxname(const char *file) {
	frg::unique_lock lock{utmpxMutex};

	if(strcmp(file, utmpxPath)) {
		if(!strcmp(file, defaultUtmpxPath)) {
			free((void *) utmpxPath);
			utmpxPath = defaultUtmpxPath;
		} else {
			char *name = strdup(file);
			if(!name)
				return -1;

			if(utmpxPath != defaultUtmpxPath)
				free((void *)utmpxPath);

			utmpxPath = name;
		}
	}

	return 0;
}

struct utmpx *getutxid(const struct utmpx *ut) {
	frg::unique_lock lock{utmpxMutex};

	if(ut->ut_type == RUN_LVL || ut->ut_type == BOOT_TIME ||
	   ut->ut_type == NEW_TIME || ut->ut_type == OLD_TIME) {
		while(true) {
			ssize_t read = 0;
			int err = mlibc::sys_read(utmpxFd.value(), &returned, sizeof(struct utmpx), &read);
			if(err || read != sizeof(struct utmpx)) {
				errno = ESRCH;
				return nullptr;
			}

			if(returned.ut_type == ut->ut_type)
				return &returned;
		}
	} else if(ut->ut_type == INIT_PROCESS || ut->ut_type == LOGIN_PROCESS ||
	          ut->ut_type == USER_PROCESS || ut->ut_type == DEAD_PROCESS) {
		while(true) {
			ssize_t read = 0;
			int err = mlibc::sys_read(utmpxFd.value(), &returned, sizeof(struct utmpx), &read);
			if(err || read != sizeof(struct utmpx)) {
				errno = ESRCH;
				return nullptr;
			}

			if(!memcmp(returned.ut_id, ut->ut_id, sizeof(ut->ut_id)))
				return &returned;
		}
	}

	errno = ESRCH;
	return NULL;
}

struct utmpx *getutxline(const struct utmpx *ut) {
	frg::unique_lock lock{utmpxMutex};

	while(true) {
		ssize_t read = 0;
		int err = mlibc::sys_read(utmpxFd.value(), &returned, sizeof(struct utmpx), &read);
		if(err || read != sizeof(struct utmpx)) {
			errno = ESRCH;
			return nullptr;
		}

		if((returned.ut_type == USER_PROCESS || returned.ut_type == LOGIN_PROCESS) &&
				!strcmp(returned.ut_line, ut->ut_line)) {
			return &returned;
		}
	}

	return NULL;
}
