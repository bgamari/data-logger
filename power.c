#include "power.h"
#include "acquire.h"
#include "usb_console.h"

/* Power management */

#define WAKEUP_PIN PIN_PTA4
#define USB_SENSE_PIN PIN_PTD6

/*
 * In low-power mode we allow the device to enter VLPR and VLPS sates
 * at the expense of most connectivity perhipherals being disabled
 * (namely SPI and USB).
 */
volatile bool low_power_mode = false;

/* enter Bypassed Low-Power Internal (BLPI) clocking mode from FLL Engaged
 * Internal (FEI)
 */
static void
enter_blpi(void)
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

/* re-enter FLL Engaged Internal (FEI) from Bypassed Low-Power
 * Internal (BLPI) clocking mode
 */
static void
exit_blpi(void)
{
        // BLPI to FBI (ref manual pg. 463)
        MCG.c2.lp = 0;

        // FBI to FEI
        MCG.c6.plls = 0; // FLL
        MCG.c1.clks = MCG_CLKS_FLLPLL;

        while (MCG.s.clkst != MCG_CLKST_FLL);
}

/* enter Very Low-Power Run state */
static void
enter_vlpr(void)
{
        // in VLPR FLASH can only run up to 1 MHz, set dividers appropriately
        SIM.clkdiv1.raw = ((struct SIM_CLKDIV1_t) { .outdiv1 = 1, .outdiv2 = 1, .outdiv4 = 3 }).raw;
        SMC.pmctrl.raw = ((struct SMC_PMCTRL) { .runm = RUNM_VLPR, .stopm = STOPM_VLPS }).raw;
}

/* exit Very Low-Power Run state */
static void
exit_vlpr(void)
{
        SIM.clkdiv1.raw = ((struct SIM_CLKDIV1_t) { .outdiv1 = 0, .outdiv2 = 0, .outdiv4 = 1 }).raw;
        SMC.pmctrl.raw = ((struct SMC_PMCTRL) { .runm = RUNM_RUN, .stopm = STOPM_STOP }).raw;
}

void
enter_low_power_mode()
{
        if (low_power_mode)
                return;
        enter_blpi();
        enter_vlpr();
        low_power_mode = true;
}

void
exit_low_power_mode()
{
        if (!low_power_mode)
                return;
        exit_vlpr();
        exit_blpi();
        low_power_mode = false;
}

void
power_init()
{
        // Enable low-power modes
        SMC.pmprot.raw = ((struct SMC_PMPROT) { .avlls = 1, .alls = 1, .avlp = 1 }).raw;

        // Configure wakeup pin
	gpio_dir(WAKEUP_PIN, GPIO_INPUT);
        pin_mode(WAKEUP_PIN, PIN_MODE_PULLUP);
        LLWU.wupe[0].wupe3 = LLWU_PE_FALLING;

        // Configure USB sense pin
	gpio_dir(USB_SENSE_PIN, GPIO_INPUT);
        LLWU.wupe[3].wupe3 = LLWU_PE_RISING;

        int_enable(IRQ_LLWU);
}

static struct timeout_ctx sleep_timeout;
static bool activity = false;

void
power_notify_activity()
{
        activity = true;
}

/*
 * If there has been no activity within the sleep timeout
 * we go back to sleep
 */
static void
back_to_sleep(void *cbdata)
{
        if (!activity)
                enter_low_power_mode();
}

void
usb_sense_pin_handler(void *cbdata)
{
        if (gpio_read(USB_SENSE_PIN)) {
                exit_low_power_mode();
                usb_console_init();
        } else {
                enter_low_power_mode();
        }
}
PIN_DEFINE_CALLBACK(USB_SENSE_PIN, PIN_CHANGE_EITHER, usb_sense_pin_handler, NULL);

void
wakeup_pin_handler(void *cbdata)
{
        if (!low_power_mode)
                return;
        // try wakeup
        exit_low_power_mode();
        acquire_blink_state();
        activity = false;
        timeout_add(&sleep_timeout, 5000, back_to_sleep, NULL);
}
PIN_DEFINE_CALLBACK(WAKEUP_PIN, PIN_CHANGE_FALLING, wakeup_pin_handler, NULL);

extern void RTC_alarm_Handler(void);

void
LLWU_Handler(void)
{
        if (LLWU.mwuf & (1<<5)) {
                // RTC Alarm
                RTC_alarm_Handler();
        }
        if (LLWU.wuf1 & (1<<3)) {
                wakeup_pin_handler(NULL);
        }
        if (LLWU.wuf2 & (1<<7)) {
                usb_sense_pin_handler(NULL);
        }
        
        LLWU.wuf1 = 0xff;
        LLWU.wuf2 = 0xff;
        LLWU.mwuf = 0xff;
}
