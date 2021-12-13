#ifndef FL_DIRECTORY_H
#define FL_DIRECTORY_H
#include "memory.h"

#define MAX_DIR_DEPTH 24

//Directory stuff

///Directory structure, folder names are stored in then order they are printed
struct Directory
{
	char const *folders[MAX_DIR_DEPTH];
	//char const *file;
	//char const *fileExt;
	//uint fileExtLength;
	//uint fileNameLength;
	uint nameLengths[MAX_DIR_DEPTH];
	uint count;
	bool global = true;
};

int GetPathLength(Directory *dir)
{
	uint pathLength = dir->count + 1;
	for(int i = 0; i<dir->count; ++i)
	{	
		pathLength += dir->nameLengths[i];
	}	
	return pathLength;
}

void ToCString(Directory dir, char *dest)
{
	char *writePos = dest;
	
	#ifndef WIN32
	if(dir.global)
	{
		*writePos = '/';
		++writePos;
	}
	#endif
	for(int i=0; i<dir.count; ++i)
	{
		Copy(dir.folders[i], dir.nameLengths[i], writePos);
		writePos += dir.nameLengths[i];
		if(i != dir.count -1)
		{
			*writePos = '/';
			++writePos;
		}
		
	}

	/*if(dir.file)
	{
		Copy(dir.file, dir.fileNameLength, dest);
		writePos += dir.fileNameLength;
	}*/

	*writePos = '\0';
}

void ToFilepathCStr(Directory dir, char *dest, char const *filename, str_size_t nameLength
						, char const *ext = NULL, str_size_t extNameLength = 0)
{
	char *writePos = dest;
	
	#ifndef WIN32
	if(dir.global)
	{
		*writePos = '/';
		++writePos;
	}
	#endif

	for(int i=0; i<dir.count; ++i)
	{
		Copy(dir.folders[i], dir.nameLengths[i], writePos);
		writePos += dir.nameLengths[i];
		*writePos = '/';
		++writePos;
	}

	Copy(filename, nameLength, writePos);
	writePos += nameLength;

	if(ext)
	{
		Copy(ext, extNameLength, writePos);
		writePos += extNameLength;
	}

	*writePos = '\0';
}

void Cd(Directory *dir, char const *folder, uint nameLength)
{
	assert(dir->count+1 < MAX_DIR_DEPTH);
	/*if(dir->file)
	{
		dir->file = NULL;
		dir->fileExt = NULL;
		dir->fileExtLength = 0;
		dir->fileNameLength = 0;
	}*/
	dir->folders[dir->count] = folder;
	dir->nameLengths[dir->count] = nameLength;
	++dir->count;
}

void CdLocal(Directory *dir, Directory localPath)
{
	assert(dir->count+localPath.count < MAX_DIR_DEPTH);
	/*if(dir->file)
	{
		dir->file = NULL;
		dir->fileExt = NULL;
		dir->fileExtLength = 0;
		dir->fileNameLength = 0;
	}*/
	for(int i=0; i<localPath.count; ++i)
	{
		Cd(dir, localPath.folders[i], localPath.nameLengths[i]);
	}
}

/*
void CdFile(Directory *dir, char const *filename, str_size_t filenameLength)
{
	dir->file = filename;
	dir->fileNameLength = filenameLength;
}*/

InlineFunc
void Cd(Directory *dir, Buffer folderName)
{
	Cd(dir, folderName.ptr, folderName.size);
}

void CdUp(Directory *dir)
{
	/*
	if(dir->file)
	{
		dir->file = NULL;
		dir->fileExt = NULL;
		dir->fileExtLength = 0;
		dir->fileNameLength = 0;
		return;
	}
	*/
	if(dir->count <= 0)
		return;

	dir->count -= 1;
	dir->folders[dir->count] = NULL;
	dir->nameLengths[dir->count] = 0;
	
}

///Build a directory structure from a source string
///This doesn't copy the data, it just points to it
void FromAbsolutePathCstr(Directory *dir, char const *absolutePath)
{
	char const *current = absolutePath;

	char const *nameStart = current;
	uint nameLength = 0;
	while(*current != '\0')
	{
		if(*current == '/' | *current == '\\' | *(current+1) == '\0')
		{
			nameLength += *(current+1) == '\0';
			
			if(nameLength > 0)
			{
				Cd(dir, nameStart, nameLength);
			}
			nameLength = 0;
			nameStart = current + 1;
		}
		else
		{
			++nameLength;
		}
		++current;
	}
	
}



#endif