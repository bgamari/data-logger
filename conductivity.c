#include <mchck.h>

void
cond_init(void)
{
        ftm_init();
        FTM0.sc.clks = 0; // disable clock
        FTM0.mode.ftmen = 1;
        int_enable(IRQ_FTM0);
        FTM0.combine[0].decapen = 1;

        // channel 0 captures falling edge
        FTM0.channel[0].csc.msa = 1;  // continuous
        FTM0.channel[0].csc.elsa = 0;
        FTM0.channel[0].csc.elsb = 1;
        FTM0.channel[0].csc.chf = 0;
        FTM0.channel[0].csc.chie = 1;

        // channel 1 captures rising edge
        FTM0.channel[1].csc.elsa = 1;
        FTM0.channel[1].csc.elsb = 0;
        FTM0.channel[1].csc.chf = 0;
        FTM0.channel[1].csc.chie = 1;

        // PTC1 = pin 34 = FTM0_CH0
        pin_mode(PIN_PTC1, PIN_MODE_MUX_ALT4 | PIN_MODE_PULLDOWN);
        FTM0.sc.clks = 1; // enable clock

        // start continuous dual-edge capture mode
        FTM0.combine[0].decap = 1;
}

void
FTM0_Handler(void)
{
        uint32_t a;
        a = FTM0.sc.raw;
        FTM0.sc.tof = 0;

        a = FTM0.channel[0].csc.raw;
        FTM0.channel[0].csc.chf = 0;
        a = FTM0.channel[1].csc.raw;
        FTM0.channel[1].csc.chf = 0;
        uint32_t t1 = FTM0.channel[0].cv;
        uint32_t t2 = FTM0.channel[1].cv;
        printf("%d %d\n", t1, t2);
}
