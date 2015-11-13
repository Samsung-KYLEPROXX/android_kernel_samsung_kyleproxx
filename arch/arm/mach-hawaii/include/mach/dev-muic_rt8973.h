#ifndef __ARM_ARCH_DEV_MUIC_RT8973_H
#define __ARM_ARCH_DEV_MUIC_RT8973_H

extern void uas_jig_force_sleep_rt8973(void);
extern int bcm_ext_bc_status_rt8973(void);
extern unsigned int musb_get_charger_type_rt8973(void);
extern void musb_vbus_changed_rt8973(int state);
#endif