

#ifndef FL_PROGRAM_CPP
#define FL_PROGRAM_CPP

#include "platform.h"
#include "includes.h"
#include "memory.h"
#include "files.h"
#include "dumb_parser.h"
#include "set.h"
#include "resources.h"
#include "list.h"
#include <time.h>

#ifndef WIN32
#include "linux/platform_linux.h"
#else
#include "windows/platform_windows.h"
#endif

//#define assert(EXPR) 

const int MAX_RESOURCES = 40000;

typedef List<char[PATH_MAX]> PathList;
#define PushPathList PushList<char[PATH_MAX]>

void TestCrap(MemoryStack *mem)
{
	EndsWithTest();
	TestMemory(mem);
	TestStringSet(mem);
	StringEqualsTest();
	TestSubstrNoExtension();
	TestSubstrBaseName();
	TestSubstrCdUp();
}

struct Arguments
{
	enum Flags : int
	{
		None = 0,
		HasObjWhitelist = 1<<0,
		HasObjBlacklist = 1<<1,
		HasProjectPath = 1<<2,
		ExclusiveMode = 1<<3,
		MarkUnused = 1<<4,
		HasRoomWhitelist = 1<<5,
		HasRoomBlacklist = 1<<6,
		GenerateProject = 1<<7,
		UnusedAsConstants = 1<<8
	};

	uint flags;
	char *filename;
	char const *genProjPrefix;
	StringSet *roomWhiteList;
	StringSet *roomBlackList;
	List<ConstPtrBuffer> *objectWhiteList;
	StringSet *objectBlackList;
};

Bool GetResourcesWithTag(DXmlWalk *walk, StAddResources *stAdd, Resources *res,  Buffer tag, Buffer stopEndTag, StringSet *blackList = NULL)
{
	while(TrySeekTagOpen(walk, tag.ptr, tag.size))
	{
		GetStringValueAndCloseTag(walk);
		
		Buffer baseName = SubstrBasename(walk->str);
		if(baseName.size > 1)
		{
			if(!blackList || !Contains(blackList, baseName.ptr, baseName.size))
				AddResource(baseName, stAdd, res);
			//printf("added " BUFF_FORMAT "\n", BUFF_PRINTARG(baseName));
		}
		
		if(TrySeekNextTagOpen(walk, stopEndTag))
		{
			return true;
			break;
		}
	}
	return false;
}

//This kis kind of like a predicting parser
void GMSProjListResources(MemoryStack *memStack, Arguments *args, Buffer proj, Resources **outRes, StringSet **outSet)
{
	Resources *res;
	DXmlWalk walk = {};

	uint lineCount = CountLines(proj.ptr, proj.size);
	printf("Proj file line count: %u\n", lineCount);
	StringSet *set = PushStringSet(memStack, 12000, lineCount);
	*outSet = set;

	DXmlWalkBegin(&walk, proj.ptr);
	SeekAfterComment(&walk);
	
	SeekAfterTagOpen(&walk, StrArg("datafiles"));

	printf("Finding rooms in datafiles\n");

	uint dataFileCount = 0;
	uint totalDatafiles = 0;

	if(StringEqual(walk.str, StrArg("datafiles")))
	{	
		while(!TrySeekNextTagOpen(&walk, StrArg("sounds")) && NextTagOpen(&walk))
		{
			if(StringEqual(walk.str, StrArg("datafile")))
			{
				totalDatafiles += 1;
				if(SeekAfterTagOpen(&walk, StrArg("filename")))
				{
					GetStringValueAndCloseTag(&walk);
				
					if(EndsWith(walk.str, StrArg(".room.gmx")))
					{
						++dataFileCount;
					}

					SeekAfterTagEnd(&walk, StrArg("datafile"));
				}
			}
		}
	}

	printf("Number of rooms in datafiles %d\n", dataFileCount);
	printf("Total datafiles %d\n", totalDatafiles);
	
	res = PushResourcesList(memStack, dataFileCount, lineCount);
	*outRes = res;

	DXmlWalkBegin(&walk, proj.ptr);
	SeekAfterComment(&walk);
	SeekAfterTagOpen(&walk, StrArg("assets"));
	
	StAddResources stAdd = BeginAddResources();
	res->dataFilesStart = res->count;

	Directory dataFileDir = {};
	Buffer dirPathBuff = PushBuffer(memStack, PATH_MAX);

	if(TrySeekAfterTagOpenName(&walk, StrArg("datafiles")))
	{
		printf("Finding external room\n");
		int depth = 0;
		Bool open = true;
		SeekBeginTagBackwards(&walk);
		while(SeekAfterNextTagName(&walk, &open))
		{
			if(StringEqual(walk.str, StrArg("datafiles")))
			{
				if(open)
				{
					if(TrySeekAfterAttributeName(&walk, StrArg("name")))
					{
						NextAttributeValue(&walk);
						Cd(&dataFileDir, walk.str);
						++depth;
					}
				}
				else
				{
					CdUp(&dataFileDir);
					--depth;
					if(depth == 0)
					{
						break;
					}
				}
			}
			else if(open && StringEqual(walk.str, StrArg("datafile")))
			{
				SeekAfterTagOpen(&walk, StrArg("filename"));
				
				GetStringValueAndCloseTag(&walk);
				
				
				if(EndsWith(walk.str, StrArg(".room.gmx")))
				{
					Buffer noExt = SubstrNoExtension(walk.str);
					
					if( ((args->flags & Arguments::ExclusiveMode) == 0) & (args->flags & Arguments::HasRoomBlacklist))
					{
						if(!Contains(args->roomBlackList, noExt.ptr, noExt.size))
						{
							if(Add(set, noExt))
							{
								AddResource(noExt, &stAdd, res);
								//printf("Added room " BUFF_FORMAT "\n", BUFF_PRINTARG(noExt));
								res->dataFileDirs[res->dataFilesCount++] = dataFileDir;
							}
						}
					}
					else
					{
						if(Add(set, noExt))
						{
							AddResource(noExt, &stAdd, res);
							//printf("Added room " BUFF_FORMAT "\n", BUFF_PRINTARG(noExt));
							res->dataFileDirs[res->dataFilesCount++] = dataFileDir;
						}
					}
				}
			}
		}

	}

	if(res->dataFilesCount > dataFileCount)
		printf("%d > %d Wtf\n\n", res->dataFilesCount, dataFileCount);

	PopMem(memStack); //Path buffer

	if(TrySeekTagOpen(&walk, StrArg("rooms")))
	{
		printf("Finding rooms\n");
		SeekBeginTagBackwards(&walk);
		while(TrySeekTagOpen(&walk, StrArg("room")))
		{
			GetStringValueAndCloseTag(&walk);
			Buffer baseName = SubstrBasename(walk.str);
			if(args->flags & Arguments::HasRoomBlacklist)
			{
				if(!Contains(args->roomBlackList, baseName.ptr, baseName.size))
				{
					if(Add(set, baseName))
					{
						AddResource(baseName, &stAdd, res);
						//printf("Added room " BUFF_FORMAT "\n", BUFF_PRINTARG(baseName));
					}
				}
			}
			else
			{
				if(Add(set, baseName))
				{
					AddResource(baseName, &stAdd, res);
					//printf("Added room " BUFF_FORMAT "\n", BUFF_PRINTARG(baseName));
				}
			}

			if(TrySeekNextTagOpen(&walk, StrArg("constants")))
			{
				break;
			}
		}
	}
	
	if(*walk.at == '\0')
	{
		printf("this shouldn't happen at line %d", __LINE__);
	}

	
	DXmlWalkBegin(&walk, proj.ptr);
	SeekAfterComment(&walk);
	
	printf("Finding sounds\n");
	NextResourceType(&stAdd, res);
	GetResourcesWithTag(&walk, &stAdd, res,  BStr("sound"), BStr("sprites"));

	printf("Finding sprites\n");
	NextResourceType(&stAdd, res);
	GetResourcesWithTag(&walk, &stAdd, res,BStr("sprite"), BStr("backgrounds"));

	printf("Finding backgrounds\n");
	NextResourceType(&stAdd, res);
	GetResourcesWithTag(&walk, &stAdd, res, BStr("background"), BStr("paths"));

	printf("Finding paths\n");
	NextResourceType(&stAdd, res);
	GetResourcesWithTag(&walk, &stAdd, res, BStr("path"), BStr("scripts"));

	printf("Finding scripts\n");
	NextResourceType(&stAdd, res);
	GetResourcesWithTag(&walk, &stAdd, res, BStr("script"), BStr("objects"));

	auto bl = args->objectBlackList;
	if((args->flags & Arguments::HasObjBlacklist) == 0)
	{
		bl = NULL;
	}
	printf("Finding objects\n");
	NextResourceType(&stAdd, res);
	GetResourcesWithTag(&walk, &stAdd, res, BStr("object"), BStr("timelines"), bl);

	printf("Finding timelines\n");
	NextResourceType(&stAdd, res);
	GetResourcesWithTag(&walk, &stAdd, res, BStr("timeline"), BStr("rooms"), bl);

	printf("\nTotal resource count excluding extensions, timelines and paths %d\n", res->count);

	printf("Total Room count %d\n", res->countByType[Resources::Room]);
	printf("External room count %d\n", res->dataFilesCount);
	printf("Object count %d\n", res->countByType[Resources::Object]);
	printf("Background count %d\n", res->countByType[Resources::Background]);
	printf("Sprite count %d\n", res->countByType[Resources::Sprite]);
	printf("Sound count %d\n", res->countByType[Resources::Sounds]);
	printf("Script count %d\n", res->countByType[Resources::Script]);
	printf("Timeline count %d\n", res->countByType[Resources::Timeline]);
	printf("Path count %d\n", res->countByType[Resources::Path]);

	Clear(set);
}

struct DSplitWalk
{
	char const *at;
	ConstPtrBuffer str;
};

void DSplitWalkBegin(DSplitWalk *walk, char const *buff)
{
	walk->at = buff;
	walk->str.ptr = walk->at;
	walk->str.size = 0;
}

InlineFunc 
Bool IsSep(char c)
{
	//static char const separators[] = {'\0','|','~','!','=','"','\t','\r', '\n',' ','\\','/', '&', ';', '(', ')', '{', '}', '[', ']', '<', '>'};
	Bool b1 = c == '\0' | c == '|' | c == ',' | c == '~' | c == ' ' | c == '\r';
	Bool b2 = c == '!' | c == '"' | c == '=' | c == '\t' | c == '\r' | c == '\t' | c == '\n';
	Bool b3 = c == '/' | c == '&' | c == ';' | c == '(' | c == ')' | c == '{' | c == '}' | c == '[' | c == ']';
	Bool b4 = c == '<' | c == '>' | c == '.' | c == '+' | c == '-' | c == '\\';
	return b1 | b2 | b3 | b4;
}

InlineFunc 
Bool IsSepLine(char c)
{
	return c == '\0' | c == '\n' | c == '\r';
}


InlineFunc
Bool NextToken(DSplitWalk *walk, char const *end)
{
	walk->str.size = 0;

	Bool isSep = 0;
	while( (walk->str.size < 1) & (walk->at < end))
	{
		isSep = ((walk->at + 1) >= end) | IsSep(*walk->at);
		walk->str.size = 0;
		walk->str.ptr = walk->at;
		
		while(!isSep)
		{
			++walk->at;
			++walk->str.size;
			isSep = ((walk->at) >= end) | IsSep(*walk->at);
			if(isSep)
			{
				break;
			}
		}

		walk->at += isSep;
	}
	
	return walk->str.size > 0;
}

InlineFunc
Bool NextLine(DSplitWalk *walk, char *end)
{
	walk->str.size = 0;

	Bool isSep = 0;
	while( (walk->str.size < 1) & (walk->at < end))
	{
		isSep = ((walk->at + 1) >= end) | IsSepLine(*walk->at);
		walk->str.size = 0;
		walk->str.ptr = walk->at;
		
		while(!isSep)
		{
			++walk->at;
			++walk->str.size;
			isSep = ((walk->at) >= end) | IsSepLine(*walk->at);
			if(isSep)
			{
				break;
			}
		}

		walk->at += isSep;
	}
	
	return walk->str.size > 0;
}

struct ResMap
{
	//StringSet *nameToResId;
	StringSet *objects;
	StringSet *scripts;
	StringSet *timelines;
	/*StringSet *backgrounds;
	StringSet *sprites;
	StringSet *sounds;
	*/
	StringSet *otherRes;
};

void MapResNamesToIndices(Resources *res, StringSet *nameToResId)
{
	printf("Mapping resource names to indices\n");
	Clear(nameToResId);
	for(uint i=0; i<res->count; ++i)
	{
		Add(nameToResId, res->names[i], res->nameLength[i], i);
	}
}

ResMap PushResMap(MemoryStack *mem, Resources *res)
{
	ResMap resMap;
	resMap.scripts = PushStringSet(mem, res->countByType[Resources::Script], res->countByType[Resources::Script]);
	resMap.objects = PushStringSet(mem, res->countByType[Resources::Object], res->countByType[Resources::Object]);
	resMap.timelines = PushStringSet(mem, res->countByType[Resources::Timeline], res->countByType[Resources::Timeline]);
	uint otherResCount = res->countByType[Resources::Sprite]
							 + res->countByType[Resources::Background]
							 + res->countByType[Resources::Path]
							 + res->countByType[Resources::Sounds];
	resMap.otherRes = PushStringSet(mem,otherResCount, otherResCount);
	/*resMap.sounds = PushStringSet(mem, res->countByType[Resources::Sounds], res->countByType[Resources::Sounds]);
	resMap.backgrounds = PushStringSet(mem, res->countByType[Resources::Background], res->countByType[Resources::Background]);
	resMap.sprites = PushStringSet(mem, res->countByType[Resources::Sprite], res->countByType[Resources::Sprite]);*/
	//MapResNamesToIndices(res, resMap.nameToResId);

	for(int i=0; i<res->countByType[Resources::Object]; ++i)
	{
		Add(resMap.objects, GetResourceNameByType(res, Resources::Object, i));
	}

	for(int i=0; i<res->countByType[Resources::Script]; ++i)
	{
		Add(resMap.scripts, GetResourceNameByType(res, Resources::Script, i));
	}

	//Other

	for(int i=0; i<res->countByType[Resources::Sounds]; ++i)
	{
		Add(resMap.otherRes, GetResourceNameByType(res, Resources::Sounds, i));
	}

	for(int i=0; i<res->countByType[Resources::Background]; ++i)
	{
		Add(resMap.otherRes, GetResourceNameByType(res, Resources::Background, i));
	}

	for(int i=0; i<res->countByType[Resources::Sprite]; ++i)
	{
		Add(resMap.otherRes, GetResourceNameByType(res, Resources::Sprite, i));
	}

	for(int i=0; i<res->countByType[Resources::Path]; ++i)
	{
		Add(resMap.otherRes, GetResourceNameByType(res, Resources::Path, i));
	}

	for(int i=0; i<res->countByType[Resources::Timeline]; ++i)
	{
		Add(resMap.timelines, GetResourceNameByType(res, Resources::Timeline, i));
	}
	

	return resMap;
}

void PopResMap(MemoryStack *mem)
{
	PopMem(mem);
	PopMem(mem);
	PopMem(mem);
	PopMem(mem);
	//PopMem(mem);
}

InlineFunc
void AddResToNextList(Buffer outDir, Buffer outName, Buffer outExt, PathList *nextScan, uint pathPastePos)
{
	assert(nextScan->count < nextScan->capacity);
	char *pathBuff = nextScan->values[nextScan->count];
	uint writePos = pathPastePos;
	*(pathBuff + writePos++) = '/';
	memcpy(pathBuff + writePos, outDir.ptr, outDir.size);
	writePos += outDir.size;
	*(pathBuff + writePos++) = '/';
	memcpy(pathBuff + writePos, outName.ptr, outName.size);
	writePos += outName.size;
	memcpy(pathBuff + writePos, outExt.ptr, outExt.size);
	writePos += outExt.size;
	*(pathBuff + writePos) = '\0';
	//printf("Path %s \n", pathBuff);
	nextScan->count += 1;
}

InlineFunc
void ScanFileAsCode(Directory const *rootFolder, Buffer const *fileBuff, ResMap *resMap, StringSet *usedRes, PathList *nextScan, uint pathPastePos)
{
	DSplitWalk walk;
	DSplitWalkBegin(&walk, fileBuff->ptr);

	Buffer outName;
	StringSetValue outVal;
	Buffer outExt;
	Buffer outDir;
	char *end = fileBuff->ptr + fileBuff->size;
	while(NextToken(&walk, end))
	{
		//printf(BUFF_FORMAT"\n", BUFF_PRINTARG(walk.str));
		Bool tryRecurse = false;
		Bool valid = false;

		auto hashedKey = StringsetComputeHashedKey(walk.str.ptr, walk.str.size);

		if(TryGet(resMap->objects, hashedKey, &outVal))
		{
			tryRecurse = true;
			valid = true;
			outDir = BStr("objects");
			outExt = BStr(".object.gmx");
		}
		else if(TryGet(resMap->scripts, hashedKey, &outVal))
		{
			tryRecurse = true;
			valid = true;
			outDir = BStr("scripts");
			outExt = BStr(".gml");
		}
		else if(TryGet(resMap->timelines, hashedKey, &outVal))
		{
			tryRecurse = true;
			valid = true;
			outDir = BStr("timelines");
			outExt = BStr(".timeline.gmx");
		}
		else 
		{
			valid = TryGet(resMap->otherRes, hashedKey, &outVal);
		}

		if(valid & (outVal.asBuff.size > 0))
		{
			outName = outVal.asBuff;
			if(Add(usedRes, outName.ptr, outName.size, outVal))
			{
				
				printf(BUFF_FORMAT "\n", BUFF_PRINTARG(outName));
				if(tryRecurse)
				{
					AddResToNextList(outDir, outName, outExt, nextScan, pathPastePos);
				}
			}
		}
	}
}

void ScanObjectFile(Directory const *rootFolder, Buffer *fileBuff, ResMap *resMap, StringSet *usedRes, PathList *nextScan, uint pathPastePos)
{
	Buffer code;
	StringSetValue outVal;

	DXmlWalk walk;
	DXmlWalkBegin(&walk, fileBuff->ptr);

	SeekAfterComment(&walk);

	if(TrySeekAfterTagOpenName(&walk, StrArg("spriteName")))
	{
		GetStringValueAndCloseTag(&walk);
		if(walk.str.size > 0)
		{
			if(TryGet(resMap->otherRes, walk.str.ptr, walk.str.size, &outVal))
			{
				//printf("has sprite " BUFF_FORMAT "\n", BUFF_PRINTARG(walk.str));
				if(Add(usedRes, outVal.asBuff))
				{
					printf(BUFF_FORMAT "\n", BUFF_PRINTARG(outVal.asBuff));
				}
			}
		}
	}


	
	if(TrySeekAfterTagOpenName(&walk, StrArg("parentName")))
	{
		GetStringValueAndCloseTag(&walk);
		if(walk.str.size > 0)
		{
			if(TryGet(resMap->objects, walk.str.ptr, walk.str.size, &outVal))
			{
				if(Add(usedRes, outVal.asBuff.ptr, outVal.asBuff.size, outVal))
				{		
					printf(BUFF_FORMAT "\n", BUFF_PRINTARG(outVal.asBuff));
					AddResToNextList(BStr("objects"), outVal.asBuff, BStr(".object.gmx"), nextScan, pathPastePos);
				}
			}
			
		}
	}

	if(TrySeekAfterTagOpenName(&walk, StrArg("maskName")))
	{
		GetStringValueAndCloseTag(&walk);
		if(walk.str.size > 0)
		{
			if(TryGet(resMap->otherRes, walk.str.ptr, walk.str.size, &outVal))
			{
				//printf("has mask " BUFF_FORMAT "\n", BUFF_PRINTARG(walk.str));
				if(Add(usedRes, outVal.asBuff))
				{
					printf(BUFF_FORMAT "\n", BUFF_PRINTARG(outVal.asBuff));
				}
			}
		}
	}

	

	if(TrySeekAfterTagOpenName(&walk, StrArg("path")))
	{
		GetStringValueAndCloseTag(&walk);
		if(walk.str.size > 0)
		{
			if(TryGet(resMap->otherRes, walk.str.ptr, walk.str.size, &outVal))
			{
				if(Add(usedRes, outVal.asBuff))
				{
					printf(BUFF_FORMAT "\n", BUFF_PRINTARG(outVal.asBuff));
				}
			}
		}
	}

	Bool open = true;
	while(TrySeekTagOpen(&walk, StrArg("string")))
	{
		GetStringValueAndCloseTag(&walk);
		if(walk.str.size > 0)
		{
			code = walk.str;
			ScanFileAsCode(rootFolder, &code, resMap, usedRes, nextScan, pathPastePos);
		}
	}

}

InlineFunc 
void ScanNextList(Directory *rootFolder, Buffer *resFile, StringSet *usedRes, ResMap *resMap, uint pathPastePos, PathList *toScan, PathList *nextScan)
{
	PathList *swapScan;
	while(nextScan->count > 0)
	{
		swapScan = nextScan;
		nextScan = toScan;
		toScan = swapScan;
		nextScan->count = 0;

		for(int i=0; i<toScan->count; ++i)
		{
			size_t fileSize;
			if(ReadTextFile(toScan->values[i], *resFile, &fileSize))
			{
				Resources::ResourceType type = Resources::Script;
				auto pathLen = strlen(toScan->values[i]);
				if(EndsWith(toScan->values[i], pathLen, StrArg(".object.gmx")))
				{
					type = Resources::Object;
				}
				else if(EndsWith(toScan->values[i], pathLen, StrArg(".timeline.gmx")))
				{
					type = Resources::Timeline;
				}

				switch (type)
				{
				case Resources::Object:
					ScanObjectFile(rootFolder, resFile, resMap, usedRes, nextScan, pathPastePos);
					break;
				case Resources::Timeline:
					ScanObjectFile(rootFolder, resFile, resMap, usedRes, nextScan, pathPastePos);
					break;
				case Resources::Script: 
				{
					Buffer code = {(uint)fileSize, resFile->ptr};
					ScanFileAsCode(rootFolder, &code, resMap, usedRes, nextScan, pathPastePos);
				}	break;
				
				default:
					break;
				}
			}
			else
			{
				fprintf(stderr, "Can't scan %s\n", toScan->values[i]);
			}	
			
		}
		toScan->count = 0;
	}
}

void ScanObjectWhiteList(Directory *rootFolder, List<ConstPtrBuffer> *whiteList, Buffer *resFile, StringSet *usedRes,
				  ResMap *resMap, PathList *listA, PathList *listB, uint pathPastePos)
{
	listB->count = 0;
	listA->count = 0;
	
	PathList *toScan = listA;
	PathList *nextScan = listB; 
	PathList *swapScan; 
	
	{
		Buffer outDir = BStr("objects");
		Buffer outExt = BStr(".object.gmx");
		StringSetValue outVal;
		for(int i=0; i < whiteList->count; ++i)
		{
			Buffer name = whiteList->values[i];
			if(TryGet(resMap->objects, name.ptr, name.size, &outVal))
			{
				if(Add(usedRes, outVal.asBuff))
				{
					AddResToNextList(outDir, outVal.asBuff, outExt, nextScan, pathPastePos);
					printf(BUFF_FORMAT "\n", BUFF_PRINTARG(outVal.asBuff));
				}
			}
		}
	}

	//Recurse the objects
	ScanNextList(rootFolder, resFile, usedRes, resMap, pathPastePos, toScan, nextScan);
}

void ScanRoom(Directory *rootFolder, Buffer *room, Buffer *resFile, StringSet *usedRes,
				  ResMap *resMap, PathList *listA, PathList *listB, uint pathPastePos)
{
	listB->count = 0;
	listA->count = 0;
	
	PathList *toScan = listA;
	PathList *nextScan = listB; 

	{
		DXmlWalk walk;
		DXmlWalkBegin(&walk, room->ptr);

		SeekAfterComment(&walk);

		//First scan code
		SeekAfterTagOpen(&walk, StrArg("code"));
		GetStringValueAndCloseTag(&walk);
		if(walk.str.size > 0)
		{
			Buffer code = walk.str;	
			ScanFileAsCode(rootFolder, &code, resMap, usedRes, nextScan, pathPastePos);
		}

		StringSetValue outVal;
		//First scan object instances
		Buffer outDir = BStr("objects");
		Buffer outExt = BStr(".object.gmx");
		Buffer code;

		if(SeekAfterTagOpen(&walk, StrArg("backgrounds")))
		{
			Bool open = true;
			SeekAfterNextTagOpenName(&walk);
			SeekBeginTagBackwards(&walk);
			while(SeekAfterNextTagName(&walk, &open))
			{
				if(StringEqual(walk.str, StrArg("background")))
				{
					if(!open)
					{
						SeekAfterChar(&walk, '>');
						break;
					}
					else
					{
						TrySeekAfterAttributeName(&walk, StrArg("name"));
						{
							NextAttributeValue(&walk);
							if(walk.str.size > 0)
							{	
								if(TryGet(resMap->otherRes, walk.str.ptr, walk.str.size, &outVal))
								{
									if(Add(usedRes, outVal.asBuff))
									{
										printf(BUFF_FORMAT "\n", BUFF_PRINTARG(outVal.asBuff));
									}
								}
							}
						}
					}
					SeekAfterChar(&walk, '>');
				}
				else
				{
					SeekAfterChar(&walk, '>');
					break;
				}
			}
		}
		SeekBeginTagBackwards(&walk);
		if(SeekAfterTagOpen(&walk, StrArg("instances")))
		{
			Bool open = true;
			SeekAfterNextTagOpenName(&walk);
			SeekBeginTagBackwards(&walk);
			while(SeekAfterNextTagName(&walk, &open))
			{
				if(StringEqual(walk.str, StrArg("instance")))
				{
					if(!open)
					{
						SeekAfterChar(&walk, '>');
						break;
					}
					else
					{
						TrySeekAfterAttributeName(&walk, StrArg("objName"));
						{
							NextAttributeValue(&walk);
							if(walk.str.size > 0)
							{	
								if(TryGet(resMap->objects, walk.str.ptr, walk.str.size, &outVal))
								{
									if(Add(usedRes, outVal.asBuff))
									{
										AddResToNextList(outDir, outVal.asBuff, outExt, nextScan, pathPastePos);
										printf(BUFF_FORMAT "\n", BUFF_PRINTARG(outVal.asBuff));
									}
								}
							}
						}
			
						TrySeekAfterAttributeName(&walk, StrArg("code"));
						{
							NextAttributeValue(&walk);
							
							if(walk.str.size > 0)
							{
								code = walk.str;
								ScanFileAsCode(rootFolder, &code, resMap, usedRes, nextScan, pathPastePos);
							}
						}
					}
					SeekAfterChar(&walk, '>');
				}
				else
				{
					SeekAfterChar(&walk, '>');
					break;
				}
			}
		}
		
		SeekBeginTagBackwards(&walk);
		//DXmlWalkBegin(&walk, room->ptr);
		//SeekAfterComment(&walk);
		if(SeekAfterTagOpen(&walk, StrArg("tiles")))
		{
			Bool open = true;
			
			SeekAfterNextTagOpenName(&walk);
			SeekBeginTagBackwards(&walk);
			while(SeekAfterNextTagName(&walk, &open))
			{
				if(StringEqual(walk.str, StrArg("tile")))
				{
					if(!open)
					{
						SeekAfterChar(&walk, '>');
						break;
					}
					else
					{
						
						TrySeekAfterAttributeName(&walk, StrArg("bgName"));
						{
							NextAttributeValue(&walk);
							if(walk.str.size > 0)
							{	
								if(TryGet(resMap->otherRes, walk.str.ptr, walk.str.size, &outVal))
								{
									if(Add(usedRes, outVal.asBuff))
									{
										//printf("TILE " BUFF_FORMAT "\n", BUFF_PRINTARG(walk.str));
										printf(BUFF_FORMAT "\n", BUFF_PRINTARG(outVal.asBuff));
									}
								}
							}
						}
					}
					SeekAfterChar(&walk, '>');
				}
				else
				{
					SeekAfterChar(&walk, '>');
					break;
				}
			}
		}


	}

	//Recurse the objects
	ScanNextList(rootFolder, resFile, usedRes, resMap, pathPastePos, toScan, nextScan);
}



void ScanUsedResources(MemoryStack *memStack, Arguments *args, Buffer absProjPath, Resources *res, StringSet *usedRes)
{
	PathList *listA = PushPathList(memStack, 5000);
	PathList *listB = PushPathList(memStack, 5000);
	Buffer pathBuff = PushBuffer(memStack, PATH_MAX);
	Buffer roomFileBuff = PushBuffer(memStack, MEGABYTES(20));
	Buffer resFileBuff = PushBuffer(memStack, MEGABYTES(10));

	ResMap resMap = PushResMap(memStack, res);

	Directory rootDir = {};
	FromAbsolutePathCstr(&rootDir, absProjPath.ptr);	
	CdUp(&rootDir);

	ToCString(rootDir, pathBuff.ptr);
	//printf("Base folder path %s\n", pathBuff.ptr );

	uint pathPastePos = strlen(pathBuff.ptr);
	for(int i=0; i<listB->capacity; ++i)
	{
		memcpy(listB->values[i], pathBuff.ptr, pathPastePos);
		memcpy(listA->values[i], pathBuff.ptr, pathPastePos);
	}

	if(args->flags & Arguments::HasObjWhitelist)
	{
		ScanObjectWhiteList(&rootDir, args->objectWhiteList, &resFileBuff, usedRes, &resMap, listA, listB, pathPastePos);
	}

	if( ((args->flags & Arguments::ExclusiveMode) != 0) & ((args->flags & Arguments::HasRoomWhitelist) != 0) )
	{
		for(uint i=0; i<res->dataFilesCount; ++i)
		{
			uint rid = i + res->dataFilesStart;
			Buffer resName = {res->nameLength[rid], res->names[rid]};

			if(!Contains(args->roomWhiteList, resName.ptr, resName.size))
			{
				continue;
			}

			Directory roomPath = rootDir;
			CdLocal(&roomPath, res->dataFileDirs[i]);
			

			ToFilepathCStr(roomPath, pathBuff.ptr, resName.ptr, resName.size, StrArg(".room.gmx"));
			//printf("External Room %s \n", pathBuff.ptr);
			if(ReadTextFile(pathBuff.ptr, roomFileBuff))
			{
				
				ScanRoom(&rootDir, &roomFileBuff, &resFileBuff, usedRes, &resMap, listA, listB, pathPastePos);
			}
			else
			{
				fprintf(stderr, "Exclusive-mode Room %s \n", pathBuff.ptr);
			}
		}

		//printf("Processing normal rooms\n");
		for(uint i=res->dataFilesStart + res->dataFilesCount; i<res->countByType[Resources::Room]; ++i)
		{
			uint rid = i + res->startIndex[Resources::Room];
			Buffer resName = {res->nameLength[rid], res->names[rid]};
			if(!Contains(args->roomWhiteList, resName.ptr, resName.size))
			{
				continue;
			}
			Directory roomPath = rootDir;
			Cd(&roomPath, StrArg("rooms"));

			ToFilepathCStr(roomPath, pathBuff.ptr, resName.ptr, resName.size, StrArg(".room.gmx"));
			//
			if(ReadTextFile(pathBuff.ptr, roomFileBuff))
			{
				ScanRoom(&rootDir, &roomFileBuff, &resFileBuff, usedRes, &resMap, listA, listB, pathPastePos);
			}
			else
			{
				fprintf(stderr, "Room %s \n", pathBuff.ptr);
			}
			
			
		}
	}
	else
	{
		//printf("Processing datafile rooms\n");
		for(int i=0; i<res->dataFilesCount; ++i)
		{
			int rid = i + res->dataFilesStart;
			Buffer resName = {res->nameLength[rid], res->names[rid]};

			if(args->flags & Arguments::HasRoomBlacklist)
			{
				if(Contains(args->roomBlackList, resName.ptr, resName.size))
				{
					continue;
				}
			}

			Directory roomPath = rootDir;
			CdLocal(&roomPath, res->dataFileDirs[i]);
			

			ToFilepathCStr(roomPath, pathBuff.ptr, resName.ptr, resName.size, StrArg(".room.gmx"));
			//printf("External Room %s \n", pathBuff.ptr);
			if(ReadTextFile(pathBuff.ptr, roomFileBuff))
			{
				
				ScanRoom(&rootDir, &roomFileBuff, &resFileBuff, usedRes, &resMap, listA, listB, pathPastePos);
			}
			else
			{
				fprintf(stderr, "External Room %s \n", pathBuff.ptr);
			}
			
		}

		//printf("Processing normal rooms\n");
		for(int i=res->dataFilesStart + res->dataFilesCount; i<res->countByType[Resources::Room]; ++i)
		{
			int rid = i + res->startIndex[Resources::Room];
			Buffer resName = {res->nameLength[rid], res->names[rid]};
			Directory roomPath = rootDir;
			Cd(&roomPath, StrArg("rooms"));

			ToFilepathCStr(roomPath, pathBuff.ptr, resName.ptr, resName.size, StrArg(".room.gmx"));
			//
			if(ReadTextFile(pathBuff.ptr, roomFileBuff))
			{
				ScanRoom(&rootDir, &roomFileBuff, &resFileBuff, usedRes, &resMap, listA, listB, pathPastePos);
			}
			else
			{
				fprintf(stderr, "Room %s \n", pathBuff.ptr);
			}
			
			
		}
	}

	PopResMap(memStack);

	PopMem(memStack);
	PopMem(memStack);
	PopMem(memStack);
	PopMem(memStack);
	PopMem(memStack);
}

Bool IsArg(char const * arg, char const *name, char const *alias)
{
	return CStringEqual(arg, name) || CStringEqual(arg, alias);
}

static char const * FL_License =
"Copyright 2021 Fabián L.\n"
"\n"
"Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation\n"
"files (the \"Software\"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify,\n"
"merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom theSoftware is\n"
"furnished to do so, subject to the following conditions:\n"
"\n"
"The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.\n"
"\n"
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT\n"
"NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND\n"
"NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES\n"
"OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN \n"
"CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n"
;

void PrintLicenses()
{
	printf("%s\n\n", FL_License);
	printf("Hash function from :\nhttps://www.codeproject.com/Articles/716530/Fastest-Hash-Function-for-Table-Lookups-in-C\n");
	printf("strstrsse4\n\n%s\n", WojciechLicense);
	
}

void PrintHelp()
{
	printf(
"GMS 1.4 List Unused Resources\n"
"2021 - https://github.com/MightyPrinny/GMSListUnusedResources\n"
"\n"
"This program analizes a GMS 1.4 project and lists all the unused resources\n"
"in it by scanning all the resources used by the rooms in the project\n"
"\n"
"USAGE: gmslur [OPTIONS] -p PROJECT_FILE\n"
"\n"
"OPTIONS\n"
"-h --help:\n"
"          Shows this text.\n"
"\n"
"-p --project:\n"
"          The path to the .project.gmx file of a GMS 1.4 project.\n"
"\n"
"-objw --obj-whitelist:\n"
"          A space separated list of the object resource names to always include\n"
"          in the project.\n"
"\n"
"-objb --obj-blacklist:\n"
"          A space separated list of the object resource names to always exclude\n"
"          from the project.\n"
"\n"
"-roomw --room-whitelist:\n"
"          A space separated list of the room resource names to include\n"
"          in the project (only used in exclusive mode).\n"
"\n"
"-roomb --room-blacklist:\n"
"          A space separated list of the room resource names to always exclude\n"
"          from the project. (not used in exclusive mode)\n"
"\n"
"-x --exclusive-mode:\n"
"          Only scan the dependencies for the white listed rooms.\n"
"\n"
"-mu --mark-unused:\n"
"          This adds \"-- unused\" after the name of an unused resource to make\n"
"          them easier to spot for humans.\n"
"\n"
"-gp --generate-project:\n"
"          Generates a new project file in a new file in the project's folder\n"
"          without the unused resources.\n"
"\n"
"-pf --project-prefix:\n"
"          Prefix used by the generated project file, the default is lw_\n"
"\n"
"-uc --unused-as-constants:\n"
"          Declare the unused resources as constants set to -4 in the generated\n"
"          project file.\n"
"\n"
"--show-licenses\n"
"          Shows the licenses for the code used to build this executable\n"
"\n"
);
}

void ParseResourceListArg(char const *arg, List<ConstPtrBuffer> *outList)
{

}

void GenerateProject(MemoryStack *memStack, Arguments *args, Buffer absProjPath, Buffer projFile, StringSet *unusedRes)
{
	Buffer atoiBuff = PushBuffer(memStack, 16);
	Buffer pathBuff = PushBuffer(memStack, PATH_MAX);
	Directory rootDir = {};
	Buffer baseNameNoExt = SubstrBasename(absProjPath);
	FromAbsolutePathCstr(&rootDir, absProjPath.ptr);	
	CdUp(&rootDir);
	ToCString(rootDir, pathBuff.ptr);
	uint len = strlen(pathBuff.ptr);
	uint writePos = len;
	
	pathBuff.ptr[writePos] = '/';
	++writePos;
	memcpy(pathBuff.ptr+writePos, args->genProjPrefix, strlen(args->genProjPrefix));
	writePos += strlen(args->genProjPrefix);
	memcpy(pathBuff.ptr + writePos, baseNameNoExt.ptr, baseNameNoExt.size);
	writePos += baseNameNoExt.size;
	memcpy(pathBuff.ptr+writePos, StrArg(".project.gmx"));
	writePos += BStr(".project.gmx").size;
	pathBuff.ptr[writePos] = '\0';

	DSplitWalk walk = {};
	walk.at = projFile.ptr;
	printf("Output project: %s\n", pathBuff.ptr);

	FILE *outFile = fopen(pathBuff.ptr, "w");

	if(!outFile)
	{
		PrintErrno("Couldn't open out file");
		return;
	}
	len = strlen(projFile.ptr);
	char *end = projFile.ptr + len;
	while(NextLine(&walk, end))
	{
		if((args->flags & Arguments::UnusedAsConstants) != 0 && StringContains(walk.str.ptr, walk.str.size, StrArg("<constants")))
		{
			break;
		}
		DSplitWalk lineWalk = {};
		lineWalk.at = walk.str.ptr;
		Bool skip = false;	
		while(NextToken(&lineWalk, (walk.str.ptr + walk.str.size)))
		{
			if(Contains(unusedRes, lineWalk.str.ptr, lineWalk.str.size))
			{
				skip = true;
				break;
			}
		}

		if(!skip)
			fprintf(outFile, "%.*s\n", walk.str.size, walk.str.ptr);	
	}

	if((args->flags & Arguments::UnusedAsConstants) != 0)
	{
		uint constantCount = 0;
		{
			DXmlWalk xWalk = {};
			//Continue from the constants
			DXmlWalkBegin(&xWalk, walk.str.ptr);
			SeekUntil(&xWalk, ' ');
			NextAttributeValue(&xWalk);
			if(xWalk.str.size > 0)
			{
				memcpy(atoiBuff.ptr, xWalk.str.ptr, xWalk.str.size);
				atoiBuff.ptr[xWalk.str.size] = '\0';
				constantCount = atoi(atoiBuff.ptr);
			}
		}

		constantCount += unusedRes->count;
		fprintf(outFile, "%s%d%s\n", "  <constants number=\"",constantCount,"\">");

		for(auto it = IterateStringSet(unusedRes); MoveNext(&it);)
		{
			fprintf(outFile, "    <constant name=\"" BUFF_FORMAT "\">-4</constant>\n", BUFF_PRINTARG(it.at->value.asBuff));
		}

		while(NextLine(&walk, end))
		{
			fprintf(outFile, "%.*s\n", walk.str.size, walk.str.ptr);	
		}
	}

	fclose(outFile);
	
}

Bool ParseListArg(MemoryStack *memStack, StringSet **outList, char const *lst)
{
	char const *at = lst;
	uint len = strlen(lst);
	ConstPtrBuffer buff = {0, lst};

	if(len < 1)
	{
		return false;
	}
	uint maxWords = 0;
	while(*at != '\0')
	{
		maxWords += (*at == ' ');
		++at;
	}
	++maxWords;

	*outList = PushStringSet(memStack, maxWords < 2048 ? maxWords : 2048, maxWords);
	auto list = *outList;
	
	at = lst;
	while(*at != '\0' && list->count < maxWords)
	{
		if((*at == ' ') | (*(at + 1) == '\0'))
		{
			buff.size += (*(at + 1) == '\0');
			if(buff.size > 0)
			{
				Add(list, buff);
			}
			buff.ptr = at + 1;
			buff.size = 0;
		}
		else
		{
			buff.size += 1;
		}
		++at;
	}

	if(list->count == 0)
	{
		PopMem(memStack);
		*outList = NULL;
		return false;
	}
	return true;
}

Bool ParseListArg(MemoryStack *memStack, List<ConstPtrBuffer> **outList, char const *lst)
{
	char const *at = lst;
	uint len = strlen(lst);
	ConstPtrBuffer buff = {0, lst};

	if(len < 1)
	{
		return false;
	}
	uint maxWords = 0;
	while(*at != '\0')
	{
		maxWords += (*at == ' ');
		++at;
	}
	++maxWords;

	*outList = PushList<ConstPtrBuffer>(memStack, maxWords);
	auto list = *outList;
	
	at = lst;
	while(*at != '\0' && list->count < maxWords)
	{
		if((*at == ' ') | (*(at + 1) == '\0'))
		{
			buff.size += (*(at + 1) == '\0');
			if(buff.size > 0)
			{
				Add(list, buff);
			}
			buff.ptr = at + 1;
			buff.size = 0;
		}
		else
		{
			buff.size += 1;
		}
		++at;
	}

	if(list->count == 0)
	{
		PopMem(memStack);
		*outList = NULL;
		return false;
	}
	return true;
}

void ParseArguments(MemoryStack *memStack, Arguments *outArgs, int argc, char ** argv)
{
	for(int i=1; i < argc; ++i)
	{	
		if(IsArg(argv[i], "--project", "-p"))
		{
			++i;
			if(i >= argc)
				continue;
			outArgs->filename = argv[i];
			//printf("Got project dir: %s\n", argv[i]);
		}
		else if(IsArg(argv[i], "--obj-whitelist", "-objw"))
		{
			++i;
			if(i >= argc)
				continue;
			if(ParseListArg(memStack, &outArgs->objectWhiteList, argv[i]))
			{
				outArgs->flags |= Arguments::HasObjWhitelist;
				printf("White listed obj count %d\n", outArgs->objectWhiteList->count);
				/*
				for(int i=0; i< outArgs->objectWhiteList->count; ++i)
				{
					printf(BUFF_FORMAT "\n", BUFF_PRINTARG(outArgs->objectWhiteList->values[i]));
				}
				*/
				printf("\n");
			}
		}
		else if(IsArg(argv[i], "--obj-blacklist", "-objb"))
		{
			++i;
			if(i >= argc)
				continue;
			if(ParseListArg(memStack, &outArgs->objectBlackList, argv[i]))
			{
				outArgs->flags |= Arguments::HasObjBlacklist;
				printf("Black listed obj count %d\n", outArgs->objectBlackList->count);
				/*
				for(auto it = IterateStringSet(outArgs->objectBlackList);
					MoveNext(&it);)
				{
					printf(BUFF_FORMAT "\n", BUFF_PRINTARG(it.at->value.asBuff));
				}
				*/
				printf("\n");
			}
		}
		else if(IsArg(argv[i], "--room-whitelist", "-roomw"))
		{
			++i;
			if(i >= argc)
				continue;
			if(ParseListArg(memStack, &outArgs->roomWhiteList, argv[i]))
			{
				outArgs->flags |= Arguments::HasRoomWhitelist;
				printf("White listed obj count %d\n", outArgs->roomWhiteList->count);
				/*
				for(auto it = IterateStringSet(outArgs->roomWhiteList);
					MoveNext(&it);)
				{
					printf(BUFF_FORMAT "\n", BUFF_PRINTARG(it.at->value.asBuff));
				}*/
				printf("\n");
			}
		}
		else if(IsArg(argv[i], "--room-blacklist", "-roomb"))
		{
			++i;
			if(i >= argc)
				continue;
			if(ParseListArg(memStack, &outArgs->roomBlackList, argv[i]))
			{
				outArgs->flags |= Arguments::HasRoomBlacklist;
				printf("Black listed room count %d\n", outArgs->roomBlackList->count);
				/*
				for(auto it = IterateStringSet(outArgs->roomBlackList);
					MoveNext(&it);)
				{
					printf(BUFF_FORMAT "\n", BUFF_PRINTARG(it.at->value.asBuff));
				}*/
				printf("\n");
			}
		}
		else if(IsArg(argv[i], "--exclusive", "-x"))
		{
			outArgs->flags |= Arguments::ExclusiveMode;
			printf("Exclusive mode\n");
		}
		else if(IsArg(argv[i], "--mark-unused", "-mu"))
		{
			outArgs->flags |= Arguments::MarkUnused;
			printf("Mark unused\n");
		}
		else if(IsArg(argv[i], "--generate-project", "-gp"))
		{
			outArgs->flags |= Arguments::GenerateProject;
		}
		else if(IsArg(argv[i], "--project-prefix", "-pf"))
		{
			++i;
			if(i >= argc)
				continue;
			outArgs->genProjPrefix = argv[i];
		}
		else if(IsArg(argv[i], "--unused-as-constants", "-uc"))
		{
			outArgs->flags |= Arguments::UnusedAsConstants;
		}
	}
}

int main(int argc, char** argv)
{
	auto tStart = time(NULL);	
	
	MemoryStack *mem = InitMemory(MEGABYTES(128));
	if(argc == 2)
	{
		if(CStringEqual(argv[1], "--tests"))
		{
			printf("Tests\n");
			TestCrap(mem);
			DeleteMemoryStack(mem);
			return 0;
		}
		else if(IsArg(argv[1], "--help", "-h"))
		{
			PrintHelp();
			DeleteMemoryStack(mem);
			return 0;
		}
		else if(CStringEqual(argv[1], "--show-licenses"))
		{
			PrintLicenses();
			DeleteMemoryStack(mem);
			return 0;
		}
	}
	else if(argc <= 1)
	{
		PrintHelp();
		DeleteMemoryStack(mem);
		return 0;
	}
	Arguments args = {};
	args.genProjPrefix = "lw_";
	ParseArguments(mem, &args, argc, argv);
	
	char const *projectFileName = args.filename;
	if(!projectFileName)
	{
		PrintHelp();
		DeleteMemoryStack(mem);
		return 0;
	}
	Buffer absoluteProjFilename = PushBuffer(mem, PATH_MAX);
	AbsolutePath(projectFileName, absoluteProjFilename.ptr);
	absoluteProjFilename.size = strlen(absoluteProjFilename.ptr);
	
	printf("realPath: %s\n", absoluteProjFilename.ptr);

	Buffer projFileBuff = PushBuffer(mem, MEGABYTES(3));
	if(!ReadTextFile(absoluteProjFilename.ptr, projFileBuff))
	{
		fprintf(stderr, "Couldn't open project file\n");
		DeleteMemoryStack(mem);
		return 1;
	}
	
	Resources *res;
	StringSet *usedResSet;
	StringSet *unusedResSet;
	
	GMSProjListResources(mem, &args, projFileBuff, &res, &usedResSet);
	
	printf("\nUSED_RESOURCES\n");
	
	ScanUsedResources(mem, &args, absoluteProjFilename, res, usedResSet);

	printf("\nUNUSED_RESOURCES\n");

	uint unused = 0;

	unusedResSet = PushStringSet(mem, usedResSet->bucketCount, usedResSet->nodePoolCapacity);
	for(int i=res->startIndex[Resources::Sounds]; i<res->count; ++i)
	{
		if(!Contains(usedResSet, res->names[i], res->nameLength[i]))
		{
			if((args.flags & Arguments::MarkUnused) != 0)
			{
				Add(unusedResSet, res->names[i], res->nameLength[i]);
				printf(BUFF_FORMAT " -- UNUSED\n", res->nameLength[i], res->names[i]);
			}
			else
			{
				Add(unusedResSet, res->names[i], res->nameLength[i]);
				printf(BUFF_FORMAT "\n", res->nameLength[i], res->names[i]);
			}
			++unused;
		}
	}

	if((args.flags & Arguments::ExclusiveMode) != 0 && (args.flags & Arguments::HasRoomWhitelist) )
	{
		for(int i=res->startIndex[Resources::Room] + res->dataFilesStart + res->dataFilesCount; i<res->countByType[Resources::Room]; ++i)
		{
			if(!Contains(args.roomWhiteList, res->names[i], res->nameLength[i]))
			{
				Add(unusedResSet, res->names[i], res->nameLength[i]);
				if((args.flags & Arguments::MarkUnused) != 0)
				{
					Add(unusedResSet, res->names[i], res->nameLength[i]);
					printf(BUFF_FORMAT " -- UNUSED\n", res->nameLength[i], res->names[i]);
				}
				else
				{
					Add(unusedResSet, res->names[i], res->nameLength[i]);
					printf(BUFF_FORMAT "\n", res->nameLength[i], res->names[i]);
				}
			}
		}
	}
	else if( (args.flags & Arguments::HasRoomBlacklist) != 0 && (args.flags & Arguments::ExclusiveMode) == 0)
	{
		for(int i=res->startIndex[Resources::Room] + res->dataFilesStart + res->dataFilesCount; i<res->countByType[Resources::Room]; ++i)
		{
			if(Contains(args.roomBlackList, res->names[i], res->nameLength[i]))
			{
				Add(unusedResSet, res->names[i], res->nameLength[i]);
				if((args.flags & Arguments::MarkUnused) != 0)
				{
					Add(unusedResSet, res->names[i], res->nameLength[i]);
					printf(BUFF_FORMAT " -- UNUSED\n", res->nameLength[i], res->names[i]);
				}
				else
				{
					Add(unusedResSet, res->names[i], res->nameLength[i]);
					printf(BUFF_FORMAT "\n", res->nameLength[i], res->names[i]);
				}
			}
		}
	}

	printf("\nTotal resources %d\n", res->count);
	printf("\nTotal resources used %d\n", usedResSet->count);
	printf("Total resources unused %d\n", unused);

	if(args.flags & Arguments::GenerateProject)
	{
		GenerateProject(mem, &args, absoluteProjFilename, projFileBuff, unusedResSet);
	}

	DeleteMemoryStack(mem);
	
	printf("Total Time taken %ld seconds\n", (time(NULL) - tStart));
	

	return 0;
}

#endif
