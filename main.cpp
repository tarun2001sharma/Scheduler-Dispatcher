#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <getopt.h>
#include <cstdlib> // For exit() and EXIT_FAILURE
#include <unordered_set>
#include <queue>
#include <iomanip>
#include <stack>
#include <climits>
#include <list>
#include <atomic>
using namespace std;

int time_cpubusy = 0, time_iobusy = 0, num_processes = 0, finishtime = 0;

int quantum = 10000;
int eventOrder = 0; // Global variable to track event insertion order
int CURRENT_TIME = 0; // Initialize at the start of the simulation
bool CALL_SCHEDULER = false; // Initialize as false; set to true when needed
int max_prio = 4;

int endIO = 0;

int verbose = 0;


// quantum = INT_MAX;


enum ProcessState { 
    CREATED, 
    READY, 
    RUNNING, 
    BLOCKED, 
    FINISHED 
};


enum TransitionType {
    TRANS_TO_READY,
    TRANS_TO_RUN,
    TRANS_TO_BLOCK,
    TRANS_TO_PREEMPT,
    TRANS_TO_DONE
    // Add other transition types as needed
};

std::string processStateToString(ProcessState state) {
    switch (state) {
        case CREATED: return "CREATED";
        case READY: return "READY";
        case RUNNING: return "RUNNG";
        case BLOCKED: return "BLOCK";
        default: return "UNKNOWN";
    }
}

struct Process {
    int pid; // Process ID
    int arrivalTime;
    int totalCPUTime;
    int CPUBurst;
    int remainingCB;
    int IOBurst;
    int staticPriority;
    int dynamicPriority;
    ProcessState state;
    ProcessState prev_state;
    int curr_event_timestamp;
    int remaining_time;
    int finishing_time;
    int io_time;
    int next_event_time;
    unsigned long sequence; // Additional field to track insertion order
    
    // Add more attributes as necessary

    Process(int id, int at, int tc, int cb, int remcb, int io, int prio, int dynproio)
        : pid(id), arrivalTime(at), totalCPUTime(tc), CPUBurst(cb), remainingCB(remcb), IOBurst(io), staticPriority(prio), dynamicPriority(prio-1), 
        state(CREATED), prev_state(CREATED), curr_event_timestamp(at), remaining_time(tc), finishing_time(at), io_time(0), next_event_time(-1) {}

    // Function to calculate the total used CPU time
    int getUsedCPUTime() const {
        return totalCPUTime - remaining_time;
    }
};

Process* CURRENT_RUNNING_PROCESS = nullptr; // No process is running initially

struct Event {
    int timestamp; // Event timestamp
    Process* process; // Associated process
    TransitionType transition; // Event type
    int order; 
    // Constructor
    Event(int t, Process* p, TransitionType et, int ord) : timestamp(t), process(p), transition(et), order(ord) {}
};


// Comparator for priority queue (min-heap by default in C++)
struct EventComparator {
    bool operator()(const Event* a, const Event* b) {
        if (a->timestamp == b->timestamp) return a->order > b->order; // Earlier order has higher priority
        return a->timestamp > b->timestamp; // Earlier time has higher priority
    }
};

// Global Event Queue
priority_queue<Event*, vector<Event*>, EventComparator> eventQueue;

// Done queue
std::queue<Process*> doneQueue;

Event* get_event() {
    if (eventQueue.empty()) {
        return nullptr; // Return nullptr if there are no more events
    }
    Event* nextEvent = eventQueue.top(); // Get the next event
    eventQueue.pop(); // Remove this event from the queue
    return nextEvent; // Return the event
}

int get_next_event_time() {
    if (eventQueue.empty()) {
        return -1; // Return -1 or some other indicator that there are no more events
    }
    return eventQueue.top()->timestamp; // Return the timestamp of the next event
}

void deleteEventByPID(priority_queue<Event*, vector<Event*>, EventComparator>& eventQueue, int pid) {
    // Temporary container to hold events that are not being deleted
    vector<Event*> tempEvents;

    // Transfer events, excluding those with the given pid
    while (!eventQueue.empty()) {
        Event* currentEvent = eventQueue.top();
        eventQueue.pop();

        if (currentEvent->process->pid != pid) {
            // Keep this event
            tempEvents.push_back(currentEvent);
        } else {
            // Delete the event object if it matches the pid
            delete currentEvent; // Assuming dynamic allocation
        }
    }

    // Rebuild the priority queue with the remaining events
    for (Event* event : tempEvents) {
        eventQueue.push(event);
    }
}

// Abstract Scheduler class
class Scheduler {
public:
    virtual void add_process(Process* p) = 0;
    virtual Process* get_next_process() = 0;
    // virtual void schedule_event(Event* e) = 0; // You may need to implement this depending on your design
    // A new method to get the name of the scheduler
    virtual string get_name() const = 0;

    // virtual void checkProcesses() = 0; 
    
    // Add more functions as necessary
    virtual bool test_preempt(Process *p, int currentTime) {
        return false; // Default behavior for schedulers that don't support preemption
    }

    virtual bool should_preempt(Process* currentProcess, int currentTime) const {
        return false; // Default implementation that can be overridden
    }
    
};

// Example: FCFS Scheduler
class FCFSScheduler : public Scheduler {
private:
    std::queue<Process*> readyQueue; // Ready queue
    int quantum = INT_MAX;
public:

    string get_name() const override { return "FCFS"; }

    void add_process(Process* p) override {
        readyQueue.push(p);
    }

    Process* get_next_process() override {
        if (readyQueue.empty()) return nullptr;
        Process* p = readyQueue.front();
        readyQueue.pop();
        return p;
    }

};


// Example: FCFS Scheduler
class LCFSScheduler : public Scheduler {
private:
    std::stack<Process*> readyQueue; // Ready queue
    int quantum = INT_MAX;
public:

    string get_name() const override { return "LCFS"; }

    void add_process(Process* p) override {
        readyQueue.push(p);
    }

    Process* get_next_process() override {
        if (readyQueue.empty()) return nullptr;
        Process* p = readyQueue.top();
        readyQueue.pop();
        return p;
    }

};


// Example: FCFS Scheduler
class RRScheduler : public Scheduler {
private:
    std::queue<Process*> readyQueue; // Ready queue for RR scheduling
    int quantum; // Time slice for each process in the ready queue

public:
    RRScheduler(int q) : quantum(q) {} // Constructor to set quantum

    string get_name() const override { return "RR"; }

    void add_process(Process* p) override {
        readyQueue.push(p); // Add process to the end of the ready queue
    }

    Process* get_next_process() override {
        if (readyQueue.empty()) return nullptr;
        Process* p = readyQueue.front();
        readyQueue.pop(); // Remove the process from the front of the queue
        // schedule_quantum_expiration(p);
        return p; // This process will be the next to run
    }

    // Assuming schedule_event is responsible for creating and managing events,
    // You'll need to adjust or remove this based on your specific event handling


    // Function to handle the end of a process's quantum
    // Call this when a process's time slice expires
    void time_slice_expired(Process* p) {
        // Add the process back to the ready queue if it hasn't finished
        if (p->state != FINISHED) {
            readyQueue.push(p);
        }
        // The next call to get_next_process will fetch the next process to run
    }
};

int iscond1 = 0;
int iscond2 = 0;

class PREPRIOScheduler : public Scheduler {
private:
    vector<queue<Process*>> activeQueues;
    vector<queue<Process*>> expiredQueues;
    int quantum;
    int maxPrio;

public:
    PREPRIOScheduler(int q, int mp) : quantum(q), maxPrio(mp) {
        activeQueues.resize(maxPrio);
        expiredQueues.resize(maxPrio);
    }

    string get_name() const override { return "PREPRIO"; }

    void add_process(Process* p) override {
        if (p->dynamicPriority == -1) {
            p->dynamicPriority = p->staticPriority - 1;
            expiredQueues[p->dynamicPriority].push(p);
        } else {
            activeQueues[p->dynamicPriority].push(p);
        }
    }

    Process* get_next_process() override {
        // Check if all queues are empty and swap if necessary

        for (int i = maxPrio - 1; i >= 0; i--) {
            if (!activeQueues[i].empty()) {
                Process* p = activeQueues[i].front();
                activeQueues[i].pop();
                return p;
            }
        }

        if (checkswap()) {
            std::swap(activeQueues, expiredQueues);
            return get_next_process();
        }
        return nullptr; // No process is ready
    }


    bool test_preempt(Process *p, int currentTime) override {
        // // This function should check if a newly ready process should preempt the current process

        iscond1 = 0;
        iscond2 = 0;

        if (CURRENT_RUNNING_PROCESS != nullptr) {
            bool cond1 = p->dynamicPriority > CURRENT_RUNNING_PROCESS->dynamicPriority;
            if(cond1) iscond1 = 1;
            bool cond2 = CURRENT_RUNNING_PROCESS->next_event_time > CURRENT_TIME;
            if(cond2) iscond2 = 1;

            if (cond1 && cond2) {
                return true;
            }
        }

        // No preemption is needed
        return false;
    }

    void update_dynamic_prio(Process *p) {
          // if quantum expires
          p->dynamicPriority -= 1;
          if (p->dynamicPriority == -1) {
              p->dynamicPriority = p->staticPriority - 1;
            //   p->dynamicPriority_reset = 1;
          } 
      }

private:
    bool checkswap() const {
        bool activeQueueEmpty = true; // Assume active queue is empty
        bool expiredQueueNotEmpty = false; // Assume expired queue is empty

        // Check if any active queue is not empty
        for (const auto &q : activeQueues) {
            if (!q.empty()) {
                activeQueueEmpty = false; // Found a non-empty active queue
                break; // No need to check further
            }
        }

        // Check if any expired queue is not empty
        for (const auto &q : expiredQueues) {
            if (!q.empty()) {
                expiredQueueNotEmpty = true; // Found a non-empty expired queue
                break; // No need to check further
            }
        }   
        // Return true only if active queue is empty and expired queue is not empty
        return activeQueueEmpty && expiredQueueNotEmpty;
    }

};


class PRIOScheduler : public Scheduler {
private:
    vector<queue<Process*>> activeQueues;
    vector<queue<Process*>> expiredQueues;
    int quantum;
    int maxPrio;

public:
    PRIOScheduler(int q, int mp) : quantum(q), maxPrio(mp) {
        activeQueues.resize(maxPrio);
        expiredQueues.resize(maxPrio);
    }

    string get_name() const override { return "PRIO"; }

    void add_process(Process* p) override {
        if (p->dynamicPriority == -1) {
            p->dynamicPriority = p->staticPriority - 1;
            expiredQueues[p->dynamicPriority].push(p);
        } else {
            activeQueues[p->dynamicPriority].push(p);
        }
    }

    Process* get_next_process() override {
        // Check if all queues are empty and swap if necessary

        for (int i = maxPrio - 1; i >= 0; i--) {
            if (!activeQueues[i].empty()) {
                Process* p = activeQueues[i].front();
                activeQueues[i].pop();
                return p;
            }
        }

        if (checkswap()) {
            std::swap(activeQueues, expiredQueues);
            return get_next_process();
        }
        return nullptr; // No process is ready
    }

    // void schedule_event(Event* e) override {
    //     // Implement specific event scheduling if necessary for PREPRIO
        
    // }

    bool test_preempt(Process *p, int currentTime) override {
        // No preemption is needed
        return false;
    }

    void update_dynamic_prio(Process *p) {
          // if quantum expires
          p->dynamicPriority -= 1;
          if (p->dynamicPriority == -1) {
              p->dynamicPriority = p->staticPriority - 1;
            //   p->dynamicPriority_reset = 1;
          } 
      }

private:
    bool checkswap() const {
        bool activeQueueEmpty = true; // Assume active queue is empty
        bool expiredQueueNotEmpty = false; // Assume expired queue is empty

        // Check if any active queue is not empty
        for (const auto &q : activeQueues) {
            if (!q.empty()) {
                activeQueueEmpty = false; // Found a non-empty active queue
                break; // No need to check further
            }
        }

        // Check if any expired queue is not empty
        for (const auto &q : expiredQueues) {
            if (!q.empty()) {
                expiredQueueNotEmpty = true; // Found a non-empty expired queue
                break; // No need to check further
            }
        }   
        // Return true only if active queue is empty and expired queue is not empty
        return activeQueueEmpty && expiredQueueNotEmpty;
    }

};

struct SRTFComparator {
    bool operator()(const Process* a, const Process* b) const {
        if (a->remaining_time == b->remaining_time)
            return a->sequence > b->sequence; // Process that came first has higher priority
        return a->remaining_time > b->remaining_time;
    }
};

// Priority queue declaration for SRTF Scheduler
priority_queue<Process*, vector<Process*>, SRTFComparator> srtfQueue;

//  SRTFScheduler using priority_queue for managing processes

class SRTFScheduler : public Scheduler {
private:
    std::priority_queue<Process*, std::vector<Process*>, SRTFComparator> srtfQueue;
    std::atomic<unsigned long> sequenceCounter{0}; // For assigning sequence numbers

public:
    string get_name() const override { return "SRTF"; }

    void add_process(Process* p) override {
        p->sequence = sequenceCounter++; // Assign and increment sequence number
        srtfQueue.push(p);
    }

    Process* get_next_process() override {
        if (srtfQueue.empty()) return nullptr;
        Process* p = srtfQueue.top();
        srtfQueue.pop();
        return p;
    }

    // Additional methods as needed...
};


vector<int> randvals;
int ofs = 0; // Offset for accessing random numbers

// Random number generator function

int myrandom(int burst) {
    int rand = 1 + (randvals[ofs] % burst); // Use current offset value
    ofs = (ofs + 1) % randvals.size(); // Increment and wrap around if needed
    return rand;
}

// Function to read random numbers from file
void readRandomNumbers(const string& filename) {
    ifstream file(filename);
    int n;
    // file >> n;
    int count, num;
    if (file >> count) {
        while (file >> num) {
            randvals.push_back(num);
        }
    }
    file.close();
}

// Function to read process specifications from file and create initial events
void readProcessSpecifications(const string& filename, int max_prio) {
    ifstream file(filename);
    int AT, TC, CB, IO, pid = 0; // Process ID initialization

    if (!file.is_open()) {
        cerr << "Failed to open process specifications file." << endl;
        return;
    }

    while (file >> AT >> TC >> CB >> IO) {
        int staticprio = myrandom(max_prio); // Assign unique priority for each process
        int dynamicprio = staticprio - 1;
        Process* proc = new Process{pid++, AT, TC, CB, -1, IO, staticprio, dynamicprio};
        Event* evt = new Event(AT, proc, TransitionType::TRANS_TO_READY, eventOrder++);
        eventQueue.push(evt); // Add the process arrival event to the global event queue
    }
    file.close();
}


std::string intToStringWithPadding(int value) {
    std::ostringstream oss;
    oss << std::setw(4) << std::setfill('0') << value;
    return oss.str();
}



void printOutput() {
    // Final calculations
    finishtime = CURRENT_TIME;
    num_processes = doneQueue.size();
    int total_turnaround_time = 0;
    int total_cpu_waiting_time = 0;

    // Copy processes from the doneQueue to a vector for sorting
    std::vector<Process*> sortedProcesses;
    while (!doneQueue.empty()) {
        Process* proc = doneQueue.front();
        doneQueue.pop();
        sortedProcesses.push_back(proc);
    }

    // Sort the vector by PID
    std::sort(sortedProcesses.begin(), sortedProcesses.end(), [](const Process* a, const Process* b) {
        return a->pid < b->pid;
    });

    // Iterate over the sorted vector to calculate statistics and print them
    for (Process* proc : sortedProcesses) {
        int turnaround_time = proc->finishing_time - proc->arrivalTime;
        int cpu_waiting_time = turnaround_time - proc->totalCPUTime - proc->io_time; // Assuming io_time is tracked for each process

        total_turnaround_time += turnaround_time;
        total_cpu_waiting_time += cpu_waiting_time;

        std::cout << std::fixed << std::setprecision(0)
                  << intToStringWithPadding(proc->pid) << ": "
                  << std::setw(4) << proc->arrivalTime << " "
                  << std::setw(4) << proc->totalCPUTime << " "
                  << std::setw(4) << proc->CPUBurst << " "
                  << std::setw(4) << proc->IOBurst << " "
                  << std::setw(1) << proc->staticPriority << " | "
                  << std::setw(5) << proc->finishing_time << " "
                  << std::setw(5) << turnaround_time << " "
                  << std::setw(5) << proc->io_time << " "  // Assuming ioTime is tracked
                  << std::setw(5) << cpu_waiting_time << std::endl;
        // Note: You should delete the Process objects here if they were dynamically allocated and are no longer needed
    }

    // Calculate the summary statistics
    double average_turnaround_time = total_turnaround_time / (double)num_processes;
    double average_cpu_waiting_time = total_cpu_waiting_time / (double)num_processes;
    double cpu_util = 100.0 * (time_cpubusy / (double)finishtime);
    double io_util = 100.0 * (time_iobusy / (double)finishtime);
    double throughput = 100.0 * (num_processes / (double)finishtime);

    // Print summary statistics
    std::cout << "SUM: " << finishtime << " "
              << std::fixed
              << std::setprecision(2)
              << std::setw(4) << cpu_util << " "
              << std::setw(4) << io_util << " "
              << std::setw(4) << average_turnaround_time << " "
              << std::setw(4) << average_cpu_waiting_time << " "
              << std::setprecision(3)
              << std::setw(3) << throughput << std::endl;
}

void Simulation(Scheduler* scheduler) {
    Event* evt = nullptr;
    while ((evt = get_event())) { // Assuming get_event() fetches the next event in line
        // std::cout << "New event"<<std::endl;
        // scheduler->checkProcesses();
        string schedulerName = scheduler->get_name();

        Process* proc = evt->process;
        proc->prev_state = proc->state;
        CURRENT_TIME = evt->timestamp;
        int transition = evt->transition;
        // Assuming you have a function to calculate the time spent in the previous state
        int timeInPrevState = CURRENT_TIME - proc->curr_event_timestamp; 
        // std::string prev_state = processStateToString(proc->state);
        // Cleanup the current event after processing
        int expirationTime;
        int randio;
        Event* nextEvent;

        // verbose statements
        bool isprempted = false;
        int onecondtrue = 0;
        string finalpreemptcall = "NO";
        // int startIO = 0;
        

        delete evt;

        switch (transition) {
            case TRANS_TO_READY:
                // Example: Move process to the ready queue
                // std::cout << "Yo this is TRANSTOREADY"<<std::endl;
                proc->state = READY;


                if (proc->prev_state == CREATED) proc->arrivalTime = CURRENT_TIME;

                if (proc->prev_state == BLOCKED){        
                    proc->io_time += timeInPrevState;
                    // time_iobusy += timeInPrevState;
                }

                proc->curr_event_timestamp = CURRENT_TIME; // Update the timestamp of the last state change
                scheduler->add_process(proc);

                if (scheduler->test_preempt(proc, CURRENT_TIME)) {
                    // Schedule preemption
                    isprempted = true;
                    int pidToDelete = CURRENT_RUNNING_PROCESS->pid;
                    deleteEventByPID(eventQueue, pidToDelete);
                    Event* preemptionEvent = new Event(CURRENT_TIME, CURRENT_RUNNING_PROCESS, TRANS_TO_PREEMPT, eventOrder++);
                    eventQueue.push(preemptionEvent);
                    CALL_SCHEDULER = true;
                }

                CALL_SCHEDULER = true;
                // if (proc->prev_state == )
                
                // std::cout << "Processes that entered the READY state at least once:" << std::endl;
                if (verbose){
                    std::cout << CURRENT_TIME << " " << proc->pid << " "<< timeInPrevState << ": "<< 
                    processStateToString(proc->prev_state) << " -> " << processStateToString(proc->state)<< std::endl;
                }
                
                
                break;

            case TRANS_TO_RUN:
                // Schedule CPU burst or preemption
                // std::cout << "Yo this is TRANSTORUN"<<std::endl;
                proc->state = RUNNING;
                // proc->remaining_time = proc->totalCPUTime - timeInPrevState;
                proc->curr_event_timestamp = CURRENT_TIME;

                if (proc->remainingCB > 0){
                    proc->remainingCB = proc->remainingCB;
                }
                else if (proc->remainingCB <= 0){
                    proc->remainingCB = myrandom(proc->CPUBurst);
                }

                if(proc->remaining_time < proc->remainingCB){
                    proc->remainingCB = proc->remaining_time;
                }

                if (verbose){
                    std::cout << CURRENT_TIME << " " << proc->pid << " "<< timeInPrevState << ": "<< 
                processStateToString(proc->prev_state) << " -> " << processStateToString(proc->state) <<
                " cb=" << proc->remainingCB << " rem="<< proc->remaining_time<< 
                " prio=" << proc->dynamicPriority << std::endl;
                }
                

                if (proc->remainingCB <= quantum){
                    // std::cout << "Hhhhhhh" << proc->remaining_time << "    "<< proc->remainingCB <<std::endl;
                    if(proc->remaining_time <= proc->remainingCB){
                        expirationTime = CURRENT_TIME + proc->remaining_time;
                        proc->next_event_time = expirationTime;
                        nextEvent = new Event(expirationTime, proc, TRANS_TO_BLOCK, eventOrder++);
                        // Add this event to the global event queue
                        eventQueue.push(nextEvent);
                    }
                    else{
                        expirationTime = CURRENT_TIME + proc->remainingCB;
                        proc->next_event_time = expirationTime;
                        nextEvent = new Event(expirationTime, proc, TRANS_TO_BLOCK, eventOrder++);
                        // Add this event to the global event queue
                        eventQueue.push(nextEvent);
                    }
                }
                else{
                    expirationTime = CURRENT_TIME + quantum;
                    proc->next_event_time = expirationTime;
                    nextEvent = new Event(expirationTime, proc, TRANS_TO_PREEMPT, eventOrder++);
                    // Add this event to the global event queue
                    eventQueue.push(nextEvent);
                }

                break;
            case TRANS_TO_BLOCK:
                // Process moves to blocked state; schedule I/O completion
                // std::cout << "Yo this is TRANSTOBLOCK"<< randio<<"  "<< expirationTime <<std::endl;

                proc->curr_event_timestamp = CURRENT_TIME;
                proc->remaining_time -= timeInPrevState;
                proc->remainingCB = -1;
                proc->state = BLOCKED;
                if (proc->prev_state == RUNNING) time_cpubusy += timeInPrevState;
                // Process Done, exit
                if (proc->remaining_time <= 0){
                    if (verbose){
                        std::cout << CURRENT_TIME << " " << proc->pid << " "<< timeInPrevState << 
                    ": Done" << std::endl;
                    }
                    
                    proc->finishing_time = CURRENT_TIME;
                    CURRENT_RUNNING_PROCESS = NULL;
                    CALL_SCHEDULER = true;
                    doneQueue.push(proc);

                    break;
                }
                
                randio = myrandom(proc->IOBurst);
                // specific to the PREPRIO scheduler
                    proc->dynamicPriority = proc->staticPriority - 1; // Reset dynamic priority if needed

                if(endIO <CURRENT_TIME + randio && endIO > CURRENT_TIME){
                        time_iobusy += (CURRENT_TIME + randio - endIO);
                        endIO = CURRENT_TIME + randio;
                    }
                else if(endIO <= CURRENT_TIME){
                        endIO = CURRENT_TIME + randio;
                        time_iobusy += randio;
                    }

                if (verbose){
                    std::cout << CURRENT_TIME << " " << proc->pid << " "<< timeInPrevState << ": "<< 
                    processStateToString(proc->prev_state) << " -> " << processStateToString(proc->state) <<
                    " ib=" << randio<< " rem="<< proc->remaining_time<< std::endl;
                }
                

                expirationTime = CURRENT_TIME + randio;
                // std::cout << "Yo this is TRANSTOBLOCK"<< randio<<"  "<< expirationTime <<std::endl;
                nextEvent = new Event(expirationTime, proc, TRANS_TO_READY, eventOrder++);
                // proc->io_time += randio;
                eventQueue.push(nextEvent);

                CURRENT_RUNNING_PROCESS = NULL;
                CALL_SCHEDULER = true;
                break;

            case TRANS_TO_PREEMPT:
                // Move the currently running process back to the ready queue if not finished
                // std::cout << "Yo this is TRANSTOPREEMPT"<<std::endl;
                if (proc->prev_state == RUNNING) time_cpubusy += timeInPrevState;
                proc->curr_event_timestamp =CURRENT_TIME;
                proc->remaining_time -= timeInPrevState;
                proc->remainingCB -= timeInPrevState;
                // proc->remainingCB = -1;
                proc->state = READY;
                
                if (verbose){
                     std::cout << CURRENT_TIME << " " << proc->pid << " "<< timeInPrevState << ": "<< 
                    processStateToString(proc->prev_state) << " -> " << processStateToString(proc->state) <<
                    " cb=" << proc->remainingCB << " rem="<< proc->remaining_time<< 
                    " prio=" << proc->dynamicPriority << std::endl;
                }
               

                // specific to the PREPRIO scheduler
                proc->dynamicPriority--; // Boost the dynamic priority due to preemption
                    // if (proc->dynamicPriority < 0) {
                    //     proc->dynamicPriority = proc->staticPriority - 1; // Reset dynamic priority if needed
                    // }
                

                if (proc->state != FINISHED) {
                    // std::cout <<"KHIKIIKHI"<<std::endl;
                    proc->curr_event_timestamp = CURRENT_TIME;
                    scheduler->add_process(proc);
                }

                // Schedule the next process to run
                CALL_SCHEDULER = true;
                CURRENT_RUNNING_PROCESS = NULL;
                break;

            case TRANS_TO_DONE:
                if (verbose){
                    std::cout << CURRENT_TIME << " " << proc->pid << " "<< timeInPrevState << 
                    ": Done" << std::endl;
                }
               
                
                proc->remaining_time -= timeInPrevState;
                proc->remainingCB -= timeInPrevState;
                proc->finishing_time = CURRENT_TIME;
                // proc->remainingCB = -1;
                proc->state = READY;
                // CALL_SCHEDULER = true;
                // CURRENT_RUNNING_PROCESS = NULL;

                // Add the process to the doneQueue
                doneQueue.push(proc);
                break;
        }

        if (CALL_SCHEDULER){
            // std::cout << "Arrived at CALL_SCHEDULER"<< std::endl;
            if (get_next_event_time() == CURRENT_TIME){
                // std::cout << "get_next_event_time() == CURRENT_TIME"<< std::endl;

                // evt = get_event();
                continue;
            }
            // std::cout << "Arrived at ELSE CALL_SCHEDULER"<<std::endl;

            CALL_SCHEDULER = false;
            if (!CURRENT_RUNNING_PROCESS) {
                // std::cout << "Arrived balls deep CALL_SCHEDULER"<<std::endl;

                CURRENT_RUNNING_PROCESS = scheduler->get_next_process();
                if (CURRENT_RUNNING_PROCESS == nullptr) continue;
                // If a process is selected, schedule it to run
                // std::cout << proc->pid<<std::endl;
                Event* evt = new Event(CURRENT_TIME, CURRENT_RUNNING_PROCESS, TransitionType::TRANS_TO_RUN, eventOrder++);
                eventQueue.push(evt);                
            }
        }        
    }
}


int main(int argc, char* argv[]) {
    // Parse command-line arguments for inputfile and randfile paths
    // Simplified example, adjust as needed
    int opt;
    bool verbose = false, traceEventExecution = false, traceEventQueue = false, tracePreemption = false;
    std::string schedSpec;

     // Parse command-line options
    while ((opt = getopt(argc, argv, "vteps:")) != -1) {
        size_t colonPosition;
        string algorithm;
        switch (opt) {
            case 's':
                schedSpec = optarg;
                colonPosition = schedSpec.find(':');
                
                if (colonPosition == string::npos) {
                    algorithm = schedSpec.substr(1);
                } else {
                    algorithm = schedSpec.substr(1, colonPosition - 1);
                }

                if (!algorithm.empty()) quantum = stoi(algorithm);

                if (colonPosition != string::npos) max_prio = stoi(schedSpec.substr(colonPosition + 1));

                break;
            // default: // '?'
            //     std::cerr << "Usage: " << argv[0] << " [-v] [-t] [-e] [-p] [-s<schedspec>] inputfile randfile\n";
            //     exit(EXIT_FAILURE);
        }
    }

    Scheduler* scheduler;

    switch (schedSpec[0]) {
            case 'F': 
            std::cout << "FCFS" <<std::endl;
            scheduler = new FCFSScheduler; break;
            case 'L': 
            std::cout << "LCFS" <<std::endl;
            scheduler = new LCFSScheduler; break;
            case 'R': 
            std::cout << "RR " << quantum <<std::endl;
            scheduler = new RRScheduler(quantum); break;
            case 'S': 
            std::cout << "SRTF" <<std::endl;
            scheduler = new SRTFScheduler; break;
            case 'P':
            std::cout << "PRIO " << quantum << std::endl;
                scheduler = new PRIOScheduler(quantum, max_prio);
                break;
            case 'E':
            std::cout << "PREPRIO " <<quantum << std::endl;
                scheduler = new PREPRIOScheduler(quantum, max_prio);
                break;
        }

    // std::cout << schedSpec;
    if (argc - optind < 2) {
        std::cerr << "Expected inputfile and randfile after options\n";
        exit(EXIT_FAILURE);
    }

    string inputFilePath = argv[optind];
    string randFilePath = argv[optind+1];

    readRandomNumbers(randFilePath);
    readProcessSpecifications(inputFilePath, max_prio);
    
    // std::cout << "Event queue size after reading specifications: " << eventQueue.size() << std::endl;
    // Create a copy of the eventQueue for printing purposes
    auto tempQueue = eventQueue;

    // std::cout << "Printing eventQueue contents:" << std::endl;
    while (!tempQueue.empty()) {
        Event* event = tempQueue.top();
        tempQueue.pop();
        // std::cout << "Timestamp: " << event->timestamp << ", Process ID: " << event->process->pid << std::endl;
        // Remember to delete the copied events if they are dynamically allocated and not needed elsewhere
    }
 

    // scheduler = new FCFSScheduler();

    // Initialize and run your simulation
    Simulation(scheduler);
    printOutput();
    // std::priority_queue<Event*, std::vector<Event*>, EventComparator> eventQueue;
    // printPriorityQueue(eventQueue); // Pass a copy; the original eventQueue remains unchanged
    delete scheduler; // Clean up the dynamically allocated scheduler

    return 0;
}