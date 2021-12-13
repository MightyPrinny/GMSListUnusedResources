#ifndef FL_SUBSTR_LIST_H
#define FL_SUBSTR_LIST_H

#include "includes.h"
#include "memory.h"

struct SubStrList
{
	char const **strings;
	uint *lengths;
	uint count;
	uint capacity;
};

void AddSubstr(SubStrList *list, char const *str, uint strLength)
{
	list->lengths[list->count] = strLength;
	list->strings[list->count] = str;
	++list->count;
}

SubStrList *PushSubstrList(MemoryStack *memStack, uint capacity)
{
	size_t totalSize = sizeof(SubStrList) 
						+	sizeof(uint)*capacity
						+ sizeof(char *)*capacity;

	char *mem = PushMem<char*>(memStack, totalSize);
	SubStrList *list = (SubStrList*)mem;
	mem += sizeof(SubStrList);

	list->lengths = (uint*)mem;
	mem += sizeof(uint)*capacity;

	list->strings = (char const **)mem;
	list->capacity = capacity;

	printf("Pushed substr list of size %zuKiB\n", totalSize/(1024));
	return list;
}

void Clear(SubStrList *list)
{
	memset(list->strings,0 , sizeof(char *)*list->capacity);
	memset(list->lengths, 0, sizeof(uint)*list->capacity);
	list->count = 0;
}

#endif