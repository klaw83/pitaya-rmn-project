/////// VERSION 2 - rapide dans un seul fichier ////// 
/////// 10 en 60ms
/////// utiliser Acquisition RMN FULL

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "rp.h"

#define DATA_SIZE_MAX 524288
#define SAMPLE_RATE 125000000

/**
 * @brief Crée un fichier avec un en-tête contenant des informations d'acquisition.
 *
 * Le fichier contiendra les informations suivantes sur une première ligne :
 *  dsize, decimation, nombre de fichiers, gain, offset, nombre de bits.
 *
 * @param fichier           Pointeur vers le fichier ouvert en écriture.
 * @param dsize             Taille des données à enregistrer.
 * @param dec               Facteur de décimation utilisé lors de l'acquisition.
 * @param number_of_steps   Nombre de fichiers à traiter/enregistrer.
 *
 * @return int              0 si succès, -1 si erreur lors de la récupération du gain.
 *
 * @note Le offset est actuellement fixé à 0 car la fonction rp_AcqAxiGetOffset n'est pas supportée.
 * @note Le gain est récupéré via rp_AcqGetGainV sur le canal 2 (RP_CH_2).
 */
int create_file(FILE* fichier, int dsize, int dec, int number_of_steps){   

    float gainValue; //1 ou 20V
    
    float offset = 0;
    int nb_bits = 14; 
    rp_channel_t channel = RP_CH_2;

    // Fonction ne marche pas rp_AcqAxigetOffset non comprise par la RedPitaya lors de la compilation
    // if (rp_AcqAxiGetOffset(channel, &offset) != RP_OK){
    //     return -1;
    // }

    if (rp_AcqGetGainV(channel, &gainValue) != RP_OK){
        return -1;
    }

    fprintf(fichier, "%d, ", dsize );
    fprintf(fichier, "%d, ", dec );
    fprintf(fichier, "%d, ", number_of_steps);
    fprintf(fichier, "%f, ",(float) gainValue);
    fprintf(fichier, "%f, ", offset);
    fprintf(fichier, "%d\n", nb_bits);

    return 0;
}

float find_max(float buff[], int size) {
    if (size <= 0) {
        printf("Error: Array size must be positive\n");
        return -1; // or handle error appropriately
    }
    
    float max = abs(buff[0]); // Initialize max with first element
    
    for (int i = 1; i < size; i++) {
        if (abs(buff[i]) > max) {
            max = abs(buff[i]);
        }
    }
    
    return max;
}


int main(int argc, char **argv)
{
    uint32_t dsize = DATA_SIZE_MAX;
    uint32_t dec =1; 
    
    
    float excitation_duration_seconds;
    float excitation_amplitude_Volts = 0.19;
    float Larmor_frequency_Hertz = 24351392.574;
    float oscillator_amplitude_Volts = 0.8;

    int number_of_steps = 1;
    char nomFichier[256];
    rp_pinState_t Gain = RP_HIGH;
    rp_channel_t CH_ACQ = RP_CH_1;


    rp_acq_trig_state_t state;
    uint32_t g_adc_axi_start,g_adc_axi_size;

    ////-------- ARGUMENTS DU PROGRAMME
    dsize = atoi(argv[1]);
    dec = atoi(argv[2]);
    number_of_steps = atoi(argv[3]);    
    strcpy(nomFichier, argv[4]);
    Larmor_frequency_Hertz = atof(argv[5]);
    excitation_duration_seconds = atof(argv[6]);
    float step_frequency_Hertz = atof(argv[7]);
    int wait = atoi(argv[8]);
    ////-------- ----------------------


    printf("original Larmor frequency %f, constant duration excitation %f\n",Larmor_frequency_Hertz, excitation_duration_seconds);

    int excitation_burst_cycles_tot = Larmor_frequency_Hertz * excitation_duration_seconds;
    printf("Initial excitation burst cycles = %d\n \n",excitation_burst_cycles_tot);

    float oscillator_frequency = Larmor_frequency_Hertz + 1000;
    float excitation_duration_useconds = excitation_duration_seconds*1000000;

    float *buff1 = (float *)malloc(dsize * sizeof(float));
    uint32_t posChA;
    bool fillState = false;


    ////-------- Table initialisation
    float *maximum_amplitude_t = (float*) malloc((number_of_steps+2) * sizeof(float));
    float *Larmor_frequency_Hertz_t = (float*) malloc((number_of_steps+2) * sizeof(float));

    maximum_amplitude_t[0] = 0; //Initial value of the max

    Larmor_frequency_Hertz_t[0] = Larmor_frequency_Hertz;
    
    Larmor_frequency_Hertz_t[1] = Larmor_frequency_Hertz;

    ////---------------------------

    if (rp_InitReset(false) != RP_OK) {
            fprintf(stderr, "Rp api init failed!\n");
            return -1;
    }


    //// INITIALISATION AQUISITION ///////
    if(rp_AcqReset()!=RP_OK){
        fprintf(stderr, "rp_AcqReset failed!\n");
        return -1;
    }
    if (rp_AcqAxiSetDecimationFactor(dec) != RP_OK) {
        fprintf(stderr, "rp_AcqAxiSetDecimationFactor failed!\n");
        return -1;
    }
    if (rp_AcqAxiSetTriggerDelay(CH_ACQ, dsize)  != RP_OK) { 
        fprintf(stderr, "rp_AcqAxiSetTriggerDelay CH_ACQ failed!\n");
        return -1;
    }
    if (rp_AcqSetGain(CH_ACQ, Gain) != RP_OK){
        fprintf(stderr, "rp_AcqSetGain CH1 Failed\n");
        return -1;
    }



    //// INITIALISATION GENERATION BURST ///////
    if(rp_GenReset() != RP_OK){
            fprintf(stderr, "rp_GenReset failed!\n");
            return -1;
    } 
    if(rp_GenWaveform(RP_CH_1, RP_WAVEFORM_SINE) != RP_OK){
        fprintf(stderr, "rp_GenWaveform RP_CH_1 SINE failed!\n");
        return -1;
    }
    if(rp_GenFreq(RP_CH_1, Larmor_frequency_Hertz) != RP_OK){
        fprintf(stderr, "rp_GenFreq RP_CH_1 failed!\n");
        return -1;
    }
    if(rp_GenAmp(RP_CH_1, excitation_amplitude_Volts) != RP_OK){
        fprintf(stderr, "rp_GenAmp RP_CH_1 failed!\n");
        return -1;
    }
    if(rp_GenMode(RP_CH_1, RP_GEN_MODE_BURST) != RP_OK){
        fprintf(stderr, "rp_GenMode RP_CH_1 BURST failed!\n");
        return -1;
    }
    if(rp_GenBurstRepetitions(RP_CH_1, 1) != RP_OK){
        fprintf(stderr, "rp_GenBurstRepetitions RP_CH_1 failed!\n");
        return -1;
    }//Répété 1 fois pour que le burst dure qq usecondes   
    if(rp_GenBurstPeriod(RP_CH_1, 1) != RP_OK){
        fprintf(stderr, "rp_GenBurstPeriod RP_CH_1 failed!\n");
        return -1;
    }//une micro seconde entre chaque répétition
    if( rp_GenOutEnable(RP_CH_1) != RP_OK){
        fprintf(stderr, "rp_GenOutEnable RP_CH_1 failed!\n");
        return -1;
    }


    //// INITIALISATION ET LANCEMENT DE L'OSCILLATEUR LOCAL
    if(rp_GenWaveform(RP_CH_2, RP_WAVEFORM_SINE) != RP_OK){
        fprintf(stderr, "rp_GenWaveform RP_CH_2 SINE failed!\n");
        return -1;
    }
    if(rp_GenFreq(RP_CH_2, oscillator_frequency) != RP_OK){
        fprintf(stderr, "rp_GenFreq RP_CH_2 failed!\n");
        return -1;
    }
    if(rp_GenAmp(RP_CH_2, oscillator_amplitude_Volts) != RP_OK){
        fprintf(stderr, "rp_GenAmp RP_CH_2 failed!\n");
        return -1;
    }
    if(rp_GenMode(RP_CH_2, RP_GEN_MODE_CONTINUOUS) != RP_OK){
        fprintf(stderr, "rp_GenMode RP_CH_2 BURST failed!\n");
        return -1;
    }
    if( rp_GenOutEnable(RP_CH_2) != RP_OK){
        fprintf(stderr, "rp_GenOutEnable RP_CH_2 failed!\n");
        return -1;
    }
  

    //// CREATION DU FICHIER    
    FILE *fichier = fopen(nomFichier, "w");
    printf("fichier crée : ");
    puts(nomFichier);
    if (fichier == NULL) {
        perror("Erreur lors de l'ouverture du fichier\n");
        return -1; // Quitte le programme avec erreur
    }
    if (create_file(fichier, dsize, dec, number_of_steps)){
        perror("Erreur de creation fichier");
        return -1;
    }


    
    //////////BOUCLE DE FICHIERS//////////
    rp_DpinSetState(RP_LED0+1, RP_HIGH);
    clock_t begin = clock();
    int i=0;
    for (i=0;i<number_of_steps;i++){


        if(rp_GenBurstCount(RP_CH_1, excitation_burst_cycles_tot) != RP_OK){
            fprintf(stderr, "rp_GenBurstCount RP_CH_1 failed!\n");
            return -1;
        }//valeur max pour GenBurstCount = 50 000

        if(rp_GenFreq(RP_CH_1, *Larmor_frequency_Hertz_t) != RP_OK){
            fprintf(stderr, "rp_GenFreq RP_CH_1 failed!\n");
        return -1;
        }

        oscillator_frequency = *Larmor_frequency_Hertz_t + 1000;
        if(rp_GenFreq(RP_CH_2, oscillator_frequency) != RP_OK){
            fprintf(stderr, "rp_GenFreq RP_CH_2 failed!\n");
        return -1;
        }
        
        fillState = false;
        state = RP_TRIG_STATE_WAITING;
        //rp_AcqResetFpga();
        rp_AcqAxiGetMemoryRegion(&g_adc_axi_start,&g_adc_axi_size);
        //printf("Reserved memory start 0x%X size 0x%X bytes\n",g_adc_axi_start,g_adc_axi_size);

        if (rp_AcqAxiSetBufferSamples(CH_ACQ,g_adc_axi_start, dsize) != RP_OK) {
        fprintf(stderr, "rp_AcqAxiSetBuffer CH_ACQ failed!\n");
        return -1;
        }
        if (rp_AcqAxiEnable(CH_ACQ, true)) {
            fprintf(stderr, "rp_AcqAxiEnable CH_ACQ failed!\n");
            return -1;
        }

       ////////////DECLENCHEMENT SYNCHRONISE///////////////
        if(rp_GenSynchronise() != RP_OK){
            fprintf(stderr, "rp_GenSynchronise failed!\n");
            return -1;
        }

        usleep(excitation_duration_useconds);
        //LANCEMENT, DECLENCGEMENT DE L'AQUISITION 
        if (rp_AcqStart() != RP_OK) {
        fprintf(stderr, "rp_AcqStart failed!\n");
        return -1;
        }
        if( rp_AcqSetTriggerSrc(RP_TRIG_SRC_NOW) != RP_OK){
            fprintf(stderr, "rp_AcqSetTriggerSrc RP_TRIG_SRC_NOW failed!\n");
            return -1;
        }

        // ATTENTE DU DECLENCHEMENT DU TRIGGER
        while(1){
            rp_AcqGetTriggerState(&state);
            if(state == RP_TRIG_STATE_TRIGGERED){
                usleep(1);
                break;
            }
        }


         
        //printf ("wait to be filled\n");
        while (!fillState) {
            if (rp_AcqAxiGetBufferFillState(CH_ACQ, &fillState) != RP_OK) {
                fprintf(stderr, "rp_AcqAxiGetBufferFillState CH_ACQ failed!\n");
                return -1;
            }
        }
        
        if(rp_AcqStop() != RP_OK){
                fprintf(stderr, "rp_AcqStop failed!\n");
                return -1;
        }

        if(rp_AcqAxiGetWritePointerAtTrig(CH_ACQ, &posChA)!=RP_OK){
            fprintf(stderr,"rp_AcqAxiGetWritePointerAtTrig Error");
        }
        //printf("Tr pos1: 0x%x\n",posChA);

        if(rp_AcqAxiGetDataV(CH_ACQ, posChA, &dsize, buff1)!=RP_OK){
            fprintf(stderr, "rp_AcqAxiGetDataV failed\n");
        }
        
        //////  Ecriture des données dans le fichier    //////
        printf("ecriture FID %d\n",i);
        for (int j = 0; j < dsize; j++) {
            //printf("[%d]\t%f\n",i,buff1[i]);
            fprintf(fichier, "%f", buff1[j]);
            if (j!= dsize -1) fprintf(fichier, ",");
        }
        fprintf(fichier, "\n");
    
        /////////////////////////////////////////
        ////// ----- Find max of Buff - ////////

        maximum_amplitude_t[i+1] = find_max(buff1, dsize);
        if (maximum_amplitude_t[i+1] != -1) {
            printf("Max Amplitude : %f\n", maximum_amplitude_t[i+1]);
        }

        //////////////////////////////////////
        ///// ---- MMPT ALGO ------ /////////
        
        if(maximum_amplitude_t[i+1]-maximum_amplitude_t[i]>0){
            if(Larmor_frequency_Hertz_t[i+1] - Larmor_frequency_Hertz_t[i]>0){
                Larmor_frequency_Hertz_t[i+2] = Larmor_frequency_Hertz_t[i+1] + step_frequency_Hertz;
                printf("Augmentation de la fréquence\t");
            }
            else{
                *(Larmor_frequency_Hertz_t+2) = *Larmor_frequency_Hertz_t - step_frequency_Hertz;
                printf("Diminution de la fréquence\t");
            }
        }
        
        else{
            if(Larmor_frequency_Hertz_t[i+1] - Larmor_frequency_Hertz_t[i]<0){
                Larmor_frequency_Hertz_t[i+2] = Larmor_frequency_Hertz_t[i+1] - step_frequency_Hertz;
                printf("Diminution de la fréquence\t");
            }
            else{
                Larmor_frequency_Hertz_t[i+2] = Larmor_frequency_Hertz_t[i+1] + step_frequency_Hertz;
                printf("Augmentation de la fréquence\t");
 
            }
        }
        
        printf("New Larmor Frequency = %f\n",Larmor_frequency_Hertz_t[i+2]);
        
        /// Recalcul des paramètres en fonction des nouvelles fréquences
        excitation_burst_cycles_tot = Larmor_frequency_Hertz_t[i+2] * excitation_duration_seconds;
        printf("New nombre des periodes dans le burst %d\n",excitation_burst_cycles_tot);


        usleep(wait);

    }
    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    rp_DpinSetState(RP_LED0+1, RP_LOW);

    printf("Temps d'execution : %lf\n",time_spent);

    fclose(fichier);
    /* Releasing resources */
    rp_AcqAxiEnable(CH_ACQ, false);
    rp_Release();
    free(buff1);
    free(Larmor_frequency_Hertz_t);
    free(maximum_amplitude_t);
    return 0;
}
