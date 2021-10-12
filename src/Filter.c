#include "Filter.h"
#include "math.h"
#include <stdlib.h>

#define TWO_PI (M_PI * 2.0)

/*-------------------------------------------------*/
void filter_init_lowpass_coeffs  (Filter* self);
void filter_init_highpass_coeffs (Filter* self);
void filter_init_bandpass_coeffs (Filter* self);
void filter_init_bandstop_coeffs (Filter* self);
void filter_init_rect_window     (Filter* self);
void filter_init_hann_window     (Filter* self);
void filter_init_hamming_window  (Filter* self);
void filter_init_blackmann_window(Filter* self);

/*-------------------------------------------------*/
struct opaque_filter_struct
{
  filter_type_t   type;
  filter_window_t window_type;
  int             order, max_order;
  float           cutoff;
  float           sample_rate;
  float*          prev_samples;
  float*          window;
  float*          coeffs;
};

/*-------------------------------------------------*/
Filter* filter_new(filter_type_t type, float cutoff, int order)
{
  Filter* self = calloc(1, sizeof(*self));
  if(self != NULL)
    {
      self->type         = type;
      self->order        = order;
      self->max_order    = order;
      self->cutoff       = cutoff;
      self->sample_rate  = 44100;
      self->prev_samples = calloc(order+1, sizeof(*(self->prev_samples)));
      if(self->prev_samples == NULL) return filter_destroy(self);
      self->window = calloc(order+1, sizeof(*(self->window)));
      if(self->window == NULL) return filter_destroy(self);
      self->coeffs = calloc(order+1, sizeof(*(self->coeffs)));
      if(self->coeffs == NULL) return filter_destroy(self);
      filter_set_window_type(self, FILTER_WINDOW_BLACKMANN); //triggers calculation of window and coeffs
    }
  
  return self;
}

/*-------------------------------------------------*/
Filter* filter_destroy(Filter* self)
{
  if(self != NULL)
    {
      if(self->window != NULL)
        free(self->window);
      if(self->coeffs != NULL)
        free(self->coeffs);
      if(self->prev_samples != NULL)
        free(self->prev_samples);

      free(self);
    }
  return (Filter*) NULL;
}

/*-------------------------------------------------*/
void            filter_clear          (Filter* self)
{
  int i;
  for(i=0; i<self->order+1; i++)
    self->prev_samples[i] = 0;
}

/*-------------------------------------------------*/
void filter_set_filter_type(Filter* self, filter_type_t type)
{
  if((self->type == type) /*|| (type < 0)*/ || (type >= FILTER_NUMBER_OF_TYPES)) 
    return;
  
  self->type = type;
  filter_set_cutoff(self, self->cutoff);
}

/*-------------------------------------------------*/
filter_type_t filter_get_filter_type(Filter* self)
{
  return self->type;
}

/*-------------------------------------------------*/
void filter_set_sample_rate(Filter* self, float sample_rate)
{
  self->sample_rate = sample_rate;
  filter_set_cutoff(self, self->cutoff);
}

/*-------------------------------------------------*/
float filter_get_sample_rate(Filter* self)
{
  return self->sample_rate;
}

/*-------------------------------------------------*/
void filter_set_cutoff(Filter* self, float cutoff)
{
  self->cutoff = cutoff;
  void (*init_coeffs)(Filter*) = NULL;
  
  switch(self->type)
    {
      case FILTER_LOW_PASS : 
        init_coeffs = filter_init_lowpass_coeffs;
        break;
      case FILTER_HIGH_PASS:
        init_coeffs = filter_init_highpass_coeffs;
        break;
      case FILTER_BAND_PASS:
        init_coeffs = filter_init_bandpass_coeffs;
        break;
      case FILTER_BAND_STOP:
        init_coeffs = filter_init_bandstop_coeffs;
        break;

      default:break;
    }
  if(init_coeffs != NULL)
    init_coeffs(self);
}

/*-------------------------------------------------*/
float filter_get_cutoff(Filter* self)
{
  return self->cutoff;
}

/*-------------------------------------------------*/
void filter_set_order(Filter* self, int order)
{
  if(order > self->max_order)
    order = self->max_order;
  if(order < 0)
    order = 0;

  self->order = order;
  filter_set_window_type(self, self->window_type); //recalculate window and coeffs
}

/*-------------------------------------------------*/
int filter_get_order(Filter* self)
{
  return self->order;
}

/*-------------------------------------------------*/
void filter_set_window_type(Filter* self, filter_window_t window)
{
  if(window == self->window_type) return;

  self->window_type = window;
  void (*init_window)(Filter*) = NULL;
  
  switch(self->window_type)
    {
      case FILTER_WINDOW_RECT : 
        init_window = filter_init_rect_window;
        break;
      case FILTER_WINDOW_HANN:
        init_window = filter_init_hann_window;
        break;
      case FILTER_WINDOW_HAMMING:
        init_window = filter_init_hamming_window;
        break;
      case FILTER_WINDOW_BLACKMANN:
        init_window = filter_init_blackmann_window;
        break;

      default:break;
    }
  if(init_window != NULL)
    init_window(self);

  filter_set_cutoff(self, self->cutoff);  
}

/*-------------------------------------------------*/
filter_window_t filter_get_window_type(Filter* self)
{
  return self->window_type;
}

/*-------------------------------------------------*/
void filter_init_lowpass_coeffs(Filter* self)
{
  int i, n=self->order+1;
  float m_over_2 = self->order / 2.0;
  float f_t = self->cutoff / self->sample_rate;
  float two_pi_f_t = TWO_PI * f_t;  
  float i_minus_m_over_2;
  float temp;

  for(i=0; i<n; i++)
    {
      if(i != m_over_2)
        {
          i_minus_m_over_2 = i - m_over_2;
          temp = sin(two_pi_f_t * i_minus_m_over_2);
          temp /= M_PI * i_minus_m_over_2;
        }
      else
        temp = 2.0 * f_t;

      temp *= self->window[i];
      self->coeffs[i] = temp;
    }  
}

/*-------------------------------------------------*/
void filter_init_highpass_coeffs(Filter* self)
{
  /* to do:                                                */
  /* this is almost identical to filter_init_lowpass_coeffs*/
  /* and could be serviced by the same function            */

  int i, n=self->order+1;
  float m_over_2 = self->order / 2.0;
  float f_t = self->cutoff / self->sample_rate;
  float two_pi_f_t = TWO_PI * f_t;  
  float i_minus_m_over_2;
  float temp;

  for(i=0; i<n; i++)
    {
      if(i != m_over_2)
        {
          i_minus_m_over_2 = i - m_over_2;
          temp = sin(two_pi_f_t * i_minus_m_over_2);
          temp /= M_PI * i_minus_m_over_2;
          temp *= -1;
        }
      else
        temp = 1 - (2.0 * f_t);

      temp *= self->window[i];
      self->coeffs[i] = temp;
    }
}

/*-------------------------------------------------*/
void filter_init_bandpass_coeffs(Filter* self)
{

}

/*-------------------------------------------------*/
void filter_init_bandstop_coeffs(Filter* self)
{

}

/*-------------------------------------------------*/
void filter_init_rect_window(Filter* self)
{
  int i, n = self->order + 1;
  for(i=0; i<n; i++)
    self->window[i] = 1;
}

/*-------------------------------------------------*/
void filter_init_hann_window(Filter* self)
{
  int i, n = self->order + 1;
  double phase = 0;
  double phase_increment = TWO_PI / (double)self->order;

  for(i=0; i<n; i++)
    {
      self->window[i] = 0.5 * (1-cos(phase));
      phase += phase_increment;
    }
}

/*-------------------------------------------------*/
void filter_init_hamming_window(Filter* self)
{
  int i, n = self->order + 1;
  double phase = 0;
  double phase_increment = TWO_PI / (double)self->order;
  for(i=0; i<n; i++)
    {
      self->window[i] = 0.54 - 0.46 * cos(phase);
      phase += phase_increment;
    }  
}

/*-------------------------------------------------*/
void filter_init_blackmann_window(Filter* self)
{
  int i, n = self->order + 1;
  double phase = 0;
  double phase_increment = TWO_PI / (double)self->order;
  double a = 0;
  double a0 = (1-a)/2.0;
  double a1 = 1/2.0;
  double a2 = a/2.0;
  
  for(i=0; i<n; i++)
    {
      self->window[i] = a0 - a1*cos(phase) + a2*cos(2*phase);
      phase += phase_increment;
    } 
}

/*-------------------------------------------------*/
void    filter_process_data(Filter* self, float* data, int num_samples)
{
  int i, n = self->order + 1;
  float output;

  while(num_samples-- > 0)
    {
      //shift previous samples
      for(i=self->order; i>0; i--)
        self->prev_samples[i] = self->prev_samples[i-1];
      self->prev_samples[0] = *data;

      //calculate filter output
      output = 0;      
      for(i=0; i<n; i++)
        output += self->prev_samples[i] * self->coeffs[i];

      *data++ = output;
    }
}

