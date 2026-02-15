
/*
 * Copyright (c) 2026 Kjetil Hvalstrand. All rights reserved.
 *
 * Denne koden er lisensiert under BSD 3-Clause License.
 * Se https://opensource.org for detaljer.
 */

#include <stdlib.h>

#include <proto/dos.h>
#include <proto/exec.h>

#include "vfork_imp.h"

extern struct DebugIFace * IDebug;

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

