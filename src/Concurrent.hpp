/*
 *  Concurrent queue, stack, stackqueue and object pool.
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

/*
 *  This file is header-only. Implements concurrent queue, stack, stackqueue
 *  and object pool
 */

#ifndef POOL_HPP
#define POOL_HPP

#include <cinttypes>

#include <atomic>
#include <mutex>
#include <thread>
#include <queue>

#include <Debug.hpp>

/*
 *  spsc - single producer single consumer
 *  spmc - single producer multiple consumer
 *  mpsc - multiple producer single consumer
 *  mpmc - multiple producer multiple consumer
 */

namespace concurrent {
	
	/*
	 *  To reduce concurrent acceses to 64-byte atomic memory blocks when not
	 *  accessing the same atomic value
	 */
	template<typename T>
	struct alignas(64) atomic : public std::atomic<T> {
		atomic(T value) : std::atomic<T>(value) {}
		atomic() : std::atomic<T>() {}
		auto operator =(const T value) {
			return std::atomic<T>::operator=(value);
		}
	};
	
	/*
	 *  Multi-purpose for any type of object with empty constructor and
	 *  assignment operator
	 */
	template<typename T>
	class spmc_queue {
	public:
		
		static char* __name;
		
		spmc_queue(spmc_queue&&) = delete;
		spmc_queue(const spmc_queue&) = delete;
		spmc_queue& operator =(const spmc_queue&) = delete;
		spmc_queue& operator =(spmc_queue&&) = delete;
		
		spmc_queue() {
			__name = (char*)__func__;
			__init_empty(1024*1024);
		}
		
		spmc_queue(uint64_t allocation_batch) {
			__name = (char*)__func__;
			__init_empty(allocation_batch);
		}
		
		~spmc_queue() {
			while(!allocated_blocks.empty()) {
				delete[] allocated_blocks.front();
				allocated_blocks.pop();
			}
//			fprintf(stderr,
//					"\n Deallocated local-nodes %llu on destructor of %s",
//					allocated, __name);
		}
		
		inline void push(const T& value) {
			node_ptr* _last = last.load();
			if(_last->__m_next == first.load()) {
				size_t size;
				node_ptr* block = create_new_empty_cycle(size);
				block[size-1].__m_next = first.load();		
				_last->__m_next = block;
			}
			_last->value = value;
			last = _last->__m_next;
		}
		
		inline bool pop(T& value) {
			for(;;) {
				node_ptr* _first = first.load();
				if(_first == last.load())
					return false;
				value = _first->value;
				if(first.compare_exchange_strong(_first, _first->__m_next))
					return true;
			}
			return false;
		}
		
		//  safe to call without any concurrent calls
		inline bool pop_unsafe(T& value) {
			node_ptr* _first = first.load();
			if(_first == last.load())
				return false;
			value = _first->value;
			first = _first->__m_next;
			return true;
		}
		
		inline uint64_t capacity() const {
			return allocated;
		}
		
		void set_allocation_batch(uint64_t batch) {
			if(batch < 16)
				batch = 16;
			batch += 15;
			batch = batch >> 4;
			batch = batch << 4;
			allocation_batch_size = batch;
		}
		
		void clear_dealloc_single_threaded_unsafe() {
			while(!allocated_blocks.empty()) {
				delete[] allocated_blocks.front();
				allocated_blocks.pop();
			}
			__init_empty(allocation_batch_size);
		}
		
	private:
		
		void __init_empty(uint64_t allocation_batch) {
			allocated = 0;
			allocation_batch_size = allocation_batch;
			size_t size;
			node_ptr* block = create_new_empty_cycle(size);
			allocated_blocks.emplace(block);
			first = block;
			last = block;
		}
		
		struct node_ptr {
			T value;
			node_ptr* __m_next;
		};
		
		node_ptr* create_new_empty_cycle(size_t& size) {
			size = allocation_batch_size;
			node_ptr* block = new node_ptr[size];
			for(size_t i=0; i<size; ++i)
				block[i].__m_next = &(block[(i+1)%size]);
			allocated += size;
			return block;
		}
		
		atomic<node_ptr*> first, last;
		std::queue<node_ptr*> allocated_blocks;
		uint64_t allocation_batch_size;
		uint64_t allocated;
	};
	template<typename T>
	char* spmc_queue<T>::__name = (char*)"spmc_queue<T>";
	
	
	
	
	
	namespace ptr {
		
		template<typename T>
		class node {
		public:
			node() : __m_next(NULL) {}
			T* __m_next;
		};
		
		template<typename T>
		class mpmc_stack;
		
		template<typename T>
		class mpsc_stack {
		public:
			
			static char* __name;
			
			mpsc_stack(mpsc_stack&&) = delete;
			mpsc_stack(const mpsc_stack&) = delete;
			mpsc_stack& operator =(const mpsc_stack&) = delete;
			mpsc_stack& operator =(mpsc_stack&&) = delete;
			
			mpsc_stack() {
				__name = (char*)__func__;
				first = NULL;
			}
			
			~mpsc_stack() {
				uint64_t C = 0;
				while(first != NULL) {
					T* node = first;
					first = node->__m_next;
					delete node;
					++C;
				}
//				fprintf(stderr, "\n Deleted %llu on destructor of %s", C, __name);
			}
			
			//	safe to call without concurrent pop
			inline T* pop() {
				for(;;) {
					T* value = first;
					if(value == NULL)
						return NULL;
					T* next = value->__m_next;
					if(first.compare_exchange_strong(value, next)) {
						value->__m_next = NULL;
						return value;
					}
				}
				return NULL;
			}
			
			//  safe to call without any concurrent calls
			inline T* pop_unsafe() {
				T* value = first;
				if(value == NULL)
					return NULL;
				first = value->__m_next;
				value->__m_next = NULL;
				return value;
			}
			
			//	pop whole stack at once, caller must handle returned list
			inline T* pop_all() {
				for(;;) {
					T* value = first;
					if(value == NULL)
						return value;
					if(first.compare_exchange_strong(value, NULL)) {
						return value;
					}
				}
			}
			
			inline void push(T* new_node) {
				if(new_node == NULL)
					return;
				for(;;) {
					new_node->__m_next = first;
					if(first.compare_exchange_weak(new_node->__m_next,
								new_node)) {
						return;
					}
				}
			}
			
			//  safe to call without any concurrent calls
			inline void push_unsafe(T* new_node) {
				if(new_node == NULL)
						return;
				new_node->__m_next = first;
				first = new_node;
			}
			
			inline void push_all(T* _first) {
				while(_first) {
					T* next = _first->__m_next;
					T* ___ignore_cmpxchg_dst = NULL;
					if(first.compare_exchange_strong(___ignore_cmpxchg_dst,
								_first))
						break;
					else
						push(_first);
					_first = next;
				}
			}
			
			//  safe to call without any concurrent calls
			inline void push_all_unsafe(T* _first) {
				if(_first == NULL)
					return;
				else if(first == NULL)
					first = _first;
				else {
					do {
						T* next = _first->__m_next;
						push_unsafe(_first);
						_first = next;
					} while(_first);
				}
			}
			
			inline void reverse() {
				T* all = pop_all();
				push_all(all);
			}
			
			//  safe to call without any concurrent calls
			inline void reverse_unsafe() {
				T* all = pop_all();
				push_all_unsafe(all);
			}
			
			friend class mpmc_stack<T>;
			
		protected:
			
			atomic<T*> first;
		};
		template<typename T>
		char* mpsc_stack<T>::__name = (char*)"mpsc_stack<T>";
		
		
		
		// uses std::mutex at pop
		template<typename T>
		class mpmc_stack {
		public:
			
			static char* __name;
			
			mpmc_stack(mpmc_stack&&) = delete;
			mpmc_stack(const mpmc_stack&) = delete;
			mpmc_stack& operator =(const mpmc_stack&) = delete;
			mpmc_stack& operator =(mpmc_stack&&) = delete;
			
			mpmc_stack() {
				__name = (char*)__func__;
			}
			
			~mpmc_stack() {
//				fprintf(stderr, "\n Destructor of %s", __name);
			}
			
			inline T* pop() {
				for(;;) {
					T* value = stack.first;
					if(value == NULL)
						return NULL;
					T* next = value->__m_next;
					std::lock_guard<std::mutex> lock(mutex);
					if(stack.first.compare_exchange_strong(value, next)) {
						return value;
					}
				}
				return NULL;
			}
			
			//	safe to call with concurrent push, but without concurrent pop
			inline T* pop_sequentially() {
				return stack.pop();
			}
			
			//	safe to call without concurrent push nor pop
			inline T* pop_unsafe() {
				return stack.pop.unsafe();
			}
			
			//	pop whole stack at once, caller must handle returned list
			inline T* pop_all() {
				return stack.pop_all();
			}
			
			inline void push(T* new_node) {
				stack.push(new_node);
			}
			
			//	safe to call without concurrent push nor pop
			inline void push_sequentially(T* new_node) {
				stack.push_sequentially(new_node);
			}
			
			inline void push_all(T* _first) {
				stack.push_all(_first);
			}
			
			//	safe to call without concurrent push nor pop
			inline void push_all_unsafe(T* _first) {
				stack.push_all_unsafe(_first);
			}
			
			inline void reverse() {
				stack.reverse();
			}
			
			//	safe to call without concurrent push nor pop
			inline void reverse_unsafe() {
				stack.reverse_unsafe();
			}
			
		private:
			
			mpsc_stack<T> stack;
			std::mutex mutex;
		};
		template<typename T>
		char* mpmc_stack<T>::__name = (char*)"mpmc_stack<T>";
		
		
		
		template<typename T>
		class spmc_queue_node : public spmc_queue<T*> {
		public:
			
			static char* __name;
			
			spmc_queue_node(spmc_queue_node&&) = delete;
			spmc_queue_node(const spmc_queue_node&) = delete;
			spmc_queue_node& operator =(const spmc_queue_node&) = delete;
			spmc_queue_node& operator =(spmc_queue_node&&) = delete;
			
			spmc_queue_node() : spmc_queue<T*>() {
				__name = (char*)__func__;
			}
			spmc_queue_node(uint64_t allocation_batch) :
				spmc_queue<T*>(allocation_batch) {
				__name = (char*)__func__;
			}
			~spmc_queue_node() {
				uint64_t C=0;
				for(;;) {
					T* value;
					if(spmc_queue<T*>::pop_unsafe(value))
						delete value;
					else
						break;
					++C;
				}
//				fprintf(stderr, "\n Deleted %llu on destructor of %s", C, __name);
			}
			
			inline T* pop() {
				T* value;
				if(spmc_queue<T*>::pop(value))
					return value;
				return NULL;
			}
			
			inline T* pop_unsafe() {
				T* value;
				if(spmc_queue<T*>::pop_unsafe(value))
					return value;
				return NULL;
			}
			
			inline void push_all(T* _first) {
				if(_first == NULL)
					return;
				do {
					T* next = _first->__m_next;
					spmc_queue<T*>::push(_first);
					_first = next;
				} while(_first);
			}
		};
		template<typename T>
		char* spmc_queue_node<T>::__name = (char*)"spmc_queue_node<T>";
		
		
		
		template<typename T>
		class mpsc_queue {
		public:
			
			static char* __name;
			
			mpsc_queue(mpsc_queue&&) = delete;
			mpsc_queue(const mpsc_queue&) = delete;
			mpsc_queue& operator =(const mpsc_queue&) = delete;
			mpsc_queue& operator =(mpsc_queue&&) = delete;
			
			mpsc_queue() {
				__name = (char*)__func__;
			}
			~mpsc_queue() {
//				fprintf(stderr, "\n Destructor of %s", __name);
			}
			
			inline void push(T* value) {
				producer.push(value);
			}
			
			inline T* pop() {
				T* value = consumer.pop();
				if(value)
					return value;
				consumer.push_all_unsafe(producer.pop_all());
				return consumer.pop();
			}
			
		private:
			
			mpsc_stack<T> producer;
			mpsc_stack<T> consumer;
		};
		template<typename T>
		char* mpsc_queue<T>::__name = (char*)"mpsc_queue<T>";
		
		
		
		template<typename T>
		class mpmc_stackqueue {
		public:
			
			static char* __name;
			
			mpmc_stackqueue(mpmc_stackqueue&&) = delete;
			mpmc_stackqueue(const mpmc_stackqueue&) = delete;
			mpmc_stackqueue& operator =(const mpmc_stackqueue&) = delete;
			mpmc_stackqueue& operator =(mpmc_stackqueue&&) = delete;
			
			mpmc_stackqueue() {
				__name = (char*)__func__;
			}
			~mpmc_stackqueue() {
//				fprintf(stderr, "\n Destructor of %s", __name);
			}
			
			inline void push(T* value) {
				producer.push(value);
			}
			
			inline T* pop() {
				T* value = consumer.pop();
				if(value)
					return value;
				consumer.push_all(producer.pop_all());
				return consumer.pop();
			}
			
		private:
			
			mpmc_stack<T> producer;
			mpmc_stack<T> consumer;
		};
		template<typename T>
		char* mpmc_stackqueue<T>::__name = (char*)"mpmc_stackqueue<T>";
		
		
		
		template<typename T>
		class mpmc_stackqueue_lock {
		public:
			
			inline static const char* __name = "mpmc_stackqueue_lock";
			
			mpmc_stackqueue_lock(mpmc_stackqueue_lock&&) = delete;
			mpmc_stackqueue_lock(const mpmc_stackqueue_lock&) = delete;
			mpmc_stackqueue_lock& operator =(const mpmc_stackqueue_lock&)
				= delete;
			mpmc_stackqueue_lock& operator =(mpmc_stackqueue_lock&&)
				= delete;
			
			mpmc_stackqueue_lock() {}
			~mpmc_stackqueue_lock() {
//				fprintf(stderr, "\n Destructor of %s", __name);
			}
			
			inline void push(T* value) {
				producer.push(value);
			}
			
			inline T* pop() {
				for(;;) {
					T* value = consumer.pop();
					if(value)
						return value;
					if(mutex.try_lock()) {
						value = consumer.pop();
						if(value) {
							mutex.unlock();
							return value;
						}
						consumer.push_all_unsafe(producer.pop_all());
						mutex.unlock();
						value = consumer.pop();
						if(value)
							value->__m_next = NULL;
						return value;
					}
				}
				return NULL;
			}
			
		private:
			
			mpmc_stack<T> producer;
			mpmc_stack<T> consumer;
			std::mutex mutex;
		};
		
		
		
		//  Behaviour should be very close to queue
		template<typename T>
		class mpmc_queue_lock {
		public:
			
			static char* __name;
			
			mpmc_queue_lock(mpmc_queue_lock&&) = delete;
			mpmc_queue_lock(const mpmc_queue_lock&) = delete;
			mpmc_queue_lock& operator =(const mpmc_queue_lock&) = delete;
			mpmc_queue_lock& operator =(mpmc_queue_lock&&) = delete;
			
			mpmc_queue_lock() {
				__name = (char*)__func__;
			}
			~mpmc_queue_lock() {
//				fprintf(stderr, "\n Destructor of %s", __name);
			}
			
			inline void push(T* value) {
				producer.push(value);
			}
			
			inline T* pop() {
				for(;;) {
					T* value = consumer.pop();
					if(value)
						return value;
					if(mutex.try_lock()) {
						value = consumer.pop();
						if(value) {
							mutex.unlock();
							return value;
						}
						consumer.push_all(producer.pop_all());
						mutex.unlock();
						value = consumer.pop();
						if(value)
							value->__m_next = NULL;
						return value;
					}
				}
				return NULL;
			}
			
		private:
			
			mpmc_stack<T> producer;
			spmc_queue_node<T> consumer;
			std::mutex mutex;
		};
		template<typename T>
		char* mpmc_queue_lock<T>::__name = (char*)"mpmc_queue_lock<T>";
		
		
		
		template<typename T, typename container_type = mpmc_stack<T>>
		class pool {
		public:
			
			inline static const char* __name = "concurrent::ptr::pool";
			
			pool(pool&&) = delete;
			pool(const pool&) = delete;
			pool& operator =(const pool&) = delete;
			pool& operator =(pool&&) = delete;
			
			pool() {}
			template<typename... _args>
			pool(size_t count, _args... args) {
				allocate(count, args...);
			}
			~pool() {
				uint64_t C=0;
				for(;;) {
					T* ptr = heap.pop();
					if(ptr)
						delete ptr;
					else
						break;
					++C;
				}
//				fprintf(stderr, "\n Deleted %llu on destructor of %s", C, __name);
			}
			
			template<typename... _args>
			void allocate(size_t count, _args... args) {
				for(size_t i=0; i<count; ++i) {
					heap.push(new T(args...));
				}
			}
			
			template<typename... _args>
			T* get(_args... args) {
				T* ptr = heap.pop();
				if(ptr)
					return ptr;
				return new T(args...);
			}
			
			void release(T* ptr) {
				heap.push(ptr);
			}
			
		private:
			
			container_type heap;
		};
	};
	
	namespace linear {
		
		template<typename T>
		class stack {
		public:
			inline static const char* __name = "concurrent::linear::stack";
			
			stack(stack&& other) {
				first = other.first;
				size = other.size;
			}
			stack(const stack&) = delete;
			stack& operator =(const stack&) = delete;
			stack& operator =(stack&&) = delete;
			
			stack() {
				first = NULL;
				size = 0;
			}
			
			~stack() {
				while(first != NULL) {
					T* node = first;
					first = node->__m_next;
					delete node;
				}
			}
			
			inline T* pop() {
				T* value = first;
				if(value == NULL)
					return NULL;
				--size;
				first = value->__m_next;
				value->__m_next = NULL;
				return value;
			}
			
			inline T* pop_all() {
				T* value = first;
				size = 0;
				first = NULL;
				return value;
			}
			
			inline void push(T* new_node) {
				if(new_node == NULL)
						return;
				++size;
				new_node->__m_next = first;
				first = new_node;
			}
			
			inline void push_all(T* _first) {
				do {
					T* next = _first->__m_next;
					push(_first);
					_first = next;
				} while(_first);
			}
			
			inline void push_all(T* _first, size_t count) {
				if(_first == NULL)
					return;
				else if(first == NULL) {
					first = _first;
					size = count;
				} else
					push_all(_first);
			}
			
			inline void reverse() {
				size_t s = size;
				T* all = pop_all();
				push_all(all, size);
			}
			
			inline void swap(stack& o) {
				std::swap(size, o.size);
				std::swap(first, o.first);
			}
			
			size_t size;
			
		private:
			
			T* first;
		};
	};
};

template<typename T>
class single_pool {
public:
	
	single_pool(const single_pool<T>&) {}
	single_pool(single_pool<T>&& o) : stack(o.stack) {}
	single_pool() {}
	~single_pool() {}
	
	inline void resize(size_t n) {
		if(stack.size > n) {
			for(size_t i=n; i<stack.size; ++i)
				delete stack.pop();
		} else if(stack.size < n) {
			allocate(n-stack.size);
		}
	}
	
	template<typename... _args>
	inline void allocate(size_t n, _args... args) {
		for(size_t i=0; i<n; ++i)
			stack.push(new T(args...));
	}
	
	inline T* get() {
		if(stack.size > 0)
			return stack.pop();
		return new T;
	}
	
	inline void release(T* ptr) {
		if(ptr) {
			stack.push(ptr);
		}
	}
	
	static inline void swap(single_pool& a, single_pool& b) {
		a.stack.swap(b.stack);
	}
	
	inline size_t size() const {
		return stack.size;
	}
	
	concurrent::linear::stack<T> stack;
};

class time_printer {
public:
	
	time_printer(const char* str) : str(str) {
		start = std::chrono::high_resolution_clock::now();
	}
	
	~time_printer() {
		end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> duration =
			std::chrono::duration<double>(end - start);
		fprintf(stderr, "\n %s took: %.2fs", str, duration.count());
		fflush(stderr);
	}
	
	const char* str;
	std::chrono::high_resolution_clock::time_point start, end;
};

#include <fstream>

template<typename T>
class single_pool_multi_thread_equalizer {
public:
	
	inline T* get() {
		static uint64_t Cc = 0;
		++Cc;
		if(Cc & 0xffff == 0xffff)
			sort();
		for(auto& p : pools) {
			if(p.size() > 0)
				return p.get();
		}
		return new T;
	}
	
	inline void release(T* ptr) {
		if(pools.size() == 0)
			pools.resize(1);
		static uint64_t Cc = 0;
		++Cc;
		if(Cc & 0xffff == 0xffff)
			sort();
		pools.back().release(ptr);
	}
	
	single_pool_multi_thread_equalizer() {
		row_id = 0;
		csv.open((std::string("log\\pool_size-")+std::to_string(time(NULL))+
			".csv").c_str());
		csv << "\npool capacity\n\n";
		csv << "id,";
		for(int i=0; i<30; ++i) {
			csv << "thread " << i << ",";
		}
	}
	
	std::ofstream csv;
	
	void sort() {
		for(int i=1; i<pools.size(); ++i) {
			bool end = true;
			for(int j=pools.size()-1; j>0; --j) {
				if(pools[j-1].size() < pools[j].size()) {
					single_pool<T>::swap(pools[j-1], pools[j]);
					end = false;
				}
			}
			if(end)
				break;
		}
	}
	
	void equalize(size_t threads, size_t amount_per_thread) {
//		time_printer printer1("Equalizing pools");
		{
			if(threads==0 || amount_per_thread==0) {
				pools.clear();
				return;
			}
			
			sort();
			single_pool<T> buffer;
			for(size_t i=threads; i<pools.size(); ++i) {
				size_t size = pools[i].size();
				buffer.stack.push_all(pools[i].stack.pop_all(), size);
			}
			
			uint64_t sum = 0;
			for(auto& pool : pools) {
				sum += pool.size();
			}
			uint64_t sum_threads = sum / threads;
			if(sum_threads < amount_per_thread*3 &&
					sum_threads > amount_per_thread) {
				amount_per_thread = sum_threads;
			} else if(sum_threads >= amount_per_thread*3) {
				amount_per_thread = sum_threads;
			}
			
			pools.resize(threads);
			
			for(int i=0; i<threads*threads; ++i) {
				sort();
				while(true) {
					if(pools.front().size() > amount_per_thread) {
						if(pools.back().size() < amount_per_thread) {
							pools.back().release(pools.front().get());
						} else
							break;
					} else
						break;
				}
			}
			
			for(auto& pool : pools) {
				while(pool.size() < amount_per_thread)
					pool.release(buffer.get());
			}
			
			for(auto& e : pools) {
				e.resize(amount_per_thread);
			}
		}
	}
	
	single_pool<T>* get_pools(size_t threads, size_t amount_per_thread) {
		print_stats();
		equalize(threads, amount_per_thread);
		return &(pools.front());
	}
	
	uint64_t row_id;
	void print_stats() {
		/*
		fprintf(stderr, "\n pools capacity:");
		for(auto& e : pools) {
			fprintf(stderr, "    %llu", e.size());
		}
		*/
		csv << "\n" << row_id << ","; ++row_id;
		for(auto& e : pools) {
			csv << e.size() << ",";
		}
	}
	
	std::vector<single_pool<T>> pools;
};

#endif

