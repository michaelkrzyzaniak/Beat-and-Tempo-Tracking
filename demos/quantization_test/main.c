//gcc *.c ../../src/*.c

#include "Quantizer.h"
#include "../../src/Statistics.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#define NUM_REALTIME_IOIS  64

void debug_print_iois(Quantizer* quantizer)
{
  unsigned i, n = quantizer_realtime_get_num_IOIs(quantizer);
  double iois[n];
  quantizer_realtime_get_quantized_IOIs(quantizer, iois);
  OnlineAverage* average = online_average_new();
  
  //fprintf(stderr, "[ ");
  for(i=0; i<n; i++)
    {
      //fprintf(stderr, "%.2lf ", iois[i]);
      online_average_update(average, iois[i]);
    }
  //fprintf(stderr, "]\r\n");
  fprintf(stderr, "mean: %f\tstd_dev: %f\r\n", online_average_mean(average), online_average_std_dev(average));
  average = online_average_destroy(average);
}

int main(void)
{
  Quantizer* quantizer = quantizer_new(NUM_REALTIME_IOIS);

  srandom(time(NULL));

  int i;
  for(i=0; i<NUM_REALTIME_IOIS; i++)
    {
      double ioi = statistics_random_normal(100.0, 0.0);
      quantizer_realtime_push(quantizer, ioi);
    }
  
  debug_print_iois(quantizer);
  
  double* iois = quantizer_offline_get_quantized_IOIs(quantizer);
  
  for(i=0; i<10; i++)
    {
      quantizer_offline_quantize(quantizer, iois, NUM_REALTIME_IOIS, 1, -1);
      debug_print_iois(quantizer);
    }
  
  quantizer = quantizer_destroy(quantizer);
}

