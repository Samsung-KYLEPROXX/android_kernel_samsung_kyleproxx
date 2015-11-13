#ifdef CONFIG_CHECK_SIMSLOT_COUNT

#include <linux/proc_fs.h>
#include <linux/i2c-gpio.h>
#include <asm/gpio.h>

#ifdef CONFIG_MACH_JAVA_SS_BAFFINLITE
/* below values would be change by H/W schematic of each model */
#define SIM_SLOT_PIN 16
#define SINGLE_SIM_VALUE 0
#define DUAL_SIM_VALUE 1
#endif


enum
{
        NO_SIM = 0,
        SINGLE_SIM,
        DUAL_SIM,
        TRIPLE_SIM
};

#endif
