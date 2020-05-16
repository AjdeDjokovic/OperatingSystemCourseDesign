// directory.h
//	Data structures to manage a UNIX-like directory of file names. 用于管理类UNIX文件名目录的数据结构。
//
//      A directory is a table of pairs: <file name, sector #>,
//	giving the name of each file in the directory, and
//	where to find its file header (the data structure describing
//	where to find the file's data blocks) on disk. 目录是一个成对的表：<file name，sector#>，给出目录中每个文件的名称，以及在磁盘上找到文件头（描述在哪里找到文件数据块的数据结构）的位置。
//
//      We assume mutual exclusion is provided by the caller. 我们假设调用方提供互斥
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#ifndef DIRECTORY_H
#define DIRECTORY_H

#include "openfile.h"

#define FileNameMaxLen 9 // for simplicity, we assume \
                         // file names are <= 9 characters long 为了简单起见，我们假设文件名长度<=9个字符

#define FilePathMaxLen 40 // for simplicity, we assume \
                         // file names are <= 9 characters long 为了简单起见，我们假设文件名长度<=9个字符

#define DirType 0

#define FileType 1

// The following class defines a "directory entry", representing a file
// in the directory.  Each entry gives the name of the file, and where
// the file's header is to be found on disk. 下列类别定义directory entry，表示目录中的文件。每个条目都给出了文件名，以及文件头在磁盘上的位置。
//
// Internal data structures kept public so that Directory operations can
// access them directly. 内部数据结构保持公共，以便目录操作可以直接访问它们。

class DirectoryEntry
{
public:
  int inUse;                    // Is this directory entry in use? 这个directory entry正在使用吗？
  int sector;                    // Location on disk to find the
                                 //   FileHeader for this file 磁盘上查找此文件的文件头的位置
  char name[FileNameMaxLen + 1]; // Text name for file, with +1 for
                                 // the trailing '\0' 文件的文本名称，后面的“\0”带有+1

  int type;
  char path[FilePathMaxLen + 1];
};

// The following class defines a UNIX-like "directory".  Each entry in
// the directory describes a file, and where to find it on disk. 下面的类定义了一个类似UNIX的“目录”。目录中的每个条目描述一个文件，以及在磁盘上的位置。
//
// The directory data structure can be stored in memory, or on disk.
// When it is on disk, it is stored as a regular Nachos file. 目录数据结构可以存储在内存中，也可以存储在磁盘上。当目录数据结构在磁盘上时，它作为一个普通的Nachos文件存储。
//
// The constructor initializes a directory structure in memory; the
// FetchFrom/WriteBack operations shuffle the directory information
// from/to disk. 构造函数初始化内存中的目录结构；FetchFrom/WriteBack操作将目录信息从/洗牌到磁盘。

class Directory
{
public:
  Directory(int size); // Initialize an empty directory
                       // with space for "size" files
  ~Directory();        // De-allocate the directory

  void FetchFrom(OpenFile *file); // Init directory contents from disk
  void WriteBack(OpenFile *file); // Write modifications to
                                  // directory contents back to disk

  int Find(char *name); // Find the sector number of the
                        // FileHeader for file: "name"

  bool Add(char *name, int newSector); // Add a file name into the directory

  bool Remove(char *name); // Remove a file from the directory

  void List();  // Print the names of all the files
                //  in the directory
  void Print(); // Verbose print of the contents
                //  of the directory -- all the file
                //  names and their contents.
  int getType(char *name);

  bool isEmpty();

  bool AddTest(char *name, char *path, int newSector, int type);

  void getTable(int &tableSize, DirectoryEntry *&table);

  bool cat(char *name);

private:
  int tableSize;         // Number of directory entries
  DirectoryEntry *table; // Table of pairs:
                         // <file name, file header location>

  int FindIndex(char *name); // Find the index into the directory
                             //  table corresponding to "name"
};

class CurDir
{
public:
  CurDir();

  void FetchFrom(OpenFile *file);
  void WriteBack(OpenFile *file);

  int sector;
  char path[FilePathMaxLen];
};

#endif // DIRECTORY_H
