/*
 *  This file is part of ICon3. Please see README for details.
 *  Copyright (C) 2020-2021 Marek Zalewski aka Drwalin
 *
 *  ICon3 is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  ICon3 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef ALLOCATOR_HPP
#define ALLOCATOR_HPP

#ifdef CPP_FILES_CPP
#include <boost/pool/pool_alloc.hpp>
#endif

#include <cinttypes>
#include <cstdio>

class allocator {
public:
	static void* _allocate(size_t bytes);
	static void* _allocate(size_t bytes, void *hint);
	static void _deallocate(void *ptr);
	static void _deallocate(void *ptr, size_t bytes);
};

template<typename T>
class allocator_any {
public:
	typedef T value_type;
	typedef size_t size_type;
	typedef T* pointer;
	typedef const T* const_pointer;
	
	allocator_any() {
	}
	allocator_any(const allocator_any &a) {}
	template <class U>                    
	allocator_any(const allocator_any<U> &a) {}
	~allocator_any() {}

	inline pointer allocate(size_type n, const void *hint) {
		return (pointer)allocator::_allocate(n*sizeof(T), (void*)hint);
	}

	inline pointer allocate(size_type n) {
		return (pointer)allocator::_allocate(n*sizeof(T));
	}

	void inline deallocate(pointer p, size_type n) {
		if(p)
			allocator::_deallocate((void*)p, sizeof(T)*n);
	}
	
	template<class U>
	inline bool operator == (allocator_any<U> const&) noexcept {
		return true;
	}
	template<class U>
	inline bool operator != (allocator_any<U> const& y) noexcept {
		return !(*this == y);
	}
};

#endif

