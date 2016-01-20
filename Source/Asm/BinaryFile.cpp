#include "BinaryFile.h"
#include <sstream>
#include <cassert>

string BinaryFile::UndefinedSymbolException::toString() const 
{
	std::ostringstream str;
	str << "Undefined symbol: " << getName();
	return str.str();
}

string BinaryFile::DuplicateSymbolException::toString() const 
{
	std::ostringstream str;
	str << "Symbol: " << getName() << " already defined";
	return str.str();
}

string BinaryFile::OutOfRangeSymbolException::toString() const 
{
	std::ostringstream str;
	str << "Symbol: " << getName() << (isRelative()? string(" relatively"):string(" absolutely")) << " out of range: " << getVal();
	return str.str();
}

size_t BinaryFile::exportSymbols( SymbolEntries& out ) const
{
	size_t count = 0;
	for (auto it=definedSyms.cbegin();it!=definedSyms.cend();++it)
	{
		assert(it->first.length() > 0);
		if (it->first[0] == '_') //internal symbol
			continue;

		if (out.count(it->first) > 0)
			throw DuplicateSymbolException(it->first);

		out.insert(*it);
		count++;
	}

	return count;
}

u16 BinaryFile::calculateSymbol( const string& symName, size_t foundVal, bool rel, size_t currIP ) const
{
	int symVal = foundVal;
	if (symVal >= 0xFFFF)
		throw OutOfRangeSymbolException(symName,symVal,false);			
	if (rel)
	{
		symVal -= static_cast<int>(currIP);
		if (symVal < -32768)
			throw OutOfRangeSymbolException(symName,symVal,true);
	}	

	return static_cast<u16>(symVal);
}

u16 BinaryFile::resolveSymbol( size_t currIP, string symbolName, bool rel /*= false*/, const SymbolEntries* externalDict /*= nullptr*/ ) const
{
	if (currIP >= 0xFFFF)
		throw std::out_of_range("IP out of address space range");

	auto it = definedSyms.find(symbolName);
	if (it == definedSyms.end())
	{
		bool failed = true;
		if (externalDict)
		{
			it = externalDict->find(symbolName);
			if (it != externalDict->end())
				failed = false;
		}

		if (failed)
		{
			if (externalDict) //we failed, this symbol will never be found
				throw UndefinedSymbolException(symbolName);
			else
			{
				wantedSyms.push_back(SymbolSlot(symbolName,currIP,rel));
				return 0; //will be filled in later during "linking"
			}
		}
	}

	return calculateSymbol(it->first, it->second, rel, currIP);
}

#include <boost/lexical_cast.hpp>
using boost::lexical_cast;

void BinaryFile::verifyRegister( Registers::RegisterType reg, string name, int num /*= -1*/)
{
	if (reg >= Registers::REG_CONSTANT)
	{
		if (num >= 0)
			name += lexical_cast<string>(num);

		throw InstructionException(name + " register out of range: " + lexical_cast<string>(static_cast<int>(reg)));
	}
}

void BinaryFile::verifyInteger( i32 val, size_t size )
{
	switch (size)
	{
	case sizeof(u8):
		if (val >= 256 || val < -128)
			throw InstructionException("Integer overflow, trying to output byte, have " + lexical_cast<string>(val));
		break;
	case sizeof(u16):
		if (val >= 65536 || val < -65536)
			throw InstructionException("Integer overflow, trying to output word, have " + lexical_cast<string>(val));
		break;
	default:
		assert(0 && "Unknown integer size");
		break;
	}	
}

u8 BinaryFile::evalOperand( const AsmLine::Operand& op, size_t num, boost::optional<u16>* remainder /*= nullptr*/, bool dst /*= false*/, bool rel /*= false*/, size_t currIP /*= 0*/ )
{
	string regName = "src";
	if (dst)
		regName = "dst";
	else
		regName += lexical_cast<string>(num+1);

	if (op.type() == typeid(Registers::RegisterType))
	{
		auto srcReg = boost::get<Registers::RegisterType>(op);
		verifyRegister(srcReg,regName);
		return static_cast<u8>(srcReg) << ((1-num)*4);
	}
	else
	{
		if (remainder == nullptr)
			assert(0 && "Remainder null when we need it");

		if (op.type() == typeid(i32))
		{
			auto srcConst = boost::get<i32>(op);
			verifyInteger(srcConst,sizeof(u16));
			*remainder = static_cast<u16>(srcConst);
			return static_cast<u8>(Registers::REG_CONSTANT) << ((1-num)*4);
		}
		else if (op.type() == typeid(string))
		{
			auto srcSymbol = boost::get<const string&>(op);
			*remainder = resolveSymbol(currIP,srcSymbol,rel,nullptr);
			return static_cast<u8>(Registers::REG_CONSTANT) << ((1-num)*4);
		}
		else if (op.type() == typeid(AsmLine::RegOffs))
		{
			auto srcRegOffs = boost::get<const AsmLine::RegOffs&>(op);
			auto srcReg = srcRegOffs.first;
			verifyRegister(srcReg,regName);
			u8 operand = static_cast<u8>(srcReg) << ((1-num)*4);
			if (srcRegOffs.second.type() == typeid(i32))
			{
				auto offsInt = boost::get<i32>(srcRegOffs.second);
				verifyInteger(offsInt,sizeof(u16));
				*remainder = static_cast<u16>(offsInt);
			}
			else if (srcRegOffs.second.type() == typeid(string))
			{
				auto offsSym = boost::get<const string&>(srcRegOffs.second);
				*remainder = resolveSymbol(currIP,offsSym,rel,nullptr);
			}
			else
				assert(0 && "Unimplemented offset type");

			return operand;
		}
		else
		{
			assert(0 && "Unimplemented operand type");
			return 0;
		}
	}
}

size_t BinaryFile::synthesizeLine( const AsmLine& line )
{
	size_t currIP = byteCode.pos();

	if (!firstIP.is_initialized())
		firstIP = currIP;

	if (!line.validCode())
		throw InstructionException("Invalid operation id " + lexical_cast<string>(line.operation));

	if (line.labeled())
	{
		if (definedSyms.count(*line.label))
			throw DuplicateSymbolException(*line.label);

		definedSyms.insert(std::make_pair(*line.label,currIP));
	}

	if (line.directive())
	{
		if (line.countOperandType<Registers::RegisterType>() > 0 || line.countOperandType<AsmLine::RegOffs>() > 0)
			throw InstructionException("Cannot use register addressing in directives");

		int dups = 1;
		if (line.dupCount.is_initialized())
			dups = *(line.dupCount);

		for (int dupCnt=0;dupCnt<dups;dupCnt++)
		{
			for (size_t i=0;i<line.operands.size();i++)
			{
				if (line.operands.at(i).type() == typeid(i32))
				{
					auto op = line.getOperand<i32>(i);
					if (line.operation == OpCodes::DIR_DB)
					{
						verifyInteger(op,sizeof(u8));
						byteCode << static_cast<i8>(op);
					}
					else if (line.operation == OpCodes::DIR_DW)
					{
						verifyInteger(op,sizeof(u16));
						byteCode << static_cast<i16>(op);
					}
					else
						assert(0 && "Unimplemented DIRECTIVE");
				}
				else if (line.operands.at(i).type() == typeid(string))
				{
					auto op = line.getOperand<const string&>(i);
					if (line.operation == OpCodes::DIR_DB)
						throw InstructionException("Cannot output symbols (" + op + ") as bytes");
					else if (line.operation == OpCodes::DIR_DW)
						byteCode << static_cast<u16>(resolveSymbol(currIP,op));
					else
						assert(0 && "Unimplemented DIRECTIVE");
				}
			}
		}
	}
	else
	{
		if (OpCodes::numOperands(line.operation) != line.operands.size())
		{
			throw InstructionException("Invalid number of operands for " + OpCodes::toString(line.operation) + 
			" have " + lexical_cast<string>(line.operands.size()) + ", expecting " + lexical_cast<string>(OpCodes::numOperands(line.operation)));
		}

		size_t numConstants = line.countOperandType<i32>() + line.countOperandType<AsmLine::RegOffs>() + line.countOperandType<string>();
		if (numConstants > 1)
			throw InstructionException("Only one constant or offset is allowed per instruction, have " + lexical_cast<string>(numConstants));
		
		u8 firstByte = static_cast<u32>(line.operation) & 0xFF;
		u8 secondByte = 0;
		boost::optional<u16> offset;

		size_t opi = 0;
		if (line.basic())
		{
			size_t numRegOffs = line.countOperandType<AsmLine::RegOffs>();
			if (numRegOffs > 0)
				throw InstructionException("Register+Offset addressing not allowed in arithmetic instructions");

			//get dst
			if (line.operation != OpCodes::OP_CMP)
			{
				try
				{
					auto dstReg = line.getOperand<Registers::RegisterType>(opi++);
					verifyRegister(dstReg,"dst");
					firstByte |= static_cast<u32>(dstReg) & 0xF;
				}
				catch(const boost::bad_get&)
				{
					throw InstructionException("Destination must be a register !");
				}
			}

			//get srcs
			for (size_t i=0;i<line.operands.size()-opi;i++)
			{
				size_t opOffs = opi+i;
				secondByte |= evalOperand(line.operands.at(opOffs),i,&offset);
			}
		}
		else
		{
			bool rel = false;
			bool transferMem[] = {false,false}; //default for mov
			bool oneIsDst = false;
			switch (line.operation)
			{
				//no-operand
			case OpCodes::OP_CLC:
			case OpCodes::OP_STC:
			case OpCodes::OP_NC:
			case OpCodes::OP_RET:
			case OpCodes::OP_HLT:
				assert(secondByte == 0);
				assert(!offset.is_initialized());
				break;
				//branching
			case OpCodes::OP_JZ:
			case OpCodes::OP_JGT:
				rel = true; //for jz and jgt
			case OpCodes::OP_JMP:
			case OpCodes::OP_CALL:
				//memory instruction, must have offset, even if only using direct register
				offset = 0;
				//no dst, so no OR
				secondByte = evalOperand(line.operands.at(0),1,&offset,false,rel,currIP);
				assert(offset.is_initialized());
				break;
				//transfers
			case OpCodes::OP_LDR:
			case OpCodes::OP_STR:
				if (line.operation == OpCodes::OP_LDR)
				{
					transferMem[0] = false; //dst must be reg
					transferMem[1] = true; //src must be mem
				}
				else //str
				{
					transferMem[0] = true; //dst must be mem
					transferMem[1] = false; //src must be reg
				}
			case OpCodes::OP_MOV: //and ldr and str
				for (size_t i=0;i<line.operands.size();i++)
				{
					bool isDst = true;
					if (i > 0)
						isDst = false;

					if (transferMem[i] == false && (line.operation != OpCodes::OP_MOV || isDst)) //operand must be register (except in MOV reg, reg+offs, which is like LEA)
					{
						try
						{
							auto opReg = line.getOperand<Registers::RegisterType>(i);
							secondByte |= evalOperand(line.operands.at(i),i,nullptr,isDst);
						}
						catch(const boost::bad_get&)
						{
							throw InstructionException((isDst?(string("dst")):(string("src")+lexical_cast<string>(i+1))) + " must be a register !");
						}
					}
					else //operand is memory (or effective memory address)
					{
						//if we have memory, we must be 4 bytes even its just register
						offset = 0;
						secondByte |= evalOperand(line.operands.at(i),i,&offset,isDst);
					}
				}
				break;
				//one register
			case OpCodes::OP_IN:
			case OpCodes::OP_MOVF:
			case OpCodes::OP_MOVFSP:
				oneIsDst = true;
			case OpCodes::OP_OUT:			
			case OpCodes::OP_MOVTSP:
				//operand must be register
				try	{ auto opReg = line.getOperand<Registers::RegisterType>(0);	}
				catch(const boost::bad_get&) { throw InstructionException((oneIsDst?string("dst"):string("src")) + " must be a register !"); }
				//only one active, so no OR
				secondByte = evalOperand(line.operands.at(0),(oneIsDst?0:1),nullptr,oneIsDst);
				assert(!offset.is_initialized());
				break;
			default:
				assert(0 && "Unknown opcode in synthesizer!");
				break;
			}
		}

		byteCode << firstByte;
		byteCode << secondByte;
		if (offset.is_initialized())
			byteCode << *offset;
	}

	return byteCode.pos() - currIP;
}

size_t BinaryFile::resolveSelf()
{
	size_t lastIP = byteCode.pos();
	size_t numResolved = 0;
	SymbolSlots unsatisfied;
	for (auto it=wantedSyms.begin();it!=wantedSyms.end();++it)
	{
		auto found = definedSyms.find(it->symName);
		if (found == definedSyms.end())
			unsatisfied.push_back(*it);
		else
		{
			//get val
			u16 resolvedVal = calculateSymbol(found->first,found->second,it->rel,it->ip);
			//apply
			byteCode.pos(it->ip+sizeof(u16));
			assert((byteCode.size()-byteCode.pos()) >= sizeof(u16));
			assert(byteCode.contents()[byteCode.pos()] == 0 && byteCode.contents()[byteCode.pos()+1] == 0);
			byteCode << resolvedVal;
			assert(byteCode.pos() == (it->ip+sizeof(u32)));
			numResolved++;
		}
	}
	wantedSyms = std::move(unsatisfied);
	byteCode.pos(lastIP);
	return numResolved;
}

SymbolSlots BinaryFile::resolveFinal(const SymbolEntries& fullDict)
{
	size_t lastIP = byteCode.pos();
	SymbolSlots unsatisfied;
	for (auto it=wantedSyms.begin();it!=wantedSyms.end();++it)
	{
		try
		{
			//get val
			u16 resolvedVal = resolveSymbol(it->ip,it->symName,it->rel,&fullDict);
			//apply
			byteCode.pos(it->ip+sizeof(u16));
			assert((byteCode.size()-byteCode.pos()) >= sizeof(u16));
			assert(byteCode.contents()[byteCode.pos()] == 0 && byteCode.contents()[byteCode.pos()+1] == 0);
			byteCode << resolvedVal;
			assert(byteCode.pos() == (it->ip+sizeof(u32)));
		}
		catch (const UndefinedSymbolException&)
		{
			unsatisfied.push_back(*it);
		}
	}
	wantedSyms = std::move(unsatisfied);
	byteCode.pos(lastIP);
	return wantedSyms;
}