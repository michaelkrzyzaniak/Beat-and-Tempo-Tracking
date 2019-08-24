#include "Timestamp.h"


/*----------------------------------------------------------*/
#if defined __APPLE__

#include <mach/mach_time.h>

timestamp_microsecs_t timestamp_get_current_time()
{
    static mach_timebase_info_data_t s_timebase_info;

    //could move this elsewhere?
    if (s_timebase_info.denom == 0)
        (void) mach_timebase_info(&s_timebase_info);

    // mach_absolute_time() returns nanoseonds,
    return (timestamp_microsecs_t)((mach_absolute_time() * s_timebase_info.numer) / (1000 * s_timebase_info.denom));
}


/*----------------------------------------------------------*/
#elif defined __linux__
#include <sys/time.h>
timestamp_microsecs_t timestamp_get_current_time()
{
  timestamp_microsecs_t result;
  struct timeval t;
  gettimeofday(&t, NULL);
  
  result  = (t.tv_sec - 1428738298) * 1000000;
  result += t.tv_usec;
  
  return result;
}

#endif