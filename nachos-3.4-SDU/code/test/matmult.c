/* matmult.c 
 *    Test program to do matrix multiplication on large arrays. 在大数组上做矩阵乘法的测试程序。
 *
 *    Intended to stress virtual memory system. 旨在强调虚拟存储系统。
 *
 *    Ideally, we could read the matrices off of the file system,
 *	and store the result back to the file system! 理想情况下，我们可以从文件系统中读取矩阵，并将结果存储回文件系统！
 */

#include "syscall.h"

#define Dim 20 /* sum total of the arrays doesn't fit in 
				* physical memory   数组的总和不适合物理内存                      
				*/

int A[Dim][Dim];
int B[Dim][Dim];
int C[Dim][Dim];

int main()
{
	int i, j, k;

	for (i = 0; i < Dim; i++) /* first initialize the matrices */
		for (j = 0; j < Dim; j++)
		{
			A[i][j] = i;
			B[i][j] = j;
			C[i][j] = 0;
		}

	for (i = 0; i < Dim; i++) /* then multiply them together */
		for (j = 0; j < Dim; j++)
			for (k = 0; k < Dim; k++)
				C[i][j] += A[i][k] * B[k][j];

	Exit(C[Dim - 1][Dim - 1]); /* and then we're done */
}
