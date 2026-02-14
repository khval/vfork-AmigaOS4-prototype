/*
 * Copyright (c) 2026 Kjetil Hvalstrand. All rights reserved.
 *
 * Denne koden er lisensiert under BSD 3-Clause License.
 * Se https://opensource.org for detaljer.
 */

#include <stdlib.h>

#include <proto/dos.h>
#include <proto/exec.h>

extern struct DebugIFace * IDebug;

ULONG vfork();
void vfork_end();

ULONG vfork_count = 0;

struct stack_info
{
	struct Process *proc;
	int cnt;
};

struct vforkcontext
{
	struct stack_info parent;
	struct stack_info child;
	int success;
};


int32 childStart(const char *arg ,int32 len ,struct ExecBase *base )
{
	Forbid();
	vfork_count++;
	Permit();

	Wait( SIGF_CHILD );

#if debug
	DebugPrintF("childStart() lives, and hopefully won't die\n");
#endif

	return 0;
}

void childStart_end()
{
}

VOID ChildFinalCode (int32 result1, int32 final_data, struct ExecBase *sysbase)
{
	Forbid();
	vfork_count--;
	Permit();

#if debug
	Forbid();
	DebugPrintF("ChildFinalCode\n");
	Permit();
#endif
}

#if debug
int32 printStack(struct Hook *hook, struct Task *task, struct StackFrameMsg *frame)
{
	struct DebugSymbol *symbol = NULL;

	struct stack_info *userdata = (struct stack_info *) ((struct Process *) FindTask(NULL)) -> pr_EntryData;

	switch (frame->State)
	{
	    case STACK_FRAME_DECODED:


		if (symbol = ObtainDebugSymbol(frame->MemoryAddress, NULL))
		{
			DebugPrintF("(%p) -> (%p) %s : %s\n", 
				frame->StackPointer, frame->MemoryAddress,
				symbol -> Name, 
				symbol->SourceFunctionName ? symbol->SourceFunctionName : "symbol not found");
			ReleaseDebugSymbol(symbol);
		}
		else
		{
			DebugPrintF("(%p) -> %p\n", frame->StackPointer, frame->MemoryAddress);
		}

		break;

	    case STACK_FRAME_INVALID_BACKCHAIN_PTR:
		DebugPrintF("(%p) invalid backchain pointer\n",  frame->StackPointer);
		break;

	    case STACK_FRAME_TRASHED_MEMORY_LOOP:
		DebugPrintF("(%p) trashed memory loop\n", frame->StackPointer);
	     	break;

	    case STACK_FRAME_BACKCHAIN_PTR_LOOP:
		DebugPrintF("(%p) backchain pointer loop\n",frame->StackPointer);
		break;

	    default:
		DebugPrintF("Unknown state=%lu\n", frame->State);
	}


	if (userdata -> cnt < 20 )
	{
		userdata -> cnt++;
		return 0;  // Continue tracing.
	}
	else	// stack too long /  corrupt.... die...
	{
		return 1;
	}
}

void dump_stack(struct stack_info *task_info)
{
	struct Task *task = ((struct Task *) task_info -> proc);
	DebugPrintF("Stack for %s:\n", task -> tc_Node.ln_Name);
	
	((struct Process *) FindTask(NULL)) -> pr_EntryData = (ULONG) task_info;

	struct Hook *hook = AllocSysObjectTags(ASOT_HOOK,ASOHOOK_Entry, printStack, TAG_END);

	if (hook != NULL)
	{
		task_info -> cnt = 0;	// reset stack frame count;

		StackTrace( task, hook);

		FreeSysObject(ASOT_HOOK, hook);
	}
}
#endif


static uint32 *find_frame(struct Task *t, APTR func_start, APTR func_end )
{
	int guard = 0;
	int vfork_size =  (ULONG) vfork_end - (ULONG) vfork;

	uint32 *sp = t -> tc_SPReg;

	while (sp && sp[0] && guard++ < 32)
	{
		uint32 lr = sp[1];

		if ( ((APTR) lr >=  (APTR) func_start) && ((APTR) lr <= (APTR) func_end) )
		{
			return (uint32 *) sp[0];
		}

		sp = (uint32 *)sp[0];
    }

    return NULL;
}

uint32 get_stack_size( uint32 *src )
{
	uint32 frame_size, tot_size = 0;

	while(src && src[0])
	{
    		tot_size += (uint32)src[0] - (uint32)src; // compute frame size
		src = (uint32 *) src[0];	
	}

	return tot_size;
}

static void copy_stack_frames( uint32 *src, uint32 *current , struct Process *child )
{
	uint32 *dst;
	uint32 offset = get_stack_size( src ) + 1024;

 	dst = (uint32 *) ((char *) child -> pr_Task.tc_SPReg - offset  );

	while(src && src[0])
	{
    		uint32 frame_size = (uint32)src[0] - (uint32)src; // compute frame size

#if debug
		DebugPrintF("copy frame_size: %d\n", frame_size );
#endif

  	  	CopyMem(src, dst, frame_size);     // copy the frame
   	 	current[0] = (uint32) dst; 		// save previus frame to current stack frame...

#if debug	    
		DebugPrintF( "%p:  prev frame %p, lr %p\n" , current, current[0], current[1]  );
#endif

		src = (uint32 *) src[0];
		current = dst;
		dst = (uint32 *) ((char *) dst + frame_size);
	}

	current[0] = 0;	// terminate stack..
}

static BOOL patch_child_return( struct Process *parent, struct Process *child)
{
	uint32 *parent_vfork = find_frame( (struct Task *) parent, (APTR) vfork, (APTR) vfork_end );
	uint32 *child_vfork  = find_frame( (struct Task *) child, (APTR) childStart, (APTR) childStart_end) ;
	uint32 this_frame_size;

	if (!parent_vfork || !child_vfork)
	{
		DebugPrintF("vfork or childStart frame not found\n");
		return FALSE;
	}

	uint32 return_to_main = parent_vfork[1];

	/* Patch LR */
	child_vfork[1] = return_to_main;

	// get stack pointer for before vfork(), should be stack for main() in this test...

	copy_stack_frames( (uint32 *)  parent_vfork[0] , (uint32 *) child_vfork[0] , child);

	return TRUE;
}

int32 childStackBuilderStart(const char *arg ,int32 len ,struct ExecBase *base )
{
	struct vforkcontext *userdata;
	struct Process *self;
	struct process_context *parent_context,*child_context;
	uint32 *parent_sp;
	uint32 *child_sp;

	self = (struct Process *) FindTask(NULL);
	userdata = (struct vforkcontext *)  self -> pr_EntryData;

	// wait for processes to wait...
	do
	{
		Delay(1);
	} while ( ! (( userdata -> child.proc  -> pr_Task.tc_SigWait & SIGF_CHILD ) && ( userdata -> parent.proc -> pr_Task.tc_SigWait & SIGF_CHILD )) );

	SuspendTask( (struct Task *) userdata -> child.proc, 0 );  // not needed will suspend it self..
	SuspendTask( (struct Task *) userdata -> parent.proc, 0 );

#if debug
	dump_stack(  &userdata -> child );
	dump_stack(  &userdata -> parent );
#endif

	userdata -> child.proc -> pr_Task.tc_SigAlloc = userdata -> parent.proc -> pr_Task.tc_SigAlloc;
	userdata -> child.proc -> pr_Task.tc_SigWait = userdata -> parent.proc -> pr_Task.tc_SigWait;
	userdata -> success = patch_child_return( userdata -> parent.proc, userdata -> child.proc);

#if debug
	dump_stack( &userdata -> child );
#endif

	parent_sp = userdata -> parent.proc -> pr_Task.tc_SPReg;
	child_sp = userdata -> child.proc -> pr_Task.tc_SPReg;

	child_sp[0] = parent_sp[0];	// dirty copy last stack frame from parent....
	child_sp[1] = parent_sp[1];	// copy LR

	RestartTask( (struct Task *) userdata -> child.proc, 0 );
	RestartTask( (struct Task *) userdata -> parent.proc, 0 );

	Signal( (struct Task *) userdata -> child.proc , SIGF_CHILD);
	Signal( (struct Task *) userdata -> parent.proc , SIGF_CHILD);

}

ULONG deep_vfork()
{
	struct vforkcontext userdata;
	struct Process *me;

//	BPTR out = Open("CON:",MODE_NEWFILE);

	userdata.success = FALSE;

	struct TagItem tags_stackbuilder_process[] = {
			{ NP_EntryData, (ULONG) &userdata },
			{ NP_Name, (ULONG) "copy process" },
			{ NP_Start,  (ULONG) childStackBuilderStart },
			{ NP_Child, TRUE },
			{TAG_END,0}
		};

	 struct TagItem tags_child_process[] = {
			{ NP_StackSize, 0 },
			{ NP_Name, (ULONG) "vfork() child" },
			{ NP_Start,  (ULONG) childStart },
//			{ NP_Output, out },
			{ NP_FinalCode , (ULONG) ChildFinalCode },
			{ NP_Child, TRUE },
			{ TAG_END , 0 }
		};


	userdata.parent.proc = (struct Process *) FindTask(NULL);

	tags_child_process[0].ti_Data = userdata.parent.proc ->  pr_StackSize;

	userdata.child.proc = CreateNewProcTagList(tags_child_process);

	CreateNewProcTagList(tags_stackbuilder_process);

	Wait( SIGF_CHILD );

	me = (struct Process *) FindTask(NULL);

	Forbid();
	DebugPrintF("vfork running from task %08lx\n",me);
	Permit();

	if (me == userdata.parent.proc) 
	{
		return ( userdata.success ) ? (ULONG) userdata.child.proc : (ULONG) 0  ;
	}

	return 0;
}

ULONG vfork()
{
	return deep_vfork();
}

void vfork_end()
{
}


void vforkExit(int code)
{
	struct Process *me;
	me = (struct Process *) FindTask(NULL);

	if (me -> pr_FinalCode  == ChildFinalCode)
	{
		RemTask(FindTask(NULL));
	}
	else
	{
		while(vfork_count>0)
		{
			Delay(1);
		}
		exit(code);
	}
}

