#pragma once

#include "AsmLine.h"
#include "Common/Pimpl.h"

struct AsmFile
{
	AsmFile(const string& fileName);
	~AsmFile();

	typedef map<int,AsmLine> LineMap;
	const LineMap& getLines() const { return lines; }
	LineMap& accessLines() { return lines; }
	bool parseLine(int lineNo, const string& fileLine);
	const AsmLine& lastLine() { assert(lines.size() > 0); return lines.rbegin()->second; }
private:
	void addParsedLine(int lineNo, const AsmLine& newLine) { lines.insert(std::make_pair(lineNo,newLine)); }
	LineMap lines;

	class LexerImpl;
	friend class LexerImpl;
	Pimpl<LexerImpl> lexerImpl;
};
