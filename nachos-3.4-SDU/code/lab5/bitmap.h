// bitmap.h
//	Data structures defining a bitmap -- an array of bits each of which
//	can be either on or off.定义位图的数据结构——一个位数组，每个位都可以打开或关闭。
//
//	Represented as an array of unsigned integers, on which we do
//	modulo arithmetic to find the bit we are interested in.表示为一个无符号整数数组，我们对其进行模运算以找到感兴趣的位。
//
//	The bitmap can be parameterized with with the number of bits being
//	managed. 位图可以用管理的位数参数化。
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef BITMAP_H
#define BITMAP_H

#include "copyright.h"
#include "utility.h"
#include "openfile.h"

// Definitions helpful for representing a bitmap as an array of integers 有助于将位图表示为整数数组的定义
#define BitsInByte 8
#define BitsInWord 32

// The following class defines a "bitmap" -- an array of bits,
// each of which can be independently set, cleared, and tested.下面的类定义了一个“位图”——一个位数组，每个位都可以独立设置、清除和测试。

//
// Most useful for managing the allocation of the elements of an array --
// for instance, disk sectors, or main memory pages.
// Each bit represents whether the corresponding sector or page is
// in use or free.对于管理数组元素（例如磁盘扇区或主内存页）的分配最有用。每一位表示对应的扇区或页是否正在使用或空闲。

class BitMap
{
public:
  BitMap(int nitems); // Initialize a bitmap, with "nitems" bits
      // initially, all bits are cleared. 初始化位图，最初使用“nitems”位，所有位都被清除。
  ~BitMap(); // De-allocate bitmap

  void Mark(int which);  // Set the "nth" bit
  void Clear(int which); // Clear the "nth" bit
  bool Test(int which);  // Is the "nth" bit set?
  int Find();            // Return the # of a clear bit, and as a side
      // effect, set the bit.
      // If no bits are clear, return -1. 返回一个清晰的位，作为副作用，设置该位。如果没有位是清除的，则返回-1。
  int NumClear(); // Return the number of clear bits

  void Print(); // Print contents of bitmap 打印位图内容

  // These aren't needed until FILESYS, when we will need to read and
  // write the bitmap to a file 直到文件系统需要将位图读写到文件中时，才需要这些
  void FetchFrom(OpenFile *file); // fetch contents from disk 从磁盘获取内容
  void WriteBack(OpenFile *file); // write contents to disk

private:
  int numBits;       // number of bits in the bitmap 位图中的位数
  int numWords;      // number of words of bitmap storage 位图存储字数
                     // (rounded up if numBits is not a
                     //  multiple of the number of bits in
                     //  a word) （如果numBits不是单词位数的倍数，则向上舍入）
  unsigned int *map; // bit storage 位存储
};

#endif // BITMAP_H
