// Per-CPU state
struct cpu {
  uchar apicid;                // Local APIC ID
  struct context *scheduler;   // swtch() here to enter scheduler
  struct taskstate ts;         // Used by x86 to find stack for interrupt
  struct segdesc gdt[NSEGS];   // x86 global descriptor table
  volatile uint started;       // Has the CPU started?
  int ncli;                    // Depth of pushcli nesting.
  int intena;                  // Were interrupts enabled before pushcli?
  struct proc *proc;           // The process running on this cpu or null
};

extern struct cpu cpus[NCPU];
extern int ncpu;

//PAGEBREAK: 17
// Saved registers for kernel context switches.
// Don't need to save all the segment registers (%cs, etc),
// because they are constant across kernel contexts.
// Don't need to save %eax, %ecx, %edx, because the
// x86 convention is that the caller has saved them.
// Contexts are stored at the bottom of the stack they
// describe; the stack pointer is the address of the context.
// The layout of the context matches the layout of the stack in swtch.S
// at the "Switch stacks" comment. Switch doesn't save eip explicitly,
// but it is on the stack and allocproc() manipulates it.
struct context {
  uint edi;
  uint esi;
  uint ebx;
  uint ebp;
  uint eip;
};

enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };


struct mlfqnode
{
  struct mlfqnode* prev, *next;
  struct proc* self;
  int state; //0: unused, 1: used, -1: header
  int level;
  int exec_time; //executed time on this level
};

// Per-process state
struct proc {
  uint sz;                     // Size of process memory (bytes)
  pde_t* pgdir;                // Page table
  char *kstack;                // Bottom of kernel stack for this process
  enum procstate state;        // Process state
  int pid;                     // Process ID
  struct proc *parent;         // Parent process
  struct trapframe *tf;        // Trap frame for current syscall
  struct context *context;     // swtch() here to run process
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)
  int schedmode;               // Scheduling mode (0: default, 1 : fixed share, 2 : mlfq)
  double path;                 // path used in stride scheduling     
  int fixedshare;              // used only in fixed share scheduling mode
  int pathlevel;               // used only in default scheduling mode
  struct mlfqnode* mnode;      // pointer for mlfq node used in mlfq scheduling 
  uint originalbase;           
  thread_t tid;                // thread id, if it's not thread, don't use this value
  void* retval;
  struct proc* master;         
  uint emptystacks[NPROC];
  int emptystackcnt;
  int thread_kill_cnt;
  struct proc* thread_header;
  struct proc* prev_thread;    // if it is main thread, then prev_thread is tail of threads list
  struct proc* next_thread;    // if it is main thread, then next_thread is head of threads list
};
// Process memory is laid out contiguously, low addresses first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap
