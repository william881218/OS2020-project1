#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sched.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/syscall.h>
#define SYSNO_GET_TIME 369
#define SYSNO_PRINTK 396


/*-------------------- for process related function... -------------------------------*/

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
	CPU_SET(core, &cpu_mask);

	if (sched_setaffinity(pid, sizeof(mask), &mask) < 0) {
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
		syscall(PRINTK, proc_info);
		exit(0);
	}

	/* Assign child to another core prevent from interupted by parant */
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

/*-------------------- process related function finished... -------------------------------*/


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
