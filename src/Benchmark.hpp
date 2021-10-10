/*
 *  Benchmarking tool for multi-threaded tasks.
 *  Copyright (C) 2020-2021 Marek Zalewski aka Drwalin
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

#include "Concurrent.hpp"

#include "Debug.hpp"

#ifdef USE_CSV_OUTPUT
# include <fstream>
# include <string>
# include <ctime>
std::ofstream csv_file((std::string("log\\log-")+std::to_string(time(NULL))+
			".csv").c_str());
uint64_t csv_line_id=0;
# define CSV_VALUE(value) {csv_file<<(value)<<",";csv_file.flush();}
# define CSV_LINE() {csv_file<<"\n";csv_file.flush();}
# define CSV_LINE_ID() {csv_file<<csv_line_id<<",";++csv_line_id;csv_file.flush();}
#else
# define CSV_VALUE(value)
# define CSV_LINE()
# define CSV_LINE_ID()
#endif

#define __TO_STRING_MACRO_EXPRESSION(__string) \
	__TO_STRING_MACRO_EXPRESSION2(__string)
#define __TO_STRING_MACRO_EXPRESSION2(__string) \
	#__string

#define __BENCHMARK_LOOP_CODE(__task, __batch_size) \
		uint64_t i=0; \
		for(; doNotStop; ) { \
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
	concurrent::atomic<size_t> done; \
	concurrent::atomic<size_t> ready; \
	volatile bool doNotStop; \
	size_t threads; \
	uint64_t *I; \
	void loop(int thread_id) { \
		__thread_local_data_type local; \
		{__thread_local_data_init}; \
		++ready; \
		while(start.load() == false) { \
		} \
		I[thread_id]=0; \
		__BENCHMARK_LOOP_CODE(__task, __batch_size) \
		__end: \
		I[thread_id]=i; \
		done++; \
	} \
public: \
	bool render_header=true; \
	bool count_only_first_thread=false; \
	__shared_data_type shared; \
	double run(float test_duration_seconds, size_t _threads) { \
		ready = 0; \
		doNotStop = true; \
		threads = _threads; \
		I = new uint64_t[threads]; \
		start = false; \
		done = 0; \
		for(size_t i=0; i<threads; ++i) { \
			std::thread(&loop, this, i).detach(); \
		} \
		std::this_thread::sleep_for(std::chrono::milliseconds(100)); \
		while(ready < threads) { \
			std::this_thread::yield(); \
		} \
		start = true; \
		std::chrono::high_resolution_clock::time_point begin = \
			std::chrono::high_resolution_clock::now(); \
		\
		for(;;) { \
			std::this_thread::yield(); \
			std::chrono::high_resolution_clock::time_point end = \
				std::chrono::high_resolution_clock::now(); \
			std::chrono::duration<double> duration = \
				std::chrono::duration<double>(end - begin); \
			if(duration.count() > test_duration_seconds) \
				break; \
			if(done.load() != 0) \
				break; \
		} \
		doNotStop = false; \
		while(done.load() < threads) \
			std::this_thread::yield(); \
		uint64_t sum=0; \
		uint64_t full_sum=0; \
		if(count_only_first_thread) \
			sum = I[0]; \
		else for(size_t j=0; j<threads; ++j) \
			sum += I[j]; \
		for(size_t j=0; j<threads; ++j) \
			full_sum += I[j]; \
		std::chrono::high_resolution_clock::time_point end = \
			std::chrono::high_resolution_clock::now(); \
		std::chrono::duration<double> duration = \
			std::chrono::duration<double>(end - begin); \
		if(render_header) \
			print_header(); \
		fprintf(stderr, \
				"\n         %llu threads %4.2f Mop/s (%llu/%llu in %.2fs)", \
				(uint64_t)threads, \
				(double)(sum) / duration.count() * 0.000001, \
				sum, full_sum, duration.count()); \
		fflush(stderr); \
		CSV_VALUE(threads); \
		CSV_VALUE((double)(sum) / duration.count() * 0.000001); \
		CSV_VALUE(sum); \
		CSV_VALUE(full_sum); \
		CSV_VALUE(duration.count()); \
		delete[] I; \
		I = NULL; \
		return duration.count(); \
	} \
	const char* title = __title; \
	void print_header() {\
		fprintf(stderr, "\n Benchmark: %s", __title); \
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

#define CREATE_ANONYMUS_BENCHMARK_CLASS(__title, \
		__batch_size, \
		__shared_data_type, \
		__task) \
	CREATE_ANONYMUS_BENCHMARK_CLASS_WITH_LOCAL(__title, \
			__batch_size, \
			__shared_data_type, \
			struct {int a;}, \
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
	if(__details == __LOW_DETAILS) \
		benchmark.print_low_details(); \
	else if(__details==__TITLE_ONLY_DETAILS) \
		benchmark.print_header(); \
	else \
		benchmark.print_high_details(); \
	for(size_t threads=__min_thread; threads<=__max_thread; ++threads) { \
		CSV_LINE(); \
		CSV_LINE_ID(); \
		CSV_VALUE(benchmark.title); \
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
			__TITLE_ONLY_DETAILS)
/*
			
	printf("\n\n\n\n\n %s\n\n\n\n\n",\
			__TO_STRING_MACRO_EXPRESSION((__class)) \
			);
			*/

#endif

