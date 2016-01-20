#include "AsmFile.h"

//#define BOOST_SPIRIT_DEBUG

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_symbols.hpp>
namespace qi=boost::spirit::qi;
#include <boost/spirit/include/phoenix.hpp>
namespace phx=boost::phoenix;

namespace
{
	template <typename Iterator, typename Skipper>
	struct AsmLineParser : qi::grammar<Iterator, AsmLine(), Skipper>
	{
		AsmLineParser(const string& fileName, const int& lineNo) : AsmLineParser::base_type(start,"AsmLine"), fileName(fileName), lineNo(lineNo)
		{
			using qi::lit;
			using qi::lexeme;
			using namespace qi::labels;
			using boost::spirit::ascii::char_;

			identifier = lexeme[(qi::alpha | char_('_')) >> *(qi::alnum | char_('_'))];
			identifier.name("identifier");
			//qi::debug(identifier);

			label = identifier >> lit(':');
			label.name("label");
			//qi::debug(label);

			qi::uint_parser<i32, 16, 1, -1> hex_num_parser;

			number = lexeme[qi::no_case[lit("0x")] > (hex_num_parser >> !char_("a-fA-F_0-9"))] | lexeme[qi::int_ >> !qi::digit];
			number.name("number");
			//qi::debug(number);

			constant = lexeme[lit('#') > number];
			constant.name("constant");
			//qi::debug(constant);

			ops.name("operation_code");
			{
				auto types = OpCodes::getTypes();
				for (auto it=types.cbegin();it!=types.cend();++it)
					ops.add(it->second,it->first);
			}

			op_rule = qi::lexeme[qi::no_case[ops]];
			op_rule.name("operation_mnem");
			//qi::debug(op_rule);
			qi::on_error<qi::fail>( op_rule, 
				std::cerr << phx::ref(fileName) << phx::val("@") << phx::ref(lineNo) << phx::val(" ") 
				<< phx::val("Unknown mnemonic ! Expecting ") <<  _4 <<  phx::val(" here: '") <<  phx::construct<std::string>(_3,_2) <<  phx::val("'\n")
				);

			regs.name("register");
			{
				for (int reg = 0;reg<Registers::REG_CONSTANT;reg++)
				{
					Registers::RegisterType realReg = static_cast<Registers::RegisterType>(reg);
					regs.add(Registers::toString(realReg),realReg);
				}
			}

			reg_rule = qi::no_case[regs];
			reg_rule.name("register_val");
			//qi::debug(reg_rule);

			auto negator_func = [](i32& val, qi::unused_type, qi::unused_type) -> void { val = -val; };
			regoffs_rule %= (reg_rule >> lit('+') > (number | identifier)) | (reg_rule >> lit('-') > number[ negator_func ]);
			regoffs_rule.name("register+offset");
			//qi::debug(regoffs_rule);

			duplication = qi::lexeme[qi::no_case[lit("dup")] > &qi::blank] > number;
			duplication.name("duplication");
			//qi::debug(duplication);
			qi::on_error<qi::fail>( duplication, 
				std::cerr << phx::ref(fileName) << phx::val("@") << phx::ref(lineNo) << phx::val(" ") 
				<< phx::val("Error! DUP expecting ") <<  _4	<<  phx::val(" here: '") <<  phx::construct<std::string>(_3,_2) <<  phx::val("'\n")
				);

			operands = (constant | regoffs_rule | reg_rule | identifier) % char_(',');
			operands.name("operands");
			//qi::debug(operands);

			start = -(label) > op_rule > -(operands) > -(duplication);
			start.name("AsmLine");
			//qi::debug(start);

			qi::on_error<qi::fail>( start, 
				std::cerr << phx::ref(fileName) << phx::val("@") << phx::ref(lineNo) << phx::val(" ") 
				<< phx::val("Error! Expecting ") <<  _4	<<  phx::val(" here: '") <<  phx::construct<std::string>(_3,_2) <<  phx::val("'\n")
				);
		}

		qi::rule<Iterator, string()>							identifier;
		qi::rule<Iterator, string(), Skipper>					label;
		qi::rule<Iterator, i32()>								number;
		qi::rule<Iterator, i32()>								constant;
		qi::symbols<char, OpCodes::OpCodeType>					ops;
		qi::rule<Iterator, OpCodes::OpCodeType()>				op_rule;
		qi::symbols<char, Registers::RegisterType>				regs;
		qi::rule<Iterator,Registers::RegisterType()>			reg_rule;
		qi::rule<Iterator,AsmLine::RegOffs(),Skipper>			regoffs_rule;
		qi::rule<Iterator,vector<AsmLine::Operand>(),Skipper>	operands;
		qi::rule<Iterator, int(), Skipper>						duplication;
		qi::rule<Iterator, AsmLine(), Skipper>					start;

		const string& fileName;
		const int& lineNo;
	};
};

#include "Common/PimplImpl.h"

class AsmFile::LexerImpl
{
public:
	LexerImpl(AsmFile& parent, const string& fileName) : parent(parent), fileName(fileName), lineNo(0), parser(fileName,lineNo) {}
	~LexerImpl() {}

	bool parseLine(int lineNo, const string& fileLine)
	{
		this->lineNo = lineNo;

		AsmLine line;
		auto first = fileLine.begin();
		auto last = fileLine.end();
		if (!qi::phrase_parse(first,last,parser,qi::blank_type(),line) || first != last)
			return false;
		else
		{
			parent.addParsedLine(lineNo,line);
			return true;
		}
	}
private:
	AsmFile& parent;

	string fileName;
	int lineNo;

	AsmLineParser<string::const_iterator,qi::blank_type> parser;
};

bool AsmFile::parseLine(int lineNo, const string& fileLine)
{
	 return lexerImpl->parseLine(lineNo, fileLine);
}

AsmFile::AsmFile(const string& fileName) : lexerImpl(*this,fileName) {}
AsmFile::~AsmFile() {}