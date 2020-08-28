// PortRecord V0.7
// MUST HAVE LIBSNDFILE AND PORTAUDIO INSTALLED
// IF CAPTURING SYSTEM AUDIO ON MAC OS, RECOMMEND USING SOUNDFLOWER:
// https://github.com/mattingalls/Soundflower
// Created by Rob Salino on 12/7/19
// Many aspects borrowed from helloring.c in The Audio Programming Book
// by Richard Boulanger and Victor Lazzarini, ISBN: 978-0-262-01446-5
// Aid in getting PortAudio to share data with libsndfile by Steve Philbert
//
// run with: cc -o portrecord Port_Record.c -lsndfile -lportaudio

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "portaudio.h"
#include <sndfile.h>
#include <string.h>
#include <unistd.h>

#define FRAME_BLOCK_LEN 256
#define SAMPLING_RATE 44100
#define MAX_LEN 128

static int channel_num;
static int bitdepth;

void print_image(FILE *fptr);

typedef struct _callbackuserdata
{
    SNDFILE *outfile; //only really need output
    int readcountTotal;
    int writecountTotal;
} Callbackuserdata;

static int audio_callback( const void *inputBuffer, void *outputBuffer,
                    unsigned long framesPerBuffer,
                    const PaStreamCallbackTimeInfo* timeInfo,
                    PaStreamCallbackFlags statusFlags,
                    void *userData)
{
    float *in = (float*) inputBuffer, *out = (float*) outputBuffer;
    long nsamples;
    nsamples = channel_num * framesPerBuffer;
    Callbackuserdata *myConfig = (Callbackuserdata *) userData;
    int writecount = sf_write_float(myConfig->outfile, in, nsamples);
    myConfig->writecountTotal += writecount;
    //If desired, can enable the next line to see all of the blocks writing to the file in terminal:
    //printf("Writing block of length: %d for %d channel(s)\n", writecount, channel_num);
    return 0;
}


enum {arg_name, arg_output, args};
int main(int argc, char *argv[])
{
    PaError err = paNoError;
    PaStream *audioStream;
    FILE* fo;
    fo = fopen(argv[arg_output],"wb"); //output file
    char * infilename;         // input file name
    char * outfilename;        // output file name
    SNDFILE * infile = NULL ;  // input sound file pointer
    SNDFILE * outfile = NULL;  // output sound file pointer
    SF_INFO sfinfoIN, sfinfoOUT;
    const PaDeviceInfo  *info;
    const PaHostApiInfo *hostapi;
    PaStreamParameters outputParameters, inputParameters;
    int i, id;
    
    memset(&sfinfoOUT, 0, sizeof(SF_INFO));  // clear sfinfo

    Callbackuserdata config;

    outfilename = argv[arg_output];

    if(argc != args) //if number of arguments entered isn't accurate, gives how to use program
    {
        printf("** Port Record! **\nUsage: ./recorder outfilename.wav\n\n");
        return 1;
    }
    
    //title text
    char *filename = "title_text.txt";
       FILE *fptr = NULL;
    
    if((fptr = fopen(filename,"r")) == NULL)
    {
        fprintf(stderr,"Could not load the beautiful %s\n",filename);
        
    }
    else
    {
        print_image(fptr);
    }
    fclose(fptr);

    printf("\n\n");
    sleep(2);
    
    printf("Please enter the bit depth (16, 24, 32): ");
    scanf("%d", &bitdepth);

    if(bitdepth == 16 || bitdepth == 24 || bitdepth == 32)
    {
        if(bitdepth == 32)
        {
            bitdepth = SF_FORMAT_FLOAT;
        }
        if(bitdepth == 24)
        {
            bitdepth = SF_FORMAT_PCM_24;
        }
        if(bitdepth == 16)
        {
            bitdepth = SF_FORMAT_PCM_16;
        }
        sfinfoOUT.format = SF_FORMAT_WAV | bitdepth;
        //%printf("Okay, %d channels.\n", sfinfo.channels); //check to see if number is correct
        //return 0;
    }
    else
    {
        printf("Invalid bit depth value, must be 16, 24, or 32.\n\n");
        return 0;
    }
    
    sfinfoOUT.samplerate = SAMPLING_RATE;
    
    Pa_Initialize();     //initialize portaudio
    
    
    for (i=0;i < Pa_GetDeviceCount(); i++)
    {
        info = Pa_GetDeviceInfo(i);         /* get information from current device */
        hostapi = Pa_GetHostApiInfo(info->hostApi); /*get info from curr. host API */

        if (info->maxInputChannels > 0)           /* if curr device supports input */
        {
            printf("%d: [%s]  %s (input); Max input channels: %d \n",i, hostapi->name, info->name , info->maxInputChannels);
        }
    }
    
    printf("\nType AUDIO input device number (from above list): ");
    scanf("%d", &id);                     /* get the input device id from the user */
    info = Pa_GetDeviceInfo(id);        /* get chosen device information structure */
    hostapi = Pa_GetHostApiInfo(info->hostApi);          /* get host API structure */
    printf("Opening AUDIO input device [%s] %s\n\n", hostapi->name, info->name);

    printf("Please enter the number of channels to be recorded (at most number of max channels from input): ");
    scanf("%d", &channel_num);
    printf("\n");
    if(channel_num <= info->maxInputChannels)
    {
        sfinfoOUT.channels = channel_num;
        //%printf("Okay, %d channels.\n", sfinfo.channels); //check to see if number is correct
        //return 0;
    }
    else if (channel_num > info->maxInputChannels)
    {
        printf("Invalid number of channels, must be equal to or fewer than input.\n\n");
        return 0;
    }
    
    
    
    inputParameters.device = id;                               /* chosen device id */
    inputParameters.channelCount = channel_num;
    outputParameters.channelCount = channel_num;
    inputParameters.sampleFormat = paFloat32;      /* 32 bit floating point input */
    outputParameters.sampleFormat = paFloat32;      /* 32 bit floating point output */
    inputParameters.suggestedLatency = info->defaultLowInputLatency; /*set default */
    inputParameters.hostApiSpecificStreamInfo = NULL;          /* no specific info */
    
    if (!sf_format_check(&sfinfoOUT))
    {
        sf_close(outfile) ;
        printf ("Invalid encoding\n") ;
        return 1;
    }
    
       if((outfile = sf_open(argv[arg_output], SFM_WRITE, &sfinfoOUT)) == NULL)
       {
            printf("Not able to open output file %s.\n", argv[arg_output]);
        }
    
    config.outfile = outfile;
    printf("channels: %d\n", inputParameters.channelCount);
    printf("channels: %d\n", sfinfoOUT.channels);

    err = Pa_OpenStream(               /* open the PaStream object and get its address */
              &audioStream,      /* get the address of the portaudio stream object */
              &inputParameters,  /* provide input parameters */
              NULL,              /* provide output parameters */
              SAMPLING_RATE,     /* set sampling rate */
              FRAME_BLOCK_LEN,   /* set frames per buffer */
              paClipOff,         /* set no clip */
              audio_callback,    /* provide the callback function address */
              &config );            /* provide data for the callback */
    
    printf("%d\n",err);
    printf(  "PortAudio error: %s\n", Pa_GetErrorText( err ) );
    Pa_StartStream(audioStream); /* start the callback mechanism */

    printf("running... press space bar and enter to exit\n");
    
    while(getchar() != ' ') Pa_Sleep(10);
    {
        Pa_StopStream( audioStream );    /* stop the callback mechanism */
        Pa_CloseStream( audioStream );   /* destroy the audio stream object */
        Pa_Terminate();
    }
    
}
void print_image(FILE *fptr)
{
    char read_string[MAX_LEN];
 
    while(fgets(read_string,sizeof(read_string),fptr) != NULL)
        printf("%s",read_string);
}
