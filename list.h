#ifndef FL_LIST_H
#define FL_LIST_H
#include "includes.h"
#include "memory.h"

template <typename T>
struct List
{
	T *values;
	uint count;
	uint capacity;
};

template <typename T>
List<T>* PushList(MemoryStack *memStack, uint capacity)
{
	size_t totalSize = sizeof(List<T>) + sizeof(T)*capacity;
	char *mem = PushMem<char*>(memStack, totalSize);
	List<T> *list = (List<T>*)mem;
	mem += sizeof(List<T>);
	list->values = (T*)mem;
	list->capacity = capacity;
	//printf("Pushed list of size %zuKiB\n", totalSize/(1024));
	return list; 
}

template <typename T>
void Clear(List<T> *list)
{
	memset(list->values, 0, list->capacity*sizeof(T));
}

template <typename T>
void Add(List<T> *list, T value)
{
	list->values[list->count++] = value;
}
#endif