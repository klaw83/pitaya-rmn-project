#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "rp.h"

int main (int argc, char **argv) {
    int unsigned led = RP_LED0;

    float excitation_duration_microseconds = 41.027;
    float excitation_amplitude_Volts = 0.1;
    float answer_amplitude_Volts = 0.5;
    float Larmor_frequency_Hertz = 24378040.422;
    int duration_burst_second = 1;
    int number_burst_cycle = Larmor_frequency_Hertz*duration_burst_second;
    
    printf("number burst %d \n",number_burst_cycle);

    if (argc > 1) {
        led = atoi(argv[1]);
    }

    printf("Blinking LED[0]\n");
    printf("Emulation started\n");

    
    // Initialization of API
    if (rp_Init() != RP_OK) {
        fprintf(stderr, "Red Pitaya API init failed!\n");
        return EXIT_FAILURE;
    }

    while(1){
        if (rp_AcqReset() != RP_OK) {
            fprintf(stderr, "rp_AcqReset failed!\n");
            return -1;
        }
        if (rp_AcqStart() != RP_OK) {
            fprintf(stderr, "rp_AcqStart failed!\n");
            return -1;
        }


        rp_GenReset();
        rp_GenWaveform(RP_CH_1, RP_WAVEFORM_SINE);
        rp_GenFreq(RP_CH_1, Larmor_frequency_Hertz);
        rp_GenAmp(RP_CH_1, 0.5);

        rp_GenMode(RP_CH_1, RP_GEN_MODE_BURST);
        rp_GenBurstCount(RP_CH_1, 50000);       //valeur max pour GenBurstCount
        rp_GenBurstRepetitions(RP_CH_1, 1000);  //Répété 1000 fois pour que le burst dure qq secondes
        rp_GenBurstPeriod(RP_CH_1, 1);          //une micro seconde entre chaque répétition
        
        rp_GenTriggerSource(RP_CH_1, RP_GEN_TRIG_SRC_EXT_PE);

        rp_GenOutEnable(RP_CH_1);
        rp_DpinSetState(led+1, RP_HIGH);
        //led indiquant que la simulation tourne


/*      //Attente
        
        while (1){
            rp_AcqGetTriggerState(&state);
            if(state == RP_TRIG_STATE_TRIGGERED){
                usleep(excitation_duration_microseconds);
                
                break;
            }
            usleep(1);
        } */

        

        rp_DpinSetState(led, RP_LOW);
        rp_DpinSetState(led+1, RP_LOW);
        sleep(3);
}
    // Releasing resources
    rp_Release();

    return EXIT_SUCCESS;
}
