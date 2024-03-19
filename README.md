
# Scheduler / Dispatcher Simulation

This repository contains a simulation designed for exploring various scheduling policies applied to processes or threads executing within a simulated environment. Developed for the Programming Assignment #2 of Class CSCI-GA.2250-001 Spring 2024, guided by Professor Hubertus Franke, it aims to provide a comprehensive understanding of scheduling algorithms' impact on process management in an operating system.

## Overview

The simulation employs a Discrete Event Simulation (DES) system, focusing on a single-core CPU model without hyperthreading. It intricately models the process life cycle, encompassing states from creation to completion, and simulates the effect of different scheduling algorithms on these processes.

### Supported Scheduling Algorithms:

- **FCFS (First-Come, First-Served)**
- **LCFS (Last-Come, First-Served)**
- **SRTF (Shortest Remaining Time First)**
- **RR (Round Robin)**
- **PRIO (Priority Scheduling)**
- **PREPRIO (Preemptive Priority Scheduling)**

![Scheduler/Dispatcher Simulation Architecture](/mnt/data/Screenshot 2024-03-19 at 02.38.42.png)
## Detailed Technical Description

### Core Design Principles

#### Discrete Event Simulation (DES)

At the core of the scheduler/dispatcher simulation lies the DES methodology, which abstractly represents the system's operation through a chronological sequence of events. Each event signifies a state transition, offering a precise and clear-cut simulation of process scheduling and execution.

#### Process Model

The simulation defines processes with attributes such as Arrival Time (AT), Total CPU Time (TC), CPU Burst (CB), and IO Burst (IO). It simulates single-threaded processes in a non-preemptive or preemptive manner, depending on the scheduling algorithm applied, closely mirroring real-world operating system behavior.

#### Scheduling Algorithms

Implemented algorithms range from basic to advanced, including FCFS, LCFS, SRTF, RR, PRIO, and PREPRIO, each demonstrating unique characteristics and impacts on process scheduling. The simulation provides insights into the algorithms' operational intricacies and their implications on system performance.

#### Event Management System

A priority queue-based event management system orchestrates the simulation, ensuring chronological processing of events. This system facilitates an in-depth analysis of the scheduling algorithms by tracking process state transitions and scheduling decisions.

### Advanced Computational Concepts

The simulation integrates advanced computer science and programming concepts, including:

- **Polymorphism and Object-Oriented Design**: Employing abstract classes and interfaces to define a common scheduler framework, enabling the seamless integration and comparison of different scheduling algorithms.
- **Priority Queue for Event Management**: Utilizing data structures to manage events efficiently, ensuring that process transitions and scheduling decisions adhere to the simulation timeline.
- **Atomic Operations for Thread Safety**: In scenarios where multithreading might be explored, atomic operations ensure data integrity without compromising performance.
- **Algorithmic Complexity Considerations**: Optimizing data structures and algorithms to minimize computational complexity, enhancing the simulation's efficiency and scalability.

## Installation and Usage

### Prerequisites

- GNU Compiler Collection (GCC) or compatible C++ compiler
- GNU Make for building the project

### Building the Simulation

1. Clone the repository:
```bash
git clone https://github.com/your-username/your-repo-name.git
```

2. Navigate to the project directory:
```bash
cd your-repo-name
```

3. Compile the project:
```bash
make
```

4. Run the simulation with the desired scheduling algorithm and input files:
```bash
./sched -s[algorithm] inputfile randfile
```

Replace `[algorithm]` with the scheduling algorithm code (e.g., `F` for FCFS), `inputfile` with the path to the process specification file, and `randfile` with the path to the random numbers file.


## Execution and Invocation Format:

Your program should follow the following invocation:
```
<program> [-v] [-t] [-e] [-p] [-s<schedspec>] inputfile randfile
```
Options should be able to be passed in any order. This is the way a good programmer will do that. See [GNU libc manual on getopt](http://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html) for examples.

Test input files and the sample file with random numbers are available as a NYU brightspace attachment.

The scheduler specification is “–s [ FLS | R<num> | P<num>[:<maxprio>] | E<num>[:<maxprios>] ]”, where F=FCFS, L=LCFS, S=SRTF and R10 and P10 are RR and PRIO with quantum 10. (e.g., “./sched –sR10”) and E10 is the preemptive prio scheduler. Supporting this parameter is required and the quantum is a positive number. In addition, the number of priority levels is specified in PRIO and PREPRIO with an optional “:num” addition. E.g., “-sE10:5” implies quantum=10 and numprios=5. If the addition is omitted then maxprios=4 by default.

The –v option stands for verbose and should print out some tracing information that allows one to follow the state transition. Though this is not mandatory, it is highly suggested you build this into your program to allow you to follow the state transition and to verify the program. I include samples from my tracing for some inputs (not all). Matching my format will allow you to run diffs and identify why results and where the results don’t match up. You can always use `/home/frankeh/Public/lab2/sched` to create your own detailed output for not provided samples. Also use `-t` and `-e` and `-p` options for more details on eventQ, runQ, and preemption. “-t” traces the event execution. “-e” shows the eventQ before and after an event is inserted and “-p” shows for the E scheduler the decision when an unblocked process attempts to preempt a running process. Remember two conditions must be met (higher prio and pending event of the currently running process is in the future, not now). Note options -t -e -p -v do NOT have to be implemented.

### Please ensure the following:

- (a) The input and randfile must accept any path and should not assume a specific location relative to the code or executable.
- (b) All output must go to the console (due to the harness testing)
- (c) All code/grading will be executed on machine `<linserv1.cims.nyu.edu>`

As always, if you detect errors in the sample inputs and outputs, let me know immediately so I can verify and correct if necessary. Please refer to the input/output file number and the line number.


## Output

At the end of the program, you should print the following information, and the example outputs provide the proper expected formatting (including precision); this is necessary to automate the results checking; all required output should go to the console (stdout / cout).

### a) Scheduler Information
- Which scheduler algorithm and, in case of RR/PRIO/PREPRIO, also the quantum.

### b) Per Process Information
Printed in the order of process appearance in the input file. For each process (assume processes start with pid=0), the correct desired format is shown below:
```
pid: AT TC CB IO PRIO | FT TT IT CW
```
- **FT**: Finishing time
- **TT**: Turnaround time (finishing time - AT)
- **IT**: I/O Time (time in blocked state)
- **PRIO**: Static priority assigned to the process (note this only has meaning in PRIO/PREPRIO case)
- **CW**: CPU Waiting time (time in Ready state)

### c) Summary Information
Finally, print a summary for the simulation:
- Finishing time of the last event (i.e., the last process finished execution)
- CPU utilization (i.e., percentage (0.0 – 100.0) of time at least one process is running)
- IO utilization (i.e., percentage (0.0 – 100.0) of time at least one process is performing IO)
- Average turnaround time among processes
- Average CPU waiting time among processes
- Throughput of number processes per 100 time units

CPU / IO utilizations and throughput are computed from time=0 till the finishing time.

#### Example:
```
FCFS
0000: 0 100 10 10 2 | 223 223 123 0
0001: 500 100 20 10 1 | 638 138 38 0
SUM: 638 31.35 25.24 180.50 0.00 0.313
```

You must strictly adhere to this format. The program’s results will be graded by a testing harness that uses “diff –b”. In particular, you must pay attention to separate the tokens and to the rounding.

For more details on formatting and precision handling in C++, see examples in `/home/frankeh/Public/ProgExamples/Format/format.cpp` as discussed in extra sessions.

## Contributing

Contributions to enhance the simulation's functionality or extend its scheduling algorithms portfolio are welcome. Please adhere to the project's code style and contribute via pull requests.

## License

Distributed under the MIT License. See `LICENSE` for more information.
