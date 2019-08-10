#ifndef _ASM_GENERIC_EMERGENCY_RESTART_H
#define _ASM_GENERIC_EMERGENCY_RESTART_H

static inline void machine_emergency_restart(void)
{
#ifdef CONFIG_HUAWEI_RESET_DETECT
	machine_restart("emergency_restart");
#else
	machine_restart(NULL);
#endif
}

#endif /* _ASM_GENERIC_EMERGENCY_RESTART_H */
