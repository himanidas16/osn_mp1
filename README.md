[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/fkrRRp25)
 

# Mini-Project 1: Operating Systems and Networks

This project implements three major components: a POSIX-compliant shell, a reliable UDP-based networking protocol (S.H.A.M.), and xv6 operating system enhancements including system calls and schedulers.



## Part 1: Shell Implementation [645 marks]

A POSIX-compliant shell supporting advanced features like piping, redirection, background execution, and job control.

### Features Implemented

#### Part A: Shell Input [65 marks]
- **A.1: Shell Prompt [10]** - Displays `<Username@SystemName:current_path>` with tilde substitution
- **A.2: User Input [5]** - Accepts and processes user commands
- **A.3: Input Parsing [50]** - Complete parser for the shell grammar with proper syntax validation

#### Part B: Shell Intrinsics [70 marks]
- **B.1: hop [20]** - Directory navigation with support for `~`, `.`, `..`, `-`, and paths
- **B.2: reveal [20]** - File/directory listing with `-a` and `-l` flags
- **B.3: log [30]** - Command history with persistence, purge, and execution capabilities

#### Part C: File Redirection and Pipes [200 marks]
- **C.1: Command Execution** - Execute external commands with proper error handling
- **C.2: Input Redirection [50]** - Redirect stdin from files using `<`
- **C.3: Output Redirection [50]** - Redirect stdout using `>` and `>>` 
- **C.4: Command Piping [100]** - Multi-command pipelines with proper process management

#### Part D: Sequential and Background Execution [200 marks]
- **D.1: Sequential Execution [100]** - Execute multiple commands with `;` separator
- **D.2: Background Execution [100]** - Background jobs with `&` and job status tracking

#### Part E: Exotic Shell Intrinsics [110 marks]
- **E.1: activities [20]** - List all running/stopped processes
- **E.2: ping [20]** - Send signals to processes
- **E.3: Signal Handling [30]** - Ctrl-C, Ctrl-D, Ctrl-Z support
- **E.4: Job Control [40]** - `fg` and `bg` commands for job management

### How to Build and Run

```bash
cd shell/
make all                    # Compiles to shell.out
./shell.out                 # Start the shell

# Example usage:
<username@hostname:~> hop Documents
<username@hostname:~/Documents> reveal -la
<username@hostname:~/Documents> cat file.txt | grep "pattern" > output.txt &
<username@hostname:~/Documents> activities
<username@hostname:~/Documents> log
```

### Key Implementation Details

- **Grammar Parser**: Implements complete CFG parsing for shell commands
- **Process Management**: Proper fork/exec with process group handling
- **Memory Management**: No memory leaks, proper cleanup on exit
- **Signal Safety**: Safe signal handling without race conditions
- **Job Control**: Complete background job tracking and control

## Part 2: Networking - S.H.A.M. Protocol [80 marks]

A reliable protocol built on top of UDP that simulates TCP features like connection management, flow control, and retransmission.

### Features Implemented

#### Core Functionalities
- **1.1: S.H.A.M. Packet Structure [5]** - Custom header with seq_num, ack_num, flags, window_size
- **1.2: Connection Management [10]** - 3-way handshake establishment, 4-way termination
- **1.3: Data Sequencing and Retransmission [25]** - Sliding window, cumulative ACKs, timeout/retransmission

#### Additional Features
- **2: Flow Control [10]** - Window-based flow control mechanism
- **3.1: Command-Line Interface** - Support for both file transfer and chat modes
- **3.2: Mode-Specific Behavior [10]** - File transfer with MD5 verification, real-time chat
- **3.3: MD5 Verification [5]** - Automatic checksum calculation and verification
- **3.4: Packet Loss Simulation [5]** - Configurable packet loss for testing
- **4: Verbose Logging [10]** - Detailed protocol event logging with timestamps

### How to Build and Run

```bash
cd networking/

# Install dependencies (Ubuntu/Debian)
sudo apt update && sudo apt install libssl-dev

# Install dependencies (macOS)
brew install openssl

# Build
make all

# File Transfer Mode
./server 8080                                           # Terminal 1
./client 127.0.0.1 8080 input.txt output.txt          # Terminal 2

# Chat Mode  
./server 8080 --chat                                   # Terminal 1
./client 127.0.0.1 8080 --chat                        # Terminal 2

# With packet loss simulation
./server 8080 0.1                                      # 10% packet loss
./client 127.0.0.1 8080 input.txt output.txt 0.1     # 10% packet loss

# Enable verbose logging
RUDP_LOG=1 ./server 8080                              # Creates server_log.txt
RUDP_LOG=1 ./client 127.0.0.1 8080 input.txt output.txt  # Creates client_log.txt
```

### Protocol Details

#### S.H.A.M. Header Structure
```c
struct sham_header {
    uint32_t seq_num;      // Sequence number (byte-based)
    uint32_t ack_num;      // Acknowledgment number (cumulative)
    uint16_t flags;        // SYN(0x1), ACK(0x2), FIN(0x4)
    uint16_t window_size;  // Flow control window
};
```

#### Connection Management
- **3-Way Handshake**: SYN → SYN-ACK → ACK
- **4-Way Termination**: FIN → ACK → FIN → ACK
- **Reliable Delivery**: Timeout-based retransmission with 500ms RTO
- **Flow Control**: Dynamic window size adjustment

## Part 3: xv6 Operating System

Enhanced xv6 with custom system calls and advanced scheduling algorithms.

### Features Implemented

#### Part A: Basic System Call
- **A.1: System Call Implementation** - `getreadcount()` tracks total bytes read
- **A.2: User Program** - Test program verifying functionality

#### Part B: Schedulers
- **FCFS Scheduler** - First-Come-First-Serve non-preemptive scheduling
- **CFS Scheduler** - Completely Fair Scheduler with virtual runtime
  - **B.1: Priority Support** - Nice values with weight calculation
  - **B.2: Virtual Runtime Tracking** - Per-process vruntime management
  - **B.3: Scheduling Logic** - Minimum vruntime selection
  - **B.4: Time Slice Calculation** - Dynamic time slice based on load

#### Bonus: MLFQ Scheduler
- Multi-Level Feedback Queue with 4 priority levels
- Aging mechanism to prevent starvation
- Different time slices per queue level

### How to Build and Run

```bash
cd xv6/

# Test basic xv6
make qemu                    # Default round-robin scheduler
# In xv6: run 'readcount' to test Part A

# Test FCFS scheduler
make clean
make qemu SCHEDULER=FCFS CPUS=1    
# In xv6: run 'schedulertest' and use Ctrl+P to see process info

# Test CFS scheduler  
make clean
make qemu SCHEDULER=CFS CPUS=1
# In xv6: run 'schedulertest' and observe fair scheduling

# Test MLFQ scheduler (bonus)
make clean  
make qemu SCHEDULER=MLFQ CPUS =1
# In xv6: run 'schedulertest' and observe multi-level queuing

# Generate performance report
# Run schedulertest on single CPU for each scheduler
# Compare average waiting and running times
```



 