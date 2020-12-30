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
	 *  Multip-purpose for any type of object with empty constructor and
	 *  assignment operator
	 */
	template<typename T>
	class spmc_queue {
	public:
		
		spmc_queue(spmc_queue&&) = delete;
		spmc_queue(const spmc_queue&) = delete;
		spmc_queue& operator =(const spmc_queue&) = delete;
		spmc_queue& operator =(spmc_queue&&) = delete;
		
		spmc_queue() {
			__init_empty(128);
		}
		
		spmc_queue(uint64_t allocation_batch) {
			__init_empty(allocation_batch);
		}
		
		~spmc_queue() {
			for(auto block : allocated_blocks)
				delete[] block;
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
			for(auto it : allocated_blocks)
				delete[] it;
			__init_empty(allocation_batch_size);
		}
		
	private:
		
		void __init_empty(uint64_t allocation_batch) {
			allocated = 0;
			allocation_batch_size = allocation_batch;
			size_t size;
			node_ptr* block = create_new_empty_cycle(size);
			allocated_blocks.emplace_back(block);
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
		std::vector<node_ptr*> allocated_blocks;
		uint64_t allocation_batch_size;
		uint64_t allocated;
	};
	
	
	
	
	
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
			
			mpsc_stack(mpsc_stack&&) = delete;
			mpsc_stack(const mpsc_stack&) = delete;
			mpsc_stack& operator =(const mpsc_stack&) = delete;
			mpsc_stack& operator =(mpsc_stack&&) = delete;
			
			mpsc_stack() {
				first = NULL;
			}
			
			~mpsc_stack() {
				while(first != NULL) {
					T* node = first;
					first = node->__m_next;
					delete node;
				}
			}
			
			//	safe to call without concurrent pop
			inline T* pop() {
				for(;;) {
					T* value = first;
					if(value == NULL)
						return NULL;
					T* next = value->__m_next;
					if(first.compare_exchange_weak(value, next)) {
						value->__m_next = NULL;
						return value;
					}
				}
				return NULL;
			}
			
			//	safe to call without concurrent push nor pop
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
					if(first.compare_exchange_weak(value, NULL))
						return value;
				}
			}
			
			inline void push(T* new_node) {
				if(new_node == NULL)
					return;
				for(;;) {
					new_node->__m_next = first;
					if(first.compare_exchange_weak(new_node->__m_next, new_node))
						return;
				}
			}
			
			//	safe to call without concurrent push nor pop
			inline void push_unsafe(T* new_node) {
				new_node->__m_next = first;
				first = new_node;
			}
			
			inline void push_all(T* _first) {
				while(*_first) {
					T* next = _first->next;
					if(first.compare_exchange_strong(NULL, _first))
						break;
					else
						push(_first);
					_first = next;
				}
			}
			
			//	safe to call without concurrent push nor pop
			inline void push_all_unsafe(T* _first) {
				if(_first == NULL)
					return;
				else if(first == NULL)
					first = _first;
				else do {
					T* next = _first->__m_next;
					push_unsafe(_first);
					_first = next;
				} while(_first);
			}
			
			inline void reverse() {
				T* all = pop_all();
				push_all(all);
			}
			
			//	safe to call without concurrent push nor pop
			inline void reverse_unsafe() {
				T* all = pop_all();
				push_all_unsafe(all);
			}
			
			friend class mpmc_stack<T>;
			
		protected:
			
			atomic<T*> first;
		};
		
		// uses std::mutex at pop
		template<typename T>
		class mpmc_stack {
		public:
			
			mpmc_stack(mpmc_stack&&) = delete;
			mpmc_stack(const mpmc_stack&) = delete;
			mpmc_stack& operator =(const mpmc_stack&) = delete;
			mpmc_stack& operator =(mpmc_stack&&) = delete;
			
			mpmc_stack() {
			}
			
			~mpmc_stack() {
			}
			
			inline T* pop() {
				for(;;) {
					T* value = stack.first;
					if(value == NULL)
						return NULL;
					T* next = value->__m_next;
					std::lock_guard<std::mutex> lock(mutex);
					if(stack.first.compare_exchange_weak(value, next)) {
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
		class spmc_queue_node : public spmc_queue<T*> {
		public:
			
			spmc_queue_node(spmc_queue_node&&) = delete;
			spmc_queue_node(const spmc_queue_node&) = delete;
			spmc_queue_node& operator =(const spmc_queue_node&) = delete;
			spmc_queue_node& operator =(spmc_queue_node&&) = delete;
			
			spmc_queue_node() : spmc_queue<T*>() {}
			spmc_queue_node(uint64_t allocation_batch) :
				spmc_queue<T*>(allocation_batch) {}
			~spmc_queue_node() {}
			
			inline T* pop() {
				T* value;
				if(spmc_queue<T>::pop(value))
					return value;
				return NULL;
			}
			
			inline void push_all(T* _first) {
				if(_first == NULL)
					return;
				do {
					T* next = _first->__m_next;
					push(_first);
					_first = next;
				} while(_first);
			}
		};
		
		template<typename T>
		class mpsc_queue {
		public:
			
			mpsc_queue(mpsc_queue&&) = delete;
			mpsc_queue(const mpsc_queue&) = delete;
			mpsc_queue& operator =(const mpsc_queue&) = delete;
			mpsc_queue& operator =(mpsc_queue&&) = delete;
			
			mpsc_queue() {}
			~mpsc_queue() {}
			
			inline void push(T* value) {
				producer.push();
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
		class mpmc_stackqueue {
		public:
			
			mpmc_stackqueue(mpmc_stackqueue&&) = delete;
			mpmc_stackqueue(const mpmc_stackqueue&) = delete;
			mpmc_stackqueue& operator =(const mpmc_stackqueue&) = delete;
			mpmc_stackqueue& operator =(mpmc_stackqueue&&) = delete;
			
			mpmc_stackqueue() {}
			~mpmc_stackqueue() {}
			
			inline void push(T* value) {
				producer.push();
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
		
		//  Behaviour should be very close to queue
		template<typename T>
		class mpmc_queue_lock {
		public:
			
			mpmc_queue_lock(mpmc_queue_lock&&) = delete;
			mpmc_queue_lock(const mpmc_queue_lock&) = delete;
			mpmc_queue_lock& operator =(const mpmc_queue_lock&) = delete;
			mpmc_queue_lock& operator =(mpmc_queue_lock&&) = delete;
			
			mpmc_queue_lock() {}
			~mpmc_queue_lock() {}
			
			inline void push(T* value) {
				producer.push();
			}
			
			inline T* pop() {
				for(;;) {
					T* value = consumer.pop();
					if(value)
						return value;
					if(mutex.try_lock()) {
						consumer.push_all(producer.pop_all());
						mutex.unlock();
						return consumer.pop();
					}
				}
				return NULL;
			}
			
		private:
			
			mpmc_stack<T> producer;
			spmc_queue<T> consumer;
			std::mutex mutex;
		};
		
		
		template<typename T,
			template<typename T2>
				typename container_type = mpmc_stack>
		class pool {
		public:
			
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
				for(;;) {
					T* ptr = heap.pop();
					if(ptr)
						delete ptr;
					else
						break;
				}
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
			
			container_type<T> heap;
		};
	};
};












































/*
template<typename T>
class Pool {
public:
	
	class Node : public T {
	public:
		template<typename... _args>
		Node(_args... args) :
			T(args...){
			next = NULL;
		}
		
		Node* next;
	};
	
	
	Pool() {
		heap = NULL;
	}
	
	~Pool() {
		while(heap != NULL) {
			Node* node = heap;
			heap = node->next;
			delete node;
		}
	}
	
	template<typename... _args>
	void Allocate(size_t amountOfObjects, _args... args) {
		for(size_t i=0; i<amountOfObjects; ++i)
			Release(new Node(args...));
	}
	
	template<typename... _args>
	T* Pop(_args... args) {
		T* node = TryPop();
		if(node != NULL)
			return node;
		return new Node(args...);
	}
	
	T* TryPop() {
		for(;;) {
			Node* tempHeap = heap;
			if(tempHeap == NULL)
				return NULL;
			Node* next = tempHeap->next;
			std::lock_guard<std::mutex> lock(mutex);
			if(heap.compare_exchange_weak(tempHeap, next)) {
				return tempHeap;
			}
		}
		return NULL;
	}
	
	void Release(T* ptr) {
		if(ptr == NULL)
			return;
		Node* node = (Node*)ptr;
		for(;;) {
			node->next = heap;
			if(heap.compare_exchange_weak(node->next, node)) {
				return;
			}
		}
	}
	
private:
	
	std::mutex mutex;
	atomic<Node*> heap;
};
*/
#endif

