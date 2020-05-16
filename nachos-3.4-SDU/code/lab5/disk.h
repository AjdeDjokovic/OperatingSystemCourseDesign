// disk.h
//	Data structures to emulate a physical disk.  A physical disk
//	can accept (one at a time) requests to read/write a disk sector;
//	when the request is satisfied, the CPU gets an interrupt, and
//	the next request can be sent to the disk.
//
//	Disk contents are preserved across machine crashes, but if
//	a file system operation (eg, create a file) is in progress when the
//	system shuts down, the file system may be corrupted.
//
//  DO NOT CHANGE -- part of the machine emulation
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef DISK_H
#define DISK_H

#include "copyright.h"
#include "utility.h"

// The following class defines a physical disk I/O device.  The disk
// has a single surface, split up into "tracks", and each track split
// up into "sectors" (the same number of sectors on each track, and each
// sector has the same number of bytes of storage).  以下类定义物理磁盘I/O设备。磁盘只有一个表面，分成“磁道”，每个磁道分成“扇区”（每个磁道上的扇区数相同，每个扇区的存储字节数相同）。
//
// Addressing is by sector number -- each sector on the disk is given
// a unique number: track * SectorsPerTrack + offset within a track.按扇区号寻址——磁盘上的每个扇区都有一个唯一的编号：磁道*扇区机架+磁道内的偏移量。
//
// As with other I/O devices, the raw physical disk is an asynchronous device --
// requests to read or write portions of the disk return immediately,
// and an interrupt is invoked later to signal that the operation completed.与其他I/O设备一样，原始物理磁盘是一个异步设备——请求立即返回磁盘的读或写部分，稍后调用中断来表示操作已完成。
//
// The physical disk is in fact simulated via operations on a UNIX file.物理磁盘实际上是通过对UNIX文件的操作来模拟的。
//
// To make life a little more realistic, the simulated time for
// each operation reflects a "track buffer" -- RAM to store the contents
// of the current track as the disk head passes by.  The idea is that the
// disk always transfers to the track buffer, in case that data is requested
// later on.  This has the benefit of eliminating the need for
// "skip-sector" scheduling -- a read request which comes in shortly after
// the head has passed the beginning of the sector can be satisfied more
// quickly, because its contents are in the track buffer.  Most
// disks these days now come with a track buffer.为了让生活更真实一些，每个操作的模拟时间反映了一个“磁道缓冲区”——RAM，当磁盘磁头经过时存储当前磁道的内容。其思想是磁盘总是传输到磁道缓冲区，以防以后请求数据。这样做的好处是消除了“跳过扇区”调度的需要——在磁头通过扇区开头后不久发出的读取请求可以更快地得到满足，因为它的内容在磁道缓冲区中。现在大多数磁盘都有一个磁道缓冲区
//
// The track buffer simulation can be disabled by compiling with -DNOTRACKBUF

#define SectorSize 128     // number of bytes per disk sector 每个磁盘扇区的字节数
#define SectorsPerTrack 32 // number of sectors per disk track 每个磁盘磁道的扇区数
#define NumTracks 32       // number of tracks per disk 每个磁盘的磁道数
#define NumSectors (SectorsPerTrack * NumTracks)
// total # of sectors per disk

class Disk
{
public:
  Disk(char *name, VoidFunctionPtr callWhenDone, _int callArg);
  // Create a simulated disk.
  // Invoke (*callWhenDone)(callArg)
  // every time a request completes.创建一个模拟磁盘。每次请求完成时调用（*callWhenDone）（callArg）
  ~Disk(); // Deallocate the disk.

  void ReadRequest(int sectorNumber, char *data);
  // Read/write an single disk sector.
  // These routines send a request to
  // the disk and return immediately.
  // Only one request allowed at a time!读/写单个磁盘扇区。这些例程向磁盘发送请求并立即返回。一次只允许一个请求！
  void WriteRequest(int sectorNumber, char *data);

  void HandleInterrupt(); // Interrupt handler, invoked when
                          // disk request finishes.中断处理程序，在磁盘请求完成时调用。

  int ComputeLatency(int newSector, bool writing);
  // Return how long a request to
  // newSector will take:
  // (seek + rotational delay + transfer)返回对newSector的请求需要多长时间：（seek+旋转延迟+传输）

private:
  int fileno;              // UNIX file number for simulated disk
  VoidFunctionPtr handler; // Interrupt handler, to be invoked
                           // when any disk request finishes 中断处理程序，在任何磁盘请求完成时调用
  _int handlerArg;         // Argument to interrupt handler
  bool active;             // Is a disk operation in progress?正在进行磁盘操作吗？
  int lastSector;          // The previous disk request上一个磁盘请求
  int bufferInit;          // When the track buffer started
                           // being loaded 当开始加载磁道缓冲区时

  int TimeToSeek(int newSector, int *rotate); // time to get to the new track 是时候走上新的磁道了
  int ModuloDiff(int to, int from);           // # sectors between to and from
  void UpdateLast(int newSector);
};

#endif // DISK_H
