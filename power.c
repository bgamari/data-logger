#include "power.h"
#include "acquire.h"

volatile bool low_power_mode = false;

/* enter Bypassed Low-Power Internal clocking mode */
static void
enter_blpi()
{
        // FEI to FBI (ref manual pg. 454)
        MCG.c1.irclken = 0;
        MCG.sc.fcrdiv = 0x3; // divide IRC by 8
        MCG.c1.irclken = 1;
        MCG.c2.ircs = MCG_IRCS_FAST;
        while (MCG.s.ircst != MCG_IRCST_FAST);
        MCG.c1.clks = MCG_CLKS_INTERNAL;
        while (MCG.s.clkst != MCG_CLKST_INTERNAL);

        // FBI to BLPI (ref manual pg. 461)
        MCG.c6.plls = 0;
        MCG.c2.lp = 1;
}

/* enter Very Low-Power Run state */
static void
enter_vlpr()
{
        // Enable very low power modes
        SIM.clkdiv1.raw = ((struct SIM_CLKDIV1_t) { .outdiv1 = 1, .outdiv2 = 1, .outdiv4 = 3 }).raw;
        SMC.pmprot.raw = ((struct SMC_PMPROT) { .avlls = 1, .alls = 1, .avlp = 1 }).raw;

        SMC.pmctrl.raw = ((struct SMC_PMCTRL) { .runm = RUNM_VLPR, .stopm = STOPM_VLPS }).raw;
}

void
enter_low_power_mode()
{
        enter_blpi();
        enter_vlpr();
        low_power_mode = true;

        LLWU.wupe[0].wupe0 = LLWU_PE_FALLING;
        int_enable(IRQ_LLWU);
}

void wakeup()
{
        // TODO: Come out of low power mode for a while
        acquire_blink_state();
}

extern void RTC_alarm_Handler(void);

void
LLWU_Handler(void)
{
        if (LLWU.mwuf & (1<<5)) {
                // RTC Alarm
                RTC_alarm_Handler();
        }
        if (LLWU.wuf1 & (1<<0)) {
                // wakeup pin
                wakeup();
        }
        LLWU.wuf1 = 0xff;
        LLWU.wuf2 = 0xff;
        LLWU.mwuf = 0xff;
}
