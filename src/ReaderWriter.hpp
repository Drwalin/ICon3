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

#ifndef READER_WRITER_HPP
#define READER_WRITER_HPP

#include <cinttypes>
#include <cmath>

#include <vector>
#include <bit>
#include <string>
#include <span>

#include "Allocator.hpp"

namespace endian {
	template<typename T>
		inline T byteswap(T value) {
			if constexpr(sizeof(T) == 1)
				return value;
			union {
				T v;
				uint8_t b[sizeof(T)];
			} src, dst;
			src.v = value;
			if constexpr(sizeof(T) == 2) {
				dst.b[0] = src.b[1];
				dst.b[1] = src.b[0];
				return dst.v;
			} else if constexpr(sizeof(T) == 4) {
				dst.b[0] = src.b[3];
				dst.b[1] = src.b[2];
				dst.b[2] = src.b[1];
				dst.b[3] = src.b[0];
				return dst.v;
			} else if constexpr(sizeof(T) == 8) {
				dst.b[0] = src.b[7];
				dst.b[1] = src.b[6];
				dst.b[2] = src.b[5];
				dst.b[3] = src.b[4];
				dst.b[4] = src.b[3];
				dst.b[5] = src.b[2];
				dst.b[6] = src.b[1];
				dst.b[7] = src.b[0];
				return dst.v;
			}
		}

	template<typename T>
		inline T ReadBigEndiand(const uint8_t* buffer) {
			if constexpr (std::endian::native == std::endian::big)
				return *(const T*)buffer;
			else
				return byteswap(*(const T*)buffer);
		}

	template<typename T>
		inline void WriteBigEndiand(T value, uint8_t* buffer) {
			if constexpr (std::endian::native == std::endian::big)
				*(T*)buffer = value;
			else
				*(T*)buffer = byteswap(value);
		}
}

namespace ReaderWriter {
	// uint8[] == int8[] = char[] = string
	enum Type {
		_uint7        = 0x00,  // 0x00:0x7F == 0x00:0x7F : [0x80]
		_string_short = 0x80,  // 0x80:0xBF -> 0x00:0x3F : [0x40]
		_array_short  = 0xC0,  // 0xC0:0xDF -> 0x00:0x1F : [0x20]
		_map_short    = 0xE0,  // 0xE0:0xEF -> 0x00:0x0F : [0x10]
		
		_one_type_types = 0xF0,
		
		_array_types_begin = 0xF0,
		
		_string_long  = 0xF0,  // [ int size, uint8[size+0x40]      ]
		_array_long   = 0xF1,  // [ int size, any[size+0x1F]        ]
		_map_long     = 0xF2,  // [ int size, {any ,any}[size+0x10] ]

		_array_uint16  = 0xF3,  // [ int size, uint16[size]      ]
		_array_uint32  = 0xF4,  // [ int size, uint32[size]      ]
		_array_uint64  = 0xF5,  // [ int size, uint64[size]      ]
		
		_array_types_end = 0xF5,
		
		_true    = 0xF6,
		_false   = 0xF7,
		_real    = 0xF8,  // [ int exponent, int mantissa ]
		_uint16  = 0xF9,
		_uint32  = 0xFA,
		_uint64  = 0xFB,
		_sint8   = 0xFC,
		_sint16  = 0xFD,
		_sint32  = 0xFE,
		_sint64  = 0xFF,
	};
	
	enum Sizes {
		_string_short_size = 0x40,
		_array_short_size  = 0x20,
		_map_short_size    = 0x10,
		
		_uint7_max = 0x7F,
	};



	inline Type GetType(const uint8_t* buffer);

	const uint8_t* ReadTypeAndSize(Type& type, uint64_t& size,
			const uint8_t* buffer, const uint8_t* end);

	template<typename T>
		inline const uint8_t* Read(T& value, const uint8_t* buffer, const uint8_t* end);

	inline const uint8_t* ReadString(char**const str, size_t& size,
			const uint8_t* buffer, const uint8_t* end);

	
	inline void InitArray(Type type, uint64_t elements,
			std::vector<uint8_t, allocator_any<uint8_t>>& buffer);
	
	inline void InitMap(uint64_t elements,
			std::vector<uint8_t, allocator_any<uint8_t>>& buffer);
	
	template<typename T>
		inline void Write(T value,
				std::vector<uint8_t, allocator_any<uint8_t>>& buffer);



	template<>
		inline const uint8_t* Read<uint64_t>(uint64_t& value,
				const uint8_t* buffer, const uint8_t* end) {
			if(buffer >= end)
				return NULL;
			uint8_t v = *buffer;
			size_t mod;
			switch(v) {
				case _uint16:
					value = endian::ReadBigEndiand<uint16_t>(buffer+1)+0x80;
					mod = 3;
				case _uint32:
					value = endian::ReadBigEndiand<uint32_t>(buffer+1)+0x80;
					mod = 5;
				case _uint64:
					value = endian::ReadBigEndiand<uint64_t>(buffer+1)+0x80;
					mod = 9;
				case _sint8:
					value = -*(int8_t*)(buffer+1);
					mod = 2;
				case _sint16:
					value = -endian::ReadBigEndiand<int16_t>(buffer+1);
					mod = 3;
				case _sint32:
					value = -endian::ReadBigEndiand<int32_t>(buffer+1);
					mod = 5;
				case _sint64:
					value = -endian::ReadBigEndiand<int64_t>(buffer+1);
					mod = 9;
				default:
					if(v >= _uint7) {
						value = v - _uint7;
						mod = 1;
					} else {
						value = 0;
						return NULL;
					}
			}
			buffer += mod;
			if(buffer > end)
				return NULL;
			return buffer;
		}

	template<>
		inline const uint8_t* Read<uint8_t>(uint8_t& value,
				const uint8_t* buffer, const uint8_t* end) {
			uint64_t v64=0;
			buffer = Read<uint64_t>(v64, buffer, end);
			value = v64;
			return buffer;
		}

	template<>
		inline const uint8_t* Read<uint16_t>(uint16_t& value,
				const uint8_t* buffer, const uint8_t* end) {
			uint64_t v64=0;
			buffer = Read<uint64_t>(v64, buffer, end);
			value = v64;
			return buffer;
		}

	template<>
		inline const uint8_t* Read<uint32_t>(uint32_t& value,
				const uint8_t* buffer, const uint8_t* end) {
			uint64_t v64=0;
			buffer = Read<uint64_t>(v64, buffer, end);
			value = v64;
			return buffer;
		}

	template<>
		inline const uint8_t* Read<int8_t>(int8_t& value,
				const uint8_t* buffer, const uint8_t* end) {
			uint64_t v64=0;
			buffer = Read<uint64_t>(v64, buffer, end);
			value = v64;
			return buffer;
		}

	template<>
		inline const uint8_t* Read<int16_t>(int16_t& value,
				const uint8_t* buffer, const uint8_t* end) {
			uint64_t v64=0;
			buffer = Read<uint64_t>(v64, buffer, end);
			value = v64;
			return buffer;
		}

	template<>
		inline const uint8_t* Read<int32_t>(int32_t& value,
				const uint8_t* buffer, const uint8_t* end) {
			uint64_t v64=0;
			buffer = Read<uint64_t>(v64, buffer, end);
			value = v64;
			return buffer;
		}

	template<>
		inline const uint8_t* Read<int64_t>(int64_t& value,
				const uint8_t* buffer, const uint8_t* end) {
			return Read<uint64_t>((uint64_t&)value, buffer, end);
		}

	template<>
		inline const uint8_t* Read<bool>(bool& value,
				const uint8_t* buffer, const uint8_t* end) {
			if(*buffer == _true)
				value = true;
			else if(*buffer == _false)
				value = false;
			else
				return NULL;
			return buffer+1;
		}

	template<>
		inline const uint8_t* Read<long double>(long double& value,
				const uint8_t* buffer, const uint8_t* end) {
			value = 0;
			if(buffer >= end)
				return NULL;
			if(*buffer == _real) {
				int64_t exponential=0, mantissa=0;
				const uint8_t* e = Read<int64_t>(mantissa, buffer+1, end);
				const uint8_t* e2 = Read<int64_t>(exponential, e, end);
				if(e2 > end)
					return NULL;
				value = mantissa;
				value = ldexpl(mantissa, exponential);
				return e2;
			}
			return NULL;
		}

	template<>
		inline const uint8_t* Read<double>(double& value,
				const uint8_t* buffer, const uint8_t* end) {
			long double v=0;
			buffer = Read<long double>(v, buffer, end);
			value = v;
			return buffer;
		}

	template<>
		inline const uint8_t* Read<float>(float& value,
				const uint8_t* buffer, const uint8_t* end) {
			long double v=0;
			buffer = Read<long double>(v, buffer, end);
			value = v;
			return buffer;
		}
		
	
	
	template<>
		inline const uint8_t* Read<std::string_view>(std::string_view& value,
				const uint8_t* buffer, const uint8_t* end) {
			size_t size;
			const char* str = NULL;
			buffer = ReadString((char**const)&str, size, buffer, end);
			if(buffer) {
				value = std::string_view((const char*)buffer,
					(const char*)buffer+size);
				return buffer;
			}
			return NULL;
		}
		
	inline const uint8_t* ReadString(char**const str, size_t& size,
				const uint8_t* buffer, const uint8_t* end) {
			Type type;
			buffer = ReadTypeAndSize(type, size, buffer, end);
			if(type == _string_long || type == _string_short) {
				*str = (char*const)buffer;
				buffer += size;
				if(buffer > end)
					return NULL;
				return buffer;
			} else {
				*str = NULL;
			}
			return NULL;
	}






	inline Type GetType(const uint8_t* buffer) {
		uint8_t v = *buffer;
		if(v >= _one_type_types)
			return (Type)v;
		else if(v >= _map_short)
			return _map_short;
		else if(v >= _array_short)
			return _array_short;
		else if(v >= _string_short)
			return _string_short;
		else
			return _uint7;
	}

	inline const uint8_t* ReadTypeAndSize(Type& type, uint64_t size,
			const uint8_t* buffer, const uint8_t* end) {
		uint8_t v = *buffer;
		if(v >= _one_type_types) {
			type = (Type)v;
			buffer++;
			if(v <= _array_types_end) {
				buffer = Read<uint64_t>(size, buffer, end);
				switch(type) {
					case _string_long:
						size += _string_short_size;
						break;
					case _map_long:
						size += _map_short_size;
						break;
					case _array_long:
						size += _array_short_size;
						break;
					default:
						break;
				}
			} else {
				size = 0;
			}
		} else {
			buffer++;
			if(v >= _map_short) {
				type = _map_short;
			} else if(v >= _array_short) {
				type = _array_short;
			} else if(v >= _string_short) {
				type = _string_short;
			} else {
				type = _uint7;
				if(buffer > end)
					return NULL;
				size = 0;
				return buffer;
			}
			size = v - type;
		}
		if(buffer > end)
			return NULL;
		return buffer;
	}












	template<>
		inline void Write<int64_t>(int64_t value,
				std::vector<uint8_t, allocator_any<uint8_t>>& buffer) {
			if(value < 0) {
				value = -value;
				if(value < 0x100) {
					buffer.emplace_back(_sint8);
					buffer.emplace_back(value);
				} else if(value < 0x10000) {
					buffer.emplace_back(_sint16);
					buffer.resize(buffer.size()+2);
					endian::WriteBigEndiand<uint16_t>(value,
							buffer.data()+buffer.size()-2);
				} else if(value < 0x10000000) {
					buffer.emplace_back(_sint32);
					buffer.resize(buffer.size()+4);
					endian::WriteBigEndiand<uint64_t>(value,
							buffer.data()+buffer.size()-4);
				} else {
					buffer.emplace_back(_sint64);
					buffer.resize(buffer.size()+8);
					endian::WriteBigEndiand<uint64_t>(value,
							buffer.data()+buffer.size()-8);
				}
			} else {
				if(value <= _uint7_max) {
					buffer.emplace_back(value + _uint7);
				} else {
					value -= _uint7_max+1;
					if(value < 0x10000) {
						buffer.emplace_back(_uint16);
						buffer.resize(buffer.size()+2);
						endian::WriteBigEndiand<uint16_t>(value,
								buffer.data()+buffer.size()-2);
					} else if(value < 0x10000000) {
						buffer.emplace_back(_uint32);
						buffer.resize(buffer.size()+4);
						endian::WriteBigEndiand<uint64_t>(value,
								buffer.data()+buffer.size()-4);
					} else {
						buffer.emplace_back(_uint64);
						buffer.resize(buffer.size()+8);
						endian::WriteBigEndiand<uint64_t>(value,
								buffer.data()+buffer.size()-8);
					}
				}
			}
		}
	
	template<>
		inline void Write<int32_t>(int32_t value,
				std::vector<uint8_t, allocator_any<uint8_t>>& buffer) {
			Write<int64_t>(value, buffer);
		}
	
	template<>
		inline void Write<int16_t>(int16_t value,
				std::vector<uint8_t, allocator_any<uint8_t>>& buffer) {
			Write<int64_t>(value, buffer);
		}
	
	template<>
		inline void Write<int8_t>(int8_t value,
				std::vector<uint8_t, allocator_any<uint8_t>>& buffer) {
			Write<int64_t>(value, buffer);
		}
	
	template<>
		inline void Write<uint8_t>(uint8_t value,
				std::vector<uint8_t, allocator_any<uint8_t>>& buffer) {
			Write<int64_t>(value, buffer);
		}
	
	template<>
		inline void Write<uint16_t>(uint16_t value,
				std::vector<uint8_t, allocator_any<uint8_t>>& buffer) {
			Write<int64_t>(value, buffer);
		}
	
	template<>
		inline void Write<uint32_t>(uint32_t value,
				std::vector<uint8_t, allocator_any<uint8_t>>& buffer) {
			Write<int64_t>(value, buffer);
		}
	
	template<>
		inline void Write<uint64_t>(uint64_t value,
				std::vector<uint8_t, allocator_any<uint8_t>>& buffer) {
			Write<int64_t>(value, buffer);
		}
	
	template<>
		inline void Write<bool>(bool value,
				std::vector<uint8_t, allocator_any<uint8_t>>& buffer) {
			buffer.emplace_back(value ? _true : _false);
		}
	
	template<>
		inline void Write<const std::string&>(const std::string& value,
				std::vector<uint8_t, allocator_any<uint8_t>>& buffer) {
			if(value.size() < _string_short_size)
				buffer.emplace_back(_string_short + value.size());
			else
				Write<uint64_t>(value.size()-_string_short_size, buffer);
			buffer.insert(buffer.end(), value.begin(), value.end());
		}
	
	template<>
		inline void Write<const std::string_view&>(const std::string_view& value,
				std::vector<uint8_t, allocator_any<uint8_t>>& buffer) {
			if(value.size() < _string_short_size)
				buffer.emplace_back(_string_short + value.size());
			else
				Write<uint64_t>(value.size()-_string_short_size, buffer);
			buffer.insert(buffer.end(), value.begin(), value.end());
		}
	
	
	inline void InitMap(uint64_t elements,
			std::vector<uint8_t, allocator_any<uint8_t>>& buffer) {

	}
	
	inline void InitArray(Type type, uint64_t elements,
			std::vector<uint8_t, allocator_any<uint8_t>>& buffer) {

	}
	
	template<>
		inline void Write<const std::span<uint8_t>&>(
				const std::span<uint8_t>& value,
				std::vector<uint8_t, allocator_any<uint8_t>>& buffer) {
			
			
		}
}

#endif

