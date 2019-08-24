/*_--__-_--__----_----_--__--__--_-------------------------------------------------------
|  \/  | |/ /   / \  (_)/ _|/ _|| |__  
| |\/| | ' /   / _ \ | | |_| |_ | '_ \ 
| |  | | . \  / ___ \| |  _|  _|| | | |
|_|  |_|_|\_\/_/   \_\_|_| |_|(_)_| |_|
                                        
read, manipuate and write aiff files.                             
---------------------------------------------------------------------------------------*/
#ifndef MK_AIFF
#define MK_AIFF 1


/*---------_---------_-------------------------------------------------------------------
(_)_ _  __| |_  _ __| |___ ___
| | ' \/ _| | || / _` / -_|_-<
|_|_||_\__|_|\_,_\__,_\___/__/
---------------------------------------------------------------------------------------*/
#include <stdlib.h>  //malloc() and calloc()
#include <stdio.h>   //FILE*, fread, fwrite
#include <string.h>  //srtrlen()
#include <stdint.h>  //uint8_t, uint16_t, uint32_t etc...


/*---------------------------------------------------------------------------------------
| |_ _  _ _ __  ___ ___
|  _| || | '_ \/ -_|_-<
 \__|\_, | .__/\___/__/
     |__/|_|           
 ---------------------------------------------------------------------------------------*/  
#if defined(__cplusplus)
extern "C"{
#endif   //(__cplusplus)

//---------------------------------------------------------------------------------------
/*  do not use anything in the 'types' section directly.                               */
/*  Use the functions below to create, manipulate and destroy.                         */

typedef enum
{
  aiffNo  = 0,
  aiffYes = 1,
}aiffYesOrNo_t;

typedef uint32_t aiffChunkIdentifier_t;
typedef unsigned char float80_t[10];
typedef unsigned char uint24_t[3];

//---------------------------------------------------------------------------------------
typedef struct aiff_dummy_list_cast
{
  void* previous;
  void* next;
}mkAiffListEntry_t;

typedef struct aiff_dummy_sizeable_list_cast
{
  void* previous;
  void* next;
  aiffChunkIdentifier_t chunkID;            
  int32_t               chunkSize;          
}mkAiffSizableListEntry_t;

//---------------------------------------------------------------------------------------
typedef struct
{
  aiffChunkIdentifier_t chunkID;            //not included in chunkSize
  int32_t               chunkSize;          //not included in chunkSize
  aiffChunkIdentifier_t formTypeID;
  //char                chunks[];           //sizeof(chunks) included in chunkSize
}mkAiffFormChunk_t;

//---------------------------------------------------------------------------------------
typedef struct
{
  aiffChunkIdentifier_t chunkID;
  int32_t               chunkSize;          // always 18
  int16_t               numChannels;
  uint32_t              numSampleFrames;    
  int16_t               bitsPerSample;      //(1-32), called, confusingly, sampleSize in the documentation
  float80_t             sampleRate; 
}mkAiffCommonChunk_t;

//---------------------------------------------------------------------------------------
typedef struct
{
  aiffChunkIdentifier_t chunkID;
  int32_t              chunkSize;          // always 16 for PCM
  uint16_t             audioFormat;        // 0x01 for PCM
  uint16_t             numChannels;
  uint32_t             sampleRate;
  uint32_t             byteRate;
  uint16_t             blockAlign;
  uint16_t             bitsPerSample;
}mkAiffWaveFormatChunk_t;


//---------------------------------------------------------------------------------------
typedef struct
{
  aiffChunkIdentifier_t chunkID;          //not included in chunkSize
  int32_t               chunkSize;        //not included in chunkSize
  uint32_t              offset;           //should normally be zero
  uint32_t              blockSize;        //should normally be zero
  //char                soundData[];      //sizeof(soundData) included in chunkSize
}mkAiffSoundChunk_t;

//---------------------------------------------------------------------------------------
typedef struct
{
  aiffChunkIdentifier_t chunkID;          //not included in chunkSize
  int32_t               chunkSize;        //not included in chunkSize
  //char                soundData[];      //sizeof(soundData) included in chunkSize
}mkAiffWaveDataChunk_t;

//---------------------------------------------------------------------------------------
typedef uint16_t mkAiffMarkerID_t;

typedef struct
{
  void*                 previous;           //not included in chunkSize
  void*                 next;               //not included in chunkSize
  mkAiffMarkerID_t      markerID;           //non-zero
  uint32_t              positionInFrames;
  char*                 name;               //pascal-string. One byte count (count byte not included in value of count byte) followed by chars.
}mkAiffMarker_t;                            //Must be padded to ensure total size of name is even

typedef struct
{
  aiffChunkIdentifier_t chunkID;              //not included in chunkSize
  int32_t               chunkSize;            //not included in chunkSize
  uint16_t              numMarkers; 
  //mkAiffMarker_t      markers[];            //included in chunk size;
  mkAiffMarker_t*       firstMarker;          //not part of specification       
}mkAiffMarkerChunk_t;

//---------------------------------------------------------------------------------------
#define                 AIFF_LOOP_MODE_OFF                  0
#define                 AIFF_LOOP_MODE_FORWARD              1
#define                 AIFF_LOOP_MODE_FORWARD_BACKWARD     2
  
typedef struct
{
  int16_t               playMode;                //one of the LOOP_MODEs defined above;
  mkAiffMarkerID_t      startMarkerID;
  mkAiffMarkerID_t      endMarkerID;
}mkAiffLoop_t;

typedef struct
{ 
  aiffChunkIdentifier_t chunkID;                 //not included in chunkSize
  int32_t               chunkSize;               //always 20, not included in chunkSize
  int8_t                baseNote;                //MIDI note number of sample
  int8_t                detune;                  //-50 to +50 cents
  int8_t                lowNote;                 //MIDI note number, suggested lowest playback pitch
  int8_t                highNote;                //MIDI note number, suggested highest playback pitch
  int8_t                lowVelocity;             //MIDI velocity, suggested highest playback velocity
  int8_t                highVelocity;            //MIDI velocity, suggested lowest playback velocity
  int16_t               decibelsGain;            //6 doubles, -6 halves;
  mkAiffLoop_t          sustainLoop;
  mkAiffLoop_t          releaseLoop;
}mkAiffInstrumentChunk_t;

//---------------------------------------------------------------------------------------
typedef struct
{
  aiffChunkIdentifier_t chunkID;                  //not included in chunkSize
  int32_t               chunkSize;                //not included in chunkSize
  //unsigned char       midiData[];               //included in chunkSize;
}mkAiffMIDIChunk_t;

//---------------------------------------------------------------------------------------
typedef struct
{
  aiffChunkIdentifier_t chunkID;    
  int32_t               chunkSize;                //always 24
  unsigned char         AESChannelStatusData[24]; //oh, dear me. 
}mkAiffRecordingChunk_t;

//---------------------------------------------------------------------------------------
typedef struct 
{ 
  aiffChunkIdentifier_t chunkID;                  //not included in chunkSize
  int32_t               chunkSize;                //not included in chunkSize
  uint32_t              applicationSignature;     //'pdos' for apple II 
  //char                data[];                   //included in chunk size
}mkAiffApplicationChunk_t; 

//---------------------------------------------------------------------------------------
typedef struct                                   //the first 2 elements of this struct must not be rearranged.
{       
  void*                 previous;                //not part of specification 
  void*                 next;                    //not part of specification 
  uint32_t              timeStamp;               //unix epoch of creation of comment
  mkAiffMarkerID_t      markerID;                //zero if not linked to a marker
  uint16_t              numChars;                //not including pad-byte if present
  char*                 text;                    //must be padded at end to ensure even num bytes. sizeof(*char) counted, sizeof(char) not.
}mkAiffComment_t;

typedef struct
{
  aiffChunkIdentifier_t chunkID;                  //not included in chunkSize
  int32_t               chunkSize;                //not included in chunkSize
  uint16_t              numComments;              //included in chunkSize
  //mkAiffComment_t     comments[];               //sizeof(comments) included in chunkSize
  mkAiffComment_t*      firstComment;             //not part of specification    
}mkAiffCommentChunk_t;

//---------------------------------------------------------------------------------------
typedef struct
{
  void*                 previous;                //not part of specification 
  void*                 next;                    //not part of specification 
  aiffChunkIdentifier_t chunkID;                 //one if the EAIFF85_CHUNK_IDs defined above. not included in chunkSize
  int32_t               chunkSize;               //not included in chunkSize  
  //char                text[];                  //sizeof(text) included in chunkSize  
  char*                 data;                    //not part of specification
}mkAiffGenericChunk_t;

typedef mkAiffGenericChunk_t  mkEAiff85TextChunk_t;

//---------------------------------------------------------------------------------------
typedef struct opaque_aiff_struct
{
  mkAiffCommonChunk_t          commonChunk;
  mkAiffSoundChunk_t           soundChunk;
  mkAiffMarkerChunk_t*         markerChunk;
  mkAiffInstrumentChunk_t*     instrumentChunk;
  mkAiffMIDIChunk_t*           midiChunkList;
  mkAiffRecordingChunk_t*      recordingChunk;
  mkAiffApplicationChunk_t*    applicationChunkList;
  mkAiffCommentChunk_t*        commentChunk;       
  mkEAiff85TextChunk_t*        nameChunk;
  mkEAiff85TextChunk_t*        copyrightChunk;
  mkEAiff85TextChunk_t*        authorChunk;
  mkEAiff85TextChunk_t*        annotationChunkList;         //use of this chunk is discouraged. use commentChunk instead;
  mkAiffGenericChunk_t*        unknownChunkList;
  int32_t*                     soundBuffer;
  int                          bufferCapacityInSamples;
  int                          numSamplesWrittenToBuffer;
  int32_t*                     playhead;
}MKAiff;


/*------------_-------_------------------------------------------------------------------
 _ __ _ _ ___| |_ ___| |_ _  _ _ __  ___ ___
| '_ \ '_/ _ \  _/ _ \  _| || | '_ \/ -_|_-<
| .__/_| \___/\__\___/\__|\_, | .__/\___/__/
|_|                       |__/|_|                       
 ---------------------------------------------------------------------------------------*/
//---------------------------------------------------------------------------------------
/*  returns an MKAiff object containing data from the specified aiff file. Currently,  */
/*  MKAiff does not support MIDI Chunks, RECORDING chunks, or Application chunks.      */
/*  MKAiff preserves these chunks in existing files, but does not provide support      */
/*  for creating, manipulating, or examining these chunks. Maybe later.                */

MKAiff*          aiffWithContentsOfFile       (char* filename);

//---------------------------------------------------------------------------------------
/*  create a mono AIFF by mixing the channels of the supplied MKAiff object            */
MKAiff*          aiffNewMono(MKAiff* aiff);

//---------------------------------------------------------------------------------------
/*  mix the channels of the current MKAiff object                                      */
void             aiffMakeMono(MKAiff* aiff);

//---------------------------------------------------------------------------------------
/*  returns a new MKAiff Object with an initial capacity of the specified number of    */
/*  seconds, samples or frames. A frame is an array of samples containing one sample   */
/*  for each channel. A sample is just a single number representing a single channel   */
/*  at a single point in time.  The sound buffer is initialized to all zeros. If less  */  
/*  data is written than was initially specified, the remainder will be ignored when   */  
/*  the file is saved.                                                                 */  
/*  If data is written that excedes the initial duration, more space will be allocated */
/*  automatically, but the excess may not be initialized. bitsPerSample must be between*/
/*  1 and 32. sampleRate is usually 44100. numChannels = 1 for mono, 2 for stereo 4    */
/*  for quad, etc. numChannels is read-only and cannot be modified once the object     */
/*  has been created.                                                                  */ 

MKAiff* aiffWithDurationInSeconds(int16_t numChannels, unsigned long sampleRate, int16_t bitsPerSample, int numSeconds);
MKAiff* aiffWithDurationInFrames (int16_t numChannels, unsigned long sampleRate, int16_t bitsPerSample, int numFrames );
MKAiff* aiffWithDurationInSamples(int16_t numChannels, unsigned long sampleRate, int16_t bitsPerSample, int numSamples);


//---------------------------------------------------------------------------------------
/*  save the MKAiff object as an AIFF file with the specified path.                    */

void             aiffSaveWithFilename         (MKAiff* aiff, char* filename);

//---------------------------------------------------------------------------------------
/*  save the MKAiff object as an Wave file with the specified path.                    */

void             aiffSaveWaveWithFilename     (MKAiff* aiff, char* filename);

//---------------------------------------------------------------------------------------
/*  add audio samples to the MKAiff object. buffer is a pointer to the data to add.    */
/*  the buffer must contain the same number of channels as the MKAiff Object, in       */
/*  interleaved format. In other respects, the data in the buffer need not             */
/*  be in the same format as the MKAiff object. numSamples is the number of samples    */
/*  in buffer. For floating-point samples, floatTypes is either aiffFloatSampleType    */
/*  or mkAiffDoubleSampleType. In either case, the samples must be within the range of */
/*  1 to -1 in order to avoid clipping.                                                */
/*  for integral samples, bytesPerSample is the size of each sample,                   */
/*  (ie. 1 if the buffer contains char, 2 for short, 4 for int). bitsPerSample is      */
/*  the number of valid bits in each sample. Note that bitsPerSample does not need     */
/*  to be equal to bytesPerSample*8. For instance, if 12 bit samples were stored       */
/*  as 16 bit shorts, bytesPerSample would be 2 and bitsPerSample would be 12.         */
/*  in the case that bitsPerSample != bytesPerSample*8, leftAligned is a boolean       */
/*  that indicates whether the valid bits are left-aligned(1) or right aligned(0).     */
/*  If buffer contains a signed data type, pass aiffYes for isSigned, otherwise aiffNo.*/
/*  if you pass aiffNo for overwrite, buffer will be mixed into the existing data at   */
/*  the current playhead location, and the number of clipped samples will be returned. */
/*  otherwise (aiffYes) the existing data, if any, will be overwritten forever.        */

typedef enum
{
  aiffFloatSampleType            = 0x0000,
  aiffDoubleSampleType           = 0x0001,
  aiffNotFloatingPointSampleType = 0x7FFF,
}mkAiffFloatingPointSampleType_t;

int aiffAddIntegerSamplesAtPlayhead(      MKAiff*       aiff, 
                                          void*         buffer, 
                                          int           numSamples, 
                                          int           bytesPerSample, 
                                          int           bitsPerSample, 
                                          aiffYesOrNo_t leftAligned, 
                                          aiffYesOrNo_t isSigned, 
                                          aiffYesOrNo_t overwrite);
                                    
int aiffAddFloatingPointSamplesAtPlayhead(MKAiff*                         aiff, 
                                          void*                           buffer, 
                                          int                             numSamples, 
                                          mkAiffFloatingPointSampleType_t floatType,
                                          aiffYesOrNo_t                   overwrite);

//from the playhead of fromAiff. You make sure channels match.
int aiffAddSamplesAtPlayheadFromAiff(MKAiff* aiff, MKAiff* fromAiff, int num_samples, aiffYesOrNo_t overwrite);

//---------------------------------------------------------------------------------------
/*  fastforwards the playhead to the end of the sound data previously written into the */
/*  file, and calls the above function with overwrite = aiffYes;                       */
                            
void aiffAppendIntegerSamples(      MKAiff*       aiff, 
                                    void*         buffer, 
                                    int           numSamples, 
                                    int           bytesPerSample, 
                                    int           bitsPerSample, 
                                    aiffYesOrNo_t leftAligned, 
                                    aiffYesOrNo_t isSigned);
                              
void aiffAppendFloatingPointSamples(MKAiff*                         aiff, 
                                    void*                           buffer, 
                                    int                             numSamples, 
                                    mkAiffFloatingPointSampleType_t floatType);

int aiffAppendSamplesAtPlayheadFromAiff(MKAiff* aiff, MKAiff* fromAiff, int num_samples);
void aiffAppendSilence(MKAiff* aiff, int num_samples);
void aiffAppendSilenceInSeconds(MKAiff* aiff, float secnds);
//---------------------------------------------------------------------------------------
/*  puts the specified number of samples into the provided buffer, and advances the   */
/*  playhead to the next sample beyond the last one read. the samples are             */
/*  left-aligned 32 bit signed ints, or floats.                                       */
/*  if shouldOverwrite == aiffNo, the samples will be summed into the supplied buffer */
/*  Returns the number of samples successfully read                                   */

int aiffReadIntegerSamplesAtPlayhead       (MKAiff* aiff, int32_t  buffer[], int numSamples, aiffYesOrNo_t shouldOverwrite);
int aiffReadFloatingPointSamplesAtPlayhead (MKAiff* aiff, float    buffer[], int numSamples, aiffYesOrNo_t shouldOverwrite);

//---------------------------------------------------------------------------------------
/*  removes the specified number of samples starting at the current playhead location  */
void aiffRemoveSamplesAtPlayhead           (MKAiff* aiff, int numSamples);

//---------------------------------------------------------------------------------------
/*  sets the sample rate. Notice that other values previously specified as duration    */
/*  in seconds are not modified to reflect this change. For instance, if you put a     */
/*  marker at 2 seconds into the file, and then double the sample rate, the marker will*/
/*  then be only one second into the file. This method does not resample the file,     */
/*  it only changes the number that represents the sample rate. To resample the file   */
/*  at a new rate, use aiffResample                                                    */

void             aiffSetSampleRate            (MKAiff* aiff, double rate);

//---------------------------------------------------------------------------------------
/*  change the sample rate and resample the audio.                                     */

typedef enum
{
 aiffInterpLinear,
}aiffInterpolation_t;

aiffYesOrNo_t    aiffResample                 (MKAiff* aiff, unsigned long rate, aiffInterpolation_t interp /*currently ignored*/);


//---------------------------------------------------------------------------------------
/*  returns the sample rate of the MKAiff object                                       */

double    aiffSampleRate               (MKAiff* aiff);


//---------------------------------------------------------------------------------------
/*  returns the number of channels of the MKAiff object                                */

int16_t          aiffNumChannels              (MKAiff* aiff);


//---------------------------------------------------------------------------------------
/*  returns the number of valid samples that have been put into the MKAiff object      */

int              aiffDurationInSamples        (MKAiff* aiff);
int              aiffDurationInFrames         (MKAiff* aiff);
float            aiffDurationInSeconds        (MKAiff* aiff);


//---------------------------------------------------------------------------------------
/*  get or set the bit depth of the the MKAiff object. May be any number between 1     */
/*  and 32. Lowering the bit-depth will non-destructively truncate the rightmost bits  */
/*  of each sample (restoring the bit rate will bring those bits back), and            */
/*  raising the bit-depth will pad the right side of each sample with zeros            */

void             aiffSetBitsPerSample         (MKAiff* aiff, int16_t sampleSize);
int16_t          aiffBitsPerSample            (MKAiff* aiff);


//---------------------------------------------------------------------------------------
/*  get the number of bytes required to store aiffBitsPerSample(aiff).                 */
/*  will be 1, 2, 3 or 4.                                                              */

uint16_t         aiffBytesPerSample           (MKAiff* aiff);


//---------------------------------------------------------------------------------------
/*  move the playhead/recordhead to the location specified in seconds, samples or      */
/*  frames. Note that a sample is a single number within the sound-buffer, containing, */
/*  information for one channel at one point in time. A frame is an array of samples,  */
/*  containing one sample for each channel. The playhead may be set to any location    */
/*  within the allocated sound buffer, even if data has not been written to that       */
/*  location yet.                                                                      */

void             aiffSetPlayheadToSeconds     (MKAiff* aiff, double numSeconds);
void             aiffSetPlayheadToFrames      (MKAiff* aiff, int numFrames);
void             aiffSetPlayheadToSamples     (MKAiff* aiff, int numSamples);


//---------------------------------------------------------------------------------------
/*  nudge the playhead the specified number of samples, seconds or frames, see above   */
/*  for a discussion of the difference. pass positive values to fast-forward, negative */
/*  to rewind                                                                          */

void             aiffAdvancePlayheadBySeconds (MKAiff* aiff, double numSeconds);
void             aiffAdvancePlayheadByFrames  (MKAiff* aiff, int numFrames);
void             aiffAdvancePlayheadBySamples (MKAiff* aiff, int numSamples);


//---------------------------------------------------------------------------------------
/*  returns the current position of the play/record head.                              */

double           aiffPlayheadPositionInSeconds(MKAiff* aiff);
int              aiffPlayheadPositionInFrames (MKAiff* aiff);
int              aiffPlayheadPositionInSamples(MKAiff* aiff);


//---------------------------------------------------------------------------------------
/*  sets the play/record head to the beginning of the sound buffer                     */

void             aiffRewindPlayheadToBeginning(MKAiff* aiff);


//---------------------------------------------------------------------------------------
/*  places the play/record head one sample beyond the last valid sample in the         */
/*  buffer. the buffer always contains at least one unaccounted sample atthe end       */

void             aiffFastForwardPlayheadToEnd (MKAiff* aiff);


//---------------------------------------------------------------------------------------
/*  add a text comment to the file. The comment is restricted to 65536 characters.     */
/*  marker is the ID of the marker with which to associate the comment, or 0 if the    */
/*  comment is not to be associated with a marker. The comment may not be edited once  */
/*  it is added although all comments can be removed with aiffDestroyCommentChunk().   */

void             aiffAddCommentWithText       (MKAiff* aiff, char text[], mkAiffMarkerID_t marker);
void             aiffRemoveAllComments        (MKAiff* aiff);

//---------------------------------------------------------------------------------------
/*  add a marker to the MKAiff object. name is restricted to 255 bytes. A more         */
/*  lengthy comment can be added to the marker using aiffAddCommentWithText(), and     */
/*  specifying the markerID of the marker you want to add the comment to. markerID     */
/*  can be any unique number between 1 and 65535. 0 is not a valid marker ID.          */
/*  positionIn... is the location of the marker. The marker must fall in between       */
/*  frames, so positions specified in seconds or samples will be rounded to the nearest*/
/*  frame.                                                                             */

void aiffAddMarkerWithPositionInSamples(MKAiff* aiff, char name[], mkAiffMarkerID_t markerID, uint32_t positionInSamples);
void aiffAddMarkerWithPositionInFrames (MKAiff* aiff, char name[], mkAiffMarkerID_t markerID, uint32_t positionInFrames );
void aiffAddMarkerWithPositionInSeconds(MKAiff* aiff, char name[], mkAiffMarkerID_t markerID, double   positionInSeconds);

//---------------------------------------------------------------------------------------
/*  get data about a marker with the specified ID or Name. Returns aiffNo if the       */
/*  marker does not exist. Otherwise, returns aiffYes and writes the requested info    */
/*  into 'result'                                                                      */

aiffYesOrNo_t aiffPositionInFramesOfMarkerWithID  (MKAiff* aiff, mkAiffMarkerID_t markerID, uint32_t         *result);
aiffYesOrNo_t aiffPositionInFramesOfMarkerWithName(MKAiff* aiff, char*            name    , uint32_t         *result);
aiffYesOrNo_t aiffNameOfMarkerWithID              (MKAiff* aiff, mkAiffMarkerID_t markerID, char*            *result);
aiffYesOrNo_t aiffMarkerIDOfMarkerWithName        (MKAiff* aiff, char*            name    , mkAiffMarkerID_t *result);

void aiffRemoveAllMarkers(MKAiff* aiff);


//---------------------------------------------------------------------------------------
/*  setup the aiff file for use in samplers and other playback instruments. baseNote is*/
/*  the MIDI note number of the aiffFile with no pitch modification (0xC3 is middle C).*/
/*  detune should be between +50 and - 50 cents. lowNote and highNote are MIDI note    */
/*  numbers of the suggested range on a keyboard for playback of the sound data, ie the*/
/*  sample will only be played back if the requested note is within this range.        */
/*  lowVelocity and highVelocity are MIDI velocity numbers (1-127) of the suggested    */
/*  range of velocities for playback of the sound data, ie the sound will only be      */
/*  played back if the requested velocity falls within this range. decibelsGain is the */
/*  ammount of gain to apply to the file during playback. 0 is onoe, 6 is twice as     */
/*  loud, -6 is half as loud, etc.. The following variables set up a sustain and       */
/*  release loop. start and end markers are the marker numbers of the markers that     */
/*  are at the location where the loop is to start or end. playMode should be one of   */
/*  AIFF_LOOP_MODE_FORWARD, AIFF_LOOP_MODE_FORWARD_BACKWARD or AIFF_LOOP_MODE_OFF if   */
/*  the loop is to be ignored.                                                         */


void aiffSetupInstrumentInfo(MKAiff*          aiff,
                             int8_t           baseNote,     
                             int8_t           detune,       
                             int8_t           lowNote,      
                             int8_t           highNote,     
                             int8_t           lowVelocity,  
                             int8_t           highVelocity, 
                             int16_t          decibelsGain, 
                             int16_t          sustainLoopPlayMode,     
                             mkAiffMarkerID_t sustainLoopStartMarkerID,
                             mkAiffMarkerID_t sustainLoopEndMarkerID,
                             int16_t          releaseLoopPlayMode,     
                             mkAiffMarkerID_t releaseLoopStartMarkerID,
                             mkAiffMarkerID_t releaseLoopEndMarkerID);

aiffYesOrNo_t    aiffHasInstrumentInfo                 (MKAiff* aiff);
int8_t           aiffInstrumentBaseNote                (MKAiff* aiff);
int8_t           aiffInstrumentDetune                  (MKAiff* aiff);
int8_t           aiffInstrumentLowNote                 (MKAiff* aiff);
int8_t           aiffInstrumentHighNote                (MKAiff* aiff);
int8_t           aiffInstrumentLowVelocity             (MKAiff* aiff);
int8_t           aiffInstrumentHighVelocity            (MKAiff* aiff);
int16_t          aiffInstrumentDecibelsGain            (MKAiff* aiff);
int16_t          aiffInstrumentSustainLoopPlayMode     (MKAiff* aiff);
mkAiffMarkerID_t aiffInstrumentSustainLoopStartMarkerID(MKAiff* aiff);
mkAiffMarkerID_t aiffInstrumentSustainLoopEndMarkerID  (MKAiff* aiff);
int16_t          aiffInstrumentReleaseLoopPlayMode     (MKAiff* aiff);
mkAiffMarkerID_t aiffInstrumentReleaseLoopStartMarkerID(MKAiff* aiff);
mkAiffMarkerID_t aiffInstrumentReleaseLoopEndMarkerID  (MKAiff* aiff);
void             aiffRemoveInstrumentInfo              (MKAiff* aiff);


//---------------------------------------------------------------------------------------
/*  set the name, author or copyright, or add an annotation, in accordance with the    */
/*  Electronic Arts 1985 IFF specification. There may be only one name, author and     */
/*  copyright, but there may be several annotations. The use of annotations is,        */
/*  however, both discouraged and widely used by Apple, who recommends using Comments  */
/*  instead. see aiffAddCommentWithText() above.                                       */

void             aiffSetName                  (MKAiff* aiff, char* text);
void             aiffSetAuthor                (MKAiff* aiff, char* text); 
void             aiffSetCopyright             (MKAiff* aiff, char* text);
void             aiffAddAnnotation            (MKAiff* aiff, char* text);

char*            aiffName                     (MKAiff* aiff);
char*            aiffAuthor                   (MKAiff* aiff); 
char*            aiffCopyright                (MKAiff* aiff);        

void             aiffRemoveName               (MKAiff* aiff);
void             aiffRemoveAuthor             (MKAiff* aiff);
void             aiffRemoveCopyright          (MKAiff* aiff);
void             aiffRemoveAllAnnotations     (MKAiff* aiff);

//---------------------------------------------------------------------------------------
/*  add a generic chunk that is otherwise unsupported.                                 */

void aiffAddGenericChunk(MKAiff* aiff, unsigned char* data, int32_t sizeOfData, aiffChunkIdentifier_t chunkID);
void aiffRemoveAllGenericChunks(MKAiff* aiff);

//---------------------------------------------------------------------------------------
/*  Normalize the file and optionally remove DC offset       (currently ignored)       */
void             aiffNormalize(MKAiff* aiff, aiffYesOrNo_t removeOffset);

//double           aiffPeakDb       (MKAiff* aiff);
//void             aiffChangeGain   (MKAiff* aiff, double dbGain);

//---------------------------------------------------------------------------------------
/*  Return a new MKAIFF object that represents the amplitude envelope of the supplied object */
/*  0.9 is the recommended smoothing coefficient. the supplied aiff should be mono. */

MKAiff*          aiffGetAmplitudeEnvelope(MKAiff* monoAiff);


//---------------------------------------------------------------------------------------
/*  Trim silence off the begining of the object. Cutoff is a number between 0 ~ 1 (0.04 is good...) */
/*  Amplitudes below cutoff will be treated as silence and removed. */
/*  if scanZeroCrossing == aiffYes, this function will scan backwards to the nearest  */
/*  zero-crossing on the channel that first surpassed the amplitude cutoff, and trim there*/
/*  otherwise it will trim from the frame of the first sample that surpassed the cutoff*/
void          aiffTrimBeginning(MKAiff* aiff, double cutoff, aiffYesOrNo_t scanZeroCrossing);


//---------------------------------------------------------------------------------------
/* see above. scans forward for zero crossing if aiffYes */
void         aiffTrimEnd     (MKAiff* aiff, double cutoff, aiffYesOrNo_t scanZeroCrossing);

//---------------------------------------------------------------------------------------
typedef enum
{
  aiffLinearFadeType             = 0x0000,
}mkAiffFadeType_t;
void         aiffFadeInOut(MKAiff* aiff, aiffYesOrNo_t in, aiffYesOrNo_t out, float fadeTime, mkAiffFadeType_t curve_type /*only linear is supported right now*/);

//---------------------------------------------------------------------------------------
/* returns number of clipped samples */
int         aiffChangeGain(MKAiff* aiff, double gain);

//---------------------------------------------------------------------------------------
/*  destroy the MKAiff object and free all of its resources. returns NULL. Correct use: */
/*  MKAiff* aiff = aiffWithFilename("MyGreatSong.aif");                                 */
/*  aiff = aiffDestroy(aiff);                                                           */
/*  This prevents many accidental segmentation faults                                   */

MKAiff*          aiffDestroy                  (MKAiff* aiff);


//---------------------------------------------------------------------------------------
#if defined(__cplusplus)
}
#endif   //(__cplusplus)

#endif //MK_AIFF
