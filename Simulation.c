#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "rp.h"

int main (int argc, char **argv) {
    int unsigned period = 1000000; // uS
    int unsigned led;


    if (argc > 1) {
        led = atoi(argv[1]);
    }

    printf("Blinking LED[0]\n");
    printf("Emulation started\n");

    led = RP_LED0;

    // Initialization of API
    if (rp_Init() != RP_OK) {
        fprintf(stderr, "Red Pitaya API init failed!\n");
        return EXIT_FAILURE;
    }
    if (rp_AcqReset() != RP_OK) {
        fprintf(stderr, "rp_AcqReser failed!\n");
        return -1;
    }
    if (rp_AcqStart() != RP_OK) {
        fprintf(stderr, "rp_AcqStart failed!\n");
        return -1;
    }

    rp_AcqSetTriggerSrc(RP_TRIG_SRC_CHA_PE);
    rp_AcqSetTriggerLevel(RP_T_CH_1,1);
    rp_acq_trig_state_t state = RP_TRIG_STATE_TRIGGERED;
    
    //Activation du port out1
    rp_GenReset();

    rp_GenWaveform(RP_CH_1, RP_WAVEFORM_SQUARE);
    rp_GenFreq(RP_CH_1, 1);
    rp_GenAmp(RP_CH_1, 2.0);

    rp_GenMode(RP_CH_1, RP_GEN_MODE_BURST);
    rp_GenBurstCount(RP_CH_1, 1);
    rp_GenBurstRepetitions(RP_CH_1, 10);
    rp_GenBurstPeriod(RP_CH_1, 5000);
    
    //Attente
    int unsigned retries = 10000; //durée de la simulation en s
    while (retries--){
        rp_AcqGetTriggerState(&state);
        if(state == RP_TRIG_STATE_TRIGGERED){
            printf("trigger\n");
            rp_GenOutEnable(RP_CH_1);
            //déclenchement out1 NOW
            rp_GenTriggerOnly(RP_CH_1);
            break;
        }

             
        //led indiquant que la simulation tourne
        rp_DpinSetState(led, RP_HIGH);
    }

    rp_DpinSetState(led, RP_LOW);
    // Releasing resources
    rp_Release();

    return EXIT_SUCCESS;
}
