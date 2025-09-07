#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct cpu cpus[NCPU];

struct proc proc[NPROC];

struct proc *initproc;

int nextpid = 1;
struct spinlock pid_lock;

extern uint ticks; // For MLFQ starvation prevention


extern void forkret(void);
static void freeproc(struct proc *p);

extern char trampoline[]; // trampoline.S

// helps ensure that wakeups of wait()ing
// parents are not lost. helps obey the
// memory model when using p->parent.
// must be acquired before any p->lock.
struct spinlock wait_lock;

// Allocate a page for each process's kernel stack.
// Map it high in memory, followed by an invalid
// guard page.
void proc_mapstacks(pagetable_t kpgtbl)
{
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++)
  {
    char *pa = kalloc();
    if (pa == 0)
      panic("kalloc");
    uint64 va = KSTACK((int)(p - proc));
    kvmmap(kpgtbl, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
  }
}

// Calculate process weight based on nice value
// weight = 1024 / (1.25 ^ nice)
int calculate_weight(int nice)
{
  // Precomputed weights for common nice values to avoid floating point
  static int weight_table[] = {
      88761, 71755, 56483, 46273, 36291, // nice -20 to -16
      29154, 23254, 18705, 14949, 11916, // nice -15 to -11
      9548, 7620, 6100, 4904, 3906,      // nice -10 to -6
      3121, 2501, 1991, 1586, 1277,      // nice -5 to -1
      1024,                              // nice 0
      820, 655, 526, 423, 335,           // nice 1 to 5
      272, 215, 172, 137, 110,           // nice 6 to 10
      87, 70, 56, 45, 36,                // nice 11 to 15
      29, 23, 18, 15, 15                 // nice 16 to 19
  };

  // Clamp nice value to valid range [-20, 19]
  if (nice < -20)
    nice = -20;
  if (nice > 19)
    nice = 19;

  // Return weight from table (offset by 20 for negative indices)
  return weight_table[nice + 20];
}

// ADD THESE MLFQ HELPER FUNCTIONS:

// Get time slice for a queue level
int get_time_slice(int queue_level) {
  switch(queue_level) {
    case 0: return 1;   // 1 tick
    case 1: return 4;   // 4 ticks
    case 2: return 8;   // 8 ticks
    case 3: return 16;  // 16 ticks
    default: return 1;
  }
}

// Move process to next lower queue (demotion due to time slice expiry)
void demote_process(struct proc *p) {
  if(p->queue_level < 3) {  // Change from 2 to 3
    p->queue_level++;
  }
  p->time_slice_used = 0;
  p->queue_entry_time = ticks;
}

// Reset process to highest queue (for starvation prevention)
void promote_to_top(struct proc *p) {
  p->queue_level = 0;
  p->time_slice_used = 0;
  p->queue_entry_time = ticks;  // Update queue entry time
}

// Check if starvation prevention is needed (every 48 ticks)
void check_starvation_prevention() {
  static int last_boost = 0;
  
  // Check every tick, but only act every 48 ticks
  if (ticks - last_boost >= 48) {
    last_boost = ticks;
    
    printf("\n*** STARVATION PREVENTION TRIGGERED AT TICK %d ***\n", ticks);
    
    int promoted_count = 0;
    for (struct proc *p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      if (p->state != UNUSED && p->pid > 2) {  // Exclude init and shell
        if (p->queue_level > 0) {
          printf("Promoting PID %d from Queue %d to Queue 0\n", p->pid, p->queue_level);
          promote_to_top(p);
          promoted_count++;
        }
      }
      release(&p->lock);
    }
    
    if (promoted_count > 0) {
      printf("*** STARVATION PREVENTION: Promoted %d processes ***\n\n", promoted_count);
    }
  }
}



// initialize the proc table.
void procinit(void)
{
  struct proc *p;

  initlock(&pid_lock, "nextpid");
  initlock(&wait_lock, "wait_lock");
  for (p = proc; p < &proc[NPROC]; p++)
  {
    initlock(&p->lock, "proc");
    p->state = UNUSED;
    p->kstack = KSTACK((int)(p - proc));
  }
}

// Must be called with interrupts disabled,
// to prevent race with process being moved
// to a different CPU.
int cpuid()
{
  int id = r_tp();
  return id;
}

// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu *
mycpu(void)
{
  int id = cpuid();
  struct cpu *c = &cpus[id];
  return c;
}

// Return the current struct proc *, or zero if none.
struct proc *
myproc(void)
{
  push_off();
  struct cpu *c = mycpu();
  struct proc *p = c->proc;
  pop_off();
  return p;
}

int allocpid()
{
  int pid;

  acquire(&pid_lock);
  pid = nextpid;
  nextpid = nextpid + 1;
  release(&pid_lock);

  return pid;
}

// Look in the process table for an UNUSED proc.
// If found, initialize state required to run in the kernel,
// and return with p->lock held.
// If there are no free procs, or a memory allocation fails, return 0.
static struct proc *
allocproc(void)
{
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if (p->state == UNUSED) {
      goto found;
    } else {
      release(&p->lock);
    }
  }
  return 0;

found:
  p->pid = allocpid();
  p->state = USED;

  // Initialize scheduling fields
  p->ctime = ticks; // Set creation time to current ticks
  p->vruntime = 0;  // Initialize virtual runtime
  p->rtime = 0;     // Initialize running time
  p->nice = 0;                           // Default nice value
  p->weight = calculate_weight(p->nice); // Calculate weight based on nice


  // ADD THESE MLFQ INITIALIZATIONS:
  p->queue_level = 0;          // Start in highest priority queue
  p->time_slice_used = 0;      // No time used initially
  p->last_scheduled = ticks;   // Set to current time
  p->queue_entry_time = ticks; // Initialize queue entry time
  p->preempted = 0;            // Initialize preemption flag
  p->total_preemptions = 0;    // Initialize preemption count


  // Allocate a trapframe page.
  if ((p->trapframe = (struct trapframe *)kalloc()) == 0)
  {
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // An empty user page table.
  p->pagetable = proc_pagetable(p);
  if (p->pagetable == 0)
  {
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&p->context, 0, sizeof(p->context));
  p->context.ra = (uint64)forkret;
  p->context.sp = p->kstack + PGSIZE;

  return p;
}

// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void
freeproc(struct proc *p)
{
  if (p->trapframe)
    kfree((void *)p->trapframe);
  p->trapframe = 0;
  if (p->pagetable)
    proc_freepagetable(p->pagetable, p->sz);
  p->pagetable = 0;
  p->sz = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->state = UNUSED;
}

// Create a user page table for a given process, with no user memory,
// but with trampoline and trapframe pages.
pagetable_t
proc_pagetable(struct proc *p)
{
  pagetable_t pagetable;

  // An empty page table.
  pagetable = uvmcreate();
  if (pagetable == 0)
    return 0;

  // map the trampoline code (for system call return)
  // at the highest user virtual address.
  // only the supervisor uses it, on the way
  // to/from user space, so not PTE_U.
  if (mappages(pagetable, TRAMPOLINE, PGSIZE,
               (uint64)trampoline, PTE_R | PTE_X) < 0)
  {
    uvmfree(pagetable, 0);
    return 0;
  }

  // map the trapframe page just below the trampoline page, for
  // trampoline.S.
  if (mappages(pagetable, TRAPFRAME, PGSIZE,
               (uint64)(p->trapframe), PTE_R | PTE_W) < 0)
  {
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmfree(pagetable, 0);
    return 0;
  }

  return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void proc_freepagetable(pagetable_t pagetable, uint64 sz)
{
  uvmunmap(pagetable, TRAMPOLINE, 1, 0);
  uvmunmap(pagetable, TRAPFRAME, 1, 0);
  uvmfree(pagetable, sz);
}

// Set up first user process.
void userinit(void)
{
  struct proc *p;

  p = allocproc();
  initproc = p;

  p->cwd = namei("/");

  p->state = RUNNABLE;

  release(&p->lock);
}

// Shrink user memory by n bytes.
// Return 0 on success, -1 on failure.
int growproc(int n)
{
  uint64 sz;
  struct proc *p = myproc();

  sz = p->sz;
  if (n > 0)
  {
    if ((sz = uvmalloc(p->pagetable, sz, sz + n, PTE_W)) == 0)
    {
      return -1;
    }
  }
  else if (n < 0)
  {
    sz = uvmdealloc(p->pagetable, sz, sz + n);
  }
  p->sz = sz;
  return 0;
}

// Create a new process, copying the parent.
// Sets up child kernel stack to return as if from fork() system call.
int kfork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *p = myproc();

  // Allocate process.
  if ((np = allocproc()) == 0)
  {
    return -1;
  }

  // Copy user memory from parent to child.
  if (uvmcopy(p->pagetable, np->pagetable, p->sz) < 0)
  {
    freeproc(np);
    release(&np->lock);
    return -1;
  }
  np->sz = p->sz;

  // copy saved user registers.
  *(np->trapframe) = *(p->trapframe);

  // Cause fork to return 0 in the child.
  np->trapframe->a0 = 0;

  // increment reference counts on open file descriptors.
  for (i = 0; i < NOFILE; i++)
    if (p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]);
  np->cwd = idup(p->cwd);

  safestrcpy(np->name, p->name, sizeof(p->name));

  pid = np->pid;

  release(&np->lock);

  acquire(&wait_lock);
  np->parent = p;
  release(&wait_lock);

  acquire(&np->lock);
  np->state = RUNNABLE;
  release(&np->lock);

  return pid;
}

// Pass p's abandoned children to init.
// Caller must hold wait_lock.
void reparent(struct proc *p)
{
  struct proc *pp;

  for (pp = proc; pp < &proc[NPROC]; pp++)
  {
    if (pp->parent == p)
    {
      pp->parent = initproc;
      wakeup(initproc);
    }
  }
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait().
void kexit(int status)
{
  struct proc *p = myproc();

  if (p == initproc)
    panic("init exiting");

  // Close all open files.
  for (int fd = 0; fd < NOFILE; fd++)
  {
    if (p->ofile[fd])
    {
      struct file *f = p->ofile[fd];
      fileclose(f);
      p->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(p->cwd);
  end_op();
  p->cwd = 0;

  acquire(&wait_lock);

  // Give any children to init.
  reparent(p);

  // Parent might be sleeping in wait().
  wakeup(p->parent);

  acquire(&p->lock);

  p->xstate = status;
  p->state = ZOMBIE;

  release(&wait_lock);

  // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int kwait(uint64 addr)
{
  struct proc *pp;
  int havekids, pid;
  struct proc *p = myproc();

  acquire(&wait_lock);

  for (;;)
  {
    // Scan through table looking for exited children.
    havekids = 0;
    for (pp = proc; pp < &proc[NPROC]; pp++)
    {
      if (pp->parent == p)
      {
        // make sure the child isn't still in exit() or swtch().
        acquire(&pp->lock);

        havekids = 1;
        if (pp->state == ZOMBIE)
        {
          // Found one.
          pid = pp->pid;
          if (addr != 0 && copyout(p->pagetable, addr, (char *)&pp->xstate,
                                   sizeof(pp->xstate)) < 0)
          {
            release(&pp->lock);
            release(&wait_lock);
            return -1;
          }
          freeproc(pp);
          release(&pp->lock);
          release(&wait_lock);
          return pid;
        }
        release(&pp->lock);
      }
    }

    // No point waiting if we don't have any children.
    if (!havekids || killed(p))
    {
      release(&wait_lock);
      return -1;
    }

    // Wait for a child to exit.
    sleep(p, &wait_lock); // DOC: wait-sleep
  }
}


void scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();

  c->proc = 0;
  for (;;)
  {
    intr_on();

#ifdef SCHEDULER_MLFQ
    check_starvation_prevention();

    struct proc *selected = 0;
    int highest_priority = 4;
    static int last_queue3_index = 0; // For round-robin in queue 3

    // Count user processes for logging
    int user_processes = 0;
    for (p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      if (p->state == RUNNABLE && p->pid > 2) {
        user_processes++;
      }
      release(&p->lock);
    }

    // Log MLFQ state when multiple user processes are running
    if (user_processes > 1) {
      printf("[MLFQ Scheduler Tick]\n");
      for (p = proc; p < &proc[NPROC]; p++) {
        acquire(&p->lock);
        if (p->state == RUNNABLE && p->pid > 2) {
          printf("PID: %d | Queue: %d | Time Used: %d/%d\n", 
                 p->pid, p->queue_level, p->time_slice_used, get_time_slice(p->queue_level));
        }
        release(&p->lock);
      }
    }

    // Find the highest priority level with runnable processes
    for (p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      if (p->state == RUNNABLE && p->queue_level < highest_priority) {
        highest_priority = p->queue_level;
      }
      release(&p->lock);
    }

    // Selection logic with Queue 3 round-robin
    if (highest_priority == 3) {
      // Round-robin scheduling for Queue 3
      int processes_checked = 0;
      int total_processes = NPROC;
      
      while (processes_checked < total_processes) {
        int current_index = (last_queue3_index + processes_checked) % NPROC;
        p = &proc[current_index];
        
        acquire(&p->lock);
        if (p->state == RUNNABLE && p->queue_level == 3) {
          selected = p;
          last_queue3_index = (current_index + 1) % NPROC;
          break;
        } else {
          release(&p->lock);
        }
        processes_checked++;
      }
    } else {
      // For other queues: Select first process at highest priority
      for (p = proc; p < &proc[NPROC]; p++) {
        acquire(&p->lock);
        if (p->state == RUNNABLE && p->queue_level == highest_priority) {
          selected = p;
          break;
        } else {
          release(&p->lock);
        }
      }
    }

    // Execute selected process
    if (selected != 0) {
      if (user_processes > 1 && selected->pid > 2) {
        if (selected->queue_level == 3) {
          printf("--> Scheduling PID %d from Queue 3 (Round-Robin)\n", selected->pid);
        } else {
          printf("--> Scheduling PID %d from Queue %d\n", selected->pid, selected->queue_level);
        }
      }

      // Run the selected process
      selected->state = RUNNING;
      c->proc = selected;
      selected->last_scheduled = ticks;

      int start_ticks = ticks;
      int time_slice = get_time_slice(selected->queue_level);

      swtch(&c->context, &selected->context);

      // Process returned to scheduler
      int ticks_used = ticks - start_ticks;
      selected->rtime += ticks_used;

      if (selected->state == RUNNABLE) {
        selected->time_slice_used += ticks_used;

        // Check if time slice is expired
        if (selected->time_slice_used >= time_slice) {
          // Time slice expired - demote process
          int old_queue = selected->queue_level;
          demote_process(selected);
          if (selected->pid > 2) {
            printf("PID %d: Time slice expired! Moving from Queue %d to Queue %d\n", 
                   selected->pid, old_queue, selected->queue_level);
          }
        } else {
          // Process yielded voluntarily - keep in same queue
          if (selected->pid > 2) {
            printf("PID %d: Voluntary yield, staying in Queue %d (used %d/%d ticks)\n", 
                   selected->pid, selected->queue_level, selected->time_slice_used, time_slice);
          }
        }
      }

      c->proc = 0;
      release(&selected->lock);
    }

#elif defined(SCHEDULER_CFS)
    // CFS Scheduler with vRuntime logging
    struct proc *min_vruntime_proc = 0;
    int min_vruntime = __INT_MAX__;
    int runnable_count = 0;

    // First pass: count runnable processes
    for (p = proc; p < &proc[NPROC]; p++)
    {
      acquire(&p->lock);
      if (p->state == RUNNABLE)
      {
        runnable_count++;
      }
      release(&p->lock);
    }

    // Log all runnable processes before scheduling decision
    if (runnable_count > 1)
    { // Only log when multiple processes
      printf("[Scheduler Tick]\n");
      for (p = proc; p < &proc[NPROC]; p++)
      {
        acquire(&p->lock);
        if (p->state == RUNNABLE)
        {
          printf("PID: %d | vRuntime: %d\n", p->pid, p->vruntime);
        }
        release(&p->lock);
      }

      // Find process with minimum vruntime
      for (p = proc; p < &proc[NPROC]; p++)
      {
        acquire(&p->lock);
        if (p->state == RUNNABLE && p->vruntime < min_vruntime)
        {
          if (min_vruntime_proc != 0)
          {
            release(&min_vruntime_proc->lock);
          }
          min_vruntime_proc = p;
          min_vruntime = p->vruntime;
        }
        else
        {
          release(&p->lock);
        }
      }

      if (min_vruntime_proc != 0)
      {
        printf("--> Scheduling PID %d (lowest vRuntime)\n", min_vruntime_proc->pid);
      }
    }
    else
    {
      // Single process case - just find it without logging
      for (p = proc; p < &proc[NPROC]; p++)
      {
        acquire(&p->lock);
        if (p->state == RUNNABLE)
        {
          min_vruntime_proc = p;
          min_vruntime = p->vruntime;
          break;
        }
        else
        {
          release(&p->lock);
        }
      }
    }

    if (min_vruntime_proc != 0)
    {
      // Calculate time slice: target_latency / runnable_processes, min 3 ticks
      int time_slice = 48 / runnable_count;
      if (time_slice < 3)
        time_slice = 3;

      // Switch to chosen process
      min_vruntime_proc->state = RUNNING;
      c->proc = min_vruntime_proc;

      int start_ticks = ticks;
      swtch(&c->context, &min_vruntime_proc->context);

      // Update vruntime after process runs
      int ticks_run = ticks - start_ticks;
      if (ticks_run > 0)
      {
        min_vruntime_proc->vruntime += ticks_run * 1024 / min_vruntime_proc->weight;
      }
      else
      {
        min_vruntime_proc->vruntime += 1; // Minimum increment
      }
      min_vruntime_proc->rtime += ticks_run;

      c->proc = 0;
      release(&min_vruntime_proc->lock);
    }

#elif defined(SCHEDULER_FCFS)
    // FCFS Scheduler with logging
    struct proc *earliest = 0;
    int earliest_time = __INT_MAX__;
    int runnable_count = 0;

    // First pass: count runnable processes
    for (p = proc; p < &proc[NPROC]; p++)
    {
      acquire(&p->lock);
      if (p->state == RUNNABLE)
      {
        runnable_count++;
      }
      release(&p->lock);
    }

    if (runnable_count > 1)
    { // Only log when multiple processes
      printf("[FCFS Scheduler Tick]\n");
      for (p = proc; p < &proc[NPROC]; p++)
      {
        acquire(&p->lock);
        if (p->state == RUNNABLE)
        {
          printf("PID: %d | Creation Time: %d\n", p->pid, p->ctime);
        }
        release(&p->lock);
      }

      // Find earliest process
      for (p = proc; p < &proc[NPROC]; p++)
      {
        acquire(&p->lock);
        if (p->state == RUNNABLE && p->ctime < earliest_time)
        {
          if (earliest != 0)
          {
            release(&earliest->lock);
          }
          earliest = p;
          earliest_time = p->ctime;
        }
        else
        {
          release(&p->lock);
        }
      }

      if (earliest != 0)
      {
        printf("--> Scheduling PID %d (earliest creation time: %d)\n", earliest->pid, earliest_time);
      }
    }
    else
    {
      // Single process case - just find it without logging
      for (p = proc; p < &proc[NPROC]; p++)
      {
        acquire(&p->lock);
        if (p->state == RUNNABLE)
        {
          earliest = p;
          earliest_time = p->ctime;
          break;
        }
        else
        {
          release(&p->lock);
        }
      }
    }

    if (earliest != 0)
    {
      // Switch to chosen process
      earliest->state = RUNNING;
      c->proc = earliest;

      int start_ticks = ticks;
      swtch(&c->context, &earliest->context);
      int ticks_run = ticks - start_ticks;

      // Update running time
      earliest->rtime += ticks_run;

      c->proc = 0;
      release(&earliest->lock);
    }

#else
    // Default Round Robin Scheduler
    for (p = proc; p < &proc[NPROC]; p++)
    {
      acquire(&p->lock);
      if (p->state == RUNNABLE)
      {
        p->state = RUNNING;
        c->proc = p;
        swtch(&c->context, &p->context);
        c->proc = 0;
      }
      release(&p->lock);
    }
#endif
  }
}

// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void sched(void)
{
  int intena;
  struct proc *p = myproc();

  if (!holding(&p->lock))
    panic("sched p->lock");
  if (mycpu()->noff != 1)
    panic("sched locks");
  if (p->state == RUNNING)
    panic("sched RUNNING");
  if (intr_get())
    panic("sched interruptible");

  intena = mycpu()->intena;
  swtch(&p->context, &mycpu()->context);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void yield(void)
{
  struct proc *p = myproc();
  acquire(&p->lock);
  p->state = RUNNABLE;
  sched();
  release(&p->lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void forkret(void)
{
  extern char userret[];
  static int first = 1;
  struct proc *p = myproc();

  // Still holding p->lock from scheduler.
  release(&p->lock);

  if (first)
  {
    // File system initialization must be run in the context of a
    // regular process (e.g., because it calls sleep), and thus cannot
    // be run from main().
    fsinit(ROOTDEV);

    first = 0;
    // ensure other cores see first=0.
    __sync_synchronize();

    // We can invoke kexec() now that file system is initialized.
    // Put the return value (argc) of kexec into a0.
    p->trapframe->a0 = kexec("/init", (char *[]){"/init", 0});
    if (p->trapframe->a0 == -1)
    {
      panic("exec");
    }
  }

  // return to user space, mimicing usertrap()'s return.
  prepare_return();
  uint64 satp = MAKE_SATP(p->pagetable);
  uint64 trampoline_userret = TRAMPOLINE + (userret - trampoline);
  ((void (*)(uint64))trampoline_userret)(satp);
}

// Sleep on channel chan, releasing condition lock lk.
// Re-acquires lk when awakened.
void sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();

  // Must acquire p->lock in order to
  // change p->state and then call sched.
  // Once we hold p->lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup locks p->lock),
  // so it's okay to release lk.

  acquire(&p->lock); // DOC: sleeplock1
  release(lk);

  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  release(&p->lock);
  acquire(lk);
}

// Wake up all processes sleeping on channel chan.
// Caller should hold the condition lock.
void wakeup(void *chan)
{
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++)
  {
    if (p != myproc())
    {
      acquire(&p->lock);
      if (p->state == SLEEPING && p->chan == chan)
      {
        p->state = RUNNABLE;
      }
      release(&p->lock);
    }
  }
}

// Kill the process with the given pid.
// The victim won't exit until it tries to return
// to user space (see usertrap() in trap.c).
int kkill(int pid)
{
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++)
  {
    acquire(&p->lock);
    if (p->pid == pid)
    {
      p->killed = 1;
      if (p->state == SLEEPING)
      {
        // Wake process from sleep().
        p->state = RUNNABLE;
      }
      release(&p->lock);
      return 0;
    }
    release(&p->lock);
  }
  return -1;
}

void setkilled(struct proc *p)
{
  acquire(&p->lock);
  p->killed = 1;
  release(&p->lock);
}

int killed(struct proc *p)
{
  int k;

  acquire(&p->lock);
  k = p->killed;
  release(&p->lock);
  return k;
}

// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
int either_copyout(int user_dst, uint64 dst, void *src, uint64 len)
{
  struct proc *p = myproc();
  if (user_dst)
  {
    return copyout(p->pagetable, dst, src, len);
  }
  else
  {
    memmove((char *)dst, src, len);
    return 0;
  }
}

// Copy from either a user address, or kernel address,
// depending on usr_src.
// Returns 0 on success, -1 on error.
int either_copyin(void *dst, int user_src, uint64 src, uint64 len)
{
  struct proc *p = myproc();
  if (user_src)
  {
    return copyin(p->pagetable, dst, src, len);
  }
  else
  {
    memmove(dst, (char *)src, len);
    return 0;
  }
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void procdump(void)
{
  static char *states[] = {
      [UNUSED] "unused",
      [USED] "used",
      [SLEEPING] "sleep ",
      [RUNNABLE] "runble",
      [RUNNING] "run   ",
      [ZOMBIE] "zombie"};
  struct proc *p;
  char *state;

  printf("\n");
  for (p = proc; p < &proc[NPROC]; p++)
  {
    if (p->state == UNUSED)
      continue;
    if (p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    printf("%d %s %s", p->pid, state, p->name);
    printf("\n");
  }
}

// Check if current process should be preempted by higher priority process
void check_preemption(void) {
  struct proc *p = myproc();
  
  if(p == 0 || p->state != RUNNING || p->pid <= 2)
    return;
    
  int current_queue = p->queue_level;
  int should_preempt = 0;
  
  // Check if there's a higher priority process ready
  for(struct proc *check_proc = proc; check_proc < &proc[NPROC]; check_proc++) {
    acquire(&check_proc->lock);
    if(check_proc != p && check_proc->state == RUNNABLE && 
       check_proc->queue_level < current_queue) {
      should_preempt = 1;
      release(&check_proc->lock);
      break;
    }
    release(&check_proc->lock);
  }
  
  if(should_preempt) {
    yield();
  }
}

