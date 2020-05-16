// disk.cc
//	Routines to simulate a physical disk device; reading and writing
//	to the disk is simulated as reading and writing to a UNIX file.
//	See disk.h for details about the behavior of disks (and
//	therefore about the behavior of this simulation).用于模拟物理磁盘设备的例程；对磁盘的读写模拟为对UNIX文件的读写。有关磁盘行为的详细信息（以及有关此模拟行为的详细信息），请参阅disk.h。
//
//	Disk operations are asynchronous, so we have to invoke an interrupt
//	handler when the simulated operation completes.磁盘操作是异步的，因此我们必须在模拟操作完成时调用中断处理程序。
//
//  DO NOT CHANGE -- part of the machine emulation 不更改--机器仿真的一部分
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "disk.h"
#include "system.h"

// We put this at the front of the UNIX file representing the
// disk, to make it less likely we will accidentally treat a useful file
// as a disk (which would probably trash the file's contents).我们将这个放在表示磁盘的UNIX文件的前面，这样就不太可能意外地将有用的文件当作磁盘（这可能会破坏文件的内容）。
#define MagicNumber 0x456789ab
#define MagicSize sizeof(int)

#define DiskSize (MagicSize + (NumSectors * SectorSize))

// dummy procedure because we can't take a pointer of a member function伪过程，因为我们不能获取成员函数的指针
static void DiskDone(_int arg) { ((Disk *)arg)->HandleInterrupt(); }

//----------------------------------------------------------------------
// Disk::Disk()
// 	Initialize a simulated disk.  Open the UNIX file (creating it
//	if it doesn't exist), and check the magic number to make sure it's
// 	ok to treat it as Nachos disk storage.初始化模拟磁盘。打开UNIX文件（如果它不存在，则创建它），并检查幻数以确保可以将其视为Nachos磁盘存储。
//
//	"name" -- text name of the file simulating the Nachos disk “name”--模拟Nachos磁盘的文件的文本名称
//	"callWhenDone" -- interrupt handler to be called when disk read/write
//	   request completes “callWhenDone”--磁盘读写请求完成时要调用的中断处理程序
//	"callArg" -- argument to pass the interrupt handler传递中断处理程序的参数
//----------------------------------------------------------------------

Disk::Disk(char *name, VoidFunctionPtr callWhenDone, _int callArg)
{
    int magicNum;
    int tmp = 0;

    DEBUG('d', "Initializing the disk, 0x%x 0x%x\n", callWhenDone, callArg);
    handler = callWhenDone;
    handlerArg = callArg;
    lastSector = 0;
    bufferInit = 0;

    fileno = OpenForReadWrite(name, FALSE);
    if (fileno >= 0)
    { // file exists, check magic number
        Read(fileno, (char *)&magicNum, MagicSize);
        ASSERT(magicNum == MagicNumber);
    }
    else
    { // file doesn't exist, create it
        fileno = OpenForWrite(name);
        magicNum = MagicNumber;
        WriteFile(fileno, (char *)&magicNum, MagicSize); // write magic number

        // need to write at end of file, so that reads will not return EOF
        Lseek(fileno, DiskSize - sizeof(int), 0);
        WriteFile(fileno, (char *)&tmp, sizeof(int));
    }
    active = FALSE;
}

//----------------------------------------------------------------------
// Disk::~Disk()
// 	Clean up disk simulation, by closing the UNIX file representing the
//	disk.
//----------------------------------------------------------------------

Disk::~Disk()
{
    Close(fileno);
}

//----------------------------------------------------------------------
// Disk::PrintSector()
// 	Dump the data in a disk read/write request, for debugging. 将数据转储到磁盘读/写请求中，以便进行调试。
//----------------------------------------------------------------------

static void
PrintSector(bool writing, int sector, char *data)
{
    int *p = (int *)data;

    if (writing)
        printf("Writing sector: %d\n", sector);
    else
        printf("Reading sector: %d\n", sector);
    for (unsigned int i = 0; i < (SectorSize / sizeof(int)); i++)
        printf("%x ", p[i]);
    printf("\n");
}

//----------------------------------------------------------------------
// Disk::ReadRequest/WriteRequest
// 	Simulate a request to read/write a single disk sector
//	   Do the read/write immediately to the UNIX file
//	   Set up an interrupt handler to be called later,
//	      that will notify the caller when the simulator says
//	      the operation has completed. 模拟一个读/写单个磁盘扇区的请求立即对UNIX文件进行读/写设置一个中断处理程序，稍后调用，当模拟器说操作已完成时，该处理程序将通知调用方。
//
//	Note that a disk only allows an entire sector to be read/written,
//	not part of a sector.
//
//	"sectorNumber" -- the disk sector to read/write
//	"data" -- the bytes to be written, the buffer to hold the incoming bytes 要写入的字节，保存传入字节的缓冲区
//----------------------------------------------------------------------

void Disk::ReadRequest(int sectorNumber, char *data)
{
    int ticks = ComputeLatency(sectorNumber, FALSE);

    ASSERT(!active); // only one request at a time
    ASSERT((sectorNumber >= 0) && (sectorNumber < NumSectors));

    DEBUG('d', "Reading from sector %d\n", sectorNumber);
    Lseek(fileno, SectorSize * sectorNumber + MagicSize, 0);
    Read(fileno, data, SectorSize);
    if (DebugIsEnabled('d'))
        PrintSector(FALSE, sectorNumber, data);

    active = TRUE;
    UpdateLast(sectorNumber);
    stats->numDiskReads++;
    interrupt->Schedule(DiskDone, (_int)this, ticks, DiskInt);
}

void Disk::WriteRequest(int sectorNumber, char *data)
{
    int ticks = ComputeLatency(sectorNumber, TRUE);

    ASSERT(!active);
    ASSERT((sectorNumber >= 0) && (sectorNumber < NumSectors));

    DEBUG('d', "Writing to sector %d\n", sectorNumber);
    Lseek(fileno, SectorSize * sectorNumber + MagicSize, 0);
    WriteFile(fileno, data, SectorSize);
    if (DebugIsEnabled('d'))
        PrintSector(TRUE, sectorNumber, data);

    active = TRUE;
    UpdateLast(sectorNumber);
    stats->numDiskWrites++;
    interrupt->Schedule(DiskDone, (_int)this, ticks, DiskInt);
}

//----------------------------------------------------------------------
// Disk::HandleInterrupt()
// 	Called when it is time to invoke the disk interrupt handler,
//	to tell the Nachos kernel that the disk request is done.在调用磁盘中断处理程序时调用，以告知Nachos内核磁盘请求已完成。
//----------------------------------------------------------------------

void Disk::HandleInterrupt()
{
    active = FALSE;
    (*handler)(handlerArg);
}

//----------------------------------------------------------------------
// Disk::TimeToSeek()
//	Returns how long it will take to position the disk head over the correct
//	track on the disk.  Since when we finish seeking, we are likely
//	to be in the middle of a sector that is rotating past the head,
//	we also return how long until the head is at the next sector boundary.返回将磁头放置在磁盘上正确磁道上所需的时间。因为当我们完成搜索时，我们很可能处于一个旋转过磁头的扇区的中间，我们还返回磁头到达下一个扇区边界的时间。
//
//   	Disk seeks at one track per SeekTime ticks (cf. stats.h)
//   	and rotates at one sector per RotationTime ticks磁盘在每个SeekTime ticks（参见stats.h）上寻找一个磁道，并在每个RotationTime ticks上旋转一个扇区
//----------------------------------------------------------------------

int Disk::TimeToSeek(int newSector, int *rotation)
{
    int newTrack = newSector / SectorsPerTrack;
    int oldTrack = lastSector / SectorsPerTrack;
    int seek = abs(newTrack - oldTrack) * SeekTime;
    // how long will seek take?
    int over = (stats->totalTicks + seek) % RotationTime;
    // will we be in the middle of a sector when
    // we finish the seek? 当我们完成搜寻时，我们会在一个区域的中间吗？

    *rotation = 0; //旋转等待时间
    if (over > 0)  // if so, need to round up to next full sector
        *rotation = RotationTime - over;
    return seek; //寻道时间
}

//----------------------------------------------------------------------
// Disk::ModuloDiff()
// 	Return number of sectors of rotational delay between target sector
//	"to" and current sector position "from"返回目标扇区“到”和当前扇区位置“从”之间的轮换延迟扇区数
//----------------------------------------------------------------------

int Disk::ModuloDiff(int to, int from) //转的块数
{
    int toOffset = to % SectorsPerTrack;
    int fromOffset = from % SectorsPerTrack;

    return ((toOffset - fromOffset) + SectorsPerTrack) % SectorsPerTrack;
}

//----------------------------------------------------------------------
// Disk::ComputeLatency()
// 	Return how long will it take to read/write a disk sector, from
//	the current position of the disk head.返回从磁盘头的当前位置读取/写入磁盘扇区需要多长时间。
//
//   	Latency = seek time + rotational latency + transfer time
//   	Disk seeks at one track per SeekTime ticks (cf. stats.h)
//   	and rotates at one sector per RotationTime ticks延迟=寻道时间+旋转延迟+传输时间。磁盘在每个SeekTime ticks（参见stats.h）上寻找一个磁道，并在每个RotationTime ticks上旋转一个扇区
//
//   	To find the rotational latency, we first must figure out where the
//   	disk head will be after the seek (if any).  We then figure out
//   	how long it will take to rotate completely past newSector after
//	that point.要找到旋转延迟，我们首先必须找出查找之后磁盘头的位置（如果有的话）。然后，我们计算出在这一点之后，完全旋转经过新闻采编器需要多长时间。
//
//   	The disk also has a "track buffer"; the disk continuously reads
//   	the contents of the current disk track into the buffer.  This allows
//   	read requests to the current track to be satisfied more quickly.
//   	The contents of the track buffer are discarded after every seek to
//   	a new track.磁盘还有一个“磁道缓冲区”；磁盘不断地将当前磁盘磁道的内容读入缓冲区。这样可以更快地满足对当前曲目的读取请求。每次搜索到新磁道后，都会丢弃磁道缓冲区的内容。
//----------------------------------------------------------------------

int Disk::ComputeLatency(int newSector, bool writing)//返回寻道时间 + 旋转等待时间 + 读取时间
{
    int rotation;
    int seek = TimeToSeek(newSector, &rotation);
    int timeAfter = stats->totalTicks + seek + rotation;

#ifndef NOTRACKBUF // turn this on if you don't want the track buffer stuff
    // check if track buffer applies 如果您不想使用track buffer，请启用此选项检查track buffer是否适用
    if ((writing == FALSE) && (seek == 0) && (((timeAfter - bufferInit) / RotationTime) > ModuloDiff(newSector, bufferInit / RotationTime)))
    {
        DEBUG('d', "Request latency = %d\n", RotationTime);
        return RotationTime; // time to transfer sector from the track buffer 从磁道缓冲器传送扇区的时间
    }
#endif

    rotation += ModuloDiff(newSector, timeAfter / RotationTime) * RotationTime;

    DEBUG('d', "Request latency = %d\n", seek + rotation + RotationTime);
    return (seek + rotation + RotationTime);
}

//----------------------------------------------------------------------
// Disk::UpdateLast
//   	Keep track of the most recently requested sector.  So we can know
//	what is in the track buffer.跟踪最近请求的扇区。这样我们就可以知道磁道缓冲器里有什么。
//----------------------------------------------------------------------

void Disk::UpdateLast(int newSector)
{
    int rotate;
    int seek = TimeToSeek(newSector, &rotate);

    if (seek != 0)
        bufferInit = stats->totalTicks + seek + rotate;
    lastSector = newSector;
    DEBUG('d', "Updating last sector = %d, %d\n", lastSector, bufferInit);
}
