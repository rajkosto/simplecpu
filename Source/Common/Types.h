#pragma once

#include <cstdint>
typedef std::uint8_t	u8;
typedef std::int8_t		i8;
typedef std::int16_t	i16;
typedef std::uint16_t	u16;
typedef std::int32_t	i32;
typedef std::uint32_t	u32;

#include <vector>
using std::vector;
typedef std::vector<u8> ByteVector;

#include <string>
using std::string;

#include <map>
using std::map;

//sizeof for counting array elements
template<typename T, size_t size>
size_t lengthof(T(&)[size]){return size;}

#ifdef _MSC_VER
#define NO_THROW
#define OVERRIDE override
#else
#define NO_THROW noexcept
#define OVERRIDE
#endif
