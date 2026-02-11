
#include <proto/dos.h>
#include <proto/exec.h>

extern ULONG vfork();


struct DebugIFace * IDebug = NULL;

int main()
{
	int pid;

       IDebug = (struct DebugIFace *) GetInterface( SysBase, "debug", 1L, NULL);


	pid = vfork();

	if (pid == 0) // this is the child
	{
		DebugPrintF("work...\n");
		Printf("worked\n");
		_exit(0);
	}
	else
	{
		DebugPrintF("this is the parent\n");
	}
	
	Printf("wait a bit....\n");
	Delay(50);

	DropInterface( (struct Interface *) IDebug);

}

