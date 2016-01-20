#include "Registers.h"

#include <sstream>

string Registers::toString( RegisterType type )
{
	std::ostringstream str;
	if (reg(type))
		str << "r" << static_cast<int>(type);
	else if (constant(type))
		str << "CONSTANT";
	else
		str << "UNKNOWN";

	return str.str();
}
