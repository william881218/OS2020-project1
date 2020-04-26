#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sched.h>
#include <string.h>
#include <errno.h>

#include "process.h"
#include "scheduler.h"

typedef struct process Process;

int main(int argc, char* argv[])
{
    //initilize, reading task from stdin
	char scheduling_policy[256];
	int proc_num = 0;
	scanf("%s%d", scheduling_policy, &proc_num);

    //creating process list
    Process *proc_list;
	proc_list = (Process *) malloc(sizeof(Process) * proc_num);

    //read in each process' info
	for (int i = 0; i < proc_num; i++) {
		scanf("%s%d%d", proc_list[i].name, &proc_list[i].ready_t, &proc_list[i].exec_t);
	}

    //specify the scheduling policy
	if (scheduling_policy[0] == 'F'){ //FIFO
		scheduling(proc_list, proc_num, FIFO);
	}else if (scheduling_policy[0] == 'R'){ //RR
		scheduling(proc_list, proc_num, RR);
	}else if (scheduling_policy[0] == 'S') { //SJF
		scheduling(proc_list, proc_num, SJF);
	}else if (scheduling_policy[0] == 'P') { //PSJF
		scheduling(proc_list, proc_num, PSJF);
	}else {
		perror("unknown scheduling policy...\n")
	}

	exit(0);
}
