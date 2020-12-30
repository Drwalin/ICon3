/*
 *  Benchmarking tool for multi-threaded tasks.
 *  Copyright (C) 2020 Marek Zalewski aka Drwalin
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef BENCHMARK_HPP
#define BENCHMARK_HPP

#include <cstdio>
#include <cinttypes>

#include <thread>
#include <atomic>
#include <chrono>

#include <Concurrent.hpp>

#define __TO_STRING_MACRO_EXPRESSION(__string) \
	__TO_STRING_MACRO_EXPRESSION2(__string)
#define __TO_STRING_MACRO_EXPRESSION2(__string) \
	#__string

#define __BENCHMARK_LOOP_CODE(__task, __batch_size) \
		for(uint64_t i=0; doNotStop; I[thread_id]=i) { \
			uint64_t end = i+__batch_size; \
			for(;i<end;++i) { \
				__task; \
			} \
		}

#define CREATE_ANONYMUS_BENCHMARK_CLASS_WITH_LOCAL(__title,\
		__batch_size, \
		__shared_data_type,\
		__thread_local_data_type, \
		__thread_local_data_init, \
		__task) \
class { \
	concurrent::atomic<bool> start; \
	volatile bool doNotStop; \
	concurrent::atomic<size_t> done; \
	size_t threads; \
	uint64_t *I; \
	void loop(int thread_id) { \
		__thread_local_data_type local; \
		__thread_local_data_init; \
		while(start.load() == false) { \
		} \
		I[thread_id]=0; \
		__BENCHMARK_LOOP_CODE(__task, __batch_size) \
		__end: \
		done++; \
	} \
public: \
	bool render_header=true; \
	bool count_only_first_thread=false; \
	__shared_data_type shared; \
	double run(float test_duration_seconds, size_t _threads) { \
		doNotStop = true; \
		threads = _threads; \
		I = new uint64_t[threads]; \
		start = false; \
		done = 0; \
		for(size_t i=0; i<threads; ++i) { \
			std::thread(&loop, this, i).detach(); \
		} \
		std::this_thread::sleep_for(std::chrono::milliseconds(100)); \
		std::chrono::high_resolution_clock::time_point begin = \
			std::chrono::high_resolution_clock::now(); \
		start = true; \
		std::this_thread::sleep_for( \
				std::chrono::nanoseconds( \
					(uint64_t)(test_duration_seconds*1000ll*1000ll*1000ll))); \
		doNotStop = false; \
		std::chrono::high_resolution_clock::time_point end = \
			std::chrono::high_resolution_clock::now(); \
		uint64_t sum=0; \
		if(count_only_first_thread) \
			sum = I[0]; \
		else \
			for(size_t j=0; j<threads; ++j) { \
				sum += I[j]; \
			} \
		while(done.load() != threads) \
			std::this_thread::yield(); \
		std::chrono::duration<double> duration = \
			std::chrono::duration<double>(end - begin); \
		if(render_header) \
			print_header(); \
		fprintf(stderr, \
				"\n         %llu threads %4.2f Mop/s (%llu in %.2fs)", \
				(uint64_t)threads, \
				(double)(sum) / duration.count() * 0.000001, \
				sum, duration.count()); \
		fflush(stderr); \
		return duration.count(); \
		delete[] I; \
		I = NULL; \
	} \
	void print_header() {\
		fprintf(stderr, "\n Benchmark: %s \n", __title); \
		fflush(stderr); \
	}\
	const char* low_details = \
			"Batch size: " __TO_STRING_MACRO_EXPRESSION(__batch_size) \
			" ;  Shared data: " __TO_STRING_MACRO_EXPRESSION(__shared_data_type) \
			" ;  Loop: " __TO_STRING_MACRO_EXPRESSION(__task); \
	void print_low_details() { \
		fprintf(stderr, "\n Benchmark title: %s  %s", __title, low_details); \
		fflush(stderr); \
	} \
	const char* high_details = \
			"Batch size: " __TO_STRING_MACRO_EXPRESSION(__batch_size) \
			" ;  Shared data: " __TO_STRING_MACRO_EXPRESSION(__shared_data_type) \
			" ;  Loop: " __TO_STRING_MACRO_EXPRESSION( \
					__BENCHMARK_LOOP_CODE(__task, __batch_size)); \
	void print_high_details() { \
		fprintf(stderr, "\n Benchmark title: %s  %s", __title, high_details); \
		fflush(stderr); \
	} \
}


#define CREATE_ANONYMUS_BENCHMARK_CLASS(__title,\
		__batch_size, \
		__shared_data_type,\
		__task) \
	CREATE_ANONYMUS_BENCHMARK_CLASS_WITH_LOCAL(__title, \
			__batch_size, \
			__shared_data_type, \
			struct {}, \
			{}, \
			__task)

enum {
	__LOW_DETAILS,
	__HIGH_DETAILS,
	__TITLE_ONLY_DETAILS
};

#define CALL_BENCHMARK_FOR_THREADS_DETAILS(__min_thread, \
		__max_thread, \
		__test_duration, \
		__set_benchmark_data, \
		__class, \
		__details) \
{ \
	__class benchmark; \
	fprintf(stderr, "\n"); \
	if(__details == __LOW_DETAILS) \
		benchmark.print_low_details(); \
	else if(__details==__TITLE_ONLY_DETAILS) \
		benchmark.print_header(); \
	else \
		benchmark.print_high_details(); \
	for(size_t threads=__min_thread; threads<=__max_thread; ++threads) { \
		{ \
			auto& shared = benchmark.shared; \
			__set_benchmark_data; \
		} \
		benchmark.render_header = false; \
		benchmark.run(__test_duration, threads); \
	} \
}

#define CALL_BENCHMARK_FOR_THREADS_HIGH(__min_thread, \
		__max_thread, \
		__test_duration, \
		__set_benchmark_data, \
		__class) \
	CALL_BENCHMARK_FOR_THREADS_DETAILS(__min_thread, \
			__max_thread, \
			__test_duration, \
			__set_benchmark_data, \
			__class, \
			__HIGH_DETAILS)

#define CALL_BENCHMARK_FOR_THREADS_LOW(__min_thread, \
		__max_thread, \
		__test_duration, \
		__set_benchmark_data, \
		__class) \
	CALL_BENCHMARK_FOR_THREADS_DETAILS(__min_thread, \
			__max_thread, \
			__test_duration, \
			__set_benchmark_data, \
			__class, \
			__LOW_DETAILS)

#define CALL_BENCHMARK_FOR_THREADS_TITLE(__min_thread, \
		__max_thread, \
		__test_duration, \
		__set_benchmark_data, \
		__class) \
	CALL_BENCHMARK_FOR_THREADS_DETAILS(__min_thread, \
			__max_thread, \
			__test_duration, \
			__set_benchmark_data, \
			__class, \
			__TITLE_ONLY_DETAILS)

#define CALL_BENCHMARK_FOR_THREADS(__min_thread, \
		__max_thread, \
		__test_duration, \
		__set_benchmark_data, \
		__class) \
	CALL_BENCHMARK_FOR_THREADS_DETAILS(__min_thread, \
			__max_thread, \
			__test_duration, \
			__set_benchmark_data, \
			__class, \
			__LOW_DETAILS)

#endif

