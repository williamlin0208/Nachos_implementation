// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextToRun(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "debug.h"
#include "scheduler.h"
#include "main.h"
#include <vector>

#define min(a, b) a<b?a:b

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads.
//	Initially, no ready threads.
//----------------------------------------------------------------------


// hw3 for SortedList
int comp(Thread *a, Thread *b){
    int aLevel = a->getLevel(); int bLevel = b->getLevel();
    if(aLevel != bLevel) return aLevel - bLevel;

    // a and b has same level
    int level = aLevel;
    if(level == 1){
        return a->getRemainingTime() - b->getRemainingTime();
    }else if(level == 2){
        return a->getPriority() - b->getPriority();
    }else if(level == 3){
        // do nothing
    }
    return 1;
}
    
Scheduler::Scheduler()
{ 
    //readyList = new List<Thread *>; 
    
    readyList = new SortedList<Thread *>(&comp);
    toBeDestroyed = NULL;
    timeQuantumExpired = false;
} 

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{ 
    delete readyList; 
} 

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
Scheduler::ReadyToRun (Thread *thread)
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    DEBUG(dbgThread, "Putting thread on ready list: " << thread->getName());
	//cout << "Putting thread on ready list: " << thread->getName() << endl ;
    thread->setStatus(READY);

    // hw3
    // determine the level of the thread before we store it to the ready list
    int priority = thread->getPriority();
    ASSERT(priority >= 0 && priority < 150);
    if(priority < 50){
        thread->setLevel(3);
    }else if(priority < 100){
        thread->setLevel(2);
    }else if(priority < 150){
        thread->setLevel(1);
    }

    readyList->Insert(thread); 
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread *
Scheduler::FindNextToRun ()
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    if (readyList->IsEmpty()) {
		return NULL;
    } else {
    	return readyList->RemoveFront();
    }
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable kernel->currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//	"finishing" is set if the current thread is to be deleted
//		once we're no longer running on its stack
//		(when the next thread starts running)
//----------------------------------------------------------------------

void
Scheduler::Run (Thread *nextThread, bool finishing)
{
    Thread *oldThread = kernel->currentThread;
    
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    if (finishing) {	// mark that we need to delete current thread
        ASSERT(toBeDestroyed == NULL);
	    toBeDestroyed = oldThread;
    }
    
    if (oldThread->space != NULL) {	// if this thread is a user program,
        oldThread->SaveUserState(); 	// save the user's CPU registers
	    oldThread->space->SaveState();
    }
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow

    kernel->currentThread = nextThread;  // switch to the next thread
    nextThread->setStatus(RUNNING);      // nextThread is now running

    // hw3
    // reset waiting time(aging) to 0 as thread turns into running state
    nextThread->setWaitingTime(0);

    DEBUG(dbgThread, "Switching from: " << oldThread->getName() << " to: " << nextThread->getName());
    
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    SWITCH(oldThread, nextThread);

    // we're back, running oldThread
      
    // interrupts are off when we return from switch!
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    DEBUG(dbgThread, "Now in thread: " << oldThread->getName());

    CheckToBeDestroyed();		// check if thread we were running
					// before this one has finished
					// and needs to be cleaned up
    
    if (oldThread->space != NULL) {	    // if there is an address space
        oldThread->RestoreUserState();     // to restore, do it.
	oldThread->space->RestoreState();
    }
}

//----------------------------------------------------------------------
// Scheduler::CheckToBeDestroyed
// 	If the old thread gave up the processor because it was finishing,
// 	we need to delete its carcass.  Note we cannot delete the thread
// 	before now (for example, in Thread::Finish()), because up to this
// 	point, we were still running on the old thread's stack!
//----------------------------------------------------------------------

void
Scheduler::CheckToBeDestroyed()
{
    if (toBeDestroyed != NULL) {
        delete toBeDestroyed;
	toBeDestroyed = NULL;
    }
}
 
//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
Scheduler::Print()
{
    cout << "Ready list contents:\n";
    readyList->Apply(ThreadPrint);
}

// hw3
// check if the thread can be preempted
// lv1: check if there is certain thread that has less remaining time
// lv2: cannot be preempted
// lv3: if front thread in ready list have higher priority, it must be preempted
//      else check if the time quantum expired.
bool
Scheduler::CheckPreempted(){
    Statistics *stats = kernel->stats;
    
    Thread *currentThread = kernel->currentThread;
    int level = currentThread->getLevel();

    Thread *frontThread = readyList->Front();
    if(level == 1){
        return frontThread->getRemainingTime() < currentThread->getRemainingTime();
    }else if(level == 2){
        return false; // can not be preempted
    }else if(level == 3){
        if(frontThread->getLevel() == 1 || frontThread->getLevel() == 2){
            return true;
        }else if(frontThread->getLevel() == 3){
            return timeQuantumExpired;
        }
    }
}

// hw3
void AddWaitingByOne(Thread *thread){
    thread->setWaitingTime(thread->getWaitingTime() + 1);
}
// hw3
void
Scheduler::IncreaseWaiting(int val){
    for(int i=0;i<val;i++) readyList->Apply(&AddWaitingByOne);
}

// hw3
// 1. clean up the ready list
// 2. check every thread in ready list and update their priority and waiting time
// 3. update the level of every thread
// 4. re-insert threads to ready list
void
Scheduler::UpdatePriority(){
    vector<Thread *> *tempV;

    // clean up threads in ready list
    while(!readyList->IsEmpty()){
        Thread *tempThread = readyList->RemoveFront();
        int waitingTime = tempThread->getWaitingTime();
        // check every thread
        // if thread's waiting time in ready list exceed 1500 ticks, priority +10
        while(waitingTime > 1500){
            tempThread->setWaitingTime(waitingTime - 1500);
            tempThread->setPriority(min(tempThread->getPriority() + 10, 149)); // valid value is 0~149
            waitingTime = tempThread->getWaitingTime();
        }

        // update the level
        int priority = tempThread->getPriority();
        ASSERT(priority < 150 && priority >=0);
        if(priority < 50){
            tempThread->setLevel(3);
        }else if(priority < 100){
            tempThread->setLevel(2);
        }else if(priority < 150){
            tempThread->setLevel(1);
        }
        tempV->push_back(tempThread);
    }

    // re-insert threads into ready list
    while(!tempV->empty()){
        readyList->Insert(tempV->back());
        tempV->pop_back();
    }

}

void Scheduler::SetTimeQuantumExpired(bool val){
    timeQuantumExpired = val;
}