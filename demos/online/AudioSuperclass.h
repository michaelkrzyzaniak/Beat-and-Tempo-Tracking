/*
 *  AudioSuperclass.h
 *  Root class for audio synthesizers, for OSX or Linux
 *
 *  Made by Michael Krzyzaniak at Arizona State University's
 *  School of Arts, Media + Engineering in Spring of 2013
 *  mkrzyzan@asu.edu
 */


#ifndef __AUDIO_SUPERCLASS__
#define __AUDIO_SUPERCLASS__ 1

#if defined(__cplusplus)
extern "C"{
#endif   //(__cplusplus)

#if defined __APPLE__
#include <AudioToolbox/AudioToolbox.h>
#elif defined __linux__
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <alsa/asoundlib.h>
#endif

//#include "MKAiff.h"
//#include "fastsin.h"
#define SIN_TWO_PI 0xFFFFFFFF

#ifndef BOOL
#define BOOL int
#endif
#ifndef NO
#define NO   0
#define YES  (!NO)
#endif

#define AU_NUM_AUDIO_BUFFERS  4
#define AU_BUFFER_NUM_FRAMES  512
#define AU_SAMPLE_RATE        44100.0
//#define AU_SAMPLE_RATE        192000.0

typedef  float auSample_t;
typedef  int (*auAudioCallback_t) (void* SELF, auSample_t* buffer, int numFrames, int numChannels);


#if defined __APPLE__
#define AUDIO_PLATFORM_SPECIFIC_GUTS                          \
  AudioQueueRef               queue                         ; \
  AudioQueueBufferRef         buffers[AU_NUM_AUDIO_BUFFERS] ; \
  AudioStreamBasicDescription dataFormat                    ; \
  
#elif defined __linux__                                    
#define AUDIO_PLATFORM_SPECIFIC_GUTS                          \
  snd_pcm_t                  *device                        ; \
  auSample_t                 *sampleBuffer                  ; \
  pthread_t                   thread                        ; \
  int                         threadIsRunning               ; \
  int                         threadShouldContinueRunning   ; \
  unsigned long int           bufferNumFrames               ;
  
#define AU_SPEAKER_DEVICE_NAME "default"  
#define AU_MIC_DEVICE_NAME     "default"  
#endif                                                        

#define AUDIO_GUTS                                            \
  AUDIO_PLATFORM_SPECIFIC_GUTS                                \
  unsigned                    numChannels                   ; \
  BOOL                        isOutput                      ; \
  BOOL                        isPlaying                     ; \
  float                       masterVolume                  ; \
  float                       targetMasterVolume            ; \
  float                       masterVolumeSmoothing         ; \
  float                       oneMinusMasterVolumeSmoothing ; \
  auAudioCallback_t           audioCallback                 ; \
  uint64_t                    current_buffer_timestamp      ; \
  Audio*                      (*destroy)(Audio*)            ; \
  BOOL                        buffer_timestamp_is_supported ; // system uptime in usec
  
typedef struct OpaqueAudioStruct Audio;


const extern double AU_TWO_PI_OVER_SAMPLE_RATE;

#define AU_RANDOM(min, max)  (((random() / (long double)RAND_MAX)) * ((max) - (min)) + (min))
#define AU_MIDI2CPS(x)   (440 * pow(2, ((x)-69) / 12.0))
#define AU_MIDI2FREQ(x)  (AU_MIDI2CPS(x) * SIN_TWO_PI / (float)AU_SAMPLE_RATE)
#define AU_CPS2MIDI(x)   (69 + 12.0 * log2((x) / 440.0))
#define AU_FREQ2MIDI(x)  AU_CPS2MIDI(x) / (SIN_TWO_PI / (float)AU_SAMPLE_RATE);
#define AU_CONSTRAIN(x, MIN, MAX) ((x) = ((x) < (MIN)) ? (MIN) : ((x) > (MAX)) ? (MAX) : (x))



/* Subcalsses call these in their own Alloc / Destroy */
/* routines. Users call the subclasses' ruotines,     */
/* not these.                                         */
Audio*  auAlloc              (int sizeofstarself, auAudioCallback_t callback, BOOL isOutput, unsigned numChannels);
Audio*  auDestroy            (Audio* self);

/* this is the destroy routine for all subclasses. It just follows a pointer to the correct */
/* destroy routine */
Audio*  auSubclassDestroy    (Audio* self);

/* cast subcalsses to (Audio* and call these)         */
BOOL    auPlay               (Audio* self);
BOOL    auPause              (Audio* self);
BOOL    auTogglePlayPause    (Audio* self);
BOOL    auIsPlaying          (Audio* self);
float   auMasterVolume       (Audio* self);
void    auSetMasterVolume    (Audio* self, float volume);
void    auSetMasterVolumeSmoothing(Audio* self, float smoothing);
float   auMasterVolumeSmoothing(Audio* self);
void    auDeinterleave       (auSample_t* buffer, int numFrames, int numChannels);

#if defined(__cplusplus)
}
#endif   //(__cplusplus)

#endif// __AUDIO_SUPERCLASS__
