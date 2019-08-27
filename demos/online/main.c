#include "Microphone.h"

void iHateCanonicalInputProcessingIReallyReallyDo(void);
void makeStdinCannonicalAgain();

typedef void   (*double_setter)(Obtain* self, double val);
typedef double (*double_getter)(Obtain* self);
typedef void   (*int_setter)   (Obtain* self, int val);
typedef int    (*int_getter)   (Obtain* self);
typedef void   (*funct)        (void);

typedef struct parameter_struct
{
  funct  set;
  funct  get;
  char   type;
  double init;
  double increment;
  char   name[100];
}param_t;

param_t params[] =
{
  {
    .set = (funct)obtain_set_use_amplitude_normalization,
    .get = (funct)obtain_get_use_amplitude_normalization,
    .type = 'i',
    .init = OBTAIN_DEFAULT_USE_AMP_NORMALIZATION,
    .increment = 1,
    .name = "USE_AMP_NORMALIZATION",
  },
  {
    .set = (funct)obtain_set_spectral_compression_gamma,
    .get = (funct)obtain_get_spectral_compression_gamma,
    .type = 'd',
    .init = OBTAIN_DEFAULT_SPECTRAL_COMPRESSION_GAMMA,
    .increment = 0.05,
    .name = "SPECTRAL_COMPRESSION_GAMMA",
  },
  {
    .set = (funct)obtain_set_oss_filter_cutoff,
    .get = (funct)obtain_get_oss_filter_cutoff,
    .type = 'd',
    .init = OBTAIN_DEFAULT_OSS_FILTER_CUTOFF,
    .increment = 0.5,
    .name = "OSS_FILTER_CUTOFF",
  },
  {
    .set = (funct)obtain_set_autocorrelation_exponent,
    .get = (funct)obtain_get_autocorrelation_exponent,
    .type = 'd',
    .init = OBTAIN_DEFAULT_CORRELATION_EXPONENT,
    .increment = 0.1,
    .name = "AUTOCORRELATION_EXPONENT",
  },
  {
    .set = (funct)obtain_set_min_tempo,
    .get = (funct)obtain_get_min_tempo,
    .type = 'd',
    .init = OBTAIN_DEFAULT_MIN_TEMPO,
    .increment = 4,
    .name = "MIN_TEMPO",
  },
  {
    .set = (funct)obtain_set_max_tempo,
    .get = (funct)obtain_get_max_tempo,
    .type = 'd',
    .init = OBTAIN_DEFAULT_MAX_TEMPO,
    .increment = 4,
    .name = "MAX_TEMPO",
  },
  {
    .set = (funct)obtain_set_onset_threshold,
    .get = (funct)obtain_get_onset_threshold,
    .type = 'd',
    .init = OBTAIN_DEFAULT_ONSET_TREHSHOLD,
    .increment = 0.1,
    .name = "ONSET_TREHSHOLD",
  },
  {
    .set = (funct)obtain_set_num_tempo_candidates,
    .get = (funct)obtain_get_num_tempo_candidates,
    .type = 'i',
    .init = OBTAIN_DEFAULT_NUM_TEMPO_CANDIDATES,
    .increment = 1,
    .name = "NUM_TEMPO_CANDIDATES",
  },
  {
    .set = (funct)obtain_set_noise_cancellation_threshold,
    .get = (funct)obtain_get_noise_cancellation_threshold,
    .type = 'd',
    .init = OBTAIN_DEFAULT_NOISE_CANCELLATION_THRESHOLD,
    .increment = 1,
    .name = "NOISE_CANCELLATION_THRESHOLD",
  },
  {
    .set = (funct)obtain_set_cbss_alpha,
    .get = (funct)obtain_get_cbss_alpha,
    .type = 'd',
    .init = OBTAIN_DEFAULT_CBSS_ALPHA,
    .increment = 0.001,
    .name = "CBSS_ALPHA",
  },
  {
    .set = (funct)obtain_set_cbss_eta,
    .get = (funct)obtain_get_cbss_eta,
    .type = 'd',
    .init = OBTAIN_DEFAULT_CBSS_ETA,
    .increment = 5,
    .name = "CBSS_ETA",
  },
  {
    .set = (funct)obtain_set_gaussian_tempo_histogram_decay,
    .get = (funct)obtain_get_gaussian_tempo_histogram_decay,
    .type = 'd',
    .init = OBTAIN_DEFAULT_GAUSSIAN_TEMPO_HISTOGRAM_DECAY,
    .increment = 0.00001,
    .name = "GAUSSIAN_TEMPO_HISTOGRAM_DECAY",
  },
  {
    .set = (funct)obtain_set_gaussian_tempo_histogram_width,
    .get = (funct)obtain_get_gaussian_tempo_histogram_width,
    .type = 'd',
    .init = OBTAIN_DEFAULT_GAUSSIAN_TEMPO_HISTOGRAM_WIDTH,
    .increment = 0.5,
    .name = "GAUSSIAN_TEMPO_HISTOGRAM_WIDTH",
  },
  {
    .set = (funct)obtain_set_log_gaussian_tempo_weight_mean,
    .get = (funct)obtain_get_log_gaussian_tempo_weight_mean,
    .type = 'd',
    .init = OBTAIN_DEFAULT_LOG_GAUSSIAN_TEMPO_WEIGHT_MEAN,
    .increment = 4,
    .name = "LOG_GAUSSIAN_TEMPO_WEIGHT_MEAN",
  },
  {
    .set = (funct)obtain_set_log_gaussian_tempo_weight_width,
    .get = (funct)obtain_get_log_gaussian_tempo_weight_width,
    .type = 'd',
    .init = OBTAIN_DEFAULT_LOG_GAUSSIAN_TEMPO_WEIGHT_WIDTH,
    .increment = 4,
    .name = "LOG_GAUSSIAN_TEMPO_WEIGHT_WIDTH",
  },
  {
    .set = (funct)obtain_set_beat_prediction_adjustment,
    .get = (funct)obtain_get_beat_prediction_adjustment,
    .type = 'i',
    .init = OBTAIN_DEFAULT_BEAT_PREDICTION_ADJUSTMENT,
    .increment = 1,
    .name = "BEAT_PREDICTION_ADJUSTMENT",
  },
  {
    .set = (funct)obtain_set_predicted_beat_trigger_index,
    .get = (funct)obtain_get_predicted_beat_trigger_index,
    .type = 'i',
    .init = OBTAIN_DEFAULT_PREDICTED_BEAT_TRIGGER_INDEX,
    .increment = 1,
    .name = "PREDICTED_BEAT_TRIGGER_INDEX",
  },
  {
    .set = (funct)obtain_set_predicted_beat_gaussian_width,
    .get = (funct)obtain_get_predicted_beat_gaussian_width,
    .type = 'd',
    .init = OBTAIN_DEFAULT_PREDICTED_BEAT_GAUSSIAN_WIDTH,
    .increment = 1,
    .name = "PREDICTED_BEAT_GAUSSIAN_WIDTH",
  },
};

int num_params = 18;//sizeof(params) / sizeof(params[0]);

/*--------------------------------------------------------------------*/
int main(void)
{
  iHateCanonicalInputProcessingIReallyReallyDo();
  
  int param_index = 0;
  double increment;
  double val;
  char c;
  
  Microphone* mic = mic_new();
  if(mic == NULL) {perror("Unable to create microphone object"); exit(-1);}
  
  Obtain* obtain = mic_get_obtain(mic);
  
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
              ((int_setter) p.set)(obtain, p.init);
            else if(p.type == 'd')
              ((double_setter) p.set)(obtain, p.init);
            break;

          case 'q': /* cascade */
          case 'Q':
            goto out;
            break;
          
          default: break;
        }

      if(p.type == 'i')
        {
          val = ((int_getter) p.get)(obtain);
          ((int_setter) p.set)(obtain, val + increment);
          val = ((int_getter) p.get)(obtain);
        }
      else if(p.type == 'd')
        {
           val = ((double_getter) p.get)(obtain);
          ((double_setter) p.set)(obtain, val + increment);
          val = ((double_getter) p.get)(obtain);
        }
      fprintf(stderr, "%s: %lf\r\n", p.name, val);
    }
  
 out:
  makeStdinCannonicalAgain();
}


/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
struct termios oldTerminalAttributes;

void iHateCanonicalInputProcessingIReallyReallyDo(void)
{
  int error;
  struct termios newTerminalAttributes;
  
  int fd = fcntl(STDIN_FILENO,  F_DUPFD, 0);
  
  error = tcgetattr(fd, &(oldTerminalAttributes));
  if(error == -1) {  fprintf(stderr, "Error getting serial terminal attributes\r\n"); return;}
  
  newTerminalAttributes = oldTerminalAttributes;
  
  cfmakeraw(&newTerminalAttributes);
  
  error = tcsetattr(fd, TCSANOW, &newTerminalAttributes);
  if(error == -1) {  fprintf(stderr,  "Error setting serial attributes\r\n"); return; }
}

/*--------------------------------------------------------------------*/
void makeStdinCannonicalAgain()
{
  int fd = fcntl(STDIN_FILENO,  F_DUPFD, 0);
  
  if (tcsetattr(fd, TCSANOW, &oldTerminalAttributes) == -1)
    fprintf(stderr,  "Error setting serial attributes\r\n");
}

