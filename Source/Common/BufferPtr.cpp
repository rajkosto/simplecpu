#include "BufferPtr.h"

#include <sstream>
#include <cassert>
#include <cstring>

using std::memcpy;

std::string BufferPtr::OutOfRangeException::toString() const
{
	std::ostringstream str;
	str << "Attempted to " << (add ? string("put") : string("get")) << " in BufferPtr (pos: " << pos << " size: " << size << ") value with size: " << esize;
	return str.str();
}

void BufferPtr::append(const u8* src, size_t cnt)
{
	if (!cnt)
		return;

	if ((pos() + cnt) > 65536)
		throw OutOfRangeException(true,pos(),cnt,size());
	
	if (_storage.size() < pos() + cnt)
		_storage.resize(pos() + cnt);

	memcpy(&_storage[pos()], src, cnt);
	pos(pos()+cnt);
}

void BufferPtr::read(u8 *dest, size_t len)
{
	if(pos() + len > size())
		throw OutOfRangeException(false, pos(), len, size());

	memcpy(dest, &_storage[pos()], len);
	pos(pos()+len);
}

void BufferPtr::trim(size_t skipB)
{
	if (this->size() < skipB)
		throw OutOfRangeException(false,pos(),skipB,size());
	else if (this->size() == skipB)
		this->clear();
	else
	{
		size_t remainSize = this->size()-skipB;
		memmove(&_storage[0],this->contents()+skipB,remainSize);
		this->resize(remainSize);
	}
}

void BufferPtr::put(size_t pos, const u8 *src, size_t cnt)
{
	if(pos + cnt > size())
		throw OutOfRangeException(true, pos, cnt, size());
	memcpy(&_storage[pos], src, cnt);
}
