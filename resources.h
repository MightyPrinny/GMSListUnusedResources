#ifndef FL_RESOURCES_H
#define FL_RESOURCES_H

#include "includes.h"
#include "memory.h"
#include "directory.h"

struct Resources
{
	enum ResourceType
	{
		Room = 0,
		Sounds = 1,
		Sprite = 2,
		Background = 3,
		Path = 4,
		Script = 5,
		Object = 6,
		Timeline = 7,
	};
	uint startIndex[8];
	uint countByType[8];
	uint count;
	uint capacity;
	char **names;
	uint *nameLength;
	
	uint dataFilesStart;
	uint dataFilesCount;
	Directory *dataFileDirs;
};

struct StAddResources
{
	Resources::ResourceType type;
};

Resources *PushResourcesList(MemoryStack *memStack, uint dataFileCount, uint maxElements = 20000)
{
	uint totalSize = sizeof(Resources) 
						+ sizeof(char*)*maxElements 
						+ sizeof(uint)*maxElements
						+ sizeof(Directory)*dataFileCount;
	printf("Resources list size: %uKiB\n", totalSize/(1024));
	char const *mem = PushMem<char*>(memStack, totalSize);
	Resources *resList = (Resources*)mem;
	mem += sizeof(Resources);

	resList->names = (char**)mem;
	mem += sizeof(char*)*maxElements;

	resList->nameLength = (uint*)mem;
	mem += sizeof(uint)*maxElements;

	resList->dataFileDirs = (Directory*)mem;

	resList->capacity = maxElements;

	return resList;
}

//Start with rooms
StAddResources BeginAddResources()
{
	StAddResources stAdd = {};
	return stAdd;
}

void AddResource(Buffer name, StAddResources *stAdd, Resources *res)
{
	assert(res->count < res->capacity);
	res->names[res->count] = name.ptr;
	res->nameLength[res->count] = name.size;
	++res->countByType[stAdd->type];
	++res->count;
}

void NextResourceType(StAddResources *stAdd, Resources *res)
{
	stAdd->type = (Resources::ResourceType)(stAdd->type + 1);
	res->startIndex[stAdd->type] = res->count;
}

InlineFunc
Buffer GetResourceName(Resources *res, uint index)
{
	return Buffer{res->nameLength[index], res->names[index]};
}

InlineFunc
Buffer GetResourceNameByType(Resources *res, Resources::ResourceType type, uint index)
{
	uint resIndex = res->startIndex[type] + index;
	return Buffer{res->nameLength[resIndex], res->names[resIndex]};
}

#endif