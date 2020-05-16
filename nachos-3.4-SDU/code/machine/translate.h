// translate.h
//	Data structures for managing the translation from
//	virtual page # -> physical page #, used for managing
//	physical memory on behalf of user programs. 用于管理来自虚拟页#->物理页#的转换的数据结构，用于代表用户程序管理物理内存。
//
//	The data structures in this file are "dual-use" - they
//	serve both as a page table entry, and as an entry in
//	a software-managed translation lookaside buffer (TLB).
//	Either way, each entry is of the form: 此文件中的数据结构是“两用的”-它们既可以用作页表项，也可以用作软件管理的翻译查找缓冲区（TLB）中的项。无论哪种方式，每个项的形式都是：
//	<virtual page #, physical page #>.
//
// DO NOT CHANGE -- part of the machine emulation 不更改--机器模拟的一部分
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef TLB_H
#define TLB_H

#include "copyright.h"
#include "utility.h"

// The following class defines an entry in a translation table -- either
// in a page table or a TLB.  Each entry defines a mapping from one
// virtual page to one physical page.
// In addition, there are some extra bits for access control (valid and
// read-only) and some bits for usage information (use and dirty). 下面的类定义了转换表中的一个条目——在页表或TLB中。每个条目定义从一个虚拟页到一个物理页的映射。此外，还有一些额外的位用于访问控制（有效和只读）和一些位用于使用信息（使用和脏）。

class TranslationEntry
{
public:
  int virtualPage;  // The page number in virtual memory.
  int physicalPage; // The page number in real memory (relative to the
                    //  start of "mainMemory"
  bool valid;       // If this bit is set, the translation is ignored.
                    // (In other words, the entry hasn't been initialized.)
  bool readOnly;    // If this bit is set, the user program is not allowed
                    // to modify the contents of the page.
  bool use;         // This bit is set by the hardware every time the
                    // page is referenced or modified.
  bool dirty;       // This bit is set by the hardware every time the
                    // page is modified.
};

#endif
