#pragma once

#include "Common/Types.h"
#include "Common/Registers.h"
#include "Common/Exception.h"
#include "Common/Opcodes.h"
#include "Common/BufferPtr.h"

class Cpu
{
public:
	struct CpuException : public GenericException<std::runtime_error> 
	{ 
		CpuException(const char* staticName) : GenericException(staticName) {} 
		~CpuException() NO_THROW {}
	};
	struct InstructionException : public CpuException
	{
		InstructionException(size_t iep, string descr) : CpuException("InstructionException"), descr(descr), iep(iep) {}
		~InstructionException() NO_THROW {}
		const string& getDescr() const { return descr; }
		size_t getIEP() const { return iep; }
		string toString() const OVERRIDE;
	private:
		string descr;
		size_t iep;
	};

	struct MemoryException : public CpuException
	{
		MemoryException(BufferPtr::OutOfRangeException o) : CpuException("MemoryException"), o(o) {}
		~MemoryException() NO_THROW {}
		const BufferPtr::OutOfRangeException& getBufferExc() const { return o; }
		string toString() const OVERRIDE;
	private:
		BufferPtr::OutOfRangeException o;
	};

	Cpu(size_t memSize = 65536) : mem(memSize) {}
	void dumpState(std::ostream& out) const;

	union Op
	{
		Op() : u(0) {}
		Op(u16 v) : u(v) {}
		Op(i16 v) : i(v) {}

		inline operator u16() const { return u; }
		inline operator i16() const { return i; }

		u16 u;
		i16 i;
	};
	Op regs[Registers::REG_COUNT];

	struct Memory
	{
		Memory(size_t memSize = 65536) : bytes(memSize,0), iep(bytes), sp(bytes)  
		{
			iep.toBegin(); sp.toEnd();
		}

		void init(const u8* data, size_t len) { sp.put(0,data,len); }
	private:
		ByteVector bytes;
	public:
		BufferPtr iep, sp;

		Cpu::Op fetchOp(size_t offset) const
		{
			return Op(sp.read<u16>(offset)); //could be iep too, they both use the same underlying memory bytes
		}
		void putOp(size_t offset, Cpu::Op op)
		{
			sp.put(offset,reinterpret_cast<const u8*>(&(op.u)),sizeof(op.u));
		}
	} mem;

	struct Psw
	{
	private:
		enum PswBits
		{
			PSW_Z_OFFS = 0,
			PSW_Z_MASK = (1 << PSW_Z_OFFS),
			PSW_O_OFFS = 1,
			PSW_O_MASK = (1 << PSW_O_OFFS),
			PSW_C_OFFS = 2,
			PSW_C_MASK = (1 << PSW_C_OFFS),
			PSW_N_OFFS = 3,
			PSW_N_MASK = (1 << PSW_N_OFFS)
		}; //how to extract and compact from/to "real" psw register

		struct Data
		{
			Data() : z(false), o(false), c(false), n(false) {}
			Data(bool z, bool o, bool c, bool n) : z(z), o(o), c(c), n(n) {}
			Data(Op realPsw) : z(realPsw.u & PSW_Z_MASK), o(realPsw.u & PSW_O_MASK), c(realPsw.u & PSW_C_MASK), n(realPsw.u & PSW_N_MASK) {}

			bool z,o,c,n;
		};
		mutable Data val; //value of directly stored flags
		mutable u8 set; //which flags to read directly

		//for lazily calculating flags of arithmetic ops
		struct LazyState
		{
			LazyState() : op(OpCodes::OP_COUNT) {}
			bool oldC; //carry bit used when evaluating OP (for add and sub)
			Op src1,src2; //operands
			Op dst; //destination (result)
			OpCodes::OpCodeType op; //which op was used
		} ocLazy;
		//destination of last zn affecting op
		Op znDst;
	public:
		Psw() : set(PSW_Z_MASK | PSW_O_MASK | PSW_C_MASK | PSW_N_MASK) {}
		inline void setC(bool isSet) { val.c = isSet; set |= PSW_C_MASK; }
		inline void setC() { setC(true); }
		inline void clearC() { setC(false); }
		
		//faster to always lazy
		inline bool getZ() const { return (znDst.u == 0); }
		inline bool getN() const { return (znDst.i < 0); }
		//all operations that change zn will do this
		inline void setZN(Op dstReg) { znDst = dstReg; }		

		inline bool getC() const
		{
			if (set & PSW_C_MASK)
				return val.c;

			switch (ocLazy.op)
			{
			case OpCodes::OP_ADD: //add with carry
				val.c = (ocLazy.dst.u < ocLazy.src1.u) || (ocLazy.oldC && (ocLazy.dst.u == ocLazy.src1.u));
				break;
			case OpCodes::OP_CMP: //normal sub
				val.c = (ocLazy.src1.u < ocLazy.src2.u);
				break;
			case OpCodes::OP_SUB: //subtract with borrow
				val.c = (ocLazy.src1.u < ocLazy.dst.u) || (ocLazy.oldC && (ocLazy.src2.u==0xFFFF));
				break;
			}

			//to avoid branch next time
			set |= PSW_C_MASK;
			return val.c;
		}

		inline void arithmeticOp(OpCodes::OpCodeType type, Op src1, Op src2, Op dst, bool oldC = false)
		{
			setZN(dst);

			switch (type)
			{
			case OpCodes::OP_ADD:
			case OpCodes::OP_SUB:
				ocLazy.oldC = oldC; //we dont need oldC with CMP
			case OpCodes::OP_CMP:
				ocLazy.src1 = src1;
				ocLazy.src2 = src2;
				ocLazy.dst = dst;
				ocLazy.op = type;
				break;
			default:
				ocLazy.op = OpCodes::OP_COUNT;
				break;
			}
		}

		inline bool getO() const
		{
			if (set & PSW_O_MASK)
				return val.o;

			//overflow is set if both operands are the same sign, but the result is a different sign
			switch (ocLazy.op)
			{
			case OpCodes::OP_ADD: //add with carry
				val.o = ((ocLazy.src1.u ^ ocLazy.src2.u ^ 0x8000) & (ocLazy.dst.u ^ ocLazy.src2.u)) & 0x8000;
				break;
			case OpCodes::OP_CMP: //normal sub
			case OpCodes::OP_SUB: //subtract with borrow
				val.o = ((ocLazy.src1.u ^ ocLazy.src2.u) & (ocLazy.src1.u ^ ocLazy.dst.u)) & 0x8000;
				break;
			}

			//to avoid branch next time
			set |= PSW_O_MASK;
			return val.o;
		}

		inline operator u8() const { return (getZ() << PSW_Z_OFFS) | (getO() << PSW_O_OFFS) | (getC() << PSW_C_OFFS) | (getN() << PSW_N_OFFS); }
		inline operator u16() const { return static_cast<u8>(*this); }
		inline operator Op() const { return Op(static_cast<u16>(*this)); }

		string toString() const;
	} psw;

	bool execute(); // executes one instruction, return value is we should keep going or not
private:
	void fetchArithmeticOps(size_t currIEP, Registers::RegisterType src1, Registers::RegisterType src2, Op& op1, Op& op2);
	void arithmeticInstr(size_t currIEP, OpCodes::OpCodeType instr, Registers::RegisterType dst, Registers::RegisterType src1, Registers::RegisterType src2);
	static void CheckNull(size_t currIEP, Registers::RegisterType r, const char* regName);
	static void CheckReg(size_t currIEP, Registers::RegisterType r, const char* regName);
	inline Op resolveAddress(Registers::RegisterType reg)
	{
		Op offset(mem.iep.read<i16>());
		if (reg == Registers::REG_CONSTANT)
			return offset;
		else
			return Op(static_cast<i16>(regs[reg].i + offset.i));
	}
	bool otherInstr(size_t currIEP, OpCodes::OpCodeType instr, Registers::RegisterType dst, Registers::RegisterType src);
};
