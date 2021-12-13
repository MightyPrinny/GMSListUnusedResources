#ifndef FL_STRINGSET_H
#define FL_STRINGSET_H

#include "includes.h"
#include "fl_string.h"
#include "memory.h"

typedef StringHash32 StringSetKeyHash;

#define StringSetComputeHash ComputeHash32
#define StringSetHashEq HashesAreEqual
#define StringSetHashToU32 HashToUnsignedInt

union StringSetValue
{
	uint asUInt;
	Buffer asBuff;
	int asInt;
};

struct StringSetHashedKey
{
	StringSetKeyHash hash;
	ConstPtrBuffer key;
};

struct StringSetBucketNode
{
	StringSetValue value;
	StringSetBucketNode *next;
	StringSetHashedKey hashedKey;
};


struct StringSet
{
	uint count;
	uint nodePoolCapacity;
	uint bucketCount;
	StringSetBucketNode **buckets;
	StringSetBucketNode *nodePool;
};


struct StringSetIterator
{
	StringSet *set;
	StringSetBucketNode *at;
	uint bucketIndex;
};


StringSetIterator IterateStringSet(StringSet *set)
{
	StringSetIterator it = {};
	it.set = set;
	return it;
}


bool MoveNext(StringSetIterator *it)
{
	if(it->at != NULL)
	{
		it->at = it->at->next;
	}
	while(it->at == NULL && it->bucketIndex < it->set->bucketCount)
	{
		it->at = it->set->buckets[it->bucketIndex++];
	}
	
	return it->at != NULL;
}

StringSet *PushStringSet(MemoryStack *mem, uint bucketCount, uint nodePoolSize = 2048)
{
	size_t totalSize = sizeof(StringSet) 
						  + sizeof(StringSetBucketNode)*nodePoolSize 
						  + sizeof(StringSetBucketNode *) * bucketCount;

	//Partition memory block
	char* setMem = PushMem<char*>(mem, totalSize);
	
	StringSet *set = (StringSet *)setMem;
	setMem += sizeof(StringSet);

	set->bucketCount = bucketCount;
	set->buckets = (StringSetBucketNode **)setMem;
	setMem += sizeof(StringSetBucketNode *) * bucketCount;

	set->nodePoolCapacity = nodePoolSize;
	set->nodePool = (StringSetBucketNode *)setMem;
	
	//setMem += sizeof(StringSetBucketNode)*nodePoolSize;
	
	set->count = 0;

	//assert(setMem == (((char*)set) + totalSize));

	//printf("Pushed string set of size %zuKiB\n", totalSize/(1024));
	
	return set;
}


InlineFunc
Bool StringSetHashedKeyEq(StringSetHashedKey a, StringSetHashedKey b)
{
	return StringSetHashEq(a.hash, b.hash) && StringEqual(a.key, b.key);
}

InlineFunc
StringSetHashedKey StringsetComputeHashedKey(char const *cStr, uint size)
{
	return StringSetHashedKey{StringSetComputeHash(cStr,size), ConstPtrBuffer{size, cStr}};
}

InlineFunc
void Clear(StringSet *set)
{
	memset(set->nodePool, 0, sizeof(StringSetBucketNode)*set->nodePoolCapacity);
	memset(set->buckets, 0, sizeof(void *)*set->bucketCount);
	set->count = 0;
}


Bool Contains(StringSet *set, char const *cStr, uint size)
{
	if(set->count == 0)
	{
		return false;
	}
	auto hashKey = StringsetComputeHashedKey(cStr, size);

	uint lookupIndex = (StringSetHashToU32(hashKey.hash))%set->bucketCount;
	auto node = set->buckets[lookupIndex];

	while(node)
	{
		if(StringSetHashedKeyEq(node->hashedKey, hashKey))
		{
			return true;
		}
		else
		{
			node = node->next;
		}
	}
	return false;
}

Bool Contains(StringSet *set, StringSetHashedKey hashedKey)
{
	if(set->count == 0)
	{
		return false;
	}

	uint lookupIndex = (StringSetHashToU32(hashedKey.hash))%set->bucketCount;
	auto node = set->buckets[lookupIndex];

	while(node)
	{
		if(StringSetHashedKeyEq(node->hashedKey, hashedKey))
		{
			return true;
		}
		else
		{
			node = node->next;
		}
	}
	return false;
}

Bool TryGet(StringSet *set,const StringSetHashedKey &hashedKey, StringSetValue *outVal)
{
	if(Unlikely(set->count == 0))
	{
		return false;
	}
	
	uint lookupIndex = (StringSetHashToU32(hashedKey.hash))%set->bucketCount;

	auto *node = set->buckets[lookupIndex];
	while(node)
	{
		if(StringSetHashedKeyEq(node->hashedKey, hashedKey))
		{
			*outVal = node->value;
			return true;
		}
		node = node->next;
	}
	return false;
}

Bool TryGet(StringSet *set, char const *cStr, uint size, StringSetValue *outVal)
{
	if(Unlikely(set->count == 0))
	{
		return false;
	}
	auto hashedKey = StringsetComputeHashedKey(cStr, size);
	uint lookupIndex = (StringSetHashToU32(hashedKey.hash))%set->bucketCount;

	auto *node = set->buckets[lookupIndex];
	while(node)
	{
		if(StringSetHashedKeyEq(node->hashedKey, hashedKey))
		{
			*outVal = node->value;
			return true;
		}
		else
		{
			node = node->next;
		}
	}
	return false;
}

Bool TryGet(StringSet *set, char const *cStr, uint size, Buffer *outVal)
{
	StringSetValue value;
	Bool result = TryGet(set, cStr, size, &value);
	*outVal = value.asBuff;
	return result;
}

Bool TryGet(StringSet *set, char const *cStr, uint size, uint *outVal)
{
	StringSetValue value;
	Bool result = TryGet(set, cStr, size, &value);
	*outVal = value.asUInt;
	return result;
}

Bool TryGet(StringSet *set, char const *cStr, uint size, int *outVal)
{
	StringSetValue value;
	Bool result = TryGet(set, cStr, size, &value);
	*outVal = value.asInt;
	return result;
}


Bool Add(StringSet *set, char const *cStr, uint size, StringSetValue value)
{
	assert((set->count+1) <= set->nodePoolCapacity);

	auto hashedKey = StringsetComputeHashedKey(cStr, size);

	StringSetBucketNode *node = set->nodePool + set->count;
	node->hashedKey = hashedKey;
	node->next = NULL;
	node->value = value;

	uint lookupIndex = (StringSetHashToU32(hashedKey.hash))%set->bucketCount;

	auto *existingNode = set->buckets[lookupIndex];

	if(existingNode)
	{
		auto *prevNode = existingNode;
		do
		{	
			if(StringSetHashedKeyEq(existingNode->hashedKey, hashedKey))
			{
				return false;
			}
			prevNode = existingNode;
			existingNode = existingNode->next;
		}while(existingNode);

		prevNode->next = node;
	}
	else
	{
		set->buckets[lookupIndex] = node;
	}

	++set->count;
	return true;
}

Bool Add(StringSet *set, StringSetHashedKey hashedKey, StringSetValue value)
{
	assert((set->count+1) <= set->nodePoolCapacity);

	StringSetBucketNode *node = set->nodePool + set->count;
	node->next = NULL;
	node->value = value;
	node->hashedKey = hashedKey;
	

	uint lookupIndex = (StringSetHashToU32(hashedKey.hash))%set->bucketCount;

	auto *existingNode = set->buckets[lookupIndex];

	if(existingNode)
	{
		auto *prevNode = existingNode;
		do
		{	
			if(StringSetHashedKeyEq(existingNode->hashedKey, hashedKey))
			{
				return false;
			}
			prevNode = existingNode;
			existingNode = existingNode->next;
		}while(existingNode);

		prevNode->next = node;
	}
	else
	{
		set->buckets[lookupIndex] = node;
	}

	++set->count;
	return true;
}


Bool Add(StringSet *set, char const *cStr, uint size)
{
	StringSetValue val;
	val.asBuff = Buffer{size, (char*)cStr};
	return Add(set, cStr, size, val);
}

Bool Add(StringSet *set, char const *cStr, uint size, uint value)
{
	StringSetValue val;
	val.asUInt = value;
	return Add(set, cStr, size, val);
}

Bool Add(StringSet *set, char const *cStr, uint size, int value)
{
	StringSetValue val;
	val.asInt = value;
	return Add(set, cStr, size, val);
}

Bool Add(StringSet *set, Buffer cStr)
{
	StringSetValue val;
	val.asBuff = cStr;
	return Add(set, cStr.ptr, cStr.size, val);
}

Bool Add(StringSet *set, Buffer cStr, Buffer value)
{
	StringSetValue val;
	val.asBuff = value;
	return Add(set, cStr.ptr, cStr.size, val);
}

void TestStringSet(MemoryStack *mem)
{
	StringSet *set = PushStringSet(mem, 240, 100);
	
	
	Add(set, StrArg("PTestStr"));
	Add(set, StrArg("GsTrTest"));
	Add(set, StrArg("ATestStr"));
	Add(set, StrArg("aTestStr"));
	Add(set, StrArg("objTestA"));

	assert(Contains(set, StrArg("PTestStr")));
	assert(Contains(set, StrArg("GsTrTest")));
	assert(!Contains(set, StrArg("gStRtEST")));
	assert(Contains(set, StrArg("ATestStr")));
	assert(Contains(set, StrArg("aTestStr")));
	assert(Contains(set, StrArg("objTestA")));
	assert(!Contains(set, StrArg("objTestB")));

	StringSet *setInt = PushStringSet(mem, 2048, 11000);

	Add(setInt, StrArg("PTestStr"), 32u);
	Add(setInt, StrArg("GsTrTest"), 16u);
	Add(setInt, StrArg("ATestStr"), 22u);
	Add(setInt, StrArg("aTestStr"), 19u);
	Add(setInt, StrArg("objTestA"),29u);

	assert(Contains(setInt, StrArg("PTestStr")));
	assert(Contains(setInt, StrArg("GsTrTest")));
	assert(!Contains(setInt, StrArg("gStRtEST")));
	assert(Contains(setInt, StrArg("ATestStr")));
	assert(Contains(setInt, StrArg("aTestStr")));
	assert(Contains(setInt, StrArg("objTestA")));
	assert(!Contains(setInt, StrArg("objTestB")));

	for(auto it = IterateStringSet(set);
		MoveNext(&it);)
	{
		printf("Element " BUFF_FORMAT "\n", BUFF_PRINTARG(it.at->value.asBuff));
	}
	PopMem(mem);
	PopMem(mem);
	printf("StringSet test passed (verify iteration with prints in console)\n");
}

#endif