#include <pthread.h>
#include "thread.hpp"
#include "ts_queue.hpp"
#include "item.hpp"
#include "transformer.hpp"

#ifndef PRODUCER_HPP
#define PRODUCER_HPP

class Producer : public Thread {
public:
	// constructor
	Producer(TSQueue<Item*>* input_queue, TSQueue<Item*>* worker_queue, Transformer* transfomrer);

	// destructor
	~Producer();

	virtual void start();
private:
	// the expected lines to write,
	// the writer thread finished after output expected lines of item
	int expected_lines;

	TSQueue<Item*>* input_queue;
	TSQueue<Item*>* worker_queue;

	Transformer* transformer;

	// the method for pthread to create a producer thread
	static void* process(void* arg);
};

Producer::Producer(TSQueue<Item*>* input_queue, TSQueue<Item*>* worker_queue, Transformer* transformer)
	: input_queue(input_queue), worker_queue(worker_queue), transformer(transformer) {
}

Producer::~Producer() {}

void Producer::start() {
	// TODO: starts a Producer thread
	pthread_create(&t, 0, Producer::process, (void*)this);
}

void* Producer::process(void* arg) {
	// TODO: implements the Producer's work
	Producer *producer = (Producer*)arg;

	while(true){
		Item *item = producer->input_queue->dequeue();
		item->val = producer->transformer->producer_transform(item->opcode, item->val); //why? 甚麼是opcode
		producer->worker_queue->enqueue(item);
	}
	//add
	// delete producer;
	return nullptr; //why?為甚麼return nullptr
}

#endif // PRODUCER_HPP
