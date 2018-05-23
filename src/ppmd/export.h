#ifndef EXPORT_H_
#define EXPORT_H_

#include <stdio.h>

size_t getUnpackSize(const char *in, size_t len);
int decompress(char *out, size_t *outLen, const char *in, size_t *inLen);
int compress(char *out, size_t *outLen, const char *in, size_t inLen, int order, int dicmemMB);

#endif
