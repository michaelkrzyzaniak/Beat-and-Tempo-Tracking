/*
 *  Synth.h
 *  Make weird noises
 *
 *  Made by Michael Krzyzaniak at Arizona State University's
 *  School of Arts, Media + Engineering in Spring of 2013
 *  mkrzyzan@asu.edu
 */

#include "Microphone.h"
#include "../../OBTAIN.h"

void mic_onset_detected_callback(void* SELF, unsigned long long sample_time);
void mic_beat_detected_callback (void* SELF, unsigned long long sample_time);

/*--------------------------------------------------------------------*/
struct OpaqueMicrophoneStruct
{
  AUDIO_GUTS                  ;
  //MKAiff*                      audio_file ;
  Obtain* obtain;
};

/*--------------------------------------------------------------------*/
int mic_audio_callback         (void* SELF, auSample_t* buffer, int num_frames, int num_channels);

Microphone* mic_destroy       (Microphone* self);


/*--------------------------------------------------------------------*/
Microphone* mic_new()
{
  Microphone* self = (Microphone*) auAlloc(sizeof(*self), mic_audio_callback, NO, 1);
  
  if(self != NULL)
    {
      self->destroy = (Audio* (*)(Audio*))mic_destroy;
      self->obtain = obtain_new(OBTAIN_SUGGESTED_SPECTRAL_FLUX_STFT_LEN,
                                OBTAIN_SUGGESTED_SPECTRAL_FLUX_STFT_OVERLAP,
                                OBTAIN_SUGGESTED_OSS_FILTER_ORDER,
                                OBTAIN_SUGGESTED_OSS_LENGTH,
                                OBTAIN_SUGGESTED_ONSET_THRESHOLD_N,
                                OBTAIN_SUGGESTED_CBSS_LENGTH,
                                OBTAIN_SUGGESTED_SAMPLE_RATE);
      if(self->obtain == NULL)
        return (Microphone*)auDestroy((Audio*)self);
    
      obtain_set_onset_tracking_callback  (self->obtain, mic_onset_detected_callback, self);
      obtain_set_beat_tracking_callback   (self->obtain, mic_beat_detected_callback , self);
      //self->audio_file = aiffWithDurationInSeconds(num_channels, AU_SAMPLE_RATE, 16, expected_seconds + 5);
      //if(self->audio_file == NULL)
        //return (Microphone*)auDestroy((Audio*)self);
    }
  return self;
}

/*--------------------------------------------------------------------*/
void mic_onset_detected_callback(void* SELF, unsigned long long sample_time)
{
  Microphone* self = (Microphone*) SELF;

}

/*--------------------------------------------------------------------*/
/* store each beat as a single click in an AIFF file so we can compare it to the original */
void mic_beat_detected_callback (void* SELF, unsigned long long sample_time)
{
  Microphone* self = (Microphone*) SELF;
  //fprintf(stderr, "beat\r\n");
  putc(0x07, stderr);
}


/*--------------------------------------------------------------------*/
/*
MKAiff* mic_get_recording(Microphone* self)
{
  return self->audio_file;
}
*/

/*--------------------------------------------------------------------*/
Microphone* mic_destroy(Microphone* self)
{
  if(self != NULL)
    {
      //self->audio_file = aiffDestroy(self->audio_file);
    }
    
  return (Microphone*) NULL;
}

/*--------------------------------------------------------------------*/
int mic_audio_callback(void* SELF, auSample_t* buffer, int num_frames, int num_channels)
{
  Microphone* self = (Microphone*)SELF;
  //if(!self->buffer_timestamp_is_supported) return 0; //extra invalid buffers come after stopping
  
  obtain_process(self->obtain, buffer, num_frames);
  
  return  num_frames;
  //return aiffAddFloatingPointSamplesAtPlayhead(self->audio_file, buffer, num_frames*num_channels, aiffFloatSampleType, aiffYes);
}

