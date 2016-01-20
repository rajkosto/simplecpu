#pragma once

#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include "Common/Opcodes.h"
#include "Common/Registers.h"

namespace
{
	template <typename OpType>
	struct TypeCounterVisitor : public boost::static_visitor<void>
	{
		TypeCounterVisitor() : count(0) {}

		void operator()(const OpType&) const { count += 1; }
		template<typename T> void operator()(const T& other) const { count += 0; }

		mutable int count;
	};
};

struct AsmLine
{
	boost::optional<string> label;
	OpCodes::OpCodeType operation;
	typedef boost::variant<i32,string> OffsType;
	typedef std::pair<Registers::RegisterType,OffsType> RegOffs;
	typedef boost::variant<i32,RegOffs,Registers::RegisterType,string> Operand;
	vector<Operand> operands;
	boost::optional<int> dupCount;
	boost::optional<size_t> insPos;
	boost::optional<size_t> insSize;

	AsmLine() : operation(OpCodes::OP_COUNT) {}
	bool labeled() const { return label.is_initialized() && label->length() > 0; }
	bool exported() const { return labeled() && label->at(0) != '_'; }
	bool directive() const { return OpCodes::directive(operation); }
	bool basic() const { return OpCodes::basic(operation); }
	bool extended() const { return OpCodes::extended(operation); }
	bool opcode() const { return OpCodes::opcode(operation); }
	bool validCode() const { return OpCodes::valid(operation); }
	bool validDup() const { return (directive() || !dupCount.is_initialized()); }
	template<typename OpType>
	int countOperandType(size_t startOffs = 0) const
	{
		TypeCounterVisitor<OpType> countVis;

		if (operands.size() > startOffs)
		{
			for (auto it=operands.cbegin()+startOffs;it!=operands.cend();++it)
				boost::apply_visitor(countVis,*it);
		}		

		return countVis.count;
	}
	template<typename OpType>
	OpType getOperand(size_t pos) const { return boost::get<OpType>(operands.at(pos)); }

	string toString() const;
};

#include <boost/fusion/adapted.hpp>

BOOST_FUSION_ADAPT_STRUCT
(
	AsmLine,
	(boost::optional<string>, label)
	(OpCodes::OpCodeType, operation)
	(std::vector<AsmLine::Operand>, operands)
	(boost::optional<int>, dupCount)
	(boost::optional<size_t>, insPos)
	(boost::optional<size_t>, insSize)
);