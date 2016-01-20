#include "Common/Types.h"
#include <fstream>
#include <boost/algorithm/string/trim.hpp>
#include <set>
#include <memory>
#include <iostream>

#include "AsmFile.h"
#include "BinaryFile.h"

namespace
{
	std::ostream& Bin2Hex(const u8* what, size_t len, std::ostream& out)
	{
		for (size_t i=0;i<len;i++)
		{
			char testMe[16] = {0};
			sprintf(testMe,"%02x",static_cast<u32>(what[i]));
			out << testMe;
			if (i != len-1)
				out << " ";
		}
		return out;
	}
};

#include <boost/program_options.hpp>
namespace po=boost::program_options;

int main(int argc, const char* argv[])
{
	vector<string> inputFileNames;
	string outputFileName;
	bool generateListing = true;
	string listingFileName;

	//options parsing
	{
		po::options_description generic("Options");
		generic.add_options()
			("version,v", "print version info")
			("help,h", "produce help message")
			("output,o", po::value<string>(&outputFileName)->default_value("a.out"), "filename to put binary output of assembled program")
			("skip-listing,s", "skip generation of listing")
			("listing-file,l", po::value<string>(&listingFileName), "filename to put text listing of the assembled program")
			;

		po::options_description hidden("");
		hidden.add_options()
			("input-file", po::value< vector<string> >(), "input assembly files"); 

		po::options_description cmdline_options;
		cmdline_options.add(generic).add(hidden);

		po::positional_options_description positionals;
		positionals.add("input-file", -1);

		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).options(cmdline_options).positional(positionals).run(), vm);
		po::notify(vm);

		if (vm.count("version"))
		{
			std::cout << "Asm 0.1" << std::endl;
			return 1;
		}

		if (vm.count("help")) 
		{
			std::cout << "Usage: asm [options] mainfile.s [morefiles.s]" << std::endl;
			std::cout << generic << std::endl;
			return 1;
		}

		if (vm.count("skip-listing")) 
			generateListing = false;

		if (vm.count("input-file"))
			inputFileNames = vm["input-file"].as< vector<string> >();
		else
		{
			std::cerr << "No input files specified" << std::endl;
			return -1;
		}
	}

	int filesGood = 0;
	int filesBad = 0;

	//lexed files
	vector<std::shared_ptr<AsmFile>> lexed;

	//for each file
	for (auto fileNameIt=inputFileNames.cbegin();fileNameIt!=inputFileNames.cend();++fileNameIt)
	{
		const string& fileName = *fileNameIt;
		std::ifstream fileStr(fileName);
		if (!fileStr.is_open())
		{
			std::cerr << "Cannot open input file " << fileName << std::endl;
			filesBad++;
			continue;
		}

		std::cout << fileName << std::endl;

		lexed.emplace_back(std::make_shared<AsmFile>(fileName));
		AsmFile& fileCtx = *(lexed.rbegin()->get());

		int lineNo = 0;
		int errors = 0;
		string fileLine;
		//lex each line
		while (std::getline(fileStr,fileLine))
		{
			lineNo++;
			auto commentStart = fileLine.find("//");
			if (commentStart != string::npos)
				fileLine = fileLine.substr(0,commentStart);
			
			boost::trim(fileLine);
			if (fileLine.length() < 1)
				continue;

			if (!fileCtx.parseLine(lineNo,fileLine))
			{
				errors++;
				continue;
			}

			auto line = fileCtx.lastLine();

			if (!line.validDup())
			{
				std::cerr << fileName << "@" << lineNo << " Error! DUP cannot be used with " << OpCodes::toString(line.operation) << std::endl;
				errors++;
				continue;
			}
		}

		if (errors > 0)
		{
			std::cout << fileName << " : " << errors << " syntax errors during lexing" << std::endl;
			filesBad++;
			continue;
		}
	}

	if (filesBad > 0)
		return -1;

	ByteVector outData;
	BufferPtr outBuf(outData);
	SymbolEntries globalSyms;
	vector<BinaryFile> binaryFiles;

	bool assemblyFailed = false;
	for (auto it=lexed.begin();it!=lexed.end();++it)
	{
		string fileName = inputFileNames[it-lexed.begin()];
		binaryFiles.emplace_back(outBuf);
		BinaryFile& currFile = *binaryFiles.rbegin();
		AsmFile& fileCtx = *(it->get());
		AsmFile::LineMap& lines = fileCtx.accessLines();
		for (AsmFile::LineMap::iterator line=lines.begin();line!=lines.end();++line)
		{
			//synthesize lines into bytecode
			try
			{
				size_t prevIP = outBuf.pos();
				AsmLine& theLine = line->second;
				size_t insSize = currFile.synthesizeLine(theLine);
				theLine.insPos = prevIP;
				theLine.insSize = insSize;
			}
			catch(const BinaryFile::AssemblyException& e)
			{
				std::cerr << fileName << "@" << line->first << " ERROR: " << e.toString() << std::endl;
				assemblyFailed = true;
			}
		}

		//resolve self-symbols
		try
		{
			currFile.resolveSelf();
		}
		catch(const BinaryFile::SymbolException& e)
		{
			std::cerr << fileName << " ERROR: " << e.toString() << std::endl;
			assemblyFailed = true;
		}

		//export symbols to the rest of the world
		try
		{
			size_t numExported = currFile.exportSymbols(globalSyms);
			std::cout << fileName << " exporting " << numExported << " symbols" << std::endl;
		}
		catch(const BinaryFile::DuplicateSymbolException& e)
		{
			std::cerr << fileName << " ERROR: " << e.toString() << std::endl;
			assemblyFailed = true;
		}	
	}

	if (assemblyFailed)
		return -2;

	//link all the files together
	std::set<string> unresolved;
	for (auto it=binaryFiles.begin();it!=binaryFiles.end();++it)
	{
		auto unFile = it->resolveFinal(globalSyms);
		for (auto unresIt=unFile.begin();unresIt!=unFile.end();++unresIt)
			unresolved.insert(unresIt->symName);
	}
	if (unresolved.size() > 0)
	{
		for (auto it=unresolved.begin();it!=unresolved.end();++it)
			std::cerr << "LINK ERR: " << BinaryFile::UndefinedSymbolException(*it).toString() << std::endl;

		return -3; //link failed
	}
	
	//generate listing
	if (generateListing)
	{
		if (listingFileName.length() < 1)
			listingFileName = outputFileName + ".txt";

		std::ofstream listingFile(listingFileName,std::ios::trunc);
		if (!listingFile.is_open())
		{
			std::cerr << "Error opening output listing file " << listingFileName << std::endl;
			return -1;
		}
		for (auto it=lexed.begin();it!=lexed.end();++it)
		{
			string fileName = inputFileNames[it-lexed.begin()];
			listingFile << "//FILE: " << fileName << std::endl;
			AsmFile& fileCtx = *(it->get());
			auto lines = fileCtx.getLines();
			for (auto line=lines.begin();line!=lines.end();++line)
			{
				listingFile << line->second.toString() << " //(" << line->first;
				if (line->second.insPos.is_initialized() && line->second.insSize.is_initialized())
				{
					char hexPos[16] = {0};
					sprintf(hexPos,"0x%04x",line->second.insPos.get());

					listingFile << " @ " << hexPos << ") = ";
					
					Bin2Hex(&outBuf.contents()[line->second.insPos.get()],line->second.insSize.get(),listingFile);
				}
				else
					listingFile << ")";

				listingFile << std::endl;
			}
		}
	}

	//generate output file
	{
		std::ofstream outputFile(outputFileName,std::ios::binary|std::ios::trunc);
		if (!outputFile.is_open())
		{
			std::cerr << "Error opening output binary file " << outputFileName << std::endl;
			return -1;
		}
		outputFile.write((const char*)outBuf.contents(),outBuf.size());
	}

	return 0;
}