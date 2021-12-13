#ifndef FL_DUMBPARSER_H
#define FL_DUMBPARSER_H

//#include "includes.h"
#include "memory.h"

struct DXmlWalk
{
	char const *at;
	char const *src;
	ConstPtrBuffer str;
};

void DXmlWalkBegin(DXmlWalk* walk, char const *buff)
{
	walk->src = buff;
	walk->at = (char*)buff;
	walk->str.ptr = walk->at;
	walk->str.size = 0;
}

InlineFunc
Bool SeekAfterChar(DXmlWalk *walk, char c)
{
	while( (*walk->at != c) & (*walk->at != '\0') )
	{
		++walk->at;
	}
	
	Bool retVal = *walk->at == c;
	walk->at += retVal;
	
	return retVal;
}

InlineFunc
Bool SeekUntil(DXmlWalk *walk, char c)
{
	while(*walk->at != c & *walk->at != '\0')
	{
		++walk->at;
	}
	Bool retVal = *walk->at == c;
	return retVal;
}

InlineFunc
Bool SeekUntilNoNullCheck(DXmlWalk *walk, char c)
{
	while(*walk->at != c)
	{
		++walk->at;
	}
	Bool retVal = *walk->at == c;
	return retVal;
}

InlineFunc
Bool SeekBackwardsUntil(DXmlWalk *walk, char c)
{
	while( (*walk->at != c) & (walk->at > walk->src))
	{
		--walk->at;
	}
	Bool retVal = *walk->at == c;
	return retVal;
}

InlineFunc
Bool BuildTagNameAndSeekAfterTag(DXmlWalk *walk)
{
	walk->str.size = 0;
	walk->str.ptr = walk->at;
	while(*walk->at != '>' & *walk->at != '\0')
	{
		++walk->str.size;
		++walk->at;
		//This is to handle the ones with attributes
		if(*walk->at == ' ')
		{
			while(*walk->at != '>' & *walk->at != '\0')
			{
				++walk->at;
			}
			break;
		}
	}
	Bool rval = (walk->str.size > 0);
	return rval;
}

InlineFunc
Bool BuildTagName(DXmlWalk *walk)
{
	walk->str.size = 0;
	walk->str.ptr = walk->at;
	while(*walk->at != '>' & *walk->at != ' ' & *walk->at != '\0') 
	{
		++walk->str.size;
		++walk->at;
	}
	Bool rval = (walk->str.size > 0);
	return rval;
}

InlineFunc
Bool BuildTagName(DXmlWalk *walk, Bool *wasSpace)
{
	walk->str.size = 0;
	walk->str.ptr = walk->at;
	while(*walk->at != '>' & *walk->at != '\0')
	{
		if(*walk->at == ' ')
		{
			*wasSpace = true;
			break;
		}
		++walk->str.size;
		++walk->at;
	}
	Bool rval = (walk->str.size > 0);
	return rval;
}

InlineFunc
Bool BuildStringUntil(DXmlWalk *walk, char c)
{
	walk->str.size = 0;
	walk->str.ptr = walk->at;
	while( (*walk->at != c) & (*walk->at != '\0') )
	{
		++walk->str.size;
		++walk->at;
	}
	Bool rval = (walk->str.size > 0);
	return rval;
}

InlineFunc
void SeekAfterComment(DXmlWalk *walk)
{
	if( (*walk->at == '<') & (*(walk->at+1) == '!'))
	{
		SeekAfterChar(walk, '>');
		SeekUntil(walk, '<');
	}
}

InlineFunc
void GetStringValueAndCloseTag(DXmlWalk *walk)
{
	BuildStringUntil(walk, '<');
	SeekAfterChar(walk, '>');
}

InlineFunc
Bool SeekAfterTagOpen(DXmlWalk *walk, char const *tag, size_t tagSize)
{
	Bool accepted = 0;
	do
	{
		do
		{
			SeekAfterChar(walk, '<');
		}while(*walk->at == '/');
		BuildTagNameAndSeekAfterTag(walk);
		++walk->at;
		if(walk->str.size != 0 && StringEqual(walk->str, tag, tagSize))
		{
			accepted = 1;
			break;
		}
	}while(*walk->at != '\0');

	
	return accepted;
}

InlineFunc
Bool SeekAfterTagOpen(DXmlWalk *walk, Buffer tag)
{
	return SeekAfterTagOpen(walk, tag.ptr, tag.size);
}

InlineFunc
void SeekAfterTagEnd(DXmlWalk *walk, char const *tag, size_t tagSize)
{
	do
	{
		if(*walk->at != '/')
		{
			do
			{
				SeekAfterChar(walk, '<');	
			}while(*walk->at != '/');
		}
		++walk->at;
		BuildTagNameAndSeekAfterTag(walk);
		++walk->at;
		
	}while(*walk->at != '\0' && (!StringEqual(walk->str, tag, tagSize)));

}

InlineFunc
void SeekAfterTagEnd(DXmlWalk *walk, Buffer tag)
{
	SeekAfterTagEnd(walk, tag.ptr, tag.size);
}

InlineFunc
Bool NextTagOpen(DXmlWalk *walk)
{
	while(true)
	{
		while( (*walk->at != '<')  & (*walk->at != '\0') )
		{
			++walk->at;
		}
		
		if(*(walk->at + 1) != '/')
		{
			break;
		}
		++walk->at;
	}
	if(*walk->at == '<')
	{
		++walk->at;
		BuildTagNameAndSeekAfterTag(walk);	
		++walk->at;
		return walk->str.size > 0;
	}
	return 0;
}

InlineFunc
Bool NextEndTag(DXmlWalk *walk)
{
	do
	{
		SeekAfterChar(walk, '<');
	}while(*walk->at != '\0');
	if(*walk->at == '/')
	{
		++walk->at;
		BuildTagNameAndSeekAfterTag(walk);
		++walk->at;
		return 1;
	}
	return 0;
}

InlineFunc
Bool TrySeekNextTagEnd(DXmlWalk *walk, char const *tag, str_size_t tagLength)
{
	char const *at = walk->at;
	Bool result = NextEndTag(walk);
	if(!result)
	{
		walk->at = at;
	}
	if(StringEqual(walk->str, tag, tagLength))
	{
		return true;
	}
	walk->at = at;
	return false;
}

InlineFunc 
Bool TrySeekNextTagEnd(DXmlWalk *walk, Buffer tag)
{
	return TrySeekNextTagEnd(walk, tag.ptr, tag.size);
}

InlineFunc
Bool TrySeekNextTagOpen(DXmlWalk *walk, char const *tag, str_size_t tagLength)
{
	char const *at = walk->at;
	Bool result = NextTagOpen(walk);
	if(!result)
	{
		walk->at = at;
	}
	if(StringEqual(walk->str, tag, tagLength))
	{
		return true;
	}
	walk->at = at;
	return false;
}

InlineFunc
Bool TrySeekTagOpen(DXmlWalk *walk, char const *tag, str_size_t tagLength)
{
	char const *at = walk->at;
	auto prevStr = walk->str;

	while(NextTagOpen(walk))
	{
		if(StringEqual(walk->str, tag, tagLength))
		{
			return true;
		}
	}
	
	walk->at = at;
	return false;
}

InlineFunc
Bool TrySeekNextTagOpen(DXmlWalk *walk, Buffer tag)
{
	return TrySeekNextTagOpen(walk, tag.ptr, tag.size);
}

InlineFunc
Bool SeekAfterNextTagName(DXmlWalk *walk, Bool *open = NULL)
{
	Bool isOpen = true;
	Bool result = 0;

	while( (*walk->at != '<') & (*walk->at != '\0') )
	{
		++walk->at;
	}
	++walk->at;
	if(*walk->at == '/')
	{
		isOpen = false;
		++walk->at;
	}

	BuildTagName(walk);
	++walk->at;
	result = *walk->at != '\0' & walk->str.size > 0;

	if(open)
		*open = isOpen;
	return result;
}

InlineFunc
Bool SeekAfterNextTagOpenName(DXmlWalk *walk)
{
	Bool result = 0;

	do
	{
		while( (*walk->at != '<') & (*walk->at != '\0') )
		{
			++walk->at;
		}
	}while((*walk->at+1) == '/');
	if(*walk->at == '<')
		++walk->at;
	BuildTagName(walk);

	++walk->at;

	result = *walk->at != '\0' & walk->str.size > 0;

	return result;
}

InlineFunc
Bool SeekBeginTagBackwards(DXmlWalk *walk, Bool *open = NULL)
{
	Bool result = 0;
	Bool wasOpen = true;
	
	while( (*walk->at != '<') & (walk->at > walk->src) )
	{
		if(*walk->at == '/')
		{
			wasOpen = false;
		}
		--walk->at;
	}

	if(open)
	{
		*open = wasOpen;
	}

	result = *walk->at == '<';

	return result;
}

InlineFunc
Bool SeekAterNextAttributeName(DXmlWalk *walk)
{
	char const *prevPos = walk->at;
	walk->str.size = -1;

	while( (*walk->at != '=') & (*walk->at != '>') )
	{
		++walk->at;
	}

	if(*walk->at == '>')
	{
		return false;
	}
	char const *eqSignPos = walk->at;
	
	while((*walk->at != ' ') & (walk->at > walk->src))
	{
		--walk->at;
		++walk->str.size;
	}

	Bool result = false;
	
	if(*walk->at == ' ')
	{
		walk->str.ptr = walk->at + 1;
		result = walk->str.size > 0;
	}
	else
	{
		walk->str.size = 0;
		result = false;
	}

	walk->at = eqSignPos + 1;
	return result;
}

InlineFunc
Bool TrySeekAfterAttributeName(DXmlWalk *walk, char const *tag, str_size_t tagLength)
{
	char const *prevPos = walk->at;
	auto prevStr = walk->str;
	Bool result = false;
	while(SeekAterNextAttributeName(walk))
	{
		if(StringEqual(walk->str, tag, tagLength))
		{
			result = true;
			break;
		}
	}
	if(!result)
	{
		walk->at = prevPos;
		walk->str = prevStr;
	}

	return result;
}

InlineFunc
Bool NextAttributeValue(DXmlWalk *walk)
{
	Bool result = 0;

	SeekUntil(walk, '"');
	++walk->at;
	BuildStringUntil(walk, '"');
	++walk->at;
	result = walk->str.size > 0;

	return result;
}

InlineFunc
Bool TrySeekAfterTagOpenName(DXmlWalk *walk, char const *tag, str_size_t tagLength)
{
	Bool result = 0;	
	char const *prev = walk->at;
	auto prevStr = walk->str;

	while(SeekAfterNextTagOpenName(walk))
	{
		if(StringEqual(walk->str, tag, tagLength))
		{
			result = true;
			break;
		}
	}

	if(!result)
	{
		walk->at = prev;
		walk->str = prevStr;
	}

	return result;
}



#endif