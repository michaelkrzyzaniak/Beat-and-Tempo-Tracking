/*
 *  Click.h
 *  Make weird noises
 *
 *  Made by Michael Krzyzaniak at Arizona State University's
 *  School of Arts, Media + Engineering in Spring of 2013
 *  mkrzyzan@asu.edu
 */

#include "Click.h"

/*--------------------------------------------------------------------*/
struct OpaqueClickStruct
{
  AUDIO_GUTS                  ;
  BOOL needs_click;
};

/*--------------------------------------------------------------------*/
int click_audio_callback         (void* SELF, auSample_t* buffer, int num_frames, int num_channels);

Click* click_destroy       (Click* self);


/*--------------------------------------------------------------------*/
Click* click_new()
{
  Click* self = (Click*) auAlloc(sizeof(*self), click_audio_callback, YES, 1);
  
  if(self != NULL)
    {
      self->destroy = (Audio* (*)(Audio*))click_destroy;
    }
  return self;
}


/*--------------------------------------------------------------------*/
Click* click_destroy(Click* self)
{
  if(self != NULL)
    {

    }
    
  return (Click*) NULL;
}

/*--------------------------------------------------------------------*/
void         click_click             (Click* self)
{
  self->needs_click = YES;
}

/*--------------------------------------------------------------------*/
int click_audio_callback(void* SELF, auSample_t* buffer, int num_frames, int num_channels)
{
  Click* self = (Click*)SELF;
  
  memset(buffer, 0, sizeof(*buffer) * num_frames * num_channels);
  
  if(self->needs_click)
    {
      buffer[0] = 1;
      self->needs_click = NO;
    }
  
  return  num_frames;
}

