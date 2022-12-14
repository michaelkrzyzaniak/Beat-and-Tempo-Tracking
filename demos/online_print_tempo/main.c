//OSX: gcc *.c ../../src/*.c -framework AudioToolbox
//Linux: gcc *.c ../../src/*.c -lasound -lm -lpthread -lrt

#include "Microphone.h"

/*--------------------------------------------------------------------*/
int main(void)
{
  Microphone* mic = mic_new();
  if(mic == NULL) {perror("Unable to create microphone object"); exit(-1);}
  
  BTT* btt = mic_get_btt(mic);
  btt_set_count_in_n(btt, 0);
  btt_set_tracking_mode(btt, BTT_ONSET_AND_TEMPO_TRACKING);
  
  auPlay((Audio*)mic);
  
  for(;;)
    {
      sleep(1);
      double tempo = btt_get_tempo_bpm(btt);
      fprintf(stderr, "%02f BPM\r\n", tempo);
    }
}
