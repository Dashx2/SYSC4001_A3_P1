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

std::tuple<std::string> run_simulation(std::vector<PCB> list_processes) {

    std::vector<PCB> ready_queue;   // The ready queue of processes
    std::vector<PCB> wait_queue;    // The wait queue of processes (doing I/O)
    std::vector<PCB> job_list;      // Track global state of all processes
    std::vector<unsigned int> wait_remaining; // Remaining I/O time per waiting process

    unsigned int current_time = 0;
    PCB running;

    // Initialize CPU as idle
    idle_CPU(running);

    std::string execution_status;

    // Header row of the execution table
    execution_status = print_exec_header();

    // Main simulation loop
    while (job_list.empty() || !all_process_terminated(job_list)) {

        //////////////////// 1) NEW -> READY (arrivals) ////////////////////
        for (auto &process : list_processes) {
            if (process.arrival_time == current_time) {
                // Assign memory and admit to READY
                assign_memory(process);

                process.state = READY;
                ready_queue.push_back(process);
                job_list.push_back(process);

                // Log NEW -> READY
                execution_status += print_exec_status(current_time,process.PID,NEW,READY);
            }
        }

        //////////////////// 2) Schedule if CPU is idle ////////////////////
        if (running.PID == -1 && !ready_queue.empty()) {
            // EP: lower PID = higher priority
            // Sort so smallest PID ends up at back(), since run_process()
            // takes ready_queue.back().
            std::sort(ready_queue.begin(), ready_queue.end(),
                      [](const PCB &a, const PCB &b) {
                          return a.PID > b.PID;   // descending; smallest at .back()
                      });

            // Dispatch chosen process
            run_process(running, job_list, ready_queue, current_time);

            // Log READY -> RUNNING (time = current_time)
            execution_status += print_exec_status(current_time,running.PID,READY,RUNNING);
        }

        //////////////////// 3) Run 1 ms of CPU ////////////////////
        // Any events from this 1 ms (I/O or termination) happen at time current_time + 1.
        if (running.PID != -1) {
            // Consume 1 ms of CPU
            if (running.remaining_time > 0) {
                running.remaining_time--;
            }

            // Total CPU used so far
            unsigned int used_cpu = running.processing_time - running.remaining_time;

            bool do_terminate = false;
            bool do_io = false;

            // Check for termination
            if (running.remaining_time == 0) {
                do_terminate = true;
            }
            // Else check for I/O event (if there is any I/O)
            else if (running.io_freq > 0 &&
                     (used_cpu % running.io_freq == 0)) {
                do_io = true;
            }

            if (do_terminate) {
                // RUNNING -> TERMINATED at time current_time + 1
                execution_status += print_exec_status(current_time + 1,running.PID,RUNNING,TERMINATED);
                terminate_process(running, job_list);
                idle_CPU(running);
            } else if (do_io) {
                // RUNNING -> WAITING at time current_time + 1
                running.state = WAITING;
                sync_queue(job_list, running);

                wait_queue.push_back(running);
                wait_remaining.push_back(running.io_duration);

                execution_status += print_exec_status(current_time + 1,running.PID,RUNNING,WAITING);
                idle_CPU(running);
            } else {
                // Still RUNNING; just sync remaining_time in job_list
                sync_queue(job_list, running);
            }
        }

        //////////////////// 4) I/O progress for WAITING processes ////////////////////
        // Each waiting process gets 1 ms of I/O service in [current_time, current_time+1).
        // If its I/O finishes, it becomes READY at time current_time + 1.
        for (std::size_t i = 0; i < wait_queue.size(); /* increment inside */) {
            if (wait_remaining[i] > 0) {
                wait_remaining[i]--;
            }

            if (wait_remaining[i] == 0) {
                PCB proc = wait_queue[i];
                proc.state = READY;
                sync_queue(job_list, proc);
                ready_queue.push_back(proc);

                // WAITING -> READY at time current_time + 1
                execution_status += print_exec_status(current_time + 1,proc.PID,WAITING,READY);

                // Remove from wait_queue/wait_remaining
                wait_queue.erase(wait_queue.begin() + i);
                wait_remaining.erase(wait_remaining.begin() + i);
            } else {
                ++i;
            }
        }

        //////////////////// 5) Advance time ////////////////////
        current_time++;
    }

    // Footer row of the execution table
    execution_status += print_exec_footer();

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