/* Red Pitaya C API example of Generating two asynced burst signals */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "rp.h"

int main(int argc, char **argv){
    /* Print error, if rp_Init() function failed */
    if(rp_Init() != RP_OK){
            fprintf(stderr, "Rp api init failed!\n");
    }

    /* Reset Generation */
    rp_GenReset();

    /* Generation */
    rp_GenWaveform(RP_CH_1, RP_WAVEFORM_SINE);
    rp_GenFreq(RP_CH_1, 4);
    rp_GenAmp(RP_CH_1, 1.0);

    rp_GenMode(RP_CH_1, RP_GEN_MODE_BURST);
    rp_GenBurstCount(RP_CH_1, 2);
    rp_GenBurstRepetitions(RP_CH_1, 1);
    rp_GenBurstPeriod(RP_CH_1, 5000);

    sleep(2);
    
    rp_GenTriggerOnly(RP_CH_1);
    
    rp_Release();
}