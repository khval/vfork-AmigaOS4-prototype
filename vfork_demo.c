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

int main()
{
	int pid;

       IDebug = (struct DebugIFace *) GetInterface( SysBase, "debug", 1L, NULL);

	// vfork returns child threads (struct Task *) casted as int on success.
	// (pid is not used so often on AmigaOS, (struct Task *) is more useful.)
	//
	// returns 0 on failure..
	
	pid = vfork();

	if (pid == 0) // this is the child
	{
		printf("this is the child thread, saying hello..\n");
		Printf("dos printf works in child\n");
		vforkExit(0); // terminate vfork thread, and prevent system crash
	}
	else
	{
		printf("this is the parent, parent got child pid: %d from vfork\n",pid);
	}
	
	Printf("wait a bit....\n");
	vforkExit(0); // wait for all vfork threads, and clean up nice.

	DropInterface( (struct Interface *) IDebug);

}

