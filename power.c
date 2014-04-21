#include "power.h"
#include "acquire.h"
#include "usb_console.h"
#include "sensors/adc_sensor.h"

/* Power management */

/*
 * These cannot be updated without also changing the LLWU
 * configuration below
 */
#define WAKEUP_PIN PIN_PTA4
//#define USB_SENSE_PIN PIN_PTD6

/*
 * In low-power mode we allow the device to enter VLPR and VLPS states
 * at the expense of most connectivity perhipherals being disabled
 * (namely SPI and USB).
 */
volatile bool low_power_mode = false;

/* the current system clockrate */
volatile uint32_t system_clockrate = 48000000;

#define LLWU_LPTMR        (1 << 0)
#define LLWU_RTC_ALARM    (1 << 5)

/* Note: Nice block diagram of MCG on pg. 430 of reference manual */

/* enter Bypassed Low-Power Internal (BLPI) clocking mode from FLL Engaged
 * Internal (FEI)
 */
static void
enter_blpi(void)
{
        const uint8_t irc_divider = 3;

        // divide fast IRC by 8
        MCG.c1.irclken = 0;
        MCG.sc.fcrdiv = irc_divider;
        MCG.c1.irclken = 1;

        // FEI to FBI (ref manual pg. 454)
        MCG.c6.plls = MCG_PLLS_FLL;
        while (MCG.s.pllst != MCG_PLLST_FLL);
        MCG.c1.irefs = MCG_IREFS_INTERNAL;
        while (MCG.s.irefst != MCG_IREFST_INTERNAL);
        MCG.c1.clks = MCG_CLKS_INTERNAL;
        while (MCG.s.clkst != MCG_CLKST_INTERNAL);
        system_clockrate = 2000000 / (1<<irc_divider);

        // FBI to BLPI (ref manual pg. 461)
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
        MCG.c6.plls = MCG_PLLS_FLL;
        while (MCG.s.pllst != MCG_PLLST_FLL);

        // "In FEI mode, only the slow Internal Reference Clock (IRC)
        // can be used as the FLL source." (ref manual pg. 428)
        MCG.c1.irefs = MCG_IREFS_INTERNAL; // slow IRC as FLL source
        while (MCG.s.irefst != MCG_IREFST_INTERNAL);

        MCG.c1.clks = MCG_CLKS_FLLPLL;
        while (MCG.s.clkst != MCG_CLKST_FLL);
        system_clockrate = 48000000;
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
#ifndef NO_VLPR
        enter_vlpr();
#endif
        low_power_mode = true;
}

void
exit_low_power_mode()
{
        if (!low_power_mode)
                return;
#ifndef NO_VLPR
        exit_vlpr();
#endif
        exit_blpi();
        low_power_mode = false;
        adc_initialized = false;
}

void
power_init()
{
        // Enable low-power modes
        SMC.pmprot.raw = ((struct SMC_PMPROT) { .avlls = 1, .alls = 1, .avlp = 1 }).raw;

        // Configure wakeup pin (LLWU_P3 = PTA4)
        gpio_dir(WAKEUP_PIN, GPIO_INPUT);
        pin_mode(WAKEUP_PIN, PIN_MODE_PULLUP);
        LLWU.wupe[0].wupe3 = LLWU_PE_FALLING;
        LLWU.filt1.filtsel = 3; // P3
        LLWU.filt1.filte = LLWU_FILTER_BOTH;

        // Configure USB sense pin (LLWU_P15 = PTD6)
#ifdef USB_SENSE_PIN
        pin_mode(USB_SENSE_PIN, PIN_MODE_MUX_GPIO);
        gpio_dir(USB_SENSE_PIN, GPIO_INPUT);
        LLWU.wupe[3].wupe3 = LLWU_PE_RISING;
        LLWU.filt1.filtsel = 15; // P15
        LLWU.filt1.filte = LLWU_FILTER_BOTH;
#endif
        LLWU.wume = LLWU_LPTMR | LLWU_RTC_ALARM;

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

#ifdef USB_SENSE_PIN
void
usb_sense_pin_handler(void *cbdata)
{
        if (gpio_read(USB_SENSE_PIN)) {
                exit_low_power_mode();
                usb_console_init();
                acquire_blink_state();
        } else {
                stdout->ops = NULL;
                enter_low_power_mode();
        }
}
PIN_DEFINE_CALLBACK(USB_SENSE_PIN, PIN_CHANGE_EITHER, usb_sense_pin_handler, NULL);
#endif

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
extern void LPT_Handler(void);

void
LLWU_Handler(void)
{
        if (LLWU.mwuf & LLWU_RTC_ALARM) {
                // RTC Alarm
                RTC_alarm_Handler();
        }
        if (LLWU.mwuf & LLWU_LPTMR) {
                // LPTMR
                LPT_Handler();
        }
        if (LLWU.wuf1 & (1<<3)) {   // WAKEUP_PIN == LLWU_P3 == PTA4
                wakeup_pin_handler(NULL);
        }
#ifdef USB_SENSE_PIN
        if (LLWU.wuf2 & (1<<7)) {   // USB_SENSE_PIN == LLWU_P15 == PTD6
                usb_sense_pin_handler(NULL);
        }
#endif        

        LLWU.wuf1 = 0xff;
        LLWU.wuf2 = 0xff;
        LLWU.mwuf = 0xff;
}
