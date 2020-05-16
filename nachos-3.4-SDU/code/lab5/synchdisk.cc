// synchdisk.cc
//	Routines to synchronously access the disk.  The physical disk
//	is an asynchronous device (disk requests return immediately, and
//	an interrupt happens later on).  This is a layer on top of
//	the disk providing a synchronous interface (requests wait until
//	the request completes). 同步访问磁盘的例程。物理磁盘是一个异步设备（磁盘请求立即返回，稍后会发生中断）。这是磁盘顶部的一层，提供同步接口（请求等待请求完成）。
//
//	Use a semaphore to synchronize the interrupt handlers with the
//	pending requests.  And, because the physical disk can only
//	handle one operation at a time, use a lock to enforce mutual
//	exclusion. 使用信号量将中断处理程序与挂起的请求同步。而且，因为物理磁盘一次只能处理一个操作，所以使用锁来强制互斥。
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synchdisk.h"

//----------------------------------------------------------------------
// DiskRequestDone
// 	Disk interrupt handler.  Need this to be a C routine, because
//	C++ can't handle pointers to member functions. 磁盘中断处理程序。这是C例程，因为C++无法处理指向成员函数的指针。
//----------------------------------------------------------------------

static void DiskRequestDone(_int arg)
{
    SynchDisk *dsk = (SynchDisk *)arg; // disk -> dsk

    dsk->RequestDone(); // disk -> dsk
}

//----------------------------------------------------------------------
// SynchDisk::SynchDisk
// 	Initialize the synchronous interface to the physical disk, in turn
//	initializing the physical disk. 初始化物理磁盘的同步接口，然后初始化物理磁盘。
//
//	"name" -- UNIX file name to be used as storage for the disk data
//	   (usually, "DISK")
//----------------------------------------------------------------------

SynchDisk::SynchDisk(char *name)
{
    semaphore = new Semaphore("synch disk", 0);
    lock = new Lock("synch disk lock");
    disk = new Disk(name, DiskRequestDone, (_int)this);
}

//----------------------------------------------------------------------
// SynchDisk::~SynchDisk
// 	De-allocate data structures needed for the synchronous disk
//	abstraction.
//----------------------------------------------------------------------

SynchDisk::~SynchDisk()
{
    delete disk;
    delete lock;
    delete semaphore;
}

//----------------------------------------------------------------------
// SynchDisk::ReadSector
// 	Read the contents of a disk sector into a buffer.  Return only
//	after the data has been read. 将磁盘扇区的内容读入缓冲区。仅在读取数据后返回。
//
//	"sectorNumber" -- the disk sector to read
//	"data" -- the buffer to hold the contents of the disk sector
//----------------------------------------------------------------------

void SynchDisk::ReadSector(int sectorNumber, char *data)
{
    lock->Acquire(); // only one disk I/O at a time
    disk->ReadRequest(sectorNumber, data);
    semaphore->P(); // wait for interrupt
    lock->Release();
}

//----------------------------------------------------------------------
// SynchDisk::WriteSector
// 	Write the contents of a buffer into a disk sector.  Return only
//	after the data has been written.
//
//	"sectorNumber" -- the disk sector to be written
//	"data" -- the new contents of the disk sector
//----------------------------------------------------------------------

void SynchDisk::WriteSector(int sectorNumber, char *data)
{
    lock->Acquire(); // only one disk I/O at a time
    disk->WriteRequest(sectorNumber, data);
    semaphore->P(); // wait for interrupt
    lock->Release();
}

//----------------------------------------------------------------------
// SynchDisk::RequestDone
// 	Disk interrupt handler.  Wake up any thread waiting for the disk
//	request to finish.
//----------------------------------------------------------------------

void SynchDisk::RequestDone()
{
    semaphore->V();
}
