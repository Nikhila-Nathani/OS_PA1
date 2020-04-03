#include <kernel.h>
#include <q.h>
#include <proc.h>
#include <lab1.h>

// returns the present scheduler

int getschedclass(){
	return scheduler_class;
}

// sets the scheduler to the given name 

void setschedclass(int sched_class){
	if(sched_class != RANDOMSCHED && sched_class != LINUXSCHED)
		return;
	scheduler_class = sched_class;
}

