#ifndef FL_PLATFORM_LINUX
#define FL_PLATFORM_LINUX

#include "../includes.h"
#include "../fl_string.h"
#include "../memory.h"
//#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

///typedef struct dirent SysDir;
typedef struct stat SysStat;

Bool ReadTextFile(const char *filename, Buffer buff, size_t *outSize)
{
	SysStat fStat = {};
	
	int result = stat(filename, &fStat);
	if(result != -1)
	{
		assert(fStat.st_size+1 <= buff.size);
		int fd = open(filename, O_RDONLY);
		if(fd != -1)
		{
			result = read(fd, buff.ptr, fStat.st_size);
			close(fd);
			assert(result != -1);
			buff.ptr[fStat.st_size] = '\0';
			if(outSize)
				*outSize = fStat.st_size;
			return true;
		}
	}
	else
	{
		PrintErrno("Error getting file stat ");
	}
	return false;
}

InlineFunc
void AbsolutePath(char const *filename, char *buffer)
{
	realpath(filename, buffer);
}

void* MemoryAlloc(size_t byteSize)
{
	void *mem = mmap(NULL, byteSize, PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if(mem == MAP_FAILED)
	{
		fprintf(stderr, "mmap failed: %s\n", strerror(errno));
		mem = NULL;
	}
	return mem;
}

void MemoryFree(void *mem, size_t bytes)
{
	munmap(mem, bytes);
}

#endif