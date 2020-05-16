/* noff.h 
 *     Data structures defining the Nachos Object Code Format 定义Nachos目标代码格式的数据结构
 *
 *     Basically, we only know about three types of segments:
 *	code (read-only), initialized data, and unitialized data 基本上，我们只知道三种类型的段代码（只读）、初始化数据和未初始化数据
 */

#define NOFFMAGIC	0xbadfad 	/* magic number denoting Nachos 
					 * object code file 
					 */

typedef struct segment {
  int virtualAddr;		/* location of segment in virt addr space  段在virt addr空间中的位置 */ 
  int inFileAddr;		/* location of segment in this file  段在文件中的位置 */
  int size;			/* size of segment */
} Segment;

typedef struct noffHeader {
   int noffMagic;		/* should be NOFFMAGIC */
   Segment code;		/* executable code segment */ 
   Segment initData;		/* initialized data segment */
   Segment uninitData;		/* uninitialized data segment --
				 * should be zero'ed before use 
				 */
} NoffHeader;
