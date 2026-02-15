
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
	ULONG childPID;
};

int vfork();
void vfork_end();

