#ifndef  __MK_SINE_TABLE__     
#define  __MK_SINE_TABLE__ 1 

#ifdef __cplusplus            
extern "C" {                
#endif                      

#include <stdint.h>


#define  SIN_NUM_SAMPLES        4096
#define  SIN_TWO_PI             0x100000000
#define  SIN_PI                 0x80000000
#define  SIN_HALF_PI            0x40000000
#define  SIN_QUARTER_PI         0x20000000
#define  fastsin_t              uint32_t

float fastsin  (fastsin_t angle);
float fastcos  (fastsin_t angle);

#define fastsin(angle) (*(sinTable + ((angle) >> 20)))
#define fastcos(angle) (*(sinTable + (((angle) + SIN_HALF_PI) >> 20)))

const extern float sinTable[SIN_NUM_SAMPLES];


//slowsin
/*
#define  SIN_TWO_PI             (2 * M_PI)
#define  SIN_PI                 (M_PI)
#define  SIN_HALF_PI            (0.5 * M_PI)
#define  SIN_QUARTER_PI         (0.25 * M_PI)
#define  fastsin_t              double

float fastsin  (fastsin_t angle);
float fastcos  (fastsin_t angle);

#define fastsin(angle) (sin(angle))
#define fastcos(angle) (cos(angle))
*/

#ifdef __cplusplus            
}                             
#endif                      

#endif//__MK_SINE_TABLE__     
