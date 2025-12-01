/**
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 * @brief template main.cpp file for Assignment 3 Part 1 of SYSC4001
 * 
 */

#include<interrupts_101306172_101265573.hpp>

void FCFS(std::vector<PCB> &ready_queue) {
    std::sort( 
                ready_queue.begin(),
                ready_queue.end(),
                []( const PCB &first, const PCB &second ){
                    return (first.arrival_time > second.arrival_time); 
                } 
            );
}

std::tuple<std::string /* add std::string for bonus mark */ > run_simulation(std::vector<PCB> list_processes) {

    std::vector<PCB> ready_queue;   //The ready queue of processes
    std::vector<PCB> wait_queue;    //The wait queue of processes
    std::vector<PCB> job_list;      //A list to keep track of all the processes. This is similar
    std::vector<unsigned int> wait_remaining; // Remaining I/O time per waiting process
                                    //to the "Process, Arrival time, Burst time" table that you
                                    //see in questions. You don't need to use it, I put it here
                                    //to make the code easier :).

    unsigned int current_time = 0;
    PCB running;
    const unsigned int TIME_QUANTUM = 100;
    unsigned int time_slice_used = 0;
    //Initialize an empty running process
    idle_CPU(running);

    std::string execution_status;

    //make the output table (the header row)
    execution_status = print_exec_header();

    //Loop while till there are no ready or waiting processes.
    //This is the main reason I have job_list, you don't have to use it.
    while(!all_process_terminated(job_list) || job_list.empty()) {

        //Inside this loop, there are three things you must do:
        // 1) Populate the ready queue with processes as they arrive
        // 2) Manage the wait queue
        // 3) Schedule processes from the ready queue

        //Population of ready queue is given to you as an example.
        //Go through the list of proceeses
        for(auto &process : list_processes) {
            if(process.arrival_time == current_time) {//check if the AT = current time
                //if so, assign memory and put the process into the ready queue
                assign_memory(process);

                process.state = READY;  //Set the process state to READY
                ready_queue.push_back(process); //Add the process to the ready queue
                job_list.push_back(process); //Add it to the list of processes

                execution_status += print_exec_status(current_time, process.PID, NEW, READY);
            }
        }

        ///////////////////////MANAGE WAIT QUEUE/////////////////////////
        //This mainly involves keeping track of how long a process must remain in the ready queue
        for (std::size_t i = 0; i < wait_queue.size(); /* increment inside */) {
            if (wait_remaining[i] > 0) {
                wait_remaining[i]--;
            }

            if (wait_remaining[i] == 0) {
                // I/O finished -> WAITING -> READY at time current_time + 1
                PCB proc = wait_queue[i];
                proc.state = READY;
                sync_queue(job_list, proc);

                ready_queue.push_back(proc);

                execution_status += print_exec_status(current_time + 1,proc.PID,WAITING,READY);
                current_time += 1;
                wait_queue.erase(wait_queue.begin() + i);
                wait_remaining.erase(wait_remaining.begin() + i);
            } else {
                ++i;
            }
        }
                if (running.PID == -1 && !ready_queue.empty()) {
            // EP_RR: lowest PID = highest priority
            // We sort so the process with *smallest* PID ends up at back()
            std::sort(
                ready_queue.begin(),
                ready_queue.end(),
                [](const PCB &a, const PCB &b) {
                    if (a.PID == b.PID) {
                        // Tie-breaker inside same priority level
                        return a.arrival_time > b.arrival_time;
                    }
                    return a.PID > b.PID; // descending; smallest PID at .back()
                });

            run_process(running, job_list, ready_queue, current_time);
            time_slice_used = 0;

            //noting the starting time to calculate metrics
            if (running.start_time == -1){
            running.start_time = current_time;
            sync_queue(job_list, running);
            }
            execution_status += print_exec_status(current_time,running.PID,READY,RUNNING);
        } 
        /////////////////////////////////////////////////////////////////

        //////////////////////////SCHEDULER//////////////////////////////
        if (running.PID != -1) {
            // Consume 1 ms of CPU
            if (running.remaining_time > 0) {
                running.remaining_time--;
            }

            time_slice_used++;

            unsigned int used_cpu =
                running.processing_time - running.remaining_time;

            bool do_terminate           = false;
            bool do_io                  = false;
            bool do_priority_preempt    = false;
            bool do_quantum_preempt     = false;

            // 1) Terminate?
            if (running.remaining_time == 0) {
                do_terminate = true;
            }
            // 2) I/O event?
            else if (running.io_freq > 0 &&
                    (used_cpu % running.io_freq == 0)) {
                do_io = true;
            }
            else {
                // 3) Check for higher-priority READY process
                int bestPID = running.PID;
                for (const auto &p : ready_queue) {
                    if (p.PID < bestPID) {
                        bestPID = p.PID;
                    }
                }
                if (bestPID < running.PID) {
                    do_priority_preempt = true;
                }
                // 4) Otherwise, quantum expiry (RR)
                else if (time_slice_used >= TIME_QUANTUM) {
                    do_quantum_preempt = true;
                }
            }

            if (do_terminate) {
                // RUNNING -> TERMINATED at time current_time
                execution_status += print_exec_status(current_time,running.PID,RUNNING,TERMINATED);
                running.completion_time = current_time + 1;
                terminate_process(running, job_list);
                idle_CPU(running);
                time_slice_used = 0;
            }
            else if (do_io) {
                // RUNNING -> WAITING at time current_time 
                running.state = WAITING;
                sync_queue(job_list, running);

                wait_queue.push_back(running);
                wait_remaining.push_back(running.io_duration);

                execution_status += print_exec_status(current_time,running.PID,RUNNING,WAITING);
                idle_CPU(running);
                time_slice_used = 0;
            }
            else if (do_priority_preempt) {
                // Higher-priority READY process exists
                // RUNNING -> READY at time current_time 
                running.state = READY;
                sync_queue(job_list, running);

                ready_queue.push_back(running);

                execution_status += print_exec_status(current_time + 1,running.PID,RUNNING,READY);
                idle_CPU(running);
                time_slice_used = 0;
            }
            else if (do_quantum_preempt) {
                // Time quantum expired - Round Robin within same priority
                running.state = READY;
                sync_queue(job_list, running);

                ready_queue.push_back(running);

                execution_status += print_exec_status(current_time + 1,running.PID,RUNNING,READY);
                idle_CPU(running);
                time_slice_used = 0;
            }
            else {
                // Still RUNNING, just sync its remaining_time etc.
                sync_queue(job_list, running);
            }
        }
        /////////////////////////////////////////////////////////////////
        current_time++;
    }
    
    //Close the output table
    execution_status += print_exec_footer();
    Metrics metrics = calculate_metrics(job_list);
    execution_status += metrics;

    return std::make_tuple(execution_status);
}


int main(int argc, char** argv) {

    //Get the input file from the user
    if(argc != 2) {
        std::cout << "ERROR!\nExpected 1 argument, received " << argc - 1 << std::endl;
        std::cout << "To run the program, do: ./interrutps <your_input_file.txt>" << std::endl;
        return -1;
    }

    //Open the input file
    auto file_name = argv[1];
    std::ifstream input_file;
    input_file.open(file_name);

    //Ensure that the file actually opens
    if (!input_file.is_open()) {
        std::cerr << "Error: Unable to open file: " << file_name << std::endl;
        return -1;
    }

    //Parse the entire input file and populate a vector of PCBs.
    //To do so, the add_process() helper function is used (see include file).
    std::string line;
    std::vector<PCB> list_process;
    while(std::getline(input_file, line)) {
        auto input_tokens = split_delim(line, ", ");
        auto new_process = add_process(input_tokens);
        list_process.push_back(new_process);
    }
    input_file.close();

    //With the list of processes, run the simulation
    auto [exec] = run_simulation(list_process);

    write_output(exec, "execution.txt");

    return 0;
}