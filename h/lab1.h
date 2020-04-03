#ifndef _LAB1_H_
#define _LAB1_H_

#ifndef RANDOMSCHED
#define RANDOMSCHED 1
#endif

#ifndef LINUXSCHED
#define LINUXSCHED 2
#endif

 
int scheduler_class;

void setschedclass(int sched_class);
int getschedclass();

#endif
