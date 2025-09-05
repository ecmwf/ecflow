/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

// Un-comment these for selective for debugging. At the moment because we have added
//            ast generation BOOST_SPIRIT_DEBUG is to verbose, making debugging a pain.
// Typically, I will comment out ONLY BOOST_SPIRIT_DEBUG and enable all the others
// #define BOOST_SPIRIT_DEBUG
// #define BOOST_SPIRIT_DUMP_PARSETREE_AS_XML
// #define PRINT_TREE
// #define PRINT_AST_TRAVERSAL
// #define PRINT_AST

#include "ecflow/node/ExprParser.hpp"

#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stack>
#include <string>

#include <boost/spirit/include/classic.hpp>
#include <boost/spirit/include/classic_actor.hpp>
#include <boost/spirit/include/classic_ast.hpp>
#include <boost/spirit/include/classic_attribute.hpp>
#include <boost/spirit/include/classic_confix.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_tree_to_xml.hpp>
#include <boost/spirit/include/phoenix1_binders.hpp>

#include "ecflow/attribute/NodeAttr.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/ExprAst.hpp"
#include "ecflow/node/ExprDuplicate.hpp"
#include "formatter/DefsWriter.hpp"

// Reference
// ‘*’   Zero or more
// ‘!’   Zero or one
// ‘+’   One or more
// ‘>>’  Sequence/concatenation
// ‘|’   alternate
// ‘-‘   not

////////////////////////////////////////////////////////////////////////////
using namespace ecf;
using namespace std;
using namespace boost::spirit;
using namespace phoenix;
using namespace BOOST_SPIRIT_CLASSIC_NS;

/////////////////////////////////////////////////////////////////////////////
typedef tree_match<const char*> treematch_t;
typedef treematch_t::tree_iterator tree_iter_t;

//////////////////////////////////////////////////////////////////////////////

struct ExpressionGrammer : public grammar<ExpressionGrammer>
{
    // The parser object is copied a lot, so only use reference/pointer variables as data members
    ExpressionGrammer()  = default;
    ~ExpressionGrammer() = default;

    static const int integer_ID            = 1;
    static const int dot_path_ID           = 2;
    static const int equal_1_ID            = 3;
    static const int equal_2_ID            = 4;
    static const int not_equal_1_ID        = 5;
    static const int not_equal_2_ID        = 6;
    static const int node_name_ID          = 7;
    static const int greater_equals_1_ID   = 8;
    static const int greater_equals_2_ID   = 9;
    static const int less_equals_1_ID      = 10;
    static const int less_equals_2_ID      = 11;
    static const int less_than_1_ID        = 12;
    static const int less_than_2_ID        = 13;
    static const int greater_than_1_ID     = 14;
    static const int greater_than_2_ID     = 15;
    static const int node_state_unknown_ID = 16;

    static const int node_state_complete_ID  = 18;
    static const int node_state_queued_ID    = 19;
    static const int node_state_submitted_ID = 20;
    static const int node_state_active_ID    = 21;
    static const int node_state_aborted_ID   = 22;

    static const int not1_ID                = 23;
    static const int not2_ID                = 24;
    static const int not3_ID                = 25;
    static const int and_ID                 = 26;
    static const int or_ID                  = 27;
    static const int dot_dot_path_ID        = 29;
    static const int base_trigger_ID        = 31;
    static const int sub_expression_ID      = 32;
    static const int node_path_state_ID     = 34;
    static const int absolute_path_ID       = 35;
    static const int event_state_ID         = 36;
    static const int variable_ID            = 37;
    static const int plus_ID                = 42;
    static const int minus_ID               = 43;
    static const int multiply_ID            = 44;
    static const int divide_ID              = 45;
    static const int modulo_ID              = 46;
    static const int calc_expression_ID     = 47;
    static const int calc_factor_ID         = 48;
    static const int calc_term_ID           = 49;
    static const int calc_grouping_ID       = 50;
    static const int calc_subexpression_ID  = 51;
    static const int basic_variable_path_ID = 52;
    static const int compare_expression_ID  = 53;

    static const int cal_date_to_julian_ID = 54;
    static const int cal_julian_to_date_ID = 55;
    static const int cal_argument_ID       = 56;

    static const int flag_path_ID       = 57;
    static const int flag_late_ID       = 58;
    static const int flag_zombie_ID     = 59;
    static const int flag_archived_ID   = 60;
    static const int flag_ID            = 61;
    static const int root_path_ID       = 62;
    static const int parent_variable_ID = 63;

    static const int datetime_ID = 64;

    template <typename ScannerT>
    struct definition
    {
        rule<ScannerT, parser_tag<cal_date_to_julian_ID>> cal_date_to_julian;
        rule<ScannerT, parser_tag<cal_julian_to_date_ID>> cal_julian_to_date;
        rule<ScannerT, parser_tag<cal_argument_ID>> cal_argument;

        rule<ScannerT, parser_tag<integer_ID>> integer;
        rule<ScannerT, parser_tag<plus_ID>> plus;
        rule<ScannerT, parser_tag<minus_ID>> minus;
        rule<ScannerT, parser_tag<multiply_ID>> multiply;
        rule<ScannerT, parser_tag<divide_ID>> divide;
        rule<ScannerT, parser_tag<modulo_ID>> modulo;

        rule<ScannerT, parser_tag<equal_1_ID>> equal_1;
        rule<ScannerT, parser_tag<equal_2_ID>> equal_2;
        rule<ScannerT, parser_tag<not_equal_1_ID>> not_equal_1;
        rule<ScannerT, parser_tag<not_equal_2_ID>> not_equal_2;
        rule<ScannerT, parser_tag<node_name_ID>> nodename;

        rule<ScannerT, parser_tag<greater_equals_1_ID>> greater_equals_1;
        rule<ScannerT, parser_tag<greater_equals_2_ID>> greater_equals_2;
        rule<ScannerT, parser_tag<less_equals_1_ID>> less_equals_1;
        rule<ScannerT, parser_tag<less_equals_2_ID>> less_equals_2;

        rule<ScannerT, parser_tag<less_than_1_ID>> less_than_1;
        rule<ScannerT, parser_tag<less_than_2_ID>> less_than_2;
        rule<ScannerT, parser_tag<greater_than_1_ID>> greater_than_1;
        rule<ScannerT, parser_tag<greater_than_2_ID>> greater_than_2;

        rule<ScannerT, parser_tag<node_state_unknown_ID>> node_state_unknown;
        rule<ScannerT, parser_tag<node_state_complete_ID>> node_state_complete;
        rule<ScannerT, parser_tag<node_state_queued_ID>> node_state_queued;
        rule<ScannerT, parser_tag<node_state_submitted_ID>> node_state_submitted;
        rule<ScannerT, parser_tag<node_state_active_ID>> node_state_active;
        rule<ScannerT, parser_tag<node_state_aborted_ID>> node_state_aborted;

        rule<ScannerT, parser_tag<not1_ID>> not1_r;
        rule<ScannerT, parser_tag<not2_ID>> not2_r;
        rule<ScannerT, parser_tag<not3_ID>> not3_r;
        rule<ScannerT, parser_tag<and_ID>> and_r;
        rule<ScannerT, parser_tag<or_ID>> or_r;

        rule<ScannerT, parser_tag<dot_path_ID>> dotpath;
        rule<ScannerT, parser_tag<dot_dot_path_ID>> dotdotpath;
        rule<ScannerT, parser_tag<absolute_path_ID>> absolutepath;
        rule<ScannerT, parser_tag<root_path_ID>> root_path;

        rule<ScannerT, parser_tag<node_path_state_ID>> nodepathstate;

        rule<ScannerT, parser_tag<event_state_ID>> event_state;
        rule<ScannerT, parser_tag<variable_ID>> variable;

        rule<ScannerT, parser_tag<calc_expression_ID>> calc_expression;
        rule<ScannerT, parser_tag<calc_factor_ID>> calc_factor;
        rule<ScannerT, parser_tag<calc_term_ID>> calc_term;
        rule<ScannerT, parser_tag<calc_grouping_ID>> calc_grouping;
        rule<ScannerT, parser_tag<calc_subexpression_ID>> calc_subexpression;
        rule<ScannerT, parser_tag<basic_variable_path_ID>> basic_variable_path;
        rule<ScannerT, parser_tag<parent_variable_ID>> parent_variable;
        rule<ScannerT, parser_tag<compare_expression_ID>> compare_expression;

        rule<ScannerT, parser_tag<flag_path_ID>> flag_path;
        rule<ScannerT, parser_tag<flag_late_ID>> flag_late;
        rule<ScannerT, parser_tag<flag_zombie_ID>> flag_zombie;
        rule<ScannerT, parser_tag<flag_archived_ID>> flag_archived;
        rule<ScannerT, parser_tag<flag_ID>> flag;

        rule<ScannerT> not_r, less_than_comparable, expression, andExpr, operators;
        rule<ScannerT> nodestate, equality_comparible, and_or, nodepath;

        rule<ScannerT, parser_tag<datetime_ID>> datetime;

        //        ‘*’   Zero or more
        //        ‘!’   Zero or one
        //        ‘+’   One or more
        //        ‘>>’  Sequence/concatenation
        //        ‘|’   alternate
        //        ‘-‘   not
        explicit definition(ExpressionGrammer const& /*self*/) {
            nodename = leaf_node_d[lexeme_d[(alnum_p || ch_p('_')) >> *(alnum_p || ch_p('_') || ch_p('.'))]];

            // Can be /suite/family/task ||  family/task ||   family

            absolutepath = leaf_node_d[!(str_p("/")) >> nodename >> *(+str_p("/") >> nodename)];
            dotdotpath // a kind of relative path
                = leaf_node_d[str_p("..") >> *(+str_p("/") >> str_p("..")) >> +(+str_p("/") >> nodename)];

            dotpath  = leaf_node_d[str_p(".") >> +(str_p("/") >> nodename)];
            nodepath = absolutepath | dotdotpath | dotpath;

            // Integer is distinct from task/family names that are integers, since nodes with integer
            // names that occur in trigger/complete expression must have path ./0 ./1
            integer   = leaf_node_d[uint_p];
            plus      = root_node_d[str_p("+")];
            minus     = root_node_d[str_p("-")];
            divide    = root_node_d[str_p("/")];
            multiply  = root_node_d[str_p("*")];
            modulo    = root_node_d[str_p("%")];
            operators = plus | minus | divide | multiply | modulo;

            equal_1             = root_node_d[str_p("==")];
            equal_2             = root_node_d[str_p("eq")];
            not_equal_1         = root_node_d[str_p("!=")];
            not_equal_2         = root_node_d[str_p("ne")];
            equality_comparible = equal_1 | equal_2 | not_equal_2 | not_equal_1;

            greater_equals_1 = root_node_d[str_p(">=")];
            greater_equals_2 = root_node_d[str_p("ge")];
            less_equals_1    = root_node_d[str_p("<=")];
            less_equals_2    = root_node_d[str_p("le")];
            less_than_1      = root_node_d[str_p("<")];
            less_than_2      = root_node_d[str_p("lt")];
            greater_than_1   = root_node_d[str_p(">")];
            greater_than_2   = root_node_d[str_p("gt")];
            // Prioritise to most common first, to speed up parsing
            less_than_comparable = greater_equals_2 | less_equals_2 | greater_than_2 | less_than_2 | greater_equals_1 |
                                   less_equals_1 | less_than_1 | greater_than_1;

            not1_r = root_node_d[str_p("not ")];
            not2_r = root_node_d[str_p("~")];
            not3_r = root_node_d[str_p("!")];
            not_r  = not1_r | not3_r | not2_r;

            and_r  = root_node_d[str_p("and")] || root_node_d[str_p("&&")] || root_node_d[str_p("AND")];
            or_r   = root_node_d[str_p("or")] || root_node_d[str_p("||")] || root_node_d[str_p("OR")];
            and_or = and_r | or_r;

            event_state = leaf_node_d[str_p("set")] || leaf_node_d[str_p("clear")];

            node_state_unknown   = root_node_d[str_p("unknown")];
            node_state_complete  = root_node_d[str_p("complete")];
            node_state_queued    = root_node_d[str_p("queued")];
            node_state_submitted = root_node_d[str_p("submitted")];
            node_state_active    = root_node_d[str_p("active")];
            node_state_aborted   = root_node_d[str_p("aborted")];
            nodestate            = node_state_complete | node_state_aborted | node_state_queued | node_state_active |
                        node_state_submitted | node_state_unknown;

            flag_late     = root_node_d[str_p("late")];
            flag_zombie   = root_node_d[str_p("zombie")];
            flag_archived = root_node_d[str_p("archived")];
            flag          = flag_late | flag_zombie | flag_archived;

            variable            = leaf_node_d[nodename];
            basic_variable_path = nodepath >> discard_node_d[ch_p(':')] >> variable;
            parent_variable =
                ch_p(':') >> variable; // if we discard_node, then we get just 'variable' and NOT parent_variable

            root_path = leaf_node_d[(str_p("/"))];
            flag_path = (nodepath | root_path) >> discard_node_d[str_p("<flag>")] >> flag;

            cal_argument = basic_variable_path | integer;
            cal_date_to_julian =
                str_p("cal::date_to_julian") >> discard_node_d[ch_p('(')] >> cal_argument >> discard_node_d[ch_p(')')];
            cal_julian_to_date =
                str_p("cal::julian_to_date") >> discard_node_d[ch_p('(')] >> cal_argument >> discard_node_d[ch_p(')')];
            datetime = leaf_node_d[lexeme_d[repeat_p(8)[digit_p] >> 'T' >> repeat_p(6)[digit_p]]];

            calc_factor = datetime | integer | basic_variable_path |
                          discard_node_d[ch_p('(')] >> calc_expression >> discard_node_d[ch_p(')')] | flag_path |
                          parent_variable | root_node_d[operators] >> calc_factor | cal_date_to_julian |
                          cal_julian_to_date;
            calc_term       = calc_factor >> *(root_node_d[operators] >> calc_factor);
            calc_expression = calc_term >> *(root_node_d[operators] >> calc_term);

            nodepathstate = nodepath >> equality_comparible >> nodestate;

            compare_expression = nodepathstate | basic_variable_path >> equality_comparible >> event_state |
                                 root_node_d[calc_expression >> *((equality_comparible | less_than_comparable) >>
                                                                  (!not_r >> calc_expression))];

            // We need to take special care so that 'and' has a higher priority than 'or'
            // (( This is done by have a custom rule for the and
            andExpr = !not_r >> compare_expression >> *(and_r >> !not_r >> compare_expression);

            calc_grouping = !not_r >> discard_node_d[ch_p('(')] >> calc_subexpression >> discard_node_d[ch_p(')')];

            calc_subexpression = (andExpr | calc_grouping) >> *((and_r | or_r) >> calc_subexpression);

            expression = calc_subexpression >> end_p;

            BOOST_SPIRIT_DEBUG_NODE(cal_argument);
            BOOST_SPIRIT_DEBUG_NODE(cal_date_to_julian);
            BOOST_SPIRIT_DEBUG_NODE(andExpr);
            BOOST_SPIRIT_DEBUG_NODE(not_r);
            BOOST_SPIRIT_DEBUG_NODE(not1_r);
            BOOST_SPIRIT_DEBUG_NODE(not2_r);
            BOOST_SPIRIT_DEBUG_NODE(expression);
            BOOST_SPIRIT_DEBUG_NODE(nodename);
            BOOST_SPIRIT_DEBUG_NODE(nodepath);
            BOOST_SPIRIT_DEBUG_NODE(dotdotpath);
            BOOST_SPIRIT_DEBUG_NODE(dotpath);
            BOOST_SPIRIT_DEBUG_NODE(absolutepath);
            BOOST_SPIRIT_DEBUG_NODE(less_than_comparable);
            BOOST_SPIRIT_DEBUG_NODE(nodestate);
            BOOST_SPIRIT_DEBUG_NODE(equality_comparible);
            BOOST_SPIRIT_DEBUG_NODE(equal_1);
            BOOST_SPIRIT_DEBUG_NODE(equal_2);
            BOOST_SPIRIT_DEBUG_NODE(not_equal_1);
            BOOST_SPIRIT_DEBUG_NODE(not_equal_2);
            BOOST_SPIRIT_DEBUG_NODE(and_or);
            BOOST_SPIRIT_DEBUG_NODE(plus);
            BOOST_SPIRIT_DEBUG_NODE(minus);
            BOOST_SPIRIT_DEBUG_NODE(divide);
            BOOST_SPIRIT_DEBUG_NODE(multiply);
            BOOST_SPIRIT_DEBUG_NODE(modulo);
            BOOST_SPIRIT_DEBUG_NODE(nodepathstate);
            BOOST_SPIRIT_DEBUG_NODE(integer);
            BOOST_SPIRIT_DEBUG_NODE(event_state);
            BOOST_SPIRIT_DEBUG_NODE(variable);
            BOOST_SPIRIT_DEBUG_NODE(basic_variable_path);
            BOOST_SPIRIT_DEBUG_NODE(parent_variable);
            BOOST_SPIRIT_DEBUG_NODE(flag_path);
            BOOST_SPIRIT_DEBUG_NODE(calc_factor);
            BOOST_SPIRIT_DEBUG_NODE(calc_expression);
            BOOST_SPIRIT_DEBUG_NODE(calc_term);
            BOOST_SPIRIT_DEBUG_NODE(compare_expression);
            BOOST_SPIRIT_DEBUG_NODE(datetime);
        };

        rule<ScannerT> const& start() const { return expression; }
    };
};

/////////////////////////////////////////////////////////////////////////////////////////////

void print(tree_parse_info<> info, const std::string& expr, const std::map<parser_id, std::string>& rule_names);

AstTop* createTopAst(tree_parse_info<> info,
                     const std::string& expr,
                     const std::map<parser_id, std::string>& rule_names,
                     std::string& error_msg);

/////////////////////////////////////////////////////////////////////////////////////////////

ExprParser::ExprParser(const std::string& expression) : expr_(expression) {
}

static std::map<parser_id, std::string> rule_names;
static void populate_rule_names() {
    if (rule_names.empty()) {
        rule_names[ExpressionGrammer::cal_date_to_julian_ID]   = "cal_date_to_julian";
        rule_names[ExpressionGrammer::cal_julian_to_date_ID]   = "cal_julian_to_date";
        rule_names[ExpressionGrammer::cal_argument_ID]         = "cal_argument";
        rule_names[ExpressionGrammer::modulo_ID]               = "MODULO";
        rule_names[ExpressionGrammer::equal_1_ID]              = "EQUALS";
        rule_names[ExpressionGrammer::equal_2_ID]              = "EQUALS";
        rule_names[ExpressionGrammer::not_equal_1_ID]          = "NOT_EQUAL";
        rule_names[ExpressionGrammer::not_equal_2_ID]          = "NOT_EQUAL";
        rule_names[ExpressionGrammer::greater_equals_1_ID]     = "GREATER_THAN_OR_EQUALS";
        rule_names[ExpressionGrammer::greater_equals_2_ID]     = "GREATER_THAN_OR_EQUALS";
        rule_names[ExpressionGrammer::less_equals_1_ID]        = "LESS_THAN_OR_EQUALS";
        rule_names[ExpressionGrammer::less_equals_2_ID]        = "LESS_THAN_OR_EQUALS";
        rule_names[ExpressionGrammer::less_than_1_ID]          = "LESS_THAN";
        rule_names[ExpressionGrammer::less_than_2_ID]          = "LESS_THAN";
        rule_names[ExpressionGrammer::greater_than_1_ID]       = "GREATER_THAN";
        rule_names[ExpressionGrammer::greater_than_2_ID]       = "GREATER_THAN";
        rule_names[ExpressionGrammer::not1_ID]                 = "NOT";
        rule_names[ExpressionGrammer::not2_ID]                 = "NOT";
        rule_names[ExpressionGrammer::not3_ID]                 = "NOT";
        rule_names[ExpressionGrammer::and_ID]                  = "AND";
        rule_names[ExpressionGrammer::or_ID]                   = "OR";
        rule_names[ExpressionGrammer::node_name_ID]            = "NODE_NAME";
        rule_names[ExpressionGrammer::node_state_unknown_ID]   = "UNKNOWN";
        rule_names[ExpressionGrammer::node_state_complete_ID]  = "COMPLETE";
        rule_names[ExpressionGrammer::node_state_queued_ID]    = "QUEUED";
        rule_names[ExpressionGrammer::node_state_submitted_ID] = "SUBMITTED";
        rule_names[ExpressionGrammer::node_state_active_ID]    = "ACTIVE";
        rule_names[ExpressionGrammer::node_state_aborted_ID]   = "ABORTED";
        rule_names[ExpressionGrammer::integer_ID]              = "INTEGER";
        rule_names[ExpressionGrammer::dot_path_ID]             = "DOT_PATH";
        rule_names[ExpressionGrammer::dot_dot_path_ID]         = "DOT_DOT_PATH";
        rule_names[ExpressionGrammer::absolute_path_ID]        = "ABSOLUTE_PATH";

        rule_names[ExpressionGrammer::base_trigger_ID]    = "BASE_TRIGGER";
        rule_names[ExpressionGrammer::sub_expression_ID]  = "SUB_EXPRESSION";
        rule_names[ExpressionGrammer::node_path_state_ID] = "NODE_PATH_STATE";
        rule_names[ExpressionGrammer::flag_path_ID]       = "FLAG_PATH";

        rule_names[ExpressionGrammer::event_state_ID] = "STRING";
        rule_names[ExpressionGrammer::variable_ID]    = "VARIABLE";

        rule_names[ExpressionGrammer::calc_expression_ID]     = "calc_expression_ID";
        rule_names[ExpressionGrammer::calc_factor_ID]         = "calc_factor_ID";
        rule_names[ExpressionGrammer::calc_term_ID]           = "calc_term_ID";
        rule_names[ExpressionGrammer::calc_grouping_ID]       = "calc_grouping_ID";
        rule_names[ExpressionGrammer::calc_subexpression_ID]  = "calc_subexpression_ID";
        rule_names[ExpressionGrammer::basic_variable_path_ID] = "basic_variable_path_ID";
        rule_names[ExpressionGrammer::parent_variable_ID]     = "parent_variable_ID";
        rule_names[ExpressionGrammer::compare_expression_ID]  = "compare_expression_ID";

        rule_names[ExpressionGrammer::datetime_ID] = "datetime";
    }
}

bool ExprParser::doParse(std::string& errorMsg) {
    if (expr_.empty()) {
        errorMsg = "Expression is empty";
        return false;
    }

    // =========================================================================
    // For large designs > 90% of triggers are identical.
    // We take advantage of this by only parsing the expression once
    // and storing then Abstract Syntax tree( via cloning ) using a map
    // This saves a huge amount of CPU time in re-parsing using spirit classic.
    // =========================================================================
    ast_ = ExprDuplicate::find(expr_);
    if (ast_.get()) {
        return true;
    }

    SimpleExprParser simpleParser(expr_);
    if (simpleParser.doParse()) {
        ast_ = simpleParser.ast();
        ExprDuplicate::add(expr_, ast_.get()); // bypass spirit if same expression used
        return true;
    }

    // SPIRIT CLASSIC parsing: very slooooow....
    ExpressionGrammer grammer;
    BOOST_SPIRIT_DEBUG_NODE(grammer);

    // Use parser to generates a Boost.Spirit Abstract Syntax Tree (AST)
    tree_parse_info<> info = ast_parse(expr_.c_str(), grammer, space_p);
    if (info.full) {

        populate_rule_names();

#if defined(BOOST_SPIRIT_DUMP_PARSETREE_AS_XML)
        tree_to_xml(cout, info.trees, expr_.c_str(), rule_names);
#endif
#if defined(PRINT_TREE)
        print(info, expr_, rule_names);
#endif
        // Boost.Spirit has created an AST for us.
        // However, it is not usable as is, so we will traverse the AST and create our OWN `Ast*` classes.
        ast_.reset(createTopAst(info, expr_, rule_names, errorMsg));
        if (ast_.get() && errorMsg.empty()) {
            ExprDuplicate::add(expr_, ast_.get());
        }
        return errorMsg.empty();
    }
    else {
        std::stringstream ss;
        ss << "Parsing failed\n";
        ss << "length = " << std::dec << info.length << "\n";
        ss << "stopped at: \": " << info.stop << "\"\n";
        errorMsg = ss.str();
    }
    return false;
}

// The evaluation function for the AST
void do_print(const tree_iter_t& i, const std::map<parser_id, std::string>& rule_names, ecf::Context& ctx) {
    ecf::Indent l1(ctx);
    auto iter = rule_names.find(i->value.id());
    if (iter != rule_names.end()) {
        std::cout << l1 << "Rule " << (*iter).second << "(size:" << i->children.size() << ")"
                  << "  " << string(i->value.begin(), i->value.end()) << endl;
    }
    else {
        std::cout << l1 << "Unknown rule(id:" << i->value.id().to_long() << ")"
                  << "(size:" << i->children.size() << ")"
                  << "  " << string(i->value.begin(), i->value.end()) << endl;
    }

    ecf::Indent l2(ctx);
    for (auto t = i->children.begin(); t != i->children.end(); ++t) {
        do_print(t, rule_names, ctx);
    }
}

void do_print(const tree_iter_t& i, const std::map<parser_id, std::string>& rule_names) {
    ecf::Context ctx = ecf::Context::make_for(PrintStyle::DEFS);
    do_print(i, rule_names, ctx);
}

void print(tree_parse_info<> info, const std::string& expr, const std::map<parser_id, std::string>& rule_names) {
    std::cout << "\nPRINT_TREE  " << expr << "\n";
    do_print(info.trees.begin(), rule_names);
}

////////////////////////////////////////////////////////////////////////////////////////////////

AstRoot* createRootNode(const tree_iter_t& i, const std::map<parser_id, std::string>& rule_names) {
#if defined(PRINT_AST_TRAVERSAL)
    Indentor in;
    std::map<parser_id, std::string>::const_iterator iter = rule_names.find(i->value.id());
    if (iter != rule_names.end()) {
        Indentor::indent(cout) << "Root Rule " << (*iter).second << "(" << i->children.size() << ")"
                               << "  " << string(i->value.begin(), i->value.end()) << endl;
    }
    else {
        Indentor::indent(cout) << "Unknown root rule "
                               << "(" << i->children.size() << ")"
                               << "  " << string(i->value.begin(), i->value.end()) << endl;
    }
#endif

    if (i->value.id() == ExpressionGrammer::equal_1_ID) {
        return new AstEqual();
    }
    if (i->value.id() == ExpressionGrammer::equal_2_ID) {
        return new AstEqual();
    }
    if (i->value.id() == ExpressionGrammer::and_ID) {
        return new AstAnd();
    }
    if (i->value.id() == ExpressionGrammer::or_ID) {
        return new AstOr();
    }
    // Needed so that testing , when recreating expression uses same name for not.
    if (i->value.id() == ExpressionGrammer::not1_ID) {
        auto* astnot = new AstNot();
        astnot->set_root_name("not ");
        return astnot;
    }
    if (i->value.id() == ExpressionGrammer::not2_ID) {
        auto* astnot = new AstNot();
        astnot->set_root_name("~ ");
        return astnot;
    }
    if (i->value.id() == ExpressionGrammer::not3_ID) {
        auto* astnot = new AstNot();
        astnot->set_root_name("! ");
        return astnot;
    }
    if (i->value.id() == ExpressionGrammer::plus_ID) {
        return new AstPlus();
    }

    if (i->value.id() == ExpressionGrammer::not_equal_1_ID) {
        return new AstNotEqual();
    }
    if (i->value.id() == ExpressionGrammer::not_equal_2_ID) {
        return new AstNotEqual();
    }
    if (i->value.id() == ExpressionGrammer::greater_equals_1_ID) {
        return new AstGreaterEqual();
    }
    if (i->value.id() == ExpressionGrammer::greater_equals_2_ID) {
        return new AstGreaterEqual();
    }
    if (i->value.id() == ExpressionGrammer::less_equals_1_ID) {
        return new AstLessEqual();
    }
    if (i->value.id() == ExpressionGrammer::less_equals_2_ID) {
        return new AstLessEqual();
    }
    if (i->value.id() == ExpressionGrammer::less_than_1_ID) {
        return new AstLessThan();
    }
    if (i->value.id() == ExpressionGrammer::less_than_2_ID) {
        return new AstLessThan();
    }
    if (i->value.id() == ExpressionGrammer::greater_than_1_ID) {
        return new AstGreaterThan();
    }
    if (i->value.id() == ExpressionGrammer::greater_than_2_ID) {
        return new AstGreaterThan();
    }

    if (i->value.id() == ExpressionGrammer::minus_ID) {
        return new AstMinus();
    }
    if (i->value.id() == ExpressionGrammer::multiply_ID) {
        return new AstMultiply();
    }
    if (i->value.id() == ExpressionGrammer::divide_ID) {
        return new AstDivide();
    }
    if (i->value.id() == ExpressionGrammer::modulo_ID) {
        return new AstModulo();
    }
    LOG_ASSERT(false, "");
    return nullptr;
}

// static bool is_node_state(const tree_iter_t& i) {
//    if ( i->value.id() == ExpressionGrammer::node_state_unknown_ID  ) return true;
//    if ( i->value.id() == ExpressionGrammer::node_state_complete_ID  ) return true;
//    if ( i->value.id() == ExpressionGrammer::node_state_queued_ID  ) return true;
//    if ( i->value.id() == ExpressionGrammer::node_state_submitted_ID  ) return true;
//    if ( i->value.id() == ExpressionGrammer::node_state_active_ID  ) return true;
//    if ( i->value.id() == ExpressionGrammer::node_state_aborted_ID  ) return true;
//    return false;
// }
// static bool has_child_node_state(const tree_iter_t& i) {
//    for (tree_iter_t t = i->children.begin(); t != i->children.end(); ++t)  {
//       if (is_node_state(t)) return true;
//    }
//    return false;
// }
// static bool has_child_event_state(const tree_iter_t& i) {
//    for (tree_iter_t t = i->children.begin(); t != i->children.end(); ++t)  {
//       if ( t->value.id() == ExpressionGrammer::event_state_ID) return true;
//    }
//    return false;
// }
// static bool child_has_path(const tree_iter_t& i) {
//    for (tree_iter_t t = i->children.begin(); t != i->children.end(); ++t)  {
//       if ( t->value.id() == ExpressionGrammer::dot_path_ID ) return true;
//       if ( t->value.id() == ExpressionGrammer::dot_dot_path_ID ) return true;
//       if ( t->value.id() == ExpressionGrammer::absolute_path_ID ) return true;
//    }
//    return false;
// }
// static bool is_comparable(const tree_iter_t& i) {
//    if ( i->value.id() == ExpressionGrammer::equal_1_ID ) return true;
//    if ( i->value.id() == ExpressionGrammer::equal_2_ID ) return true;
//    if ( i->value.id() == ExpressionGrammer::not_equal_1_ID ) return true;
//    if ( i->value.id() == ExpressionGrammer::not_equal_2_ID ) return true;
//    return false;
// }
// static bool has_child_not(const tree_iter_t& i) {
//    for (tree_iter_t t = i->children.begin(); t != i->children.end(); ++t)  {
//       if (is_not(t)) return true;
//    }
//    return false;
// }

static bool is_not(const tree_iter_t& i) {
    if (i->value.id() == ExpressionGrammer::not1_ID) {
        return true;
    }
    if (i->value.id() == ExpressionGrammer::not2_ID) {
        return true;
    }
    if (i->value.id() == ExpressionGrammer::not3_ID) {
        return true;
    }
    return false;
}
static bool child_has(const tree_iter_t& i, int id) {
    for (auto& t : i->children) {
        if (t.value.id() == id) {
            return true;
        }
    }
    return false;
}
static bool is_root_node(const tree_iter_t& i) {
    if (i->value.id() == ExpressionGrammer::equal_1_ID) {
        return true;
    }
    if (i->value.id() == ExpressionGrammer::equal_2_ID) {
        return true;
    }
    if (i->value.id() == ExpressionGrammer::and_ID) {
        return true;
    }
    if (i->value.id() == ExpressionGrammer::or_ID) {
        return true;
    }
    if (i->value.id() == ExpressionGrammer::not1_ID) {
        return true;
    }
    if (i->value.id() == ExpressionGrammer::not2_ID) {
        return true;
    }
    if (i->value.id() == ExpressionGrammer::not3_ID) {
        return true;
    }
    if (i->value.id() == ExpressionGrammer::plus_ID) {
        return true;
    }

    if (i->value.id() == ExpressionGrammer::not_equal_1_ID) {
        return true;
    }
    if (i->value.id() == ExpressionGrammer::not_equal_2_ID) {
        return true;
    }
    if (i->value.id() == ExpressionGrammer::greater_equals_1_ID) {
        return true;
    }
    if (i->value.id() == ExpressionGrammer::greater_equals_2_ID) {
        return true;
    }
    if (i->value.id() == ExpressionGrammer::less_equals_1_ID) {
        return true;
    }
    if (i->value.id() == ExpressionGrammer::less_equals_2_ID) {
        return true;
    }
    if (i->value.id() == ExpressionGrammer::less_than_1_ID) {
        return true;
    }
    if (i->value.id() == ExpressionGrammer::less_than_2_ID) {
        return true;
    }
    if (i->value.id() == ExpressionGrammer::greater_than_1_ID) {
        return true;
    }
    if (i->value.id() == ExpressionGrammer::greater_than_2_ID) {
        return true;
    }

    if (i->value.id() == ExpressionGrammer::minus_ID) {
        return true;
    }
    if (i->value.id() == ExpressionGrammer::multiply_ID) {
        return true;
    }
    if (i->value.id() == ExpressionGrammer::divide_ID) {
        return true;
    }
    if (i->value.id() == ExpressionGrammer::modulo_ID) {
        return true;
    }
    return false;
}

Ast* createAst(const tree_iter_t& i, const std::map<parser_id, std::string>& rule_names) {
#if defined(PRINT_AST_TRAVERSAL)
    Indentor in;
    std::map<parser_id, std::string>::const_iterator iter = rule_names.find(i->value.id());
    if (iter != rule_names.end()) {
        Indentor::indent(cout) << "Create AST Rule " << (*iter).second << "(" << i->children.size() << ")"
                               << "  '" << string(i->value.begin(), i->value.end()) << "'\n";
    }
    else {
        Indentor::indent(cout) << "Create AST Unknown rule "
                               << "(" << i->children.size() << ")"
                               << "  '" << string(i->value.begin(), i->value.end()) << "'\n";
    }
#endif

    if (i->value.id() == ExpressionGrammer::node_name_ID) {

        string thevalue(i->value.begin(), i->value.end());
        ecf::algorithm::trim(thevalue); // don't know why we get leading/trailing spaces
        LOG_ASSERT(!thevalue.empty(), "");
        return new AstNode(thevalue);
    }
    else if (i->value.id() == ExpressionGrammer::node_state_complete_ID) {

        return new AstNodeState(DState::COMPLETE);
    }
    else if (i->value.id() == ExpressionGrammer::basic_variable_path_ID) {
        LOG_ASSERT((i->children.size() == 2), "");
        auto theNodePathIter = i->children.begin();
        auto theNameIter     = i->children.begin() + 1;

        string nodePath(theNodePathIter->value.begin(), theNodePathIter->value.end());
        string name(theNameIter->value.begin(), theNameIter->value.end());
        ecf::algorithm::trim(nodePath); // don't know why we get leading/trailing spaces
        ecf::algorithm::trim(name);     // don't know why we get leading/trailing spaces
        return new AstVariable(nodePath, name);
    }
    else if (i->value.id() == ExpressionGrammer::parent_variable_ID) {

        // tree_iter_t the_colon = i->children.begin(); ignore
        auto the_variable_t = i->children.begin() + 1;

        string the_variable(the_variable_t->value.begin(), the_variable_t->value.end());
        ecf::algorithm::trim(the_variable); // don't know why we get leading/trailing spaces
        LOG_ASSERT(!the_variable.empty(), "");
        return new AstParentVariable(the_variable);
    }
    else if (i->value.id() == ExpressionGrammer::dot_dot_path_ID) {

        string thevalue(i->value.begin(), i->value.end());
        ecf::algorithm::trim(thevalue); // don't know why we get leading/trailing spaces
        LOG_ASSERT(!thevalue.empty(), "");
        return new AstNode(thevalue);
    }
    if (i->value.id() == ExpressionGrammer::absolute_path_ID) {

        string thevalue(i->value.begin(), i->value.end());
        ecf::algorithm::trim(thevalue); // don't know why we get leading/trailing spaces
        LOG_ASSERT(!thevalue.empty(), "");
        return new AstNode(thevalue);
    }
    else if (i->value.id() == ExpressionGrammer::dot_path_ID) {

        string thevalue(i->value.begin(), i->value.end());
        ecf::algorithm::trim(thevalue); // don't know why we get leading/trailing spaces
        LOG_ASSERT(!thevalue.empty(), "");
        return new AstNode(thevalue);
    }
    else if (i->value.id() == ExpressionGrammer::event_state_ID) {

        string thevalue(i->value.begin(), i->value.end());
        ecf::algorithm::trim(thevalue); // don't know why we get leading/trailing spaces
        if (thevalue == Event::SET()) {
            return new AstEventState(true);
        }
        assert(thevalue == Event::CLEAR());
        return new AstEventState(false);
    }
    else if (i->value.id() == ExpressionGrammer::datetime_ID) {

        string thevalue(std::begin(i->value), std::end(i->value));
        ecf::Instant instant = Instant::parse(thevalue);
        return new AstInstant(instant);
    }
    else if (i->value.id() == ExpressionGrammer::integer_ID) {

        string thevalue(i->value.begin(), i->value.end());
        ecf::algorithm::trim(thevalue); // don't know why we get leading/trailing spaces
        auto theInt = ecf::convert_to<int>(thevalue);
        return new AstInteger(theInt);
    }
    else if (i->value.id() == ExpressionGrammer::node_state_aborted_ID) {

        return new AstNodeState(DState::ABORTED);
    }
    else if (i->value.id() == ExpressionGrammer::node_state_active_ID) {

        return new AstNodeState(DState::ACTIVE);
    }
    else if (i->value.id() == ExpressionGrammer::node_state_queued_ID) {

        return new AstNodeState(DState::QUEUED);
    }
    else if (i->value.id() == ExpressionGrammer::node_state_submitted_ID) {

        return new AstNodeState(DState::SUBMITTED);
    }
    else if (i->value.id() == ExpressionGrammer::node_state_unknown_ID) {

        return new AstNodeState(DState::UNKNOWN);
    }
    else if (i->value.id() == ExpressionGrammer::cal_date_to_julian_ID) {

        LOG_ASSERT((i->children.size() == 2), ""); // 1st is function, 2nd is arg(integer | basic variable path)

        return new AstFunction(AstFunction::DATE_TO_JULIAN, createAst(i->children.begin() + 1, rule_names));
    }
    else if (i->value.id() == ExpressionGrammer::cal_julian_to_date_ID) {

        LOG_ASSERT((i->children.size() == 2), ""); // 1st is function, 2nd is arg(integer | basic variable path)

        return new AstFunction(AstFunction::JULIAN_TO_DATE, createAst(i->children.begin() + 1, rule_names));
    }
    else if (i->value.id() == ExpressionGrammer::flag_path_ID) {

        LOG_ASSERT((i->children.size() == 2), "");
        auto theNodePathIter = i->children.begin();
        auto theFlagIter     = i->children.begin() + 1;

        string nodePath(theNodePathIter->value.begin(), theNodePathIter->value.end());
        string flag(theFlagIter->value.begin(), theFlagIter->value.end());
        ecf::algorithm::trim(nodePath); // don't know why we get leading/trailing spaces
        ecf::algorithm::trim(flag);     // don't know why we get leading/trailing spaces

        return new AstFlag(nodePath, ecf::Flag::string_to_flag_type(flag));
    }
    return nullptr;
}

// The evaluation function for the AST
Ast* doCreateAst(const tree_iter_t& i, const std::map<parser_id, std::string>& rule_names, Ast* top) {

    if (i->children.size() == 3) {
        // child 1: left                0
        // child 2: root(i.e ==,!=)    +1
        // child 3: right              +2

        AstRoot* someRoot = createRootNode(i->children.begin() + 1, rule_names);
        if (someRoot) {
            Ast* left = doCreateAst(i->children.begin(), rule_names, someRoot);
            if (left) {
                someRoot->addChild(left);
            }
            Ast* right = doCreateAst(i->children.begin() + 2, rule_names, someRoot);
            if (right) {
                someRoot->addChild(right);
            }

            if (top) {
                top->addChild(someRoot);
            }
            else {
                return someRoot;
            }
        }
    }
    else if (is_root_node(i) && i->children.size() == 2) {

        AstRoot* someRoot = createRootNode(i, rule_names);

        Ast* left = doCreateAst(i->children.begin(), rule_names, someRoot);
        if (left) {
            someRoot->addChild(left);
        }
        Ast* right = doCreateAst(i->children.begin() + 1, rule_names, someRoot);
        if (right) {
            someRoot->addChild(right);
        }

        if (top) {
            top->addChild(someRoot);
        }
        else {
            return someRoot;
        }
    }
    else if (i->children.size() == 4 && is_not(i->children.begin())) {
        // child 0: notRoot                 0
        // child 1: notChild               +1
        // child 2: someRoot(i.e ==,!=)    +2
        // child 3: right                  +3
        // Create as:         someRoot
        //              notRoot          right
        //      notChild

        LOG_ASSERT(is_not(i->children.begin()), "");
        AstRoot* notRoot = createRootNode(i->children.begin(), rule_names);

        Ast* notChild = doCreateAst(i->children.begin() + 1, rule_names, notRoot /*top*/);
        if (notChild) {
            notRoot->addChild(notChild);
        }

        AstRoot* someRoot = createRootNode(i->children.begin() + 2, rule_names);
        someRoot->addChild(notRoot); // left

        Ast* right = doCreateAst(i->children.begin() + 3, rule_names, someRoot /* top*/);
        if (right) {
            someRoot->addChild(right);
        }
        if (top) {
            top->addChild(someRoot);
        }
        else {
            return someRoot;
        }
    }
    else if (i->children.size() == 4 && is_root_node(i->children.begin() + 1) && is_not(i->children.begin() + 2)) {
        // child 0: child                   0
        // child 1: someRoot(i.e ==,!=)    +1
        // child 2: NOT                    +2
        // child 3: integer | variable     +3
        //
        // Create as:         someRoot
        //              child          notRoot
        //                                    notChild

        AstRoot* someRoot = createRootNode(i->children.begin() + 1, rule_names);
        Ast* varPath      = doCreateAst(i->children.begin(), rule_names, someRoot /*top*/);
        if (varPath) {
            someRoot->addChild(varPath); // left
        }

        AstRoot* notRoot = createRootNode(i->children.begin() + 2, rule_names);
        someRoot->addChild(notRoot); // right

        Ast* notChild = doCreateAst(i->children.begin() + 3, rule_names, notRoot /*top*/);
        if (notChild) {
            notRoot->addChild(notChild);
        }

        if (top) {
            top->addChild(someRoot);
        }
        else {
            return someRoot;
        }
    }
    else if (i->children.size() == 2 && is_not(i->children.begin())) {
        // child 1: not     0
        // child 2: left   +1

        AstRoot* notRoot = createRootNode(i->children.begin(), rule_names);

        if (child_has(i, ExpressionGrammer::basic_variable_path_ID)) {
            // special case where we treat as a event i.e ! ../../../prod2diss/operation_is_late:yes
            notRoot->addChild(createAst(i->children.begin() + 1, rule_names));
        }
        else {
            Ast* left = doCreateAst(i->children.begin() + 1, rule_names, notRoot);
            if (left) {
                notRoot->addChild(left);
            }
        }

        if (top) {
            top->addChild(notRoot);
        }
        else {
            return notRoot;
        }
    }
    else if (i->children.size() >= 5) {
        // Must be multiple and's could have nots  Could have:
        //     !a and b
        //      a and !b
        // We always treat the not as *child*

        stack<Ast*> childs;
        stack<Ast*> parents;
        Ast* not_ast = nullptr;
        for (auto t = i->children.begin(); t != i->children.end(); ++t) {
            if (is_root_node(t) && !is_not(t)) {
                Ast* and_ast = createRootNode(t, rule_names);
                assert(and_ast);
                assert(parents.empty());
                parents.push(and_ast);
            }
            else {
                if (is_not(t)) {
                    assert(!not_ast);
                    not_ast = createRootNode(t, rule_names);
                    assert(not_ast);
                    childs.push(not_ast);
                }
                else {
                    Ast* child_ast = doCreateAst(t, rule_names, /*Top*/ nullptr);
                    assert(child_ast);
                    if (not_ast) {
                        not_ast->addChild(child_ast);
                        not_ast = nullptr;
                    }
                    else {
                        childs.push(child_ast);
                    }
                }
            }

            if (parents.size() == 1 && childs.size() == 2) {
                Ast* parent = parents.top();
                parents.pop();
                Ast* child1 = childs.top();
                childs.pop();
                Ast* child2 = childs.top();
                childs.pop();
                parent->addChild(child2);
                parent->addChild(child1);
                assert(parents.empty() && childs.empty());
                childs.push(parent);
            }
        }
        assert(top);
        assert(childs.size() == 1);
        if (top) {
            top->addChild(childs.top());
        }
    }
    else {
        Ast* child = createAst(i, rule_names);
        if (top && child) {
            top->addChild(child);
        }
        else {
            return child;
        }
    }
    return nullptr;
}

AstTop* createTopAst(tree_parse_info<> info,
                     const std::string& expr,
                     const std::map<parser_id, std::string>& rule_names,
                     std::string& error_msg) {
#if defined(PRINT_AST_TRAVERSAL)
    std::cout << "\nPRINT_AST_TRAVERSAL  " << expr << "\n";
#endif

    std::unique_ptr<AstTop> ast = std::make_unique<AstTop>();
    try {
        (void)doCreateAst(info.trees.begin(), rule_names, ast.get());
    }
    catch (std::runtime_error& error) {
        error_msg = error.what();
        return nullptr;
    }

    if (!ast->is_valid_ast(error_msg)) {
        return nullptr;
    }

#if defined(PRINT_AST)
    if (ast.get()) {
        std::stringstream s2;
        ast->print_flat(s2, true /*add_brackets*/);
        std::cout << "\nPRINT_AST  " << s2.str() << "\n";
        std::cout << "PRINT_AST  " << expr << "\n";
        std::cout << *ast.get();
    }
#endif
    return ast.release();
}

///////////////////////////////////////////////////////////////////////////
// SimpleExprParser
//////////////////////////////////////////////////////////////////////////

bool has_complex_expressions(const std::string& expr) {
    // we allow . and /
    if (expr.find('(') != string::npos) {
        return true;
    }
    if (expr.find(':') != string::npos) {
        return true;
    }
    if (expr.find('.') != string::npos) {
        return true;
    }
    if (expr.find('/') != string::npos) {
        return true;
    }
    if (expr.find(" not ") != string::npos) {
        return true;
    }
    if (expr.find(" and ") != string::npos) {
        return true;
    }
    if (expr.find(" or ") != string::npos) {
        return true;
    }
    if (expr.find('!') != string::npos) {
        return true;
    }
    if (expr.find("&&") != string::npos) {
        return true;
    }
    if (expr.find("||") != string::npos) {
        return true;
    }
    if (expr.find('<') != string::npos) {
        return true;
    }
    if (expr.find('>') != string::npos) {
        return true;
    }
    if (expr.find('+') != string::npos) {
        return true;
    }
    if (expr.find('-') != string::npos) {
        return true;
    }
    if (expr.find('*') != string::npos) {
        return true;
    }
    if (expr.find('~') != string::npos) {
        return true;
    }
    if (expr.find(" ne ") != string::npos) {
        return true;
    }
    if (expr.find(" ge ") != string::npos) {
        return true;
    }
    if (expr.find("<=") != string::npos) {
        return true;
    }
    if (expr.find(">=") != string::npos) {
        return true;
    }
    if (expr.find(" le ") != string::npos) {
        return true;
    }
    if (expr.find(" gt ") != string::npos) {
        return true;
    }
    if (expr.find(" lt ") != string::npos) {
        return true;
    }
    return false;
}

bool SimpleExprParser::doParse() {
    // 3199.def 57Mg
    // a == b, || a eq b i.e simple  76940
    // complex                       39240
    if (has_complex_expressions(expr_)) {
        return false;
    }

    // look for path == <state> || path eq state
    // This gets round issue with Str::split:
    //   "a = complete" will be split
    // since will split on *ANY* of character, and hence this would not be reported as an error
    std::vector<std::string> tokens;
    if (expr_.find("==") != string::npos) {
        Str::split(expr_, tokens, "==");
    }
    else if (expr_.find(" eq ") != string::npos) {
        Str::split(expr_, tokens, " eq ");
    }
    else {
        return false;
    }

    if (tokens.size() == 2) {

        ecf::algorithm::trim(tokens[0]);
        ecf::algorithm::trim(tokens[1]);

        if (tokens[0].find(' ') != string::npos) {
            //         cout << "Found space " << expr_ << "\n";
            return false;
        }

        if (DState::isValid(tokens[1])) {

            ast_          = std::make_unique<AstTop>();
            Ast* someRoot = new AstEqual();
            someRoot->addChild(new AstNode(tokens[0]));
            someRoot->addChild(new AstNodeState(DState::toState(tokens[1])));
            ast_->addChild(someRoot);
            //       cout << "simple expr : " << expr_ << " `" << tokens[0] << "' = '" << tokens[1] << "'\n";
            return true;
        }
        else {
            try {
                auto left     = ecf::convert_to<int>(tokens[0]);
                auto right    = ecf::convert_to<int>(tokens[1]);
                ast_          = std::make_unique<AstTop>();
                Ast* someRoot = new AstEqual();
                someRoot->addChild(new AstInteger(left));
                someRoot->addChild(new AstInteger(right));
                ast_->addChild(someRoot);
                //               cout << "simple INT expr : " << expr_ << " `" << tokens[0] << "' = '" << tokens[1] <<
                //               "'\n";
                return true;
            }
            catch (const ecf::bad_conversion&) {
                //            cout << "simple INT FAILED expr : " << expr_ << " `" << tokens[0] << "' = '"
                //                     << tokens[1] << "'\n";
            }
        }
    }
    return false;
}
