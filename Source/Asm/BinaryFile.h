#pragma once

#include "Common/BufferPtr.h"
#include "Common/Exception.h"
#include "AsmLine.h"

typedef map<string,size_t> SymbolEntries;
struct SymbolSlot
{
	SymbolSlot(string symName, size_t ip, bool rel = false) : symName(symName), ip(ip), rel(rel) {}

	string symName;
	size_t ip;
	bool rel;
};
typedef vector<SymbolSlot> SymbolSlots;

struct BinaryFile
{
	BinaryFile(BufferPtr& byteCode) : byteCode(byteCode) {}
	BinaryFile& operator = (const BinaryFile& other) { return *this; }

	struct AssemblyException : public GenericException<std::runtime_error>
	{
		AssemblyException(const char* staticName) : GenericException(staticName) {}
		~AssemblyException() NO_THROW {}
	};

	struct SymbolException : public AssemblyException
	{
		SymbolException(const char* staticName, string symName ) : AssemblyException(staticName), symName(symName) {}
		~SymbolException() NO_THROW {}
		const string& getName() const { return symName; }
	private:
		string symName;
	};

	struct UndefinedSymbolException : public SymbolException 
	{ 
		UndefinedSymbolException(string symName) : SymbolException("UndefinedSymbol",symName) {}
		~UndefinedSymbolException() NO_THROW {}
		string toString() const OVERRIDE;
	};

	struct DuplicateSymbolException : public SymbolException
	{
		DuplicateSymbolException(string symName) : SymbolException("DuplicateSymbol",symName) {}
		~DuplicateSymbolException() NO_THROW {}
		string toString() const OVERRIDE;
	};

	struct OutOfRangeSymbolException : public SymbolException
	{
		OutOfRangeSymbolException(string symName, int symVal, bool relative = false) 
			: SymbolException("OutOfRangeSymbol",symName), symVal(symVal), relative(relative) {}
		~OutOfRangeSymbolException() NO_THROW {}
		string toString() const OVERRIDE;
		int getVal() const { return symVal; }
		bool isRelative() const { return relative; }
	private:
		int symVal;
		bool relative;
	};

	struct InstructionException : public AssemblyException
	{
		InstructionException(string errorMsg) : AssemblyException("InstructionException"), errorMsg(errorMsg) {}
		~InstructionException() NO_THROW {}
		string toString() const OVERRIDE { return errorMsg; }
	private:
		string errorMsg;
	};

	size_t exportSymbols(SymbolEntries& out) const;
	size_t synthesizeLine(const AsmLine& line);
	size_t resolveSelf();
	SymbolSlots resolveFinal(const SymbolEntries& fullDict);
private:
	u16 calculateSymbol( const string& symName, size_t foundVal, bool rel, size_t currIP ) const;
	u16 resolveSymbol(size_t currIP, string symbolName, bool rel = false, const SymbolEntries* externalDict = nullptr) const;

	static void verifyRegister(Registers::RegisterType reg, string name, int num = -1);
	static void verifyInteger(i32 val, size_t size);
	u8 evalOperand(const AsmLine::Operand& op, size_t num, boost::optional<u16>* remainder = nullptr, bool dst = false, bool rel = false, size_t currIP = 0);

	BufferPtr& byteCode;

	SymbolEntries definedSyms;
	mutable SymbolSlots wantedSyms;
	boost::optional<size_t> firstIP;
};
