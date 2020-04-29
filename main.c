#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sched.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#define SYSNO_GET_TIME 369
#define SYSNO_PRINTK 396


//-------------------- for process related function... -------------------------------

#define CHILD_CPU 1
#define PARENT_CPU 0

#define TIME_UNIT() {for (volatile unsigned long i = 0; i < 1000000UL; i++);} //Running one unit time

struct process {
	char name[32];
	int t_ready;
	int t_exec;
	pid_t pid;
};

int proc_assign_cpu(int pid, int core); //Assign process to specific core
int proc_exec(struct process proc); // Execute the process and return pid
int set_low_priority(int pid); //Set low priority tp process
int set_high_priority(int pid); //Set high priority to process

int proc_assign_cpu(int pid, int core_num){

	cpu_set_t cpu_mask;
	CPU_ZERO(&cpu_mask);
	CPU_SET(core_num, &cpu_mask);

	if (sched_setaffinity(pid, sizeof(cpu_mask), &cpu_mask) < 0) {
		perror("sched_setaffinity error");
		exit(1);
	}

	return 0;
}

int proc_exec(struct process proc)
{
	int pid = fork();

	if (pid < 0) {
		perror("fork");
		return -1;
	}else if(pid == 0){ //child process
		unsigned long start_sec, start_nsec, end_sec, end_nsec;
		char proc_info[200];

        //get the starting time of the process
		syscall(SYSNO_GET_TIME, &start_sec, &start_nsec);

		for (int i = 0; i < proc.t_exec; i++) {
			TIME_UNIT();
		}

        //get the finish time of the process
		syscall(SYSNO_GET_TIME, &end_sec, &end_nsec);

        //print the info in kernel then exit
		sprintf(proc_info, "[project1] %d %lu.%09lu %lu.%09lu\n", getpid(), start_sec, start_nsec, end_sec, end_nsec);
		syscall(SYSNO_PRINTK, proc_info);
		exit(0);
	}

	// Assign child to another core prevent from interupted by parant
	proc_assign_cpu(pid, CHILD_CPU);

	return pid;
}

int set_low_priority(int pid){

	struct sched_param param;
	param.sched_priority = 0; //SCHED_IDLE should set priority to 0

    int ret;
	if ((ret = sched_setscheduler(pid, SCHED_IDLE, &param)) < 0){
		perror("sched_setscheduler");
		return -1;
	}
	return ret;
}

int set_high_priority(int pid){

	struct sched_param param;
	param.sched_priority = 0; //SCHED_OTHER should set priority to 0

    int ret;
	if ((ret = sched_setscheduler(pid, SCHED_OTHER, &param)) < 0){
		perror("sched_setscheduler error");
		return -1;
	}
	return ret;
}

//-------------------- process related function finished... -------------------------------

//-------------------- scheduling related function... -------------------------------

#define FIFO 1
#define RR 2
#define SJF 3
#define PSJF 4



static int cur_time = 0; //current time
static int t_last = 0; //denote the last context switch time for RR
static int running = -1; //current running process, -1 for no process running
static int finish_cnt = 0; //number of finished process

//find the next process to be scheduled
int next_process(struct process *proc, int process_num, int policy);

int scheduling(struct process *proc, int process_num, int policy);

void unit_time() { volatile unsigned long i; for(i=0;i<1000000UL;i++); }

// Sort processes by ready time
int cmp(const void *a, const void *b) {
	return ((struct process *)a)->t_ready - ((struct process *)b)->t_ready;
}

int FIFO_next(struct process *proc, int process_num){
    if (running != -1) return running;
    int next_p = -1;
    for(int i = 0; i < process_num; i++) {
        if(proc[i].pid == -1 || proc[i].t_exec == 0)
            continue;
        if(next_p == -1 || proc[i].t_ready < proc[next_p].t_ready)
            next_p = i;
    }
    return next_p;
}

int RR_next(struct process *proc, int process_num){
    int next_p = -1;

    if (running == -1) { //first one
        for (int i = 0; i < process_num; i++) {
            if (proc[i].pid != -1 && proc[i].t_exec > 0){
                return i;
            }
        }
    }else if ((cur_time - t_last) % 500 == 0)  {
        next_p = (running + 1) % process_num;
        while (proc[next_p].pid == -1 || proc[next_p].t_exec == 0)
            next_p = (next_p + 1) % process_num;
        return next_p;
    }
    //nothing happen, round robin continue...
    return running;
}

int SJF_next(struct process *proc, int process_num, int P){ //P for PSJF
    if (!P && running != -1) return running;
    int next_p = -1;

    for (int i = 0; i < process_num; i++) {
        if (proc[i].pid == -1 || proc[i].t_exec == 0)
            continue;
        if (next_p == -1 || proc[i].t_exec < proc[next_p].t_exec)
            next_p = i;
    }
    return next_p;
}


int scheduling(struct process *process_list, int process_num, int policy)
{
	qsort(process_list, process_num, sizeof(struct process), cmp);

	// Initial pid = -1 imply not ready
	for (int i = 0; i < process_num; i++)
		process_list[i].pid = -1;

	// Set single core prevent from preemption
	proc_assign_cpu(getpid(), PARENT_CPU);

	// Set high priority to scheduler
	set_high_priority(getpid());

	// Initial scheduler
	cur_time = 0;
	running = -1;
	finish_cnt = 0;

	while(1) {
		//fprintf(stderr, "Current time: %d\n", cur_time);

		// Check if running process finish
		if (running != -1 && process_list[running].t_exec == 0) {
			//kill(running, SIGKILL);
			waitpid(process_list[running].pid, NULL, 0);
			printf("%s %d\n", process_list[running].name, process_list[running].pid);
			fflush(stdout);
			fsync(1);
			running = -1;
			finish_cnt++;

			// All process finish
			if (finish_cnt == process_num)
				break;
		}

		// Check if process ready and execute
		for (int i = 0; i < process_num; i++) {
			if (process_list[i].t_ready == cur_time) {
				process_list[i].pid = proc_exec(process_list[i]);
				set_low_priority(process_list[i].pid);
			}

		}

		// Select next running  process
        int next_p = -1;

        if (policy == FIFO) next_p = FIFO_next(process_list, process_num);
        else if (policy == RR) next_p = RR_next(process_list, process_num);
        else if (policy == SJF) next_p = SJF_next(process_list, process_num, 0);
        else if (policy == PSJF) next_p = SJF_next(process_list, process_num, 1);

		if (next_p != -1){ //we need context switch
			if (running != next_p) {
				set_high_priority(process_list[next_p].pid);
				set_low_priority(process_list[running].pid);
				running = next_p;
				t_last = cur_time;
			}
		}

		// Run an unit of time
		unit_time();
		if (running != -1)
			process_list[running].t_exec--;
		cur_time++;
	}

	return 0;
}

//-------------------- scheduling related function finished -------------------------------


int main(int argc, char* argv[])
{
    //initilize, reading task from stdin
	char scheduling_policy[256];
	int process_num = 0;
	scanf("%s%d", scheduling_policy, &process_num);

    //creating process list
    struct process *process_list;
	process_list = (struct process *) malloc(sizeof(struct process) * process_num);

    //read in each process' info
	for (int i = 0; i < process_num; i++) {
		scanf("%s%d%d", process_list[i].name, &process_list[i].t_ready, &process_list[i].t_exec);
	}

    //specify the scheduling policy
	if (scheduling_policy[0] == 'F'){ //FIFO
		scheduling(process_list, process_num, FIFO);
	}else if (scheduling_policy[0] == 'R'){ //RR
		scheduling(process_list, process_num, RR);
	}else if (scheduling_policy[0] == 'S') { //SJF
		scheduling(process_list, process_num, SJF);
	}else if (scheduling_policy[0] == 'P') { //PSJF
		scheduling(process_list, process_num, PSJF);
	}else{
		perror("unknown scheduling policy...\n");
	}

	exit(0);
}
