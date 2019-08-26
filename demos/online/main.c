#include "Microphone.h"

/*--------------------------------------------------------------------*/
int main(void)
{
  Microphone* mic = mic_new();
  if(mic == NULL) {perror("Unable to create miccrophone object"); exit(-1);}
  
  auPlay((Audio*)mic);
  for(;;)
    sleep(1);
}

