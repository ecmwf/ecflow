/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_parser_VariableParser_HPP
#define ecflow_node_parser_VariableParser_HPP

///
/// \brief
///   edit fred 'val'
///   edit FRED "smscheck %WSHOST% %EXPVER% %ECF_JOB%"
///
/// In the old SMS single quotes are used if no interpretation
/// is required and double quotes to allow variable substitution
/// However this is ONLY a requirement for CDP.
/// Note when OLD SMS uses the show command it always outputs with single quotes
///
/// Do we remove the quotes when we store internally ?
///   i.e if we have :
///      edit YMD '20090901'
/// Then the value '20090901' is not immediately convertible to an integer
/// *************************************************************
/// This is required for evaluation in the abstract syntax tree
/// *************************************************************
///
/// Hence we will do the following:
///   a/ On parsing always remove quotes ie single or double
///   b/ On serialising always add single quotes
///

#include "ecflow/node/parser/Parser.hpp"

class VariableParser : public Parser {
public:
    explicit VariableParser(DefsStructureParser* p, bool parsing_defs = false)
        : Parser(p),
          parsing_defs_(parsing_defs) {}
    const char* keyword() const override { return "edit"; }
    bool doParse(const std::string& line, std::vector<std::string>& lineTokens) override;

private:
    bool parsing_defs_;
};

#endif /* ecflow_node_parser_VariableParser_HPP */
