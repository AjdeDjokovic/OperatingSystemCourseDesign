// filesys.cc
//	Routines to manage the overall operation of the file system.
//	Implements routines to map from textual file names to files.
//
//	Each file in the file system has:
//	   A file header, stored in a sector on disk
//		(the size of the file header data structure is arranged
//		to be precisely the size of 1 disk sector)
//	   A number of data blocks
//	   An entry in the file system directory
//
// 	The file system consists of several data structures:
//	   A bitmap of free disk sectors (cf. bitmap.h)
//	   A directory of file names and file headers
//
//      Both the bitmap and the directory are represented as normal
//	files.  Their file headers are located in specific sectors
//	(sector 0 and sector 1), so that the file system can find them
//	on bootup.
//
//	The file system assumes that the bitmap and directory files are
//	kept "open" continuously while Nachos is running.
//
//	For those operations (such as Create, Remove) that modify the
//	directory and/or bitmap, if the operation succeeds, the changes
//	are written immediately back to disk (the two files are kept
//	open during all this time).  If the operation fails, and we have
//	modified part of the directory and/or bitmap, we simply discard
//	the changed version, without writing it back to disk.
//
// 	Our implementation at this point has the following restrictions:
//
//	   there is no synchronization for concurrent accesses
//	   files have a fixed size, set when the file is created
//	   files cannot be bigger than about 3KB in size
//	   there is no hierarchical directory structure, and only a limited
//	     number of files can be added to the system
//	   there is no attempt to make the system robust to failures
//	    (if Nachos exits in the middle of an operation that modifies
//	    the file system, it may corrupt the disk)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "disk.h"
#include "bitmap.h"
#include "directory.h"
#include "filehdr.h"
#include "filesys.h"

// Sectors containing the file headers for the bitmap of free sectors,
// and the directory of files.  These file headers are placed in well-known
// sectors, so that they can be located on boot-up.
#define FreeMapSector 0
#define DirectorySector 1
#define CurDirSector 2

// Initial file sizes for the bitmap and directory; until the file system
// supports extensible files, the directory size sets the maximum number
// of files that can be loaded onto the disk.
#define FreeMapFileSize (NumSectors / BitsInByte)
#define NumDirEntries 10
#define DirectoryFileSize (sizeof(DirectoryEntry) * NumDirEntries)

#define CurDirFileSize (sizeof(CurDir))

//----------------------------------------------------------------------
// FileSystem::FileSystem
// 	Initialize the file system.  If format = TRUE, the disk has
//	nothing on it, and we need to initialize the disk to contain
//	an empty directory, and a bitmap of free sectors (with almost but
//	not all of the sectors marked as free).
//
//	If format = FALSE, we just have to open the files
//	representing the bitmap and the directory.
//
//	"format" -- should we initialize the disk?
//----------------------------------------------------------------------

// FileSystem::FileSystem(bool format)
// {
//     DEBUG('f', "Initializing the file system.\n");
//     if (format)
//     {
//         BitMap *freeMap = new BitMap(NumSectors);
//         Directory *directory = new Directory(NumDirEntries);
//         FileHeader *mapHdr = new FileHeader;
//         FileHeader *dirHdr = new FileHeader;

//         DEBUG('f', "Formatting the file system.\n");

//         // First, allocate space for FileHeaders for the directory and bitmap
//         // (make sure no one else grabs these!)
//         freeMap->Mark(FreeMapSector);
//         freeMap->Mark(DirectorySector);

//         // Second, allocate space for the data blocks containing the contents
//         // of the directory and bitmap files.  There better be enough space!

//         ASSERT(mapHdr->Allocate(freeMap, FreeMapFileSize));
//         ASSERT(dirHdr->Allocate(freeMap, DirectoryFileSize));

//         // Flush the bitmap and directory FileHeaders back to disk
//         // We need to do this before we can "Open" the file, since open
//         // reads the file header off of disk (and currently the disk has garbage
//         // on it!).

//         DEBUG('f', "Writing headers back to disk.\n");
//         mapHdr->WriteBack(FreeMapSector);
//         dirHdr->WriteBack(DirectorySector);

//         // OK to open the bitmap and directory files now
//         // The file system operations assume these two files are left open
//         // while Nachos is running.

//         freeMapFile = new OpenFile(FreeMapSector);
//         directoryFile = new OpenFile(DirectorySector);

//         // Once we have the files "open", we can write the initial version
//         // of each file back to disk.  The directory at this point is completely
//         // empty; but the bitmap has been changed to reflect the fact that
//         // sectors on the disk have been allocated for the file headers and
//         // to hold the file data for the directory and bitmap.

//         DEBUG('f', "Writing bitmap and directory back to disk.\n");
//         freeMap->WriteBack(freeMapFile); // flush changes to disk
//         directory->WriteBack(directoryFile);

//         if (DebugIsEnabled('f'))
//         {
//             freeMap->Print();
//             directory->Print();

//             delete freeMap;
//             delete directory;
//             delete mapHdr;
//             delete dirHdr;
//         }
//     }
//     else
//     {
//         // if we are not formatting the disk, just open the files representing
//         // the bitmap and directory; these are left open while Nachos is running
//         freeMapFile = new OpenFile(FreeMapSector);
//         directoryFile = new OpenFile(DirectorySector);
//     }
// }

//----------------------------------------------------------------------
// FileSystem::Create
// 	Create a file in the Nachos file system (similar to UNIX create).
//	Since we can't increase the size of files dynamically, we have
//	to give Create the initial size of the file.
//
//	The steps to create a file are:
//	  Make sure the file doesn't already exist
//        Allocate a sector for the file header
// 	  Allocate space on disk for the data blocks for the file
//	  Add the name to the directory
//	  Store the new file header on disk
//	  Flush the changes to the bitmap and the directory back to disk
//
//	Return TRUE if everything goes ok, otherwise, return FALSE.
//
// 	Create fails if:
//   		file is already in directory
//	 	no free space for file header
//	 	no free entry for file in directory
//	 	no free space for data blocks for the file
//
// 	Note that this implementation assumes there is no concurrent access
//	to the file system!
//
//	"name" -- name of file to be created
//	"initialSize" -- size of file to be created
//----------------------------------------------------------------------

bool FileSystem::Create(char *name, int initialSize)
{
    Directory *directory;
    BitMap *freeMap;
    FileHeader *hdr;
    int sector;
    bool success;

    DEBUG('f', "Creating file %s, size %d\n", name, initialSize);

    directory = new Directory(NumDirEntries);
    directory->FetchFrom(directoryFile);

    if (directory->Find(name) != -1)
        success = FALSE; // file is already in directory
    else
    {
        freeMap = new BitMap(NumSectors);
        freeMap->FetchFrom(freeMapFile);
        sector = freeMap->Find(); // find a sector to hold the file header
        if (sector == -1)
            success = FALSE; // no free block for file header
        else if (!directory->Add(name, sector))
            success = FALSE; // no space in directory
        else
        {
            hdr = new FileHeader;
            if (!hdr->Allocate(freeMap, initialSize))
                success = FALSE; // no space on disk for data
            else
            {
                success = TRUE;
                // everthing worked, flush all changes back to disk
                hdr->WriteBack(sector);
                directory->WriteBack(directoryFile);
                freeMap->WriteBack(freeMapFile);
            }
            delete hdr;
        }
        delete freeMap;
    }
    delete directory;
    return success;
}

//----------------------------------------------------------------------
// FileSystem::Open
// 	Open a file for reading and writing.
//	To open a file:
//	  Find the location of the file's header, using the directory
//	  Bring the header into memory
//
//	"name" -- the text name of the file to be opened
//----------------------------------------------------------------------

OpenFile *
FileSystem::Open(char *name)
{
    Directory *directory = new Directory(NumDirEntries);
    OpenFile *openFile = NULL;
    int sector;

    DEBUG('f', "Opening file %s\n", name);
    directory->FetchFrom(directoryFile);
    sector = directory->Find(name);
    if (sector >= 0)
        openFile = new OpenFile(sector); // name was found in directory
    delete directory;
    return openFile; // return NULL if not found
}

//----------------------------------------------------------------------
// FileSystem::Remove
// 	Delete a file from the file system.  This requires:
//	    Remove it from the directory
//	    Delete the space for its header
//	    Delete the space for its data blocks
//	    Write changes to directory, bitmap back to disk
//
//	Return TRUE if the file was deleted, FALSE if the file wasn't
//	in the file system.
//
//	"name" -- the text name of the file to be removed
//----------------------------------------------------------------------

bool FileSystem::Remove(char *name)
{
    Directory *directory;
    BitMap *freeMap;
    FileHeader *fileHdr;
    int sector;

    directory = new Directory(NumDirEntries);
    directory->FetchFrom(directoryFile);
    sector = directory->Find(name);
    if (sector == -1)
    {
        delete directory;
        return FALSE; // file not found
    }
    fileHdr = new FileHeader;
    fileHdr->FetchFrom(sector);

    freeMap = new BitMap(NumSectors);
    freeMap->FetchFrom(freeMapFile);

    fileHdr->Deallocate(freeMap); // remove data blocks
    freeMap->Clear(sector);       // remove header block
    directory->Remove(name);

    freeMap->WriteBack(freeMapFile);     // flush to disk
    directory->WriteBack(directoryFile); // flush to disk
    delete fileHdr;
    delete directory;
    delete freeMap;
    return TRUE;
}

//----------------------------------------------------------------------
// FileSystem::List
// 	List all the files in the file system directory.
//----------------------------------------------------------------------

void FileSystem::List()
{
    Directory *directory = new Directory(NumDirEntries);

    directory->FetchFrom(directoryFile);
    directory->List();
    delete directory;
}

//----------------------------------------------------------------------
// FileSystem::Print
// 	Print everything about the file system:
//	  the contents of the bitmap
//	  the contents of the directory
//	  for each file in the directory,
//	      the contents of the file header
//	      the data in the file
//----------------------------------------------------------------------

void FileSystem::Print()
{
    FileHeader *bitHdr = new FileHeader;
    FileHeader *dirHdr = new FileHeader;
    BitMap *freeMap = new BitMap(NumSectors);
    Directory *directory = new Directory(NumDirEntries);

    printf("Bit map file header:");
    bitHdr->FetchFrom(FreeMapSector);
    bitHdr->Print();

    printf("\nDirectory file header:");
    dirHdr->FetchFrom(DirectorySector);
    dirHdr->Print();

    freeMap->FetchFrom(freeMapFile);
    freeMap->Print();

    directory->FetchFrom(directoryFile);
    directory->Print();

    delete bitHdr;
    delete dirHdr;
    delete freeMap;
    delete directory;
}

BitMap *FileSystem::getBitMap()
{
    //NumSectors: DISK 上总扇区数（共有 32*32=1024 个扇区）
    BitMap *freeBitMap = new BitMap(NumSectors);
    freeBitMap->FetchFrom(freeMapFile);
    return freeBitMap;
}

int FileSystem::FindDir(char *name)
{
    int sector;
    OpenFile *openFile;
    Directory *directory;
    if (name[0] == '/')
    {
        sector = DirectorySector;
        directory = new Directory(NumDirEntries);
        directory->FetchFrom(directoryFile);
        int str_pos = 1;
        int sub_str_pos = 0;
        char sub_str[10];
        while (str_pos < strlen(name))
        {
            sub_str[sub_str_pos++] = name[str_pos++];
            if (name[str_pos] == '/')
            {
                sub_str[sub_str_pos] = '\0';
                if ((sector = directory->Find(sub_str)) == -1 || directory->getType(sub_str))
                {
                    DEBUG('f', "FindDir\n");
                    delete directory;
                    return -1;
                }

                openFile = new OpenFile(sector);
                directory->FetchFrom(openFile);
                delete openFile;
                str_pos++;
                sub_str_pos = 0;
            }
        }
        delete directory;
    }
    else
    {
        CurDir *curDir = new CurDir;
        curDir->FetchFrom(curDirFile);
        sector = curDir->sector;
        delete curDir;

        directory = new Directory(NumDirEntries);
        directory->FetchFrom(curDirectoryFile);
        int str_pos = 0;
        int sub_str_pos = 0;
        char sub_str[10];
        while (str_pos < strlen(name))
        {
            sub_str[sub_str_pos++] = name[str_pos++];
            if (name[str_pos] == '/')
            {
                sub_str[sub_str_pos] = '\0';
                if ((sector = directory->Find(sub_str)) == -1 || directory->getType(sub_str))
                {
                    DEBUG('f', "FindDir\n");
                    delete directory;
                    return -1;
                }

                openFile = new OpenFile(sector);
                directory->FetchFrom(openFile);
                delete openFile;
                str_pos++;
                sub_str_pos = 0;
            }
        }
        DEBUG('f', "%d\n", sector);
        delete directory;
    }
    return sector;
}

bool FileSystem::FindName(char *name, char *fileName)
{
    int pos = -1;
    for (int i = strlen(name) - 1; i >= 0; i--)
        if (name[i] == '/')
        {
            pos = i + 1;
            break;
        }
    if (pos == -1)
        strcpy(fileName, name);
    else
    {
        int i;
        for (i = 0; i < strlen(name) - pos; i++)
            fileName[i] = name[pos + i];
        fileName[i] = '\0';
    }
    return true;
}

void FileSystem::setBitMap(BitMap *freeMap)
{
    freeMap->WriteBack(freeMapFile);
}

bool FileSystem::CreateTest(char *name, int initialSize)
{
    OpenFile *openFile;
    Directory *directory;
    BitMap *freeMap;
    FileHeader *hdr;

    int sector;
    int dir_sector;
    char file_name[FileNameMaxLen + 1];
    DEBUG('f', "CreateTest file %s, size %d\n", name, initialSize);

    // if (name[0] != '/')
    // {
    //     DEBUG('f', "CreateTest name[0] != '/'\n");
    //     return false;
    // }

    if (((dir_sector = FindDir(name)) == -1) || !FindName(name, file_name))
    {
        DEBUG('f', "CreateTest findDir fail\n");
        return false;
    }

    openFile = new OpenFile(dir_sector);
    directory = new Directory(NumDirEntries);
    directory->FetchFrom(openFile);

    if (directory->Find(file_name) != -1)
    {
        DEBUG('f', "CreateTest find(filename) fail\n");
        delete openFile;
        delete directory;
        return false; // file is already in directory
    }
    freeMap = new BitMap(NumSectors);
    freeMap->FetchFrom(freeMapFile);
    sector = freeMap->Find(); // find a sector to hold the file header
    if (sector == -1)
    {
        DEBUG('f', "CreateTest freeMap->Find() fail\n");
        delete openFile;
        delete directory;
        delete freeMap;
        return false; // no free block for file header
    }

    //处理相对地址字符
    if (name[0] != '/')
    {
        DEBUG('f', "CreateTest name[0] != '/'\n");
        char tmp_file_name[FileNameMaxLen + 1];
        CurDir *curDir = new CurDir;
        curDir->FetchFrom(curDirFile);
        strcpy(tmp_file_name, curDir->path);
        if (!strcmp(tmp_file_name, "/"))
        {
            strcat(tmp_file_name, name);
            strcpy(name, tmp_file_name);
        }
        else
        {
            strcat(tmp_file_name, "/");
            strcat(tmp_file_name, name);
            strcpy(name, tmp_file_name);
        }
        delete curDir;
    }

    if (initialSize == -1)
    {
        if (!directory->AddTest(file_name, name, sector, DirType))
        {
            DEBUG('f', "CreateTest AddTest fail\n");
            delete openFile;
            delete directory;
            delete freeMap;
            return false;
        }
        hdr = new FileHeader;
        initialSize = DirectoryFileSize;
        if (!hdr->Allocate(freeMap, initialSize))
        {
            DEBUG('f', "CreateTest hdr->Allocate fail\n");
            delete openFile;
            delete directory;
            delete freeMap;
            delete hdr;
            return false;
        }
        hdr->WriteBack(sector);
        OpenFile *tmpOpenFile = new OpenFile(sector);
        Directory *tmpDirectory = new Directory(NumDirEntries);
        tmpDirectory->WriteBack(tmpOpenFile);
        delete tmpOpenFile;
        delete tmpDirectory;

        directory->WriteBack(openFile);
        freeMap->WriteBack(freeMapFile);
    }
    else
    {
        if (!directory->AddTest(file_name, name, sector, FileType))
        {
            DEBUG('f', "CreateTest AddTest fail\n");
            delete openFile;
            delete directory;
            delete freeMap;
            return false;
        }
        hdr = new FileHeader;
        if (!hdr->Allocate(freeMap, initialSize))
        {
            DEBUG('f', "CreateTest hdr->Allocate fail\n");
            delete openFile;
            delete directory;
            delete freeMap;
            delete hdr;
            return false;
        }
        hdr->WriteBack(sector);
        directory->WriteBack(openFile);
        freeMap->WriteBack(freeMapFile);
    }
    delete freeMap;
    delete hdr;

    delete openFile;
    delete directory;
    return true;
}

OpenFile *FileSystem::OpenTest(char *name)
{
    Directory *directory;
    OpenFile *resFile = NULL;
    OpenFile *openFile;
    int sector;
    int dir_sector;
    char file_name[FileNameMaxLen + 1];

    DEBUG('f', "OpenTest file %s\n", name);

    if (((dir_sector = FindDir(name)) == -1) || !FindName(name, file_name))
    {
        DEBUG('f', "OpenTest FindDir\n");
        return resFile;
    }

    openFile = new OpenFile(dir_sector);
    directory = new Directory(NumDirEntries);
    directory->FetchFrom(openFile);

    if ((sector = directory->Find(file_name)) != -1)
        resFile = new OpenFile(sector); // name was found in directory
    delete openFile;
    delete directory;
    return resFile; // return NULL if not found
}

bool FileSystem::RemoveTest(char *name, int cascade)
{
    Directory *directory;
    BitMap *freeMap;
    FileHeader *fileHdr;
    OpenFile *openFile;
    int sector;
    int dir_sector;
    char file_name[FileNameMaxLen + 1];

    if (((dir_sector = FindDir(name)) == -1) || !FindName(name, file_name))
    {
        DEBUG('f', "RemoveTest %s FindDir fasle\n", name);
        return false;
    }

    openFile = new OpenFile(dir_sector);
    directory = new Directory(NumDirEntries);
    directory->FetchFrom(openFile);
    if ((sector = directory->Find(file_name)) == -1)
    {
        DEBUG('f', "RemoveTest %s Find fasle\n", name);
        delete openFile;
        delete directory;
        return false; // file not found
    }
    if (directory->getType(file_name) == 0)
    {
        OpenFile *removeFile = new OpenFile(sector);
        Directory *removeDirectory = new Directory(NumDirEntries);
        removeDirectory->FetchFrom(removeFile);
        if (!removeDirectory->isEmpty())
        {
            if (cascade == 0)
            {
                DEBUG('f', "RemoveTest %s -r fasle\n", name);
                delete openFile;
                delete directory;
                delete removeFile;
                delete removeDirectory;
                return false;
            }
            int tableSize;
            DirectoryEntry *table;
            removeDirectory->getTable(tableSize, table);
            for (int i = 0; i < tableSize; i++)
                if (table[i].inUse && !RemoveTest(table[i].path, 1))
                {
                    DEBUG('f', "RemoveTest %s -r fasle\n", name);
                    delete openFile;
                    delete directory;
                    delete removeFile;
                    delete removeDirectory;
                    return false;
                }
        }
        delete removeFile;
        delete removeDirectory;
    }
    fileHdr = new FileHeader;
    fileHdr->FetchFrom(sector);

    freeMap = new BitMap(NumSectors);
    freeMap->FetchFrom(freeMapFile);

    fileHdr->Deallocate(freeMap); // remove data blocks
    freeMap->Clear(sector);       // remove header block
    directory->Remove(file_name);

    freeMap->WriteBack(freeMapFile); // flush to disk
    directory->WriteBack(openFile);  // flush to disk
    delete fileHdr;
    delete directory;
    delete freeMap;
    return true;
}

bool FileSystem::cd(char *name)
{
    OpenFile *openFile;
    Directory *directory;
    int sector;
    int dir_sector;
    char file_name[FileNameMaxLen + 1];

    // if (name[0] != '/')
    // {
    //     DEBUG('f', "cd name[0] != '/'\n");
    //     return false;
    // }
    if (!strcmp(name, "/"))
    {
        sector = 1;
        CurDir *curDir = new CurDir;
        curDir->sector = sector;
        strcpy(curDir->path, name);
        curDir->WriteBack(curDirFile);
        delete curDir;
        delete curDirectoryFile;
        curDirectoryFile = new OpenFile(sector);
        return true;
    }

    if (((dir_sector = FindDir(name)) == -1) || !FindName(name, file_name))
    {
        DEBUG('f', "cd %s FindDir fasle\n", name);
        return false;
    }
    openFile = new OpenFile(dir_sector);
    directory = new Directory(NumDirEntries);
    directory->FetchFrom(openFile);
    if ((sector = directory->Find(file_name)) == -1 || directory->getType(file_name))
    {
        DEBUG('f', "cd %s Find fasle\n", name);
        delete openFile;
        delete directory;
        return false;
    }

    //处理相对地址字符
    if (name[0] != '/')
    {
        DEBUG('f', "CreateTest name[0] != '/'\n");
        char tmp_file_name[FileNameMaxLen + 1];
        CurDir *curDir = new CurDir;
        curDir->FetchFrom(curDirFile);
        strcpy(tmp_file_name,curDir->path);
        if (!strcmp(tmp_file_name, "/"))
        {
            strcat(tmp_file_name, name);
            strcpy(name, tmp_file_name);
        }
        else
        {
            strcat(tmp_file_name, "/");
            strcat(tmp_file_name, name);
            strcpy(name, tmp_file_name);
        }
        delete curDir;
    }

    CurDir *curDir = new CurDir;
    curDir->sector = sector;
    strcpy(curDir->path, name);
    curDir->WriteBack(curDirFile);
    delete curDir;
    delete curDirectoryFile;
    curDirectoryFile = new OpenFile(sector);

    delete openFile;
    delete directory;
    return true;
}

void FileSystem::pwd()
{
    CurDir *curDir = new CurDir;
    curDir->FetchFrom(curDirFile);
    printf("curpath: %s\n", curDir->path);
}

void FileSystem::ListTest()
{
    Directory *directory = new Directory(NumDirEntries);

    directory->FetchFrom(curDirectoryFile);
    directory->List();
    delete directory;
}

FileSystem::FileSystem(bool format)
{
    DEBUG('f', "Initializing the file system.\n");
    if (format)
    {
        BitMap *freeMap = new BitMap(NumSectors);
        Directory *directory = new Directory(NumDirEntries);
        CurDir *curDir = new CurDir;
        FileHeader *mapHdr = new FileHeader;
        FileHeader *dirHdr = new FileHeader;
        FileHeader *curDirHdr = new FileHeader;

        DEBUG('f', "Formatting the file system.\n");

        // First, allocate space for FileHeaders for the directory and bitmap
        // (make sure no one else grabs these!)
        freeMap->Mark(FreeMapSector);
        freeMap->Mark(DirectorySector);
        freeMap->Mark(CurDirSector);

        // Second, allocate space for the data blocks containing the contents
        // of the directory and bitmap files.  There better be enough space!

        ASSERT(mapHdr->Allocate(freeMap, FreeMapFileSize));
        ASSERT(dirHdr->Allocate(freeMap, DirectoryFileSize));
        ASSERT(curDirHdr->Allocate(freeMap, CurDirFileSize));

        // Flush the bitmap and directory FileHeaders back to disk
        // We need to do this before we can "Open" the file, since open
        // reads the file header off of disk (and currently the disk has garbage
        // on it!).

        DEBUG('f', "Writing headers back to disk.\n");
        mapHdr->WriteBack(FreeMapSector);
        dirHdr->WriteBack(DirectorySector);
        curDirHdr->WriteBack(CurDirSector);

        // OK to open the bitmap and directory files now
        // The file system operations assume these two files are left open
        // while Nachos is running.

        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);
        curDirFile = new OpenFile(CurDirSector);

        // Once we have the files "open", we can write the initial version
        // of each file back to disk.  The directory at this point is completely
        // empty; but the bitmap has been changed to reflect the fact that
        // sectors on the disk have been allocated for the file headers and
        // to hold the file data for the directory and bitmap.

        DEBUG('f', "Writing bitmap and directory back to disk.\n");
        freeMap->WriteBack(freeMapFile); // flush changes to disk
        directory->WriteBack(directoryFile);

        curDir->sector = 1;
        strcpy(curDir->path, "/");
        curDir->WriteBack(curDirFile);
        curDirectoryFile = new OpenFile(curDir->sector);

        if (DebugIsEnabled('f'))
        {
            freeMap->Print();
            directory->Print();
        }
        delete freeMap;
        delete directory;
        delete curDir;
        delete mapHdr;
        delete dirHdr;
        delete curDirHdr;
    }
    else
    {
        // if we are not formatting the disk, just open the files representing
        // the bitmap and directory; these are left open while Nachos is running
        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);
        curDirFile = new OpenFile(CurDirSector);

        CurDir *curDir = new CurDir;
        curDir->FetchFrom(curDirFile);
        curDirectoryFile = new OpenFile(curDir->sector);

        delete curDir;
    }
}

bool FileSystem::cat(char *name)
{
    Directory *directory;
    OpenFile *openFile;
    int sector;
    int dir_sector;
    char file_name[FileNameMaxLen + 1];

    DEBUG('f', "cat file %s\n", name);

    if (!strcmp(name, "/"))
    {
        Print();
        return true;
    }

    if (((dir_sector = FindDir(name)) == -1) || !FindName(name, file_name))
    {
        DEBUG('f', "cat FindDir\n");
        return false;
    }

    openFile = new OpenFile(dir_sector);
    directory = new Directory(NumDirEntries);
    directory->FetchFrom(openFile);

    if ((sector = directory->Find(file_name)) == -1)
    {
        delete openFile;
        delete directory;
        return false;
    }
    directory->cat(file_name);
    if (!directory->getType(file_name))
    {
        delete openFile;
        openFile = new OpenFile(sector);
        directory->FetchFrom(openFile);
        directory->Print();
    }
    delete openFile;
    delete directory;
    return true;
}