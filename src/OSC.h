/*!
 * @header OSC.h
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

#ifndef  __MK_OSC__     
#define  __MK_OSC__ 1 

#ifdef __cplusplus            
extern "C" {                
#endif 

/*-----------------------------------------------------*/

#include <stdint.h>
#include <stdarg.h>
#include <strings.h>

typedef float float32_t;

typedef union osc_value_union
{
  float32_t f;
  int32_t   i;
  char*     s;
}oscValue_t;

/*-------------------------------------------------------------------------*/
/*! 
 * @function oscParse
 * @abstract Takes a buffer of bytes that you think might contain
 *   a valid OSC message (perhaps obtained through udpReceive())
 *   and parses it into an address, type tag, and arguments (floats,
 *   ints and symbols). 
 * @param buffer
 *   on call this should contain bytes that you think might be
 *   a valid OSC message. This method will overwrite the contents of
 *   buffer and reallocate it for the returned values. 
 * @param numValidCharsInBuffer
 *   On call, this should indicate how many valid bytes are in buffer.
 * @param returnedAddress
 *   On return this will be made to point to a null-terminated string 
 *   that is the address component of the associate OSC message, or ""
 *   if there was no valid address;
 * @param returnedTypeTag
 *   On return this will contain a null-terminated string that
 *   indicates the data types of each returned value. A 'f' indicates
 *   a float, 'i' for int, and 's' for symbol. For example if this 
 *   returns "fis", then three values were returned, the first a float,
 *   the second and int, and the third a symbol. If there are no valid
 *   values, "" will be returned.
 * @param returnedValues
 *   On return, this will contain an array of the values associated with
 *   the OSC message. The values are stored in a union, so returnedFormat 
 *   must be queried to determine which member contains valid data.
 * @result
 *  the number of returned values. -1 indicates that the entire message
 *  message was not valid. 0 indicates a valid address with no values.
 * 
 */
int oscParse(char* buffer, int numValidCharsInBuffer, char** returnedAddress, char** returnedTypeTag, oscValue_t* returnedValues[]);

/*-------------------------------------------------------------------------*/
/*! 
 * @function oscConstruct
 * @abstract Take and address, type tag, and a variadic list of floats,
 *   ints and symbols, and formats them as OSC, and puts the result
 *   in buffer.
 * @param buffer
 *   the buffer where the OSC message will be written.
 * @param bufferSize
 *   the capacity, in bytes, of the buffer.
 * @param address
 *   the address associated with the OSC message.
 * @param typeTag
 *   "fsi" ect...
 * @param ...
 *   the values associate with the OSC message
 * @result
 *   the number of bytes written into buffer.
 */
int oscConstruct(char* buffer, int bufferSize, char* address, char* typeTag, ...);


/*-------------------------------------------------------------------------*/
/*!
 * @function oscCountAddressComponents
 * @abstract count the number of components in the address, as separated
 *   '/'. Use this prior to oscExplodeAddress() to find out how much 
 *   how much space to allocate for components[].
 * @param address
 *   the address associated with the OSC message.
 */
int oscCountAddressComponents(char* address);

/*-------------------------------------------------------------------------*/
/*! 
 * @function oscExplodeAddress
 * @abstract breakes up an OSC address into components, as separated by '/'.
 *   This function removes the '/' and returns pointers to null-terminated
 *   strings representing the components.
 *  @param components
 *    on call, an pointer to an empty array of strings. On return the array
 *    will be populated with pointers to strings. The strings are not 
 *    dynamically allocated and do not need to be freed. 
 * @param componentsCapacity
 *    the capacity in bytes of *components.
 * @result
 *    the number of address components written to components[]
 */
int oscExplodeAddress(char* address, char* components[], int componentsCapacity);


/*-------------------------------------------------------------------------*/
/*! 
 * @function oscValueToFloat
 * @abstract take an OSC value union and convert its value to float, by
 *   casting if it contains an int or atof it contains a string.
 * @param value
 *   the value to convert
 * @param type
 *   one of 'i', 'f', or 's', as indicated by the type tag fpr the 
 *   message.
 * @result 
 *   a float representation of the value.
 */
float oscValueAsFloat(oscValue_t value, char type);


/*-------------------------------------------------------------------------*/
/*! 
 * @function oscValueToInt
 * @abstract take an OSC value union and convert its value to int, by
 *   casting if it contains a float or atoi it contains a string.
 * @param value
 *   the value to convert
 * @param type
 *   one of 'i', 'f', or 's', as indicated by the type tag for the 
 *   message.
 * @result 
 *   an int representation of the value.
 */
int oscValueAsInt(oscValue_t value, char type);


/*-------------------------------------------------------------------------*/
/*! 
 * @function oscValueToString
 * @abstract take an OSC value union and convert its value to string. The
 *  result is dynamically allocated and needs to be freed.
 * @param value
 *   the value to convert
 * @param type
 *   one of 'i', 'f', or 's', as indicated by the type tag for the 
 *   message.
 * @result 
 *   an string representation of the value.
 */
char* oscValueAsString(oscValue_t value, char type);



/*-------------------------------------------------------------------------*/
/*! 
 * @function oscHash
 * @abstract return a (probably) unique value for a wide variety of addresses
 *  / type-tags / symbols. Hashing and comparing known values in a switch case
 *  statement is over twice as fast as calling strcmp in if statements. 
 * @param str
 *   
 * @result 
 *   a number that is probably unique to str.
 */
uint32_t oscHash(unsigned char* str);


#ifdef __cplusplus            
}                             
#endif

#endif//__MK_OSC__

