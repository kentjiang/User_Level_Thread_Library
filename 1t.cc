#include "1t.h"
#include <ucontext.h>
#include <map>
#include <queue>
#include <stdlib.h>
#include <cstdlib>
#include <stdio.h>
#include <iostream>
#include <assert.h>
#include "interrupt.h"
using namespace std;

bool ifinited = false;
typedef ucontext_t* ucontext_ptr;
queue<ucontext_ptr> thread_q;
ucontext_ptr parent_thread, current_thread, init_thread;
struct semaphore {
	unsigned int ID;
	unsigned int value;
	queue<ucontext_ptr> wait_q;
	semaphore(unsigned int _ID, unsigned int _value) : ID(_ID), value(_value) {}
};
map<int, semaphore*> sem_map;

int dthreads_start(dthreads_func_t func, void *arg);

void mask_function(dthreads_func_t func, void *arg){
	std::cout << "Into Mask" << std::endl;
	interrupt_enable();
	assert_interrupts_enabled();
	func(arg);
	current_thread->uc_stack.ss_flags = 1;
	interrupt_disable();
	assert_interrupts_disabled();
	swapcontext(current_thread, init_thread);
}
int dthreads_init(dthreads_func_t func, void *arg){
	std::cout << "Start Init" << std::endl;
	interrupt_disable();
	assert_interrupts_disabled();

	if (ifinited){
		interrupt_enable();
		assert_interrupts_enabled();
		return -1;
	}
	ifinited = true;
	current_thread = new ucontext_t;

	init_thread = new ucontext_t;
	if (getcontext(init_thread) == 0){
		char *stack = new char [STACK_SIZE];
    	init_thread->uc_stack.ss_sp = stack;
    	init_thread->uc_stack.ss_size = STACK_SIZE;
    	init_thread->uc_stack.ss_flags = 0;
    	init_thread->uc_link = NULL;
	}
	else{
		std::cout << "Init init_thread Error" << std::endl;
		return -1;
	}
	makecontext(init_thread, (void (*)()) dthreads_init, 2, func, arg);

	interrupt_enable();
	assert_interrupts_enabled();
	
	dthreads_start(func, arg);

	
	std::cout << "Into Loop" << std::endl;

    while (!thread_q.empty()){
    	current_thread = thread_q.front();
    	thread_q.pop();
    	interrupt_disable();
    	assert_interrupts_disabled();
    	std::cout << "Swap to Another Thread" << std::endl;
    	swapcontext(init_thread, current_thread);
    	if (current_thread->uc_stack.ss_flags){
    		delete[] (char*)current_thread->uc_stack.ss_sp;
    		delete current_thread;
    	}
    	std::cout << "Back to Init" << std::endl;
    	interrupt_enable();
    	assert_interrupts_enabled();
    }

    std::cout << "Out of While Loop" << std::endl;
    std::cout << "Thread library exiting.\n";
    exit(0);
}

int dthreads_start(dthreads_func_t func, void *arg){
	interrupt_disable();
	assert_interrupts_disabled();

	ucontext_ptr new_thread = new ucontext_t;
	if (!ifinited){
		interrupt_enable();
		assert_interrupts_enabled();
		return -1;
	}
	if (getcontext(new_thread) == 0){
		char *stack = new char [STACK_SIZE];
    	new_thread->uc_stack.ss_sp = stack;
    	new_thread->uc_stack.ss_size = STACK_SIZE;
    	new_thread->uc_stack.ss_flags = 0;
    	new_thread->uc_link = NULL;
	}
	else{
		std::cout << "start error" << std::endl;
		return -1;
	}
	makecontext(new_thread, (void (*)()) mask_function, 2, func, arg);
	thread_q.push(new_thread);
	std::cout << "Thread Created" << std::endl;
	interrupt_enable();
	assert_interrupts_enabled();
	return 0;
}

int dthreads_yield(){
	interrupt_disable();
	assert_interrupts_disabled();
	thread_q.push(current_thread);
	swapcontext(current_thread, init_thread);
	interrupt_enable();
	assert_interrupts_enabled();
	return 0;
}

int dthreads_seminit(unsigned int sem, unsigned int value){
	interrupt_disable();
	assert_interrupts_disabled();
	if (!ifinited){
		std::cout << "Not Inited before Seminit" << std::endl;
		interrupt_enable();
		assert_interrupts_enabled();
		return -1;
	}
	std::cout << "Into Sem Init" << std::endl;
	semaphore* new_sem = new semaphore(sem, value);
	sem_map.insert(pair<unsigned int, semaphore*>(sem, new_sem));
	interrupt_enable();
	assert_interrupts_enabled();
	return 0;
}

int dthreads_semup(unsigned int sem){
	interrupt_disable();
	assert_interrupts_disabled();
	std::cout << "Sem Up" << std::endl;
	semaphore* current_sem = sem_map[sem];
	if (current_sem->wait_q.empty()){
		current_sem->value++;
	}else{
		ucontext_ptr out_of_waitq_thread = current_sem->wait_q.front();
		current_sem->wait_q.pop();
		thread_q.push(out_of_waitq_thread);
	}
	interrupt_enable();
	assert_interrupts_enabled();
	return 0;
}

int dthreads_semdown(unsigned int sem){
	std::cout << "Sem Down" << std::endl;
	interrupt_disable();
	assert_interrupts_disabled();
	semaphore* current_sem = sem_map[sem];
	if (current_sem->value <= 0){
		current_sem->wait_q.push(current_thread);
		swapcontext(current_thread, init_thread);
	}
	else{
		current_sem->value--;
	}
	interrupt_enable();
	assert_interrupts_enabled();
	return 0;
}