/*
 *  This file is part of ICon3. Please see README for details.
 *  Copyright (C) 2020 Marek Zalewski aka Drwalin
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

/*
 * Types:
 *   array_short
 *   array_long
 *   string_short
 *   string_long
 *   object_small
 *   object_big
 *   array_typed:
 *     uint8
 *     uint16
 *     uint32
 *     uint64
 *     bool
 *     float
 *   uint_uber_short <1B
 *   int8
 *   int16
 *   int32
 *   int64
 *   uint8
 *   uint16
 *   uint32
 *   uint64
 *   true
 *   false
 *   float // [ int(exponent), int(mantissa) ]
 */

namespace ReaderWriter {
	// uint8[] == int8[] = char[] = string
	enum Type {
		_null    = 0x00,
		_true    = 0x01,
		_false   = 0x02,
		_real    = 0x03, // [ int exponent, int mantissa ]
		_uint8   = 0x04,
		_uint16  = 0x05,
		_uint32  = 0x06,
		_uint64  = 0x07,
		_sint8   = 0x08,
		_sint16  = 0x09,
		_sint32  = 0x0A,
		_sint64  = 0x0B,

		_array_long   = 0x0C,  // [ int size, any[size+0x10]        ]
		_object_long  = 0x0D,  // [ int size, {any ,any}[size+0x10] ]
		_string_long  = 0x0E,  // [ int size, uint8[size+0x40]      ]

		_array_uint16  = 0x0F,  // [ int size, uint16[size]      ]
		_array_uint32  = 0x10,  // [ int size, uint32[size]      ]
		_array_uint64  = 0x11,  // [ int size, uint64[size]      ]
		_array_bool    = 0x12,  // [ int size, uint8[(size+7)/8] ]
		_array_float   = 0x13,  // [ int size, float[size+0x0C]  ]

		_array_float_short = 0x14,  // 0x14:0x1F -> 0x01:0x0C : [0x0C]
		_array_short       = 0x20,  // 0x20:0x2F -> 0x01:0x10 : [0x10]
		_object_short      = 0x30,  // 0x30:0x3F -> 0x01:0x10 : [0x10]
		_string_short      = 0x60,  // 0x40:0x7F -> 0x01:0x40 : [0x40]
		_uint7             = 0x80,  // 0x80:0xFF =  0x01:0x80 : [0x80]
	};



	inline Type GetType(const uint8_t* buffer);

	inline const uint8_t* ReadTypeSize(Type& type, uint64_t* size,
			const uint8_t* buffer, const uint8_t* end);

	template<typename T>
		inline const uint8_t* Read(T& value, const uint8_t* buffer, const uint8_t* end);

	template<typename T>
		inline void Write(T value,
				std::vector<uint8_t, allocator_any<uint8_t>>& buffer);
	
	inline const uint8_t* ReadString(char**const str, size_t& size,
				const uint8_t* buffer, const uint8_t* end);



	template<>
		inline const uint8_t* Read<uint64_t>(uint64_t& value,
				const uint8_t* buffer, const uint8_t* end) {
			if(buffer >= end)
				return NULL;
			uint8_t v = *buffer;
			size_t mod = 1;
			switch(v) {
				case _null:
					value = 0;
					mod = 1;
				case _uint8:
					value = *(uint8_t*)(buffer+1);
					mod = 2;
				case _uint16:
					value = endian::ReadBigEndiand<uint16_t>(buffer+1);
					mod = 3;
				case _uint32:
					value = endian::ReadBigEndiand<uint32_t>(buffer+1);
					mod = 5;
				case _uint64:
					value = endian::ReadBigEndiand<uint64_t>(buffer+1);
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
			switch(*buffer) {
				case _false:
					value = false;
					return buffer+1;
				case _true:
					value = true;
					return buffer+1;
					return NULL;
			}
			return NULL;
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
		inline const uint8_t* Read<std::string>(std::string& value,
				const uint8_t* buffer, const uint8_t* end) = delete;/* {
			size_t size;
			char* const str = NULL;
			buffer = ReadString((char**const)&str, size, buffer, end);
			if(buffer) {
				value.clear();
				value.insert(value.end(), str, str+size);
				return buffer;
			}
			return NULL;
		}*/
		
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
			buffer = ReadTypeSize(type, &size, buffer, end);
			if(type == _null) {
				*str = NULL;
				return NULL;
			}
			if(type == _string_long || type == _string_short) {
				*str = (char*const)buffer;
				buffer += size;
				if(buffer > end)
					return NULL;
				return buffer;
			}
			return NULL;
	}






	inline Type GetType(const uint8_t* buffer) {
		uint8_t v = ((uint8_t*)buffer)[0];
		if(v < _array_float_short)
			return (Type)v;
		else if(v < _array_short)
			return _array_float_short;
		else if(v < _object_short)
			return _array_short;
		else if(v < _string_short)
			return _object_short;
		else if(v < _uint7)
			return _string_short;
		else
			return _uint7;
	}

	inline const uint8_t* ReadTypeSize(Type& type, uint64_t* size,
			const uint8_t* buffer, const uint8_t* end) {
		uint8_t v = ((uint8_t*)buffer)[0];
		if(v < _array_float_short) {
			type = (Type)v;
			if(type>=_array_long && type<=_array_float) {
				buffer = Read<uint64_t>(*size, buffer+1, end);
				switch(type) {
					case _array_long:
					case _object_long:
						size += 0x10;
						break;
					case _string_long:
						size += 0x40;
						break;
					case _array_float:
						size += 0x0c;
						break;
					default:
						return buffer;
				}
				return buffer;
			}
			*size = 0;
		} else {
			if(v < _array_short)
				type = _array_float_short;
			else if(v < _object_short)
				type = _array_short;
			else if(v < _string_short)
				type = _object_short;
			else if(v < _uint7)
				type = _string_short;
			else {
				type = _uint7;
				*size = 0;
				return buffer+1;
			}
			*size = v - (uint8_t)type;
		}
		return buffer+1;
	}












	template<>
		inline void Write<int64_t>(int64_t value,
				std::vector<uint8_t, allocator_any<uint8_t>>& buffer) {
			if(value == 0) {
				buffer.emplace_back(_null);
			} else if(value < 0) {
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
				if(value < 0x80) {
					buffer.emplace_back(value + _uint7);
				} else if(value < 0x100) {
					buffer.emplace_back(_uint8);
					buffer.emplace_back(value);
				} else if(value < 0x10000) {
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
			if(value.size() <= 0x40)
				buffer.emplace_back(_string_short + value.size());
			else
				Write<uint64_t>(value.size(), buffer);
			buffer.insert(buffer.end(), value.begin(), value.end());
		}
}

#endif

