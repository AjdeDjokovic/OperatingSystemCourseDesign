// synchdisk.h
// 	Data structures to export a synchronous interface to the raw
//	disk device.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#ifndef SYNCHDISK_H
#define SYNCHDISK_H

#include "disk.h"
#include "synch.h"

// The following class defines a "synchronous" disk abstraction.
// As with other I/O devices, the raw physical disk is an asynchronous device --
// requests to read or write portions of the disk return immediately,
// and an interrupt occurs later to signal that the operation completed.
// (Also, the physical characteristics of the disk device assume that
// only one operation can be requested at a time). 下面的类定义了一个“同步”磁盘抽象。与其他I/O设备一样，原始物理磁盘是一个异步设备——请求读取或写入磁盘的一部分会立即返回，并且稍后会发生中断，以表示操作已完成。（同样，磁盘设备的物理特性假设只有一个可以一次请求操作）。
//
// This class provides the abstraction that for any individual thread
// making a request, it waits around until the operation finishes before
// returning.这个类提供了一个抽象，对于发出请求的任何单个线程，它都会一直等到操作完成后再返回。
class SynchDisk
{
public:
  SynchDisk(char *name); // Initialize a synchronous disk,
                         // by initializing the raw Disk.通过初始化原始磁盘来初始化同步磁盘。
  ~SynchDisk();          // De-allocate the synch disk data

  void ReadSector(int sectorNumber, char *data);
  // Read/write a disk sector, returning
  // only once the data is actually read
  // or written.  These call
  // Disk::ReadRequest/WriteRequest and
  // then wait until the request is done.读/写磁盘扇区，仅在实际读或写数据时返回。这些调用Disk：：ReadRequest/WriteRequest，然后等待请求完成。
  void WriteSector(int sectorNumber, char *data);

  void RequestDone(); // Called by the disk device interrupt
                      // handler, to signal that the
                      // current disk operation is complete.由磁盘设备中断处理程序调用，以表示当前磁盘操作已完成。

private:
  Disk *disk;           // Raw disk device
  Semaphore *semaphore; // To synchronize requesting thread
                        // with the interrupt handler 将请求线程与中断句柄同步
  Lock *lock;           // Only one read/write request
                        // can be sent to the disk at a time 一次只能向磁盘发送一个读/写请求
};

#endif // SYNCHDISK_H
