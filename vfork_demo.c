
#include <proto/dos.h>
#include <proto/exec.h>

extern ULONG vfork();
extern ULONG vforkExit();


struct DebugIFace * IDebug = NULL;

int main()
{
	int pid;

       IDebug = (struct DebugIFace *) GetInterface( SysBase, "debug", 1L, NULL);

	// vfork returns child threads (struct Task *) on success.
	// (pid is not used so often on AmigaOS, (struct Task *) is more useful.)
	//
	// returns 0 on failure..
	
	pid = vfork();

	if (pid == 0) // this is the child
	{
		DebugPrintF("work...\n");
		Printf("worked\n");
		vforkExit(0); // terminate vfork thread, and brevent system crash
	}
	else
	{
		DebugPrintF("this is the parent\n");
	}
	
	Printf("wait a bit....\n");
	vforkExit(0); // wait for all vfork threads, and clean up nice.

	DropInterface( (struct Interface *) IDebug);

}

