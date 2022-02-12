#ifndef _CF_H_
#define _CF_H_

#include <iconv.h>
#include <memory.h>

extern int code_convert(const char* from_charset, const char* to_charset, char* inbuf, size_t inlen,char* outbuf, size_t outlen);


extern int u2g(char* inbuf, size_t inlen, char* outbuf, size_t outlen);


extern int g2u(char* inbuf, size_t inlen, char* outbuf, size_t outlen);

#endif
