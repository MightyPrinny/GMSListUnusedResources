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
	std::string_view vStr    = std::string_view(cStrA, std::string_view::size_type(sizeA));
	std::string_view vStrB = std::string_view(cStrB, std::string_view::size_type(sizeB));
	return vStr == vStrB;
}

InlineFunc
Bool StringEqual(const Buffer &cStrA, const Buffer &cStrB)
{
	int result = 0;
	std::string_view vStr    = std::string_view(cStrA.ptr, std::string_view::size_type(cStrA.size));
	std::string_view vStrB = std::string_view(cStrB.ptr, std::string_view::size_type(cStrB.size));
	return vStr == vStrB;
}

InlineFunc
Bool StringEqual(const Buffer &cStrA, char const *cStrB, str_size_t sizeB)
{
	std::string_view vStr    = std::string_view(cStrA.ptr, std::string_view::size_type(cStrA.size));
	std::string_view vStrB = std::string_view(cStrB, std::string_view::size_type(sizeB));
	return vStr == vStrB;
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
	   std::string_view vStr    = std::string_view(str, std::string_view::size_type(size));
	      std::string_view vSubStr = std::string_view(subStr, std::string_view::size_type(subStrSize));

	               return vStr.find(vSubStr) != std::string_view::npos;
	
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
	assert(StringEqual(BStr("abc"), BStr("abc")));
	assert(!StringEqual(BStr("Abc"), BStr("abc")));
	assert(!StringEqual(BStr("Abc"), BStr("Abccheeks")));
	assert(!StringEqual(BStr("Axscheeks"), BStr("Abccheeks")));
	assert(!StringEqual(BStr("Abccheeks"), BStr("Abc")));
	assert(StringContains(StrArg("Abccheeks"), StrArg("Abc")));
	assert(!StringContains(StrArg("Abc"), StrArg("Abcs")));
	assert(StringContains(StrArg("Abccheeks"), StrArg("Abc")));
	assert(!StringContains(StrArg("Abccheeks"), StrArg("Abcs")));
	assert(StringContains(StrArg("</room>"), StrArg("</room")));
	assert(!StringContains(StrArg("</rooms>"), StrArg("</roomsz>")));
}

void EndsWithTest()
{
	assert(EndsWith(BStr("Abc"), BStr("bc")));
	assert(EndsWith(BStr("Cars.room.gmx"), BStr(".room.gmx")));
	assert(EndsWith(BStr("Cars.room.goop"), BStr(".room.goop")));
	assert(!EndsWith(BStr("Cars.room.goop"), BStr(".room.gmx")));
	assert(!EndsWith(BStr("cat.object"), BStr("tce")));
	printf("Ends With test pabced\n");
}

void TestSubstrNoExtension()
{
	assert( StringEqual(BStr("abc"), SubstrNoExtension(BStr("abc.Room.gmx"))) );
	assert( !StringEqual(BStr(".abc"), SubstrNoExtension(BStr(" "))) );
	assert( StringEqual(BStr("Mario"), SubstrNoExtension(BStr("Mario.abc"))) );
	assert( StringEqual(BStr("Mario"), SubstrNoExtension(BStr("Mario"))) );
}

void TestSubstrBaseName()
{
	assert( StringEqual(SubstrBasename(BStr("Dir0/abc")), BStr("abc")) );
	auto specialCase = SubstrBasename(BStr("Dir0/.abc"));
	assert(specialCase.size == 0);
	assert( StringEqual(SubstrBasename(BStr("Dir1/dir2/Mario.abc")), BStr("Mario")) );
	assert( StringEqual(SubstrBasename(BStr("Mario")), BStr("Mario")) );
	//Same with backslash
	assert( StringEqual(SubstrBasename(BStr("Dir0\\abc")), BStr("abc")) );
	specialCase = SubstrBasename(BStr("Dir0\\.abc"));
	assert(specialCase.size == 0);
	assert( StringEqual(SubstrBasename(BStr("Dir1\\dir2\\Mario.abc")), BStr("Mario")) );
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
