// exception.cc
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code: 从用户程序进入Nachos内核的入口点。有两种情况会导致控件从用户代码传输回此处：
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt". syscall——用户代码显式请求调用Nachos内核中的过程。现在，我们唯一支持的功能是“停止”。
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.   异常——用户代码做一些CPU无法处理的事情。例如，访问不存在的内存、算术错误等。
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere. 中断（也可能导致控制权从用户代码转移到Nachos内核）在其他地方处理。
//
// For now, this only handles the Halt() system call.
// Everything else core dumps. 目前，它只处理Halt（）系统调用。其他所有核心转储。
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"

extern void StartProcess(int spaceId);

void AdvancePC()
{
    machine->WriteRegister(PCReg, machine->ReadRegister(PCReg) + 4);
    machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
}
//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception. 进入Nachos内核的入口点。当用户程序正在执行时调用，或者执行系统调用，或者生成寻址或算术异常。
//
// 	For system calls, the following is the calling convention: 对于系统调用，以下是调用约定：
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2.
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever! 在返回之前别忘了增加pc。（否则您将永远循环进行同一个系统调用！
//
//	"which" is the kind of exception.  The list of possible exceptions
//	are in machine.h.
//----------------------------------------------------------------------

void ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if ((which == SyscallException) && (type == SC_Halt))
    {
        DEBUG('x', "thread:%s\tShutdown, initiated by user program.\n", currentThread->getName());
        interrupt->Halt();
    }
    else if ((which == SyscallException) && (type == SC_Exec))
    {
        // printf("Execute system call of Exec()\n");
        // DEBUG('x', "Execute system call of Exec()\n");
        char filename[50];
        int addr = machine->ReadRegister(4);
        int i = 0;
        do
        {
            //read filename from mainMemory
            machine->ReadMem(addr + i, 1, (int *)&filename[i]);
        } while (filename[i++] != '\0');
        // printf("Exec(%s):\n",filename);
        DEBUG('x', "thread:%s\tExec(%s):\n", currentThread->getName(), filename);

        if (filename[0] == 'l' && filename[1] == 's') //ls
        {
            DEBUG('x', "thread:%s\tFile(s) on Nachos DISK:\n", currentThread->getName());
            fileSystem->List();
            machine->WriteRegister(2, 127); //
            AdvancePC();
            return;
        }

        OpenFile *executable = fileSystem->OpenTest(filename);
        AddrSpace *space;

        if (executable == NULL)
        {
            printf("Unable to open file %s\n", filename);
            // return;

            ASSERT(false);
        }
        space = new AddrSpace(executable);

        Thread *thread = new Thread(filename);
        thread->Fork(StartProcess, space->getSpaceId());

        machine->WriteRegister(2, space->getSpaceId());
        AdvancePC();

        // currentThread->Yield();

        delete executable;
    }
    else if ((which == SyscallException) && (type == SC_Exit))
    {
        int ExitStatus = machine->ReadRegister(4);
        DEBUG('x', "thread:%s\tExit(%d):\n", currentThread->getName(), ExitStatus);
        currentThread->pcb->setExitStatus(ExitStatus);

        List *terminatedList = scheduler->getTerminatedList();
        if (currentThread->pcb->getExitStatus() == 99)
        {
            DEBUG('x', "thread:%s\tparent delete terminatedList:\n", currentThread->getName());
            Thread *thread;
            while ((thread = (Thread *)(terminatedList->Remove())) != NULL)
                delete thread;
        }

        currentThread->Finish();
        AdvancePC();
    }
    else if ((which == SyscallException) && (type == SC_Yield))
    {
        DEBUG('x', "thread:%s\tYield:\n", currentThread->getName());
        currentThread->Yield();
        AdvancePC();
    }
    else if ((which == SyscallException) && (type == SC_Join))
    {
        int spaceId = machine->ReadRegister(4);
        DEBUG('x', "thread:%s\tJoin %d:\n", currentThread->getName(), spaceId);
        currentThread->Join(spaceId);
        //返回 Joinee 的退出码 waitProcessExitCode
        machine->WriteRegister(2, currentThread->pcb->waitProcessExitCode);
        DEBUG('x', "waitProcessExitCode %d:\n", currentThread->pcb->waitProcessExitCode);
        AdvancePC();
    }
    else if ((which == SyscallException) && (type == SC_Create))
    {
        int base = machine->ReadRegister(4);
        int value;
        int count = 0;
        char *FileName = new char[128];
        do
        {
            machine->ReadMem(base + count, 1, &value);
            FileName[count] = *(char *)&value;
            count++;
        } while (*(char *)&value != '\0' && count < 128);
        if (!fileSystem->CreateTest(FileName, 0))
        {
            printf("thread:%s\tcreate file:%s failed!\n", currentThread->getName(), FileName);
            ASSERT(false);
        }
        else
            DEBUG('x', "thread:%s\tcreate file:%s success\n", currentThread->getName(), FileName);
        AdvancePC();
    }
    else if ((which == SyscallException) && (type == SC_Open))
    {
        int base = machine->ReadRegister(4);
        int value;
        int count = 0;
        char *FileName = new char[128];
        do
        {
            machine->ReadMem(base + count, 1, &value);
            FileName[count] = *(char *)&value;
            count++;
        } while (*(char *)&value != '\0' && count < 128);

        OpenFile *openfile;
        openfile = fileSystem->OpenTest(FileName);
        int fileId;
        if (openfile == NULL)
        {
            //file not existes, not found
            printf("thread:%s\topen file:%s failed!\n", currentThread->getName(), FileName);
            fileId = -1;
            ASSERT(false);
        }
        else
        {
            fileId = currentThread->pcb->addFile(openfile);
            DEBUG('x', "thread:%s\topen file:%s success\tfileId:%d\n", currentThread->getName(), FileName, fileId);
        }
        machine->WriteRegister(2, fileId);
        AdvancePC();
    }
    else if ((which == SyscallException) && (type == SC_Write))
    {
        int base = machine->ReadRegister(4);
        int size = machine->ReadRegister(5);
        //bytes written to file   
        int fileId = machine->ReadRegister(6); //fd   
        int value;
        int count = 0;

        if (fileId == 0)
        {
            printf("thread:%s\tstdin cannot write!\n", currentThread->getName());
            ASSERT(false);
        }

        char *buffer = new char[128];
        do
        {
            machine->ReadMem(base + count, 1, &value);
            buffer[count] = *(char *)&value;
            count++;
        } while (count < size);
        buffer[size] = '\0';

        if (fileId == 1)
            printf("thread:%s\toutput to stdout:%s\n", currentThread->getName(), buffer);
        else
        {
            OpenFile *openfile;
            if ((openfile = currentThread->pcb->getFile(fileId)) == NULL)
            {
                printf("thread:%s\tfileId:%d not open!\n", currentThread->getName(), fileId);
                ASSERT(false);
            }

            int res = openfile->Write(buffer, size);
            openfile->WriteBack();
            if (res != size)
            {
                printf("thread:%s\tfileId:%d write failed!\n", currentThread->getName(), fileId);
                ASSERT(false);
            }
            else
                DEBUG('x', "thread:%s\tfileId:%d write success\tlength:%d!\n", currentThread->getName(), fileId, size);
        }
        AdvancePC();
    }
    else if ((which == SyscallException) && (type == SC_Read))
    {
        int base = machine->ReadRegister(4);
        int size = machine->ReadRegister(5);
        //bytes written to file   
        int fileId = machine->ReadRegister(6); //fd   
        int value;
        int count = 0;

        if (fileId == 1)
        {
            printf("thread:%s\tstdout cannot read!\n", currentThread->getName());
            ASSERT(false);
        }

        char *buffer = new char[128];

        if (fileId == 0)
        {
            for (int i = 0; i < size; i++)
            {
                buffer[i] = getchar();
                machine->WriteMem(base + i, 1, buffer[i]);
            }
            buffer[size] = '\0';
            DEBUG('x', "thread:%s\tinput from stdin:%s\n", currentThread->getName(), buffer);
        }
        else
        {
            OpenFile *openfile;
            if ((openfile = currentThread->pcb->getFile(fileId)) == NULL)
            {
                printf("thread:%s\tfileId:%d not open!\n", currentThread->getName(), fileId);
                ASSERT(false);
            }

            int res = openfile->Read(buffer, size);
            buffer[size] = '\0';
            if (res != size)
            {
                printf("thread:%s\tfileId:%d read failed!\tres:%d\n", currentThread->getName(), fileId, res);
                ASSERT(false);
            }
            else
                DEBUG('x', "thread:%s\tfileId:%d read success\tlength:%d!\tcontent:%s\n", currentThread->getName(), fileId, size, buffer);

            for (int i = 0; i < size; i++)
                machine->WriteMem(base + i, 1, buffer[i]);
        }
        AdvancePC();
    }
    else if ((which == SyscallException) && (type == SC_Close))
    {
        int fileId = machine->ReadRegister(4);

        currentThread->pcb->releaseFile(fileId);
        DEBUG('x', "thread:%s\tfileId:%d close success\n", currentThread->getName(), fileId);
        AdvancePC();
    }
    else
    {
        printf("Unexpected user mode exception %d %d\n", which, type);
        ASSERT(FALSE);
    }
}
