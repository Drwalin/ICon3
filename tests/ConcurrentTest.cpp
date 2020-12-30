
#include <cstdio>
#include <ctime>
#include <thread>
#include <chrono>
#include <functional>
#include <fstream>
#include <string>

#include <windows.h>
#include <conio.h>

#include <Concurrent.hpp>

#include <Benchmark.hpp>

#include <boost/lockfree/stack.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/lockfree/queue.hpp>
#include <stack>
#include <queue>
#include "thirdparty/concurrentqueue.h"

template<typename T>
class PoolBoost {
public:
	boost::lockfree::stack<T*> queue;
	PoolBoost() : queue(1) {}
	T* Pop() {
		T* ret = NULL;
		if(queue.pop(ret)) return ret;
		return new T;
	}
	void Release(T* ptr) { queue.push(ptr); }
};

template<typename T>
class PoolMoody {
public:
	moodycamel::ConcurrentQueue<T*> queue;
	PoolMoody() : queue(1) {}
	T* Pop() {
		T* ret = NULL;
		if(queue.try_dequeue(ret)) return ret;
		return new T;
	}
	void Release(T* ptr) { queue.enqueue(ptr); }
};

void benchmark_regular_increments_and_atomic();
void benchmark_spmc_queue();
void benchmark_pool();
void spcm_queue_validity_check();

concurrent::atomic<uint64_t> totalAllocated(0);
class Huge : public concurrent::ptr::node<Huge> {
public:
	Huge() {
		++totalAllocated;
	}
	uint8_t data[1024*64+123];
};

concurrent::ptr::pool<Huge> pool(100);









int main() {
	fprintf(stderr, "\n Benchmark running!");
	
	
	CALL_BENCHMARK_FOR_THREADS(1, 12, 5.0f,
			{
			},
			CREATE_ANONYMUS_BENCHMARK_CLASS("", 1000,
				struct {
					int a;
				},
				{
					const int count = 10;
					Huge* ptr[count];
					for(int i=0; i<count; ++i)
						ptr[i] = pool.get();
					for(int i=0; i<count; ++i)
						pool.release(ptr[i]);
					i += (count<<1)-1;
				}));
	
	
	concurrent::spmc_queue<uint64_t> queue;
	
	queue.set_allocation_batch(64*8192);
	{
		int beg = clock();
		for(uint64_t i=0; i<1000*1000*1000; ++i) {
			queue.push(i);
		}
		float t = (float)(clock()-beg)*0.001f;
		printf("\n pushed with allocation ~%llu elements in: %.2fs without concurrent pops", queue.capacity(), t);
		
		CALL_BENCHMARK_FOR_THREADS(3, 12, 0.5f,
				{
					shared.queue = &queue;
				},
				CREATE_ANONYMUS_BENCHMARK_CLASS("", 10000,
					struct {
						concurrent::spmc_queue<uint64_t> *queue;
					},
					{
						uint64_t value;
						if(!shared.queue->pop(value)) {
							I[thread_id] = i;
							goto __end;
						}
					}));
		
		uint64_t value;
		while(queue.pop(value)){
		}
		
		beg = clock();
		for(uint64_t i=0; i<queue.capacity(); ++i) {
			queue.push(i);
		}
		t = (float)(clock()-beg)*0.001f;
		printf("\n pushed ~%llu elements without concurrent pops in: %.2fs", queue.capacity(), t);
		
		while(queue.pop(value)){
		}
		
		CALL_BENCHMARK_FOR_THREADS(1, 12, 0.5f,
				{
					shared.queue = &queue;
					benchmark.count_only_first_thread = true;
				},
				CREATE_ANONYMUS_BENCHMARK_CLASS("counting only push with concurent pops", 10000,
					struct {
						concurrent::spmc_queue<uint64_t> *queue;
					},
					{
						if(thread_id==0) {
							shared.queue->push(i);
						} else {
							uint64_t value;
							if(shared.queue->pop(value)) {
							}
						}
					}));
		
	}
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	/*
	const int thr = 10;
	printf("\n what value to add: ");
	uint64_t incrementer;
	scanf("%llu", &incrementer);
	for(int thr=11; thr>0; --thr) {
//		std::thread(Consumer).detach();
//		std::thread(Producer).detach();
	
	endcount = 0;
	std::thread threads[thr];
	for(int i=0; i<thr; ++i) {
		threads[i] = std::thread(FunctionIncrementNormalInteger, i, incrementer);//Function);
	}
	
	Sleep(1000);
	int beg = clock();
	start = true;
	Sleep(5000);
	uint64_t _pop=pop.load(), _release=release.load(), _alloc=totalAllocated.load();
	uint64_t sum = 0;
	for(int i=0; i<thr; ++i) {
		sum += count[i]/incrementer;
	}
	start = false;
	int t = clock() - beg;
	float T = (float)(t) * 0.001f;
	
	for(int i=0; i<thr; ++i)
		threads[i].join();
	printf("\n Calculated %llu in %.2fs = %.2f Mop/s with %i threads",
			sum,
			T,
			(float)sum * 0.000001 / T,
			thr);
	*/
	
	/*
	Sleep(1000);
	int beg = clock();
	start = true;
	Sleep(5000);
	uint64_t _pop=pop.load(), _release=release.load(), _alloc=totalAllocated.load();
	int t = clock() - beg;
	start = false;
	printf("\n Done %f Mpop/s, %f Mrel/s", (float)_pop*0.001f/(float)(t),
			(float)_release*0.001f/(float)(t));
	printf("\n allocated: %llu / %llu pops", _alloc, _pop);
	*/
	
	return 0;
}


#define BATCH_SIZE 10000

void spmc_queue_validity_check() {
	concurrent::spmc_queue<uint64_t> queue;
	CALL_BENCHMARK_FOR_THREADS(2, 12, 5.0f,
			{
				for(int i=0; i<10; ++i) {
					queue.push(i);
				}
				shared.queue = &queue;
				
			},
			CREATE_ANONYMUS_BENCHMARK_CLASS("", 1000,
				struct {
					concurrent::spmc_queue<uint64_t> *queue;
				},
				{
					if(thread_id == 0) {
						shared.queue->push(i*threads+thread_id);
					} else {
						uint64_t value;
						
						shared.queue->pop(value);
					}
				}));
	printf("\n queue capacity: %llu", queue.capacity());
	concurrent::atomic<uint64_t> invalid(0);
	CALL_BENCHMARK_FOR_THREADS(2, 2, 5.0f,
			{
				uint64_t value;
				while(queue.pop(value)) {
				}
				shared.queue = &queue;
				shared.invalid = &invalid;
				shared.next_value = 0;
			},
			CREATE_ANONYMUS_BENCHMARK_CLASS("", 1000,
				struct {
					concurrent::spmc_queue<uint64_t> *queue;
					uint64_t next_value;
					concurrent::atomic<uint64_t> *invalid;
				},
				{
					if(thread_id==0) {
						shared.queue->push(i);
					} else {
						uint64_t value;
						for(int j=0; j<1000; ++j) {
							if(shared.queue->pop(value)) {
								if(shared.next_value != value) {
									(*shared.invalid)++;
									printf("\n %llu != %llu", shared.next_value, value);
								}
								shared.next_value = value+1;
								break;
							}
						}
					}
				}));
	printf("\n invalid count: %llu", invalid.load());
	
	uint64_t prev_value[256];
	CALL_BENCHMARK_FOR_THREADS(3, 12, 0.5f,
			{
				printf("\n invalid count (%llu): %llu", threads, invalid.load());
				invalid = 0;
				uint64_t value;
				while(queue.pop(value)) {
				}
				shared.queue = &queue;
				shared.invalid = &invalid;
				shared.prev_value = 0;
				shared.prev_value = prev_value;
				for(int i=1; i<threads; ++i) {
				prev_value[i] = 0;
				}
			},
			CREATE_ANONYMUS_BENCHMARK_CLASS("", 1000,
				struct {
					concurrent::spmc_queue<uint64_t> *queue;
					uint64_t *prev_value;
					concurrent::atomic<uint64_t> *invalid;
				},
				{
					if(thread_id==0) {
						shared.queue->push(i);
					} else {
						uint64_t value;
						for(int j=0; j<50; ++j) {
							if(shared.queue->pop(value)) {
								if(shared.prev_value[thread_id] >= value) {
									(*shared.invalid)++;
								}
								shared.prev_value[thread_id] = value;
								break;
							}
						}
					}
				}));
	printf("\n invalid count: %llu", invalid.load());
}

void benchmark_regular_increments_and_atomic() {
	uint64_t* _data = new uint64_t[4096*256];
	float testDuration = 5.0f;
	
	CALL_BENCHMARK_FOR_THREADS(1, 12, testDuration,
			shared.ptr = (decltype(shared.ptr))_data;,
			CREATE_ANONYMUS_BENCHMARK_CLASS("", BATCH_SIZE,
				struct{uint64_t * ptr;},
				shared.ptr[thread_id] += 1
				));
	
	CALL_BENCHMARK_FOR_THREADS(1, 12, testDuration,
			{shared.ptr = (decltype(shared.ptr))_data;},
			CREATE_ANONYMUS_BENCHMARK_CLASS("", BATCH_SIZE,
				struct{uint64_t * ptr;},
				shared.ptr[thread_id*8] += 1;
				));
	
	CALL_BENCHMARK_FOR_THREADS(1, 12, testDuration,
			{shared.ptr = (decltype(shared.ptr))_data;},
			CREATE_ANONYMUS_BENCHMARK_CLASS("", BATCH_SIZE,
				struct{volatile uint64_t * ptr;},
				shared.ptr[thread_id] += 1;
				));
	
	CALL_BENCHMARK_FOR_THREADS(1, 12, testDuration,
			{shared.ptr = (decltype(shared.ptr))_data;},
			CREATE_ANONYMUS_BENCHMARK_CLASS("", BATCH_SIZE,
				struct{volatile uint64_t * ptr;},
				shared.ptr[thread_id*8] += 1;
				));
	
	CALL_BENCHMARK_FOR_THREADS(1, 12, testDuration,
			{shared.ptr = (decltype(shared.ptr))_data;},
			CREATE_ANONYMUS_BENCHMARK_CLASS("", BATCH_SIZE,
				struct{volatile uint64_t * volatile ptr;},
				shared.ptr[thread_id] += 1;
				));
	
	CALL_BENCHMARK_FOR_THREADS(1, 12, testDuration,
			{shared.ptr = (decltype(shared.ptr))_data;},
			CREATE_ANONYMUS_BENCHMARK_CLASS("", BATCH_SIZE,
				struct{volatile uint64_t * volatile ptr;},
				shared.ptr[thread_id*8] += 1;
				));
	
	CALL_BENCHMARK_FOR_THREADS(1, 12, testDuration,
			{shared.ptr = (decltype(shared.ptr))_data;},
			CREATE_ANONYMUS_BENCHMARK_CLASS("", BATCH_SIZE,
				struct{std::atomic<uint64_t> * ptr;},
				shared.ptr[thread_id] += 1;
				));
	
	CALL_BENCHMARK_FOR_THREADS(1, 12, testDuration,
			{shared.ptr = (decltype(shared.ptr))_data;},
			CREATE_ANONYMUS_BENCHMARK_CLASS("", BATCH_SIZE,
				struct{concurrent::atomic<uint64_t> * ptr;},
				shared.ptr[thread_id*8] += 1;
				));
	
	CALL_BENCHMARK_FOR_THREADS(1, 12, testDuration,
			{shared.ptr = (decltype(shared.ptr))_data;},
			CREATE_ANONYMUS_BENCHMARK_CLASS("", BATCH_SIZE,
				struct{std::atomic<uint64_t> * ptr;},
				shared.ptr[thread_id*64] += 1;
				));
	
	CALL_BENCHMARK_FOR_THREADS(1, 12, testDuration,
			{shared.ptr = (decltype(shared.ptr))_data;},
			CREATE_ANONYMUS_BENCHMARK_CLASS("", BATCH_SIZE,
				struct{std::atomic<uint64_t> * ptr;},
					shared.ptr[0] += 1;
				));
	
	delete[] _data;
}

