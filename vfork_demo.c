
// File is copyrighted 2026, by Kjetil Hvalstrand
// under LGPL license.


#include <proto/dos.h>
#include <proto/exec.h>

#include <stdio.h>
#include <stdlib.h>

extern ULONG vfork();
extern void vforkExit(int retcode);


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
		vforkExit(0); // terminate vfork thread, and brevent system crash
	}
	else
	{
		printf("this is the parent, parent got child pid: %d from vfork\n",pid);
	}
	
	Printf("wait a bit....\n");
	vforkExit(0); // wait for all vfork threads, and clean up nice.

	DropInterface( (struct Interface *) IDebug);

}

