#ifndef FL_PLATFORM_WINDOWS_H
#define FL_PLATFORM_WINDOWS_H

#include <stdio.h>
#include "../includes.h"
#include "../fl_string.h"
#include <windows.h>
#include <fileapi.h>
#include <minwinbase.h>
#include <pathcch.h>
#include <memoryapi.h>

Bool ReadTextFile(const char *filename, Buffer buff, size_t *outSize)
{
	HANDLE hFile = CreateFileA	(filename,               // file to open
                       GENERIC_READ,          // open for reading
                       FILE_SHARE_READ,       // share for reading
                       NULL,                  // default security
                       OPEN_EXISTING,         // existing file only
                       FILE_ATTRIBUTE_NORMAL, // normal file
                       NULL);
					   
	if (hFile == INVALID_HANDLE_VALUE) 
	{
		fprintf(stderr, "Couldn't open file for read\n");
		CloseHandle(hFile);
		return false;
	}
	DWORD bytesRead = 0;
	if( FALSE == ReadFile(hFile, buff.ptr, (DWORD)buff.size-1, &bytesRead, 0) )
    {
        fprintf(stderr, "Couldn't read file\n");
        CloseHandle(hFile);
        return false;
    }
	buff.ptr[bytesRead] = '\0';
	
	if(outSize)
		*outSize = (size_t)bytesRead;
	CloseHandle(hFile);
	return bytesRead>0;
	
	
}

char *Win2UnixPath (char *filename)
{
	char *s = filename;
	while (*s) 
	{
		if (*s == '\\')
			*s = '/';
		++s;
	}
	return filename;
}

InlineFunc
void AbsolutePath(char const *filename, char *buffer)
{
	auto result = GetFullPathNameA(filename, PATH_MAX, buffer, NULL);
	if(result)
		Win2UnixPath(buffer);
}

void *MemoryAlloc(size_t bytes)
{
	void *mem = VirtualAlloc(NULL, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	return mem;
}

void MemoryFree(void *mem, size_t bytes)
{
	VirtualFree(mem, bytes, MEM_RELEASE);
}

#endif