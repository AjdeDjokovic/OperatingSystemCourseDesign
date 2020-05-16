/* halt.c
 *	Simple program to test whether running a user program works.
 *	
 *	Just do a "syscall" that shuts down the OS. 只要做一个关闭操作系统的“系统调用”。
 *
 * 	NOTE: for some reason, user programs with global data structures 
 *	sometimes haven't worked in the Nachos environment.  So be careful
 *	out there!  One option is to allocate data structures as 
 * 	automatics within a procedure, but if you do this, you have to
 *	be careful to allocate a big enough stack to hold the automatics! 由于某些原因，具有全局数据结构的用户程序有时无法在Nachos环境中工作。所以小心点！一种选择是将数据结构分配为过程中的自动化程序，但如果这样做，则必须小心分配足够大的堆栈来容纳自动化程序！
 */

#include "syscall.h"

int main()
{
    Yield();
    Yield();
    Exit(6);
}
