// directory.cc
//	Routines to manage a directory of file names.
//
//	The directory is a table of fixed length entries; each
//	entry represents a single file, and contains the file name,
//	and the location of the file header on disk.  The fixed size
//	of each directory entry means that we have the restriction
//	of a fixed maximum size for file names. 目录是一个固定长度的条目表；每个条目代表一个文件，包含文件名和文件头在磁盘上的位置。每个目录项的固定大小意味着我们对文件名有固定的最大大小限制。
//
//	The constructor initializes an empty directory of a certain size;
//	we use ReadFrom/WriteBack to fetch the contents of the directory
//	from disk, and to write back any modifications back to disk. 构造函数初始化一个特定大小的空目录；我们使用ReadFrom/write back从磁盘获取目录的内容，并将任何修改写回磁盘。
//
//	Also, this implementation has the restriction that the size
//	of the directory cannot expand.  In other words, once all the
//	entries in the directory are used, no more files can be created.
//	Fixing this is one of the parts to the assignment. 此外，此实现具有目录大小无法扩展的限制。换句话说，一旦使用了目录中的所有条目，就不能再创建文件了。解决这一问题是分配的一部分。
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "utility.h"
#include "filehdr.h"
#include "directory.h"
#include "system.h"

//----------------------------------------------------------------------
// Directory::Directory
// 	Initialize a directory; initially, the directory is completely
//	empty.  If the disk is being formatted, an empty directory
//	is all we need, but otherwise, we need to call FetchFrom in order
//	to initialize it from disk. 初始化目录；最初，目录是完全空的。如果正在格式化磁盘，我们只需要一个空目录，否则，我们需要调用FetchFrom来从磁盘初始化它。
//
//	"size" is the number of entries in the directory
//----------------------------------------------------------------------

Directory::Directory(int size)
{
    table = new DirectoryEntry[size];
    tableSize = size;
    for (int i = 0; i < tableSize; i++)
        table[i].inUse = FALSE;
}

//----------------------------------------------------------------------
// Directory::~Directory
// 	De-allocate directory data structure.
//----------------------------------------------------------------------

Directory::~Directory()
{
    delete[] table;
}

//----------------------------------------------------------------------
// Directory::FetchFrom
// 	Read the contents of the directory from disk.
//
//	"file" -- file containing the directory contents
//----------------------------------------------------------------------

void Directory::FetchFrom(OpenFile *file)
{
    (void)file->ReadAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::WriteBack
// 	Write any modifications to the directory back to disk
//
//	"file" -- file to contain the new directory contents
//----------------------------------------------------------------------

void Directory::WriteBack(OpenFile *file)
{
    (void)file->WriteAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::FindIndex
// 	Look up file name in directory, and return its location in the table of
//	directory entries.  Return -1 if the name isn't in the directory. 在目录中查找文件名，并返回其在目录项表中的位置。如果名称不在目录中，则返回-1。
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int Directory::FindIndex(char *name)
{
    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse && !strncmp(table[i].name, name, FileNameMaxLen))
            return i;
    return -1; // name not in directory
}

//----------------------------------------------------------------------
// Directory::Find
// 	Look up file name in directory, and return the disk sector number
//	where the file's header is stored. Return -1 if the name isn't
//	in the directory. 在目录中查找文件名，并返回存储文件头的磁盘扇区号。如果名称不在目录中，则返回-1。
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int Directory::Find(char *name)
{
    int i = FindIndex(name);

    if (i != -1)
        return table[i].sector;
    return -1;
}

//----------------------------------------------------------------------
// Directory::Add
// 	Add a file into the directory.  Return TRUE if successful;
//	return FALSE if the file name is already in the directory, or if
//	the directory is completely full, and has no more space for
//	additional file names. 将文件添加到目录中。如果成功，则返回TRUE；如果文件名已在目录中，或者目录已满，并且没有更多的空间容纳其他文件名，则返回FALSE。
//
//	"name" -- the name of the file being added
//	"newSector" -- the disk sector containing the added file's header
//----------------------------------------------------------------------

bool Directory::Add(char *name, int newSector)
{
    if (FindIndex(name) != -1)
        return FALSE;

    for (int i = 0; i < tableSize; i++)
        if (!table[i].inUse)
        {
            table[i].inUse = TRUE;
            strncpy(table[i].name, name, FileNameMaxLen);
            table[i].sector = newSector;
            return TRUE;
        }
    return FALSE; // no space.  Fix when we have extensible files.
}

//----------------------------------------------------------------------
// Directory::Remove
// 	Remove a file name from the directory.  Return TRUE if successful;
//	return FALSE if the file isn't in the directory. 从目录中删除文件名。如果成功，则返回TRUE；如果文件不在目录中，则返回FALSE。
//
//	"name" -- the file name to be removed
//----------------------------------------------------------------------

bool Directory::Remove(char *name)
{
    int i = FindIndex(name);

    if (i == -1)
        return FALSE; // name not in directory
    table[i].inUse = FALSE;
    return TRUE;
}

//----------------------------------------------------------------------
// Directory::List
// 	List all the file names in the directory. 列出目录中的所有文件名。
//----------------------------------------------------------------------

void Directory::List()
{
    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse)
            printf("%s\n", table[i].name);
}

//----------------------------------------------------------------------
// Directory::Print
// 	List all the file names in the directory, their FileHeader locations,
//	and the contents of each file.  For debugging. 列出目录中的所有文件名、它们的文件头位置以及每个文件的内容。用于调试。
//----------------------------------------------------------------------

void Directory::Print()
{
    FileHeader *hdr = new FileHeader;

    printf("\nDirectory contents:\n");
    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse)
        {
            printf("Name: %s, Path: %s, Sector: %d, Type: %s\n", table[i].name, table[i].path, table[i].sector, table[i].type ? "File" : "Directory");
            hdr->FetchFrom(table[i].sector);
            hdr->Print();
        }
    printf("\n");
    delete hdr;
}

int Directory::getType(char *name)
{
    int i = FindIndex(name);
    if (i != -1)
        return table[i].type;
    return -1;
}

bool Directory::isEmpty()
{
    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse)
            return false;
    return true;
}

bool Directory::AddTest(char *name, char *path, int newSector, int type)
{
    if (FindIndex(name) != -1)
        return FALSE;

    for (int i = 0; i < tableSize; i++)
        if (!table[i].inUse)
        {
            table[i].inUse = TRUE;
            strncpy(table[i].name, name, FileNameMaxLen + 1);
            strncpy(table[i].path, path, FilePathMaxLen + 1);
            table[i].sector = newSector;
            table[i].type = type;
            return TRUE;
        }
    return FALSE; // no space.  Fix when we have extensible files.
}

void Directory::getTable(int &tableSize, DirectoryEntry *&table)
{
    tableSize = this->tableSize;
    table = this->table;
}

bool Directory::cat(char *name)
{
    FileHeader *hdr = new FileHeader;

    int i = FindIndex(name);
    if (i == -1)
        return false;
    printf("File contents:\n");
    printf("Name: %s, Path: %s, Sector: %d, Type: %s\n", table[i].name, table[i].path, table[i].sector, table[i].type ? "File" : "Directory");
    hdr->FetchFrom(table[i].sector);
    hdr->Print();
    printf("\n");
    delete hdr;
    return true;
}

CurDir::CurDir()
{
}

void CurDir::FetchFrom(OpenFile *file)
{
    (void)file->ReadAt((char *)&sector, sizeof(CurDir), 0);
}

void CurDir::WriteBack(OpenFile *file)
{
    (void)file->WriteAt((char *)&sector, sizeof(CurDir), 0);
}
