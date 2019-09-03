#include "Microphone.h"

void i_hate_canonical_input_processing(void);
void make_stdin_cannonical_again();

typedef void   (*double_setter)(BTT* self, double val);
typedef double (*double_getter)(BTT* self);
typedef void   (*int_setter)   (BTT* self, int val);
typedef int    (*int_getter)   (BTT* self);
typedef void   (*funct)        (void);

typedef struct parameter_struct
{
  funct  set;
  funct  get;
  char   type;
  double init;
  double increment;
  char   name[128];
}param_t;

param_t params[] =
{
  {
    .set = (funct)btt_set_tracking_mode,
    .get = (funct)btt_get_tracking_mode,
    .type = 'i',
    .init = BTT_DEFAULT_TRACKING_MODE,
    .increment = 1,
    .name = "btt_set_tracking_mode",
  },
  {
    .set = (funct)btt_set_noise_cancellation_threshold,
    .get = (funct)btt_get_noise_cancellation_threshold,
    .type = 'd',
    .init = BTT_DEFAULT_NOISE_CANCELLATION_THRESHOLD,
    .increment = 1,
    .name = "btt_set_noise_cancellation_threshold",
  },
  {
    .set = (funct)btt_set_use_amplitude_normalization,
    .get = (funct)btt_get_use_amplitude_normalization,
    .type = 'i',
    .init = BTT_DEFAULT_USE_AMP_NORMALIZATION,
    .increment = 1,
    .name = "btt_set_use_amplitude_normalization",
  },
  {
    .set = (funct)btt_set_spectral_compression_gamma,
    .get = (funct)btt_get_spectral_compression_gamma,
    .type = 'd',
    .init = BTT_DEFAULT_SPECTRAL_COMPRESSION_GAMMA,
    .increment = 0.05,
    .name = "btt_set_spectral_compression_gamma",
  },
  {
    .set = (funct)btt_set_oss_filter_cutoff,
    .get = (funct)btt_get_oss_filter_cutoff,
    .type = 'd',
    .init = BTT_DEFAULT_OSS_FILTER_CUTOFF,
    .increment = 0.5,
    .name = "btt_set_oss_filter_cutoff",
  },
  {
    .set = (funct)btt_set_onset_threshold,
    .get = (funct)btt_get_onset_threshold,
    .type = 'd',
    .init = BTT_DEFAULT_ONSET_TREHSHOLD,
    .increment = 0.1,
    .name = "btt_set_onset_threshold",
  },
  {
    .set = (funct)btt_set_onset_threshold_min,
    .get = (funct)btt_get_onset_threshold_min,
    .type = 'd',
    .init = BTT_DEFAULT_ONSET_TREHSHOLD_MIN,
    .increment = 0.01,
    .name = "btt_set_onset_threshold_min",
  },
  {
    .set = (funct)btt_set_autocorrelation_exponent,
    .get = (funct)btt_get_autocorrelation_exponent,
    .type = 'd',
    .init = BTT_DEFAULT_AUTOCORRELATION_EXPONENT,
    .increment = 0.1,
    .name = "btt_set_autocorrelation_exponent",
  },
  {
    .set = (funct)btt_set_min_tempo,
    .get = (funct)btt_get_min_tempo,
    .type = 'd',
    .init = BTT_DEFAULT_MIN_TEMPO,
    .increment = 4,
    .name = "btt_set_min_tempo",
  },
  {
    .set = (funct)btt_set_max_tempo,
    .get = (funct)btt_get_max_tempo,
    .type = 'd',
    .init = BTT_DEFAULT_MAX_TEMPO,
    .increment = 4,
    .name = "btt_set_max_tempo",
  },
  {
    .set = (funct)btt_set_num_tempo_candidates,
    .get = (funct)btt_get_num_tempo_candidates,
    .type = 'i',
    .init = BTT_DEFAULT_NUM_TEMPO_CANDIDATES,
    .increment = 1,
    .name = "btt_set_num_tempo_candidates",
  },
  {
    .set = (funct)btt_set_gaussian_tempo_histogram_decay,
    .get = (funct)btt_get_gaussian_tempo_histogram_decay,
    .type = 'd',
    .init = BTT_DEFAULT_GAUSSIAN_TEMPO_HISTOGRAM_DECAY,
    .increment = 0.00001,
    .name = "btt_set_gaussian_tempo_histogram_decay",
  },
  {
    .set = (funct)btt_set_gaussian_tempo_histogram_width,
    .get = (funct)btt_get_gaussian_tempo_histogram_width,
    .type = 'd',
    .init = BTT_DEFAULT_GAUSSIAN_TEMPO_HISTOGRAM_WIDTH,
    .increment = 0.5,
    .name = "btt_set_gaussian_tempo_histogram_width",
  },
  {
    .set = (funct)btt_set_log_gaussian_tempo_weight_mean,
    .get = (funct)btt_get_log_gaussian_tempo_weight_mean,
    .type = 'd',
    .init = BTT_DEFAULT_LOG_GAUSSIAN_TEMPO_WEIGHT_MEAN,
    .increment = 4,
    .name = "btt_set_log_gaussian_tempo_weight_mean",
  },
  {
    .set = (funct)btt_set_log_gaussian_tempo_weight_width,
    .get = (funct)btt_get_log_gaussian_tempo_weight_width,
    .type = 'd',
    .init = BTT_DEFAULT_LOG_GAUSSIAN_TEMPO_WEIGHT_WIDTH,
    .increment = 4,
    .name = "btt_set_log_gaussian_tempo_weight_width",
  },
  {
    .set = (funct)btt_set_cbss_alpha,
    .get = (funct)btt_get_cbss_alpha,
    .type = 'd',
    .init = BTT_DEFAULT_CBSS_ALPHA,
    .increment = 0.01,
    .name = "btt_set_cbss_alpha",
  },
  {
    .set = (funct)btt_set_cbss_eta,
    .get = (funct)btt_get_cbss_eta,
    .type = 'd',
    .init = BTT_DEFAULT_CBSS_ETA,
    .increment = 5,
    .name = "btt_set_cbss_eta",
  },
  {
    .set = (funct)btt_set_beat_prediction_adjustment,
    .get = (funct)btt_get_beat_prediction_adjustment,
    .type = 'i',
    .init = BTT_DEFAULT_BEAT_PREDICTION_ADJUSTMENT,
    .increment = 1,
    .name = "btt_set_beat_prediction_adjustment",
  },
  {
    .set = (funct)btt_set_predicted_beat_trigger_index,
    .get = (funct)btt_get_predicted_beat_trigger_index,
    .type = 'i',
    .init = BTT_DEFAULT_PREDICTED_BEAT_TRIGGER_INDEX,
    .increment = 1,
    .name = "btt_set_predicted_beat_trigger_index",
  },
  {
    .set = (funct)btt_set_predicted_beat_gaussian_width,
    .get = (funct)btt_get_predicted_beat_gaussian_width,
    .type = 'd',
    .init = BTT_DEFAULT_PREDICTED_BEAT_GAUSSIAN_WIDTH,
    .increment = 1,
    .name = "btt_set_predicted_beat_gaussian_width",
  },
  {
    .set = (funct)btt_set_ignore_spurious_beats_duration,
    .get = (funct)btt_get_ignore_spurious_beats_duration,
    .type = 'd',
    .init = BTT_DEFAULT_IGNORE_SPURIOUS_BEATS_DURATION,
    .increment = 5,
    .name = "btt_set_ignore_spurious_beats_duration",
  },
};

int num_params = 19; //sizeof(params) / sizeof(params[0]);

/*--------------------------------------------------------------------*/
int main(void)
{
  i_hate_canonical_input_processing();
  
  fprintf(stderr, "'q' to quit\r\n'<' or '>' to scroll through paramaters\r\n'+' or '-' to change the parameter values\r\n");
  
  int param_index = 0;
  double increment;
  double val;
  char c;
  
  Microphone* mic = mic_new();
  if(mic == NULL) {perror("Unable to create microphone object"); exit(-1);}
  
  BTT* btt = mic_get_btt(mic);
  
  auPlay((Audio*)mic);
  
  for(;;)
    {
      c = getchar();
      increment = 0;
      param_t p = params[param_index];
      switch(c)
        {
          case '.': /* cascade */
          case '>':
            ++param_index; param_index %= num_params;
            p = params[param_index];
            break;
          
          case ',': /* cascade */
          case '<':
            --param_index;
            if(param_index < 0) param_index = num_params - 1;
            p = params[param_index];
            break;

          case '-': /* cascade */
          case '_':
            increment = -p.increment;
            break;
          
          case '=': /* cascade */
          case '+':
            increment = p.increment;
            break;

          case 'd': /* cascade */
          case 'D':
            if(p.type == 'i')
              ((int_setter) p.set)(btt, p.init);
            else if(p.type == 'd')
              ((double_setter) p.set)(btt, p.init);
            break;

          case 'q': /* cascade */
          case 'Q':
            goto out;
            break;
          
          default: break;
        }

      if(p.type == 'i')
        {
          val = ((int_getter) p.get)(btt);
          ((int_setter) p.set)(btt, val + increment);
          val = ((int_getter) p.get)(btt);
        }
      else if(p.type == 'd')
        {
           val = ((double_getter) p.get)(btt);
          ((double_setter) p.set)(btt, val + increment);
          val = ((double_getter) p.get)(btt);
        }
      fprintf(stderr, " %s(btt, %lf);                       \r", p.name, val);
    }
  
 out:
  make_stdin_cannonical_again();
}


/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
struct termios old_terminal_attributes;

void i_hate_canonical_input_processing(void)
{
  int error;
  struct termios new_terminal_attributes;
  
  int fd = fcntl(STDIN_FILENO,  F_DUPFD, 0);
  
  error = tcgetattr(fd, &(old_terminal_attributes));
  if(error == -1) {  fprintf(stderr, "Error getting serial terminal attributes\r\n"); return;}
  
  new_terminal_attributes = old_terminal_attributes;
  
  cfmakeraw(&new_terminal_attributes);
  
  error = tcsetattr(fd, TCSANOW, &new_terminal_attributes);
  if(error == -1) {  fprintf(stderr,  "Error setting serial attributes\r\n"); return; }
}

/*--------------------------------------------------------------------*/
void make_stdin_cannonical_again()
{
  int fd = fcntl(STDIN_FILENO,  F_DUPFD, 0);
  
  if (tcsetattr(fd, TCSANOW, &old_terminal_attributes) == -1)
    fprintf(stderr,  "Error setting serial attributes\r\n");
}

