#pragma once

#include "Types.h"

struct Registers
{
	enum RegisterType
	{
		REG_R0 = 0,
		REG_R1,
		REG_R2,
		REG_R3,
		REG_R4,
		REG_R5,
		REG_R6,
		REG_R7,
		REG_R8,
		REG_R9,
		REG_R10,
		REG_R11,
		REG_R12,
		REG_R13,
		REG_R14,
		REG_CONSTANT,
		REG_COUNT
	};

	static bool reg(RegisterType reg) { return reg < REG_CONSTANT; }
	static bool constant(RegisterType reg) { return reg == REG_CONSTANT; }
	static bool valid(RegisterType reg) { return reg < REG_COUNT; }

	static string toString(RegisterType type);
};