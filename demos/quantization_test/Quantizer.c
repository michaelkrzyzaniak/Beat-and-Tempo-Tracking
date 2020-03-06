#include "Quantizer.h"
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h> //usleep

/*----------------------------------------------------------*/
struct opaque_quantizer_strcut
{
  pthread_t       realtime_update_thread;
  pthread_mutex_t realtime_mutex;
  int             realtime_is_running;
  unsigned        realtime_update_interval;
  unsigned        num_network_nodes;
  double*         network_nodes;
  double          network_error;
  quantizer_interaction_function_t interaction_function;
  double          peak;
  double          decay;
  void*           user_parameters;
};

void* quantizer_realtime_run_loop(void* SELF);

/*----------------------------------------------------------*/
Quantizer* quantizer_new                            (unsigned num_realtime_IOIs)
{
  if(num_realtime_IOIs < 1) return NULL;
  
  Quantizer* self = calloc(1, sizeof(*self));
  if(self != NULL)
    {
      self->network_nodes = calloc(num_realtime_IOIs, sizeof(*(self->network_nodes)));
      if(self->network_nodes == NULL) return quantizer_destroy(self);
      
      self->num_network_nodes = num_realtime_IOIs;
      self->interaction_function = quantizer_default_interaction_function;
      
      int error = pthread_mutex_init(&self->realtime_mutex, NULL);
      if(error != 0) return quantizer_destroy(self);
      
      self->network_error = -1;
      self->interaction_function = quantizer_default_interaction_function;
      self->user_parameters = self;
      self->peak   = 6;
      self->decay = -1;
    }
  return self;
}

/*----------------------------------------------------------*/
Quantizer* quantizer_destroy                        (Quantizer* self)
{
  if(self != NULL)
    {
      
      quantizer_realtime_stop(self);
      pthread_mutex_destroy(&self->realtime_mutex);
      
      if(self->network_nodes != NULL)
        free(self->network_nodes);
        
      free(self);
    }
  return (Quantizer*) NULL;
}

/*----------------------------------------------------------*/
double     quantizer_realtime_get_expectancy_at_time(Quantizer* self, double time)
{
  return quantizer_offline_get_expectancy_at_time (self->network_nodes, self->num_network_nodes, time);
}

/*----------------------------------------------------------*/
unsigned   quantizer_realtime_get_num_IOIs          (Quantizer* self)
{
  return self->num_network_nodes;
}

/*----------------------------------------------------------*/
void       quantizer_realtime_get_quantized_IOIs    (Quantizer* self, double* IOIs)
{
  unsigned i = self->num_network_nodes;
  double*  p = self->network_nodes;
  
  //should check error, but how should it be handled?
  pthread_mutex_lock(&self->realtime_mutex);
  while(i--) *IOIs++ = *p++;
  pthread_mutex_unlock(&self->realtime_mutex);
}

/*----------------------------------------------------------*/
//returns true on success
int        quantizer_realtime_start                 (Quantizer* self, unsigned update_interval_usecs)
{
  int success = 0;
  if(!self->realtime_is_running)
    {
      self->realtime_is_running = 1;
      success = pthread_create(&self->realtime_update_thread, NULL, quantizer_realtime_run_loop, self);
    }
  return success;
}

/*----------------------------------------------------------*/
void       quantizer_realtime_stop                  (Quantizer* self)
{
  if(self->realtime_is_running)
    {
      pthread_cancel(self->realtime_update_thread);
      pthread_join(self->realtime_update_thread, NULL);
    }
}

/*----------------------------------------------------------*/
int        quantizer_reatime_is_running             (Quantizer* self)
{
  return self->realtime_is_running;
}

/*----------------------------------------------------------*/
unsigned   quantizer_realtime_get_update_interval   (Quantizer* self)
{
  return self->realtime_update_interval;
}

/*----------------------------------------------------------*/
void       quantizer_realtime_set_update_interval   (Quantizer* self, unsigned update_interval_usecs)
{
  pthread_mutex_lock(&self->realtime_mutex);
  self->realtime_update_interval = update_interval_usecs;
  pthread_mutex_unlock(&self->realtime_mutex);
}

/*----------------------------------------------------------*/
void       quantizer_realtime_push                  (Quantizer* self, double IOI)
{
  unsigned i = self->num_network_nodes;
  double*  p = self->network_nodes;
  
  pthread_mutex_lock(&self->realtime_mutex);
  while(i-- > 1)
    {
      p[0] = p[1];
      p++;
    }
  *p = IOI;
  pthread_mutex_unlock(&self->realtime_mutex);
}

/*----------------------------------------------------------*/
void       quantizer_realtime_flush                 (Quantizer* self)
{
  unsigned i = self->num_network_nodes;
  double*  p = self->network_nodes;
  
  pthread_mutex_lock(&self->realtime_mutex);
  while(i-- > 0)
    *p++ = 0;

  pthread_mutex_unlock(&self->realtime_mutex);  
}

/*----------------------------------------------------------*/
double     quantizer_realtime_get_network_error     (Quantizer* self)
{
  double result;
  pthread_mutex_lock(&self->realtime_mutex);
  result = self->network_error;
  pthread_mutex_unlock(&self->realtime_mutex);
  return result;
}

/*----------------------------------------------------------*/
double     quantizer_offline_get_expectancy_at_time (double* IOIs, unsigned num_IOIs, double time)
{
  return 0;
}

/*----------------------------------------------------------*/
#include <stdio.h> //testing only
double     quantizer_offline_quantize_once(Quantizer* self, double* IOIs, unsigned num_IOIs)
{
  struct sum_cell_struct
    {
      double*  basic_cells;
      unsigned num_basic_cells;
      double   sum;
    }left_cell, right_cell, *low_cell, *high_cell;
  
  unsigned left_start_index;
  unsigned i;
  double change, ratio, min_IOI, delta, max_delta=0;
  
  min_IOI = IOIs[0];
  for(i=1; i<num_IOIs; i++)
    if(IOIs[i] < min_IOI)
      min_IOI = IOIs[i];
  
  for(left_cell.num_basic_cells=1; left_cell.num_basic_cells<num_IOIs; left_cell.num_basic_cells++)
    {
      for(left_start_index=0; left_start_index<num_IOIs-left_cell.num_basic_cells; left_start_index++)
        {
          left_cell.basic_cells  = IOIs + left_start_index;
          right_cell.basic_cells = left_cell.basic_cells + left_cell.num_basic_cells;
          
          for(right_cell.num_basic_cells=1; right_cell.num_basic_cells<=num_IOIs-left_cell.num_basic_cells - left_start_index; right_cell.num_basic_cells++)
            {
              //fprintf(stderr, "num_left_nodes: %u left_start_index: %u num_right_nodes: %u\r\n", left_cell.num_basic_cells, left_start_index, right_cell.num_basic_cells);
              left_cell.sum = 0;
              for(i=0; i<left_cell.num_basic_cells; i++)
                left_cell.sum += left_cell.basic_cells[i];
              right_cell.sum = 0;
              for(i=0; i<right_cell.num_basic_cells; i++)
                right_cell.sum += right_cell.basic_cells[i];
              
              if(left_cell.sum >= right_cell.sum)
                {high_cell = &left_cell; low_cell = &right_cell;}
              else
                {high_cell = &right_cell; low_cell = &left_cell;}
              
              if(low_cell->sum <= 0) continue;
              
              ratio  = high_cell->sum / low_cell->sum;
              change = self->interaction_function(self->user_parameters, ratio);  

              //this variant was included in an addendum to 1989 paper
              //delta = low_cell->sum * change;
              delta = min_IOI * change;
              delta /= 1 + ratio + change;
              
              //fprintf(stderr, "left_sum: %lf right_sum: %lf\r\n", left_cell.sum, right_cell.sum);
              //fprintf(stderr, "delta: %lf change: %lf, ratio: %lf\r\n", delta, change, ratio);
              
              for(i=0; i<high_cell->num_basic_cells; i++)
                high_cell->basic_cells[i] += delta * high_cell->basic_cells[i] / high_cell->sum;
              for(i=0; i<low_cell->num_basic_cells; i++)
                low_cell->basic_cells[i] -= delta * low_cell->basic_cells[i] / low_cell->sum;
                
              delta = fabs(delta);
              if(delta > max_delta)
                max_delta = delta;
            }
        }
    }
  return max_delta;
}

/*----------------------------------------------------------*/
void quantizer_snap_to_grid(double* IOIs, int n, double grid_duration)
{
  int i;
  for(i=0; i<n; i++)
    {
      double new = round(IOIs[i]/grid_duration) * grid_duration;
      if(i < n-2)
        IOIs[i+1] = IOIs[i] - new;
      IOIs[i] = new;
    }
}

/*----------------------------------------------------------*/
void quantizer_intervals_to_times(double* IOIs, int n, double start, double divisor)
{
  if(n<=0) return;
  
  int i;
  double temp = IOIs[0] / divisor;
  IOIs[0] = start;
  
  for(i=1; i<n; i++)
    IOIs[i] = IOIs[i-1] + (IOIs[i]/divisor - temp);
}

/*----------------------------------------------------------*/
double     quantizer_offline_quantize(Quantizer* self, double* IOIs, unsigned num_IOIs, int max_num_reps, double max_delta)
{
  double delta = -1;

  if(max_num_reps > 0)
    {
      int i;
      for(i=0; i<max_num_reps; i++)
        {
          delta = quantizer_offline_quantize_once(self, IOIs, num_IOIs);
          if(delta < max_delta)
            break;
        }
    }    
  else
    {
      for(;;)
        {
          delta = quantizer_offline_quantize_once(self, IOIs, num_IOIs);
          if(delta < max_delta)
            break;
        }      
    }
  
  return delta;
}

/*----------------------------------------------------------*/
double*    quantizer_offline_get_quantized_IOIs     (Quantizer* self)
{
  return self->network_nodes;
}

/*----------------------------------------------------------*/
void       quantizer_set_interaction_function       (Quantizer* self, quantizer_interaction_function_t interaction_function, void* user_parameters)
{
  user_parameters = (interaction_function == quantizer_default_interaction_function) ? (void*)self : user_parameters;
  pthread_mutex_lock(&self->realtime_mutex);
  self->user_parameters = user_parameters;
  self->interaction_function = interaction_function;
  pthread_mutex_unlock(&self->realtime_mutex);
}

/*----------------------------------------------------------*/
quantizer_interaction_function_t quantizer_get_interaction_function(Quantizer* self, void** user_parameters)
{
  void* params;
  quantizer_interaction_function_t result;

  result = self->interaction_function;
  params = self->user_parameters;
  
  if(user_parameters != NULL)
    *user_parameters = params;
  return result;
}

/*----------------------------------------------------------*/
void       quantizer_set_default_interaction_function_peak_parameter (Quantizer* self, double peak)
{
  pthread_mutex_lock(&self->realtime_mutex);
  self->peak = peak;
  pthread_mutex_unlock(&self->realtime_mutex);
}

/*----------------------------------------------------------*/
double     quantizer_get_default_interaction_function_peak_parameter (Quantizer* self)
{
  return self->peak;
}

/*----------------------------------------------------------*/
void       quantizer_set_default_interaction_function_decay_parameter(Quantizer* self, double decay)
{
  pthread_mutex_lock(&self->realtime_mutex);
  self->decay = decay;
  pthread_mutex_unlock(&self->realtime_mutex);
}

/*----------------------------------------------------------*/
double     quantizer_get_default_interaction_function_decay_parameter(Quantizer* self)
{
  return self->decay;
}

/*----------------------------------------------------------*/
double quantizer_default_interaction_function(void* user_parameters, double r /*will be >= 1*/)
{
  Quantizer* self = (Quantizer*)user_parameters;
  double round_r = round(r);
  double result;
  
  result  = 2 * (r - floor(r) - 0.5);
  result  = fabs(result);
  result  = pow(result, self->peak);
  result *= round_r - r;
  result *= pow(round_r, self->decay);
  
  return result;
}

/*----------------------------------------------------------*/
void* quantizer_realtime_run_loop(void* SELF)
{
  Quantizer* self = (Quantizer*)SELF;
  unsigned usecs;
  double error;
  
  for(;;)
    {
      pthread_mutex_lock(&self->realtime_mutex);      
      //if(self->network_error > self->min_network_error)
        {
          error = quantizer_offline_quantize(self, self->network_nodes, self->num_network_nodes, 1, -1);
          self->network_error = error;
        }
      usecs = self->realtime_update_interval;
      pthread_mutex_unlock(&self->realtime_mutex);

      usleep(usecs);
    }
  return NULL;
}
