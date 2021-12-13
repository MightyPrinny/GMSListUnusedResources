#ifndef FL_STRING_H
#define FL_STRING_H

#include "includes.h"
#include "memory.h"

//A char* with it's size for convenience
struct Buffer
{
	str_size_t size;
	char *ptr;
};

struct ConstPtrBuffer
{
	str_size_t size;
	char const *ptr;
	operator Buffer() const { return Buffer{size, (char*)ptr}; }
};

#define BStr(LITERAL) Buffer{sizeof(LITERAL)-1, (char*)LITERAL}
#define StrArg(LITERAL) LITERAL, sizeof(LITERAL)-1
#define BUFF_FORMAT "%.*s"
#define BUFF_PRINTARG(BUFF) BUFF.size, BUFF.ptr

InlineFunc
Buffer PushBuffer(MemoryStack *mem, str_size_t byteSize)
{
	char* memBlock = PushMem<char*>(mem, byteSize);
	Buffer buff = {byteSize, memBlock};
	return buff;
}

void PrintAsString(Buffer b)
{
	printf(BUFF_FORMAT, BUFF_PRINTARG(b));
}

InlineFunc
Bool StringEqual(char const *cStrA, str_size_t sizeA, char const *cStrB, str_size_t sizeB)
{
	int result = 0;
	if( (sizeA > 0) & (sizeB == sizeA))
	{
		result = sse42_strstr(cStrA, sizeA, cStrB, sizeB) == 0;
	}
	return result;
}

InlineFunc
Bool StringEqual(const Buffer &cStrA, const Buffer &cStrB)
{
	int result = 0;
	if( (cStrA.size > 0) & (cStrA.size == cStrB.size))
	{
		result = sse42_strstr(cStrA.ptr, cStrA.size, cStrB.ptr, cStrB.size) == 0;
	}
	return result;
}

InlineFunc
Bool StringEqual(const Buffer &cStrA, char const *cStrB, str_size_t sizeB)
{
	int result = 0;
	if( (cStrA.size > 0) & (cStrA.size == sizeB))
	{
		result = sse42_strstr(cStrA.ptr, cStrA.size, cStrB, sizeB) == 0;
	}
	return result;
}

InlineFunc
Bool CStringEqual(char const *strA, char const *strB)
{
	return strcmp(strA, strB) == 0;
}

Buffer SubstrNoExtension(Buffer str)
{
	Buffer result = Buffer{0,str.ptr};
	char const *at = str.ptr;

	while(*at != '.' && (at - str.ptr) < (str.size))
	{
		++at;
	}
	
	if(at > str.ptr)
	{
		result.size = at - str.ptr;
	}
	
	return result;
}


Buffer SubstrBasename(Buffer str)
{
	Buffer result = SubstrNoExtension(str);
	if(result.size > 0)
	{
		str = result;
		result.ptr = str.ptr + str.size - 1;

		while( (*result.ptr != '/' & *result.ptr != '\\' ) && ( result.ptr >= str.ptr))
		{
			--result.ptr;
		}
		
		if(result.ptr >= str.ptr)
		{
			++result.ptr;
			result.size = (str.ptr + str.size) - result.ptr;
			return result;
		}
		
		return str;
	}
	return result;
}

Buffer SubstrCdUp(Buffer str)
{
	char const *at = str.ptr + str.size - 1;

	while(*at != '/' & *at != '\\')
	{
		--at;
		if(at < str.ptr)
		{
			++at;
			str.size = 0;
			return str;
		}
	}

	str.size = at - str.ptr;

	return str;
}


Bool EndsWith(char const *strA, str_size_t strASize, char const *suffixStr, str_size_t suffixSize)
{
	Bool result = strASize != 0 & suffixSize != 0 & suffixSize <= strASize;
	if(!result)
		return false;

	result = true;

	char const *atSuffix = suffixStr + (suffixSize-1);
	char const *atStr = strA + (strASize-1);

	do
	{
		if(*atSuffix != *atStr)
		{
			result = false;
			break;
		}
		--atSuffix;
		--atStr;
	}while(atSuffix >= suffixStr & atStr >= strA);

	return result;
}

Bool StringContains(char const *str, str_size_t size, char const *subStr, str_size_t subStrSize, int scanDir = 1)
{
	return sse42_strstr(str, size, subStr, subStrSize) <= (size - subStrSize);
	
}

InlineFunc
Bool EndsWith(Buffer strA, Buffer strB)
{
	return EndsWith(strA.ptr, strA.size, strB.ptr, strB.size);
}

InlineFunc
Bool EndsWith(Buffer strA, char const *strB, str_size_t bSize)
{
	return EndsWith(strA.ptr, strA.size, strB, bSize);
}

void Copy(char const *source, str_size_t copySize, char *dest)
{
	memcpy(dest, source, copySize);
}

void Copy(Buffer source, char *dest)
{
	memcpy(dest, source.ptr, source.size);
}

void StringEqualsTest()
{
	assert(StringEqual(BStr("ass"), BStr("ass")));
	assert(!StringEqual(BStr("Ass"), BStr("ass")));
	assert(!StringEqual(BStr("Ass"), BStr("Asscheeks")));
	assert(!StringEqual(BStr("Axscheeks"), BStr("Asscheeks")));
	assert(!StringEqual(BStr("Asscheeks"), BStr("Ass")));
	assert(StringContains(StrArg("Asscheeks"), StrArg("Ass")));
	assert(!StringContains(StrArg("Ass"), StrArg("Asss")));
	assert(StringContains(StrArg("Asscheeks"), StrArg("Ass")));
	assert(!StringContains(StrArg("Asscheeks"), StrArg("Asss")));
	assert(StringContains(StrArg("</room>"), StrArg("</room")));
	assert(!StringContains(StrArg("</rooms>"), StrArg("</roomsz>")));
}

void EndsWithTest()
{
	assert(EndsWith(BStr("Ass"), BStr("ss")));
	assert(EndsWith(BStr("Cars.room.gmx"), BStr(".room.gmx")));
	assert(EndsWith(BStr("Cars.room.goop"), BStr(".room.goop")));
	assert(!EndsWith(BStr("Cars.room.goop"), BStr(".room.gmx")));
	assert(!EndsWith(BStr("cat.object"), BStr("tce")));
	printf("Ends With test passed\n");
}

void TestSubstrNoExtension()
{
	assert( StringEqual(BStr("ass"), SubstrNoExtension(BStr("ass.Room.gmx"))) );
	assert( !StringEqual(BStr(".ass"), SubstrNoExtension(BStr(" "))) );
	assert( StringEqual(BStr("Mario"), SubstrNoExtension(BStr("Mario.ass"))) );
	assert( StringEqual(BStr("Mario"), SubstrNoExtension(BStr("Mario"))) );
}

void TestSubstrBaseName()
{
	assert( StringEqual(SubstrBasename(BStr("Dir0/ass")), BStr("ass")) );
	auto specialCase = SubstrBasename(BStr("Dir0/.ass"));
	assert(specialCase.size == 0);
	assert( StringEqual(SubstrBasename(BStr("Dir1/dir2/Mario.ass")), BStr("Mario")) );
	assert( StringEqual(SubstrBasename(BStr("Mario")), BStr("Mario")) );
	//Same with backslash
	assert( StringEqual(SubstrBasename(BStr("Dir0\\ass")), BStr("ass")) );
	specialCase = SubstrBasename(BStr("Dir0\\.ass"));
	assert(specialCase.size == 0);
	assert( StringEqual(SubstrBasename(BStr("Dir1\\dir2\\Mario.ass")), BStr("Mario")) );
	assert( StringEqual(SubstrBasename(BStr("Mario")), BStr("Mario")) );
}

void TestSubstrCdUp()
{
	assert( StringEqual(SubstrCdUp( BStr("Opus/Magnus/file.xml") ), BStr("Opus/Magnus")));
	assert( StringEqual(SubstrCdUp( BStr("Opus/Magnus") ), BStr("Opus")));
	auto specialCase = SubstrCdUp(BStr("Opus"));
	assert(specialCase.size == 0);
	
}

uint CountLines(char const *str, uint size)
{
	uint lineCount = 0;
	for(uint pos = 0; pos < size; ++pos)
	{
		lineCount += (*(str + pos) == '\n');
	}
	return lineCount;
}

#endif