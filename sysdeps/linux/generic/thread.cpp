#include <mlibc/thread-entry.hpp>
#include <mlibc/all-sysdeps.hpp>
#include <mlibc/thread.hpp>
#include <bits/ensure.h>
#include <sys/mman.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>

extern "C" void __mlibc_enter_thread(void *entry, void *user_arg) {
	// The linux kernel already sets the TCB in sys_clone().
	auto tcb = mlibc::get_current_tcb();

	// Wait until our parent sets up the TID.
	while(!__atomic_load_n(&tcb->tid, __ATOMIC_RELAXED))
		mlibc::sys_futex_wait(&tcb->tid, 0, nullptr);

	void *(*func)(void *) = reinterpret_cast<void *(*)(void *)>(entry);
	auto result = func(user_arg);

	tcb->returnValue = result;
	__atomic_store_n(&tcb->didExit, 1, __ATOMIC_RELEASE);
	mlibc::sys_futex_wake(&tcb->didExit);

	mlibc::sys_thread_exit();
}

namespace mlibc {

int sys_prepare_stack(void **stack, void *entry, void *user_arg, void *tcb, size_t stack_size, size_t guard_size) {
	uintptr_t map = reinterpret_cast<uintptr_t>(
			mmap(nullptr, stack_size + guard_size,
				PROT_NONE,
				MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)
			);
	if (reinterpret_cast<void*>(map) == MAP_FAILED)
		return EAGAIN;
	int ret = mprotect(reinterpret_cast<void*>(map + guard_size), stack_size,
			PROT_READ | PROT_WRITE);
	if(ret)
		return EAGAIN;

	auto sp = reinterpret_cast<uintptr_t*>(map + stack_size + guard_size);
	*--sp = reinterpret_cast<uintptr_t>(user_arg);
	*--sp = reinterpret_cast<uintptr_t>(entry);
	*stack = reinterpret_cast<void*>(sp);
	return 0;
}
} // namespace mlibc
