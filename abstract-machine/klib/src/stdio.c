#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

//static char buf[1024];
static void reverse(char *s,int len){
  char *start = s;
  char *end = s + len - 1;
  char tmp;
  while(start < end){
    tmp = *start;
    *start = *end;
    *end = tmp;
    start++;
    end--;
  }
  
}
static int transstoi(int n,char *s,int base){
  assert(base <= 16);
  int i = 0,sign = n,bit;
  if(sign < 0) n=-n;
  do{
    bit = n % base;
    if(bit >= 10) s[i++] = 'a'+ bit - 10;
    else s[i++] = '0' + bit; 
  }while((n/=base)>0);
  if(sign < 0) s[i++] = '-';
  s[i] = '\0';
  reverse(s,i);
  return i;

}
int printf(const char *fmt, ...) {
 // va_list ap;
 // int n;
  //va_start(ap,fmt);
  //n = vsprintf(buf,fmt,ap);
  //va_end(ap);
  //return n;
  return 0;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  return 0;
}

int sprintf(char *out, const char *fmt, ...) {
  va_list ap;
  va_start(ap,fmt);
  char *start = out;
  for(;*fmt != '\0';++fmt){
    if(*fmt != '%'){
      *out = *fmt;
      ++out;
    }
    else{
      switch(*(++fmt)){
        case '%':*out=*fmt;++out;break;
        case 'd':out+=transstoi(va_arg(ap,int),out,10);break;
        case 's':{
          char *s = va_arg(ap,char*);
          strcpy(out,s);
          out+=strlen(out);
          break;
        }
      }
    }
  }
  *out = '\0';
  va_end(ap);
  return out - start;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not Implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not Implemented");
}

#endif
