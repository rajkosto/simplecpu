#pragma once

#include "Types.h"
#include "Exception.h"
#include <ostream>

class BufferPtr
{
public:
	class OutOfRangeException : public GenericException<std::runtime_error>
	{
	public:
		OutOfRangeException(bool _add, size_t _pos, size_t _esize, size_t _size)
			: GenericException("BufferPtr::OutOfRange"), add(_add), pos(_pos), esize(_esize), size(_size) {}

		std::string toString() const OVERRIDE;
	private:
		bool add;
		size_t pos;
		size_t esize;
		size_t size;
	};

	template<class T>
	struct Unused
	{
		Unused() {}
	};

public:
	BufferPtr(ByteVector& storage): _pos(0), _storage(storage) {}
	BufferPtr(ByteVector& storage, size_t res): _pos(0), _storage(storage) {_storage.reserve(res);}

	void clear()
	{
		_storage.clear();
		pos(0);
	}

	template <typename T> void put(size_t pos,T value)
	{
		put(pos,(u8 *)&value,sizeof(value));
	}

	BufferPtr& operator<<(bool value)
	{
		u8 result = (value)?1:0;
		append<u8>(result);
		return *this;
	}

	BufferPtr& operator<<(char value)
	{
		append<char>(value);
		return *this;
	}

	BufferPtr& operator<<(u8 value)
	{
		append<u8>(value);
		return *this;
	}

	BufferPtr& operator<<(u16 value)
	{
		append<u16>(value);
		return *this;
	}

	BufferPtr& operator<<(u32 value)
	{
		append<u32>(value);
		return *this;
	}

	// signed as in 2e complement
	BufferPtr& operator<<(i8 value)
	{
		append<i8>(value);
		return *this;
	}

	BufferPtr& operator<<(i16 value)
	{
		append<i16>(value);
		return *this;
	}

	BufferPtr& operator<<(i32 value)
	{
		append<i32>(value);
		return *this;
	}

	BufferPtr& operator>>(bool& value)
	{
		value = read<char>() > 0 ? true : false;
		return *this;
	}

	BufferPtr& operator>>(char& value)
	{
		value = read<char>();
		return *this;
	}

	BufferPtr& operator>>(u8& value)
	{
		value = read<u8>();
		return *this;
	}

	BufferPtr& operator>>(u16& value)
	{
		value = read<u16>();
		return *this;
	}

	BufferPtr& operator>>(u32& value)
	{
		value = read<u32>();
		return *this;
	}

	//signed as in 2e complement
	BufferPtr& operator>>(i8& value)
	{
		value = read<i8>();
		return *this;
	}

	BufferPtr& operator>>(i16& value)
	{
		value = read<i16>();
		return *this;
	}

	BufferPtr& operator>>(i32& value)
	{
		value = read<i32>();
		return *this;
	}

	template<class T>
	BufferPtr& operator>>(Unused<T> const&)
	{
		read_skip<T>();
		return *this;
	}

	template <typename T> T fetch()
	{
		T readMe;
		(*this) >> readMe;
		return readMe;
	}

	u8 operator[](size_t pos) const
	{
		return read<u8>(pos);
	}

	size_t pos() const { return _pos; }
	size_t pos(size_t pos_)
	{
		_pos = pos_;
		return _pos;
	}
	void advance(size_t len) 
	{ 
		pos(pos()+len); 
		if (pos() > size()) 
			toEnd(); 
	}
	void backtrack(size_t len) 
	{ 
		if (len > pos()) 
			pos(0); 
		else 
			pos(pos()-len); 
	}
	void toBegin() { pos(0); }
	void toEnd() { pos(size()); }

	template<typename T>
	void read_skip() { read_skip(sizeof(T)); }

	void read_skip(size_t skip)
	{
		if(pos() + skip > size())
			throw OutOfRangeException(false, pos(), skip, size());

		pos(pos()+skip);
	}

	template <typename T> T read()
	{
		T r = read<T>(pos());
		pos(pos()+sizeof(T));
		return r;
	}

	template <typename T> T read(size_t pos) const
	{
		if(pos + sizeof(T) > size())
			throw OutOfRangeException(false, pos, sizeof(T), size());

		T val = *((T const*)&_storage[pos]);
		return val;
	}

	void read(u8* dest, size_t len);

	template <typename T>
	void read(T* dest, size_t numElems)
	{
		return read(reinterpret_cast<u8*>(dest),numElems*sizeof(T));
	}

	void read(BufferPtr& another, size_t len)
	{
		if(pos() + len > size())
			throw OutOfRangeException(false, pos(), len, size());

		another.append(&this->contents()[pos()],len);
		pos(pos()+len);
	}

	void read(BufferPtr& another)
	{
		this->read(another,this->remaining());
	}

	const u8* contents() const { return &_storage[0]; }
	u8* contents() { return &_storage[0]; }

	size_t size() const { return _storage.size(); }
	bool empty() const { return _storage.empty(); }

	size_t remaining() const
	{
		return this->size() - pos();
	}

	void resize(size_t newsize)
	{
		_storage.resize(newsize);
		if (pos() > size())
			pos(size());
	}

	void trim(size_t skipB);
	void trim() { trim(pos()); }

	void reserve(size_t ressize)
	{
		if (ressize > size())
			_storage.reserve(ressize);
	}

	void append(const std::string& str, bool zeroterm=false)
	{
		if (zeroterm)
			append((u8 const*)str.c_str(), str.length() + 1);
		else
			append((u8 const*)str.data(), str.size());
	}

	void append(const char *src, size_t cnt)
	{
		return append((const u8 *)src, cnt);
	}

	template<class T> void append(const T *src, size_t cnt)
	{
		return append((const u8 *)src, cnt * sizeof(T));
	}

	template<class T> void append(const std::vector<T>& input)
	{
		return append((const u8 *)&input[0], input.size()*sizeof(T));
	}

	void append(const u8 *src, size_t cnt);

	void append(const BufferPtr& buffer, bool all=true)
	{
		if (all)
			append(buffer.contents(), buffer.size());
		else if(buffer.pos())
			append(buffer.contents(), buffer.pos());
	}

	void put(size_t pos, const u8* src, size_t cnt);

private:
	// private to disallow appending of any type
	template <typename T> void append(T value)
	{
		append((u8 *)&value, sizeof(value));
	}

protected:
	size_t _pos;
	ByteVector& _storage;
};

template<typename T, size_t N>
inline BufferPtr& operator<<(BufferPtr& b, T(&arr)[N])
{
	b.append(&arr[0], N);
	return b;
}

template<typename T, size_t N>
inline BufferPtr& operator>>(BufferPtr& b, T(&arr)[N])
{
	b.read(&arr[0], N);
	return b;
}

template <typename T>
inline BufferPtr& operator<<(BufferPtr& b, const vector<T>& v)
{
	b << static_cast<u32>(v.size());
	for (auto it = v.begin(); it != v.end(); ++it)
		b << *it;

	return b;
}

template <typename T>
inline BufferPtr& operator>>(BufferPtr& b, vector<T>& v)
{
	v.resize(b.fetch<u32>());
	for(size_t i=0;i<v.size();i++)
		b >> v[i];

	return b;
}
