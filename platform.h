#ifndef FL_PLATFORM_H
#define FL_PLATFORM_H

#include "includes.h"
struct Buffer;
typedef int Bool;

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

Bool ReadTextFile(const char *filename, Buffer buff, size_t *outSize = NULL);

void AbsolutePath(char const *filename, char *buffer);

void* MemoryAlloc(size_t bytes);

void MemoryFree(void *mem, size_t bytes);


#endif

