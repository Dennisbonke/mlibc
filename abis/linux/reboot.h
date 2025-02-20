#ifndef _ABIBITS_REBOOT_H
#define _ABIBITS_REBOOT_H

#include <mlibc-config.h>

#if !__MLIBC_LINUX_OPTION
#  error "<linux/reboot.h> is inherently Linux specific. Enable the Linux option or do not use this header."
#endif /* !__MLIBC_LINUX_OPTION */

#define RB_AUTOBOOT 0x01234567
#define RB_HALT_SYSTEM 0xcdef0123
#define RB_ENABLE_CAD 0x89abcdef
#define RB_DISABLE_CAD 0
#define RB_POWER_OFF 0x4321fedc
#define RB_SW_SUSPEND 0xd000fce2
#define RB_KEXEC 0x45584543

#endif /* _ABIBITS_REBOOT_H */
