/*__--------------_-----------_---------------------__---------------------

-------------------------------------------------------------------------*/

#ifndef __STFT__
#define __STFT__ 1

#if defined(__cplusplus)
extern "C"{
#endif   //(__cplusplus)

#include "DFT.h"

/*--------------------------------------------------------------------*/
typedef struct Opaque_STFT_Struct STFT;

typedef void (*stft_onprocess_t)(void* onprocess_self, dft_sample_t* real, dft_sample_t* imag, int N);

STFT*   stft_new     (int window_size /*power of 2 please*/, int overlap /* 1, 2, 4, 8 */, int should_resynthesize);
STFT*   stft_destroy (STFT* self);
void    stft_process (STFT* self, dft_sample_t* real_input, int len, stft_onprocess_t onprocess, void* onprocess_self);

/*--------------------------------------------------------------------*/
typedef struct Opaque_TWO_STFTS_Struct TWO_STFTS;

typedef void (*two_stfts_onprocess_t)(void* onprocess_self, dft_sample_t* real, dft_sample_t* imag, dft_sample_t* real_2, dft_sample_t* imag_2, int N);

TWO_STFTS*   two_stfts_new(int window_size /*power of 2 please*/, int overlap /* 1, 2, 4, 8 */, int should_resynthesize);
TWO_STFTS*   two_stfts_destroy(TWO_STFTS* self);
void         two_stfts_process(TWO_STFTS* self, dft_sample_t* real_input, dft_sample_t* real_input_2, int len, int two_inverses, two_stfts_onprocess_t onprocess, void* onprocess_self);

#if defined(__cplusplus)
}
#endif   //(__cplusplus)

#endif   // __DFT__
