#ifndef __MK_DESAIN_QUANTIZER__
#define __MK_DESAIN_QUANTIZER__

// implementation of 
// Desain, P. and Honing, H. 1989. The quantization of musical time: A connectionist approach. Computer Music Journal. (1989), 56-66
// by Michael Krzyzaniak

#if defined(__cplusplus)
extern "C"{
#endif   //(__cplusplus)

/*----------------------------------------------------------*/
typedef struct opaque_quantizer_strcut Quantizer;

//must retun amount of change of ratio
typedef double (*quantizer_interaction_function_t)(void* user_parameters, double ratio /*will be >= 1*/);
double quantizer_default_interaction_function(void* user_parameters, double r /*will be >= 1*/);

/*----------------------------------------------------------*/
Quantizer* quantizer_new                            (unsigned num_realtime_IOIs);
Quantizer* quantizer_destroy                        (Quantizer* self);
double     quantizer_realtime_get_expectancy_at_time(Quantizer* self, double time);

unsigned   quantizer_realtime_get_num_IOIs          (Quantizer* self);
void       quantizer_realtime_get_quantized_IOIs    (Quantizer* self, double* IOIs);
int        quantizer_realtime_start                 (Quantizer* self, unsigned update_interval_usecs);//returns true on success
void       quantizer_realtime_stop                  (Quantizer* self);
int        quantizer_reatime_is_running             (Quantizer* self);
unsigned   quantizer_realtime_get_update_interval   (Quantizer* self);
void       quantizer_realtime_set_update_interval   (Quantizer* self, unsigned update_interval_usecs);
void       quantizer_realtime_push                  (Quantizer* self, double IOI);
void       quantizer_realtime_flush                 (Quantizer* self);
double     quantizer_realtime_get_network_error     (Quantizer* self);
//double     quantizer_realtime_get_min_network_error (Quantizer* self);
//double     quantizer_realtime_set_min_network_error (Quantizer* self, double);

//return network error after max_num_reps or max_network_error is surpassed. Pass <0 for either if no care
double*    quantizer_offline_get_quantized_IOIs     (Quantizer* self);
double     quantizer_offline_get_expectancy_at_time (double* IOIs, unsigned num_IOIs, double time);
double     quantizer_offline_quantize               (Quantizer* self, 
                                                     double* IOIs,
                                                     unsigned num_IOIs,
                                                     int      max_num_reps,
                                                     double   max_delta);

/* if interaction function is quantizer_default_interaction_function,  user_parameters will be ignored and self will be used */
void       quantizer_set_interaction_function                        (Quantizer* self, quantizer_interaction_function_t interaction_function, void* user_parameters);
quantizer_interaction_function_t quantizer_get_interaction_function(Quantizer* self, void** user_parameters);
void       quantizer_set_default_interaction_function_peak_parameter (Quantizer* self, double peak /*usually 2 ~ 12*/);
double     quantizer_get_default_interaction_function_peak_parameter (Quantizer* self);
void       quantizer_set_default_interaction_function_decay_parameter(Quantizer* self, double decay /*usually -1 ~ -3*/);
double     quantizer_get_default_interaction_function_decay_parameter(Quantizer* self);

void       quantizer_snap_to_grid(double* IOIs, int n, double grid_duration);
void       quantizer_intervals_to_times(double* IOIs, int n, double start, double divisor);

/*----------------------------------------------------------*/
#if defined(__cplusplus)
}
#endif   //(__cplusplus)

#endif //__MK_DESAIN_QUANTIZER__
