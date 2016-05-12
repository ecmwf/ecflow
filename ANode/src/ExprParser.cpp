//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #30 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================

// Un-comment these for selective for debugging. At the moment because we have added
//            ast generation BOOST_SPIRIT_DEBUG is to verbose, making debugging a pain.
// Tpyically I will comment out ONLY BOOST_SPIRIT_DEBUG and enable all the others
//#define BOOST_SPIRIT_DEBUG
//#define BOOST_SPIRIT_DUMP_PARSETREE_AS_XML
//#define PRINT_TREE
//#define PRINT_AST_TRAVERSAL
//#define PRINT_AST

#include <boost/spirit/include/classic.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_actor.hpp>
#include <boost/spirit/include/classic_attribute.hpp>
#include <boost/spirit/include/classic_confix.hpp>
#include <boost/spirit/include/phoenix1_binders.hpp>
#include <boost/spirit/include/classic_ast.hpp>
#include <boost/spirit/include/classic_tree_to_xml.hpp>

#include "boost/lambda/lambda.hpp"
#include "boost/lambda/bind.hpp"
#include "boost/cast.hpp"
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <iostream>
#include <string>
#include <utility>
#include <stack>
#include <sstream>
#include <map>

#include "ExprParser.hpp"
#include "ExprAst.hpp"
#include "ExprDuplicate.hpp"
#include "Indentor.hpp"
#include "Log.hpp"
#include "Str.hpp"

// Reference
//‘*’   Zero or more
//‘!’   Zero or one
//‘+’   One or more
//‘>>’  Sequence/concatenation
//‘|’   alternate
//‘-‘   not

////////////////////////////////////////////////////////////////////////////
using namespace ecf;
using namespace std;
using namespace boost::spirit;
using namespace phoenix;
using namespace BOOST_SPIRIT_CLASSIC_NS;

/////////////////////////////////////////////////////////////////////////////
typedef tree_match<const char*>    treematch_t;
typedef treematch_t::tree_iterator tree_iter_t;

//////////////////////////////////////////////////////////////////////////////

struct ExpressionGrammer : public grammar<ExpressionGrammer>
{
	// The parser object is copied a lot, so only use reference/pointer variables as data members
	ExpressionGrammer(){}
	~ExpressionGrammer(){}

	static const int integer_ID  = 1;
	static const int dot_path_ID = 2;
	static const int equal_1_ID  = 3;
	static const int equal_2_ID  = 4;
	static const int not_equal_1_ID = 5;
	static const int not_equal_2_ID = 6;
	static const int node_name_ID   = 7;
	static const int greater_equals_1_ID = 8;
	static const int greater_equals_2_ID = 9;
	static const int less_equals_1_ID   = 10;
	static const int less_equals_2_ID   = 11;
	static const int less_than_1_ID     = 12;
	static const int less_than_2_ID     = 13;
	static const int greater_than_1_ID  = 14;
	static const int greater_than_2_ID  = 15;
	static const int node_state_unknown_ID    = 16;

 	static const int node_state_complete_ID   = 18;
	static const int node_state_queued_ID     = 19;
	static const int node_state_submitted_ID  = 20;
	static const int node_state_active_ID     = 21;
	static const int node_state_aborted_ID    = 22;
	static const int not1_ID           = 23;
   static const int not2_ID          = 24;
   static const int not3_ID          = 25;
	static const int and_ID           = 26;
	static const int or_ID            = 27;
	static const int event_ID         = 28;
 	static const int dot_dot_path_ID     = 29;
	static const int event_name_ID       = 30;
 	static const int base_trigger_ID     = 31;
	static const int sub_expression_ID   = 32;
	static const int grouping_ID         = 33;
	static const int node_path_state_ID  = 34;
	static const int absolute_path_ID    = 35;
	static const int some_string_ID      = 36;
	static const int variable_ID         = 37;
	static const int variable_path_ID    = 38;
	static const int normal_variable_path_ID   = 39;
	static const int grouped_variable_path_ID  = 40;
	static const int variable_expression_ID    = 41;
	static const int plus_ID      = 42;
	static const int minus_ID     = 43;
	static const int multiply_ID  = 44;
   static const int divide_ID    = 45;
   static const int modulo_ID    = 46;

    template <typename ScannerT>
    struct definition {
        rule<ScannerT,parser_tag<integer_ID> > integer;
        rule<ScannerT,parser_tag<plus_ID> > plus;
        rule<ScannerT,parser_tag<minus_ID> > minus;
        rule<ScannerT,parser_tag<multiply_ID> > multiply;
        rule<ScannerT,parser_tag<divide_ID> > divide;
        rule<ScannerT,parser_tag<modulo_ID> > modulo;

        rule<ScannerT,parser_tag<equal_1_ID> > equal_1;
        rule<ScannerT,parser_tag<equal_2_ID> > equal_2;
        rule<ScannerT,parser_tag<not_equal_1_ID> > not_equal_1;
        rule<ScannerT,parser_tag<not_equal_2_ID> > not_equal_2;
        rule<ScannerT,parser_tag<node_name_ID> > nodename;

        rule<ScannerT,parser_tag<greater_equals_1_ID> > greater_equals_1;
        rule<ScannerT,parser_tag<greater_equals_2_ID> > greater_equals_2;
        rule<ScannerT,parser_tag<less_equals_1_ID> > less_equals_1;
        rule<ScannerT,parser_tag<less_equals_2_ID> > less_equals_2;

        rule<ScannerT,parser_tag<less_than_1_ID> > less_than_1;
        rule<ScannerT,parser_tag<less_than_2_ID> > less_than_2;
        rule<ScannerT,parser_tag<greater_than_1_ID> > greater_than_1;
        rule<ScannerT,parser_tag<greater_than_2_ID> > greater_than_2;

        rule<ScannerT,parser_tag<node_state_unknown_ID> >    node_state_unknown;
        rule<ScannerT,parser_tag<node_state_complete_ID> >   node_state_complete;
        rule<ScannerT,parser_tag<node_state_queued_ID> >     node_state_queued;
        rule<ScannerT,parser_tag<node_state_submitted_ID> >  node_state_submitted;
        rule<ScannerT,parser_tag<node_state_active_ID> >     node_state_active;
        rule<ScannerT,parser_tag<node_state_aborted_ID> >    node_state_aborted;

        rule<ScannerT,parser_tag<not1_ID> >   not1_r;
        rule<ScannerT,parser_tag<not2_ID> >   not2_r;
        rule<ScannerT,parser_tag<not3_ID> >   not3_r;
        rule<ScannerT,parser_tag<and_ID> >   and_r;
        rule<ScannerT,parser_tag<or_ID> >    or_r;
        rule<ScannerT,parser_tag<event_ID> > event;

        rule<ScannerT,parser_tag<dot_path_ID> >      dotpath;
        rule<ScannerT,parser_tag<dot_dot_path_ID> >  dotdotpath;
        rule<ScannerT,parser_tag<absolute_path_ID> > absolutepath;
        rule<ScannerT,parser_tag<event_name_ID> >    eventname;

        rule<ScannerT,parser_tag<base_trigger_ID> >     baseTrigger;
        rule<ScannerT,parser_tag<sub_expression_ID> >   subexpression;
        rule<ScannerT,parser_tag<grouping_ID> >         grouping;
        rule<ScannerT,parser_tag<node_path_state_ID> >  nodepathstate;

        rule<ScannerT,parser_tag<some_string_ID> >   some_string;
        rule<ScannerT,parser_tag<variable_ID> >      variable;
        rule<ScannerT,parser_tag<variable_path_ID> > variable_path;
        rule<ScannerT,parser_tag<normal_variable_path_ID> >  normal_variable_path;
        rule<ScannerT,parser_tag<grouped_variable_path_ID> > grouped_variable_path;
        rule<ScannerT,parser_tag<variable_expression_ID> >   variable_expression;

        rule<ScannerT> not_r,less_than_comparable,expression,andExpr,orExpr,operators;
        rule<ScannerT> nodestate, equality_comparible, and_or, nodepath ;
        rule<ScannerT> andsubexpression, orsubexpression,integerComparison,notGrouping ;

        //        ‘*’   Zero or more
        //        ‘!’   Zero or one
        //        ‘+’   One or more
        //        ‘>>’  Sequence/concatenation
        //        ‘|’   alternate
        //        ‘-‘   not
        definition(ExpressionGrammer const& /*self*/)
        {
        	 nodename
                = leaf_node_d[
								  lexeme_d [ (alnum_p || ch_p('_')) >> *(alnum_p || ch_p('_'))  ]
                             ]
                ;

        	 // Can be /suite/family/task
        	 //        family/task
        	 //        family
        	 absolutepath
				 = 	leaf_node_d[
								!(str_p("/")) >> nodename
        									  >> *(
        											+ str_p("/")  >> nodename
        										  )
        				   	  ]
        		;
        	 dotdotpath  // a kind of relative path
				 = 	leaf_node_d[
									str_p("..")
									>> *(
											+ str_p("/") >> str_p("..")
										)
									>> +(
											+ str_p("/")  >> nodename
										)
				   	           ]
				  ;
        	 dotpath = leaf_node_d[ str_p(".") >> +( str_p("/") >> nodename) ];
        	 nodepath =  absolutepath | dotdotpath | dotpath  ;

        	 // Integer is distinct from task/family names that are integers, since nodes with integer
        	 // names that occur in trigger/complete expression must have path ./0 ./1
        	 integer  =   leaf_node_d[  uint_p  ];
        	 plus = root_node_d [ str_p("+") ];
        	 minus = root_node_d [ str_p("-") ];
        	 divide = root_node_d [ str_p("/") ];
          multiply = root_node_d [ str_p("*") ];
          modulo = root_node_d [ str_p("%") ];
        	 operators = plus | minus | divide | multiply | modulo ;

        	 equal_1 = root_node_d [ str_p("==") ];
        	 equal_2 = root_node_d [ str_p("eq") ];
        	 not_equal_1 = root_node_d [ str_p("!=") ];
        	 not_equal_2 = root_node_d [ str_p("ne") ];
        	 equality_comparible  = equal_1 | equal_2 | not_equal_2  | not_equal_1;

        	 greater_equals_1 = root_node_d [ str_p(">=") ];
        	 greater_equals_2 = root_node_d [ str_p("ge") ];
        	 less_equals_1 = root_node_d [ str_p("<=") ];
        	 less_equals_2 = root_node_d [ str_p("le") ];
        	 less_than_1 = root_node_d [ str_p("<") ];
        	 less_than_2 = root_node_d [ str_p("lt") ];
          greater_than_1 = root_node_d [ str_p(">") ];
          greater_than_2 = root_node_d [ str_p("gt") ];
          // Prioritise to most common first, to speed up parsing
        	 less_than_comparable
				 =  greater_equals_2
				    | less_equals_2
				    | greater_than_2
				    | less_than_2
				    | greater_equals_1
				    | less_equals_1
				    | less_than_1
				    | greater_than_1
				  ;

        	 not1_r = root_node_d [ str_p("not ") ];
          not2_r = root_node_d [ str_p("~") ];
          not3_r = root_node_d [ str_p("!") ];
        	 not_r = not1_r | not3_r | not2_r;

        	 and_r = root_node_d [ str_p("and") ] || root_node_d [ str_p("&&") ] ;
        	 or_r = root_node_d [ str_p("or") ]   || root_node_d [ str_p("||") ] ;
        	 and_or =  and_r |  or_r;


          eventname = leaf_node_d [ nodename ];
        	 event
				 =  nodepath
				    >>  discard_node_d[ ch_p(':') ]
				    >>  eventname
 				  ;

        	 some_string = leaf_node_d[ str_p("set") ] || leaf_node_d[ str_p("clear")] ;

        	 variable = leaf_node_d [ nodename ];
        	 normal_variable_path
				 = nodepath
				   >> discard_node_d[ ch_p(':') ]
				   >> variable
			      >> !(operators >> (integer | normal_variable_path))
			     ;
        	 grouped_variable_path
				 =  discard_node_d[ ch_p('(') ]
				    >> normal_variable_path
				    >> discard_node_d[ ch_p(')') ]
				 ;
        	 variable_path = grouped_variable_path | normal_variable_path ;

        	 variable_expression
				  =   variable_path
 					  >> ( less_than_comparable | equality_comparible  )
					  >> !not_r
					  >> ( variable_path | integer | some_string  )
				  ;


        	 node_state_unknown = root_node_d [ str_p("unknown") ];
          node_state_complete = root_node_d [ str_p("complete") ];
        	 node_state_queued = root_node_d [ str_p("queued") ];
        	 node_state_submitted = root_node_d [ str_p("submitted") ];
        	 node_state_active = root_node_d [ str_p("active") ];
        	 node_state_aborted = root_node_d [ str_p("aborted") ];
        	 nodestate
				  =  node_state_complete
				    | node_state_aborted
				    | node_state_active
				    | node_state_queued
				    | node_state_submitted
 				    | node_state_unknown
				    ;
        	  nodepathstate = nodepath >> equality_comparible >> nodestate;

        	  integerComparison = integer >> ( less_than_comparable | equality_comparible) >> ( integer | variable_path );

         	  baseTrigger
				  =  !not_r
				     >> (
				    		nodepathstate
				    		| variable_expression
				    		| event                 // event if of the form  'a:name'
				    		| integerComparison     // 1 eq 1
				    	)
				  ;
        	  andExpr = baseTrigger >> and_r >> baseTrigger;
        	  orExpr = baseTrigger >> or_r >> baseTrigger;

        	  // We need to take special care so that 'and' has a higher priority then 'or'
        	  andsubexpression = andExpr >> *(and_or >> subexpression);

        	  orsubexpression =  baseTrigger >> +(or_r >> subexpression);

        	  subexpression =  (andsubexpression |  orsubexpression | baseTrigger | notGrouping | integer)
        	                   >> *(and_or >> subexpression );

        	  grouping
				  =  discard_node_d[ ch_p('(') ]
				     >> subexpression
				     >> discard_node_d[ ch_p(')') ]
			      ;

        	  notGrouping =  !not_r >> grouping >> *(and_or >> subexpression);
        	  expression = ( notGrouping  | subexpression  ) >> end_p;

              BOOST_SPIRIT_DEBUG_NODE(notGrouping);
              BOOST_SPIRIT_DEBUG_NODE(expression);
              BOOST_SPIRIT_DEBUG_NODE(andExpr);
              BOOST_SPIRIT_DEBUG_NODE(not_r);
              BOOST_SPIRIT_DEBUG_NODE(not1_r);
              BOOST_SPIRIT_DEBUG_NODE(not2_r);
              BOOST_SPIRIT_DEBUG_NODE(orExpr);
              BOOST_SPIRIT_DEBUG_NODE(expression);
              BOOST_SPIRIT_DEBUG_NODE(nodename);
              BOOST_SPIRIT_DEBUG_NODE(nodepath);
              BOOST_SPIRIT_DEBUG_NODE(dotdotpath);
              BOOST_SPIRIT_DEBUG_NODE(dotpath);
              BOOST_SPIRIT_DEBUG_NODE(absolutepath);
              BOOST_SPIRIT_DEBUG_NODE(event);
              BOOST_SPIRIT_DEBUG_NODE(less_than_comparable);
              BOOST_SPIRIT_DEBUG_NODE(nodestate);
              BOOST_SPIRIT_DEBUG_NODE(equality_comparible);
              BOOST_SPIRIT_DEBUG_NODE(equal_1);
              BOOST_SPIRIT_DEBUG_NODE(equal_2);
              BOOST_SPIRIT_DEBUG_NODE(not_equal_1);
              BOOST_SPIRIT_DEBUG_NODE(not_equal_2);
              BOOST_SPIRIT_DEBUG_NODE(and_or);
              BOOST_SPIRIT_DEBUG_NODE(operators);
              BOOST_SPIRIT_DEBUG_NODE(baseTrigger);
              BOOST_SPIRIT_DEBUG_NODE(subexpression);
              BOOST_SPIRIT_DEBUG_NODE(andsubexpression);
              BOOST_SPIRIT_DEBUG_NODE(orsubexpression);
              BOOST_SPIRIT_DEBUG_NODE(grouping);
              BOOST_SPIRIT_DEBUG_NODE(nodepathstate);
              BOOST_SPIRIT_DEBUG_NODE(integer);
              BOOST_SPIRIT_DEBUG_NODE(some_string);
              BOOST_SPIRIT_DEBUG_NODE(variable);
              BOOST_SPIRIT_DEBUG_NODE(variable_path);
              BOOST_SPIRIT_DEBUG_NODE(variable_expression);
              BOOST_SPIRIT_DEBUG_NODE(integerComparison);
         };

        rule<ScannerT> const& start() const { return expression; }
    };
};

/////////////////////////////////////////////////////////////////////////////////////////////

void print(tree_parse_info<> info,
           const std::string& expr,
           const std::map< parser_id, std::string >& rule_names);

AstTop* createAst( 	tree_parse_info< > info,
					const std::string& expr,
					const std::map< parser_id, std::string >& rule_names );

/////////////////////////////////////////////////////////////////////////////////////////////

ExprParser::ExprParser(  const std::string& expression ): expr_(expression) {}

static std::map< parser_id, std::string > rule_names;
static void populate_rule_names()
{
   if (rule_names.empty()) {
      rule_names[ExpressionGrammer::equal_1_ID] = "EQUALS";
      rule_names[ExpressionGrammer::equal_2_ID] = "EQUALS";
      rule_names[ExpressionGrammer::not_equal_1_ID] = "NOT_EQUAL";
      rule_names[ExpressionGrammer::not_equal_2_ID] = "NOT_EQUAL";
      rule_names[ExpressionGrammer::greater_equals_1_ID] = "GREATER_THAN_OR_EQUALS";
      rule_names[ExpressionGrammer::greater_equals_2_ID] = "GREATER_THAN_OR_EQUALS";
      rule_names[ExpressionGrammer::less_equals_1_ID] = "LESS_THAN_OR_EQUALS";
      rule_names[ExpressionGrammer::less_equals_2_ID] = "LESS_THAN_OR_EQUALS";
      rule_names[ExpressionGrammer::less_than_1_ID] = "LESS_THAN";
      rule_names[ExpressionGrammer::less_than_2_ID] = "LESS_THAN";
      rule_names[ExpressionGrammer::greater_than_1_ID] = "GREATER_THAN";
      rule_names[ExpressionGrammer::greater_than_2_ID] = "GREATER_THAN";
      rule_names[ExpressionGrammer::not1_ID ] = "NOT";
      rule_names[ExpressionGrammer::not2_ID ] = "NOT";
      rule_names[ExpressionGrammer::not3_ID ] = "NOT";
      rule_names[ExpressionGrammer::and_ID ] = "AND";
      rule_names[ExpressionGrammer::or_ID ] = "OR";
      rule_names[ExpressionGrammer::node_name_ID] = "NODE_NAME";
      rule_names[ExpressionGrammer::node_state_unknown_ID ] = "UNKNOWN";
      rule_names[ExpressionGrammer::node_state_complete_ID ] = "COMPLETE";
      rule_names[ExpressionGrammer::node_state_queued_ID ] = "QUEUED";
      rule_names[ExpressionGrammer::node_state_submitted_ID ] = "SUBMITTED";
      rule_names[ExpressionGrammer::node_state_active_ID ] = "ACTIVE";
      rule_names[ExpressionGrammer::node_state_aborted_ID ] = "ABORTED";
      rule_names[ExpressionGrammer::integer_ID] = "INTEGER";
      rule_names[ExpressionGrammer::event_ID ] = "EVENT";
      rule_names[ExpressionGrammer::dot_path_ID ] = "DOT_PATH";
      rule_names[ExpressionGrammer::dot_dot_path_ID ] = "DOT_DOT_PATH";
      rule_names[ExpressionGrammer::absolute_path_ID ] = "ABSOLUTE_PATH";
      rule_names[ExpressionGrammer::event_name_ID ] = "EVENT_NAME";

      rule_names[ExpressionGrammer::base_trigger_ID  ] = "BASE_TRIGGER";
      rule_names[ExpressionGrammer::sub_expression_ID  ] = "SUB_EXPRESSION";
      rule_names[ExpressionGrammer::grouping_ID  ] = "GROUPING";
      rule_names[ExpressionGrammer::node_path_state_ID  ] = "NODE_PATH_STATE";

      rule_names[ExpressionGrammer::some_string_ID  ]   = "STRING";
      rule_names[ExpressionGrammer::variable_ID  ]      = "VARIABLE";
      rule_names[ExpressionGrammer::variable_path_ID  ] = "VARIABLE_PATH";
      rule_names[ExpressionGrammer::grouped_variable_path_ID  ] = "GROUPED_VARIABLE_PATH";
      rule_names[ExpressionGrammer::normal_variable_path_ID  ] = "NORMAL_VARIABLE_PATH";
      rule_names[ExpressionGrammer::variable_expression_ID  ] = "VARIABLE_EXPRESSION";
   }
}

bool ExprParser::doParse(std::string& errorMsg)
{
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
	   ExprDuplicate::add(expr_,ast_.get());  // bypass spirit if same expression used
	   return true;
	}

	// SPIRIT CLASSIC parsing: very slooooow....
	ExpressionGrammer grammer;
   BOOST_SPIRIT_DEBUG_NODE(grammer);

   // Use parser that generates a abstract syntax tree
   tree_parse_info<> info = ast_parse( expr_.c_str(), grammer, space_p);
   if (info.full) {

	   populate_rule_names();

#if defined(BOOST_SPIRIT_DUMP_PARSETREE_AS_XML)
		tree_to_xml( cout, info.trees, expr_.c_str(), rule_names );
#endif
#if defined(PRINT_TREE)
		print(info,expr_,rule_names);
#endif
		// Spirit has created a AST for us. However it is not use able as is
		// we will traverse the AST and create our OWN  persist-able AST.
 		ast_.reset( createAst(info,expr_,rule_names) );
 		if (ast_->empty())  errorMsg = "Abstract syntax tree creation failed";
 		else {
 		  ExprDuplicate::add(expr_,ast_.get());
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
void do_print(const tree_iter_t& i,  const std::map< parser_id, std::string >& rule_names)
{
	Indentor in;
	std::map< parser_id, std::string >::const_iterator iter = rule_names.find(i->value.id());
	if (iter != rule_names.end()) {
		Indentor::indent(cout) << "Rule " << (*iter).second
			<< "  " << string( i->value.begin(), i->value.end() ) << endl;
	}
	else {
		Indentor::indent(cout) << "Unknown rule "
			<< "  " << string( i->value.begin(), i->value.end() ) <<  endl;
	}

	Indentor in2;
 	for (tree_iter_t t = i->children.begin(); t != i->children.end(); ++t) {
		do_print( t, rule_names );
	}
}

void print(tree_parse_info<> info,
           const std::string& expr,
           const std::map< parser_id, std::string >& rule_names)
{
	std::cout << "\nPRINT_TREE  " << expr << "\n";
	do_print(info.trees.begin(),rule_names);
}

////////////////////////////////////////////////////////////////////////////////////////////////

AstRoot* createRootNode(const tree_iter_t& i,  const std::map< parser_id, std::string >& rule_names)
{
#if defined(PRINT_AST_TRAVERSAL)
 	Indentor in;
	std::map< parser_id, std::string >::const_iterator iter = rule_names.find(i->value.id());
	if (iter != rule_names.end()) {
		Indentor::indent(cout) << "Root Rule " << (*iter).second
			<< "  " << string( i->value.begin(), i->value.end() ) << endl;
	}
	else {
		Indentor::indent(cout) << "Unknown root rule "
			<< "  " << string( i->value.begin(), i->value.end() ) <<  endl;
	}
#endif

	if ( i->value.id() == ExpressionGrammer::equal_1_ID ) return new AstEqual();
	if ( i->value.id() == ExpressionGrammer::equal_2_ID ) return new AstEqual();
	if ( i->value.id() == ExpressionGrammer::and_ID ) return new AstAnd();
	if ( i->value.id() == ExpressionGrammer::or_ID ) return new AstOr();
	if ( i->value.id() == ExpressionGrammer::not1_ID ) return new AstNot();
   if ( i->value.id() == ExpressionGrammer::not2_ID ) return new AstNot();
   if ( i->value.id() == ExpressionGrammer::not3_ID ) return new AstNot();
	if ( i->value.id() == ExpressionGrammer::plus_ID ) return new AstPlus();

	if ( i->value.id() == ExpressionGrammer::not_equal_1_ID ) return new AstNotEqual();
	if ( i->value.id() == ExpressionGrammer::not_equal_2_ID ) return new AstNotEqual();
	if ( i->value.id() == ExpressionGrammer::greater_equals_1_ID ) return new AstGreaterEqual();
	if ( i->value.id() == ExpressionGrammer::greater_equals_2_ID ) return new AstGreaterEqual();
	if ( i->value.id() == ExpressionGrammer::less_equals_1_ID ) return new AstLessEqual();
	if ( i->value.id() == ExpressionGrammer::less_equals_2_ID ) return new AstLessEqual();
	if ( i->value.id() == ExpressionGrammer::less_than_1_ID ) return new AstLessThan();
	if ( i->value.id() == ExpressionGrammer::less_than_2_ID ) return new AstLessThan();
	if ( i->value.id() == ExpressionGrammer::greater_than_1_ID ) return new AstGreaterThan();
	if ( i->value.id() == ExpressionGrammer::greater_than_2_ID ) return new AstGreaterThan();

	if ( i->value.id() == ExpressionGrammer::minus_ID ) return new AstMinus();
	if ( i->value.id() == ExpressionGrammer::multiply_ID ) return new AstMultiply();
   if ( i->value.id() == ExpressionGrammer::divide_ID ) return new AstDivide();
   if ( i->value.id() == ExpressionGrammer::modulo_ID ) return new AstModulo();
	LOG_ASSERT(false,"");
	return NULL;
}

Ast* createAst( const tree_iter_t& i, const std::map< parser_id, std::string >& rule_names ) {
#if defined(PRINT_AST_TRAVERSAL)
	Indentor in;
	std::map< parser_id, std::string >::const_iterator iter = rule_names.find(i->value.id());
	if (iter != rule_names.end()) {
		Indentor::indent(cout) << "Create AST Rule " << (*iter).second
			<< "  '" << string( i->value.begin(), i->value.end() ) << "'\n";
	}
	else {
		Indentor::indent(cout) << "Create AST Unknown rule "
			<< "  '" << string( i->value.begin(), i->value.end() ) <<  "'\n";
	}
#endif


	if ( i->value.id() == ExpressionGrammer::node_name_ID) {

	   string thevalue( i->value.begin(), i->value.end() );
	   boost::algorithm::trim(thevalue); // don't know why we get leading/trailing spaces
		LOG_ASSERT( !thevalue.empty(), "" );
	 	return new AstNode( thevalue );
	}
   else if ( i->value.id() == ExpressionGrammer::node_state_complete_ID) {

      return new AstNodeState( DState::COMPLETE );
   }
	else if ( i->value.id() == ExpressionGrammer::normal_variable_path_ID) {

		LOG_ASSERT((i->children.size() == 2 || i->children.size() == 4), "");
 		tree_iter_t theNodePathIter = i->children.begin();
		tree_iter_t theNameIter = i->children.begin()+1;

		string nodePath( theNodePathIter->value.begin(), theNodePathIter->value.end() );
		string name( theNameIter->value.begin(), theNameIter->value.end() );
		boost::algorithm::trim(nodePath); // don't know why we get leading/trailing spaces
		boost::algorithm::trim(name);     // don't know why we get leading/trailing spaces

		if (  i->children.size() == 4) {
 			AstRoot* operatorRoot = createRootNode(  i->children.begin()+2, rule_names  );
			Ast* astInteger = createAst(i->children.begin()+3,  rule_names);

			operatorRoot->addChild( new AstVariable( nodePath, name ));
			operatorRoot->addChild(astInteger);
			return operatorRoot;
		}

 		return new AstVariable( nodePath, name );
	}
	else if ( i->value.id() == ExpressionGrammer::dot_dot_path_ID) {

	   string thevalue( i->value.begin(), i->value.end() );
	   boost::algorithm::trim(thevalue); // don't know why we get leading/trailing spaces
		LOG_ASSERT( !thevalue.empty() , "");
	 	return new AstNode( thevalue );
	}
   if ( i->value.id() == ExpressionGrammer::absolute_path_ID) {

      string thevalue( i->value.begin(), i->value.end() );
      boost::algorithm::trim(thevalue); // don't know why we get leading/trailing spaces
      LOG_ASSERT( !thevalue.empty() ,"");
      return new AstNode( thevalue );
   }
	else if ( i->value.id() == ExpressionGrammer::dot_path_ID) {

	   string thevalue( i->value.begin(), i->value.end() );
	   boost::algorithm::trim(thevalue); // don't know why we get leading/trailing spaces
		LOG_ASSERT( !thevalue.empty() , "");
	 	return new AstNode( thevalue );
	}
   else if ( i->value.id() == ExpressionGrammer::event_ID) {

      LOG_ASSERT(i->children.size() >= 2, "");
      tree_iter_t theNodePathIter = i->children.begin();
      tree_iter_t theEventNameIter = i->children.begin()+1;

      string nodePath( theNodePathIter->value.begin(), theNodePathIter->value.end() );
      string eventName( theEventNameIter->value.begin(), theEventNameIter->value.end() );
      boost::algorithm::trim(nodePath);  // don't know why we get leading/trailing spaces
      boost::algorithm::trim(eventName); // don't know why we get leading/trailing spaces

      return new AstVariable( nodePath, eventName );
   }
	else if ( i->value.id() == ExpressionGrammer::some_string_ID) {

	   string thevalue( i->value.begin(), i->value.end() );
	   boost::algorithm::trim(thevalue); // don't know why we get leading/trailing spaces
 		return new AstString(thevalue);
	}
   else if ( i->value.id() == ExpressionGrammer::integer_ID) {

      string thevalue( i->value.begin(), i->value.end() );
      boost::algorithm::trim(thevalue); // don't know why we get leading/trailing spaces
      int theInt = boost::lexical_cast<int>(thevalue);
      return new AstInteger(theInt);
   }
	else if ( i->value.id() == ExpressionGrammer::node_state_aborted_ID) {

 	 	return new AstNodeState( DState::ABORTED );
	}
	else if ( i->value.id() == ExpressionGrammer::node_state_active_ID) {

 	 	return new AstNodeState( DState::ACTIVE );
	}
	else if ( i->value.id() == ExpressionGrammer::node_state_queued_ID) {

 	 	return new AstNodeState( DState::QUEUED );
	}
	else if ( i->value.id() == ExpressionGrammer::node_state_submitted_ID) {

 	 	return new AstNodeState( DState::SUBMITTED );
	}
	else if ( i->value.id() == ExpressionGrammer::node_state_unknown_ID) {

 	 	return new AstNodeState( DState::UNKNOWN );
	}

 	return NULL;
}

// Needed so that testing , when recreating expression uses same name for not.
static void set_not_name(AstRoot* not_root, boost::spirit::classic::parser_id id)
{
   if (not_root) {
      if (id == ExpressionGrammer::not1_ID)  not_root->set_root_name("not ");
      if (id == ExpressionGrammer::not2_ID)  not_root->set_root_name("~ ");
      if (id == ExpressionGrammer::not3_ID)  not_root->set_root_name("! ");
   }
}

// The evaluation function for the AST
Ast* doCreateAst(  const tree_iter_t& i,
                   const std::map< parser_id, std::string >& rule_names,
                   Ast* top)
{
#if defined(PRINT_AST_TRAVERSAL)
	Indentor in;
	std::map< parser_id, std::string >::const_iterator iter = rule_names.find(i->value.id());
	if (iter != rule_names.end()) {
		Indentor::indent(cout) << "Rule " << (*iter).second
			<< "  " << string( i->value.begin(), i->value.end() ) << endl;
	}
	else {
		Indentor::indent(cout) << "Unknown rule "
			<< "  " << string( i->value.begin(), i->value.end() ) <<  endl;
	}
#endif

	Indentor in2;
	if ( i->value.id() == ExpressionGrammer::event_ID ) {
		// Event need to handled in a custom way
		LOG_ASSERT(i->children.size() == 2,"");
		if (i->children.size() == 2) {
		   // a:eventname ||  ../a/b:eventname
		   // child 1: path                     a || ../a/b
		   // child 2: event name
		   // WE add an event state so that when the event is evaluated it is compared to true
		   Ast* someRoot =  new AstEqual();
		   Ast* leftEvent = createAst(i, rule_names);
		   Ast* rightEvent = new AstEventState( true );
		   someRoot->addChild(leftEvent);
		   someRoot->addChild(rightEvent);
		   top->addChild(someRoot);
		}
 	}
	else if (i->children.size() == 4 &&
			  (i->children.begin()->value.id() == ExpressionGrammer::not1_ID ||
			   i->children.begin()->value.id() == ExpressionGrammer::not2_ID ||
			   i->children.begin()->value.id() == ExpressionGrammer::not3_ID) ) {
		// child 0: notRoot                 0
		// child 1: notChild               +1
		// child 2: someRoot(i.e ==,!=)    +2
		// child 3: right                  +3
		// Create as:         someRoot
		//              notRoot          right
		//      notChild
		LOG_ASSERT((i->children.begin()->value.id() == ExpressionGrammer::not1_ID ||
		            i->children.begin()->value.id() == ExpressionGrammer::not2_ID ||
		            i->children.begin()->value.id() == ExpressionGrammer::not3_ID),"");
		AstRoot* notRoot = createRootNode(  i->children.begin(), rule_names  );
      set_not_name(notRoot,i->children.begin()->value.id());

		Ast* notChild = doCreateAst(  i->children.begin() + 1, rule_names, notRoot/*top*/ );
		if (notChild) notRoot->addChild(notChild);

		AstRoot* someRoot = createRootNode(  i->children.begin() + 2, rule_names  );
		someRoot->addChild(notRoot); //left

		Ast* right = doCreateAst(  i->children.begin() + 3, rule_names,  someRoot/* top*/ );
 		if (right) someRoot->addChild(right);
		top->addChild(someRoot);
	}
	else if (i->children.size() == 4 && i->value.id() == ExpressionGrammer::variable_expression_ID  ) {
		LOG_ASSERT((i->children.begin()->value.id() == ExpressionGrammer::normal_variable_path_ID), "");
		// child 0: NORMAL_VARIABLE_PATH    0
		// child 1: someRoot(i.e ==,!=)    +1
		// child 2: NOT                    +2
		// child 3: integer | variable     +3
		//
		// Create as:         someRoot
		//              varPath          notRoot
		//                                    notChild
  		AstRoot* someRoot = createRootNode(  i->children.begin()+1, rule_names  );
		Ast* varPath = doCreateAst(  i->children.begin(), rule_names, someRoot/*top*/ );

 		AstRoot* notRoot = createRootNode(  i->children.begin()+2, rule_names  );
		Ast* notChild = doCreateAst(  i->children.begin() + 3, rule_names, notRoot/*top*/ );

		notRoot->addChild(notChild);

 		someRoot->addChild(varPath); //left
 		someRoot->addChild(notRoot); //right
		top->addChild(someRoot);
	}
	else if (i->children.size() == 3) {
		// child 1: left                0
		// child 2: root(i.e ==,!=)    +1
		// child 3: right              +2
		AstRoot* someRoot = createRootNode(  i->children.begin() + 1, rule_names  );
		Ast* left  = doCreateAst(  i->children.begin(), rule_names, someRoot );
		Ast* right = doCreateAst(  i->children.begin() + 2, rule_names,  someRoot );
		if (left) someRoot->addChild(left);
		if (right) someRoot->addChild(right);
		top->addChild(someRoot);
	}
	else if (i->children.size() == 2 &&
			        (i->children.begin()->value.id() == ExpressionGrammer::not1_ID ||
		            i->children.begin()->value.id() == ExpressionGrammer::not2_ID ||
		            i->children.begin()->value.id() == ExpressionGrammer::not3_ID ) ) {
 		// child 1: not     0
		// child 2: left   +1
 		AstRoot* someRoot = createRootNode(  i->children.begin(), rule_names  );
 		set_not_name(someRoot,i->children.begin()->value.id());

		Ast* left  = doCreateAst(  i->children.begin() + 1, rule_names, someRoot );
  		if (left) someRoot->addChild(left);
 		top->addChild(someRoot);
	}
	else {
		return createAst(i,rule_names);
	}
	return NULL;
}

AstTop* createAst( 	tree_parse_info< > info,
					const std::string& expr,
					const std::map< parser_id, std::string >& rule_names
)
{
#if defined(PRINT_AST_TRAVERSAL)
	std::cout << "\nPRINT_AST_TRAVERSAL  " << expr << "\n";
#endif

	std::auto_ptr<AstTop> ast(new AstTop);
	(void)doCreateAst(info.trees.begin(),rule_names,ast.get());

#if defined(PRINT_AST)
 	if (ast.get())  {
 		std::cout << "\nPRINT_AST  " << expr << "\n";
 		std::cout << *ast.get();
 	}
#endif
	return ast.release();
}

///////////////////////////////////////////////////////////////////////////
// SimpleExprParser
//////////////////////////////////////////////////////////////////////////

bool has_complex_expressions(const std::string& expr)
{
   // we allow . and /
   if (expr.find('(') != string::npos) return true;
   if (expr.find(':') != string::npos) return true;
   if (expr.find('.') != string::npos) return true;
   if (expr.find('/') != string::npos) return true;
   if (expr.find(" not ") != string::npos) return true;
   if (expr.find(" and ") != string::npos) return true;
   if (expr.find(" or ") != string::npos) return true;
   if (expr.find('!') != string::npos) return true;
   if (expr.find("&&") != string::npos) return true;
   if (expr.find("||") != string::npos) return true;
   if (expr.find('<') != string::npos) return true;
   if (expr.find('>') != string::npos) return true;
   if (expr.find('+') != string::npos) return true;
   if (expr.find('-') != string::npos) return true;
   if (expr.find('*') != string::npos) return true;
   if (expr.find('~') != string::npos) return true;
   if (expr.find(" ne ") != string::npos) return true;
   if (expr.find(" ge ") != string::npos) return true;
   if (expr.find("<=") != string::npos) return true;
   if (expr.find(">=") != string::npos) return true;
   if (expr.find(" le ") != string::npos) return true;
   if (expr.find(" gt ") != string::npos) return true;
   if (expr.find(" lt ") != string::npos) return true;
   return false;
}

bool SimpleExprParser::doParse()
{
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

      boost::algorithm::trim(tokens[0]);
      boost::algorithm::trim(tokens[1]);

      if (tokens[0].find(' ') != string::npos) {
//         cout << "Found space " << expr_ << "\n";
         return false;
      }

      if (DState::isValid(tokens[1])) {

         ast_.reset(new AstTop);
         Ast* someRoot = new AstEqual();
         someRoot->addChild(new AstNode(tokens[0]));
         someRoot->addChild(new AstNodeState(DState::toState(tokens[1])));
         ast_->addChild(someRoot);
//       cout << "simple expr : " << expr_ << " `" << tokens[0] << "' = '" << tokens[1] << "'\n";
         return true;
      }
      else {
         try {
            int left = boost::lexical_cast<int>(tokens[0]);
            int right = boost::lexical_cast<int>(tokens[1]);
            ast_.reset(new AstTop);
            Ast* someRoot = new AstEqual();
            someRoot->addChild(new AstInteger(left));
            someRoot->addChild(new AstInteger(right));
            ast_->addChild(someRoot);
//               cout << "simple INT expr : " << expr_ << " `" << tokens[0] << "' = '" << tokens[1] << "'\n";
            return true;
         }
         catch ( boost::bad_lexical_cast& e ) {
//            cout << "simple INT FAILED expr : " << expr_ << " `" << tokens[0] << "' = '"
//                     << tokens[1] << "'\n";
         }
      }
   }
   return false;
}
