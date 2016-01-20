#pragma once

#include "Types.h"

struct OpCodes
{
	enum OpCodeType
	{
		//3-op
		OP_ADD = (0 << 7) | (0 << 4),
		OP_SUB = (0 << 7) | (1 << 4),
		OP_CMP = (0 << 7) | (2 << 4),
		OP_SAR = (0 << 7) | (3 << 4),
		OP_SAL = (0 << 7) | (4 << 4),
		OP_AND = (0 << 7) | (5 << 4),
		OP_OR  = (0 << 7) | (6 << 4),
		OP_NOT = (0 << 7) | (7 << 4),

		//2-op
		OP_JMP = (1 << 7) | 0,
		OP_JZ,
		OP_JGT,
		OP_MOV,
		OP_LDR,
		OP_STR,
		OP_IN,
		OP_OUT,
		OP_CLC,
		OP_STC,
		OP_NC,
		OP_MOVF,
		OP_MOVTSP,
		OP_MOVFSP,
		OP_CALL,
		OP_RET,
		OP_HLT,
		OP_COUNT,

		//directives
		DIR_DB = (1 << 16),
		DIR_DW,
		DIR_COUNT
	};

	static string toString(OpCodeType type);

	static bool directive(OpCodeType operation) { return operation >= DIR_DB && operation < DIR_COUNT; }
	static bool basic(OpCodeType operation) { return operation >= OP_ADD && operation <= OP_NOT; }
	static bool extended(OpCodeType operation) { return operation >= OP_JMP && operation < OP_COUNT; }
	static bool opcode(OpCodeType operation) { return basic(operation) || extended(operation); }
	static bool valid(OpCodeType operation) { return opcode(operation) || directive(operation); }

	static int numOperands(OpCodeType operation);

	static const map<OpCodeType,string>& getTypes() { populate(); return types; }

private:
	static void populate();
	static map<OpCodeType,string> types;
};