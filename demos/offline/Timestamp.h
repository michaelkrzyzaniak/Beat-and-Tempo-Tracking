#ifndef __MK_TIMESTAMP__
#define __MK_TIMESTAMP__


#if defined(__cplusplus)
extern "C"{
#endif   //(__cplusplus)

#include <stdint.h>

typedef uint64_t timestamp_microsecs_t;
timestamp_microsecs_t timestamp_get_current_time();

#if defined(__cplusplus)
}
#endif   //(__cplusplus)

#endif //__MK_TIMESTAMP__