/*
 *  AudioSuperclass.c
 *  Root class for audio synthesizers, for OSX or Linux
 *
 *  Made by Michael Krzyzaniak at Arizona State University's
 *  School of Arts, Media + Engineering in Spring of 2013
 *  mkrzyzan@asu.edu
 */



//#define RECORD_MODE 1


#include <math.h>
#include <stdio.h>
#include "AudioSuperclass.h"

const double AU_TWO_PI_OVER_SAMPLE_RATE = SIN_TWO_PI / AU_SAMPLE_RATE;

#if defined __APPLE__
void  auAudioOutputCallback(void*, AudioQueueRef, AudioQueueBufferRef);
void  auAudioInputCallback (void*, AudioQueueRef, AudioQueueBufferRef, const AudioTimeStamp*, UInt32, const AudioStreamPacketDescription*);

#elif defined __linux__
void* auAudioCallback(void* SELF);
#endif


/*OpaqueAudioStruct----------------------------------------*/
struct OpaqueAudioStruct
{
  AUDIO_GUTS;
};

/*auNew---------------------------------------------------*/
Audio* auAlloc(int sizeofstarself, auAudioCallback_t callback, BOOL isOutput, unsigned numChannels)
{
  Audio* self = (Audio*)calloc(1, sizeofstarself);

  if(self != NULL)
    {
      self->isOutput                     = isOutput;
      self->isPlaying                    = NO;
      self->audioCallback                = callback;
      self->numChannels                  = numChannels;
      self->targetMasterVolume           = 1;
      self->destroy                      = auDestroy;
      auSetMasterVolumeSmoothing(self, 0.9999);
      
#if defined __APPLE__
      int i;
      OSStatus error;
      
      self->dataFormat.mSampleRate       = AU_SAMPLE_RATE;
      self->dataFormat.mBytesPerPacket   = 4 * numChannels  ;
      self->dataFormat.mFramesPerPacket  = 1             ;
      self->dataFormat.mBytesPerFrame    = 4 * numChannels  ;
      self->dataFormat.mBitsPerChannel   = 32            ;
      self->dataFormat.mChannelsPerFrame = numChannels      ;
      self->dataFormat.mFormatID         = kAudioFormatLinearPCM;
      //self->dataFormat.mFormatFlags    = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
      self->dataFormat.mFormatFlags      = kLinearPCMFormatFlagIsFloat;
      
      if(isOutput)
        error = AudioQueueNewOutput(&(self->dataFormat), auAudioOutputCallback, self, NULL, NULL, 0, &(self->queue));
      else
        error = AudioQueueNewInput (&(self->dataFormat), auAudioInputCallback, self, NULL, NULL, 0, &(self->queue));
      if(error) 
        {
          fprintf(stderr, "Audio.c: unable to allocate Audio queue\n"); 
          return auDestroy(self);
        }
      else //(!error)
        {
          unsigned bufferNumBytes = AU_BUFFER_NUM_FRAMES * numChannels * sizeof(auSample_t);
          for(i=0; i<AU_NUM_AUDIO_BUFFERS; i++)
            {
              error = AudioQueueAllocateBuffer(self->queue, bufferNumBytes, &((self->buffers)[i]));
              if(error) 
                {
                  self = auDestroy(self);
                  fprintf(stderr, "Audio.c: allocate buffer error\n");
                  break;
                }
            }
        }

#elif defined __linux__
      int error = 0;

      snd_pcm_hw_params_t  *hardwareParameters;
      snd_pcm_hw_params_alloca(&hardwareParameters);

      //untested for input stream...
      const char* name = (isOutput) ? AU_SPEAKER_DEVICE_NAME : AU_MIC_DEVICE_NAME;
      unsigned direction = (isOutput) ? SND_PCM_STREAM_PLAYBACK : SND_PCM_STREAM_CAPTURE;
      
      error = snd_pcm_open(&(self->device), name, direction, 0);
      if(error < 0) fprintf(stderr, "Audio.c: Unable to open speaker device %s: %s\n", AU_SPEAKER_DEVICE_NAME, snd_strerror(error));
            
      if(error >= 0)
        {
          error = snd_pcm_hw_params_any(self->device, hardwareParameters);
            if(error < 0) fprintf(stderr, "Audio.c: Unable to get a generic hardware configuration: %s\n", snd_strerror(error));
        }
      if(error >= 0)
        {
          error = snd_pcm_hw_params_set_access(self->device, hardwareParameters, SND_PCM_ACCESS_RW_INTERLEAVED);
          if(error < 0) fprintf(stderr, "Audio.c: Device does not support interleaved audio access: %s\n", snd_strerror(error));
        }
      if(error >= 0)
        {
          error = snd_pcm_hw_params_set_format(self->device, hardwareParameters, /*SND_PCM_FORMAT_S16*/ SND_PCM_FORMAT_FLOAT) ;
          if(error < 0) fprintf(stderr, "Audio.c: Unable to set sample format: %s\n", snd_strerror(error));
        }
      if(error >= 0)
        {
          //self->numChannels = AU_NUM_CHANNELS;
          error = snd_pcm_hw_params_set_channels_near(self->device, hardwareParameters, &self->numChannels);
          if(error < 0) fprintf(stderr, "Audio.c: unable to set the number of channels to %i: %s\n", self->numChannels, snd_strerror(error));
          else if(self->numChannels != numChannels)
            fprintf(stderr, "Audio.c: device does not support %u numChannels, %i will be used instead\n", numChannels, self->numChannels);  
        }
      if(error >= 0)
        {
          unsigned int sampleRate = AU_SAMPLE_RATE;
          error = snd_pcm_hw_params_set_rate_near(self->device, hardwareParameters, &sampleRate, 0);
          if(error < 0) fprintf(stderr, "Audio.c: Unable to set the sample rate to %u: %s\n", sampleRate, snd_strerror(error));
          else if(sampleRate != AU_SAMPLE_RATE)
            fprintf(stderr, "Audio.c: device does not support %i sample rate, %u will be used instead\n", (int)AU_SAMPLE_RATE, sampleRate);
        }
      if(error >= 0)
        {
          int dir = 0;
          self->bufferNumFrames = AU_BUFFER_NUM_FRAMES; //the buffer I give to ALSA
          error = snd_pcm_hw_params_set_period_size_near(self->device, hardwareParameters, &(self->bufferNumFrames), &dir);
          if(error < 0) fprintf(stderr, "Audio.cpp: Unable to set the sample buffer size to %lu: %s\n", self->bufferNumFrames, snd_strerror(error));
          else if(self->bufferNumFrames != AU_BUFFER_NUM_FRAMES)
            fprintf(stderr, "Audio.c: device does not support %i period size, %lu will be used instead\n", AU_BUFFER_NUM_FRAMES, self->bufferNumFrames);
        }
      if(error >= 0)
        {
          unsigned long int internalBufferNumFrames = self->bufferNumFrames * AU_NUM_AUDIO_BUFFERS; //the buffer ALSA uses internally
          error = snd_pcm_hw_params_set_buffer_size_near(self->device, hardwareParameters, &internalBufferNumFrames);
          if(error < 0) fprintf(stderr, "Audio.c: Unable to set the internal buffer size to %lu: %s\n", internalBufferNumFrames, snd_strerror(error));
          else if(internalBufferNumFrames != AU_NUM_AUDIO_BUFFERS * self->bufferNumFrames)
              fprintf(stderr, "Audio.c: device does not support %lu internal buffer size, %lu will be used instead\n", AU_NUM_AUDIO_BUFFERS * self->bufferNumFrames, internalBufferNumFrames);
        }
      if(error >= 0)
        {
          error = snd_pcm_hw_params(self->device, hardwareParameters);
          if(error < 0) fprintf(stderr, "Audio.c: Unable to load the hardware parameters into the device: %s\n", snd_strerror(error));
        }
      if(error >= 0)
       {
         unsigned int size = sizeof(auSample_t) * self->numChannels * self->bufferNumFrames;
         self->sampleBuffer = (auSample_t*)malloc(size);
         if(self->sampleBuffer == NULL)
           {
              error = -1;
              perror("Audio.c: Unable to allocate audio buffers \n");
           }
       }
      if (error < 0) self = auDestroy(self);
#endif
    }
  else perror("Audio.c: Unable to create new Audio object");
  
  srandom((unsigned)time(NULL));

  return self;
}

/*auDestroy-----------------------------------------------*/
Audio* auDestroy(Audio* self)
{
  if(self != NULL)
    {
      if(self->isPlaying) auPause(self);

#if defined __APPLE__
      int i;
      for(i=0; i<AU_NUM_AUDIO_BUFFERS; i++)
        if(self->buffers[i] != NULL)
          AudioQueueFreeBuffer(self->queue, self->buffers[i]);
          
      if(self->queue != NULL)
        AudioQueueDispose(self->queue, YES);

#elif defined __linux__
      if(self->device != NULL)
        {
          snd_pcm_close(self->device);
          self->device = NULL;
        }
      if(self->sampleBuffer)
        free(self->sampleBuffer);
      //if(self->thread != NULL)
        //pthread_detach(self->thread);
#endif

      free(self);
    }
  return (Audio*)NULL;
}

/*auDestroy-----------------------------------------------*/
Audio*  auSubclassDestroy    (Audio* self)
{
  Audio* result = NULL;
  
  if(self != NULL)
    result = self->destroy(self);
  
  return result;
}

/*auStart-------------------------------------------------*/
BOOL auPlay(Audio* self)
{ 
  if(!self->isPlaying)
    {

#ifdef __APPLE__
      int i;
      for(i=0; i<AU_NUM_AUDIO_BUFFERS; i++)
        {
          if(self->isOutput)
            auAudioOutputCallback(self, self->queue, self->buffers[i]);
          else
            AudioQueueEnqueueBuffer(self->queue, self->buffers[i],0, NULL);
        }
      OSStatus error = AudioQueueStart(self->queue, NULL);
      if(error) fprintf(stderr, "Audio.c: unable to start queue\n");
    
#elif defined __linux__
      int error = pthread_create(&(self->thread), NULL, auAudioCallback, self);
      if(error != 0) perror("Audio.c: error creating Audio thread");

#endif
      else self->isPlaying = YES;
    }
    
  return self->isPlaying;
}

/*auPause--------------------------------------------------*/
BOOL auPause(Audio* self)
{
  
  if(self->isPlaying)
    {

#ifdef __APPLE__
      OSStatus error = AudioQueueFlush(self->queue);
      if(!error)
        error = AudioQueueStop (self->queue, YES);
      self->isPlaying = (error != 0);

#elif defined __linux__
      self->threadShouldContinueRunning = NO;
      if(self->threadIsRunning)
        pthread_join(self->thread, NULL);
      self->isPlaying = NO;
      
#endif
    }
  return self->isPlaying;
}

/*auTogglePlayPause---------------------------------------*/
BOOL    auTogglePlayPause     (Audio* self)
{
  return (self->isPlaying) ? auPause(self) : auPlay(self);
}

/*auMasterVolume------------------------------------------*/
float  auMasterVolume   (Audio* self)
{
  return sqrt(self->targetMasterVolume);
}

/*auSetMasterVolume---------------------------------------*/
void    auSetMasterVolume(Audio* self, float volume)
{
  if(volume > 1) volume = 1;
  if(volume < 0) volume = 0;
  volume *= volume;
  self->targetMasterVolume = volume;
}

/*auSetMasterVolumeSmoothing-------------------------------*/
void auSetMasterVolumeSmoothing(Audio* self, float smoothing)
{
  //strange, I know.
  self->masterVolumeSmoothing         = 1 - smoothing;
  self->oneMinusMasterVolumeSmoothing = smoothing;
}

/*auMasterVolumeSmoothing----------------------------------*/
float   auMasterVolumeSmoothing(Audio* self)
{
  return self->oneMinusMasterVolumeSmoothing;
}

/*auisPlaying----------------------------------------------*/
BOOL    auIsPlaying      (Audio* self)
{
  return self->isPlaying;
}

/*auPlayFile------------------------------------------------*/
/*
void    auPlayFile       (Audio* self, const char* path)
{
  char* command;
  asprintf(&command, "afplay %s&", path);
  system(command);
}
*/

/*auDeinterleave-------------------------------------------*/
//if buffer is big, use calloc instead for temp
//there is probably a better way that uses less space...
void    auDeinterleave       (auSample_t* buffer, int numFrames, int numChannels)
{
  if(numChannels <= 1) return;
  
  int i, j;
  auSample_t  temp[numFrames * numChannels];
  auSample_t* t=temp;
  memcpy(temp, buffer, numFrames * numChannels * sizeof(auSample_t));
  
  for(i=0; i<numFrames; i++)
    for(j=0; j<numChannels; j++)
      buffer[i + numFrames*j] = *t++;
}

/*auAudioCallback-------------------------------------------*/
#if defined __APPLE__

void  auAudioOutputCallback(void *SELF, AudioQueueRef queue, AudioQueueBufferRef buffer)
{
  Audio* self = (Audio*) SELF;
  int resultFrames = self->audioCallback(SELF, (auSample_t*)buffer->mAudioData, (buffer->mAudioDataBytesCapacity / self->dataFormat.mBytesPerFrame), self->dataFormat.mChannelsPerFrame);
  buffer->mAudioDataByteSize = resultFrames * self->dataFormat.mBytesPerFrame;//buffer->mAudioDataBytesCapacity;
  AudioQueueEnqueueBuffer(queue, buffer, 0, NULL);
}

void auAudioInputCallback(
   void                                *SELF,
   AudioQueueRef                       queue,
   AudioQueueBufferRef                 buffer,
   const AudioTimeStamp                *startTime,
   UInt32                              inNumberPacketDescriptions,
   const AudioStreamPacketDescription  *inPacketDescs
)
{
  Audio* self = (Audio*) SELF;

  if(startTime->mFlags & kAudioTimeStampHostTimeValid)
    {
      self->buffer_timestamp_is_supported = YES;
      self->current_buffer_timestamp = startTime->mHostTime / 1000;
    }
  else  self->buffer_timestamp_is_supported = NO;
  
  auAudioOutputCallback(SELF, queue, buffer);
}

#elif defined __linux__
BOOL auTransferData(Audio* self, snd_pcm_sframes_t (*transfer)(snd_pcm_t*, void*, snd_pcm_uframes_t));

void* auAudioCallback(void* SELF)
{
  Audio* self = (Audio*)SELF;
  self->threadIsRunning = self->threadShouldContinueRunning = YES;
  signal(SIGPIPE, SIG_IGN);
  int success = YES;
  
  while(self->threadShouldContinueRunning)
    {
      if(!self->isOutput) 
        success = auTransferData(self, snd_pcm_readi);
        
      if(success)
        self->audioCallback(SELF, self->sampleBuffer, self->bufferNumFrames, self->numChannels);
      
      if(self->isOutput) 
        success = auTransferData(self, (snd_pcm_sframes_t (*)(snd_pcm_t*, void*, snd_pcm_uframes_t)) snd_pcm_writei);

      if(!success) break;
    }
  self->threadIsRunning = NO;
  return NULL;
}

BOOL auTransferData(Audio* self, snd_pcm_sframes_t (*transfer)(snd_pcm_t*, void*, snd_pcm_uframes_t))
{
  int    numFramesTransferred = 0, error = 0;
  int    numFramesLeft        = self->bufferNumFrames;
  auSample_t*    p            = self->sampleBuffer;

   while((numFramesLeft > 0) && self->threadShouldContinueRunning)
     {
       error = numFramesTransferred = transfer(self->device, p, numFramesLeft);

       if(numFramesTransferred < 0)
         {  
           //fprintf(stderr, "Audio.c: audio device error while transferring samples: %s, attempting to recover... ", snd_strerror(error));
           switch(error)
            {
              case -EPIPE:   //overflow / underflow
                snd_pcm_wait(self->device, 100);
                if((error = snd_pcm_avail(self->device)) < 0)          //broken pipe
                  usleep(10000);                                       //wait for more samples to come
                else numFramesLeft = 0;                                //overrun, skip remaining samples;

                error = snd_pcm_prepare(self->device); 
                break;
             
             case -ESTRPIPE: 
              while(((error = snd_pcm_resume(self->device)) == -EAGAIN) && self->threadShouldContinueRunning) 
                sleep(1);
              if(error == -ENOSYS) error = snd_pcm_prepare(self->device); 
              break;
             
           }
           if(error < 0)
             {
               //fprintf(stderr, "Aborting\n");
               self->threadShouldContinueRunning = NO;
               break;
             }
           else
             {
               //fprintf(stderr, "Okay\n");
               numFramesTransferred = 0;
             } 
         }
        p +=  numFramesTransferred * self->numChannels;
        numFramesLeft -= numFramesTransferred;
     }
  return (numFramesLeft == 0) ? YES : NO;     
}

#endif
