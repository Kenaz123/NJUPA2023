#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  if(s == NULL) return 0;
  size_t n = 0;
  while(s[n]!='\0'){
    ++n;
  }
  return n;
}

char *strcpy(char *dst, const char *src) {
  if(dst == NULL || src == NULL) return dst;
  char *res = dst;
 // while((*dst++=*src++)!='\0');
  do{
    *dst = *src;
    dst++;
    src++;
  }while(*src != '\0');
  return res;
}

char *strncpy(char *dst, const char *src, size_t n) {
  if(dst == NULL || src == NULL) return dst; 
  char *res = dst;
  while(*src!='\0'&& n!=0){
    *dst=*src;
    ++dst;
    ++src;
    --n;
  }
  while(n!=0){
    *dst='\0';
    ++dst;
    --n;
  }
  return res;
}

char *strcat(char *dst, const char *src) {
  if(dst == NULL || src == NULL) return dst;
  char *res = dst;
  while(*dst!='\0'){
    ++dst;
  }
  //while((*dst++=*src++)!='\0'); 
  do{
    *dst = *src;
    dst++;
    src++;
  }while(*src != '\0');
  return res;
}

int strcmp(const char *s1, const char *s2) {
  if(s1 == NULL || s2 == NULL) return 0;
  while(*s1!='\0' && *s2!='\0' && *s1==*s2){
    s1++;
    s2++;
  }
  return *s1 == *s2 ? 0 : (((unsigned char)*s1 < (unsigned char)*s2)? -1:1);
}

int strncmp(const char *s1, const char *s2, size_t n) {
  if(s1 == NULL || s2 == NULL) return 0;
  while(*s1!='\0' && *s2!='\0' && *s1==*s2 && n!=0){
    ++s1;
    ++s2;
    --n;
  }
  return *s1 == *s2 || n==0 ? 0 : (((unsigned char)*s1 < (unsigned char)*s2)? -1:1);
}

void *memset(void *s, int c, size_t n) {
  if(s == NULL) return s;
  unsigned char *src=s;
  while(n!=0){
    --n;
    *src=c;
    ++src;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  if(dst == NULL || src == NULL || n == 0 || dst == src) return dst;
  unsigned char *dest = dst;
  const unsigned char *source = src;
  if(dst<src){
    while(n != 0){
      --n;
      *dest = *source;
      ++dest;
      ++source;
    }
  } else{
    dest+=n;
    source+=n;
    while(n!=0){
      --n;
      --dest;
      --source;
      *dest=*source;
    }
  }
  return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
  if(out == NULL || in == NULL || n == 0 || out == in) return out;
  unsigned char *dest = out;
  const unsigned char *src = in;
  while(n!=0){
    *dest = *src;
    --n;
    ++dest;
    ++src;
  }
  return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  if(s1 == NULL || s2 == NULL) return 0;
  const unsigned char *src1 = s1;
  const unsigned char *src2 = s2;
  while(n!=0 && *src1==*src2 && *src1!='\0' && *src2!='\0'){
    --n;
    ++src1;
    ++src2;
  }
  return *src1 == *src2|| n==0 ? 0:((*src1<*src2)?-1:1);
}

#endif
