/* Red Pitaya C API example Acquiring a signal from a buffer
 * This application acquires a signal on a specific channel */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "rp.h"

#define DATA_SIZE 64
#define SAMPLE_RATE 125000000

int main(int argc, char **argv)
{
    int dsize = DATA_SIZE;
    uint32_t dec = 64;

    float excitation_duration_seconds = 41.027e-06;
    float excitation_duration_microseconds = 41.027e-06*1000000;
    float excitation_amplitude_Volts = 0.19;
    float Larmor_frequency_Hertz = 24378040.422;
    uint32_t excitation_burst_cycles =Larmor_frequency_Hertz * excitation_duration_seconds;

    char* nomFichier = "donnees.csv";
    if (argc >= 4){
        dsize = atoi(argv[1]);
        dec = atoi(argv[2]);
        nomFichier = argv[3]; //modifier pour être sur qu'on met bien le nom d'un fichier .csv"
    }

    /* Creation du ficher pour recolter les resultats*/
    FILE *fichier = fopen(nomFichier, "w");

    if (fichier == NULL) {
        perror("Erreur lors de l'ouverture du fichier");
        return EXIT_FAILURE; // Quitter le programme avec un code d'erreur
    }

    /* Print error, if rp_Init() function failed */
    if (rp_InitReset(false) != RP_OK) {
        fprintf(stderr, "Rp api init failed!\n");
        return -1;
    }

    uint32_t g_adc_axi_start,g_adc_axi_size;
    rp_AcqAxiGetMemoryRegion(&g_adc_axi_start,&g_adc_axi_size);

    printf("Reserved memory start 0x%X size 0x%X bytes\n",g_adc_axi_start,g_adc_axi_size);
 //rp_AcqResetFpga();

    if (rp_AcqAxiSetDecimationFactor(dec) != RP_OK) {
        fprintf(stderr, "rp_AcqAxiSetDecimationFactor failed!\n");
        return -1;
    }
    if (rp_AcqAxiSetTriggerDelay(RP_CH_1, 0 )  != RP_OK) { //Trigger at the begining of the buffer
        fprintf(stderr, "rp_AcqAxiSetTriggerDelay RP_CH_1 failed!\n");
        return -1;
    }
    if (rp_AcqAxiSetBufferSamples(RP_CH_1, g_adc_axi_start, dsize) != RP_OK) {
        fprintf(stderr, "rp_AcqAxiSetBuffer RP_CH_1 failed!\n");
        return -1;
    }
    if (rp_AcqAxiEnable(RP_CH_1, true)) {
        fprintf(stderr, "rp_AcqAxiEnable RP_CH_1 failed!\n");
        return -1;
    }
    if (rp_AcqStart() != RP_OK) {
        fprintf(stderr, "rp_AcqStart failed!\n");
        return -1;
    }
    

   //rp_AcqSetTriggerSrc(RP_TRIG_SRC_CHA_PE);
     rp_GenReset();

    rp_GenWaveform(RP_CH_1, RP_WAVEFORM_SINE);
    rp_GenFreq(RP_CH_1, Larmor_frequency_Hertz);
    rp_GenAmp(RP_CH_1, excitation_amplitude_Volts);

    rp_GenMode(RP_CH_1, RP_GEN_MODE_BURST);
    rp_GenBurstCount(RP_CH_1, excitation_burst_cycles);
    rp_GenBurstRepetitions(RP_CH_1, 1);
    rp_GenBurstPeriod(RP_CH_1, 1);

    

    printf("enable");
    rp_GenOutEnable(RP_CH_1);
    rp_GenTriggerOnly(RP_CH_1);

    printf("sleep"); 
    usleep(excitation_duration_microseconds);
    rp_AcqSetTriggerSrc(RP_TRIG_SRC_NOW);
    
    rp_acq_trig_state_t state = RP_TRIG_STATE_TRIGGERED;
    while(1){
        rp_AcqGetTriggerState(&state);
        if(state == RP_TRIG_STATE_TRIGGERED){
            sleep(1);
            break;
        }
    }


    bool fillState = false;
    while (!fillState) {
        if (rp_AcqAxiGetBufferFillState(RP_CH_1, &fillState) != RP_OK) {
            fprintf(stderr, "rp_AcqAxiGetBufferFillState RP_CH_1 failed!\n");
            return -1;
        }
    }

    rp_AcqStop();

    uint32_t posChA;
    rp_AcqAxiGetWritePointerAtTrig(RP_CH_1,&posChA);
    fprintf(stderr,"Tr pos1: 0x%X\n",posChA);

    float *buff1 = (float *)malloc(dsize * sizeof(float));

    uint32_t size1 = dsize;
    rp_AcqAxiGetDataV(RP_CH_1, posChA, &size1, buff1);


    for (int i = 0; i < dsize; i++) {
        // printf("[%d]\t%f\n",i,buff1[i]);
        fprintf(fichier, "%f", buff1[i]);
        if (i!= dsize -1) fprintf(fichier, ",");
    }
    fprintf(fichier, "\n");

    for (int i = 0; i < dsize; i++) {
        float time = (float)i/(float)SAMPLE_RATE*dec;
	fprintf(fichier, "%f", time);
        if (i!= dsize -1) fprintf(fichier, ",");
    }

    fprintf(fichier, "\n");
    fclose(fichier);


    /* Releasing resources */
    rp_AcqAxiEnable(RP_CH_1, false);
    rp_Release();
    free(buff1);
    return 0;
}
