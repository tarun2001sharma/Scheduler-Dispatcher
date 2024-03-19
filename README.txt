
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

## Contributing

Contributions to enhance the simulation's functionality or extend its scheduling algorithms portfolio are welcome. Please adhere to the project's code style and contribute via pull requests.

## License

Distributed under the MIT License. See `LICENSE` for more information.
