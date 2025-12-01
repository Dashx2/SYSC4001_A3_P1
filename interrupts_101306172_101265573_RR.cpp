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
                                    //to the "Process, Arrival time, Burst time" table that you
                                    //see in questions. You don't need to use it, I put it here
                                    //to make the code easier :).

    unsigned int current_time = 0;
    PCB running;

    const unsigned int TIME_QUANTUM = 100; // setting time quantum
    unsigned int time_slice_used = 0; //checking the time that has been utilized within time quantum
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
        static std::vector<unsigned int> wait_remaining;
        if (wait_remaining.size() != wait_queue.size()) {
            // Initialize if needed (first time some processes enter wait_queue).
            wait_remaining.assign(wait_queue.size(), 0);
        }

        for (std::size_t i = 0; i < wait_queue.size(); /* increment inside */) {
            if (wait_remaining[i] > 0) {
                wait_remaining[i]--;
            }

            if (wait_remaining[i] == 0) {
                // I/O finished -> move to READY
                PCB proc = wait_queue[i];
                proc.state = READY;
                sync_queue(job_list, proc);

                // Enqueue at READY queue "tail" (see comment above about mapping)
                ready_queue.insert(ready_queue.begin(), proc);

                // Log WAITING -> READY at time current_time + 1
                execution_status += print_exec_status(current_time + 1,proc.PID,WAITING,READY);
                current_time += 1;
                // Remove from wait_queue & wait_remaining
                wait_queue.erase(wait_queue.begin() + i);
                wait_remaining.erase(wait_remaining.begin() + i);
            } else {
                ++i;
            }
        }
        /////////////////////////////////////////////////////////////////

        //////////////////////////SCHEDULER//////////////////////////////
        if (running.PID == -1 && !ready_queue.empty()) {
            // Next process to run is at ready_queue.back() (see comments above)
            run_process(running, job_list, ready_queue, current_time);

            // Reset time slice for this process
            time_slice_used = 0;
            //noting the starting time to calculate metrics
            if (running.start_time == -1){
            running.start_time = current_time;
            sync_queue(job_list, running);
            }
            // Log READY -> RUNNING at current_time
            execution_status += print_exec_status(current_time,running.PID,READY,RUNNING);
        }
        if (running.PID != -1) {
            // Consume 1ms of CPU
            if (running.remaining_time > 0) {
                running.remaining_time--;
            }

            time_slice_used++;

            // Total CPU used so far by this process
            unsigned int used_cpu = running.processing_time - running.remaining_time;

            bool do_terminate = false;
            bool do_io = false;
            bool do_preempt = false;

            // 1) Check for termination
            if (running.remaining_time == 0) {
                do_terminate = true;
            }
            // 2) Check for I/O event (if process actually does I/O)
            else if (running.io_freq > 0 &&
                    (used_cpu % running.io_freq == 0)) {
                do_io = true;
            }
            // 3) Check for quantum expiration (RR preemption)
            else if (time_slice_used >= TIME_QUANTUM) {
                do_preempt = true;
            }

            if (do_terminate) {
                // RUNNING -> TERMINATED at time current_time + 1
                execution_status += print_exec_status(current_time + 1,running.PID,RUNNING,TERMINATED);
                running.completion_time = current_time + 1;
                terminate_process(running, job_list);
                idle_CPU(running);
                time_slice_used = 0;
            }
            else if (do_io) {
                // RUNNING -> WAITING at time current_time + 1
                running.state = WAITING;
                sync_queue(job_list, running);

                wait_queue.push_back(running);

                // Ensure wait_remaining has same size as wait_queue
                if (wait_remaining.size() < wait_queue.size()) {
                    wait_remaining.push_back(running.io_duration);
                } else {
                    wait_remaining[wait_queue.size() - 1] = running.io_duration;
                }

                execution_status += print_exec_status(current_time + 1,running.PID,RUNNING,WAITING);
                idle_CPU(running);
                time_slice_used = 0;
            }
            else if (do_preempt) {
                // Time quantum expired but process still has work and no I/O
                // RUNNING -> READY at time current_time + 1
                running.state = READY;
                sync_queue(job_list, running);

                // Enqueue at end of RR queue (tail)
                ready_queue.insert(ready_queue.begin(), running);

                execution_status += print_exec_status(current_time + 1,running.PID,RUNNING,READY);
                idle_CPU(running);
                time_slice_used = 0;
            }
            else {
                // Still RUNNING; just sync remaining_time etc.
                sync_queue(job_list, running);
            }
        }

        ////////////////////// Advance simulated time //////////////////////
        current_time++;
    }
        /////////////////////////////////////////////////////////////////
    
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