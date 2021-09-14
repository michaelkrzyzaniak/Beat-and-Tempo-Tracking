//compile with:
//gcc *.c ../../src/*.c ../offline/MKAiff.c

#include "../../BTT.h"
#include "../offline/MKAiff.h"

#define AUDIO_BUFFER_SIZE 64

/*--------------------------------------------------------------------*/
int main(int argc, char* argv[])
{
  if(argc < 2)
    {
      fprintf(stderr, "Please specify an aiff or wav file you would like to process.\r\n"); exit(-1);
    }
  
  /*open the audio file, make it mono, and check sample rate*/
  MKAiff* aiff = aiffWithContentsOfFile(argv[1]);
  aiffMakeMono(aiff);
  double sample_rate = aiffSampleRate(aiff);
  if(sample_rate != BTT_SUGGESTED_SAMPLE_RATE)
    {fprintf(stderr, "Audio file should be %lf Hz but is %lf. Aborting.\r\n", (double)BTT_SUGGESTED_SAMPLE_RATE, sample_rate); exit(-1);}
  
  /*get an audio buffer*/
  dft_sample_t* buffer = calloc(AUDIO_BUFFER_SIZE, sizeof(*buffer));
  if(buffer == NULL){perror("unable to allocate buffer"); exit(-3);}
  
  /*make beat tracking object*/
  BTT* btt =  btt_new_default();
  if(btt == NULL){perror("unable to create btt object"); exit(-2);}
  
  
  /*This will save a little CPU since we jst want the tempo estimate and don't care about the exact location of each beat*/
  btt_set_tracking_mode(btt, BTT_ONSET_AND_TEMPO_TRACKING);
  
  /* don't decay the histogram just assume constant tempo and accumulate estimates over the whole song */
  btt_set_gaussian_tempo_histogram_decay(btt, 1);
  
  for(;;)
    {
      int num_samples = aiffReadFloatingPointSamplesAtPlayhead(aiff, buffer, AUDIO_BUFFER_SIZE, aiffYes);
      btt_process(btt, buffer, num_samples);
      if(num_samples < AUDIO_BUFFER_SIZE)  break;
    }
  
  double tempo = btt_get_tempo_bpm(btt);
  fprintf(stderr, "Tempo of %s is %02f\r\n", argv[1], tempo);
  
  /*cleanup*/
  btt_destroy(btt);
  aiffDestroy(aiff);
}

