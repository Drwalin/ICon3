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

#ifndef CPP_FILES_CPP
#define CPP_FILES_CPP
#endif

#include "Allocator.hpp"

using boost_allocator = boost::pool_allocator<uint8_t>;

void* allocator::_allocate(size_t bytes, void *hint) {
	return boost_allocator::allocate(bytes, (uint8_t*)hint);
}

void* allocator::_allocate(size_t bytes) {
	return boost_allocator::allocate(bytes);
}

void allocator::_deallocate(void *ptr, size_t bytes) {
	if(ptr)
		boost_allocator::deallocate((uint8_t*)ptr, bytes);
}

void allocator::_deallocate(void *ptr) {
	if(ptr)
		boost_allocator::deallocate((uint8_t*)ptr, 0);
}

