# vfork-AmigaOS4-prototype

⚠️ WARNING – EXTREMELY NON-STANDARD vfork() IMPLEMENTATION (AmigaOS 4)

This implementation performs manual stack reconstruction and continuation transfer on AmigaOS 4.

It is NOT fully compatible with POSIX vfork() semantics as implemented on UNIX/Linux systems.
AmigaOS 4 does not use paged stacks or separate process address spaces, and this implementation reconstructs stack frames manually.

This is a compatibility mechanism — not a true fork.

⚠️ Should You Use This?

You probably should NOT use this.

Modern development practice (even on Linux) discourages fork() and vfork() in favor of:

spawn()-style APIs

proper process creation

threads (pthread)

task-based designs

On AmigaOS 4 specifically, this implementation:

Copies stack frames

Modifies return addresses

Rebuilds execution flow manually

In many cases, this can be slower than spawning a process or using threads.

✅ When Might It Be Useful?

This exists for:

Compiling legacy UNIX software

Porting codebases that assume vfork()

Situations where rewriting thread/process logic would take days or weeks

If you are trying to port legacy software and cannot easily determine where a thread logically ends, this may allow the code to run with minimal modification.

This is a compatibility bridge, not a recommended architecture.

🚫 What the Child MUST NOT Do
❌ Do NOT call exit()

You MUST call:

vforkExit(code);


If you call exit() inside a vfork child:

All libraries may be closed

Memory may be freed

Parent process resources may be destroyed

System instability or crashes may occur

This is extremely dangerous.

❌ Do NOT use parent stack addresses

Local variable addresses are not valid after stack reconstruction.

Example:

int x;
int *p = &x;   // Unsafe across vfork boundary


After vfork(), the stack has been rebuilt.
Addresses of local variables may no longer match expectations.

This can cause:

DSI crashes

Corrupted memory

Silent data corruption

❌ Be Careful With Open Files (BPTR)

AmigaOS file handles (BPTR) are pointers.

If a file is closed asynchronously, or by the wrong task, it may result in:

Invalid memory access

DSI exceptions

File system corruption

You must ensure that:

Files are not closed prematurely

Ownership is clearly defined

The parent and child do not race on file handles

❌ RoadShow / TCP/IP Sockets

RoadShow sockets are not shared by default.

Duplicate sockets before use in the child process.

Failure to do so can result in undefined behavior or crashes.

⚠️ Concurrency & Shared State

Many AmigaOS system resources are not fully thread-safe, and documentation is often incomplete.

You should:

Protect shared global variables with mutexes

Avoid Forbid() / Permit() unless absolutely required

Avoid using signal-based semaphores unless you fully understand the implications

Mutexes are generally safer than semaphores in this context.

Improper synchronization can result in:

Deadlocks

Lockups

Random crashes

Corrupted system state

⚠️ Parent Behavior

Calling vforkExit() in the parent is safe and recommended.

vforkExit() will:

Wait for active vfork children

Ensure clean shutdown

Prevent premature termination while children still run

⚠️ Known Limitations

This implementation:

Reconstructs stack frames manually

Patches return addresses

Does not duplicate full process state

Does not clone libc internal state

Does not emulate UNIX memory semantics

There may be unintended side effects.

Not everything will behave exactly as expected after vfork().

🚨 Final Warning

This is advanced, low-level stack manipulation.

If used incorrectly, it may result in:

Silent corruption

Random crashes

DSI exceptions

System instability

Use only if you understand:

AmigaOS task model

Stack backchains

Exec process lifecycle

Signal behavior

Resource ownership rules

This is a compatibility tool for experts — not a general-purpose API.
