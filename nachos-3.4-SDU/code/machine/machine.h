// machine.h
//	Data structures for simulating the execution of user programs
//	running on top of Nachos. 用于模拟在Nachos上运行的用户程序的执行的数据结构。
//
//	User programs are loaded into "mainMemory"; to Nachos,
//	this looks just like an array of bytes.  Of course, the Nachos
//	kernel is in memory too -- but as in most machines these days,
//	the kernel is loaded into a separate memory region from user
//	programs, and accesses to kernel memory are not translated or paged. 用户程序被加载到“主内存”中；对Nachos来说，这看起来就像一个字节数组。当然，Nachos内核也在内存中——但是和现在大多数机器一样，内核被加载到一个与用户程序不同的内存区域中，对内核内存的访问不会被翻译或分页。
//
//	In Nachos, user programs are executed one instruction at a time,
//	by the simulator.  Each memory reference is translated, checked
//	for errors, etc. 在Nachos中，用户程序由模拟器一次执行一条指令。每个内存引用都被翻译、检查错误等。
//
//  DO NOT CHANGE -- part of the machine emulation
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef MACHINE_H
#define MACHINE_H

#include "copyright.h"
#include "utility.h"
#include "translate.h"
#include "disk.h"

// Definitions related to the size, and format of user memory 与用户内存大小和格式相关的定义

#define PageSize SectorSize // set the page size equal to \
							// the disk sector size, for  \
							// simplicity

#define NumPhysPages 64 //32
#define MemorySize (NumPhysPages * PageSize)
#define TLBSize 4 // if there is a TLB, make it small

enum ExceptionType
{
	NoException,		   // Everything ok!
	SyscallException,	   // A program executed a system call.
	PageFaultException,	   // No valid translation found
	ReadOnlyException,	   // Write attempted to page marked
						   // "read-only"
	BusErrorException,	   // Translation resulted in an
						   // invalid physical address
	AddressErrorException, // Unaligned reference or one that
						   // was beyond the end of the
						   // address space 未对齐的引用或超出地址空间末尾的引用
	OverflowException,	   // Integer overflow in add or sub.
	IllegalInstrException, // Unimplemented or reserved instr.

	NumExceptionTypes
};

// User program CPU state.  The full set of MIPS registers, plus a few
// more because we need to be able to start/stop a user program between
// any two instructions (thus we need to keep track of things like load
// delay slots, etc.) 用户程序CPU状态。全套MIPS寄存器，再加上一些，因为我们需要能够在任意两条指令之间启动/停止用户程序（因此我们需要跟踪诸如加载延迟时隙等）

#define StackReg 29	  // User's stack pointer
#define RetAddrReg 31 // Holds return address for procedure calls
#define NumGPRegs 32  // 32 general purpose registers on MIPS
#define HiReg 32	  // Double register to hold multiply result
#define LoReg 33
#define PCReg 34		// Current program counter
#define NextPCReg 35	// Next program counter (for branch delay) 下一个程序计数器（用于分支延迟）
#define PrevPCReg 36	// Previous program counter (for debugging) 上一个程序计数器（用于调试）
#define LoadReg 37		// The register target of a delayed load. 延迟加载的寄存器目标。
#define LoadValueReg 38 // The value to be loaded by a delayed load. 延迟加载要加载的值。
#define BadVAddrReg 39	// The failing virtual address on an exception 异常上的失败虚拟地址

#define NumTotalRegs 40

// The following class defines an instruction, represented in both
// 	undecoded binary form 下面的类定义了一个指令，它以两种未编码的二进制形式表示
//      decoded to identify 解码以识别
//	    operation to do
//	    registers to act on
//	    any immediate operand value  任何立即数操作数值

class Instruction
{
public:
	void Decode(); // decode the binary representation of the instruction

	unsigned int value; // binary representation of the instruction

	char opCode;	 // Type of instruction.  This is NOT the same as the
					 // opcode field from the instruction: see defs in mips.h
	char rs, rt, rd; // Three registers from instruction.
	int extra;		 // Immediate or target or shamt field or offset.
					 // Immediates are sign-extended.
};

// The following class defines the simulated host workstation hardware, as
// seen by user programs -- the CPU registers, main memory, etc.
// User programs shouldn't be able to tell that they are running on our
// simulator or on the real hardware, except
//	we don't support floating point instructions
//	the system call interface to Nachos is not the same as UNIX
//	  (10 system calls in Nachos vs. 200 in UNIX!)
// If we were to implement more of the UNIX system calls, we ought to be
// able to run Nachos on top of Nachos! 下面的类定义了用户程序所看到的模拟主机工作站硬件——CPU寄存器、主内存等。用户程序不应该知道它们是在我们的模拟器上运行还是在实际硬件上运行，除非我们不支持浮点指令，否则Nachos的系统调用接口与UNIX不同（Nachos中有10个系统调用，UNIX中有200个！）如果我们要实现更多的UNIX系统调用，我们应该能够在Nachos之上运行Nachos！
//
// The procedures in this class are defined in machine.cc, mipssim.cc, and
// translate.cc.

class Machine
{
public:
	Machine(bool debug); // Initialize the simulation of the hardware
						 // for running user programs
	~Machine();			 // De-allocate the data structures

	// Routines callable by the Nachos kernel
	void Run(); // Run a user program

	int ReadRegister(int num); // read the contents of a CPU register

	void WriteRegister(int num, int value);
	// store a value into a CPU register

	// Routines internal to the machine simulation -- DO NOT call these 机器模拟内部的例程--不要调用这些

	void OneInstruction(Instruction *instr);
	// Run one instruction of a user program. 运行用户程序的一条指令。
	void DelayedLoad(int nextReg, int nextVal);
	// Do a pending delayed load (modifying a reg) 执行挂起的延迟加载（修改寄存器）

	bool ReadMem(int addr, int size, int *value);
	bool WriteMem(int addr, int size, int value);
	// Read or write 1, 2, or 4 bytes of virtual
	// memory (at addr).  Return FALSE if a
	// correct translation couldn't be found. 读取或写入1、2或4字节的虚拟内存（位于addr）。如果找不到正确的转换，则返回FALSE。

	ExceptionType Translate(int virtAddr, int *physAddr, int size, bool writing);
	// Translate an address, and check for
	// alignment.  Set the use and dirty bits in
	// the translation entry appropriately,
	// and return an exception code if the
	// translation couldn't be completed. 转换一个地址，并检查是否对齐。在转换项中适当设置use和dirty位，如果转换无法完成，则返回异常代码。

	void RaiseException(ExceptionType which, int badVAddr);
	// Trap to the Nachos kernel, because of a
	// system call or other exception. 由于系统调用或其他异常，陷入到Nachos内核。

	void Debugger();  // invoke the user program debugger 调用用户程序调试器
	void DumpState(); // print the user CPU and memory state 打印用户CPU和内存状态

	// Data structures -- all of these are accessible to Nachos kernel code. 数据结构——所有这些都可以由Nachos内核代码访问。
	// "public" for convenience. 为了方便而公开。
	//
	// Note that *all* communication between the user program and the kernel
	// are in terms of these data structures. 注意，用户程序和内核之间的*all*通信是根据这些数据结构进行的。

	char *mainMemory;			 // physical memory to store user program,
								 // code and data, while executing
	int registers[NumTotalRegs]; // CPU registers, for executing user programs

	// NOTE: the hardware translation of virtual addresses in the user program
	// to physical addresses (relative to the beginning of "mainMemory")
	// can be controlled by one of: 用户程序中虚拟地址到物理地址的硬件转换（相对于“主存储器”的开头）可以由以下之一控制：
	//	a traditional linear page table
	//  	a software-loaded translation lookaside buffer (tlb) -- a cache of
	//	  mappings of virtual page #'s to physical page #'s 软件加载的翻译查询缓冲区（tlb）——虚拟页到物理页映射的缓存
	//
	// If "tlb" is NULL, the linear page table is used
	// If "tlb" is non-NULL, the Nachos kernel is responsible for managing
	//	the contents of the TLB.  But the kernel can use any data structure 如果“tlb”不为空，那么Nachos内核负责管理tlb的内容。但是内核可以使用它想要的任何数据结构（例如分段分页）来处理TLB缓存未命中。
	//	it wants (eg, segmented paging) for handling TLB cache misses.
	//
	// For simplicity, both the page table pointer and the TLB pointer are
	// public.  However, while there can be multiple page tables (one per address
	// space, stored in memory), there is only one TLB (implemented in hardware).
	// Thus the TLB pointer should be considered as *read-only*, although
	// the contents of the TLB are free to be modified by the kernel software. 为了简单起见，页表指针和TLB指针都是公共的。然而，虽然可以有多个页表（每个地址空间一个，存储在内存中），但只有一个TLB（在硬件中实现）。因此，TLB指针应被视为*只读*，尽管TLB的内容可以由内核软件自由修改。

	TranslationEntry *tlb; // this pointer should be considered
						   // "read-only" to Nachos kernel code 这个指针应该被认为是Nachos内核代码的“只读”

	TranslationEntry *pageTable;
	unsigned int pageTableSize;

private:
	bool singleStep;  // drop back into the debugger after each
					  // simulated instruction 在每一条模拟指令完成后，返回到调试器中
	int runUntilTime; // drop back into the debugger when simulated  
					  // time reaches this value 当模拟时间达到此值时，返回到调试器    执行多少条指令后中断   到这个时间调试
};

extern void ExceptionHandler(ExceptionType which);
// Entry point into Nachos for handling
// user system calls and exceptions
// Defined in exception.cc 进入Nachos的入口点，用于处理用户系统调用和异常

// Routines for converting Words and Short Words to and from the
// simulated machine's format of little endian.  If the host machine
// is little endian (DEC and Intel), these end up being NOPs. 用于将Word和短Word转换为模拟机器的小尾数格式的例程。如果主机是little endian（DEC和Intel），那么这些最终将成为NOPs。
//
// What is stored in each format: 以每种格式存储的内容：
//	host byte ordering: 主机字节顺序：
//	   kernel data structures 核心数据结构
//	   user registers 用户寄存器
//	simulated machine byte ordering: 模拟机器字节排序：
//	   contents of main memory 主存内容

unsigned int WordToHost(unsigned int word);
unsigned short ShortToHost(unsigned short shortword);
unsigned int WordToMachine(unsigned int word);
unsigned short ShortToMachine(unsigned short shortword);

#endif // MACHINE_H
