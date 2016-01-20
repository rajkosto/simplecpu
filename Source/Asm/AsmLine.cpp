#include "AsmFile.h"
#include <sstream>

string AsmLine::toString() const
{
	std::stringstream str;
	if (label.is_initialized())
		str << *label << ":\t";
	else
		str << "\t\t";

	str << OpCodes::toString(operation) << " ";

	struct PrinterVisitor : public boost::static_visitor<void>
	{
		PrinterVisitor(std::ostream& out) : out(out) {}

		void operator()(const Registers::RegisterType val) const { out << Registers::toString(val); }
		void operator()(const AsmLine::RegOffs& val) const 
		{ 
			struct OffsetPrinter : public boost::static_visitor<void>
			{
				OffsetPrinter(std::ostream& out) : out(out) {}

				void operator()(const i32 val) const 
				{
					if (val >= 0)
						out << "+";
					out << val; 
				}
				void operator()(const string& val) const { out << "+" << val; }
			private:
				std::ostream& out;
			} offsetVis(out);

			(*this)(val.first);
			boost::apply_visitor(offsetVis,val.second);
		}
		void operator()(const i32 val) const { out << "#" << val; }
		void operator()(const string& val) const { out << val; }
	private:
		std::ostream& out;
	} printVis(str);

	for (auto it=operands.cbegin();it!=operands.cend();++it)
	{
		boost::apply_visitor(printVis,*it);
		if (operands.cend()-it > 1)
			str << ", ";
	}

	if (dupCount.is_initialized())
		str << " DUP " << *dupCount;

	return str.str();
}
