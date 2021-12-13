

#ifndef FL_MEMORYSTACK_H
#define FL_MEMORYSTACK_H

#include "includes.h"
#include "platform.h"
#include <assert.h>
#include <string.h>

#define MEGABYTES(MB) MB*1024*1024
#define KILOBYTES(MB) MB*1024

const uint MEM_STACK_MAX_ALLOCS = 2048;

struct MemoryStack
{
	char* memory;
	size_t length;
	char* baseFreeAddr;
	size_t allocSizes[MEM_STACK_MAX_ALLOCS];
	uint allocCount;
};

MemoryStack* InitMemory(size_t byteSize)
{
	size_t totalSize = sizeof(MemoryStack) + byteSize;
	char *memAlloc = (char*)MemoryAlloc(totalSize);
	if(!memAlloc)
	{
		fprintf(stderr, "Error initializing memory\n");
		assert(false);
	}
	MemoryStack *mem = (MemoryStack*)memAlloc;
	memAlloc += sizeof(MemoryStack);
	mem->memory = (char*)memAlloc;
	mem->baseFreeAddr = mem->memory;
	mem->length = byteSize;
	//fprintf(stderr, "Initialized memory with %lu Megabytes\n", byteSize/(1024*1024));
	return mem;
}

void DeleteMemoryStack(MemoryStack *mem)
{
	MemoryFree(mem, mem->length + sizeof(MemoryStack));
}

template <typename T>
T PushMem(MemoryStack *mem, size_t byteSize)
{
	assert((mem->baseFreeAddr + byteSize) <= (mem->memory + mem->length));
	assert(mem->allocCount+1 < MEM_STACK_MAX_ALLOCS);

	T retMem = (T)mem->baseFreeAddr;
	memset(mem->baseFreeAddr, 0, byteSize);
	//memset(mem->baseFreeAddr, 0, byteSize);
	mem->allocSizes[mem->allocCount++] = byteSize;
	mem->baseFreeAddr += byteSize;
	
	return retMem;
}

template <typename T>
inline
T* PushStruct(MemoryStack *mem)
{
	return PushMem<T*>(mem, sizeof(T));
}

void PopMem(MemoryStack *mem)
{
	assert(mem->allocCount - 1 >= 0);
	size_t byteSize = mem->allocSizes[--mem->allocCount];
	assert((mem->baseFreeAddr - byteSize) >= mem->memory);

	//memset(mem->baseFreeAddr - byteSize, 0, byteSize);
	
	mem->baseFreeAddr -= byteSize;
}

void TestMemory(MemoryStack *mem)
{
	struct TestStruct
	{
		int x;
		int y;
		char chars[64];
	};

	PushMem<TestStruct*>(mem, sizeof(TestStruct));
	PushMem<TestStruct*>(mem, sizeof(TestStruct));
	PushMem<TestStruct*>(mem, sizeof(TestStruct));
	PushMem<TestStruct*>(mem, sizeof(TestStruct));
	PushMem<char*>(mem, 129);
	PushMem<char*>(mem, 1);
	PushMem<char*>(mem, 200);
	assert(mem->allocCount == 7);
	assert(mem->baseFreeAddr == (mem->memory + sizeof(TestStruct)*4 + 129 + 1+ 200));

	PopMem(mem);
	PopMem(mem);
	PopMem(mem);
	PopMem(mem);
	
	PopMem(mem);
	PopMem(mem);
	PopMem(mem);
	assert(mem->allocCount == 0);
	assert(mem->baseFreeAddr == mem->memory);

	printf("MemoryStack test passed\n");
}

#endif