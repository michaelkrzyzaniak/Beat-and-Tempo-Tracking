/*__--------------_-----------_---------------------__---------------------
 / _|___ _  _ _ _(_)___ _ _  | |_ _ _ __ _ _ _  ___/ _|___ _ _ _ __  
|  _/ _ \ || | '_| / -_) '_| |  _| '_/ _` | ' \(_-<  _/ _ \ '_| '  \ 
|_| \___/\_,_|_| |_\___|_|    \__|_| \__,_|_||_/__/_| \___/_| |_|_|_|
-------------------------------------------------------------------------*/

#ifndef __DFT__
#define __DFT__ 1

#if defined(__cplusplus)
extern "C"{
#endif   //(__cplusplus)

typedef float dft_sample_t;

void   dft_init_blackman_window (dft_sample_t* window, int N);
void   dft_init_hann_window     (dft_sample_t* window, int N);
void   dft_init_hamming_window  (dft_sample_t* window, int N);
void   dft_init_half_sine_window(dft_sample_t* window, int N);
void   dft_apply_window         (dft_sample_t* real,   dft_sample_t* window, int N);

void   dft_raw_forward_dft      (dft_sample_t* real,   dft_sample_t* imag,   int N);
void   dft_raw_inverse_dft      (dft_sample_t* real,   dft_sample_t* imag,   int N);
void   dft_bit_reverse_indices  (dft_sample_t* real,   dft_sample_t* imag,   int N);
void   dft_complex_forward_dft  (dft_sample_t* real,   dft_sample_t* imag,   int N);
void   dft_complex_inverse_dft  (dft_sample_t* real,   dft_sample_t* imag,   int N);
void   dft_real_forward_dft     (dft_sample_t* real,   dft_sample_t* imag,   int N);
void   dft_real_inverse_dft     (dft_sample_t* real,   dft_sample_t* imag,   int N);
void   dft_2_real_forward_dfts  (dft_sample_t* real_1, dft_sample_t* real_2, dft_sample_t* imag_1, dft_sample_t* imag_2, int N);
void   dft_2_real_inverse_dfts  (dft_sample_t* real_1, dft_sample_t* real_2, dft_sample_t* imag_1, dft_sample_t* imag_2, int N);

/* user should zero-pad data to twice its original length for correlation and convolution */
void   dft_real_convolve        (dft_sample_t* real_1, dft_sample_t* real_2, dft_sample_t* imag_1, dft_sample_t* imag_2, int N);
void   dft_real_correlate       (dft_sample_t* real_1, dft_sample_t* real_2, dft_sample_t* imag_1, dft_sample_t* imag_2, int N);
void   dft_real_autocorrelate   (dft_sample_t* real  , dft_sample_t* imag  , int N);
/* generalized cross-correlation with phase transform */
void   dft_real_generalized_autocorrelation  (dft_sample_t* real  , dft_sample_t* imag, int N, double exponent);

void   dft_rect_to_polar        (dft_sample_t* real,   dft_sample_t* imag,   int N);
void   dft_polar_to_rect        (dft_sample_t* real,   dft_sample_t* imag,   int N);
void   dft_magnitude_to_db      (dft_sample_t* real,   int N);
void   dft_normalize_magnitude  (dft_sample_t* real,   int N);

double dft_bin_of_frequency     (double hz, double sample_rate, int N);
double dft_frequency_of_bin     (double bin, double sample_rate, int N);
int    dft_smallest_power_of_2_at_least_as_great_as(int n);

#if defined(__cplusplus)
}
#endif   //(__cplusplus)

#endif   // __DFT__
