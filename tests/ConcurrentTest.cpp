
#include <cstdio>
#include <ctime>
#include <thread>
#include <chrono>
#include <functional>
#include <fstream>
#include <string>

#include <windows.h>
#include <conio.h>

#include <Debug.hpp>

#include <Concurrent.hpp>

#define USE_CSV_OUTPUT
#include <Benchmark.hpp>

#include <boost/lockfree/stack.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/lockfree/queue.hpp>
#include <stack>
#include <queue>
#include "thirdparty/concurrentqueue.h"

class node_type : public concurrent::ptr::node<node_type> {
public:
};

size_t core_count = 10;
single_pool_multi_thread_equalizer<node_type> pools_equalizer;
single_pool<node_type>* pool = NULL;
size_t initial_data_amount_per_thread = 70*1000*1000;

template<typename T, typename cont>
class mpmc_boost {
public:
	
	mpmc_boost() : container(100000000) {}
	
	static const char* __name;
	
	~mpmc_boost() {
		uint64_t C=0;
		for(;;) {
			T* ptr = pop();
			if(ptr)
				pools_equalizer.pools[0].release(ptr);
			else
				break;
			++C;
		}
//		fprintf(stderr, "\n Deleted %llu on destructor of %s", C, __name);
	}
	
	inline T* pop() {
		T* value;
		if(container.pop(value))
			return value;
		return NULL;
	}
	
	inline void push(T* ptr) {
		while(!container.push(ptr)) {
		}
	}
	
	cont container;
};

template<>
const char* mpmc_boost<node_type, boost::lockfree::queue<node_type*>>::
		__name = "boost::lockfree::queue";
template<>
const char* mpmc_boost<node_type, boost::lockfree::stack<node_type*>>::
		__name = "boost::lockfree::stack";

template<typename T>
class mpmc_moodycamel_queue {
public:
	
	mpmc_moodycamel_queue() : queue(100000000) {}
	
	inline static const char* __name = "mpmc_moodycamel_queue";
	
	~mpmc_moodycamel_queue() {
		uint64_t C = 0;
		for(;;) {
			T* ptr = pop();
			if(ptr)
				pools_equalizer.pools[0].release(ptr);
			else
				break;
			++C;
		}
//		fprintf(stderr, "\n Deleted %llu on destructor of %s", C, __name);
	}
	
	inline T* pop() {
		T* value;
		if(queue.try_dequeue(value))
			return value;
		return NULL;
	}
	
	inline void push(T* ptr) {
		queue.enqueue(ptr);
	}
	
	moodycamel::ConcurrentQueue<T*> queue;
};

void benchmark_regular_increments_and_atomic();
void benchmark_spmc_queue();
void benchmark_pool();
void spcm_queue_validity_check();

void benchmark_different_pool_containers(float testTime);

/*
concurrent::atomic<uint64_t> totalAllocated(0);
class Huge : public concurrent::ptr::node<Huge> {
public:
	Huge() {
		++totalAllocated;
	}
	uint8_t data[1024*64+123];
};
*/
//concurrent::ptr::pool<Huge> pool(100);



int main() {
	fprintf(stderr, "\n Benchmark running!\n\n");
	benchmark_different_pool_containers(0.4f);
	
	pools_equalizer.equalize(0, 0);
	
	int t = clock();
	int ms = t%60000;
	t /= 60000;
	float s = ms*0.001f;
	int m = t % 60;
	int h = t / 60;
	
	fprintf(stderr, "\n Benchmarking done in %i:%i:%.2f", h, m, s);
	fprintf(stderr, "\n Type text and press enter to exit");
	char tmp[4096];
	scanf("%s", tmp);
	return 0;
}



#define BATCH_SIZE 10000
/*
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
				struct {concurrent::spmc_queue<uint64_t> *queue;},
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
				struct { concurrent::spmc_queue<uint64_t> *queue;
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
	
	fprintf(stderr, "\n");
	CALL_BENCHMARK_FOR_THREADS(1, core_count, testDuration,
			shared.ptr = (decltype(shared.ptr))_data;,
			CREATE_ANONYMUS_BENCHMARK_CLASS("", BATCH_SIZE,
				struct{uint64_t * ptr;},
				shared.ptr[thread_id] += 1
				));
	
	fprintf(stderr, "\n");
	CALL_BENCHMARK_FOR_THREADS(1, core_count, testDuration,
			{shared.ptr = (decltype(shared.ptr))_data;},
			CREATE_ANONYMUS_BENCHMARK_CLASS("", BATCH_SIZE,
				struct{uint64_t * ptr;},
				shared.ptr[thread_id*8] += 1;
				));
	
	fprintf(stderr, "\n");
	CALL_BENCHMARK_FOR_THREADS(1, core_count, testDuration,
			{shared.ptr = (decltype(shared.ptr))_data;},
			CREATE_ANONYMUS_BENCHMARK_CLASS("", BATCH_SIZE,
				struct{volatile uint64_t * ptr;},
				shared.ptr[thread_id] += 1;
				));
	
	fprintf(stderr, "\n");
	CALL_BENCHMARK_FOR_THREADS(1, core_count, testDuration,
			{shared.ptr = (decltype(shared.ptr))_data;},
			CREATE_ANONYMUS_BENCHMARK_CLASS("", BATCH_SIZE,
				struct{volatile uint64_t * ptr;},
				shared.ptr[thread_id*8] += 1;
				));
	
	fprintf(stderr, "\n");
	CALL_BENCHMARK_FOR_THREADS(1, core_count, testDuration,
			{shared.ptr = (decltype(shared.ptr))_data;},
			CREATE_ANONYMUS_BENCHMARK_CLASS("", BATCH_SIZE,
				struct{volatile uint64_t * volatile ptr;},
				shared.ptr[thread_id] += 1;
				));
	
	fprintf(stderr, "\n");
	CALL_BENCHMARK_FOR_THREADS(1, core_count, testDuration,
			{shared.ptr = (decltype(shared.ptr))_data;},
			CREATE_ANONYMUS_BENCHMARK_CLASS("", BATCH_SIZE,
				struct{volatile uint64_t * volatile ptr;},
				shared.ptr[thread_id*8] += 1;
				));
	
	fprintf(stderr, "\n");
	CALL_BENCHMARK_FOR_THREADS(1, core_count, testDuration,
			{shared.ptr = (decltype(shared.ptr))_data;},
			CREATE_ANONYMUS_BENCHMARK_CLASS("", BATCH_SIZE,
				struct{std::atomic<uint64_t> * ptr;},
				shared.ptr[thread_id] += 1;
				));
	
	fprintf(stderr, "\n");
	CALL_BENCHMARK_FOR_THREADS(1, core_count, testDuration,
			{shared.ptr = (decltype(shared.ptr))_data;},
			CREATE_ANONYMUS_BENCHMARK_CLASS("", BATCH_SIZE,
				struct{concurrent::atomic<uint64_t> * ptr;},
				shared.ptr[thread_id*8] += 1;
				));
	
	fprintf(stderr, "\n");
	CALL_BENCHMARK_FOR_THREADS(1, core_count, testDuration,
			{shared.ptr = (decltype(shared.ptr))_data;},
			CREATE_ANONYMUS_BENCHMARK_CLASS("", BATCH_SIZE,
				struct{std::atomic<uint64_t> * ptr;},
				shared.ptr[thread_id*64] += 1;
				));
	
	fprintf(stderr, "\n");
	CALL_BENCHMARK_FOR_THREADS(1, core_count, testDuration,
			{shared.ptr = (decltype(shared.ptr))_data;},
			CREATE_ANONYMUS_BENCHMARK_CLASS("", BATCH_SIZE,
				struct{std::atomic<uint64_t> * ptr;},
					shared.ptr[0] += 1;
				));
	
	delete[] _data;
}
*/


template<typename T>
void clear(T* container) {
	for(short i=0;; ++i) {
		if(i==0)
			pools_equalizer.sort();
		auto ptr = container->pop();
		if(!ptr)
			break;
		pools_equalizer.release(ptr);
	}
}

template<bool yield_condition>
class benchmark_yield {
public:
	
	
	template<typename T>
	void benchmark_count_only_push_while_pop(T* container, size_t threads_min,
			size_t threads_max, float testTime) {
		CALL_BENCHMARK_FOR_THREADS(threads_min, threads_max, testTime,
			{
				clear<T>(container);
				shared.container = container;
				benchmark.count_only_first_thread = true;
				pool = pools_equalizer.get_pools(core_count,
						initial_data_amount_per_thread);
				if(yield_condition) {CSV_VALUE("yield");}
				else {CSV_VALUE("noyield");}
				CSV_VALUE(T::__name);
			},
			CREATE_ANONYMUS_BENCHMARK_CLASS("push while pop", 10000,
				struct { T *container; },
				{
					if(yield_condition)
						std::this_thread::yield();
					if(thread_id == 0) {
						shared.container->push(pool[thread_id].get());
					} else {
						node_type* v = shared.container->pop();
						if(v) pool[thread_id].release(v);
						else {break;}
					}
				}));
	}
	
	template<typename T>
	void benchmark_count_only_pop_while_push(T* container, size_t threads_min,
			size_t threads_max, float testTime) {
		CALL_BENCHMARK_FOR_THREADS(threads_min, threads_max, testTime,
			{
				clear<T>(container);
				shared.container = container;
				benchmark.count_only_first_thread = true;
				pools_equalizer.sort();
				for(size_t i=0; i<50000000; ++i)
					container->push(pools_equalizer.get());
				pool = pools_equalizer.get_pools(core_count,
						initial_data_amount_per_thread);
				if(yield_condition) {CSV_VALUE("yield");}
				else {CSV_VALUE("noyield");}
				CSV_VALUE(T::__name);
			},
			CREATE_ANONYMUS_BENCHMARK_CLASS("pop while push", 10000,
				struct { T *container; },
				{
					if(yield_condition)
						std::this_thread::yield();
					if(thread_id == 0) {
						node_type* v = shared.container->pop();
						if(v) pool[thread_id].release(v);
						//else {++i;break;}
					} else {
						shared.container->push(pool[thread_id].get());
					}
				}));
	}
	
	template<typename T>
	void benchmark_test_for_pushes_and_pops_equal(T* container,
			size_t threads_min, size_t threads_max, float testTime) {
		CALL_BENCHMARK_FOR_THREADS(threads_min, threads_max, testTime,
			{
				clear<T>(container);
				shared.container = container;
				pools_equalizer.sort();
				for(size_t i=0; i<50000000; ++i)
					container->push(pools_equalizer.get());
				pool = pools_equalizer.get_pools(core_count,
						initial_data_amount_per_thread);
				if(yield_condition) {CSV_VALUE("yield");}
				else {CSV_VALUE("noyield");}
				CSV_VALUE(T::__name);
			},
			CREATE_ANONYMUS_BENCHMARK_CLASS("push and pop", 10000,
				struct { T *container; },
				{
					if(yield_condition)
						std::this_thread::yield();
					if(thread_id&1) {
						shared.container->push(pool[thread_id].get());
					} else {
						node_type* v = shared.container->pop();
						if(v) pool[thread_id].release(v);
						//else {break;}
					}
				}));
	}
	
	template<typename T>
	void benchmark_test_for_only_push(T* container, size_t threads_min,
			size_t threads_max, float testTime) {
		CALL_BENCHMARK_FOR_THREADS(threads_min, threads_max, testTime,
			{
				clear<T>(container);
				shared.container = container;
				pool = pools_equalizer.get_pools(core_count,
						initial_data_amount_per_thread);
				if(yield_condition) {CSV_VALUE("yield");}
				else {CSV_VALUE("noyield");}
				CSV_VALUE(T::__name);
			},
			CREATE_ANONYMUS_BENCHMARK_CLASS("only push", 1000,
				struct { T *container; },
				{
					if(yield_condition)
						std::this_thread::yield();
					if(pool[thread_id].size() == 0) {
						goto __end;
					}
					shared.container->push(pool[thread_id].get());
				}));
	}
	
	template<typename T>
	void benchmark_test_for_only_pop(T* container, size_t threads_min,
			size_t threads_max, float testTime) {
		CALL_BENCHMARK_FOR_THREADS(threads_min, threads_max, testTime,
			{
				clear<T>(container);
				shared.container = container;
				pools_equalizer.sort();
				for(size_t i=0; i<50000000; ++i)
					container->push(pools_equalizer.get());
				pool = pools_equalizer.get_pools(core_count,
						initial_data_amount_per_thread);
				if(yield_condition) {CSV_VALUE("yield");}
				else {CSV_VALUE("noyield");}
				CSV_VALUE(T::__name);
			},
			CREATE_ANONYMUS_BENCHMARK_CLASS("only pop", 10000,
				struct { T *container; },
				{
					if(yield_condition)
						std::this_thread::yield();
					node_type* v = shared.container->pop();
					if(v) pool[thread_id].release(v);
					else {
						goto __end;
					}
				}));
	}
	
	
	
	template<typename type>
	void benchmark_mpsc_container(bool require_realloc, float testTime) {
		type container;
		if(require_realloc) {
			pools_equalizer.sort();
			for(size_t i=0; i<100000000; ++i)
				container.push(pools_equalizer.get());
			clear<type>(&container);
		}
		fprintf(stderr, "\n\n\n                  MPSC benchmark: %s",
				type::__name);
		benchmark_count_only_push_while_pop<type>(&container, 2, 2, testTime);
		benchmark_count_only_pop_while_push<type>(&container, 2, core_count,
				testTime);
		fprintf(stderr, "\n\n    test only push -> only pop");
		for(int i=1; i<=core_count; ++i) {
			benchmark_test_for_only_push<type>(&container, i, i, testTime);
			benchmark_test_for_only_pop<type>(&container, 1, 1, testTime*i);
			fprintf(stderr, "\n");
		}
		clear<type>(&container);
	}
	
	template<typename type>
	void benchmark_mpmc_container(bool require_realloc, float testTime) {
		type container;
		if(require_realloc) {
			pools_equalizer.sort();
			for(size_t i=0; i<100000000; ++i)
				container.push(pools_equalizer.get());
			clear<type>(&container);
		}
		fprintf(stderr, "\n\n\n                  MPMC benchmark: %s",
				type::__name);
		benchmark_count_only_push_while_pop<type>(&container, 2, core_count,
				testTime);
		benchmark_count_only_pop_while_push<type>(&container, 2, core_count,
				testTime);
		benchmark_test_for_pushes_and_pops_equal<type>(&container, 2,
				core_count, testTime);
		fprintf(stderr, "\n\n    test only push -> only pop");
		for(int i=1; i<=core_count; ++i) {
			benchmark_test_for_only_push<type>(&container, i, i, testTime);
			benchmark_test_for_only_pop<type>(&container, i, i, testTime);
			fprintf(stderr, "\n");
		}
		clear<type>(&container);
	}
	
	template<typename type>
	void benchmark_spmc_container(bool require_realloc, float testTime) {
		type container;
		if(require_realloc) {
			pools_equalizer.sort();
			for(size_t i=0; i<100000000; ++i)
				container.push(pools_equalizer.get());
			clear<type>(&container);
		}
		fprintf(stderr, "\n\n\n                  SPMC benchmark: %s",
				type::__name);
		benchmark_count_only_push_while_pop<type>(&container, 2, core_count,
				testTime);
		benchmark_count_only_pop_while_push<type>(&container, 2, 2, testTime);
		fprintf(stderr, "\n\n    test only push -> only pop");
		for(int i=1; i<=core_count; ++i) {
			benchmark_test_for_only_push<type>(&container, 1, 1, testTime*i);
			benchmark_test_for_only_pop<type>(&container, i, i, testTime);
			fprintf(stderr, "\n");
		}
		clear<type>(&container);
	}
	
	
	void benchmark_different_pool_containers(float testTime) {
		if(yield_condition)
			printf("\n     Benchmark with yield()");
		else
			printf("\n     Benchmark without yield()");
		
		pool = pools_equalizer.get_pools(core_count,
				initial_data_amount_per_thread);
		CSV_LINE();
		benchmark_mpsc_container<
			concurrent::ptr::mpsc_stack<node_type>>(false, testTime);
		CSV_LINE();
		benchmark_mpsc_container<
			concurrent::ptr::mpsc_queue<node_type>>(false, testTime);
		
		CSV_LINE();
		benchmark_mpmc_container<
			concurrent::ptr::mpmc_stack<node_type>>(false, testTime);
		CSV_LINE();
		benchmark_mpmc_container<
			concurrent::ptr::mpmc_queue_lock<node_type>>(false, testTime);
		CSV_LINE();
		benchmark_mpmc_container<
			concurrent::ptr::mpmc_stackqueue<node_type>>(false, testTime);
		CSV_LINE();
		benchmark_mpmc_container<
			concurrent::ptr::mpmc_stackqueue_lock<node_type>>(false, testTime);
		
		CSV_LINE();
		benchmark_mpmc_container<mpmc_boost<node_type,
			boost::lockfree::queue<node_type*>>>(true, testTime);
		CSV_LINE();
		benchmark_mpmc_container<mpmc_boost<node_type,
			boost::lockfree::stack<node_type*>>>(true, testTime);
		CSV_LINE();
		benchmark_mpmc_container<mpmc_moodycamel_queue<node_type>>(true,
				testTime);
		
		CSV_LINE();
		benchmark_spmc_container<
			concurrent::ptr::spmc_queue_node<node_type>>(true, testTime);
	}
	
	benchmark_yield(float testTime) {
		benchmark_different_pool_containers(testTime);
	}
};

void benchmark_different_pool_containers(float testTime) {
	CSV_LINE();
	CSV_VALUE("id");
	CSV_VALUE("title");
	CSV_VALUE("yield/noyield");
	CSV_VALUE("type");
	CSV_VALUE("threads");
	CSV_VALUE("Mop/s");
	CSV_VALUE("counted ops");
	CSV_VALUE("total ops");
	CSV_VALUE("duration");
	CSV_LINE();
	{benchmark_yield<false> b(testTime);};
	//{benchmark_yield<true> a(testTime);};
}

