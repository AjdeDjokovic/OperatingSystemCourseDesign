// bitmap.c
//	Routines to manage a bitmap -- an array of bits each of which
//	can be either on or off.  Represented as an array of integers. 管理位图的例程——一个位数组，每个位都可以打开或关闭。表示为整数数组。
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "bitmap.h"

//----------------------------------------------------------------------
// BitMap::BitMap
// 	Initialize a bitmap with "nitems" bits, so that every bit is clear.
//	it can be added somewhere on a list. 用“nitems”位初始化位图，以便每个位都是清晰的。它可以添加到列表中的某个位置。
//
//	"nitems" is the number of bits in the bitmap. “nitems”是位图中的位数。
//----------------------------------------------------------------------

BitMap::BitMap(int nitems)
{
    numBits = nitems;
    numWords = divRoundUp(numBits, BitsInWord);
    map = new unsigned int[numWords];
    for (int i = 0; i < numBits; i++)
        Clear(i);
}

//----------------------------------------------------------------------
// BitMap::~BitMap
// 	De-allocate a bitmap.
//----------------------------------------------------------------------

BitMap::~BitMap()
{
    delete map;
}

//----------------------------------------------------------------------
// BitMap::Set
// 	Set the "nth" bit in a bitmap.  在位图中设置“n”位。
//
//	"which" is the number of the bit to be set.   “which”是要设置的位号。
//----------------------------------------------------------------------

void BitMap::Mark(int which)
{
    ASSERT(which >= 0 && which < numBits);
    map[which / BitsInWord] |= 1 << (which % BitsInWord);
}

//----------------------------------------------------------------------
// BitMap::Clear
// 	Clear the "nth" bit in a bitmap.
//
//	"which" is the number of the bit to be cleared.
//----------------------------------------------------------------------

void BitMap::Clear(int which)
{
    ASSERT(which >= 0 && which < numBits);
    map[which / BitsInWord] &= ~(1 << (which % BitsInWord));
}

//----------------------------------------------------------------------
// BitMap::Test
// 	Return TRUE if the "nth" bit is set.
//
//	"which" is the number of the bit to be tested.
//----------------------------------------------------------------------

bool BitMap::Test(int which)
{
    ASSERT(which >= 0 && which < numBits);

    if (map[which / BitsInWord] & (1 << (which % BitsInWord)))
        return TRUE;
    else
        return FALSE;
}

//----------------------------------------------------------------------
// BitMap::Find
// 	Return the number of the first bit which is clear.
//	As a side effect, set the bit (mark it as in use).
//	(In other words, find and allocate a bit.) 返回清晰的第一个位的编号。作为副作用，设置该位（将其标记为正在使用中）。（换句话说，查找并分配一个位）
//
//	If no bits are clear, return -1.
//----------------------------------------------------------------------

int BitMap::Find()
{
    for (int i = 0; i < numBits; i++)
        if (!Test(i))
        {
            Mark(i);
            return i;
        }
    return -1;
}

//----------------------------------------------------------------------
// BitMap::NumClear
// 	Return the number of clear bits in the bitmap.
//	(In other words, how many bits are unallocated?)
//----------------------------------------------------------------------

int BitMap::NumClear()
{
    int count = 0;

    for (int i = 0; i < numBits; i++)
        if (!Test(i))
            count++;
    return count;
}

//----------------------------------------------------------------------
// BitMap::Print
// 	Print the contents of the bitmap, for debugging.
//
//	Could be done in a number of ways, but we just print the #'s of
//	all the bits that are set in the bitmap. 可以通过多种方式完成，但我们只打印位图中设置的所有位的#。
//----------------------------------------------------------------------

void BitMap::Print()
{
    printf("Bitmap set:\n");
    for (int i = 0; i < numBits; i++)
        if (Test(i))
            printf("%d, ", i);
    printf("\n");
}

// These aren't needed until the FILESYS assignment 在分配文件系统FILESYS之前不需要这些

//----------------------------------------------------------------------
// BitMap::FetchFromFile
// 	Initialize the contents of a bitmap from a Nachos file. 从Nachos文件初始化位图的内容。
//
//	"file" is the place to read the bitmap from   “文件”是从中读取位图的位置
//----------------------------------------------------------------------

void BitMap::FetchFrom(OpenFile *file)
{
    file->ReadAt((char *)map, numWords * sizeof(unsigned), 0);
}

//----------------------------------------------------------------------
// BitMap::WriteBack
// 	Store the contents of a bitmap to a Nachos file.
//
//	"file" is the place to write the bitmap to
//----------------------------------------------------------------------

void BitMap::WriteBack(OpenFile *file)
{
    file->WriteAt((char *)map, numWords * sizeof(unsigned), 0);
}
