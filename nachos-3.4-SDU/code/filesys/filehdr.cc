// filehdr.cc
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector,
//
//      Unlike in a real system, we do not keep track of file permissions,
//	ownership, last modification date, etc., in the file header.
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "filehdr.h"

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

FileHeader::FileHeader()
{
    numBytes = 0;                       //文件大小
    numSectors = 0;                     //文件扇区数
    for (int i = 0; i < NumDirect; i++) // NumDirector=30：文件最多拥有的扇区数
        dataSectors[i] = 0;
}

bool FileHeader::Allocate(BitMap *freeMap, int fileSize)
{
    numBytes = fileSize;
    numSectors = divRoundUp(fileSize, SectorSize);
    if (freeMap->NumClear() < numSectors)
        return FALSE; // not enough space

    for (int i = 0; i < numSectors; i++)
        dataSectors[i] = freeMap->Find();
    return TRUE;
}

bool FileHeader::Allocate(BitMap *freeMap, int fileSize, int incrementBytes)
{
    if (numSectors > 30) //限定每个文件最多可分配 30 个扇区
        return false;    //超出限定的文件大小
    if ((fileSize == 0) && (incrementBytes > 0))
    {                                //在一个空文件后追加数据
        if (freeMap->NumClear() < 1) //至少需要一个扇区块
            return false;            //磁盘已满，无空闲扇区可分配
        //为添加数据先分配一个空闲磁盘块，并更新文件头信息
        dataSectors[0] = freeMap->Find();
        numSectors = 1;
        numBytes = 0;
    }
    numBytes = fileSize;
    int offset = numBytes % SectorSize; //原文件最后一个扇区块数据偏移量
    int newSectorBytes = incrementBytes - (SectorSize - (offset + 1));
    //最后一个扇区块剩余空间足以容纳追加数据, 不需分配新的扇区块
    if (newSectorBytes <= 0)
    {
        numBytes = numBytes + incrementBytes; //更新文件头中的文件大小
        return TRUE;
    }
    //最后一个扇区的剩余空间不足以容纳要写入的数据，分配新的磁盘块
    int moreSectors = divRoundUp(newSectorBytes, SectorSize); //新加扇区块数
    if (numSectors + moreSectors > 30)
        return FALSE;                      //文件过大，超出 30 个磁盘块
    if (freeMap->NumClear() < moreSectors) //磁盘无足够的空闲块
        return false;
    //没有超出文件大小的限制，并且磁盘有足够的空闲块
    for (int i = numSectors; i < numSectors + moreSectors; i++)
        dataSectors[i] = freeMap->Find();
    numBytes = numBytes + incrementBytes; //更新文件大小
    numSectors = numSectors + moreSectors; //更新文件扇区块数
    return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void FileHeader::Deallocate(BitMap *freeMap)
{
    for (int i = 0; i < numSectors; i++)
    {
        ASSERT(freeMap->Test((int)dataSectors[i])); // ought to be marked!
        freeMap->Clear((int)dataSectors[i]);
    }
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk.
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void FileHeader::FetchFrom(int sector)
{
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk.
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void FileHeader::WriteBack(int sector)
{
    synchDisk->WriteSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int FileHeader::ByteToSector(int offset)
{
    return (dataSectors[offset / SectorSize]);
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header. 打印文件头的内容，以及文件头指向的所有数据块的内容。
//----------------------------------------------------------------------

void FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];

    printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
    for (i = 0; i < numSectors; i++)
        printf("%d ", dataSectors[i]);
    printf("\nFile contents:\n");
    for (i = k = 0; i < numSectors; i++)
    {
        synchDisk->ReadSector(dataSectors[i], data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++)
        {
            if ('\040' <= data[j] && data[j] <= '\176') // isprint(data[j])
                printf("%c", data[j]);
            else
                printf("\\%x", (unsigned char)data[j]);
        }
        printf("\n");
    }
    delete[] data;
}
