#ifndef _SYS_MOUNT_H
#define _SYS_MOUNT_H

#include <abi-bits/mount.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MS_BIND 1
#define MS_RDONLY 2
#define MS_NOSUID 4
#define MS_NODEV 8
#define MS_NOEXEC 16
#define MS_SYNCHRONOUS 32
#define MS_NOATIME 64

int mount(const char *source, const char *target,
		const char *fstype, unsigned long flags, const void *data);
int umount(const char *target);
int umount2(const char *target, int flags);

#ifdef __cplusplus
}
#endif

#endif // _SYS_MOUNT_H
