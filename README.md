# vfork-AmigaOS4-prototype

# ⚠️ WARNING – NON-STANDARD vfork() IMPLEMENTATION (AmigaOS 4)

It is not a true POSIX vfork() implementation.
AmigaOS 4 does not provide:

* Paged stacks
  (Copy-on-write memory, only works on paged stack.)
* Native fork()/vfork() semantics
* Independent duplicated task address spaces

### Instead, this implementation:

* Creates a new Process
* Manually reconstructs the parent stack
* Transfers continuation execution into the child
* Synchronizes parent/child manually
* Uses RemTask() for child termination

It exists for legacy UNIX code compatibility, not as a general multiprocessing primitive.

## When Should You Use This?

* Porting legacy UNIX applications
* Code that immediately calls exec() after vfork()
* Situations where rewriting to pthreads would be extremely complex

## When Should You NOT Use This?

* New software
* High reliability systems
* Security-sensitive environments
* Complex multi-threaded designs

This is a compatibility bridge — not a general multiprocessing solution.

## Performance Warning

This vfork():

* Copies stack memory
* Performs manual continuation patching
* Synchronizes tasks manually

It may be slower than:

pthread
spawn
Native task creation
Modern UNIX systems often discourage fork()/vfork() usage except for exec() scenarios.

Use this only when porting legacy UNIX software where rewriting the process model is impractical.

### 🚫 What the Child MUST NOT Do

*	❌ Do NOT call exit()
		You MUST call:
		vforkExit(code);

	If you call exit() inside a vfork child:
	All libraries may be closed
	Memory may be freed
	Parent process resources may be destroyed
	System instability or crashes may occur
	This is extremely dangerous.

*	❌ Do NOT use parent stack addresses

*	Local variable addresses are not valid after stack reconstruction.

	**Example:**

            int x;
			int *p = &x;   // Unsafe across vfork boundary


	After vfork(), the stack has been rebuilt.
	Addresses of local variables may no longer match expectations.

This can cause:

*	DSI crashes
*	Corrupted memory
*	Silent data corruption

## ❌ Be Careful With Open Files (BPTR)

AmigaOS file handles (BPTR) are pointers.

If a file is closed asynchronously, or by the wrong task, it may result in:

Invalid memory access

* DSI exceptions
* File system corruption

You must ensure that:

* Files are not closed prematurely
* Ownership is clearly defined
* The parent and child do not race on file handles

❌ RoadShow / TCP/IP Sockets

* RoadShow sockets are not shared by default.
  Duplicate sockets before use in the child process.

Failure to do so can result in undefined behavior or crashes.

## ⚠️ Concurrency & Shared State

Many AmigaOS system resources are not fully thread-safe, and documentation is often incomplete.

**You should:**

Protect shared global variables with mutexes

* Avoid Forbid() / Permit() unless absolutely required

* Avoid using signal-based semaphores unless you fully understand the implications

Mutexes are generally safer than semaphores in this context.

Improper synchronization can result in:

* Deadlocks

* Lockups

* Random crashes

* Corrupted system state

## ⚠️ Parent Behavior

Calling vforkExit() in the parent is safe and recommended.

vforkExit() will:

* Wait for active vfork children

* Ensure clean shutdown

* Prevent premature termination while children still run

## Final Notes

This implementation relies on:

* Manual stack reconstruction
* Continuation frame patching
* Shared address space assumptions
* Unexpected side effects may occur.

If something behaves strangely after vfork(),
assume stack layout is the cause first.

