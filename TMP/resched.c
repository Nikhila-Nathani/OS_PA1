#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <stdio.h>
#include <lab1.h>
unsigned long currSP;   /* REAL sp of current process */
extern int ctxsw(int, int, int, int);
extern int ctr1000;

/*-----------------------------------------------------------------------
 *  * resched  --  reschedule processor to highest priority ready process
 *   *
 *    * Notes:       Upon entry, currpid gives current process id.
 *     *              Proctab[currpid].pstate gives correct NEXT state for
 *      *                      current process if other than PRREADY.
 *       *------------------------------------------------------------------------
 *        */

int resched()
{

        if(getschedclass() == RANDOMSCHED){

            register struct pentry  *optr;  /* pointer to old process entry */
            register struct pentry  *nptr;  /* pointer to new process entry */
				
	    optr= &proctab[currpid];
            if (optr->pstate == PRCURR) {
                optr->pstate = PRREADY;
                insert(currpid,rdyhead,optr->pprio);
	    	}
	
		int sum = ppriosum();
		int take_r = 0;
		if (sum > 0){
			take_r = randfn(sum);
		}	
		int exec_pid = 0;
		int proc = q[rdytail].qprev;
				
		if(sum > 0){
//			kprintf("\n ** SUM: %d ** RAND: %d \n", sum, take_r);
			int proc = q[rdytail].qprev;
			while(proc != rdyhead){
				currpid = proc;
				if(take_r < q[proc].qkey){
					break;  
				}
				else{
					take_r -= q[proc].qkey;
				}		
				proc = q[proc].qprev; 
			}
		}
		else{
			currpid = 0;
		}	
		dequeue(currpid);
		//kprintf("\n new ptr : dequeued is %d proc is %d \n ", currpid, proc);
		/*
		if( proc == 0){ // if the process is the null process
			currpid = 0;
			dequeue(currpid);
		}
		*/
		nptr = &proctab[currpid];   
    		nptr->pstate = PRCURR;          /* mark it currently running    */
        #ifdef  RTCLOCK
                preempt = QUANTUM;              /* reset preemption counter     */
        #endif

		ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);

		/* The OLD process returns here when resumed. */
		return OK;
        }
        else if(getschedclass() == LINUXSCHED){
		
		int epoch = 0;

		register struct	pentry	*optr;	/* pointer to old process entry */
		register struct	pentry	*nptr;	/* pointer to new process entry */
		
		optr = &proctab[currpid];
		optr->gdns = optr->gdns - optr->ctr + preempt; /* The goodness of current proc is initialized to priority by removing the previous ctr */
		optr->ctr = preempt;

		if(currpid == NULLPROC || optr->ctr <= 0){
			optr->ctr = 0;
			optr->gdns = 0;
		}	

		int gdIndex = maxgdns();		/* The pid of the max goodness process in ready queue */
		
		struct pentry *newproc;
		newproc = &proctab[gdIndex];
		int mgdns = newproc->gdns;		/* The max goodness */
		
		if(mgdns == 0 && ((optr->pstate != PRCURR) || (optr->ctr == 0))){
		//if(mgdns == 0){		
			/* update gcq and schedule the null proc if it's not PRCURR */
			epoch +=1;
			updategcq();
			
			preempt = optr->ctr;
			optr= &proctab[currpid];
			if(currpid == NULLPROC){
				return OK;
			}
			
			
			if (optr->pstate == PRCURR) {
				optr->pstate = PRREADY;
				insert(currpid,rdyhead,optr->pprio);
			}
			int neproc = NULLPROC;
			currpid = dequeue(neproc);
			nptr = &proctab[currpid];   
			nptr->pstate = PRCURR;          /* mark it currently running    */
			#ifdef  RTCLOCK
					preempt = QUANTUM;              /* reset preemption counter     */
			#endif

			ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);

			return OK;
			
		}
		
		
		if(optr->gdns > mgdns && optr->pstate == PRCURR && optr->gdns > 0){	
			/* keep as is */
			preempt = optr->ctr;
			return OK;
		}
		else if(optr->gdns < mgdns || optr->ctr == 0 || optr->pstate != PRCURR){
			
			/* schedule the new process with maximum goodness greater than that of the current process */
			
			if(optr->pstate == PRCURR){
				optr->pstate = PRREADY;
				insert(currpid,rdyhead,optr->pprio);
			}
			
			currpid = dequeue(gdIndex);
			
			/* remove highest priority process from ready queue  */

			nptr = &proctab[currpid];
			nptr->pstate = PRCURR;          /* mark it currently running    */
			preempt = nptr->ctr;
			ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
			return OK;
		} 
				
	
        }
        else{
            
	    register struct pentry  *optr;  /* pointer to old process entry */
            register struct pentry  *nptr;  /* pointer to new process entry */

            /* no switch needed if current process priority higher than next*/

            if ( ( (optr= &proctab[currpid])->pstate == PRCURR) &&
                (lastkey(rdytail)<optr->pprio)) {
                    return(OK);
            }

            /* force context switch */

            if (optr->pstate == PRCURR) {
                optr->pstate = PRREADY;
                insert(currpid,rdyhead,optr->pprio);
            }

            /* remove highest priority process at end of ready list */

            nptr = &proctab[ (currpid = getlast(rdytail)) ];
            nptr->pstate = PRCURR;          /* mark it currently running    */
        #ifdef  RTCLOCK
                preempt = QUANTUM;              /* reset preemption counter     */
        #endif

            ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);

            /* The OLD process returns here when resumed. */
            return OK;

        }
}

int ppriosum(){

	int sum = 0;
	int proc = q[rdytail].qprev;

	while( proc != rdyhead ){
//		kprintf("\n In sum fn ~ process: %d - priority: %d \n",proc,q[proc].qkey);
		sum = sum + q[proc].qkey;  
		proc = q[proc].qprev;   
	}
//	kprintf("\n ** The sum for above is %d **\n",sum);
	return sum;
}

int randfn(int sum){
	
	srand(ctr1000);
	int take_r = rand()%sum;
	return take_r;
}


int maxgdns(){						/* The max goodness from the ready queue processes is found & its index is returned */
		
	int gdIndex = 0; 
	int mgdns = 0;
	struct pentry *newproc;
	int proc = q[rdytail].qprev;	
	while(proc != rdyhead){ 		
		newproc = &proctab[proc];
		if(newproc->gdns > mgdns){
			mgdns = newproc->gdns;
			gdIndex = proc; 		
		}
		proc = q[proc].qprev;
	}
	
	return gdIndex;
}

void updategcq(){					/* Updating the goodness counter and quantum of a process */
	
	struct pentry *newproc;
	int i=0;
	while(i < NPROC){
		newproc = &proctab[i];
		if(newproc->pstate != PRFREE){
			if(newproc->ctr == 0 || newproc->ctr == newproc->tquant){ /* proc thats never executed (or) proc that has never exhausted quantum */
				newproc->tquant = newproc->pprio;
			}
			else{
				newproc->tquant = (newproc->ctr)/2 + newproc->pprio;
			}
			newproc->ctr = newproc->tquant;
			newproc->gdns = newproc->ctr + newproc->pprio;
		}	

		i++;
	}
}  
