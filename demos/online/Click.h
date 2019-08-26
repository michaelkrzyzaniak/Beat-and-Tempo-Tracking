/*
 *  Click.h
 *  Make weird noises
 *
 *  Made by Michael Krzyzaniak at Arizona State University's
 *  School of Arts, Media + Engineering in Spring of 2013
 *  mkrzyzan@asu.edu
 */

#ifndef __CLICK__
#define __CLICK__ 1

#if defined(__cplusplus)
extern "C"{
#endif   //(__cplusplus)

#include "AudioSuperclass.h"

typedef struct OpaqueClickStruct Click;

Click*       click_new               ();
void         click_click             (Click* self);

//Click*  click_destroy             (Click*      self      );
//call with self->destroy(self);

#if defined(__cplusplus)
}
#endif   //(__cplusplus)

#endif   // __CLICK__
