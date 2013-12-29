#include <mchck.h>

void
cond_init(void)
{
        ftm_init();
        FTM0.sc.clks = 0; // disable clock
        FTM0.sc.ps = FTM_PS_DIV4; // prescale
        FTM0.mode.ftmen = 1;
        int_enable(IRQ_FTM0);
        FTM0.combine[0].decapen = 1;

        // channel 0 captures falling edge
        FTM0.channel[0].csc.msa = 1;  // continuous
        FTM0.channel[0].csc.elsa = 0;
        FTM0.channel[0].csc.elsb = 1;
        FTM0.channel[0].csc.chf = 0;

        // channel 1 captures rising edge
        FTM0.channel[1].csc.elsa = 1;
        FTM0.channel[1].csc.elsb = 0;
        FTM0.channel[1].csc.chf = 0;
        FTM0.channel[1].csc.chie = 1;

        // PTC1 = pin 34 = FTM0_CH0
        FTM0.filter.ch0fval = 5;
        pin_mode(PIN_PTC1, PIN_MODE_MUX_ALT4); // | PIN_MODE_PULLDOWN);
}

void
cond_start(void)
{
        // start continuous dual-edge capture mode
        FTM0.sc.clks = 1; // enable clock
        FTM0.combine[0].decap = 1;
}

void
cond_stop(void)
{
        FTM0.sc.clks = 0; // disable clock
}

#define BUFFER_LEN 128
int32_t buffer[BUFFER_LEN];
unsigned int i = 0;

void
FTM0_Handler(void)
{
        // acknowledge interrupt
        uint32_t a;
        a = FTM0.channel[0].csc.raw;
        FTM0.channel[0].csc.chf = 0;
        a = FTM0.channel[1].csc.raw;
        FTM0.channel[1].csc.chf = 0;

        // fetch edge times
        uint32_t t1 = FTM0.channel[0].cv;
        uint32_t t2 = FTM0.channel[1].cv;
        int32_t dt = t1 - t2;
        if (dt < 0)
                dt += 0xffff;

        //buffer[i] = dt;
        //i = (i+1) % BUFFER_LEN;
        i++;
        printf("%5d  %5ld  %5ld\n", dt, t1, t2);
}
