#include <pthread.h>
#include <unistd.h>
#include <vector>
#include <iostream>
#include "consumer.hpp"
#include "ts_queue.hpp"
#include "item.hpp"
#include "transformer.hpp"

#ifndef CONSUMER_CONTROLLER
#define CONSUMER_CONTROLLER

class ConsumerController : public Thread {
public:
	// constructor
	ConsumerController(
		TSQueue<Item*>* worker_queue,
		TSQueue<Item*>* writer_queue,
		Transformer* transformer,
		int check_period,
		int low_threshold,
		int high_threshold
	);

	// destructor
	~ConsumerController();

	virtual void start();

private:
	std::vector<Consumer*> consumers;

	TSQueue<Item*>* worker_queue;
	TSQueue<Item*>* writer_queue;

	Transformer* transformer;

	// Check to scale down or scale up every check period in microseconds.
	int check_period;
	// When the number of items in the worker queue is lower than low_threshold,
	// the number of consumers scaled down by 1.
	int low_threshold;
	// When the number of items in the worker queue is higher than high_threshold,
	// the number of consumers scaled up by 1.
	int high_threshold;

	static void* process(void* arg);
};

// Implementation start

ConsumerController::ConsumerController(
	TSQueue<Item*>* worker_queue,
	TSQueue<Item*>* writer_queue,
	Transformer* transformer,
	int check_period,
	int low_threshold,
	int high_threshold
) : worker_queue(worker_queue),
	writer_queue(writer_queue),
	transformer(transformer),
	check_period(check_period),
	low_threshold(low_threshold),
	high_threshold(high_threshold) {
}

ConsumerController::~ConsumerController() {}

void ConsumerController::start() {
	// TODO: starts a ConsumerController thread
	pthread_create(&t, 0, ConsumerController::process, (void*)this); //this=arg pointer
}

void* ConsumerController::process(void* arg) {
	// TODO: implements the ConsumerController's work
	ConsumerController *consumerController = (ConsumerController*)arg; //why?

	int workerLoading;

	while(true){
		usleep(consumerController->check_period);
		int size = consumerController->consumers.size();
		//add
		//std::cout <<"consumer size"<<consumerController->consumers.size()<<std::endl;
		int workerLoading = consumerController->worker_queue->get_size(); 
		if(workerLoading > consumerController->high_threshold){      // loading is higher than high threshold
			consumerController->consumers.push_back(new Consumer(consumerController->worker_queue, consumerController->writer_queue, consumerController->transformer));
			consumerController->consumers.back()->start(); //why?
			std::cout<<"Scaling up consumers from "<<size<<" to "<<size+1<<"\n";
		}else if(workerLoading < consumerController->low_threshold){ // loading is lower than low threshold
			// Ensure there is at least one thread being executed
			if(size >= 2){ //why?
				if(consumerController->consumers.back()->cancel()){//讓pthread停止
					std::cout<<"Scaling down consumeres from "<<size<<" to "<<size-1<<"\n";
				}
				delete consumerController->consumers.back(); //why?有cancel還要再delete // destructor ~Consumer(); 刪除指標指到東西
				consumerController->consumers.pop_back(); //why? consumers為vector，pointer刪掉 
			}
		}
		//add
		//usleep(consumerController->check_period);
	}
	
	return nullptr;
}

#endif // CONSUMER_CONTROLLER_HPP
