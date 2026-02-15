/*
 * Copyright (c) 2026 Kjetil Hvalstrand. All rights reserved.
 *
 * Denne koden er lisensiert under BSD 3-Clause License.
 * Se https://opensource.org for detaljer.
 */

#include <proto/dos.h>
#include <proto/exec.h>

#include <stdio.h>
#include <stdlib.h>

extern int vfork();
extern void vforkExit(int retcode);

/********************************************
	vfork() 
	returns pid of child on success.
	return 0 if its the child.
	return -1 if it failes to create a thread.
*********************************************/

struct DebugIFace * IDebug = NULL;

	int pid[10];

#undef debug
#define debug 1



int main()
{
	int n;
	int _pid;

#if debug
	IDebug = (struct DebugIFace *) GetInterface( SysBase, "debug", 1L, NULL);
#endif

	// vfork returns child threads (struct Task *) casted as int on success.
	// (pid is not used so often on AmigaOS, (struct Task *) is more useful.)
	//
	// returns 0 on failure..
	
	for (n=0;n<10;n++)
	{
		printf("-- top of for loop --\n");

		_pid = vfork();

		printf("task %p, n is %d, address of %p\n",FindTask(NULL),n, &n);

//		pid[n]=_pid;

		switch (_pid) // this is the child
		{
			case 0:

				printf("this is the child thread, saying hello..\n");
				Delay(2);
				Printf("dos printf works in child\n");
				vforkExit(0); // terminate vfork thread, and prevent system crash

			case -1:
				printf("vfork failed, no thread created\n");
				break;

			default:
				printf("this is the parent, parent got child pid: %d from vfork\n",_pid);
				break;
		}
	}
	
	Printf("wait a bit....\n");
	vforkExit(0); // wait for all vfork threads, and clean up nice.

#ifdef debug
	DropInterface( (struct Interface *) IDebug);
#endif

}

