/*__--------------_-----------_---------------------__---------------------
 / _|___ _  _ _ _(_)___ _ _  | |_ _ _ __ _ _ _  ___/ _|___ _ _ _ __  
|  _/ _ \ || | '_| / -_) '_| |  _| '_/ _` | ' \(_-<  _/ _ \ '_| '  \ 
|_| \___/\_,_|_| |_\___|_|    \__|_| \__,_|_||_/__/_| \___/_| |_|_|_|
-------------------------------------------------------------------------*/
//MODIFIED AUGUST 29 2018 TO FIX ERRORS IN REAL FORWARD AND INVERSE TRANSFORMS!
//MODIFIED MARCH  25 2023 TO INCLUDE IN PLACE REAL TRANSFORMS

#include "DFT.h"
#include "fastsin.h"

#include <math.h>
#include <float.h>

/*-----------------------------------------------------------------------*/
//produces output in bit-reversed order (Decimation in Frequency)
void dft_raw_forward_dft(dft_sample_t* real, dft_sample_t* imag, int N)
{
  fastsin_t two_pi_over_N = (fastsin_t)(SIN_TWO_PI / N), omega_two_pi_over_n;
  int sub_transform, butterfly;
  int num_sub_transforms = 1, num_butterflies = N/2, omega;
  dft_sample_t wr, wi, *r=real, *i=imag, tempr, tempi;
  int top_index = 0, bottom_index;
  
  while(num_sub_transforms < N)
    {
      top_index = 0;
      for(sub_transform=0; sub_transform<num_sub_transforms; sub_transform++)
        {
          omega = 0;
          for(butterfly=0; butterfly<num_butterflies; butterfly++)
            {
              bottom_index        = num_butterflies + top_index;
              omega_two_pi_over_n = omega * two_pi_over_N;
              wr                  =  fastcos(omega_two_pi_over_n);
              wi                  = -fastsin(omega_two_pi_over_n);
              tempr               = r[top_index];
              tempi               = i[top_index];
              r[top_index]       += r[bottom_index];
              i[top_index]       += i[bottom_index];
              r[bottom_index]     = tempr - r[bottom_index];
              i[bottom_index]     = tempi - i[bottom_index];
              tempr = (r[bottom_index] * wr) - (i[bottom_index] * wi);
              tempi = (r[bottom_index] * wi) + (i[bottom_index] * wr);
              r[bottom_index]     = tempr;
              i[bottom_index]     = tempi;
              omega              += num_sub_transforms;
              top_index++;
            }
          top_index += num_butterflies;
        }
      num_sub_transforms <<= 1;
      num_butterflies    >>= 1;
    }
}

/*-----------------------------------------------------------------------*/
//takes input in bit-reversed order (Decimation in Time)
void dft_raw_inverse_dft(dft_sample_t* real, dft_sample_t* imag, int N)
{
  fastsin_t two_pi_over_N = (fastsin_t)(SIN_TWO_PI / N), omega_two_pi_over_n;
  int sub_transform, butterfly;
  int num_sub_transforms = N, num_butterflies = 1, omega;
  dft_sample_t wr, wi, *r=real, *i=imag, tempr, tempi;
  int top_index = 0, bottom_index;
  
  while((num_sub_transforms >>= 1) > 0)
    {
      top_index = 0;
      for(sub_transform=0; sub_transform<num_sub_transforms; sub_transform++)
        {
          omega = 0;
          for(butterfly=0; butterfly<num_butterflies; butterfly++)
            {
              bottom_index        = num_butterflies + top_index;
              omega_two_pi_over_n = omega * two_pi_over_N;
              wr                  = fastcos(omega_two_pi_over_n);
              wi                  = fastsin(omega_two_pi_over_n);
              tempr = (r[bottom_index] * wr) - (i[bottom_index] * wi);
              tempi = (r[bottom_index] * wi) + (i[bottom_index] * wr);
              wr                  = r[top_index]; 
              wi                  = i[top_index];
              r[top_index]       += tempr;
              i[top_index]       += tempi;
              r[bottom_index]     = wr - tempr;
              i[bottom_index]     = wi - tempi;
              omega              += num_sub_transforms;
              top_index++;
            }
          top_index += num_butterflies;
        }
      num_butterflies <<= 1;
    }
 
  tempr = 1.0/N;
  for(omega=0; omega<N; omega++)
    {
      r[omega] *= tempr;
      i[omega] *= tempr;
    }
}

/*-----------------------------------------------------------------------*/
void   dft_complex_forward_dft (dft_sample_t* real,   dft_sample_t* imag,   int N)
{
  //normal order in, normal order out
  dft_raw_forward_dft(real, imag, N);
  dft_bit_reverse_indices(real, imag, N);
}

/*-----------------------------------------------------------------------*/
void   dft_complex_inverse_dft (dft_sample_t* real,   dft_sample_t* imag,   int N)
{
  //normal order in, normal order out
  dft_bit_reverse_indices(real, imag, N);
  dft_raw_inverse_dft(real, imag, N);
}

/*-----------------------------------------------------------------------*/
void dft_real_forward_dft(dft_sample_t* real, dft_sample_t* imag, int N)
{
  //takes and returns normal order. real input, complex output
  //i.e. give me real_1 and I'll give back real and imag
  //computes positive and negative frequencies
  
  int i;
  int N_over_2 = N >> 1;
  int two_i;
  fastsin_t pi_over_N = SIN_PI / N_over_2;
  fastsin_t i_pi_over_N;
  float ar, ai, br, bi;
  dft_sample_t r1, i1;
  
  for(i=0; i<N_over_2; i++)
    {
      two_i = i << 1;
      real[i] = real[two_i];
      imag[i] = real[two_i + 1];
    }
  
  dft_raw_forward_dft(real, imag, N_over_2);
  dft_bit_reverse_indices(real, imag, N_over_2);
  
  for(i=1; i<N_over_2; i++)
    {
      i_pi_over_N = i * pi_over_N;
      ar = 0.5 * (1-fastsin(i_pi_over_N));
      ai = 0.5 * ( -fastcos(i_pi_over_N));
      br = 0.5 * (1+fastsin(i_pi_over_N));
      bi = 0.5 * (  fastcos(i_pi_over_N));
      r1 = real[i] * ar - imag[i] * ai + real[N_over_2-i] * br + imag[N_over_2-i] * bi;
      i1 = imag[i] * ar + real[i] * ai + real[N_over_2-i] * bi - imag[N_over_2-i] * br;
      real[N-i] = r1;
      imag[N-i] = -i1;
    }

  real[N_over_2] = real[0] - imag[0];
  imag[N_over_2] = 0;
  ai = -0.5;
  ar = 0.5;
  bi = 0.5;
  br = 0.5;
  r1 = real[0] * ar - imag[0] * ai + real[0] * br + imag[0] * bi;
  i1 = imag[0] * ar + real[0] * ai + real[0] * bi - imag[0] * br;
  real[0] = r1;
  imag[0] = i1;

  for(i=1; i<N_over_2; i++)
    {
      real[i] = real[N-i];
      imag[i] = -imag[N-i];
    }
}

/*-----------------------------------------------------------------------*/
void dft_real_inverse_dft(dft_sample_t* real, dft_sample_t* imag, int N)
{
  //takes and returns normal order. complex input, real output
  //i.e. give me real and imag and I'll give back real
  //top halves of real and imag will be ignored (assumed to be conjugates of bottom halves)
  
  int i;
  int N_over_2 = N >> 1;
  int two_i;
  int i_plus_N_over_2;
  fastsin_t pi_over_N = SIN_PI / N_over_2;
  fastsin_t i_pi_over_N;
  float ar, ai, br, bi;
  dft_sample_t r1, i1;
  
  
  for(i=0; i<N_over_2; i++)
    {
      i_pi_over_N = i * pi_over_N;
      ar = 0.5 * (1-fastsin(i_pi_over_N));
      ai = 0.5 * (  fastcos(i_pi_over_N));
      br = 0.5 * (1+fastsin(i_pi_over_N));
      bi = 0.5 * (  -fastcos(i_pi_over_N));
      
      i_plus_N_over_2 = i + N_over_2;
      
      //put it in the top half of the buffers to conserve space
      r1 = real[i] * ar - imag[i] * ai + real[N_over_2-i] * br + imag[N_over_2-i] * bi;
      i1 = imag[i] * ar + real[i] * ai + real[N_over_2-i] * bi - imag[N_over_2-i] * br;
      real[i_plus_N_over_2] = r1;
      imag[i_plus_N_over_2] = i1;
    }

  dft_bit_reverse_indices(real+N_over_2, imag+N_over_2, N_over_2);
  dft_raw_inverse_dft    (real+N_over_2, imag+N_over_2, N_over_2);
      
  for(i=0; i<N_over_2; i++)
    {
      i_plus_N_over_2 = i + N_over_2;
      two_i = i << 1;
      real[two_i]   = real[i_plus_N_over_2];
      real[two_i+1] = imag[i_plus_N_over_2];
      imag[i] = 0;
      imag[i_plus_N_over_2] = 0;
    }
}

/*-----------------------------------------------------------------------*/
void dft_2_real_forward_dfts(dft_sample_t* real_1, dft_sample_t* real_2, dft_sample_t* imag_1, dft_sample_t* imag_2, int N)
{
  //takes and returns normal order. real input, complex output
  //i.e. give me real_1 and real_2 and I'll give back real_1, real_2, imag_1 and imag_2
  
  int i;
  int N_over_2 = N >> 1;
  
  dft_sample_t *real = real_1, *imag = real_2;
  dft_sample_t r1, i1, r2, i2;
  
  dft_raw_forward_dft(real, imag, N);
  dft_bit_reverse_indices(real, imag, N);

  *imag_1 = 0;
  *imag_2 = 0;
  real_1[N_over_2] = real[N_over_2];
  imag_1[N_over_2] = 0;
  real_2[N_over_2] = imag[N_over_2];
  imag_2[N_over_2] = 0;
  
  for(i=1; i<N_over_2; i++)
    {
      r1 = 0.5 * (real[i] + real[N-i]);
      i1 = 0.5 * (imag[i] - imag[N-i]);
      r2 = 0.5 * (imag[i] + imag[N-i]);
      i2 = -0.5 * (real[i] - real[N-i]);
      real_1[i] = r1;
      imag_1[i] = i1;
      real_2[i] = r2;
      imag_2[i] = i2;
      real_1[N-i] = r1;
      imag_1[N-i] = -i1;
      real_2[N-i] = r2;
      imag_2[N-i] = -i2;
    }
}

/*-----------------------------------------------------------------------*/
void dft_2_real_inverse_dfts(dft_sample_t* real_1, dft_sample_t* real_2, dft_sample_t* imag_1, dft_sample_t* imag_2, int N)
{
  //takes and returns normal order. complex input, real output;
  //i.e. give me real_1, real_2, imag_1 and imag_2 real_1 and real_2 and I'll give back real_1 and real_2
  //top halves of real and imag must contain correct values (cojugate of lower halves)
  
  int i;
  dft_sample_t *real = real_1, *imag = real_2;
  dft_sample_t temp;
  
  for(i=0; i<N; i++)
    {
      temp    = *real_1 - *imag_2;
      *real_2 = *imag_1 + *real_2;
      *real_1 = temp;
      ++real_1; ++real_2;
      ++imag_1; ++imag_2;
    }
  dft_bit_reverse_indices(real, imag, N);
  dft_raw_inverse_dft    (real, imag, N);
}

/*-----------------------------------------------------------------------*/
void   rdft_real_forward_dft     (dft_sample_t* real,  int N)
{
  int i, j, k;
  int n1, n2, n4;
  int i1, i2, i3, i4;
  int m=1; while(1<<m < N) m++;
  
  fastsin_t A, two_pi_over_N1;
  dft_sample_t temp, cc, ss, t1, t2;
  
  rdft_bit_reverse_indices(real, N);
  
  //length 2 butterfiles
  for(i=0; i<N; i+=2)
    {
      temp      = real[i];
      real[i]   = temp + real[i+1];
      real[i+1] = temp - real[i+1];
    }
    
  //other butterflies
  n2 = 1;
  for(k=1; k<m; k++)
    {
      n4 = n2;
      n2 <<= 1;
      n1 = n2 << 1;
      two_pi_over_N1 = SIN_PI / n2;
      
      for(i=0; i<N; i+=n1)
        {
          temp = real[i];
          real[i]   = temp + real[i+n2];
          real[i+n2] = temp - real[i+n2];
          real[i + n2 + n4] = -real[i + n2 + n4];
          A = two_pi_over_N1;
          
          for(j=1; j<n4; j++)
            {
              i1 = i+j;
              i2 = i-j+n2;
              i3 = i+j+n2;
              i4 = i-j+n1;
              cc = fastcos(A);
              ss = fastsin(A);
              A += two_pi_over_N1;
              t1 = real[i3] * cc + real[i4] * ss;
              t2 = real[i3] * ss - real[i4] * cc;
              real[i4] =  real[i2] - t2;
              real[i3] = -real[i2] - t2;
              real[i2] =  real[i1] - t1;
              real[i1] =  real[i1] + t1;
            }
        }
    }
}

/*-----------------------------------------------------------------------*/
void   rdft_real_inverse_dft     (dft_sample_t* real,  int N)
{
  int i, j, k;
  int n2, n4, n8;
  int is, id, i1, i2, i3, i4, i5, i6, i7, i8;
  
  fastsin_t two_pi_over_N2, A, A3;
  dft_sample_t t1, t2, t3, t4, t5;
  dft_sample_t cc1, cc3, ss1, ss3;
  dft_sample_t one_over_root_2 = fastsin(SIN_PI >> 2);
  
  int m=1; while(1<<m < N) m++;

  //L shaped butterflies
  n2 = N << 1;
  for(k=1; k<m; k++)
    {
      is = 0;
      id = n2;
      n2 >>= 1;
      n4 = n2 >> 2;
      n8 = n4 >> 1;
      two_pi_over_N2 = (SIN_PI / n2) << 1;

      while(is < N-1)
        {
          for(i=is; i<N; i+=id)
            {
              i1 = i;
              i2 = i1 + n4;
              i3 = i2 + n4;
              i4 = i3 + n4;
              
              t1 = real[i1] - real[i3];
              real[i1] =  real[i1] + real[i3];
              real[i2] *= 2;
              real[i3] = t1 - 2*real[i4];
              real[i4] = t1 + 2*real[i4];
              if(n4 == 1)
                continue;
              i1 += n8;
              i2 += n8;
              i3 += n8;
              i4 += n8;
              t1 = (real[i2] - real[i1]) * one_over_root_2;
              t2 = (real[i4] + real[i3]) * one_over_root_2;
              real[i1] += real[i2];
              real[i2] = real[i4] - real[i3];
              real[i3] = 2 * ((-t2) - t1);
              real[i4] = 2 * ((-t2) + t1);
            }
          is = 2 * id - n2;
          id <<= 2;
        }
      A = two_pi_over_N2;
        
      for(j=2; j<=n8; j++)
        {
          A3 = 3 * A;
          cc1 = fastcos(A);
          ss1 = fastsin(A);
          cc3 = fastcos(A3);
          ss3 = fastsin(A3);
          A = j * two_pi_over_N2;
          is = 0;
          id = n2 << 1;
  
          while(is < N-1)
            {
              for(i=is; i<N; i+=id)
                {
                  i1 = i+j - 1;
                  i2 = i1 + n4;
                  i3 = i2 + n4;
                  i4 = i3 + n4;
                  i5 = i + n4 - j + 1;
                  i6 = i5 + n4;
                  i7 = i6 + n4;
                  i8 = i7 + n4;
                  t1 = real[i1] - real[i6];
                  real[i1] += real[i6];
                  t2 = real[i5] - real[i2];
                  real[i5] += real[i2];
                  t3 = real[i8] + real[i3];
                  real[i6] = real[i8] - real[i3];
                  t4 = real[i4] + real[i7];
                  real[i2] = real[i4] - real[i7];
                  t5 = t1 - t4;
                  t1 += t4;
                  t4 = t2 - t3;
                  t2 += t3;
                  real[i3] =  t5*cc1 + t4*ss1;
                  real[i7] =  -t4*cc1 + t5*ss1;
                  real[i4] = t1*cc3 - t2*ss3;
                  real[i8] = t2*cc3 + t1*ss3;
                }
              is = 2 * id - n2;
              id *= 4;
            }
        }
    }
  
  //length 2 butterflies
  is = 1;
  id = 4;
  while(is < N)
    {
      for(i=is-1; i<N; i+=id)
        {
          i1 = i + 1;
          t1 = real[i];
          real[i] = t1 + real[i1];
          real[i1] = t1 - real[i1];
        }
      is = 2 * id - 1;
      id *= 4;
    }
  
  rdft_bit_reverse_indices(real, N);
  
  for(i=0; i<N; i++)
    real[i] /= (dft_sample_t)N;
}

/*-----------------------------------------------------------------------*/
void rdft_2_real_forward_dfts(dft_sample_t* real_1, dft_sample_t* real_2, int N)
{
  int i;
  int N_over_2 = N >> 1;
  
  dft_sample_t *real = real_1, *imag = real_2;
  dft_sample_t r1, i1, r2, i2;
  
  dft_raw_forward_dft(real, imag, N);
  dft_bit_reverse_indices(real, imag, N);

  real_1[0] = real[0];
  real_2[0] = imag[0];
  real_1[N_over_2] = real[N_over_2];
  real_2[N_over_2] = imag[N_over_2];
  
  for(i=1; i<N_over_2; i++)
    {
      r1 = 0.5 * (real[i] + real[N-i]);
      i1 = 0.5 * (imag[i] - imag[N-i]);
      r2 = 0.5 * (imag[i] + imag[N-i]);
      i2 = -0.5 * (real[i] - real[N-i]);
      real_1[i]   = r1;
      real_1[N-i] = i1;
      real_2[i]   = r2;
      real_2[N-i] = i2;
    }
}

/*-----------------------------------------------------------------------*/
void rdft_2_real_inverse_dfts(dft_sample_t* real_1, dft_sample_t* real_2, int N)
{
  int i;
  int N_over_2 = N >> 1;
  dft_sample_t *real = real_1, *imag = real_2;
  dft_sample_t r1, i1, r2, i2;
  
  //already correct
  /*
  real[0] = real_1[0];
  imag[0] = real_2[0];
  real[N_over_2] = real_1[N_over_2];
  imag[N_over_2] = real_2[N_over_2];
  */
  
  for(i=1; i<N_over_2; i++)
    {
      r1 = real_1[i]    - real_2[N-i];
      i1 = real_1[N-i]  + real_2[i];
      r2 = real_1[i]    + real_2[N-i];
      i2 = -real_1[N-i] + real_2[i];
      
      real[i] = r1;
      imag[i] = i1;
      real[N-i] = r2;
      imag[N-i] = i2;
    }

  dft_bit_reverse_indices(real, imag, N);
  dft_raw_inverse_dft    (real, imag, N);
}

/*-----------------------------------------------------------------------*/
void   rdft_rect_to_polar       (dft_sample_t* real,  int N)
{
  dft_sample_t temp;
  dft_sample_t* imag = real + N - 1;
  ++real;
  N >>= 1;
  while(N-- > 1)
    {
      temp = *real;
      *real = hypot(*real, *imag);
      *imag = atan2(*imag, temp);
      ++real, --imag;
    }
}

/*-----------------------------------------------------------------------*/
void   rdft_polar_to_rect       (dft_sample_t* real,  int N)
{
  dft_sample_t temp;
  dft_sample_t* imag = real + N - 1;
  ++real;
  N >>= 1;

  while(N-- > 1)
    {
      temp = *real;
      *real = *real * cos(*imag);
      *imag = temp  * sin(*imag);
      ++real, --imag;
    }
}

/*-----------------------------------------------------------------------*/
void   dft_real_convolve       (dft_sample_t* real_1, dft_sample_t* real_2, dft_sample_t* imag_1, dft_sample_t* imag_2, int N)
{
  int i;
  int n_over_2 = N >> 1;
  dft_sample_t re, im;
  
  dft_2_real_forward_dfts(real_1, real_2, imag_1, imag_2, N);
  
  for(i=0; i<n_over_2; i++)
    {
      re = real_1[i]*real_2[i] - imag_1[i]*imag_2[i];
      im = real_1[i]*imag_2[i] + real_2[i]*imag_1[i];
      real_1[i] = re;
      imag_1[i] = im;
    }
  
  dft_real_inverse_dft(real_1, imag_1, N);
}

/*-----------------------------------------------------------------------*/
void   dft_real_correlate      (dft_sample_t* real_1, dft_sample_t* real_2, dft_sample_t* imag_1, dft_sample_t* imag_2, int N)
{
  int i;
  int n_over_2 = N >> 1;
  dft_sample_t re, im;
  
  dft_2_real_forward_dfts(real_1, real_2, imag_1, imag_2, N);
  
  for(i=0; i<n_over_2; i++)
    {
      re = real_1[i]*real_2[i] + imag_1[i]*imag_2[i];
      im = real_2[i]*imag_1[i] - real_1[i]*imag_2[i];
      real_1[i] = re;
      imag_1[i] = im;
    }
  
  dft_real_inverse_dft(real_1, imag_1, N);
}

/*-----------------------------------------------------------------------*/
void   dft_real_autocorrelate  (dft_sample_t* real, dft_sample_t* imag, int N)
{
  int i;
  int n_over_2 = N >> 1;
  dft_sample_t re, im;
  
  dft_real_forward_dft(real, imag, N);
  
  for(i=0; i<n_over_2; i++)
    {
      re = real[i]*real[i] + imag[i]*imag[i];
      im = real[i]*imag[i] - real[i]*imag[i];
      real[i] = re;
      imag[i] = im;
    }
  
  dft_real_inverse_dft(real, imag, N);
}

/*-----------------------------------------------------------------------*/
void   dft_real_generalized_autocorrelation  (dft_sample_t* real  , dft_sample_t* imag, int N, double exponent)
{
  int i;
  int n_over_2 = N >> 1;
  
  dft_real_forward_dft(real, imag, N);
  dft_rect_to_polar(real, imag, n_over_2);
  
  for(i=0; i<n_over_2; i++)
    {
      real[i] = pow(real[i], exponent);
      imag[i] = 0;
    }
  
  dft_real_inverse_dft(real, imag, N);
}

/*-----------------------------------------------------------------------*/
void   rdft_real_generalized_autocorrelation  (dft_sample_t* real, int N, double exponent)
{
  int i;
  int n_over_2 = N >> 1;
  
  rdft_real_forward_dft(real, N);
  rdft_rect_to_polar(real, N);
  
  real[0] = pow(real[0], exponent);
  for(i=1; i<n_over_2; i++)
    {
      real[i] = pow(real[i], exponent);
      real[N-i-1] = 0;
    }
  real[n_over_2] = pow(real[n_over_2], exponent);
  
  //it seems like polar to rect should be performed here but the Percival Tzanetakis paper says not in Eq 3.
  //further testing should probably be done
  
  rdft_real_inverse_dft(real, N);
}


/*-----------------------------------------------------------------------*/
void rdft_bit_reverse_indices(dft_sample_t* real, int N)
{
  dft_sample_t temp;
  
  int N_over_2 = N >> 1;
  unsigned n, bit, rev;
  unsigned n_reversed = N_over_2;
  
  for(n=1; n<N; n++)
    {
      if(n_reversed > n)
        {
          temp            = real[n]        ;
          real[n]         = real[n_reversed];
          real[n_reversed] = temp           ;
        }
      bit = ~n & (n + 1);
      rev = N_over_2 / bit;
      n_reversed ^= (N - 1) & ~(rev - 1);
    }
}

/*-----------------------------------------------------------------------*/
void dft_bit_reverse_indices(dft_sample_t* real, dft_sample_t* imag, int N)
{
  dft_sample_t temp;
  int N_over_2 = N >> 1;
  unsigned n, bit, rev;
  unsigned n_reversed = N_over_2;
  
  for(n=1; n<N; n++)
    {
      if(n_reversed > n)
        {
          temp            = real[n]         ;
          real[n]         = real[n_reversed];
          real[n_reversed] = temp           ;
          temp            = imag[n]         ;
          imag[n]         = imag[n_reversed];
          imag[n_reversed] = temp           ;
        }
      bit = ~n & (n + 1);
      rev = N_over_2 / bit;
      n_reversed ^= (N - 1) & ~(rev - 1);
    }
}

/*-----------------------------------------------------------------------*/
void dft_rect_to_polar(dft_sample_t* real, dft_sample_t* imag, int N)
{
  dft_sample_t temp;

  while(N-- > 0)
    {
      temp = *real;
      *real = hypot(*real, *imag);
      *imag = atan2(*imag, temp);
      ++real, ++imag;    
    }
}

/*-----------------------------------------------------------------------*/
void dft_polar_to_rect(dft_sample_t* real, dft_sample_t* imag, int N)
{
  dft_sample_t temp;

  while(N-- > 0)
    {
      temp = *real;
      *real = *real * cos(*imag);
      *imag = temp  * sin(*imag);
      ++real, ++imag;
    }
}

/*-----------------------------------------------------------------------*/
void   dft_magnitude_to_db     (dft_sample_t* real,   int N)
{
  int i;
  
  for(i=0; i<N; i++)
    {
      if(*real == 0)
        *real = FLT_MIN;
      else
        *real = 20 * log10(*real);
      ++real;
    }
}

/*-----------------------------------------------------------------------*/
void   dft_normalize_magnitude (dft_sample_t* real,   int N)
{
  dft_sample_t max = 0;
  int i;
  dft_sample_t* r = real;
  
  for(i=0; i<N; i++)
    {
      if(*r > max)
        max = *r;
      ++r;
    }
  
  r = real;
  if(max > 0)
    {
      max = 1 / max;
      for(i=0; i<N; i++)
        *r++ *= max;
    }
}

/*-----------------------------------------------------------------------*/
double dft_frequency_of_bin    (double bin, double sample_rate, int N)
{
  return bin * (sample_rate / N);
}

/*-----------------------------------------------------------------------*/
double dft_bin_of_frequency    (double hz, double sample_rate, int N)
{
  return hz * ((double)N / (double)sample_rate);
}

/*-----------------------------------------------------------------------*/
int dft_smallest_power_of_2_at_least_as_great_as(int n)
{
  int result = 1;
  while(result < n)
    result <<= 1;
  return result;
}

/*-----------------------------------------------------------------------*/
void dft_init_blackman_window(dft_sample_t* window, int N)
{
  //could be faster using fastsin
  //for now this is only initialization
  //so it doesn't really matter
  int i;
  long double phase = 0;
  long double phase_increment = 2*M_PI / (long double)N;
  double a = 0;
  double a0 = (1-a)/2.0;
  double a1 = 1/2.0;
  double a2 = a/2.0;
  
  for(i=0; i<N; i++)
    {
      *window++ = a0 - a1*cos(phase) + a2*cos(2*phase);
      phase += phase_increment;
    }  
}

/*-----------------------------------------------------------------------*/
void dft_init_hann_window(dft_sample_t* window, int N)
{
  int i;
  long double phase = 0;
  long double phase_increment = 2*M_PI / (long double)N;
  for(i=0; i<N; i++)
    {
      *window++ = 0.5 * (1-cos(phase));
      phase += phase_increment;
    }  
}

/*-----------------------------------------------------------------------*/
void dft_init_hamming_window(dft_sample_t* window, int N)
{
  int i;
  long double phase = 0;
  long double phase_increment = 2*M_PI / (long double)N;
  for(i=0; i<N; i++)
    {
      *window++ = 0.54 - 0.46 * cos(phase);
      phase += phase_increment;
    }  
}

/*-----------------------------------------------------------------------*/
void   dft_init_half_sine_window(dft_sample_t* window, int N)
{
  int i;
  long double coeff = M_PI / (long double)N;
  for(i=0; i<N; i++)
    *window++ = sin(coeff * (i+0.5));
}

/*-----------------------------------------------------------------------*/
void dft_apply_window(dft_sample_t* real, dft_sample_t* window, int N)
{
  while(N-- > 0)
    *real++ *= *window++;
}
