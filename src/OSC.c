/*!
 * @header OSC.c
 * @author
 *  Written by Michael Krzyzaniak at Arizona State 
 *  University's School of Arts, Media + Engineering
 *  in Spring of 2013.
 *
 *  mkrzyzan@asu.edu
 *
 * @unsorted
 */
/*-----------------------------------------------------*/

#include "OSC.h"
#include <stdlib.h>
#include <stdio.h>

void   oscConstructSymbol    (char*     value, char** buffer, int* bufferSize);
void   oscConstructTypeTag   (char*     value, char** buffer, int* bufferSize);
void   oscConstructInt       (int32_t   value, char** buffer, int* bufferSize);
void   oscConstructFloat     (float32_t value, char** buffer, int* bufferSize);

char*   oscParseString        (char** buffer, int* numValidCharsInBuffer);
char*   oscParseTypeTag       (char** buffer, int* numValidCharsInBuffer);
int     oscParseInt           (char** buffer, int* numValidCharsInBuffer);
float   oscParseFloat         (char** buffer, int* numValidCharsInBuffer);


/*-------------------------------------------------------------------------*/
void oscConstructSymbol(char* value, char** buffer, int* bufferSize)
{
  char* b = *buffer;

  while((*value != '\0') && ((*bufferSize)-- > 0))
    *b++ = *value++;

  if((*bufferSize)-- > 0)
    *b++ = '\0';
    
  while( ((b - *buffer) % 4) && ((*bufferSize)-- > 0))
    *b++ = 0;
  
  *buffer = b;
}

/*-------------------------------------------------------------------------*/
void oscConstructTypeTag(char* value, char** buffer, int* bufferSize)
{
  char  typeTag[strlen(value) + 2];
  char* v = typeTag;
  
  *v++ = ',';
  while(*value != '\0')
    *v++ = *value++;
  *v = '\0';

  oscConstructSymbol(typeTag, buffer, bufferSize);   
}

/*-------------------------------------------------------------------------*/
void   oscConstructInt         (int32_t value, char** buffer, int* bufferSize)
{
   char* b = *buffer;
   int i;
   value = htonl(value);
   char* c = ((char*) &value);   

   for(i=0; i<4; i++)
     if((*bufferSize)-- > 0)
       *b++ = *c++;

   *buffer = b;
}

/*-------------------------------------------------------------------------*/
void   oscConstructFloat       (float32_t value, char** buffer, int* bufferSize)
{
  int32_t intVal = *((int32_t*)(&value));
  oscConstructInt(intVal, buffer, bufferSize);
}

/*-------------------------------------------------------------------------*/
int oscConstruct(char* buffer, int bufferSize, char* address, char* typeTag, ...)
{
  char* b = buffer;
  va_list args;
  int numArgs = strlen(typeTag);
  int i;

  oscConstructSymbol (address, &b, &bufferSize);
  oscConstructTypeTag(typeTag, &b, &bufferSize);
  
  va_start(args, typeTag);
  
  for(i=0; i<numArgs; i++)
    {
      switch(typeTag[i])
        {
          case 'i':
            oscConstructInt   (va_arg(args, int)   , &b, &bufferSize);
            break;
          case 'f':
            oscConstructFloat (va_arg(args, double), &b, &bufferSize);
            break;
          case 's':
            oscConstructSymbol(va_arg(args, char *), &b, &bufferSize);
            break;
          default: break;
        }
    }
  va_end(args);

  return b - buffer;
}


/*-------------------------------------------------------------------------*/
char*  oscParseSymbol        (char** buffer, int* numValidCharsInBuffer)
{
  //return *buffer if it contains a valid string.
  //return NULL if there is no '\0' or it contains invalid chars;
  //advance *buffer to the end of symbol + padding;
  
  char* b = *buffer;
  char* result = NULL;
  char  next;
  
  while((*numValidCharsInBuffer)-- > 0)
    {
      next = *b++;
      if(next == '\0')
        {
          result = *buffer;
          break;
        }
      if((next < ' ') || (next > '~'))
        break;
    }
  
  while(((b - *buffer) % 4) && (*numValidCharsInBuffer)-- > 0) 
    b++;
  //actually dont return NULL, just put '\0' in the first byte and return that
  if(result == NULL)
    {
      **buffer = '\0';
      result = *buffer;
    }
  
  *buffer = b;
  return result;
}

/*-------------------------------------------------------------------------*/
char* oscParseTypeTag(char** buffer, int* numValidCharsInBuffer)
{
  char* result = oscParseSymbol(buffer, numValidCharsInBuffer);
  
  if(result[0] == ',')
    result++;

  return result;
}

/*-------------------------------------------------------------------------*/
int oscParseInt           (char** buffer, int* numValidCharsInBuffer)
{
  char bytes[4] = {0, 0, 0, 0};
  char *v = bytes, *b = *buffer;
  int i = 0;
  while((i++ < 4) && (*numValidCharsInBuffer)-- > 0)
    *v++ = *b++;
    
  *buffer = b;
  int32_t value = *((int32_t *)(bytes));
  return ntohl(value);
}

/*-------------------------------------------------------------------------*/
float32_t  oscParseFloat         (char** buffer, int* numValidCharsInBuffer)
{
  int32_t value = oscParseInt(buffer, numValidCharsInBuffer);
  return *((float32_t*)(&value));
}

oscValue_t valuesBuffer[100];
/*-------------------------------------------------------------------------*/
int oscParse(char* buffer, int numValidCharsInBuffer, char** returnedAddress, char** returnedTypeTag, oscValue_t** returnedValues)
{
  int i, numReturnedValues = -1; 
  if(numValidCharsInBuffer % 4) return -1;
  
  *returnedAddress = oscParseSymbol(&buffer, &numValidCharsInBuffer);
  if(**returnedAddress != '\0')
    numReturnedValues = 0;
  
  *returnedTypeTag = oscParseTypeTag(&buffer, &numValidCharsInBuffer);
  if(**returnedTypeTag != '\0') 
    numReturnedValues = (strlen(*returnedTypeTag));
  
  for(i=0; i<numReturnedValues; i++)
    {
      switch((*returnedTypeTag)[i])
        {
          case 'i':
            valuesBuffer[i].i = oscParseInt(&buffer, &numValidCharsInBuffer);
            break;
          case 'f':
            valuesBuffer[i].f = oscParseFloat(&buffer, &numValidCharsInBuffer); 
            break;
          case 's':
            valuesBuffer[i].s = oscParseSymbol(&buffer, &numValidCharsInBuffer);
            //printf("here: %s\n", valuesBuffer[i].s);
            if(*(valuesBuffer[i].s) == '\0')
              {
                numReturnedValues = 0;
                goto ABORT;
              }
            break;
          default:
            numReturnedValues = 0;
            goto ABORT;
            //break;
        }
    }
  
    ABORT:
    *returnedValues = valuesBuffer;
    return numReturnedValues;
}

/*-------------------------------------------------------------------------*/
int oscCountAddressComponents(char* address)
{
  int numComponents = 0;
  
  if (*address++ != '\0')
    numComponents++;
    
  while(*address != '\0')
    {
      if(*address == '/')
        ++numComponents;
      ++address;
    }
   return numComponents;
}

/*-------------------------------------------------------------------------*/
//what if the last part is '/'?
int oscExplodeAddress(char* address, char* components[], int componentsCapacity)
{
  int numComponents = 0;
  if((address[0] != '/') && (componentsCapacity >= 1))
    components[numComponents++] = address;
  
  while((*address != '\0') && (numComponents < componentsCapacity))
    {
      if(*address == '/')
        {
          *address = '\0';
          components[numComponents++] = address+1;
        }
      address++;
    }
  
  return numComponents;
}


/*-------------------------------------------------------------------------*/
float oscValueAsFloat(oscValue_t value, char type)
{
  float result = 0;
  switch(type)
    {
      case 'f': 
        result = value.f;
        break;
      case 'i':
        result = value.i;
        break;
      case 's':
        result = (float)atof(value.s);
        break;
      default: 
        break;
    }
    return result;
}

/*-------------------------------------------------------------------------*/
int oscValueAsInt(oscValue_t value, char type)
{
  int result = 0;
  switch(type)
    {
      case 'f': 
        result = value.f;
        break;
      case 'i':
        result = value.i;
        break;
      case 's':
        result = atoi(value.s);
        break;
      default: 
        break;
    }
    return result;  
}

/*-------------------------------------------------------------------------*/
char* oscValueAsString(oscValue_t value, char type)
{
  char* result = NULL;
  switch(type)
    {
      case 'f': 
        asprintf(&result, "%f", value.f);
        break;
      case 'i':
        asprintf(&result, "%i", value.i);
        break;
      case 's':
        asprintf(&result, "%s", value.s);
        break;
      default: 
        asprintf(&result, "%s", "?");
        break;
    }
    return result; 
}

/*-------------------------------------------------------------------------*/
inline uint32_t oscHash(unsigned char* str)
{
	uint32_t hash = 5381;
	while (*str != '\0')
		hash = hash * 33 ^ (int)*str++;
	return hash;
}

