/* FFdecsa -- fast decsa algorithm
 *
 * Copyright (C) 2003-2004  fatih89r
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#include <mmintrin.h>

struct group_t{
  __m64 s1;
};
typedef struct group_t group;

#define GROUP_PARALLELISM 64

group static inline FF0(){
  group res;
  res.s1=_mm_setzero_si64();
  return res;
}

group static inline FF1(){
  group res;
  __m64 zero = _mm_setzero_si64();
  res.s1=_mm_cmpeq_pi32(zero, zero);
  return res;
}

group static inline FFAND(const group& a, const group& b){
  group res;
  res.s1=_m_pand(a.s1,b.s1);
  return res;
}

group static inline FFOR(const group& a, const group& b){
  group res;
  res.s1=_m_por(a.s1,b.s1);
  return res;
}

group static inline FFXOR(const group& a, const group& b){
  group res;
  res.s1=_m_pxor(a.s1,b.s1);
  return res;
}

group static inline FFNOT(const group& a){
  group res;
  res.s1=_m_pxor(a.s1,FF1().s1);
  return res;
}


/* 64 rows of 64 bits */

void static inline FFTABLEIN(unsigned char *tab, int g, unsigned char *data){
#if 1
  *(((int *)tab)+2*g)=*((int *)data);
  *(((int *)tab)+2*g+1)=*(((int *)data)+1);
#else
  *(((long long int *)tab)+g)=*((long long int *)data);
#endif
}

void static inline FFTABLEOUT(unsigned char *data, unsigned char *tab, int g){
#if 1
  *((int *)data)=*(((int *)tab)+2*g);
  *(((int *)data)+1)=*(((int *)tab)+2*g+1);
#else
  *((long long int *)data)=*(((long long int *)tab)+g);
#endif
}

void static inline FFTABLEOUTXORNBY(int n, unsigned char *data, unsigned char *tab, int g){
  int j;
  for(j=0;j<n;j++){
    *(data+j)^=*(tab+8*g+j);
  }
}


struct batch_t{
  __m64 s1;
};
typedef struct batch_t batch;

#define BYTES_PER_BATCH 8

batch static inline B_FFAND(const batch& a, const batch& b){
  batch res;
  res.s1=_m_pand(a.s1,b.s1);
  return res;
}

batch static inline B_FFOR(const batch& a, const batch& b){
  batch res;
  res.s1=_m_por(a.s1,b.s1);
  return res;
}

batch static inline B_FFXOR(const batch& a, const batch& b){
  batch res;
  res.s1=_m_pxor(a.s1,b.s1);
  return res;
}

batch static inline B_FFN_ALL_29(){
  batch res;
  res.s1=_mm_set1_pi32(0x29292929);
  return res;
}
batch static inline B_FFN_ALL_02(){
  batch res;
  res.s1=_mm_set1_pi32(0x02020202);
  return res;
}
batch static inline B_FFN_ALL_04(){
  batch res;
  res.s1=_mm_set1_pi32(0x04040404);
  return res;
}
batch static inline B_FFN_ALL_10(){
  batch res;
  res.s1=_mm_set1_pi32(0x10101010);
  return res;
}
batch static inline B_FFN_ALL_40(){
  batch res;
  res.s1=_mm_set1_pi32(0x40404040);
  return res;
}
batch static inline B_FFN_ALL_80(){
  batch res;
  res.s1=_mm_set1_pi32(0x80808080);
  return res;
}

batch static inline B_FFSH8L(const batch& a, int n){
  batch res;
  res.s1=_m_psllqi(a.s1,n);
  return res;
}

batch static inline B_FFSH8R(const batch& a, int n){
  batch res;
  res.s1=_m_psrlqi(a.s1,n);
  return res;
}

void static inline M_EMPTY(void){
  _m_empty();
}


#undef XOR_8_BY
#define XOR_8_BY(d,s1,s2)    do{ __m64 *pd=(__m64 *)(d), *ps1=(__m64 *)(s1), *ps2=(__m64 *)(s2); \
                                 *pd = _m_pxor( *ps1 , *ps2 ); }while(0)

#undef XOREQ_8_BY
#define XOREQ_8_BY(d,s)      do{ __m64 *pd=(__m64 *)(d), *ps=(__m64 *)(s); \
                                 *pd = _m_pxor( *ps, *pd ); }while(0)

#undef COPY_8_BY
#define COPY_8_BY(d,s)       do{ __m64 *pd=(__m64 *)(d), *ps=(__m64 *)(s); \
                                 *pd =  *ps; }while(0)

#undef BEST_SPAN
#define BEST_SPAN            8

#undef XOR_BEST_BY
#define XOR_BEST_BY(d,s1,s2) do{ XOR_8_BY(d,s1,s2); }while(0);

#undef XOREQ_BEST_BY
#define XOREQ_BEST_BY(d,s)   do{ XOREQ_8_BY(d,s); }while(0);

#undef COPY_BEST_BY
#define COPY_BEST_BY(d,s)    do{ COPY_8_BY(d,s); }while(0);
