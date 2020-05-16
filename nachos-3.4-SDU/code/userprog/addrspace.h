// addrspace.h
//	Data structures to keep track of executing user programs
//	(address spaces). 用于跟踪正在执行的用户程序（地址空间）的数据结构。
//
//	For now, we don't keep any information about address spaces.
//	The user level CPU state is saved and restored in the thread
//	executing the user program (see thread.h). 现在，我们不保留任何关于地址空间的信息，用户级CPU状态保存并还原在执行用户程序的线程中（参见thread.h）。
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"

#define UserStackSize 1024 // increase this as necessary!

class AddrSpace
{
public:
  AddrSpace(OpenFile *executable); // Create an address space,
                                   // initializing it with the program
                                   // stored in the file "executable" 创建一个地址空间，用存储在文件“executable”中的程序初始化它
  ~AddrSpace();                    // De-allocate an address space 取消分配地址空间

  void InitRegisters(); // Initialize user-level CPU registers,
                        // before jumping to user code 在跳转到用户代码之前，初始化用户级CPU寄存器

  void SaveState();    // Save/restore address space-specific 保存/还原特定地址空间
  void RestoreState(); // info on a context switch 上下文开关上的信息

  void Print();

private:
  TranslationEntry *pageTable; // Assume linear page table translation 现在假设线性页表翻译！
                               // for now!
  unsigned int numPages;       // Number of pages in the virtual
                               // address space 虚拟地址空间中的页数
};

#endif // ADDRSPACE_H
