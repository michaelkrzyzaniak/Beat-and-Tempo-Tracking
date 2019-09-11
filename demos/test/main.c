#include "../../src/DFT.h"
#include "../../src/fastsin.h"

#include "fix_fft.c"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int  main(void)
{
  int buff_size = 64;
  dft_sample_t* real_buff = calloc(buff_size, sizeof(*real_buff));
  dft_sample_t* imag_buff = calloc(buff_size, sizeof(*imag_buff));
  
  short* fixed_buff = calloc(buff_size, sizeof(*fixed_buff));
  short* fixed_imag = calloc(buff_size, sizeof(*fixed_imag));
  
  int freq = 10;
  
  int i;
  for(i=0; i<buff_size; i++)
    //real_buff[i] = sin(freq * i * 2 * M_PI / buff_size);
    fixed_buff[i] = 0x7FFF * sin(freq * i * 2 * M_PI / buff_size);
  
  //dft_real_forward_dft(real_buff, imag_buff, buff_size);
  //dft_rect_to_polar(real_buff, imag_buff, buff_size);
  
  //dft_one_sided_forward_dft(real_buff, buff_size);
  //dft_rect_to_polar(real_buff, real_buff+(buff_size/2), (buff_size/2));
  
  //fix_fft(fixed_buff, fixed_imag, 6, 0);
  fix_fftr(fixed_buff, 6, 0);
  //fix_fftr(fixed_buff, 6, 1);
  
  for(i=0; i<(buff_size/2); i++)
    //fprintf(stderr, "%hi\t%hi\r\n", fixed_buff[i], fixed_imag[i]);
    fprintf(stderr, "%hi\t%hi\r\n", fixed_buff[i], fixed_buff[i+(buff_size/2)]);
}
