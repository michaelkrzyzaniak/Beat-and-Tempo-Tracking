/*
 *  Synth.h
 *  Make weird noises
 *
 *  Made by Michael Krzyzaniak at Arizona State University's
 *  School of Arts, Media + Engineering in Spring of 2013
 *  mkrzyzan@asu.edu
 */

#include "Microphone.h"
#include "Click.h"

void mic_onset_detected_callback(void* SELF, unsigned long long sample_time);
void mic_beat_detected_callback (void* SELF, unsigned long long sample_time);

/*--------------------------------------------------------------------*/
struct OpaqueMicrophoneStruct
{
  AUDIO_GUTS            ;
  BTT* btt              ;
  Click*  click         ;
};

/*--------------------------------------------------------------------*/
int         mic_audio_callback  (void* SELF, auSample_t* buffer, int num_frames, int num_channels);
Microphone* mic_destroy         (Microphone* self);

/*--------------------------------------------------------------------*/
Microphone* mic_new()
{
  Microphone* self = (Microphone*) auAlloc(sizeof(*self), mic_audio_callback, NO, 2);
  
  if(self != NULL)
    {
      self->destroy = (Audio* (*)(Audio*))mic_destroy;
    
      self->click = click_new();
      if(self->click == NULL)
        return (Microphone*)auDestroy((Audio*)self);
    
      self->btt = btt_new_default();
      if(self->btt == NULL)
        return (Microphone*)auDestroy((Audio*)self);
    
      btt_set_onset_tracking_callback  (self->btt, mic_onset_detected_callback, self);
      btt_set_beat_tracking_callback   (self->btt, mic_beat_detected_callback , self);
    }
  
  //there should be a play callback that I can intercept and do this there.
  auPlay((Audio*)self->click);
  return self;
}

/*--------------------------------------------------------------------*/
void mic_onset_detected_callback(void* SELF, unsigned long long sample_time)
{
  Microphone* self = (Microphone*) SELF;
  if(btt_get_tracking_mode(self->btt) <= BTT_ONSET_TRACKING)
    {
      click_click(self->click);
      fprintf(stderr, "onset\r\n");
    }
}

/*--------------------------------------------------------------------*/
/* store each beat as a single click in an AIFF file so we can compare it to the original */
void mic_beat_detected_callback (void* SELF, unsigned long long sample_time)
{
  Microphone* self = (Microphone*) SELF;
  click_click(self->click);
  fprintf(stderr, "beat\r\n");
}

/*--------------------------------------------------------------------*/
Microphone* mic_destroy(Microphone* self)
{
  if(self != NULL)
    {
      btt_destroy(self->btt);
      auDestroy((Audio*)self->click);
    }
    
  return (Microphone*) NULL;
}

/*--------------------------------------------------------------------*/
BTT*           mic_get_btt        (Microphone* self)
{
  return self->btt;
}

/*--------------------------------------------------------------------*/
int mic_audio_callback(void* SELF, auSample_t* buffer, int num_frames, int num_channels)
{
  Microphone* self = (Microphone*)SELF;
  int frame, channel;
  
  //mix to mono without correcting amplitude
  for(frame=0; frame<num_frames; frame++)
    for(channel=1; channel<num_channels; channel++)
      buffer[frame] += buffer[frame * num_channels + channel];
  
  btt_process(self->btt, buffer, num_frames);
  return  num_frames;
}

