#include <iostream>
#include <fstream>
#include "Common/Types.h"
#include "Cpu.h"

int main(int argc, const char* argv[])
{
	if (argc != 2)
	{
		std::cout << "Usage: Emu objectCode.o" << std::endl;
		return -1;
	}

	string fileName = argv[1];

	ByteVector objectCode;
	std::ifstream inFile(argv[1],std::ios::binary);
	if (!inFile.is_open())
	{
		std::cerr << "Error opening input file " << fileName << std::endl;
		return -1;
	}

	inFile.seekg(0,std::ios::end);
	if (inFile.tellg() > 0)
	{
		if (inFile.tellg() > 65536)
		{
			std::cerr << "Input file " << fileName << " is too big (" << inFile.tellg() << " , max is " << 65536 << ")" << std::endl;
			return -3;
		}
		objectCode.resize(inFile.tellg());
		inFile.seekg(0,std::ios::beg);
		inFile.read(reinterpret_cast<char*>(&objectCode[0]),objectCode.size());
	}
	else
	{
		std::cerr << "Input file " << fileName << " is zero-length!" << std::endl;
		return -2;
	}
	
	inFile.close();

	Cpu cpuCtx;
	cpuCtx.mem.init(&objectCode[0],objectCode.size());

	bool failed = false;
	for (;;)
	{
		try
		{
			if (cpuCtx.execute() == false)
				break;
		}
		catch(const Cpu::CpuException& e)
		{
			failed = true;
			std::cerr << e.toString() << std::endl;
			break;
		}
	}
	if (!failed)
	{
		std::cout << "Program properly finished executing" << std::endl;
		cpuCtx.dumpState(std::cout);
	}
	else
	{
		cpuCtx.dumpState(std::cerr);
		return -4;
	}

	return 0;
}