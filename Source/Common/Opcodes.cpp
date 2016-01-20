#include "Opcodes.h"

map<OpCodes::OpCodeType,string> OpCodes::types;

void OpCodes::populate()
{
	if (!types.size())
	{
		using std::make_pair;

		types.insert(make_pair(OP_ADD, "add"));
		types.insert(make_pair(OP_SUB, "sub"));
		types.insert(make_pair(OP_CMP, "cmp"));
		types.insert(make_pair(OP_SAR, "sar"));
		types.insert(make_pair(OP_SAL, "sal"));
		types.insert(make_pair(OP_AND, "and"));
		types.insert(make_pair(OP_OR,	"or"));
		types.insert(make_pair(OP_NOT, "not"));

		types.insert(make_pair(OP_JMP, "jmp"));
		types.insert(make_pair(OP_JZ,	"jz"));
		types.insert(make_pair(OP_JGT, "jgt"));
		types.insert(make_pair(OP_MOV, "mov"));
		types.insert(make_pair(OP_LDR, "ldr"));
		types.insert(make_pair(OP_STR, "str"));
		types.insert(make_pair(OP_IN,	"in"));
		types.insert(make_pair(OP_OUT, "out"));
		types.insert(make_pair(OP_CLC, "clc"));
		types.insert(make_pair(OP_STC, "stc"));
		types.insert(make_pair(OP_NC,	"nc"));
		types.insert(make_pair(OP_MOVF, "movf"));
		types.insert(make_pair(OP_MOVTSP, "movtsp"));
		types.insert(make_pair(OP_MOVFSP, "movfsp"));
		types.insert(make_pair(OP_CALL, "call"));
		types.insert(make_pair(OP_RET, "ret"));
		types.insert(make_pair(OP_HLT, "hlt"));

		types.insert(make_pair(DIR_DB, "db"));
		types.insert(make_pair(DIR_DW, "dw"));
	}
}


string OpCodes::toString( OpCodeType type )
{
	populate();
	auto it = types.find(type);

	if (it == types.end())
		return "UNKNOWN";
	else 
		return it->second;
}

int OpCodes::numOperands( OpCodeType operation )
{
	switch (operation)
	{
	case OP_ADD:
	case OP_SUB:
	case OP_SAR:
	case OP_SAL:
	case OP_AND:
	case OP_OR:
		return 3;
	case OP_NOT:
	case OP_CMP:
	case OP_MOV:
	case OP_LDR:
	case OP_STR:
		return 2;
	case OP_JMP:
	case OP_JZ:
	case OP_JGT:
	case OP_IN:
	case OP_OUT:
	case OP_MOVF:
	case OP_MOVTSP:
	case OP_MOVFSP:
	case OP_CALL:
		return 1;
	case OP_CLC:
	case OP_STC:
	case OP_NC:
	case OP_RET:
	case OP_HLT:
		return 0;
	default:
		return -1;
	}
}
