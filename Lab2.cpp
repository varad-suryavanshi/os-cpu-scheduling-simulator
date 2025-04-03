#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <sstream>
#include <queue>
#include <stack>
#include <list>
#include <random>
#include <set>
#include <iomanip> 
#include <getopt.h>
#include <algorithm>
using namespace std;

vector<int> randvals;  
int ofs = 0;           
void load_random_numbers(const string& filename) {
    ifstream infile(filename);
    int count;
    infile >> count;
    randvals.resize(count);
    for (int i = 0; i < count; ++i) {
        infile >> randvals[i];
    }
}


int myrandom(int burst) {
    int result = 1 + (randvals[ofs] % burst);  
    ofs = (ofs + 1) % randvals.size();  
    return result;
}

struct Process{
    int pid;
    int AT;
    int TC;
    int io_b;
    int cpu_b;

    enum process_state {CREATED, READY, RUNNING , BLOCKED, TERMINATED};
    process_state state;

    int remaining_cpu_time;
    int static_priority;
    int dynamic_priority;
    int finish_time=0;
    int wait_time=0;
    int turnaround_time=0;
    int state_ts;
    int cpuactivet=0;
    int ioactivet=0;
    int arrival_readyq;
    int dyn_cpu_b=0;  

    Process(int p, int at, int tc, int cb, int io,int state_ts, int sp) 
    : pid(p), AT(at), TC(tc), cpu_b(cb), io_b(io),state_ts(state_ts), remaining_cpu_time(tc), static_priority(sp){
        dynamic_priority=static_priority-1;
    }


};

bool CALL_SCHEDULER=false;
int CURRENT_TIME=0;
Process* CURRENT_RUNNING_PROCESS=nullptr;
int total_cpu_time=0;
int total_io_time = 0;
int total_turnaround_time = 0;
int total_waiting_time =0;
int io_dummy=0;
int diff=0;
int lastIOtime = 0;  
int ioAcount=0;

struct Event {
    int timestamp;         
    Process* process;       
    enum Transition {TRANS_TO_READY, TRANS_TO_PREEMPT, TRANS_TO_RUN, TRANS_TO_BLOCK, TRANS_TO_TERMINATE};
    
    Transition transition; 
};

list<Event*> event_queue;

vector<Process*> read_file(const string& filename, int maxprio) {
    ifstream infile(filename); 
    string line;
    vector<Process*> processes; 
    int pid = 0; 

    while (getline(infile, line)) {
        istringstream iss(line); 
        int at, tc, cb, io;

        
        if (iss >> at >> tc >> cb >> io) {
            Process* new_process = new Process(pid++, at, tc, cb, io,0,myrandom(maxprio));
            processes.push_back(new_process);
        }
    }
    return processes; 
}



void add_event(int ts, Process* process, Event::Transition event_type) {
    Event* new_event = new Event();  
    new_event->timestamp = ts;
    new_event->process = process;
    new_event->transition = event_type;

    
    if (event_queue.empty()) {
        event_queue.push_back(new_event);
        return;
    }

    
    auto it = event_queue.begin();
    while (it != event_queue.end() && (*it)->timestamp <= ts) {
        ++it;
    }
    event_queue.insert(it, new_event);
}




Event* get_event() {
    if (event_queue.empty()) return nullptr;  
    Event* e = event_queue.front();
    event_queue.pop_front();
    return e;
}


int get_next_event_time() {
    if (event_queue.empty()) {
        return -1;  
    }
    
    Event* e = event_queue.front();  
    int time = e->timestamp;
    return time;
}

class Scheduler {
public:
    virtual ~Scheduler() {}
    virtual void add_process(Process* p) = 0;
    virtual Process* get_next_process() = 0;
};

// FCFS Scheduler
class FCFS : public Scheduler {
public:
    queue<Process*> ready_queue;

    
    void add_process(Process* p) override {
        if (p->state != Process::READY) {
            return;
        }
        ready_queue.push(p);
    }

    
    Process* get_next_process() override {
        if (ready_queue.empty()) {
            return nullptr;  
        }
        
        Process* p = ready_queue.front(); 
        ready_queue.pop();                
        return p;                         
    }

};


// LCFS Scheduler
class LCFS : public Scheduler{
public:
    stack<Process*> ready_stack;  

    void add_process(Process* p) override {
        if (p->state != Process::READY) {
            return;
        }
        ready_stack.push(p);  
    }

    Process* get_next_process() override {
        if (ready_stack.empty()) {
            return nullptr;
        }
        
        Process* p = ready_stack.top();  
        ready_stack.pop();               
        return p;                        
    }

};



// SRTF Scheduler
class SRTF : public Scheduler{
public:
    std::list<Process*> ready_list;  

    void add_process(Process* p) override {
        if (p->state != Process::READY) {
            return;
        }
        ready_list.push_back(p);  
    }

    Process* get_next_process() override {
        if (ready_list.empty()) {
            return nullptr;  
        }

        auto it = std::min_element(ready_list.begin(), ready_list.end(), 
                                   [](Process* p1, Process* p2) {
            return p1->remaining_cpu_time < p2->remaining_cpu_time;
        });

        Process* shortest_process = *it;
        ready_list.erase(it);  
        return shortest_process;
    }
};


// RR Scheduler
class RR : public Scheduler {
    public:
    RR (){}
    queue<Process*> ready_queue;
    
    void add_process(Process* p) override{
        if (p->state != Process::READY) {
            return;
        }
        ready_queue.push(p);
    }

    Process* get_next_process() override{
        if (ready_queue.empty()) {
            return nullptr;  
        }
        
        Process* p = ready_queue.front(); 
        ready_queue.pop();                
        return p;                         
    }
};

// PRIO Scheduler
class PRIO : public Scheduler {
    public:
   
    std::vector<std::queue<Process*>>* active_queue;  
    std::vector<std::queue<Process*>>* expired_queue;  
    int maxprio;                                       
    int time_quantum;

    PRIO(int maxprio, int time_quantum) : maxprio(maxprio), time_quantum(time_quantum) {
        active_queue = new std::vector<std::queue<Process*>>(maxprio);  
        expired_queue = new std::vector<std::queue<Process*>>(maxprio);  
    }

    ~PRIO() {
        delete active_queue;
        delete expired_queue;
    }

    void add_process(Process* p) override {
        if (p->state != Process::READY) {
            return;
        }
        if (p-> dynamic_priority>=0){
            active_queue->at(p->dynamic_priority).push(p);
        }
        else{
            p->dynamic_priority=p->static_priority-1;
            expired_queue->at(p->dynamic_priority).push(p);
        }
    }

    Process* get_next_process() override {
        Process* pr=nullptr;
        for(int counter=maxprio-1;counter>=0;counter--){
        if (!(*active_queue)[counter].empty()) 
        {
            pr = active_queue->at(counter).front();
            active_queue->at(counter).pop();
            return pr;
        }}
        if(pr==nullptr){
            swap(active_queue,expired_queue);
            for(int counter=maxprio-1;counter>=0;counter--){
                if (!(*active_queue)[counter].empty()) 
                {
                    pr = active_queue->at(counter).front();
                    active_queue->at(counter).pop();
                    return pr;
        }}
        }
        
        
        return pr;                         
    }

    
};

// PREPRIO Scheduler
class PREPRIO : public Scheduler{
    public:
   
    std::vector<std::queue<Process*>>* active_queue;  
    std::vector<std::queue<Process*>>* expired_queue;  
    int maxprio;                                       
    int time_quantum;

    PREPRIO(int maxprio, int time_quantum) : maxprio(maxprio), time_quantum(time_quantum) {
        active_queue = new std::vector<std::queue<Process*>>(maxprio);  
        expired_queue = new std::vector<std::queue<Process*>>(maxprio);  
    }

    ~PREPRIO() {
        delete active_queue;
        delete expired_queue;
    }

    void add_process(Process* p) override{
    if (p->state != Process::READY) {
        return;
    }

        if (CURRENT_RUNNING_PROCESS != nullptr) {
                        
            if (p->dynamic_priority > CURRENT_RUNNING_PROCESS->dynamic_priority) {
                int e_t;
                for (auto it = event_queue.begin(); it != event_queue.end(); ++it) {
                        Event* evt = *it;
                        if (evt != nullptr && evt->process == CURRENT_RUNNING_PROCESS) {
                            e_t = evt->timestamp;
                            break;  
                        }
                    }
                if(e_t!=CURRENT_TIME){
                    int evnt_ts;
                    for (auto it = event_queue.begin(); it != event_queue.end(); ++it) {
                        Event* evt = *it;
                        if (evt != nullptr && evt->process == CURRENT_RUNNING_PROCESS) {
                            evnt_ts = evt->timestamp;
                            
                            event_queue.erase(it);  
                            break;  
                        }
                    }
                    
                    CURRENT_RUNNING_PROCESS->remaining_cpu_time += evnt_ts-CURRENT_TIME;
                    CURRENT_RUNNING_PROCESS->dyn_cpu_b+= evnt_ts-CURRENT_TIME;
                    CURRENT_RUNNING_PROCESS->cpuactivet -= evnt_ts-CURRENT_TIME;  

                    add_event(CURRENT_TIME, CURRENT_RUNNING_PROCESS, Event::TRANS_TO_PREEMPT);
                    CURRENT_RUNNING_PROCESS = nullptr;
                }
            }
        }

        if (p->dynamic_priority >= 0) {
            active_queue->at(p->dynamic_priority).push(p);
        } else {
            p->dynamic_priority = p->static_priority - 1;
            expired_queue->at(p->dynamic_priority).push(p);
        }

        
    }

    Process* get_next_process() override {
        Process* pr=nullptr;
        for(int counter=maxprio-1;counter>=0;counter--){
        if (!(*active_queue)[counter].empty()) 
        {
            pr = active_queue->at(counter).front();
            active_queue->at(counter).pop();
            return pr;
        }}
        if(pr==nullptr){
            swap(active_queue,expired_queue);
            for(int counter=maxprio-1;counter>=0;counter--){
                if (!(*active_queue)[counter].empty()) 
                {
                    pr = active_queue->at(counter).front();
                    active_queue->at(counter).pop();
                    return pr;
                }
            }
        }
        return pr;                         
    }

    
};




void Simulation(Scheduler* scheduler,int time_quantum) {        
        Event* evt;
        while( (evt = get_event()) != nullptr ) {
        Process *proc = evt->process;
         // this is the process the event works on
        CURRENT_TIME = evt->timestamp;
        int transition = evt->transition;
        int timeInPrevState = CURRENT_TIME - proc->state_ts; // for accounting
        proc->state_ts = CURRENT_TIME;
         // remove cur event obj and don’t touch anymore

        if (proc->state == Process::BLOCKED) {
            ioAcount--;
            if (ioAcount == 0 && lastIOtime != -1) {
                total_io_time += CURRENT_TIME - lastIOtime;
                lastIOtime = -1;  
            }
        }
        switch(transition) {
            case Event::TRANS_TO_READY:
                if(proc->state== Process::BLOCKED){
                    proc->dynamic_priority= proc->static_priority-1;
                }
                proc->state = Process::READY;
                scheduler->add_process(proc);
                CALL_SCHEDULER = true;
                break;

            case Event::TRANS_TO_PREEMPT:
                proc->dynamic_priority--;
                proc->state = Process::READY;
                CURRENT_RUNNING_PROCESS=nullptr;
                scheduler->add_process(proc);
                CALL_SCHEDULER = true;
                break;

            case Event::TRANS_TO_RUN: {
                proc->state=Process::RUNNING;
                int tq;
                int cpub=0;
                if (proc->dyn_cpu_b == 0) {  
                    int rcpu = proc->remaining_cpu_time;
                    cpub = myrandom(proc->cpu_b);
                    cpub = min(cpub, rcpu);  
                    proc->dyn_cpu_b = cpub;  
                }
                tq = min(time_quantum, proc->dyn_cpu_b);
                proc->cpuactivet += tq;
                proc->dyn_cpu_b -= tq;
                proc->remaining_cpu_time -= tq;

                if (proc->dyn_cpu_b > 0) {
                    add_event(CURRENT_TIME + tq, proc, Event::TRANS_TO_PREEMPT);
                } else {
                    if (proc->remaining_cpu_time > 0) {
                        add_event(CURRENT_TIME + tq, proc, Event::TRANS_TO_BLOCK);
                    } else {
                        add_event(CURRENT_TIME + tq, proc, Event::TRANS_TO_TERMINATE);
                    }
                }
                break;
            }

            case Event::TRANS_TO_BLOCK: {                
                proc->state = Process::BLOCKED;
                int iob = myrandom(proc->io_b);
                proc->ioactivet += iob;
                add_event(CURRENT_TIME + iob, proc, Event::TRANS_TO_READY);

                if (ioAcount == 0) {
                    lastIOtime = CURRENT_TIME;
                }
                ioAcount++;

                CURRENT_RUNNING_PROCESS = nullptr;
                CALL_SCHEDULER = true;
                break;
            }

            case Event::TRANS_TO_TERMINATE:
                proc->state = Process::TERMINATED;
                proc->finish_time = CURRENT_TIME;
                proc->turnaround_time = proc->finish_time - proc->AT;
                proc->wait_time = proc->turnaround_time - proc->cpuactivet - proc->ioactivet;
                
                total_cpu_time += proc->cpuactivet;
                total_turnaround_time += proc->turnaround_time;
                total_waiting_time += proc->wait_time;

                CURRENT_RUNNING_PROCESS = nullptr;
                CALL_SCHEDULER = true;
                break;
        }


        if(CALL_SCHEDULER) {
            if (get_next_event_time() == CURRENT_TIME)
                continue; //process next event from Event queue
            CALL_SCHEDULER = false; // reset global flag
            if (CURRENT_RUNNING_PROCESS == nullptr) {
                CURRENT_RUNNING_PROCESS = scheduler->get_next_process();
                if (CURRENT_RUNNING_PROCESS == nullptr)
                    continue;
                add_event(CURRENT_TIME,CURRENT_RUNNING_PROCESS,Event::TRANS_TO_RUN);
                // create event to make this process runnable for same time.
            } 
        }
        delete evt;
        evt = nullptr;
    } 
}



void print_results(const vector<Process*>& processes, int finishing_time, int total_cpu_time, int total_io_time, int total_turnaround_time, int total_waiting_time, int num_processes, string print_header) {
    cout << print_header << endl;

    for (const auto* process : processes) {
        cout << setw(4) << setfill('0') << process->pid << ": "
            << setw(4) << setfill(' ') << process->AT << " "          
            << setw(4) << process->TC << " "                          
            << setw(4) << process->cpu_b << " "                       
            << setw(4) << process->io_b << " "                        
            << setw(1) << process->static_priority << " | "           
            << setw(5) << process->finish_time << " "                 
            << setw(5) << process->turnaround_time << " "             
            << setw(5) << process->ioactivet << " "                   
            << setw(5) << process->wait_time << endl;                 
    }

    double cpu_utilization = (static_cast<double>(total_cpu_time) / finishing_time) * 100;
    double io_utilization = (static_cast<double>(total_io_time) / finishing_time) * 100;
    double avg_turnaround_time = static_cast<double>(total_turnaround_time) / num_processes;
    double avg_waiting_time = static_cast<double>(total_waiting_time) / num_processes;
    double throughput = (num_processes / static_cast<double>(finishing_time)) * 100;

    cout << "SUM: " << finishing_time << " "
         << fixed << setprecision(2)
         << cpu_utilization << " "
         << io_utilization << " "
         << avg_turnaround_time << " "
         << avg_waiting_time << " "
         << fixed << setprecision(3) 
         << throughput << endl;
}


int main(int argc, char *argv[]) {
    string inputfile, randfile;
    int time_quantum = 10000, maxprio = 4;
    string scheduler_spec;
    string print_header;

    int opt;
    while ((opt = getopt(argc, argv, "vs:")) != -1) {
        if(opt=='s') {
                scheduler_spec = optarg;
                break;
        }
    }

    inputfile = argv[optind];
    randfile = argv[optind + 1];

    load_random_numbers(randfile);

    Scheduler *scheduler = nullptr;
    if (scheduler_spec[0] == 'F') {
        scheduler = new FCFS();
        print_header="FCFS";
    } else if (scheduler_spec[0] == 'L') {
        scheduler = new LCFS();
        print_header="LCFS";
    } else if (scheduler_spec[0] == 'S') {
        scheduler = new SRTF();
        print_header="SRTF";
    } else if (scheduler_spec[0] == 'R') {
        sscanf(scheduler_spec.c_str(), "R%d", &time_quantum);
        scheduler = new RR();
        print_header="RR "+to_string(time_quantum);
    } else if (scheduler_spec[0] == 'P') {
        sscanf(scheduler_spec.c_str(), "P%d:%d", &time_quantum, &maxprio);
        scheduler = new PRIO(maxprio, time_quantum);
        print_header="PRIO "+to_string(time_quantum);
    } else if (scheduler_spec[0] == 'E') {
        sscanf(scheduler_spec.c_str(), "E%d:%d", &time_quantum, &maxprio);
        scheduler = new PREPRIO(maxprio, time_quantum);
        print_header="PREPRIO "+to_string(time_quantum);
    }
    
    vector<Process*> processes = read_file(inputfile,maxprio);
    for (Process* process : processes) {
        add_event(process->AT, process, Event::TRANS_TO_READY);
    }

    Simulation(scheduler,time_quantum);

    print_results(processes, CURRENT_TIME, total_cpu_time, total_io_time, total_turnaround_time, total_waiting_time, processes.size(),print_header);

    for (auto* process : processes) {
        delete process;  
    }
    processes.clear();  

    event_queue.clear();
    delete scheduler;

    return 0;
}
