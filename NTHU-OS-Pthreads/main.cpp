#include <assert.h>
#include <stdlib.h>
#include <vector>
#include "ts_queue.hpp"
#include "item.hpp"
#include "reader.hpp"
#include "writer.hpp"
#include "producer.hpp"
#include "consumer_controller.hpp"
#include <chrono>

#define NUM_PRODUCERS 4   //4
#define READER_QUEUE_SIZE 200    //200
#define WORKER_QUEUE_SIZE 200    //200
#define WRITER_QUEUE_SIZE 4000   //4000
#define CONSUMER_CONTROLLER_LOW_THRESHOLD_PERCENTAGE 20  //20
#define CONSUMER_CONTROLLER_HIGH_THRESHOLD_PERCENTAGE 80  //80
#define CONSUMER_CONTROLLER_CHECK_PERIOD 1000000 //1000000

int main(int argc, char** argv) {
	assert(argc == 4);
	auto start_time = std::chrono::high_resolution_clock::now();  // Record the start time
	int n = atoi(argv[1]);
	std::string input_file_name(argv[2]);
	std::string output_file_name(argv[3]);

	// TODO: implements main function

	Transformer *transformer = new Transformer;

	TSQueue<Item*> *input_q = new TSQueue<Item*>(READER_QUEUE_SIZE);
	TSQueue<Item*> *worker_q = new TSQueue<Item*>(WORKER_QUEUE_SIZE);
	TSQueue<Item*> *output_q = new TSQueue<Item*>(WRITER_QUEUE_SIZE);

	Reader *reader = new Reader(n, input_file_name, input_q);
	Writer *writer = new Writer(n, output_file_name, output_q);

	std::vector<Producer*> producers;
	for(int i=0;i<NUM_PRODUCERS;i++){
		producers.emplace_back(new Producer(input_q, worker_q, transformer));
	}

	int low_threshold = CONSUMER_CONTROLLER_LOW_THRESHOLD_PERCENTAGE*WORKER_QUEUE_SIZE/100;
	int high_threshold = CONSUMER_CONTROLLER_HIGH_THRESHOLD_PERCENTAGE*WORKER_QUEUE_SIZE/100;
	ConsumerController *consumer_controller = new ConsumerController(worker_q, output_q, transformer, CONSUMER_CONTROLLER_CHECK_PERIOD, low_threshold, high_threshold);

	reader->start();
	writer->start();

	for(int i=0;i<NUM_PRODUCERS;i++){
		producers[i]->start();
	}

	consumer_controller->start();

	reader->join();
	writer->join();

	delete reader;
	delete writer;
	delete transformer;
	delete input_q;
	delete output_q;
	delete worker_q;
	//add
	// delete consumer_controller;
	// for(int i=0;i<NUM_PRODUCERS;i++){
	// 	delete producers[i];
	// }
	auto end_time = std::chrono::high_resolution_clock::now();  // Record the end time

    // Calculate the duration and print the result
	auto duration_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count();
    std::cout << "Execution time: " << duration_seconds << " seconds" << std::endl;
	return 0;
}
