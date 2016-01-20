#pragma once

#include <stdexcept>
#include <ostream>

template <typename ExceptType>
class GenericException : public ExceptType
{
public:
	GenericException(const char* staticName) : ExceptType(staticName) {}
	virtual ~GenericException() NO_THROW {}

	virtual std::string toString() const = 0;
	const char* what() const NO_THROW { _str = this->toString(); return _str.c_str(); }
	void print(std::ostream& out) const { out << this->toString() << std::endl; }
private:
	mutable std::string _str;
};
