#include "Cpu.h"
#include <sstream>
#include <iostream>

namespace
{
	template<typename T>
	string ValueToString(T w)
	{
		char hexPos[16] = {0};
		char formatStr[] = "0x%00x";
		formatStr[4] = '0' + sizeof(T)*2;
		sprintf(hexPos,formatStr,static_cast<u32>(w));
		return hexPos;
	}
};

string Cpu::InstructionException::toString() const
{
	std::ostringstream str;
	str << "Error executing instruction at " << ValueToString(getIEP()) << " : " << getDescr();
	return str.str();
}

string Cpu::MemoryException::toString() const 
{
	return o.toString();
}

string Cpu::Psw::toString() const
{
	std::ostringstream str;
	str << "Z: " << getZ() << " O: " << getO() << " C: " << getC() << " N: " << getN();
	return str.str();
}

void Cpu::dumpState(std::ostream& out) const
{
	for (size_t i=0;i<lengthof(regs);i++)
		out << "R" << i << ": " << ValueToString(regs[i].u) << " ";
	out << std::endl;

	out << "FLAGS: " << psw.toString() << std::endl;
	out << "IEP: " << ValueToString(mem.iep.pos()) << " SP: " << ValueToString(mem.sp.pos()) << std::endl;
}

void Cpu::fetchArithmeticOps( size_t currIEP, Registers::RegisterType src1, Registers::RegisterType src2, Op& op1, Op& op2 )
{
	op1 = regs[src1];
	op2 = regs[src2];
	if (src1 == Registers::REG_CONSTANT || src2 == Registers::REG_CONSTANT)
	{
		Op constant(mem.iep.read<u16>());
		if (src1 == src2)
			throw InstructionException(currIEP, "Only src1 or src2 can be memory addresses, not both");
		else if (src1 == Registers::REG_CONSTANT)
			op1 = constant;
		else //src2 == Registers::REG_CONSTANT
			op2 = constant;
	}
}

bool Cpu::execute() /* executes one instruction, return value is we should keep going or not */
{
	try
	{
		u8 instr;
		u8 dst,src1,src2;
		size_t currIEP = mem.iep.pos();
		//std::cout << "IEP: " << ValueToString(currIEP) << std::endl;
		//fetch and expand first 2 bytes of instruction
		{
			instr = mem.iep.read<u8>();
			u8 secondByte = mem.iep.read<u8>();
			if (instr & 0x80) //non arithmetic instr
			{
				dst = (secondByte >> 4) & 0x0F;
				src1 = (secondByte >> 0) & 0x0F;
				return otherInstr(currIEP,
					static_cast<OpCodes::OpCodeType>(instr),
					static_cast<Registers::RegisterType>(dst),
					static_cast<Registers::RegisterType>(src1));
			}
			else //3 op instr
			{
				dst = instr & 0x0F;
				instr &= 0xF0;
				src1 = (secondByte >> 4) & 0x0F;
				src2 = (secondByte >> 0) & 0x0F;

				arithmeticInstr(currIEP, 
					static_cast<OpCodes::OpCodeType>(instr),
					static_cast<Registers::RegisterType>(dst),
					static_cast<Registers::RegisterType>(src1),
					static_cast<Registers::RegisterType>(src2));
				return true;
			}
		}	
	}
	catch (const BufferPtr::OutOfRangeException& memE)
	{
		throw MemoryException(memE);
	}
}



void Cpu::arithmeticInstr( size_t currIEP, OpCodes::OpCodeType instr, Registers::RegisterType dst, Registers::RegisterType src1, Registers::RegisterType src2 )
{
	if (dst >= Registers::REG_CONSTANT)
		throw InstructionException(currIEP, "dst must be a register");

	Op op1,op2;

	switch (instr)
	{
	case OpCodes::OP_ADD:
		{
			fetchArithmeticOps(currIEP,src1,src2,op1,op2);
			u16 carryBit = psw.getC();
			regs[dst].u = op1.u + op2.u + carryBit;
			psw.arithmeticOp(instr,op1,op2,regs[dst],carryBit);
		}
		break;
	case OpCodes::OP_SUB:
		{
			fetchArithmeticOps(currIEP,src1,src2,op1,op2);
			i16 carryBit = psw.getC();
			regs[dst].i = op1.i - op2.i - carryBit;
			psw.arithmeticOp(instr,op1,op2,regs[dst],carryBit);
		}
		break;
	case OpCodes::OP_CMP:
		fetchArithmeticOps(currIEP,src1,src2,op1,op2);
		psw.arithmeticOp(instr,op1,op2,static_cast<i16>(op1.i - op2.i));
		break;
	case OpCodes::OP_SAR:
		{
			fetchArithmeticOps(currIEP,src1,src2,op1,op2);
			i16 shifted = op1.i;
			bool newCarry = ((shifted >>= op2.u) << op2.u) != op1.i;
			psw.setC(newCarry);
			regs[dst].i = shifted;
			psw.setZN(regs[dst]);
		}
		break;
		case OpCodes::OP_SAL:
		{
			fetchArithmeticOps(currIEP,src1,src2,op1,op2);
			i16 shifted = op1.i;
			bool newCarry = ((shifted <<= op2.u) >> op2.u) != op1.i;
			psw.setC(newCarry);
			regs[dst].i = shifted;
			psw.setZN(regs[dst]);
		}
		break;
	case OpCodes::OP_AND:
		fetchArithmeticOps(currIEP,src1,src2,op1,op2);
		regs[dst].u = op1.u & op2.u;
		psw.setZN(regs[dst]);
		break;
	case OpCodes::OP_OR:
		fetchArithmeticOps(currIEP,src1,src2,op1,op2);
		regs[dst].u = op1.u | op2.u;
		psw.setZN(regs[dst]);
		break;
	case OpCodes::OP_NOT:
		if (src2 != 0)
			throw InstructionException(currIEP,"OP_NOT src2 must be 0");

		fetchArithmeticOps(currIEP,src1,src2,op1,op2);
		regs[dst].u = ~(op1.u);
		psw.setZN(regs[dst]);
		break;
	default:
		throw InstructionException(currIEP,"Unknown instruction opcode");
		break;
	}
}

void Cpu::CheckNull( size_t currIEP, Registers::RegisterType r, const char* regName )
{
	if (r != 0)
		throw InstructionException(currIEP,regName + string(" must be zero!"));
}

void Cpu::CheckReg( size_t currIEP, Registers::RegisterType r, const char* regName )
{
	if (r >= Registers::REG_CONSTANT)
		throw InstructionException(currIEP,regName + string(" must be a register!"));
}

bool Cpu::otherInstr( size_t currIEP, OpCodes::OpCodeType instr, Registers::RegisterType dst, Registers::RegisterType src )
{
	if (dst >= Registers::REG_CONSTANT)
		throw InstructionException(currIEP, "dst must be a register");

	switch(instr)
	{
	case OpCodes::OP_JMP:
		CheckNull(currIEP,dst,"dst");
		mem.iep.pos(resolveAddress(src).u);
		break;
	case OpCodes::OP_JZ:
		{
			CheckNull(currIEP,dst,"dst");
			i16 relAddr = resolveAddress(src).i;
			if (psw.getZ())
				mem.iep.pos(static_cast<int>(currIEP)+relAddr);
		}
		break;
	case OpCodes::OP_JGT:
		{
			CheckNull(currIEP,dst,"dst");
			i16 relAddr = resolveAddress(src).i;
			if ((psw.getZ()==0) && (psw.getN()==psw.getO()))
				mem.iep.pos(static_cast<int>(currIEP)+relAddr);
		}		
		break;
	case OpCodes::OP_MOV:
		CheckReg(currIEP,dst,"dst");
		regs[dst] = resolveAddress(src); //register val + 0 or register val + offset or just offset
		psw.setZN(regs[dst]);
		break;
	case OpCodes::OP_LDR:
		{
			CheckReg(currIEP,dst,"dst");
			u16 wantedAddress = resolveAddress(src).u;
			//std::cout << "Loading " << ValueToString(wantedAddress) << " to " << Registers::toString(dst) << std::endl;
			regs[dst] = mem.fetchOp(wantedAddress);
			psw.setZN(regs[dst]);
		}		
		break;
	case OpCodes::OP_STR:
		{
			CheckReg(currIEP,src,"src");
			u16 wantedAddress = resolveAddress(dst).u;
			//std::cout << "Storing " << Registers::toString(src) << "(" << ValueToString(regs[src].u) << ") to " << ValueToString(wantedAddress) << std::endl;
			mem.putOp(wantedAddress,regs[src]);
		}
		break;
	case OpCodes::OP_IN:
		CheckNull(currIEP,src,"src");
		CheckReg(currIEP,dst,"dst");
		std::cout << "IEP: " << ValueToString(currIEP) << " Enter value into " << Registers::toString(dst) << ":";
		std::cin >> regs[dst].i;
		psw.setZN(regs[dst]);
		break;
	case OpCodes::OP_OUT:
		CheckReg(currIEP,src,"src");
		CheckNull(currIEP,dst,"dst");
		std::cout << "IEP: " << ValueToString(currIEP) << " Value of " << Registers::toString(src) << ": " << regs[src].i << std::endl;
		break;
	case OpCodes::OP_CLC:
		CheckNull(currIEP,src,"src");
		CheckNull(currIEP,dst,"dst");
		psw.clearC();
		break;
	case OpCodes::OP_STC:
		CheckNull(currIEP,src,"src");
		CheckNull(currIEP,dst,"dst");
		psw.setC();
		break;
	case OpCodes::OP_NC:
		CheckNull(currIEP,src,"src");
		CheckNull(currIEP,dst,"dst");
		psw.setC(!psw.getC());
		break;
	case OpCodes::OP_MOVF:
		CheckNull(currIEP,src,"src");
		CheckReg(currIEP,dst,"dst");
		regs[dst].u = static_cast<u16>(psw);
		break;
	case OpCodes::OP_MOVTSP:
		CheckReg(currIEP,src,"src");
		CheckNull(currIEP,dst,"dst");
		mem.sp.pos(regs[src].u);
		break;
	case OpCodes::OP_MOVFSP:
		CheckNull(currIEP,src,"src");
		CheckReg(currIEP,dst,"dst");
		regs[dst].u = mem.sp.pos();
		break;
	case OpCodes::OP_CALL:
		{
			CheckNull(currIEP,dst,"dst");
			u16 funcLoc = resolveAddress(src).u;
			mem.sp.backtrack(sizeof(u16));
			mem.putOp(mem.sp.pos(),Op(static_cast<u16>(mem.iep.pos())));
			mem.iep.pos(funcLoc);
		}
		break;
	case OpCodes::OP_RET:
		CheckNull(currIEP,src,"src");
		CheckNull(currIEP,dst,"dst");
		mem.iep.pos(mem.sp.read<u16>());
		break;
	case OpCodes::OP_HLT:
		return false;
	}

	return true;
}
