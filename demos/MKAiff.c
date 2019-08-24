/*_--__-_--__----_----_--__--__----------------------------------------------------------
|  \/  | |/ /   / \  (_)/ _|/ _| ___ 
| |\/| | ' /   / _ \ | | |_| |_ / __|
| |  | | . \  / ___ \| |  _|  _| (__ 
|_|  |_|_|\_\/_/   \_\_|_| |_|(_)___|                                          
Beta, 1.0 May 03 2011
Modified Nov 30 2012 to fix bugs pertaining to 64 bit pointers   
Modified MAY 30 2013 to redefine multi-char consts as hex
Modified JUN 16 2013 to add makeMono() and resample(), and use binary mode to read / write files.
Modified AUG  1 2013 to fix a bug in resample().  
Modified SEPT  1 2013 to add trimBeginning(), trimEnd(), getAmplitudeEnvelope() and removeSamplesAtPlayhead, fix clipping for floating point samples.  
Modified DEC  28 2014 to get name, author, copyright
Modified JAN  09 2014 to fix fclose() bug
Modified AUG  11 2018 to ad support for reading WAVE files, aiffFadeInOut()
Modified AUG  28 2018 to ad support for writing WAVE files, and fix endian thing
---------------------------------------------------------------------------------------*/
#include "MKAiff.h"
#include <math.h>
//#define AIFF_PRINT_DEBUG_MESSAGES

/*--_------__-_--------------------------------------------------------------------------
 __| |___ / _(_)_ _  ___ ___
/ _` / -_)  _| | ' \/ -_|_-<
\__,_\___|_| |_|_||_\___/__/
---------------------------------------------------------------------------------------*/
#define EA_IFF85_NAME_CHUNK_ID       0x4E414D45	//'NAME'
#define EA_IFF85_AUTHOR_CHUNK_ID     0x41555448	//'AUTH'
#define EA_IFF85_COPYRIGHT_CHUNK_ID  0x28632920	//'(c) '
#define EA_IFF85_ANNOTATION_CHUNK_ID 0x414E4E4F	//'ANNO'

#define WAVE_FORM_CHUNK_ID           0x52494646 //'RIFF'
#define WAVE_FORM_TYPE_ID            0x57415645 //'WAVE'

#define AIFF_FORM_CHUNK_ID           0x464F524D	//'FORM'
#define AIFF_FORM_TYPE_ID            0x41494646	//'AIFF'
#define AIFF_COMMON_CHUNK_ID         0x434F4D4D	//'COMM'
#define AIFF_SOUND_CHUNK_ID          0x53534E44	//'SSND'
#define AIFF_MARKER_CHUNK_ID         0x4D41524B	//'MARK'
#define AIFF_INSTRUMENT_CHUNK_ID     0x494E5354	//'INST'
#define AIFF_MIDI_CHUNK_ID           0x4D494449	//'MIDI'
#define AIFF_RECORDING_CHUNK_ID      0x41455344	//'AESD'
#define AIFF_APPLICATION_CHUNK_ID    0x4150504C	//'APPL'
#define AIFF_COMMENT_CHUNK_ID        0x434F4D54	//'COMT'

#define WAVE_FMT_CHUNK_ID            0x666d7420 //'FMT '
#define WAVE_DATA_CHUNK_ID           0x64617461 //'DATA'

/*--------_----------_------------_--------_---------------_---_-------------------------
 _ __ _ _(_)_ ____ _| |_ ___   __| |___ __| |__ _ _ _ __ _| |_(_)___ _ _  ___
| '_ \ '_| \ V / _` |  _/ -_) / _` / -_) _| / _` | '_/ _` |  _| / _ \ ' \(_-<
| .__/_| |_|\_/\__,_|\__\___| \__,_\___\__|_\__,_|_| \__,_|\__|_\___/_||_/__/
|_|                                                                          
---------------------------------------------------------------------------------------*/ 
//these return 1 on success, 0 otherwise
int aiffExtractFormChunkFromFile(mkAiffFormChunk_t* formChunk, FILE* file);
int aiffExtractCommonChunkFromFile              (MKAiff* aiff, FILE* file);
int aiffExtractSoundChunkAndSoundBufferFromFile (MKAiff* aiff, FILE* file);
int aiffExtractCommentChunkFromFile             (MKAiff* aiff, FILE* file);
int aiffExtractMarkerChunkFromFile              (MKAiff* aiff, FILE* file);
int aiffExtractInstrumentChunkFromFile          (MKAiff* aiff, FILE* file);
int aiffExtractMIDIChunkFromFile                (MKAiff* aiff, FILE* file);
int aiffExtractRecordingChunkFromFile           (MKAiff* aiff, FILE* file);
int aiffExtractApplicationChunkFromFile         (MKAiff* aiff, FILE* file);
int aiffExtractTextChunkFromFile                (MKAiff* aiff, FILE* file, aiffChunkIdentifier_t chunkID, aiffYesOrNo_t isWave);
int aiffExtractGenericChunkFromFile             (MKAiff* aiff, FILE* file, aiffChunkIdentifier_t chunkID, aiffYesOrNo_t isWave);
int aiffExtractAnnotationChunkFromFile          (MKAiff* aiff, FILE* file);

void aiffWriteFormChunkToFile                   (MKAiff* aiff, FILE* file, aiffYesOrNo_t isWave);
void aiffWriteCommonChunkToFile                 (MKAiff* aiff, FILE* file);
void aiffWriteSoundChunkAndSoundBufferToFile    (MKAiff* aiff, FILE* file);
void aiffWriteCommentChunkToFile                (MKAiff* aiff, FILE* file);
void aiffWriteMarkerChunkToFile                 (MKAiff* aiff, FILE* file);
void aiffWriteInstrumentChunkToFile             (MKAiff* aiff, FILE* file);
void aiffWriteMIDIChunksToFile                  (MKAiff* aiff, FILE* file);
void aiffWriteRecordingChunkToFile              (MKAiff* aiff, FILE* file);
void aiffWriteApplicationChunksChunkToFile      (MKAiff* aiff, FILE* file);      
void aiffWriteTextChunkToFile                   (MKAiff* aiff, FILE* file, mkEAiff85TextChunk_t* textChunk);
void aiffWriteAnnotationChunksToFile            (MKAiff* aiff, FILE* file);
void aiffWriteGenericChunksToFile               (MKAiff* aiff, FILE* file);

typedef enum
{
  aiffMarkerCriterionMarkerID,
  aiffMarkerCriterionName
}mkAiffMarkerCriterion_t;

mkAiffMarker_t* aiffFindMarkerWithCriterion     (MKAiff* aiff, mkAiffMarkerCriterion_t criterion, void* value);


mkEAiff85TextChunk_t* aiffCreateTextChunk(aiffChunkIdentifier_t chunkID, char* text);

void aiffAppendEntryToList(mkAiffListEntry_t* newEntry, mkAiffListEntry_t* firstEntry); 

MKAiff* aiffAllocate();
int aiffAllocateSoundBuffer(MKAiff* aiff, int numSamples);

int aiffNumBytesInList(mkAiffSizableListEntry_t* listEntry, int numBytesPerEntryNotCountedInChunkSize);

int aiffAddSamplesAtPlayhead(MKAiff* aiff, 
                            void*                           buffer, 
                            int                             numSamples, 
                            aiffYesOrNo_t                   isFloat,
                            mkAiffFloatingPointSampleType_t floatType,
                            int                             bytesPerSample, 
                            int                             bitsPerSample, 
                            aiffYesOrNo_t                   leftAligned, 
                            aiffYesOrNo_t                   isSigned, 
                            aiffYesOrNo_t                   overwrite);

void ConvertToIeeeExtended(double num, unsigned char* bytes);
double ConvertFromIeeeExtended(unsigned char* bytes);


/*---------_--------------------_--------------------------------------------------------
| |__ _  _| |_ ___   ___ _ _ __| |___ _ _ 
| '_ \ || |  _/ -_) / _ \ '_/ _` / -_) '_|
|_.__/\_, |\__\___| \___/_| \__,_\___|_|  
      |__/                                
---------------------------------------------------------------------------------------*/  
aiffYesOrNo_t aiffWriteIntBigEndian(FILE* file, uint32_t val, int numBytes)
{
  int i, numBytesWritten = 0;
  uint8_t v;
  for(i=numBytes-1; i>=0; i--)
    {
      v = (val >> (8*i)) & 0xFF;
      numBytesWritten += fwrite(&v, 1, 1, file);
    }
  
  return (numBytesWritten == numBytes) ? aiffNo : aiffYes;
}

//--------------------------------------------------------------------------------------
aiffYesOrNo_t aiffWriteIntLittleEndian(FILE* file, uint32_t val, int numBytes)
{
  int i, numBytesWritten = 0;
  uint8_t v;
  for(i=0; i<numBytes; i++)
    {
      v = (val >> (8*i)) & 0xFF;
      numBytesWritten += fwrite(&v, 1, 1, file);
    }
  
  return (numBytesWritten == numBytes) ? aiffNo : aiffYes;
}

//--------------------------------------------------------------------------------------
aiffYesOrNo_t aiffWriteBuffer(FILE* file, void* val, int numBytes)
{
  int numBytesWritten = fwrite(val, 1, numBytes, file);
  return (numBytesWritten == numBytes) ? aiffNo : aiffYes;
}

//--------------------------------------------------------------------------------------
uint32_t aiffReadIntBigEndian(FILE* file, int numBytes, aiffYesOrNo_t *error)
{
  int i;
  uint32_t val = 0;
  int numBytesRead = 0;
  uint8_t v;
  
  for(i=0; i<numBytes; i++)
    {
      val <<= 8;
      numBytesRead += fread(&v, 1, 1, file);
      val |= v;
    }
  
  *error = (numBytesRead == numBytes) ? aiffNo : aiffYes;
  return val;
}

//--------------------------------------------------------------------------------------
uint32_t aiffReadIntLittleEndian(FILE* file, int numBytes, aiffYesOrNo_t *error)
{
  int i;
  uint32_t val = 0;
  int numBytesRead = 0;
  uint8_t v;
  
    for(i=0; i<numBytes; i++)
    {
      numBytesRead += fread(&v, 1, 1, file);
      val |= ((uint32_t)v << (8*i));
    }

  *error = (numBytesRead == numBytes) ? aiffNo : aiffYes;
  return val;
}

//--------------------------------------------------------------------------------------
void aiffReadBuffer(FILE* file, void* buffer, int numBytes, aiffYesOrNo_t *error)
{
  int numBytesRead = fread(buffer, 1, numBytes, file);
  *error = (numBytesRead == numBytes) ? aiffNo : aiffYes;
}

/*-------_---------------_---_----------------_-------------_----------------------------
 _____ _| |_ _ _ __ _ __| |_(_)_ _  __ _   __| |_ _  _ _ _ | |__ ___
/ -_) \ /  _| '_/ _` / _|  _| | ' \/ _` | / _| ' \ || | ' \| / /(_-<
\___/_\_\\__|_| \__,_\__|\__|_|_||_\__, | \__|_||_\_,_|_||_|_\_\/__/
                                   |___/                           
---------------------------------------------------------------------------------------*/

//extractFormChunkFromFile---------------------------------------------------------------
int aiffExtractFormChunkFromFile(mkAiffFormChunk_t* formChunk, FILE* file)
{
  aiffYesOrNo_t error;
  formChunk->chunkID    = aiffReadIntBigEndian(file, 4, &error); if(error) return 0;
  
  if(formChunk->chunkID == AIFF_FORM_CHUNK_ID)
    {formChunk->chunkSize  = aiffReadIntBigEndian(file, 4, &error); if(error) return 0;}
  else if(formChunk->chunkID == WAVE_FORM_CHUNK_ID)
    {formChunk->chunkSize  = aiffReadIntLittleEndian(file, 4, &error); if(error) return 0;}
  else return 0;
  
  formChunk->formTypeID = aiffReadIntBigEndian(file, 4, &error); if(error) return 0;

  #ifdef AIFF_PRINT_DEBUG_MESSAGES
  fprintf(stderr, "Form Chunk detected: id: %08X, size: %i, Type: %08X\r\n", formChunk->chunkID, formChunk->chunkSize, formChunk->formTypeID);
  #endif
  return 1;
}

//extractCommonChunkFromFile-------------------------------------------------------------
int aiffExtractCommonChunkFromFile(MKAiff* aiff, FILE* file)
{
  aiffYesOrNo_t error;
  aiff->commonChunk.chunkID         = AIFF_COMMON_CHUNK_ID;
  aiff->commonChunk.chunkSize       = aiffReadIntBigEndian(file, 4,  &error); if(error) return 0;
  aiff->commonChunk.numChannels     = aiffReadIntBigEndian(file, 2,  &error); if(error) return 0;
  aiff->commonChunk.numSampleFrames = aiffReadIntBigEndian(file, 4,  &error); if(error) return 0;
  aiff->commonChunk.bitsPerSample   = aiffReadIntBigEndian(file, 2,  &error); if(error) return 0;
  aiffReadBuffer (file, aiff->commonChunk.sampleRate, 10, &error); if(error) return 0;
  #ifdef AIFF_PRINT_DEBUG_MESSAGES
  printf("common chunk detected: numChannels: %i, sampleRate: %i, bitsPerSample: %i, numFrames: %i, duration: %f\n", aiffNumChannels(aiff), (int)aiffSampleRate(aiff), aiffBitsPerSample(aiff), aiff->commonChunk.numSampleFrames, (float)aiff->commonChunk.numSampleFrames / (float)aiffSampleRate(aiff));
  #endif  
  return 1;
}

//extractCommonChunkFromFile-------------------------------------------------------------
int aiffExtractWaveFormatChunkFromFile(MKAiff* aiff, FILE* file)
{
  mkAiffWaveFormatChunk_t chunk;
  
  aiffYesOrNo_t error;
  chunk.chunkSize      = aiffReadIntLittleEndian(file, 4,  &error); if(error) return 0;
  chunk.audioFormat    = aiffReadIntLittleEndian(file, 2,  &error); if(error) return 0;
  chunk.numChannels    = aiffReadIntLittleEndian(file, 2,  &error); if(error) return 0;
  chunk.sampleRate     = aiffReadIntLittleEndian(file, 4,  &error); if(error) return 0;
  chunk.byteRate       = aiffReadIntLittleEndian(file, 4,  &error); if(error) return 0;
  chunk.blockAlign     = aiffReadIntLittleEndian(file, 2,  &error); if(error) return 0;
  chunk.bitsPerSample  = aiffReadIntLittleEndian(file, 2,  &error); if(error) return 0;
 
  //I have no idea what type of format this is, but if Reaper decodes is correctly then it sample-identical to linear PCM.
  //just with a longer header
  //type:65534 len:40
  
  //fprintf(stderr, "wave type: %i, len: %i\r\n", chunk.audioFormat, chunk.chunkSize);
  if((chunk.audioFormat != 1) && (chunk.audioFormat != 65534))
    {fprintf(stderr, "This is not a known type of linear PCM wave, but I'll try to open it anyhow.\r\n"); }
  
  int garbage_size = chunk.chunkSize - 16;
  if(garbage_size > 0)
    {
      uint8_t garbage[garbage_size];
      if(!fread(&garbage,       garbage_size, 1, file)) return 0;
    }
  
  aiff->commonChunk.chunkID          = AIFF_COMMON_CHUNK_ID;
  aiff->commonChunk.chunkSize        = 18;
  aiff->commonChunk.numChannels      = chunk.numChannels;
  aiff->commonChunk.numSampleFrames  = 0; //this can't be calculated here, but it isnt used os who cares.
  aiff->commonChunk.bitsPerSample    = chunk.bitsPerSample;
  aiffSetSampleRate(aiff, chunk.sampleRate);
  
  #ifdef AIFF_PRINT_DEBUG_MESSAGES
  printf("wave format chunk detected: numChannels: %i, sampleRate: %i, bitsPerSample: %i, numFrames: %i, duration: %f\n", aiffNumChannels(aiff), (int)aiffSampleRate(aiff), aiffBitsPerSample(aiff), aiff->commonChunk.numSampleFrames, (float)aiff->commonChunk.numSampleFrames / (float)aiffSampleRate(aiff));
  #endif
  return 1;
}

//extractCommonChunkFromFile-------------------------------------------------------------
int aiffExtractWaveDataChunkFromFile(MKAiff* aiff, FILE* file)
{
  int i, bits, bytes, numBytes, numSamples;
  int32_t sample;
  mkAiffWaveDataChunk_t chunk;
  aiffYesOrNo_t error;
  chunk.chunkSize = aiffReadIntLittleEndian(file, 4,  &error); if(error) return 0;
  
  aiff->soundChunk.chunkID   = AIFF_SOUND_CHUNK_ID;
  aiff->soundChunk.chunkSize = chunk.chunkSize + 8;
  aiff->soundChunk.offset    = 0;
  aiff->soundChunk.blockSize = 0;

  //interstingly, the chunks could be out of order, so we might not yet know how many bytes per sample
  bytes    = aiffBytesPerSample(aiff);
  if((bytes < 1) || (bytes > 4)) return 0;
  bits     = aiffBitsPerSample(aiff);
  numBytes = aiff->soundChunk.chunkSize - 8;
  numSamples = numBytes / bytes;
  
  //fprintf(stderr, "data -- chunkSize: %i, bytes=%i, bits=%i, samples=%i\r\n", chunk.chunkSize, bytes, bits, numSamples);
  
  if(!aiffAllocateSoundBuffer(aiff, numSamples)) return 0;
  for(i=0; i<numSamples; i++)
    {
      sample = aiffReadIntLittleEndian(file, bytes,  &error); if(error) return 0;
      sample <<= 32-bits;
      aiffAppendIntegerSamples(aiff, &sample, 1, 4, 32, aiffYes, aiffYes);
    }

  #ifdef AIFF_PRINT_DEBUG_MESSAGES
  printf("Wave Data chunk detected\n");
  #endif
  return 1;
}

//extractSoundChunkAndSoundBufferFromFile------------------------------------------------
int aiffExtractSoundChunkAndSoundBufferFromFile(MKAiff* aiff, FILE* file)
{
  aiffYesOrNo_t error;
  int i, bits, bytes, numBytes, numSamples;
  int32_t sample;
  
  aiff->soundChunk.chunkID   = AIFF_SOUND_CHUNK_ID;
  aiff->soundChunk.chunkSize = aiffReadIntBigEndian(file, 4,  &error); if(error) return 0;
  aiff->soundChunk.offset    = aiffReadIntBigEndian(file, 4,  &error); if(error) return 0;
  aiff->soundChunk.blockSize = aiffReadIntBigEndian(file, 4,  &error); if(error) return 0;
  
  //interstingly, the chunks could be out of order, so we might not yet know how many bytes per sample
  bytes    = aiffBytesPerSample(aiff);
  if((bytes < 1) || (bytes > 4)) return 0;
  bits     = aiffBitsPerSample(aiff); if(bits <= 0) return 0;
  numBytes = aiff->soundChunk.chunkSize - 8;
  numSamples = numBytes / bytes;
  
  int total_bytes_read = 0;
  
  if(!aiffAllocateSoundBuffer(aiff, numSamples)) return 0;
  
  for(i=0; i<numSamples; i++)
    {
      sample = aiffReadIntBigEndian(file, bytes,  &error); if(error) return 0;
      total_bytes_read += bytes;
      sample <<= 32-bits;
      //fprintf(stderr, "%i\r\n", sample);
      aiffAppendIntegerSamples(aiff, &sample, 1, 4, 32, aiffYes, aiffYes);
    }

  #ifdef AIFF_PRINT_DEBUG_MESSAGES
  printf("aiff sound chunk detected: read samples: %i, total_bytes_read: %i, bits_per_sample: %i, bytes_per_sample:%i \r\n", numSamples, total_bytes_read, bits, bytes);
  #endif
  return 1;
}

//extractCommentChunkFromFile------------------------------------------------------------
int aiffExtractCommentChunkFromFile(MKAiff* aiff, FILE* file)
{
  mkAiffCommentChunk_t tempCommentChunk;
  aiffYesOrNo_t error;
  
  //tempCommentChunk.chunkID = AIFF_COMMENT_CHUNK_ID;
  tempCommentChunk.chunkSize   = aiffReadIntBigEndian(file, 4,  &error); if(error) return 0;
  tempCommentChunk.numComments = aiffReadIntBigEndian(file, 2,  &error); if(error) return 0;
  
  int i;
  mkAiffComment_t tempComment;
  for(i=0; i<tempCommentChunk.numComments; i++)
    {
      tempComment.timeStamp = aiffReadIntBigEndian(file  , 4,  &error); if(error) return 0;
      tempComment.markerID  = aiffReadIntBigEndian(file  , 2,  &error); if(error) return 0;
      tempComment.numChars  = aiffReadIntBigEndian(file  , 2,  &error); if(error) return 0;
    
      char text[tempComment.numChars + 1];
      aiffReadBuffer(file, text, tempComment.numChars, &error); if(error) return 0;
      if(!fread(text, tempComment.numChars, 1, file)) return 0;
      text[tempComment.numChars] = '\0';
      aiffAddCommentWithText(aiff, text, tempComment.markerID);
      //if numChars is odd there will be an extra byte of padding that can be read into somewhere and ignored
      if(tempComment.numChars & 0x01) if(!fread(text,  1, 1, file)) return 0;
      #ifdef AIFF_PRINT_DEBUG_MESSAGES
      printf("comment detected: %s\n", text);
      #endif
    }
  
  int discrepancy = tempCommentChunk.chunkSize - aiff->commentChunk->chunkSize; 
  if(discrepancy > 0)
    {
      //why does Logic write garbage into its files?
      char garbage[discrepancy];
      if(!fread(garbage, discrepancy, 1, file)) return 0;
    }
  return 1;
} 

//extractMarkerChunkFromFile-------------------------------------------------------------
int aiffExtractMarkerChunkFromFile(MKAiff* aiff, FILE* file)
{
  aiffYesOrNo_t error;
  mkAiffMarkerChunk_t tempMarkerChunk;
  //tempMarkerChunk.chunkID = AIFF_MARKER_CHUNK_ID;
  tempMarkerChunk.chunkSize   = aiffReadIntBigEndian(file, 4,  &error); if(error) return 0;
  tempMarkerChunk.numMarkers  = aiffReadIntBigEndian(file, 2,  &error); if(error) return 0;

  int i;
  mkAiffMarker_t tempMarker;
  for(i=0; i<tempMarkerChunk.numMarkers; i++)
    {
      tempMarker.markerID         = aiffReadIntBigEndian(file, 2,  &error); if(error) return 0;
      tempMarker.positionInFrames = aiffReadIntBigEndian(file, 4,  &error); if(error) return 0;
      char size = aiffReadIntBigEndian(file, 1,  &error); if(error) return 0;
      char text[size+1];
      aiffReadBuffer(file, text, size, &error); if(error) return 0;
      text[(int)size] = '\0';
      aiffAddMarkerWithPositionInFrames(aiff, text, tempMarker.markerID, tempMarker.positionInFrames);
      //if numChars is odd there will be an extra byte of padding that can be read into somewhere and ignored
      if((size+1) & (0x01)) if(!fread(text,  1, 1, file)) return 0;
      #ifdef AIFF_PRINT_DEBUG_MESSAGES
      printf("marker detcted: %s\n", text);
      #endif
    }
  int discrepancy = tempMarkerChunk.chunkSize - aiff->markerChunk->chunkSize; 
  if(discrepancy > 0)
    {
      char garbage[discrepancy];
      if(!fread(garbage, discrepancy, 1, file)) return 0;
    }
  return 1;
} 


//extractInstrumentChunkFromFile---------------------------------------------------------
int aiffExtractInstrumentChunkFromFile         (MKAiff* aiff, FILE* file)
{
  aiffYesOrNo_t error;
  mkAiffInstrumentChunk_t tempInstChunk;
  
  tempInstChunk.chunkSize     = aiffReadIntBigEndian(file, 4,  &error); if(error) return 0;
  tempInstChunk.baseNote      = aiffReadIntBigEndian(file, 1,  &error); if(error) return 0;
  tempInstChunk.detune        = aiffReadIntBigEndian(file, 1,  &error); if(error) return 0;
  tempInstChunk.lowNote       = aiffReadIntBigEndian(file, 1,  &error); if(error) return 0;
  tempInstChunk.highNote      = aiffReadIntBigEndian(file, 1,  &error); if(error) return 0;
  tempInstChunk.lowVelocity   = aiffReadIntBigEndian(file, 1,  &error); if(error) return 0;
  tempInstChunk.highVelocity  = aiffReadIntBigEndian(file, 1,  &error); if(error) return 0;
  tempInstChunk.decibelsGain  = aiffReadIntBigEndian(file, 2,  &error); if(error) return 0;
  tempInstChunk.sustainLoop.playMode      = aiffReadIntBigEndian(file, 2,  &error); if(error) return 0;
  tempInstChunk.sustainLoop.startMarkerID = aiffReadIntBigEndian(file, 2,  &error); if(error) return 0;
  tempInstChunk.sustainLoop.endMarkerID   = aiffReadIntBigEndian(file, 2,  &error); if(error) return 0;
  tempInstChunk.releaseLoop.playMode      = aiffReadIntBigEndian(file, 2,  &error); if(error) return 0;
  tempInstChunk.releaseLoop.startMarkerID = aiffReadIntBigEndian(file, 2,  &error); if(error) return 0;
  tempInstChunk.releaseLoop.endMarkerID   = aiffReadIntBigEndian(file, 2,  &error); if(error) return 0;
  
  #ifdef AIFF_PRINT_DEBUG_MESSAGES
  printf("Instrument chunk detected, pitch: %hhu\n", tempInstChunk.baseNote);
  #endif
  aiffSetupInstrumentInfo(aiff, tempInstChunk.baseNote, tempInstChunk.detune, tempInstChunk.lowNote, tempInstChunk.highNote,
			  tempInstChunk.lowVelocity, tempInstChunk.highVelocity, tempInstChunk.decibelsGain, 
			  tempInstChunk.sustainLoop.playMode, tempInstChunk.sustainLoop.startMarkerID, tempInstChunk.sustainLoop.endMarkerID,
			  tempInstChunk.releaseLoop.playMode, tempInstChunk.releaseLoop.startMarkerID, tempInstChunk.releaseLoop.endMarkerID);
  return 1;
}

//extractMIDIChunkFromFile---------------------------------------------------------------
int aiffExtractMIDIChunkFromFile               (MKAiff* aiff, FILE* file)
{
return 0;
}

//extractRecordingChunkFromFile----------------------------------------------------------
int aiffExtractRecordingChunkFromFile          (MKAiff* aiff, FILE* file)
{
return 0;
}

//extractApplicationChunkFromFile--------------------------------------------------------
int aiffExtractApplicationChunkFromFile        (MKAiff* aiff, FILE* file)
{
return 0;
}

//extractTextChunkFromFile---------------------------------------------------------------
int aiffExtractTextChunkFromFile               (MKAiff* aiff, FILE* file, aiffChunkIdentifier_t chunkID, aiffYesOrNo_t isWave)
{
  aiffYesOrNo_t error;
  int32_t size;
  
  if(isWave)
    {size = (int32_t) aiffReadIntLittleEndian(file, 4,  &error); if(error) return 0;}
  else
    {size = (int32_t) aiffReadIntBigEndian(file, 4,  &error); if(error) return 0;}

 #ifdef AIFF_PRINT_DEBUG_MESSAGES
  //printf("%c%c%c%c chunk detected, %s\n", ((chunkID&0xFF000000)>>24),((chunkID&0x00FF0000)>>16),((chunkID&0x0000FF00)>>8),(chunkID&0x000000FF), text);
  printf("%c%c%c%c chunk detected, size: %i\n", ((chunkID&0xFF000000)>>24),((chunkID&0x00FF0000)>>16),((chunkID&0x0000FF00)>>8),(chunkID&0x000000FF), size);
#endif

  if(size > 0)
  {
    char text[size+1];
    aiffReadBuffer(file, text, size, &error); if(error) return 0;
    text[size] = '\0';
    switch(chunkID)
    {
      case EA_IFF85_NAME_CHUNK_ID:       aiffSetName        (aiff, text); break;
      case EA_IFF85_AUTHOR_CHUNK_ID:     aiffSetAuthor      (aiff, text); break;
      case EA_IFF85_COPYRIGHT_CHUNK_ID:  aiffSetCopyright   (aiff, text); break;
      case EA_IFF85_ANNOTATION_CHUNK_ID: aiffAddAnnotation  (aiff, text); break;
      default:                           aiffAddGenericChunk(aiff, (unsigned char*)text, size, chunkID); break;
    }
  }

  return 1;
}

//extractGenericChunkFromFile------------------------------------------------------------
int aiffExtractGenericChunkFromFile               (MKAiff* aiff, FILE* file, aiffChunkIdentifier_t chunkID, aiffYesOrNo_t isWave)
{
  return aiffExtractTextChunkFromFile(aiff, file, chunkID, isWave);
}

/*----------_-_---_----------------_-------------_---------------------------------------
__ __ ___ _(_) |_(_)_ _  __ _   __| |_ _  _ _ _ | |__ ___
\ V  V / '_| |  _| | ' \/ _` | / _| ' \ || | ' \| / /(_-<
 \_/\_/|_| |_|\__|_|_||_\__, | \__|_||_\_,_|_||_|_\_\/__/
                        |___/                                                                        
---------------------------------------------------------------------------------------*/

//writeFormChunkToFile-------------------------------------------------------------------
void aiffWriteFormChunkToFile(MKAiff* aiff, FILE* file, aiffYesOrNo_t isWave)
{
  aiffYesOrNo_t error;
  mkAiffFormChunk_t formChunk = {AIFF_FORM_CHUNK_ID, /*chunkSize*/46+(aiff->numSamplesWrittenToBuffer)*aiffBytesPerSample(aiff), AIFF_FORM_TYPE_ID};
  if(isWave)
    {
      formChunk.chunkID    = WAVE_FORM_CHUNK_ID;
      formChunk.formTypeID = WAVE_FORM_TYPE_ID;
      formChunk.chunkSize   = 4;     //for the form RIFF chunk
      formChunk.chunkSize += 8 + 16; //for the format chunk
      formChunk.chunkSize += 8 + aiff->numSamplesWrittenToBuffer*aiffBytesPerSample(aiff); //for the data chunk
    }
  else
    {
      if(aiff->commentChunk         != NULL) formChunk.chunkSize += 8+(aiff->commentChunk->chunkSize);
      if(aiff->markerChunk          != NULL) formChunk.chunkSize += 8+(aiff->markerChunk->chunkSize);
      if(aiff->instrumentChunk      != NULL) formChunk.chunkSize += 28;
      if(aiff->midiChunkList        != NULL) formChunk.chunkSize += aiffNumBytesInList((mkAiffSizableListEntry_t*)(aiff->midiChunkList), 8);
      if(aiff->recordingChunk       != NULL) formChunk.chunkSize += 32;
      if(aiff->applicationChunkList != NULL) formChunk.chunkSize += aiffNumBytesInList((mkAiffSizableListEntry_t*)(aiff->applicationChunkList), 12);
      if(aiff->nameChunk            != NULL) formChunk.chunkSize += 8+(aiff->nameChunk->chunkSize);
      if(aiff->copyrightChunk       != NULL) formChunk.chunkSize += 8+(aiff->copyrightChunk->chunkSize);
      if(aiff->authorChunk          != NULL) formChunk.chunkSize += 8+(aiff->authorChunk->chunkSize);
      if(aiff->annotationChunkList  != NULL) formChunk.chunkSize += aiffNumBytesInList((mkAiffSizableListEntry_t*)(aiff->annotationChunkList), 8);
      if(aiff->unknownChunkList     != NULL) formChunk.chunkSize += aiffNumBytesInList((mkAiffSizableListEntry_t*)(aiff->unknownChunkList), 8);
    }

  error = aiffWriteIntBigEndian(file, formChunk.chunkID, 4); if(error) return;

  if(isWave)
    error = aiffWriteIntLittleEndian(file, formChunk.chunkSize, 4);
  else
    error = aiffWriteIntBigEndian(file, formChunk.chunkSize, 4);
  
  if(error) return;
  
  error = aiffWriteIntBigEndian(file, formChunk.formTypeID, 4); if(error) return;
}

//writeCommonChunkToFile-----------------------------------------------------------------
void aiffWriteCommonChunkToFile(MKAiff* aiff, FILE* file)
{
  aiffYesOrNo_t error;
  mkAiffCommonChunk_t tempCommonChunk = aiff->commonChunk;
  tempCommonChunk.numSampleFrames = (aiff->numSamplesWrittenToBuffer) / (aiff->commonChunk.numChannels);
  
  error = aiffWriteIntBigEndian(file, tempCommonChunk.chunkID        , 4); if(error) return;
  error = aiffWriteIntBigEndian(file, tempCommonChunk.chunkSize      , 4); if(error) return;
  error = aiffWriteIntBigEndian(file, tempCommonChunk.numChannels    , 2); if(error) return;
  error = aiffWriteIntBigEndian(file, tempCommonChunk.numSampleFrames, 4); if(error) return;
  error = aiffWriteIntBigEndian(file, tempCommonChunk.bitsPerSample  , 2); if(error) return;
  error = aiffWriteBuffer      (file, tempCommonChunk.sampleRate     ,10); if(error) return;
}

//extractCommonChunkFromFile-------------------------------------------------------------
void aiffWriteWaveFormatChunkToFile(MKAiff* aiff, FILE* file)
{
  aiffYesOrNo_t error;
  mkAiffWaveFormatChunk_t chunk;
  
  chunk.chunkID     = WAVE_FMT_CHUNK_ID;
  chunk.chunkSize   = 16;
  chunk.audioFormat = 1;
  chunk.numChannels = aiffNumChannels(aiff);
  chunk.sampleRate  = aiffSampleRate(aiff);
  chunk.byteRate    = chunk.sampleRate * chunk.numChannels * aiff->commonChunk.bitsPerSample / 8;
  chunk.blockAlign  = chunk.numChannels * aiff->commonChunk.bitsPerSample / 8;
  chunk.bitsPerSample = aiff->commonChunk.bitsPerSample;
  
  error = aiffWriteIntBigEndian   (file, chunk.chunkID     , 4); if(error) return;
  error = aiffWriteIntLittleEndian(file, chunk.chunkSize   , 4); if(error) return;
  error = aiffWriteIntLittleEndian(file, chunk.audioFormat , 2); if(error) return;
  error = aiffWriteIntLittleEndian(file, chunk.numChannels , 2); if(error) return;
  error = aiffWriteIntLittleEndian(file, chunk.sampleRate  , 4); if(error) return;
  error = aiffWriteIntLittleEndian(file, chunk.byteRate    , 4); if(error) return;
  error = aiffWriteIntLittleEndian(file, chunk.blockAlign  , 2); if(error) return;
  error = aiffWriteIntLittleEndian(file, chunk.bitsPerSample,2); if(error) return;
}

//writeSoundChunkAndSoundBufferToFile----------------------------------------------------
void aiffWriteSoundChunkAndSoundBufferToFile(MKAiff* aiff, FILE* file)
{
  aiffYesOrNo_t error;
  int bytesPerSample = aiffBytesPerSample(aiff);
  int bitsPerSample  = aiffBitsPerSample(aiff);
  mkAiffSoundChunk_t tempSoundChunk = aiff->soundChunk;
  tempSoundChunk.chunkSize = 8+(aiff->numSamplesWrittenToBuffer)*bytesPerSample;
  
  error = aiffWriteIntBigEndian   (file, tempSoundChunk.chunkID   , 4); if(error) return;
  error = aiffWriteIntBigEndian   (file, tempSoundChunk.chunkSize , 4); if(error) return;
  error = aiffWriteIntBigEndian   (file, tempSoundChunk.offset    , 4); if(error) return;
  error = aiffWriteIntBigEndian   (file, tempSoundChunk.blockSize , 4); if(error) return;
  
  int32_t i, sample, *bufferPointer = aiff->soundBuffer;

  for(i=0; i<aiff->numSamplesWrittenToBuffer; i++)
    {
      sample = *bufferPointer++;
      sample >>= (32-bitsPerSample);
      error = aiffWriteIntBigEndian(file, sample, bytesPerSample);
      if(error) return;
    }
}

//aiffWriteWaveDataChunkToFile---------------------------------------------------------
void aiffWriteWaveDataChunkToFile(MKAiff* aiff, FILE* file)
{
  aiffYesOrNo_t error;
  int bytesPerSample = (int)aiffBytesPerSample(aiff);
  int bitsPerSample  = aiffBitsPerSample(aiff);
  
  mkAiffWaveDataChunk_t chunk;
  chunk.chunkID = WAVE_DATA_CHUNK_ID;
  chunk.chunkSize    = (aiff->numSamplesWrittenToBuffer)*bytesPerSample;;
  
  error = aiffWriteIntBigEndian   (file, chunk.chunkID   , 4); if(error) return;
  error = aiffWriteIntLittleEndian(file, chunk.chunkSize , 4); if(error) return;
  
  int32_t i, sample, *bufferPointer = aiff->soundBuffer;
  
  for(i=0; i<aiff->numSamplesWrittenToBuffer; i++)
    {
      sample = *bufferPointer++;
      sample >>= (32-bitsPerSample);
      error = aiffWriteIntLittleEndian(file, sample, bytesPerSample);
      if(error) return;
    }
}

//writeCommentChunkToFile----------------------------------------------------------------
void aiffWriteCommentChunkToFile(MKAiff* aiff, FILE* file)
{
  if(aiff->commentChunk!=NULL)
    {
      aiffYesOrNo_t error;
      mkAiffCommentChunk_t tempCommentChunk = *(aiff->commentChunk);
    
      error = aiffWriteIntBigEndian   (file, tempCommentChunk.chunkID    , 4); if(error) return;
      error = aiffWriteIntBigEndian   (file, tempCommentChunk.chunkSize  , 4); if(error) return;
      error = aiffWriteIntBigEndian   (file, tempCommentChunk.numComments, 2); if(error) return;
    
      int i;
      mkAiffComment_t tempComment = *(aiff->commentChunk->firstComment);
      for(i=0; i<aiff->commentChunk->numComments; i++)
        {
          error = aiffWriteIntBigEndian   (file, tempComment.timeStamp , 4);  if(error) return;
          error = aiffWriteIntBigEndian   (file, tempComment.markerID  , 2);  if(error) return;
          error = aiffWriteIntBigEndian   (file, tempComment.numChars  , 2);  if(error) return;
          error = aiffWriteBuffer         (file, tempComment.text, tempComment.numChars); if(error) return;
          if(tempComment.numChars & 0x01)  //if numChars is odd, pad the end with a random byte
            aiffWriteIntBigEndian   (file, 0, 1);
    
          if(tempComment.next != NULL) tempComment = *((mkAiffComment_t*)tempComment.next);
        }
    }
}

//writeMarkerChunkToFile-----------------------------------------------------------------
void aiffWriteMarkerChunkToFile(MKAiff* aiff, FILE* file)
{
  if(aiff->markerChunk!=NULL)
    {
      aiffYesOrNo_t error;
      mkAiffMarkerChunk_t tempMarkerChunk = *(aiff->markerChunk);
    
      error = aiffWriteIntBigEndian   (file, tempMarkerChunk.chunkID    , 4); if(error) return;
      error = aiffWriteIntBigEndian   (file, tempMarkerChunk.chunkSize  , 4); if(error) return;
      error = aiffWriteIntBigEndian   (file, tempMarkerChunk.numMarkers , 2); if(error) return;

      int i;
      mkAiffMarker_t tempMarker = *(aiff->markerChunk->firstMarker);
      for(i=0; i<aiff->markerChunk->numMarkers; i++)
        {
          unsigned char size = strlen(tempMarker.name);
        
           error = aiffWriteIntBigEndian   (file, tempMarker.markerID         , 2); if(error) return;
           error = aiffWriteIntBigEndian   (file, tempMarker.positionInFrames , 4); if(error) return;
           error = aiffWriteIntBigEndian   (file, size                        , 1); if(error) return;
           error = aiffWriteBuffer         (file, tempMarker.name          , size); if(error) return;

          if((tempMarker.name[0]+1) & 0x01)  //if total size is odd, pad the end with a random byte
	          aiffWriteIntBigEndian   (file, 0, 1);;

          if(tempMarker.next != NULL) tempMarker = *((mkAiffMarker_t*)tempMarker.next);
        }
    }
}

//writeInstrumentChunkToFile-------------------------------------------------------------
void aiffWriteInstrumentChunkToFile(MKAiff* aiff, FILE* file)
{
  aiffYesOrNo_t error;
  mkAiffInstrumentChunk_t tempInstChunk = *(aiff->instrumentChunk);
  
  error = aiffWriteIntBigEndian   (file, tempInstChunk.chunkID        , 4); if(error) return;
  error = aiffWriteIntBigEndian   (file, tempInstChunk.chunkSize      , 4); if(error) return;
  error = aiffWriteIntBigEndian   (file, tempInstChunk.baseNote       , 1); if(error) return;
  error = aiffWriteIntBigEndian   (file, tempInstChunk.detune         , 1); if(error) return;
  error = aiffWriteIntBigEndian   (file, tempInstChunk.lowNote        , 1); if(error) return;
  error = aiffWriteIntBigEndian   (file, tempInstChunk.highNote       , 1); if(error) return;
  error = aiffWriteIntBigEndian   (file, tempInstChunk.lowVelocity    , 1); if(error) return;
  error = aiffWriteIntBigEndian   (file, tempInstChunk.highVelocity   , 1); if(error) return;
  error = aiffWriteIntBigEndian   (file, tempInstChunk.decibelsGain   , 1); if(error) return;
  error = aiffWriteIntBigEndian   (file, tempInstChunk.sustainLoop.playMode        , 2); if(error) return;
  error = aiffWriteIntBigEndian   (file, tempInstChunk.sustainLoop.startMarkerID   , 2); if(error) return;
  error = aiffWriteIntBigEndian   (file, tempInstChunk.sustainLoop.endMarkerID     , 2); if(error) return;
  error = aiffWriteIntBigEndian   (file, tempInstChunk.releaseLoop.playMode        , 2); if(error) return;
  error = aiffWriteIntBigEndian   (file, tempInstChunk.releaseLoop.startMarkerID   , 2); if(error) return;
  error = aiffWriteIntBigEndian   (file, tempInstChunk.releaseLoop.endMarkerID     , 2); if(error) return;

}

//writeMIDIChunksToFile------------------------------------------------------------------
void aiffWriteMIDIChunksToFile(MKAiff* aiff, FILE* file)
{
  
}

//writeRecordingChunkToFile--------------------------------------------------------------
void aiffWriteRecordingChunkToFile(MKAiff* aiff, FILE* file)
{
    
}

//writeApplicationChunksChunkToFile------------------------------------------------------
void aiffWriteApplicationChunksChunkToFile(MKAiff* aiff, FILE* file)    
{
    
}

//writeTextChunkChunkToFile--------------------------------------------------------------
void aiffWriteGenericChunkToFile(MKAiff* aiff, FILE* file, mkAiffGenericChunk_t* genericChunk)
{
  aiffYesOrNo_t error;
  error = aiffWriteIntBigEndian   (file, genericChunk->chunkID    , 4); if(error) return;
  error = aiffWriteIntBigEndian   (file, genericChunk->chunkSize  , 4); if(error) return;
  error = aiffWriteBuffer         (file, genericChunk->data, genericChunk->chunkSize); if(error) return;
}

//writeAnnotationChunksToFile------------------------------------------------------------
void aiffWriteAnnotationChunksToFile(MKAiff* aiff, FILE* file) 
{
  mkAiffGenericChunk_t* textChunk = (mkAiffGenericChunk_t*)aiff->annotationChunkList;
  while(textChunk != NULL)
  {
    aiffWriteGenericChunkToFile(aiff, file, textChunk);
    textChunk = (mkAiffGenericChunk_t*)textChunk->next;
  }
}

//writeGenericChunksToFile---------------------------------------------------------------
void aiffWriteGenericChunksToFile(MKAiff* aiff, FILE* file)
{
  mkAiffGenericChunk_t* genericChunk = (mkAiffGenericChunk_t*)aiff->unknownChunkList;
  while(genericChunk != NULL)
  {
    aiffWriteGenericChunkToFile(aiff, file,  genericChunk);
    genericChunk = (mkAiffGenericChunk_t*)genericChunk->next;
  }  
}



/*--_--------_----------------_----------------_-------------_---------------------------
 __| |___ __| |_ _ _ ___ _  _(_)_ _  __ _   __| |_ _  _ _ _ | |__ ___
/ _` / -_|_-<  _| '_/ _ \ || | | ' \/ _` | / _| ' \ || | ' \| / /(_-<
\__,_\___/__/\__|_| \___/\_, |_|_||_\__, | \__|_||_\_,_|_||_|_\_\/__/
                         |__/       |___/                            
---------------------------------------------------------------------------------------*/


//removeAllComments----------------------------------------------------------------------
void aiffRemoveAllComments(MKAiff* aiff)
{
  if(aiff->commentChunk == NULL) return;
  
  int i;
  mkAiffComment_t* comment = aiff->commentChunk->firstComment;
  for(i=0; i<aiff->commentChunk->numComments; i++)
    {
      if(comment->previous != NULL) free((mkAiffComment_t*)(comment->previous));
      free(comment->text);
      if(comment->next != NULL) comment = (mkAiffComment_t*)(comment->next);
    }
  free(comment);
  free(aiff->commentChunk);
  aiff->commentChunk = NULL;
}

//removeAllMarkers-----------------------------------------------------------------------
void aiffRemoveAllMarkers(MKAiff* aiff)
{
  if(aiff->markerChunk == NULL) return;
  
  int i;
  mkAiffMarker_t* marker = aiff->markerChunk->firstMarker;
  for(i=0; i<aiff->markerChunk->numMarkers; i++)
    {
      if(marker->previous != NULL) free((mkAiffMarker_t*)(marker->previous));
      free(marker->name);
      if(marker->next != NULL) marker = (mkAiffMarker_t*)(marker->next);
    }
  free(marker);
  free(aiff->markerChunk);
  aiff->markerChunk = NULL;
}

//removeInstrumentInfo-------------------------------------------------------------------
void aiffRemoveInstrumentInfo(MKAiff* aiff)
{
  if(aiff->instrumentChunk != NULL)
    {
      free(aiff->instrumentChunk);
      aiff->instrumentChunk = NULL;
    }
}

//destroyGenericChunk----------------------------------------------------------------------
void aiffDestroyGenericChunk(mkEAiff85TextChunk_t* genericChunk)
{
  if (genericChunk != NULL)
  {
    if(genericChunk->data != NULL)
      free(genericChunk->data);
    
    free(genericChunk);
  }
}

//removeName-----------------------------------------------------------------------------
void aiffRemoveName(MKAiff* aiff)
{
  aiffDestroyGenericChunk((mkAiffGenericChunk_t*)(aiff->nameChunk));
  aiff->nameChunk = NULL;
}

//removeAuthor---------------------------------------------------------------------------
void aiffRemoveAuthor(MKAiff* aiff)
{
  aiffDestroyGenericChunk((mkAiffGenericChunk_t*)(aiff->authorChunk));
  aiff->authorChunk = NULL;
}

//removeCopyright----------------------------------------------------------------------
void aiffRemoveCopyright(MKAiff* aiff)
{
  aiffDestroyGenericChunk((mkAiffGenericChunk_t*)(aiff->copyrightChunk));
  aiff->copyrightChunk = NULL;
}

//removeAllAnnotations-------------------------------------------------------------------
void aiffRemoveAllAnnotations(MKAiff* aiff)
{
  mkAiffGenericChunk_t* textChunk = (mkAiffGenericChunk_t*)(aiff->annotationChunkList);
  mkAiffGenericChunk_t* tempTextChunk;
  while(textChunk != NULL)
  {
    tempTextChunk = (mkAiffGenericChunk_t*)textChunk->next;
    aiffDestroyGenericChunk(textChunk);
    textChunk = tempTextChunk;
  }
  aiff->annotationChunkList = NULL;
}

//removeAllGenericChunks-----------------------------------------------------------------
void aiffRemoveAllGenericChunks(MKAiff* aiff)
{
  mkAiffGenericChunk_t* genericChunk = (aiff->unknownChunkList);
  mkAiffGenericChunk_t* tempGenericChunk;
  while(genericChunk != NULL)
  {
    tempGenericChunk = (mkAiffGenericChunk_t*)genericChunk->next;
    aiffDestroyGenericChunk(genericChunk);
    genericChunk = tempGenericChunk;
  }
  aiff->unknownChunkList = NULL;
}

/*---------------_---_----------------_-------------_------------------------------------
 __ _ _ ___ __ _| |_(_)_ _  __ _   __| |_ _  _ _ _ | |__ ___
/ _| '_/ -_) _` |  _| | ' \/ _` | / _| ' \ || | ' \| / /(_-<
\__|_| \___\__,_|\__|_|_||_\__, | \__|_||_\_,_|_||_|_\_\/__/
                           |___/                            
---------------------------------------------------------------------------------------*/


void aiffAppendEntryToList(mkAiffListEntry_t* newEntry, mkAiffListEntry_t* firstEntry)
{
  mkAiffListEntry_t* thisEntry = firstEntry;
  while (thisEntry->next != NULL) 
    {
      thisEntry = (mkAiffListEntry_t*) (thisEntry->next);
    }
  thisEntry->next = newEntry;
  newEntry->previous = thisEntry;
  newEntry->next = NULL;
}

int aiffNumBytesInList(mkAiffSizableListEntry_t* listEntry, int numBytesPerEntryNotCountedInChunkSize)
{
  int count = 0;
  while(listEntry != NULL)
  {
    count += numBytesPerEntryNotCountedInChunkSize;
    count += listEntry->chunkSize;
    listEntry = (mkAiffSizableListEntry_t*)listEntry->next;
  }
  return count;
}

//createTextChunk------------------------------------------------------------------------
mkEAiff85TextChunk_t* aiffCreateTextChunk(aiffChunkIdentifier_t chunkID, char* text)
{
  mkEAiff85TextChunk_t* textChunk = (mkEAiff85TextChunk_t*)malloc(sizeof(*textChunk));
  textChunk->chunkID = chunkID;
  //the spec dosent include the '\0' but I'm putting it anyhow
  //this could be a dangerous practice
  textChunk->chunkSize = strlen(text) + 1;
  textChunk->data = (char*)malloc(textChunk->chunkSize);
  
  int i;
  for(i=0; i<textChunk->chunkSize; i++)
    (textChunk->data)[i] = text[i];
  
  return textChunk; 
}

/*---------_----_-_----------------------------------------------------------------------
 _ __ _  _| |__| (_)__                                   
| '_ \ || | '_ \ | / _|                                  
| .__/\_,_|_.__/_|_\__|                                  
|_|                                                                                                  
---------------------------------------------------------------------------------------*/                                            
                                             


//aiffWithContentsOfFile-----------------------------------------------------------------
MKAiff* aiffWithContentsOfFile(char* filename)
{
  /*oh god, what a wreck; graph this function!*/

  MKAiff* aiff = aiffAllocate();
  if(aiff == NULL) return NULL;
  
  FILE* file = fopen(filename, "rb");
  if(file == NULL) {/*fprintf(stderr, "unable to open %s for reading\n", filename);*/ return NULL;}
  
  mkAiffFormChunk_t formChunk;  
  int success = aiffExtractFormChunkFromFile(&formChunk, file);
  if(!success) return NULL;
  
  aiffYesOrNo_t isWave = aiffNo;
  
  if((formChunk.chunkID == AIFF_FORM_CHUNK_ID) && (formChunk.formTypeID == AIFF_FORM_TYPE_ID))
    {isWave = aiffNo; /*fprintf(stderr, "aiff file detected\n");*/}
  else if ((formChunk.chunkID == WAVE_FORM_CHUNK_ID) && (formChunk.formTypeID == WAVE_FORM_TYPE_ID))
    {isWave = aiffYes; /*fprintf(stderr, "WAVE file detected\n");*/}
  else return NULL;
  
  while(aiffYes)
    {
      aiffYesOrNo_t error;
      aiffChunkIdentifier_t chunkID = aiffReadIntBigEndian(file, 4, &error);
      if(error) break;
    
      switch(chunkID)
        {
        case WAVE_FMT_CHUNK_ID: success = aiffExtractWaveFormatChunkFromFile(aiff, file);
          if(!success){/*fprintf(stderr, "unable to parse Wave Format Chunk\n");*/ goto read_error;}
          break;
        case WAVE_DATA_CHUNK_ID: success = aiffExtractWaveDataChunkFromFile(aiff, file);
          if(!success){/*fprintf(stderr, "unable to parse Wave Data Chunk\n");*/ goto read_error;}
          break;
        case AIFF_COMMON_CHUNK_ID: success = aiffExtractCommonChunkFromFile(aiff, file);
          if(!success){/*fprintf(stderr, "unable to parse Common Chunk\n");*/ goto read_error;} 
          break;        
        case AIFF_SOUND_CHUNK_ID: success = aiffExtractSoundChunkAndSoundBufferFromFile(aiff, file);
          if(!success){/*fprintf(stderr, "unable to parse Sound Chunk\n");*/ goto read_error;} 
          break;
        case AIFF_MARKER_CHUNK_ID: success = aiffExtractMarkerChunkFromFile(aiff, file); 
          if(!success){/*fprintf(stderr, "unable to parse Marker Chunk\n");*/ goto read_error;} 
          break;
        case AIFF_INSTRUMENT_CHUNK_ID: success = aiffExtractInstrumentChunkFromFile(aiff, file); 
          if(!success){/*fprintf(stderr, "unable to parse Instrument Chunk\n");*/ goto read_error;} 
          break;
          //case AIFF_MIDI_CHUNK_ID: success = aiffExtractMIDIChunkFromFile(aiff, file); 
          //if(!success){/*fprintf(stderr, "unable to parse MIDI Chunk\n");*/ goto read_error;} 
          //break;
          //case AIFF_RECORDING_CHUNK_ID: success = aiffExtractRecordingChunkFromFile(aiff, file); 
          //if(!success){/*fprintf(stderr, "unable to parse Recording Chunk\n");*/ goto read_error;} 
          //break;
          //case AIFF_APPLICATION_CHUNK_ID: success = aiffExtractApplicationChunkFromFile(aiff, file); 
          //if(!success){/*fprintf(stderr, "unable to parse Application Chunk\n");*/ goto read_error;} 
          //break;  
        case AIFF_COMMENT_CHUNK_ID: success = aiffExtractCommentChunkFromFile(aiff, file); 
          if(!success){/*fprintf(stderr, "unable to parse Comment Chunk\n");*/ goto read_error;} 
          break;
        case EA_IFF85_NAME_CHUNK_ID: 
        case EA_IFF85_AUTHOR_CHUNK_ID: 
        case EA_IFF85_COPYRIGHT_CHUNK_ID:
        case EA_IFF85_ANNOTATION_CHUNK_ID: success = aiffExtractTextChunkFromFile(aiff, file, chunkID, isWave);
          if(!success){/*fprintf(stderr, "unable to parse Text Chunk\n");*/ goto read_error;} 
          break;
        default:
          success = aiffExtractGenericChunkFromFile(aiff, file, chunkID, isWave);
          if(!success){/*fprintf(stderr, "unable to parse generic Chunk\n");*/ goto read_error;}
          break;
        }
    }

  aiffRewindPlayheadToBeginning(aiff);
  aiffSetName(aiff, filename);
  fclose(file);  
  return aiff;
  
  read_error:
    fclose(file);
    return aiffDestroy(aiff);
} 

//aiffWithDurationInSeconds--------------------------------------------------------------
MKAiff* aiffWithDurationInSeconds(int16_t numChannels, unsigned long sampleRate, int16_t bitsPerSample, int numSeconds)
{
  int numSamples = sampleRate * numSeconds * numChannels;
  return aiffWithDurationInSamples(numChannels, sampleRate, bitsPerSample, numSamples);
}

//aiffWithDurationInFrames---------------------------------------------------------------
MKAiff* aiffWithDurationInFrames (int16_t numChannels, unsigned long sampleRate, int16_t bitsPerSample, int numFrames)
{
  int numSamples = numFrames * numChannels;
  return aiffWithDurationInSamples(numChannels, sampleRate, bitsPerSample, numSamples);
}

//aiffWithDurationInSamples--------------------------------------------------------------
MKAiff* aiffWithDurationInSamples(int16_t numChannels, unsigned long sampleRate, int16_t bitsPerSample, int numSamples)
{
  MKAiff* aiff = aiffAllocate();
  if(aiff != NULL)
    {
      mkAiffCommonChunk_t tempCommonChunk = {AIFF_COMMON_CHUNK_ID, /*chunkSize*/18, numChannels, /*numSampleFrames*/0, bitsPerSample, /*sampleRate*/};
      aiff->commonChunk = tempCommonChunk;
      aiffSetSampleRate(aiff, sampleRate); 
      
      mkAiffSoundChunk_t tempSoundChunk = {AIFF_SOUND_CHUNK_ID, /*chunkSize*/8, /*offset*/0, /*blockSize*/0};
      aiff->soundChunk = tempSoundChunk;

      int success = aiffAllocateSoundBuffer(aiff, numSamples);
      if(!success) {aiffDestroy(aiff); return(NULL);}
    }
  return aiff;
}

//aiffAllocate----------------------------------------------------------------------------
MKAiff* aiffAllocate()
{
  MKAiff* aiff = (MKAiff*)calloc(1, sizeof(*aiff));
  return aiff;
}

//aiffNewMono----------------------------------------------------------------------------
MKAiff* aiffNewMono(MKAiff* aiff)
{
  int numFrames   = aiffDurationInFrames(aiff);
  int numChannels = aiffNumChannels(aiff);
  MKAiff* monoAiff = aiffWithDurationInFrames(1, aiffSampleRate(aiff), aiffBitsPerSample(aiff), numFrames);
  if(monoAiff != NULL)
    {
      aiffRewindPlayheadToBeginning(aiff);

      int i;
      int32_t frame[numChannels];
      int32_t monoFrame;
      while(numFrames--)
        {
           monoFrame = 0;
           aiffReadIntegerSamplesAtPlayhead(aiff, frame, numChannels, aiffYes);
           for(i=0; i<numChannels; i++)
             monoFrame += frame[i] / numChannels;
           aiffAppendIntegerSamples(monoAiff, &monoFrame, 1, 4, 32, aiffNo, aiffYes);
        }
      aiffRewindPlayheadToBeginning(monoAiff);
    }
  return monoAiff;
}

//aiffMakeMono---------------------------------------------------------------------------
void             aiffMakeMono(MKAiff* aiff)
{
  int numChannels = aiffNumChannels(aiff);
  if(numChannels > 1)
    {
      unsigned i, duration = aiffDurationInFrames(aiff);
      int32_t *in, *out;
      in = out = aiff->soundBuffer;
      float oneOverNumChannels = 1.0 / numChannels;
      
      while(duration-- > 0)
        {
          *out = *in / numChannels;
          for(i=1; i<numChannels; i++)
            out[0] += in[i] * oneOverNumChannels;
          out++;
          in += numChannels;
        }
      aiff->numSamplesWrittenToBuffer *= oneOverNumChannels;
      aiff->commonChunk.numChannels = 1;
    }
    aiffRewindPlayheadToBeginning(aiff);
}

//allocateSoundBuffer--------------------------------------------------------------------
int aiffAllocateSoundBuffer(MKAiff* aiff, int numSamples)
{
  //always keep at least one unaccounted byte at end!
  if((aiff->soundBuffer) == NULL)
    {
      aiff->soundBuffer = (int32_t*)calloc(numSamples+1, 4);
      if(aiff->soundBuffer == NULL) return 0;
      aiffRewindPlayheadToBeginning(aiff);
      aiff->numSamplesWrittenToBuffer = 0;
      aiff->bufferCapacityInSamples = numSamples; 
    }
  else
    {
      #ifdef AIFF_PRINT_DEBUG_MESSAGES
      printf("reallocating sound buffer (because it is going to overflow)\n");
      #endif //AIFF_PRINT_DEBUG_MESSAGES
      int currentPlayheadPosition = aiffPlayheadPositionInSamples(aiff);
      aiff->soundBuffer = (int32_t*)realloc(aiff->soundBuffer, (numSamples+1)*4);
      if(aiff->soundBuffer == NULL) return 0;
      aiff->bufferCapacityInSamples = numSamples;
      aiffSetPlayheadToSamples(aiff, currentPlayheadPosition);
    }
  return 1;
}

//numBytesInFloatType--------------------------------------------------------------------
int aiffNumBytesInFloatType(mkAiffFloatingPointSampleType_t floatType)
{
  int bytesPerSample;
  switch (floatType) 
    {
      case aiffFloatSampleType : bytesPerSample = sizeof(float ); break;
      case aiffDoubleSampleType: bytesPerSample = sizeof(double); break;
      default:                   bytesPerSample = 4;              break;
  }
  return bytesPerSample;
}

//appendIntegerSamples-------------------------------------------------------------------
void aiffAppendIntegerSamples(      MKAiff*       aiff, 
                                    void*         buffer, 
                                    int           numSamples, 
                                    int           bytesPerSample, 
                                    int           bitsPerSample, 
                                    aiffYesOrNo_t leftAligned, 
                                    aiffYesOrNo_t isSigned)
{
  aiffFastForwardPlayheadToEnd(aiff);
  aiffAddSamplesAtPlayhead(aiff, buffer, numSamples, /*isFloat*/aiffNo, aiffNotFloatingPointSampleType, 
                            bytesPerSample, bitsPerSample, leftAligned, isSigned, /*overwrite*/aiffYes);
}

//appendFloatingPointSamples-------------------------------------------------------------
void aiffAppendFloatingPointSamples(MKAiff* aiff, 
                                    void* buffer, 
                                    int numSamples, 
                                    mkAiffFloatingPointSampleType_t floatType)
{

  aiffFastForwardPlayheadToEnd(aiff);
  aiffAddSamplesAtPlayhead(aiff, buffer, numSamples, /*isFloat*/aiffYes, floatType, 
                          /*bytesPerSample*/aiffNumBytesInFloatType(floatType), 
                          /*bitsPerSample*/32, /*leftAligned*/aiffNo, /*isSigned*/aiffYes, 
                          /*overwrite*/aiffYes);
}

//---------------------------------------------------------------------------------------
int aiffAppendSamplesAtPlayheadFromAiff(MKAiff* aiff, MKAiff* fromAiff, int numSamples)
{
  int duration = aiffDurationInSamples(fromAiff);
  int position = aiffPlayheadPositionInSamples(fromAiff);
  if((position + numSamples) > duration)
    numSamples = duration - position;
  
  aiffFastForwardPlayheadToEnd(aiff);
  return aiffAddSamplesAtPlayhead(aiff, fromAiff->playhead, numSamples, /*isFloat*/aiffNo,
                                  aiffNotFloatingPointSampleType, 4, 32, aiffYes, aiffYes, aiffYes);
}

//---------------------------------------------------------------------------------------
void aiffAppendSilence(MKAiff* aiff, int num_samples)
{
  int i;
  aiffFastForwardPlayheadToEnd(aiff);
  uint32_t silence = 0;
  for(i=0; i<num_samples; i++)
      aiffAddSamplesAtPlayhead(aiff, &silence, 1, /*isFloat*/aiffNo, aiffNotFloatingPointSampleType,
                                    4, 32, aiffYes, aiffYes, aiffYes);
}

//---------------------------------------------------------------------------------------
void aiffAppendSilenceInSeconds(MKAiff* aiff, float seconds)
{
  int num_samples = aiffSampleRate(aiff) * seconds;
  aiffAppendSilence(aiff, num_samples);
  
}

//addIntegerSamplesAtPlayhead------------------------------------------------------------
int aiffAddIntegerSamplesAtPlayhead(MKAiff* aiff, 
                                    void* buffer, 
                                    int numSamples, 
                                    int bytesPerSample, 
                                    int bitsPerSample, 
                                    aiffYesOrNo_t leftAligned, 
                                    aiffYesOrNo_t isSigned, 
                                    aiffYesOrNo_t overwrite)
{
    return aiffAddSamplesAtPlayhead(aiff, buffer, numSamples, /*isFloat*/aiffNo, aiffNotFloatingPointSampleType, 
                                    bytesPerSample, bitsPerSample, leftAligned, isSigned, overwrite);
}

//addFloatingPointSamplesAtPlayhead------------------------------------------------------                                    
int aiffAddFloatingPointSamplesAtPlayhead(MKAiff*                     aiff, 
                                          void*                       buffer, 
                                          int                         numSamples, 
                                          mkAiffFloatingPointSampleType_t floatType,
                                          aiffYesOrNo_t               overwrite)
{
  return aiffAddSamplesAtPlayhead(aiff, buffer, numSamples, /*isFloat*/aiffYes, floatType, 
                                 /*bytesPerSample*/aiffNumBytesInFloatType(floatType), 
                                 /*bitsPerSample*/32, /*leftAligned*/aiffNo, /*isSigned*/aiffYes, 
                                 overwrite);
}

                              
//---------------------------------------------------------------------------------------
int aiffAddSamplesAtPlayheadFromAiff(MKAiff* aiff, MKAiff* fromAiff, int numSamples, aiffYesOrNo_t overwrite)
{
  int duration = aiffDurationInSamples(fromAiff);
  int position = aiffPlayheadPositionInSamples(fromAiff);
  if((position + numSamples) > duration)
    numSamples = duration - position;
  
  return aiffAddSamplesAtPlayhead(aiff, fromAiff->playhead, numSamples, /*isFloat*/aiffNo,
                                  aiffNotFloatingPointSampleType, 4, 32, aiffYes, aiffYes, overwrite);
}


//addSamplesAtPlayhead-------------------------------------------------------------------
int aiffAddSamplesAtPlayhead(MKAiff*                        aiff, 
                            void*                           buffer, 
                            int                             numSamples, 
                            aiffYesOrNo_t                   isFloat,
                            mkAiffFloatingPointSampleType_t floatType,
                            int                             bytesPerSample, 
                            int                             bitsPerSample, 
                            aiffYesOrNo_t                   leftAligned, 
                            aiffYesOrNo_t                   isSigned, 
                            aiffYesOrNo_t                   overwrite)                            
{
  //determine if this machine is big endian for handling 24-bit samples
  int test = 1;
  int bigEndian = !(*((char*)&test));
  int ammountToSubtract = (isSigned) ? 0 : ( (leftAligned) ?  0x01 << (bytesPerSample*8-1) : 0x01 << (bitsPerSample-1));
  int shiftAmmount = (leftAligned) ? 32-bytesPerSample*8 : 32-bitsPerSample;
  if((bytesPerSample == 3) && (bigEndian)) {shiftAmmount -= 8; ammountToSubtract <<= 8;}
  int numClippedSamples = 0;
  int32_t nextSample; 
   
  while(numSamples)
    {
      if((aiff->numSamplesWrittenToBuffer) >= (aiff->bufferCapacityInSamples))
        aiffAllocateSoundBuffer(aiff, (aiff->bufferCapacityInSamples)*2); 
      if(isFloat)
        {
          double next=0;
          switch(floatType)
            {
              case aiffFloatSampleType:  next = *(float* )buffer; break;
              case aiffDoubleSampleType: next = *(double*)buffer; break;
              default: break;
            }
          if(next >  1)  {next =  1; numClippedSamples++;}
          if(next < -1)  {next = -1; numClippedSamples++;}
          nextSample = (int32_t)(next * 0x7FFFFFFF);
        }
        
      //oh sweet jesus let this be right...
      else
        {
          switch(bytesPerSample)
          {
            case 1: nextSample = (isSigned) ? ((int32_t)*((int8_t* )buffer))     : (((int32_t)*((uint8_t* )buffer))-ammountToSubtract);  break;
            case 2: nextSample = (isSigned) ? ((int32_t)*((int16_t*)buffer))     : (((int32_t)*((uint16_t*)buffer))-ammountToSubtract);  break;
            case 4: nextSample = (isSigned) ? (         *((int32_t*)buffer))     : (((int32_t)*((uint32_t*)buffer))-ammountToSubtract);  break;
            case 3: if(bigEndian)
              {   nextSample = (isSigned) ? ((*(int32_t*)buffer) & 0xFFFFFF00) : (((int32_t)*(((uint32_t*)buffer)) & 0xFFFFFF00)-ammountToSubtract);  }
            else{ nextSample = (isSigned) ? ((*(int32_t*)buffer) & 0x00FFFFFF) : (((int32_t)*(((uint32_t*)buffer)) & 0x00FFFFFF)-ammountToSubtract);  }
            break;
            default: break;
          }
          nextSample <<= shiftAmmount;
        }
        
      if(overwrite) *(aiff->playhead) = nextSample;
      else
        {
          if(((long)*(aiff->playhead) + (long)nextSample) <= -0x7FFFFFFF)
            {  *(aiff->playhead) = -0x7FFFFFFF; numClippedSamples++;  }
          else if(((long)*(aiff->playhead) + (long)nextSample) >= 0x7FFFFFFF) 
            {  *(aiff->playhead) = 0x7FFFFFFF;  numClippedSamples++;  }
          else *(aiff->playhead) += nextSample;
        }
        
      buffer += bytesPerSample;
      aiffAdvancePlayheadBySamples(aiff, 1);
      if(aiffPlayheadPositionInSamples(aiff) > aiffDurationInSamples(aiff))
        {aiff->numSamplesWrittenToBuffer = aiffPlayheadPositionInSamples(aiff);}
        
      numSamples--;
    }
  return numClippedSamples;
}

//readIntegerSamplesAtPlayhead-----------------------------------------------------------
int aiffReadIntegerSamplesAtPlayhead(MKAiff* aiff, int32_t buffer[], int numSamples, aiffYesOrNo_t shouldOverwrite)
{
  if(aiffPlayheadPositionInSamples(aiff) + numSamples > aiff->numSamplesWrittenToBuffer)
    numSamples = aiff->numSamplesWrittenToBuffer - aiffPlayheadPositionInSamples(aiff);
  
  int i, numSamplesSuccessfullyRead=0;
  for(i=0; i<numSamples; i++)
    {
      if(shouldOverwrite)
        buffer[i] = *(aiff->playhead);
      else 
        buffer[i] += *(aiff->playhead);
      aiffAdvancePlayheadBySamples(aiff, 1);
      numSamplesSuccessfullyRead++;      
    }
  return numSamplesSuccessfullyRead;
}

//readFloatingPointSamplesAtPlayhead-----------------------------------------------------
int aiffReadFloatingPointSamplesAtPlayhead(MKAiff* aiff, float buffer[], int numSamples, aiffYesOrNo_t shouldOverwrite)
{
  if(aiffPlayheadPositionInSamples(aiff) + numSamples > aiff->numSamplesWrittenToBuffer)
    numSamples = aiff->numSamplesWrittenToBuffer - aiffPlayheadPositionInSamples(aiff);

  int i, numSamplesSuccessfullyRead=0;
  
  if(shouldOverwrite)
    for(i=0; i<numSamples; i++)
      {
        buffer[i] = *(aiff->playhead) / (long double)0x7FFFFFFF;
        aiffAdvancePlayheadBySamples(aiff, 1);
        numSamplesSuccessfullyRead++;
      }
  
  else
    for(i=0; i<numSamples; i++)
      {
        buffer[i] += *(aiff->playhead) / (long double)0x7FFFFFFF;
        aiffAdvancePlayheadBySamples(aiff, 1);
        numSamplesSuccessfullyRead++;
      }  

  return numSamplesSuccessfullyRead;
}

//aiffRemoveSamplesAtPlayhead------------------------------------------------------------
void aiffRemoveSamplesAtPlayhead           (MKAiff* aiff, int numSamples)
{
  if(numSamples <= 0) return;
  
  int start    = aiffPlayheadPositionInSamples(aiff);
  int duration = aiffDurationInSamples(aiff);
  int numSamplestoCopy;

  int32_t *head, *tail;
  int i;
  
  if((start+numSamples) > duration)
    numSamples = duration-start;
  
  head = aiff->playhead;
  tail = head + numSamples;
  numSamplestoCopy = duration - numSamples - start;
  
  for(i=0; i<numSamplestoCopy; i++)
    *head++ = *tail++;
  
  aiff->numSamplesWrittenToBuffer -= numSamples;
}

//saveWithFilename-----------------------------------------------------------------------
void aiffSaveWithFilename(MKAiff* aiff, char* filename)
{
  aiffSetName(aiff, filename);
  FILE* file = fopen(filename, "wb+");
  if(file == NULL) {/*fprintf(stderr, "unable to open %s for writing\n", filename);*/ return;}
  
  aiffWriteFormChunkToFile                                                          (aiff, file, aiffNo);
  aiffWriteCommonChunkToFile                                                        (aiff, file);
  if(aiff->numSamplesWrittenToBuffer  !=0) aiffWriteSoundChunkAndSoundBufferToFile  (aiff, file);
  if(aiff->commentChunk               != NULL) aiffWriteCommentChunkToFile          (aiff, file);
  if(aiff->markerChunk                != NULL) aiffWriteMarkerChunkToFile           (aiff, file);
  if(aiff->instrumentChunk            != NULL) aiffWriteInstrumentChunkToFile       (aiff, file);
  if(aiff->midiChunkList              != NULL) aiffWriteMIDIChunksToFile            (aiff, file);
  if(aiff->recordingChunk             != NULL) aiffWriteRecordingChunkToFile        (aiff, file);
  if(aiff->applicationChunkList       != NULL) aiffWriteApplicationChunksChunkToFile(aiff, file);
  if(aiff->nameChunk                  != NULL) aiffWriteGenericChunkToFile          (aiff, file, (mkAiffGenericChunk_t*)aiff->nameChunk);
  if(aiff->authorChunk                != NULL) aiffWriteGenericChunkToFile          (aiff, file, (mkAiffGenericChunk_t*)aiff->authorChunk);
  if(aiff->copyrightChunk             != NULL) aiffWriteGenericChunkToFile          (aiff, file, (mkAiffGenericChunk_t*)aiff->copyrightChunk);
  if(aiff->annotationChunkList        != NULL) aiffWriteAnnotationChunksToFile      (aiff, file);
  if(aiff->unknownChunkList           != NULL) aiffWriteGenericChunksToFile         (aiff, file);
  
  fclose(file);
}

//saveWaveWithFilename-------------------------------------------------------------------
void             aiffSaveWaveWithFilename     (MKAiff* aiff, char* filename)
{
  aiffSetName(aiff, filename);
  FILE* file = fopen(filename, "wb+");
  if(file == NULL) {/*fprintf(stderr, "unable to open %s for writing\n", filename);*/ return;}
  
  aiffWriteFormChunkToFile               (aiff, file, aiffYes);
  aiffWriteWaveFormatChunkToFile         (aiff, file);
  aiffWriteWaveDataChunkToFile           (aiff, file);

  fclose(file);
}

//setSampleRate--------------------------------------------------------------------------
void aiffSetSampleRate(MKAiff* aiff, double r)
{
  ConvertToIeeeExtended(r, aiff->commonChunk.sampleRate);
  /*
  int i = 0;
  
  uint32_t rate = r;
  
  if(r <= 0xFFFF)
    {
      for (i=0; (rate & 0xFFFFFFFF) != 0; rate <<= 1, i++)
        if ((rate & 0x8000) != 0)  
          break;
    }
  else
    {
      for (i=0; (rate & 0xFFFFFFFF) != 0; rate >>= 1, i--)
        if ((rate & 0x8000) != 0)  
          break;
     } 
  
  aiff->commonChunk.sampleRate[1] = 14-i;
  aiff->commonChunk.sampleRate[0] = 0x40;
  
  aiff->commonChunk.sampleRate[3] = rate        & 0xFF;
  aiff->commonChunk.sampleRate[2] = (rate >> 8) & 0xFF;
  
  for(i=4; i<sizeof(float80_t); i++) aiff->commonChunk.sampleRate[i] = 0;
  */
}

//sampleRate-----------------------------------------------------------------------------
double aiffSampleRate(MKAiff* aiff)
{
  return ConvertFromIeeeExtended(aiff->commonChunk.sampleRate);
/*
  unsigned long rate = 0;
  int i = 14 - aiff->commonChunk.sampleRate[1];
  rate = (aiff->commonChunk.sampleRate[2] << 8) | (aiff->commonChunk.sampleRate[3]);
  rate >>= i;

  return rate;
  */
}

//aiffResample---------------------------------------------------------------------------
aiffYesOrNo_t aiffResample (MKAiff* aiff, unsigned long rate, aiffInterpolation_t interp /*currently ignored*/)
{  
  unsigned long currentRate = aiffSampleRate(aiff);
  int numChannels = aiffNumChannels(aiff);
  int32_t *out, *in, *buffer = NULL;
  double increment = (double)currentRate / (double)rate;
  unsigned newNumFrames = aiffDurationInFrames(aiff) / increment;
  unsigned i, j;
  double index = 0;
  double mantissa;
  unsigned int base;
  int32_t s1, s2;
  
  if(currentRate != rate)
    {
      in = out = aiff->soundBuffer;
      
      if(currentRate < rate)
        {
          int numSamples = newNumFrames * numChannels;
          buffer = malloc(numSamples * sizeof(*buffer));
          if(buffer == NULL)
            return aiffNo;
          //else
            out = buffer;
            aiff->bufferCapacityInSamples = numSamples;
        }
        
      for(i=0; i<newNumFrames; i++)
        {
          base = (int)index;
          mantissa = index - base;
          base *= numChannels;
          for(j=0; j<numChannels; j++)
            {
              s1 = in[base + j]; 
              s2 = in[base + numChannels + j];
              *out++ = s1 + (mantissa * s2) - (mantissa * s1) + 0.5;
              //*a++ = s1 + mantissa * (s2 - s1) + 0.5; //refactoring could result in integer overflow
            }
          index += increment;
        }
        
      if(buffer != NULL)
        {
          out = aiff->soundBuffer;  //just reuse a variable
          aiff->soundBuffer = buffer;
          if(out != NULL)
            free(out);
        }

      aiff->numSamplesWrittenToBuffer = newNumFrames * numChannels;
      aiffSetSampleRate(aiff, rate);
    }
    
  aiffRewindPlayheadToBeginning(aiff);
  return aiffYes;
}

//numChannels----------------------------------------------------------------------------
int16_t aiffNumChannels(MKAiff* aiff)
{
  return aiff->commonChunk.numChannels;
}

//setSampleSizeInBits--------------------------------------------------------------------
void aiffSetBitsPerSample(MKAiff* aiff, int16_t numBits)
{
  aiff->commonChunk.bitsPerSample = numBits;
}

//bitsPerSample--------------------------------------------------------------------------
int16_t aiffBitsPerSample(MKAiff* aiff)
{
  return aiff->commonChunk.bitsPerSample;
}

//aiffDurationInSamples------------------------------------------------------------------
int              aiffDurationInSamples        (MKAiff* aiff)
{
  return aiff->numSamplesWrittenToBuffer;
}

//aiffDurationInFrames-------------------------------------------------------------------
int              aiffDurationInFrames         (MKAiff* aiff)
{
  return aiffDurationInSamples(aiff) / aiffNumChannels(aiff);
}

//aiffDurationInSeconds------------------------------------------------------------------
float            aiffDurationInSeconds        (MKAiff* aiff)
{
  return (float)aiffDurationInSamples(aiff) / ((float)aiffSampleRate(aiff) * (float)aiffNumChannels(aiff));
}

//bytesPerSample-------------------------------------------------------------------------
uint16_t aiffBytesPerSample(MKAiff* aiff)
{
  int numBits = (int)aiff->commonChunk.bitsPerSample;
  
  if(numBits <= 8)
    return 1;
  if(numBits <= 16)
    return 2;
  if(numBits <= 24)
    return 3;
  else //(numBits <= 32)
    return 4;
    
  //return 0;
}

//setPlayheadToSeconds-------------------------------------------------------------------
void aiffSetPlayheadToSeconds(MKAiff* aiff, double numSeconds)
{
  int numSamples = numSeconds * aiffSampleRate(aiff) * aiffNumChannels(aiff) + 0.5;
  aiffSetPlayheadToSamples(aiff, numSamples);
}

//setPlayheadToFrames--------------------------------------------------------------------
void aiffSetPlayheadToFrames(MKAiff* aiff, int numFrames)
{
  int numSamples = numFrames * aiffNumChannels(aiff);
  aiffSetPlayheadToSamples(aiff, numSamples);
}

//setPlayheadToSamples-------------------------------------------------------------------
void aiffSetPlayheadToSamples(MKAiff* aiff, int numSamples)
{
  aiffRewindPlayheadToBeginning(aiff);
  aiffAdvancePlayheadBySamples(aiff, numSamples);
}

//advancePlayheadBySeconds---------------------------------------------------------------
void aiffAdvancePlayheadBySeconds(MKAiff* aiff, double numSeconds)
{
  int numSamples = numSeconds * aiffSampleRate(aiff) * aiffNumChannels(aiff) + 0.5;
  aiffAdvancePlayheadBySamples(aiff, numSamples);
}

//advancePlayheadByFrames----------------------------------------------------------------
void aiffAdvancePlayheadByFrames(MKAiff* aiff, int numFrames)
{
  int numSamples = numFrames * aiffNumChannels(aiff);
  aiffAdvancePlayheadBySamples(aiff, numSamples);  
}

//advancePlayheadBySamples---------------------------------------------------------------
void aiffAdvancePlayheadBySamples(MKAiff* aiff, int numSamples)
{
  int offset = aiffPlayheadPositionInSamples(aiff);
  offset += numSamples;
  if(offset <= 0) aiffRewindPlayheadToBeginning(aiff);
  else if(offset > aiff->numSamplesWrittenToBuffer+1) aiffFastForwardPlayheadToEnd(aiff);
  else aiff->playhead += numSamples;
}

//playheadPositionInSeconds--------------------------------------------------------------
double aiffPlayheadPositionInSeconds(MKAiff* aiff)
{
  return aiffPlayheadPositionInSamples(aiff) / (double)( aiffSampleRate(aiff) * aiffNumChannels(aiff) ); 
}

//playheadPositionInFrames---------------------------------------------------------------
int aiffPlayheadPositionInFrames(MKAiff* aiff)
{
  return aiffPlayheadPositionInSamples(aiff) / aiffNumChannels(aiff);
}

//playheadPositionInSamples--------------------------------------------------------------
int aiffPlayheadPositionInSamples(MKAiff* aiff)
{
  uint64_t offset = (uint64_t)aiff->playhead - (uint64_t)aiff->soundBuffer;
  offset /= 4;
  //printf("offset: %u, bufferSize: %u\n", (unsigned int)offset, (unsigned int)aiff->bufferCapacityInSamples);
  return offset;
}

//rewindPlayheadToBeginning--------------------------------------------------------------
void aiffRewindPlayheadToBeginning(MKAiff* aiff)
{
  aiff->playhead = aiff->soundBuffer;
}

//fastForwardPlayheadToEnd---------------------------------------------------------------
void aiffFastForwardPlayheadToEnd(MKAiff* aiff)
{
  //printf("%i, %i\n", aiff->numSamplesWrittenToBuffer, aiff->bufferCapacityInSamples);
  aiffRewindPlayheadToBeginning(aiff);
  aiff->playhead += aiff->numSamplesWrittenToBuffer;
}

//addComment-----------------------------------------------------------------------------
void aiffAddCommentWithText(MKAiff* aiff, char text[], mkAiffMarkerID_t marker)
{
  if(aiff->commentChunk == NULL)
    {
      aiff->commentChunk = (mkAiffCommentChunk_t*)malloc(sizeof(*(aiff->commentChunk)));
      aiff->commentChunk->chunkID = AIFF_COMMENT_CHUNK_ID;
      aiff->commentChunk->chunkSize = 2; //two bytes for sizeof(aiff->commentChunk->numComments) 
      aiff->commentChunk->numComments = 0;
      aiff->commentChunk->firstComment = NULL;
    }
  mkAiffComment_t* newComment = (mkAiffComment_t*)malloc(sizeof(*newComment));
  newComment->timeStamp = 0;//(uint32_t)time(NULL);
  newComment->markerID = marker;
  newComment->numChars = strlen(text);
  newComment->text = (char*)malloc(newComment->numChars);
  int i;
  for(i=0; i<newComment->numChars; i++) 
    {  newComment->text[i] = text[i];  }
  
  if(aiff->commentChunk->firstComment == NULL)
    {
      aiff->commentChunk->firstComment = newComment;
      newComment->previous = NULL; newComment->next = NULL;
    } 
  else aiffAppendEntryToList((mkAiffListEntry_t*)newComment, (mkAiffListEntry_t*)aiff->commentChunk->firstComment);
  
  aiff->commentChunk->chunkSize += 8;  //for first three variables in mkAiffComment_t
  aiff->commentChunk->chunkSize += newComment->numChars;
  if(newComment->numChars & 0x01) aiff->commentChunk->chunkSize++; //if numChars is odd, it will be padded later, so increment chunkSize now;
  aiff->commentChunk->numComments++;
}

//addMarker------------------------------------------------------------------------------
void aiffAddMarkerWithPositionInSamples(MKAiff* aiff, char name[], mkAiffMarkerID_t markerID, uint32_t positionInSamples)
{
  aiffAddMarkerWithPositionInFrames (aiff, name, markerID, positionInSamples / aiffNumChannels(aiff));
}

void aiffAddMarkerWithPositionInSeconds(MKAiff* aiff, char name[], mkAiffMarkerID_t markerID, double positionInSeconds)
{
  aiffAddMarkerWithPositionInFrames (aiff, name, markerID, positionInSeconds * aiffSampleRate(aiff));
}

void aiffAddMarkerWithPositionInFrames (MKAiff* aiff, char name[], mkAiffMarkerID_t markerID, uint32_t positionInFrames)
{
  if(aiff->markerChunk == NULL)
    {
      aiff->markerChunk = (mkAiffMarkerChunk_t*)malloc(sizeof(*(aiff->markerChunk)));
      aiff->markerChunk->chunkID = AIFF_MARKER_CHUNK_ID;
      aiff->markerChunk->chunkSize = 2; //two bytes for sizeof(aiff->markerChunk->numMarkers) 
      aiff->markerChunk->numMarkers = 0;
      aiff->markerChunk->firstMarker = NULL;
    }
  mkAiffMarker_t* newMarker = (mkAiffMarker_t*)malloc(sizeof(*newMarker));
  newMarker->markerID = markerID;
  newMarker->positionInFrames = positionInFrames;
  
  int count = strlen(name);
  newMarker->name = (char*)malloc(count+1);
  int i;
  //(newMarker->name)[0] = count;
  for(i=0; i<count; i++) 
    {  newMarker->name[i] = name[i];  }
  newMarker->name[count] = '\0';
  
  if(aiff->markerChunk->firstMarker == NULL)
    {
      aiff->markerChunk->firstMarker = newMarker;
      newMarker->previous = NULL; newMarker->next = NULL;
    } 
  else aiffAppendEntryToList((mkAiffListEntry_t*)newMarker, (mkAiffListEntry_t*)aiff->markerChunk->firstMarker);
  
  aiff->markerChunk->chunkSize += 6;  //for markerID and positionInFrames
  aiff->markerChunk->chunkSize += count+1;
  if((count+1) & 0x01) aiff->markerChunk->chunkSize++; //if numChars is odd, it will be padded later, so increment chunkSize now;
  aiff->markerChunk->numMarkers++;
}

mkAiffMarker_t* aiffFindMarkerWithCriterion (MKAiff* aiff, mkAiffMarkerCriterion_t criterion, void* value)
{
  if(aiff->markerChunk != NULL)
    {
      mkAiffMarker_t* thisMarker = aiff->markerChunk->firstMarker;
      while(thisMarker != NULL)
       {
         switch(criterion)
           {
             case aiffMarkerCriterionMarkerID:
               if(thisMarker->markerID == *((mkAiffMarkerID_t*)value))
                 return thisMarker;
             break;
             case aiffMarkerCriterionName:
                if(strcmp(thisMarker->name, (char*)value) == 0)
                  return thisMarker;
             break;
           } 
         thisMarker = (mkAiffMarker_t*)(thisMarker->next);
       }
    }
  return NULL;
}

aiffYesOrNo_t aiffPositionInFramesOfMarkerWithID  (MKAiff* aiff, mkAiffMarkerID_t markerID, uint32_t* result)
{
  mkAiffMarker_t* marker = aiffFindMarkerWithCriterion(aiff, aiffMarkerCriterionMarkerID, &markerID);
  if(marker == NULL) return aiffNo;
  *result = marker->positionInFrames;
  return aiffYes;
}
aiffYesOrNo_t aiffPositionInFramesOfMarkerWithName(MKAiff* aiff, char* name, uint32_t* result)
{
  mkAiffMarker_t* marker = aiffFindMarkerWithCriterion(aiff, aiffMarkerCriterionName, name);
  if(marker == NULL) return aiffNo;
  *result = marker->positionInFrames;
  return aiffYes;
}
aiffYesOrNo_t aiffNameOfMarkerWithID (MKAiff* aiff, mkAiffMarkerID_t markerID, char** result)
{
  mkAiffMarker_t* marker = aiffFindMarkerWithCriterion(aiff, aiffMarkerCriterionMarkerID, &markerID);
  if(marker == NULL) return aiffNo;
  *result = marker->name;
  return aiffYes;
}
aiffYesOrNo_t aiffMarkerIDOfMarkerWithName        (MKAiff* aiff, char* name, mkAiffMarkerID_t* result)
{
  mkAiffMarker_t* marker = aiffFindMarkerWithCriterion(aiff, aiffMarkerCriterionName, name);
  if(marker == NULL) return aiffNo;
  *result = marker->markerID;
  return aiffYes;
}

//setupInstrumentInfo--------------------------------------------------------------------
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
                             mkAiffMarkerID_t releaseLoopEndMarkerID)
{
  if(aiff->instrumentChunk != NULL)
    aiffRemoveInstrumentInfo(aiff);
  aiff->instrumentChunk = (mkAiffInstrumentChunk_t*)malloc(sizeof(*(aiff->instrumentChunk)));
  aiff->instrumentChunk->chunkID                   = AIFF_INSTRUMENT_CHUNK_ID;
  aiff->instrumentChunk->chunkSize                 = 20;
  aiff->instrumentChunk->baseNote                  = baseNote;
  aiff->instrumentChunk->detune                    = detune;
  aiff->instrumentChunk->lowNote                   = lowNote;
  aiff->instrumentChunk->highNote                  = highNote;
  aiff->instrumentChunk->lowVelocity               = lowVelocity;
  aiff->instrumentChunk->highVelocity              = highVelocity;
  aiff->instrumentChunk->decibelsGain              = decibelsGain;
  aiff->instrumentChunk->sustainLoop.playMode      = sustainLoopPlayMode;
  aiff->instrumentChunk->sustainLoop.startMarkerID = sustainLoopStartMarkerID;
  aiff->instrumentChunk->sustainLoop.endMarkerID   = sustainLoopEndMarkerID;
  aiff->instrumentChunk->releaseLoop.playMode      = releaseLoopPlayMode;
  aiff->instrumentChunk->releaseLoop.startMarkerID = releaseLoopStartMarkerID;
  aiff->instrumentChunk->releaseLoop.startMarkerID = releaseLoopEndMarkerID;
}

aiffYesOrNo_t    aiffHasInstrumentInfo                 (MKAiff* aiff)
{return (aiff->instrumentChunk == NULL) ? aiffNo : aiffYes;}
int8_t           aiffInstrumentBaseNote                (MKAiff* aiff)
{return (aiffHasInstrumentInfo(aiff)) ? aiff->instrumentChunk->baseNote                  : 0;}
int8_t           aiffInstrumentDetune                  (MKAiff* aiff)
{return (aiffHasInstrumentInfo(aiff)) ? aiff->instrumentChunk->detune                    : 0;}
int8_t           aiffInstrumentLowNote                 (MKAiff* aiff)
{return (aiffHasInstrumentInfo(aiff)) ? aiff->instrumentChunk->lowNote                   : 0;}
int8_t           aiffInstrumentHighNote                (MKAiff* aiff)
{return (aiffHasInstrumentInfo(aiff)) ? aiff->instrumentChunk->highNote                  : 0;}
int8_t           aiffInstrumentLowVelocity             (MKAiff* aiff)
{return (aiffHasInstrumentInfo(aiff)) ? aiff->instrumentChunk->lowVelocity               : 0;}
int8_t           aiffInstrumentHighVelocity            (MKAiff* aiff)
{return (aiffHasInstrumentInfo(aiff)) ? aiff->instrumentChunk->highVelocity              : 0;}
int16_t          aiffInstrumentDecibelsGain            (MKAiff* aiff)
{return (aiffHasInstrumentInfo(aiff)) ? aiff->instrumentChunk->decibelsGain              : 0;}
int16_t          aiffInstrumentSustainLoopPlayMode     (MKAiff* aiff)
{return (aiffHasInstrumentInfo(aiff)) ? aiff->instrumentChunk->sustainLoop.playMode      : 0;}
mkAiffMarkerID_t aiffInstrumentSustainLoopStartMarkerID(MKAiff* aiff)
{return (aiffHasInstrumentInfo(aiff)) ? aiff->instrumentChunk->sustainLoop.startMarkerID : 0;}
mkAiffMarkerID_t aiffInstrumentSustainLoopEndMarkerID  (MKAiff* aiff)
{return (aiffHasInstrumentInfo(aiff)) ? aiff->instrumentChunk->sustainLoop.endMarkerID   : 0;}
int16_t          aiffInstrumentReleaseLoopPlayMode     (MKAiff* aiff)
{return (aiffHasInstrumentInfo(aiff)) ? aiff->instrumentChunk->releaseLoop.playMode      : 0;}
mkAiffMarkerID_t aiffInstrumentReleaseLoopStartMarkerID(MKAiff* aiff)
{return (aiffHasInstrumentInfo(aiff)) ? aiff->instrumentChunk->releaseLoop.startMarkerID : 0;}
mkAiffMarkerID_t aiffInstrumentReleaseLoopEndMarkerID  (MKAiff* aiff)
{return (aiffHasInstrumentInfo(aiff)) ? aiff->instrumentChunk->releaseLoop.startMarkerID : 0;}

//setName--------------------------------------------------------------------------------
char*            aiffName                     (MKAiff* aiff)
{
  char* result = NULL;
  if(aiff->nameChunk != NULL)
    result = aiff->nameChunk->data;
  
  return result;
}

//setName--------------------------------------------------------------------------------
char*            aiffAuthor                   (MKAiff* aiff) 
{
  char* result = NULL;
  if(aiff->authorChunk != NULL)
    result = aiff->authorChunk->data;
  
  return result;
}

//setName--------------------------------------------------------------------------------
char*            aiffCopyright                (MKAiff* aiff)
{
  char* result = NULL;
  if(aiff->copyrightChunk != NULL)
    result = aiff->copyrightChunk->data;

  return result;
}

//setName--------------------------------------------------------------------------------
void             aiffSetName                  (MKAiff* aiff, char* text)
{
  if(aiff->nameChunk != NULL) 
    aiffRemoveName(aiff);
  aiff->nameChunk = aiffCreateTextChunk(EA_IFF85_NAME_CHUNK_ID, text);
}

//setAuthor------------------------------------------------------------------------------
void             aiffSetAuthor                (MKAiff* aiff, char* text) 
{
  if(aiff->authorChunk != NULL) 
    aiffRemoveAuthor(aiff);
  aiff->authorChunk = aiffCreateTextChunk(EA_IFF85_AUTHOR_CHUNK_ID, text);
}

//setCopyright---------------------------------------------------------------------------
void             aiffSetCopyright             (MKAiff* aiff, char* text)
{
  if(aiff->copyrightChunk != NULL) 
    aiffRemoveCopyright(aiff);
  aiff->copyrightChunk = aiffCreateTextChunk(EA_IFF85_COPYRIGHT_CHUNK_ID, text);
}

//addAnnotation--------------------------------------------------------------------------
void             aiffAddAnnotation            (MKAiff* aiff, char* text)
{
  mkEAiff85TextChunk_t* annotationChunk = aiffCreateTextChunk(EA_IFF85_ANNOTATION_CHUNK_ID, text); 
  if(aiff->annotationChunkList == NULL) 
    aiff->annotationChunkList = annotationChunk;
  else aiffAppendEntryToList((mkAiffListEntry_t*)annotationChunk, (mkAiffListEntry_t*)aiff->annotationChunkList);
}

//addGenericChunk------------------------------------------------------------------------
void aiffAddGenericChunk(MKAiff* aiff, unsigned char* data, int32_t sizeOfData, aiffChunkIdentifier_t chunkID)
{
  mkAiffGenericChunk_t* genericChunk = (mkAiffGenericChunk_t*)malloc(sizeof(*genericChunk));
  genericChunk->previous = genericChunk->next = NULL;
  genericChunk->chunkID = chunkID;
  genericChunk->chunkSize = sizeOfData;
  genericChunk->data = (char*)malloc(sizeOfData);
  int i;
  for(i=0; i<sizeOfData; i++)(genericChunk->data)[i] = data[i];
  if(aiff->unknownChunkList == NULL) 
    aiff->unknownChunkList = genericChunk;
  else aiffAppendEntryToList((mkAiffListEntry_t*)genericChunk, (mkAiffListEntry_t*)aiff->unknownChunkList);
}

//aiffNormalize------------------------------------------------------------------------
void             aiffNormalize(MKAiff* aiff, aiffYesOrNo_t removeOffset)
{
  //int64_t min=-0x7FFFFFFF, max=0x7FFFFFFF;
  //int64_t maxMinusMin = min-max;
  int64_t maxMinusMinSample;
  int32_t minSample = 0x7FFFFFFF, maxSample = 0;
  int32_t* b = aiff->soundBuffer;
  int       N = aiffDurationInSamples(aiff);
  
  
  while(N--)
    {
      if(abs(*b) > maxSample)
        maxSample = abs(*b);
      //if(*b < minSample)
        //minSample = *b;
      //if(*b > maxSample)
        //maxSample = *b;
      b++;
    }
    
  minSample = 0;
  
  N = aiffDurationInSamples(aiff);
  b = aiff->soundBuffer;
  maxMinusMinSample = minSample - maxSample;
  
  if(maxSample == 0) return;
  while(N-- > 0)
    {
      *b *= ((double)0x7FFFFFFF / (double)maxSample);
      //*b =  maxMinusMin * (((int64_t)*b - minSample) / (double)maxMinusMinSample) + min;
      b++;
    }
}

/*---------------------------------------------------------*/
int aiffChangeGain(MKAiff* aiff, double gain)
{
  /* todo: use integer math */
  int32_t* b = aiff->soundBuffer;
  int       N = aiffDurationInSamples(aiff);
  long double sample;
  int i;
  int num_clipped_samples = 0;
  for(i=0; i<N; i++)
    {
      sample = *b * (long double) gain;

      if(fabsl(sample) > (long double)0x7FFFFFFF)
        {
          long double multiplier = (sample < 0) ? -1 : 1;
          sample = (long double)0x7FFFFFFF * multiplier;
          ++num_clipped_samples;
        }
    
      *b++ = (int32_t)sample;
    }
  return num_clipped_samples;
}

/*---------------------------------------------------------*/
void aiffHilbertTransform(int32_t* in, int32_t* out, int numSamples)
{
   /* Adapted from Joe Mietus */
    int i, l;
    double yt;
    int numCoeffs       = 128; //must be even
    int numCoeffsOver2  = numCoeffs >> 1;
    double coeffs[129] = {0, -0.005013, -0.005093, -0.005176, -0.005261, -0.005350, -0.005441, -0.005536, -0.005634, -0.005735, -0.005841, -0.005950, -0.006063, -0.006181, -0.006303, -0.006431, -0.006563, -0.006701, -0.006845, -0.006996, -0.007153, -0.007317, -0.007490, -0.007670, -0.007860, -0.008058, -0.008268, -0.008488, -0.008721, -0.008966, -0.009226, -0.009502, -0.009794, -0.010105, -0.010436, -0.010790, -0.011169, -0.011575, -0.012012, -0.012483, -0.012992, -0.013545, -0.014147, -0.014805, -0.015527, -0.016324, -0.017206, -0.018189, -0.019292, -0.020536, -0.021952, -0.023579, -0.025465, -0.027679, -0.030315, -0.033506, -0.037448, -0.042441, -0.048971, -0.057875, -0.070736, -0.090946, -0.127324, -0.212207, -0.636620, 0.636620, 0.212207, 0.127324, 0.090946, 0.070736, 0.057875, 0.048971, 0.042441, 0.037448, 0.033506, 0.030315, 0.027679, 0.025465, 0.023579, 0.021952, 0.020536, 0.019292, 0.018189, 0.017206, 0.016324, 0.015527, 0.014805, 0.014147, 0.013545, 0.012992, 0.012483, 0.012012, 0.011575, 0.011169, 0.010790, 0.010436, 0.010105, 0.009794, 0.009502, 0.009226, 0.008966, 0.008721, 0.008488, 0.008268, 0.008058, 0.007860, 0.007670, 0.007490, 0.007317, 0.007153, 0.006996, 0.006845, 0.006701, 0.006563, 0.006431, 0.006303, 0.006181, 0.006063, 0.005950, 0.005841, 0.005735, 0.005634, 0.005536, 0.005441, 0.005350, 0.005261, 0.005176, 0.005093, 0.005013};
    
    /* generate filter coeffecients */
    //it might be better if this were done in advance
    //rather than calculated every time because they are 
    //constants
    /*
    for (i=1; i<=numCoeffs; i++){
      coeffs[i] = 1 / ((i - numCoeffsOver2) - 0.5) / 3.14159265;
      printf("%f, ", (float)coeffs[i] );
     }
     */
     
    for (l=1; l<=numSamples-numCoeffs+1; l++) 
      {
        yt = 0.0;
        for (i=1; i<=numCoeffs; i++) 
          yt = yt + in[l+i-1] * coeffs[numCoeffs+1-i];
        
        if(yt > 0x7FFFFFFF) yt = 0x7FFFFFFF;
        out[l] = yt;
      }

    /* shifting lfilt/1+1/2 points */
    for (i=1; i<=numSamples-numCoeffs; i++) 
        out[i] = ((int64_t)out[i] + (int64_t)out[i+1]) >> 1;
        
    for (i=numSamples-numCoeffs; i>=1; i--)
        out[i + numCoeffsOver2] = out[i];

    /* writing zeros */
    for (i=1; i<=numCoeffsOver2; i++) 
      {
        out[i]              = 0.0;
        out[numSamples-i] = 0.0;
      }
}

/*---------------------------------------------------------*/
MKAiff*          aiffGetAmplitudeEnvelope(MKAiff* monoAiff)
{
  int numSamples = aiffDurationInSamples(monoAiff);
  int i;
  int32_t *a, *b;
  
  MKAiff* result = aiffWithDurationInSamples(aiffNumChannels(monoAiff), aiffSampleRate(monoAiff), aiffBitsPerSample(monoAiff), numSamples);
  
  if(result != NULL)
    {
      a = monoAiff->soundBuffer;
      b = result->soundBuffer;
      
      aiffHilbertTransform(a, b, numSamples);
      result->numSamplesWrittenToBuffer = numSamples;
      
      for(i=0; i<numSamples; i++)
        {
          *b = (abs(*a)>>1) + (abs(*b)>>1);
          a++; b++;
        }
    
      b = result->soundBuffer;
      for(i=1; i<numSamples; i++)
        {  
          int64_t temp = (0.01 * *b) + (0.99 * b[-1]);
          if(temp > 0x7FFFFFFF) temp = 0x7FFFFFFF;
          *b = temp;
          b++;
        }
    }

  return result;
}

//aiffTrimBeginingAndEnd-----------------------------------------------------------------
void          aiffTrimBeginning(MKAiff* aiff, double cutoff, aiffYesOrNo_t scanZeroCrossing)
{
  if(cutoff > 1) cutoff = 1;
  if(cutoff < 0) cutoff = 0;
  
  int32_t int_cutoff = cutoff * 0x7FFFFFFF;
  int32_t *b = aiff->soundBuffer;
  int numSamples = aiffDurationInSamples(aiff);
  int numChannels = aiffNumChannels(aiff);
  int i;
  int multiplier;
  
  //find first sample above cutoff on any channel
  for(i=0; i<numSamples; i++)
    {
      if(abs(*b) >= int_cutoff)
        {
          multiplier = (*b >= 0) ? 1 : -1;
          break;
        }
      b++;
    }
    
  //scan backwards for zero crossing on this channel
  if(scanZeroCrossing)
    {
      for(; i>=0; i -= numChannels)
        {
          if((*b * multiplier) <= 0)
            break;
          b-=numChannels;
        }
    }
  
  //ensures that i is an integer number of frames
  i /= numChannels;
  i *= numChannels;  
  
  aiffRewindPlayheadToBeginning(aiff);
  aiffRemoveSamplesAtPlayhead(aiff, i);
}

//aiffTrimBeginningAndEnd-----------------------------------------------------------------
void         aiffTrimEnd(MKAiff* aiff, double cutoff, aiffYesOrNo_t scanZeroCrossing)
{
  if(cutoff > 1) cutoff = 1;
  if(cutoff < 0) cutoff = 0;
  
  int32_t int_cutoff = cutoff * 0x7FFFFFFF;
  int numSamples = aiffDurationInSamples(aiff);
  int32_t *b = aiff->soundBuffer + (numSamples - 1);
  int numChannels = aiffNumChannels(aiff);
  int i;
  int multiplier;
  
  //find last sample above cutoff on any channel
  
  for(i=(numSamples-1); i>=0; i--)
    {
      if(abs(*b) >= int_cutoff)
        {
          multiplier = (*b >= 0) ? 1 : -1;
          break;
        }
      b--;
    }

  //scan forwards for zero crossing on this channel
  if(scanZeroCrossing)
    {
      for(; i<numSamples; i+=numChannels)
        {
          if((*b * multiplier) <= 0)
            break;
          b += numChannels;
        }
    }
  
  //ensures that i is an integer number of frames
  i /= numChannels;
  i *= numChannels;
  
  aiffSetPlayheadToSamples(aiff, i);
  aiffRemoveSamplesAtPlayhead(aiff, numSamples /* this function will boundary check this number which is clearly too big */);
}

//---------------------------------------------------------------------------------------
void aiffFadeInOut(MKAiff* aiff, aiffYesOrNo_t in, aiffYesOrNo_t out, float fadeTime, mkAiffFadeType_t curve_type /*only linear is supported right now*/)
{
  if((!in)&&(!out)) return;

  int frame, channel;
  int numChannels = aiffNumChannels(aiff);
  int N = aiffDurationInFrames(aiff);
  unsigned t = fadeTime * aiffSampleRate(aiff);
  int divisor = (in && out) ? 2 : 1;
  unsigned t_min = (t < N/divisor) ? t: N/divisor;
  
  if(in)
    {
      aiffRewindPlayheadToBeginning(aiff);
      for(frame=0; frame<t_min; frame++)
        {
          double fade = frame / (double)t_min;
          for(channel=0; channel<numChannels; channel++)
            *aiff->playhead++ *= fade;
        }
    }
  
  if(out)
    {
      aiffFastForwardPlayheadToEnd(aiff);
      aiffAdvancePlayheadByFrames(aiff, -t_min);
      for(frame=N-t_min; frame<N; frame++)
        {
          double fade = (N-frame) / (double)t_min;
          for(channel=0; channel<numChannels; channel++)
            *aiff->playhead++ *= fade;
        }
    }
}

//destroy--------------------------------------------------------------------------------
MKAiff* aiffDestroy(MKAiff* aiff)
{
  if(aiff != NULL)
    {
      if(aiff->soundBuffer != NULL)
         free(aiff->soundBuffer);
      if(aiff->commentChunk != NULL)
         aiffRemoveAllComments(aiff);
      if(aiff->markerChunk != NULL)
         aiffRemoveAllMarkers(aiff);
      if(aiff->instrumentChunk != NULL)
         aiffRemoveInstrumentInfo(aiff);
      if(aiff->nameChunk != NULL)
        aiffRemoveName(aiff);
      if(aiff->authorChunk != NULL)
        aiffRemoveAuthor(aiff); 
      if(aiff->copyrightChunk != NULL)
        aiffRemoveCopyright(aiff);  
      if(aiff->annotationChunkList != NULL)
        aiffRemoveAllAnnotations(aiff); 
      if(aiff->unknownChunkList != NULL)
        aiffRemoveAllGenericChunks(aiff);
      free(aiff);
    }
  
  return (MKAiff*)NULL;
}


//below here from:
//http://www.onicos.com/staff/iz/formats/ieee.c
/*
 * C O N V E R T   T O   I E E E   E X T E N D E D
 */

/* Copyright (C) 1988-1991 Apple Computer, Inc.
 * All rights reserved.
 *
 * Machine-independent I/O routines for IEEE floating-point numbers.
 *
 * NaN's and infinities are converted to HUGE_VAL or HUGE, which
 * happens to be infinity on IEEE machines.  Unfortunately, it is
 * impossible to preserve NaN's in a machine-independent way.
 * Infinities are, however, preserved on IEEE machines.
 *
 * These routines have been tested on the following machines:
 *    Apple Macintosh, MPW 3.1 C compiler
 *    Apple Macintosh, THINK C compiler
 *    Silicon Graphics IRIS, MIPS compiler
 *    Cray X/MP and Y/MP
 *    Digital Equipment VAX
 *
 *
 * Implemented by Malcolm Slaney and Ken Turkowski.
 *
 * Malcolm Slaney contributions during 1988-1990 include big- and little-
 * endian file I/O, conversion to and from Motorola's extended 80-bit
 * floating-point format, and conversions to and from IEEE single-
 * precision floating-point format.
 *
 * In 1991, Ken Turkowski implemented the conversions to and from
 * IEEE double-precision format, added more precision to the extended
 * conversions, and accommodated conversions involving +/- infinity,
 * NaN's, and denormalized numbers.
 */

#ifndef HUGE_VAL
# define HUGE_VAL HUGE
#endif /*HUGE_VAL*/

# define FloatToUnsigned(f)      ((unsigned long)(((long)(f - 2147483648.0)) + 2147483647L) + 1)

void ConvertToIeeeExtended(double num, unsigned char* bytes)
{
    int    sign;
    int expon;
    double fMant, fsMant;
    unsigned long hiMant, loMant;

    if (num < 0) {
        sign = 0x8000;
        num *= -1;
    } else {
        sign = 0;
    }

    if (num == 0) {
        expon = 0; hiMant = 0; loMant = 0;
    }
    else {
        fMant = frexp(num, &expon);
        if ((expon > 16384) || !(fMant < 1)) {    /* Infinity or NaN */
            expon = sign|0x7FFF; hiMant = 0; loMant = 0; /* infinity */
        }
        else {    /* Finite */
            expon += 16382;
            if (expon < 0) {    /* denormalized */
                fMant = ldexp(fMant, expon);
                expon = 0;
            }
            expon |= sign;
            fMant = ldexp(fMant, 32);
            fsMant = floor(fMant);
            hiMant = FloatToUnsigned(fsMant);
            fMant = ldexp(fMant - fsMant, 32);
            fsMant = floor(fMant);
            loMant = FloatToUnsigned(fsMant);
        }
    }
  
    bytes[0] = expon >> 8;
    bytes[1] = expon;
    bytes[2] = hiMant >> 24;
    bytes[3] = hiMant >> 16;
    bytes[4] = hiMant >> 8;
    bytes[5] = hiMant;
    bytes[6] = loMant >> 24;
    bytes[7] = loMant >> 16;
    bytes[8] = loMant >> 8;
    bytes[9] = loMant;
}


/*
 * C O N V E R T   F R O M   I E E E   E X T E N D E D
 */

/*
 * Copyright (C) 1988-1991 Apple Computer, Inc.
 * All rights reserved.
 *
 * Machine-independent I/O routines for IEEE floating-point numbers.
 *
 * NaN's and infinities are converted to HUGE_VAL or HUGE, which
 * happens to be infinity on IEEE machines.  Unfortunately, it is
 * impossible to preserve NaN's in a machine-independent way.
 * Infinities are, however, preserved on IEEE machines.
 *
 * These routines have been tested on the following machines:
 *    Apple Macintosh, MPW 3.1 C compiler
 *    Apple Macintosh, THINK C compiler
 *    Silicon Graphics IRIS, MIPS compiler
 *    Cray X/MP and Y/MP
 *    Digital Equipment VAX
 *
 *
 * Implemented by Malcolm Slaney and Ken Turkowski.
 *
 * Malcolm Slaney contributions during 1988-1990 include big- and little-
 * endian file I/O, conversion to and from Motorola's extended 80-bit
 * floating-point format, and conversions to and from IEEE single-
 * precision floating-point format.
 *
 * In 1991, Ken Turkowski implemented the conversions to and from
 * IEEE double-precision format, added more precision to the extended
 * conversions, and accommodated conversions involving +/- infinity,
 * NaN's, and denormalized numbers.
 */

#ifndef HUGE_VAL
# define HUGE_VAL HUGE
#endif /*HUGE_VAL*/

# define UnsignedToFloat(u)         (((double)((long)(u - 2147483647L - 1))) + 2147483648.0)

/****************************************************************
 * Extended precision IEEE floating-point conversion routine.
 ****************************************************************/

double ConvertFromIeeeExtended(unsigned char* bytes /* LCN */)
{
    double    f;
    int    expon;
    unsigned long hiMant, loMant;
  
    expon = ((bytes[0] & 0x7F) << 8) | (bytes[1] & 0xFF);
    hiMant    =    ((unsigned long)(bytes[2] & 0xFF) << 24)
            |    ((unsigned long)(bytes[3] & 0xFF) << 16)
            |    ((unsigned long)(bytes[4] & 0xFF) << 8)
            |    ((unsigned long)(bytes[5] & 0xFF));
    loMant    =    ((unsigned long)(bytes[6] & 0xFF) << 24)
            |    ((unsigned long)(bytes[7] & 0xFF) << 16)
            |    ((unsigned long)(bytes[8] & 0xFF) << 8)
            |    ((unsigned long)(bytes[9] & 0xFF));

    if (expon == 0 && hiMant == 0 && loMant == 0) {
        f = 0;
    }
    else {
        if (expon == 0x7FFF) {    /* Infinity or NaN */
            f = HUGE_VAL;
        }
        else {
            expon -= 16383;
            f  = ldexp(UnsignedToFloat(hiMant), expon-=31);
            f += ldexp(UnsignedToFloat(loMant), expon-=32);
        }
    }

    if (bytes[0] & 0x80)
        return -f;
    else
        return f;
}
