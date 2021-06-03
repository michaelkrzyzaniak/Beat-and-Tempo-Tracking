#include "STFT.h"

#include <stdlib.h> //calloc

/*--------------------------------------------------------------------*/
struct Opaque_STFT_Struct
{
  int           should_resynthesize;
  int           window_size;
  int           fft_N;
  int           overlap;
  int           hop_size;
  int           sample_counter;
  int           input_index;
  int           output_index;;
  dft_sample_t* window;
  dft_sample_t* running_input;
  dft_sample_t* running_output;
  dft_sample_t* real;
  dft_sample_t* imag;
};

/*--------------------------------------------------------------------*/
STFT* stft_new(int window_size /*power of 2 please*/, int overlap /* 1, 2, 4, 8 */, int should_resynthesize)
{
  STFT* self = calloc(1, sizeof(*self));
  if(self != NULL)
    {
      self->should_resynthesize  = should_resynthesize;
      self->window_size          = window_size;
      self->fft_N                = (should_resynthesize) ? overlap * window_size : window_size;
      self->overlap              = overlap;
      self->hop_size             = window_size / overlap;
      self->sample_counter       = 0;
      self->input_index          = 0;
      self->output_index         = 0;
      self->window               = calloc(self->window_size, sizeof(*(self->window)));
      self->running_input        = calloc(self->window_size, sizeof(*(self->running_input)));
      self->running_output       = calloc(self->fft_N      , sizeof(*(self->running_output)));
      self->real                 = calloc(self->fft_N      , sizeof(*(self->real)));
      self->imag                 = calloc(self->fft_N      , sizeof(*(self->imag)));
      if(self->window           == NULL) return stft_destroy(self);
      if(self->running_input    == NULL) return stft_destroy(self);
      if(self->running_output   == NULL) return stft_destroy(self);
      if(self->real             == NULL) return stft_destroy(self);
      if(self->imag             == NULL) return stft_destroy(self);
  
      //no choice for now
      dft_init_blackman_window(self->window, self->window_size);
  }
  return self;
}

/*--------------------------------------------------------------------*/
STFT* stft_destroy(STFT* self)
{
  if(self != NULL)
    {
      if(self->window != NULL)
        free(self->window);
      if(self->running_input != NULL)
        free(self->running_input);
      if(self->running_output != NULL)
        free(self->running_output);
      if(self->real != NULL)
        free(self->real);
      if(self->imag != NULL)
        free(self->imag);
    
      free(self);
    }
  return (STFT*) NULL;
}

/*--------------------------------------------------------------------*/
/* resynthesized samples, if any, returned in real_input */
void stft_process(STFT* self, dft_sample_t* real_input, int len, stft_onprocess_t onprocess, void* onprocess_self)
{
  int i, j;

  for(i=0; i<len; i++)
    {
      self->running_input[self->input_index++] = real_input[i];
      self->input_index %= self->window_size;
    
      if(self->should_resynthesize)
        {
          real_input[i] = self->running_output[self->output_index];
          self->running_output[self->output_index++] = 0;
          self->output_index %= self->fft_N;
        }
    
      if(++self->sample_counter == self->hop_size)
        {
          self->sample_counter = 0;
        
          for(j=0; j<self->window_size; j++)
            {
              self->real[j] = self->running_input[(self->input_index+j) % self->window_size];
              self->imag[j] = 0;
            }

          dft_apply_window(self->real, self->window, self->window_size);
          for(j=self->window_size; j<self->fft_N; j++)
            {
              self->real[j] = 0;
              self->imag[j] = 0;
            }
        
          dft_real_forward_dft(self->real, self->imag, self->fft_N);
          onprocess           (onprocess_self, self->real, self->imag, self->fft_N);
          if(self->should_resynthesize)
            {
              dft_real_inverse_dft(self->real, self->imag, self->fft_N);
              for(j=0; j<self->fft_N; j++)
                self->running_output[(self->output_index+j) % self->fft_N] += self->real[j];
            }
        }
    }
}

/*--------------------------------------------------------------------*/
int   stft_get_N (STFT* self)
{
  return self->fft_N;
}

/*--------------------------------------------------------------------*/
int     stft_get_overlap (STFT* self)
{
  return self->overlap;
}

/*--------------------------------------------------------------------*/
int     stft_get_hop     (STFT* self)
{
  return self->hop_size;
}

/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
struct Opaque_TWO_STFTS_Struct
{
  int           should_resynthesize;
  int           window_size;
  int           fft_N;
  int           overlap;
  int           hop_size;
  int           sample_counter;
  int           input_index;
  int           output_index;;
  dft_sample_t* window;
  dft_sample_t* running_input;
  dft_sample_t* running_output;
  dft_sample_t* real;
  dft_sample_t* imag;
  dft_sample_t* running_input_2;
  dft_sample_t* running_output_2;
  dft_sample_t* real_2;
  dft_sample_t* imag_2;
};



/*--------------------------------------------------------------------*/
TWO_STFTS* two_stfts_new(int window_size /*power of 2 please*/, int overlap /* 1, 2, 4, 8 */, int should_resynthesize)
{
  TWO_STFTS* self = calloc(1, sizeof(*self));
  if(self != NULL)
    {
      self->should_resynthesize  = should_resynthesize;
      self->window_size          = window_size;
      self->fft_N                = (should_resynthesize) ? overlap * window_size : window_size;
      self->overlap              = overlap;
      self->hop_size             = window_size / overlap;
      self->sample_counter       = 0;

      self->input_index          = 0;
      self->output_index         = 0;

      self->window               = calloc(self->window_size, sizeof(*(self->window)));
      self->running_input        = calloc(self->window_size, sizeof(*(self->running_input)));
      self->running_output       = calloc(self->fft_N      , sizeof(*(self->running_output)));
      self->real                 = calloc(self->fft_N      , sizeof(*(self->real)));
      self->imag                 = calloc(self->fft_N      , sizeof(*(self->imag)));
      self->running_input_2      = calloc(self->window_size, sizeof(*(self->running_input_2)));
      self->running_output_2     = calloc(self->fft_N      , sizeof(*(self->running_output_2)));
      self->real_2               = calloc(self->fft_N      , sizeof(*(self->real_2)));
      self->imag_2               = calloc(self->fft_N      , sizeof(*(self->imag_2)));

      if(self->window           == NULL) return two_stfts_destroy(self);
      if(self->running_input    == NULL) return two_stfts_destroy(self);
      if(self->running_output   == NULL) return two_stfts_destroy(self);
      if(self->real             == NULL) return two_stfts_destroy(self);
      if(self->imag             == NULL) return two_stfts_destroy(self);
      if(self->running_input_2  == NULL) return two_stfts_destroy(self);
      if(self->running_output_2 == NULL) return two_stfts_destroy(self);
      if(self->real_2           == NULL) return two_stfts_destroy(self);
      if(self->imag_2           == NULL) return two_stfts_destroy(self);

      //no choice for now
      dft_init_blackman_window(self->window, self->window_size);
  }
  
  return self;
}

/*--------------------------------------------------------------------*/
TWO_STFTS* two_stfts_destroy(TWO_STFTS* self)
{
  if(self != NULL)
    {
      if(self->window != NULL)
        free(self->window);
      if(self->running_input != NULL)
        free(self->running_input);
      if(self->running_output != NULL)
        free(self->running_output);
      if(self->real != NULL)
        free(self->real);
      if(self->imag != NULL)
        free(self->imag);
      if(self->running_input_2 != NULL)
        free(self->running_input_2);
      if(self->running_output_2 != NULL)
        free(self->running_output_2);
      if(self->real_2 != NULL)
        free(self->real_2);
      if(self->imag_2 != NULL)
        free(self->imag_2);
      free(self);
    }
  return (TWO_STFTS*) NULL;
}

/*--------------------------------------------------------------------*/
void two_stfts_process(TWO_STFTS* self, dft_sample_t* real_input, dft_sample_t* real_input_2, int len, int two_inverses, two_stfts_onprocess_t onprocess, void* onprocess_self)
{
  int i, j;

  for(i=0; i<len; i++)
    {
      self->running_input[self->input_index] = real_input[i];
      self->running_input_2[self->input_index] = real_input_2[i];
      ++self->input_index; self->input_index %= self->window_size;
    
      //?
      if(self->should_resynthesize)
        {
          real_input[i] = self->running_output[self->output_index];
          self->running_output[self->output_index] = 0;
          if(two_inverses)
            {
              real_input_2[i] = self->running_output_2[self->output_index];
              self->running_output_2[self->output_index] = 0;
            }
          ++self->output_index; self->output_index %= self->fft_N;
        }

      ++self->sample_counter;

      //new dft if necessary
      if(self->sample_counter == self->hop_size)
        {
          self->sample_counter = 0;
        
          for(j=0; j<self->window_size; j++)
            {
              self->real[j] = self->running_input[(self->input_index+j) % self->window_size];
              self->imag[j] = 0;
              self->real_2[j] = self->running_input_2[(self->input_index+j) % self->window_size];
              self->imag_2[j] = 0;
            }

          dft_apply_window(self->real, self->window, self->window_size);
          dft_apply_window(self->real_2, self->window, self->window_size);
          for(j=self->window_size; j<self->fft_N; j++)
            {
              self->real[j] = 0;
              self->imag[j] = 0;
              self->real_2[j] = 0;
              self->imag_2[j] = 0;
            }

          dft_2_real_forward_dfts  (self->real, self->real_2, self->imag, self->imag_2, self->fft_N);
          onprocess                  (onprocess_self, self->real, self->real_2, self->imag, self->imag_2, self->fft_N);

          if(self->should_resynthesize)
            {
              if(two_inverses)
                dft_2_real_inverse_dfts(self->real, self->real_2, self->imag, self->imag_2, self->fft_N);
              else
                dft_real_inverse_dft(self->real, self->imag, self->fft_N);
        
              for(j=0; j<self->fft_N; j++)
                {
                  self->running_output[(self->output_index+j) % self->fft_N] += self->real[j];
                  if(two_inverses)
                    self->running_output_2[(self->output_index+j) % self->fft_N] += self->real_2[j];
                }
            }
        }
    }
}
