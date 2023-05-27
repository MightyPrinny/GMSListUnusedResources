#ifndef FLGLOBAL_INCLUDES_H
#define FLGLOBAL_INCLUDES_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
//#include "meow_hash_x64_aesni.h"
#include <assert.h>


#ifndef WIN32
#define InlineFunc inline __attribute__((always_inline))
#else
#define InlineFunc inline
#endif

#define ArraySize(x) ((sizeof(x) / sizeof(0 [x])) / ((size_t)(!(sizeof(x) % sizeof(0 [x])))))

#ifndef WIN32
#define Likely(x)    __builtin_expect (!!(x), 1)
#define Unlikely(x)  __builtin_expect (!!(x), 0)
#else
#define Likely(x) x 
#define Unlikely(x)  x
#endif


typedef unsigned int str_size_t;

typedef unsigned int uint;
typedef int Bool;

static size_t PageSize;

struct StringHash
{
	long long unsigned a;
	long long unsigned b;
};

struct StringHash32
{
	uint32_t val;
};

//Hash function from :
//https://www.codeproject.com/Articles/716530/Fastest-Hash-Function-for-Table-Lookups-in-C
#define _PADr_KAZE(x, n) ( ((x) << (n))>>(n) )
uint32_t FNV1A_Pippip_Yurii(const char *str, size_t wrdlen) 
{
	const uint32_t PRIME = 591798841u; uint32_t hash32; uint64_t hash64 = 14695981039346656037u;
	size_t Cycles, NDhead;
	if (wrdlen > 8) 
	{
		Cycles = ((wrdlen - 1)>>4) + 1; NDhead = wrdlen - (Cycles<<3);
#ifndef WIN32
		#pragma nounroll

#endif
		for(; Cycles--; str += 8) 
		{
			hash64 = ( hash64 ^ (*(uint64_t *)(str)) ) * PRIME;        
			hash64 = ( hash64 ^ (*(uint64_t *)(str+NDhead)) ) * PRIME;        
		}
	} 
	else
		hash64 = ( hash64 ^ _PADr_KAZE(*(uint64_t *)(str+0), (8-wrdlen)<<3) ) * PRIME;        
	hash32 = (uint32_t)(hash64 ^ (hash64>>32)); return hash32 ^ (hash32 >> 16);
}

/*
InlineFunc
StringHash ComputeHash(char const *cStr, str_size_t size)
{
	StringHash result;
	meow_u128 hash = MeowHash(MeowDefaultSeed, size, cStr);
	result.a = MeowU64From(hash, 0);
	result.b = MeowU64From(hash, 1);

	return result;
}
*/

InlineFunc
StringHash32 ComputeHash32(char const *cStr, str_size_t size)
{
	StringHash32 result;
	result.val = FNV1A_Pippip_Yurii(cStr, size);
	return result;
}

InlineFunc 
Bool HashesAreEqual(StringHash a, StringHash b)
{
	return (a.a == b.a) & (b.b == a.b);
}

InlineFunc 
Bool HashesAreEqual(StringHash32 a, StringHash32 b)
{
	return a.val == b.val;
}

InlineFunc 
uint HashToUnsignedInt(StringHash hash)
{
	return hash.a;
}

InlineFunc 
uint HashToUnsignedInt(StringHash32 hash)
{
	return hash.val;
}

void PrintErrno(char const *str)
{
	fprintf(stderr, "%s", str);
	fprintf(stderr,"%s\n", strerror(errno));
}

#endif
