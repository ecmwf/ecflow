////============================================================================
//// Name        : OldCheckPtParser
//// Author      : Avi
//// Revision    : $Revision: #11 $ 
////
//// Copyright 2009-2012 ECMWF. 
//// This software is licensed under the terms of the Apache Licence version 2.0 
//// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
//// In applying this licence, ECMWF does not waive the privileges and immunities 
//// granted to it by virtue of its status as an intergovernmental organisation 
//// nor does it submit to any jurisdiction. 
////
//// Description :
////============================================================================
//
//// Define these before including anything else
//#define PHOENIX_LIMIT 10
//#define BOOST_SPIRIT_CLOSURE_LIMIT 10
//
//#include "OldCheckPtParser.hpp"
//
//// UN- comment the lines below to debug the parser
//// To debug a single defs file use:
//// #bjam clean
//// #bjam define=DEBUG_SINGLE_DEFS_FILE
//#ifdef DEBUG_SINGLE_DEFS_FILE
//#define DEBUG_NODE_STACK 1;
//#define BOOST_SPIRIT_DEBUG
//#endif
//
//#include <boost/spirit/include/classic.hpp>
//#include <boost/spirit/include/classic_core.hpp>
//#include <boost/spirit/include/classic_actor.hpp>
//#include <boost/spirit/include/classic_attribute.hpp>
//#include <boost/spirit/include/classic_confix.hpp>
//#include <boost/spirit/include/phoenix1_binders.hpp>
//#include <boost/spirit/include/classic_exceptions.hpp> // assertion,
//#include <boost/spirit/include/classic_actor.hpp>
//
//#include "boost/lambda/lambda.hpp"
//#include "boost/lambda/bind.hpp"
//#include "boost/cast.hpp"
//#include "boost/tuple/tuple.hpp"
//#include <boost/foreach.hpp>
//
//#include <iostream>
//#include <fstream>
//#include <string>
//#include <utility>
//#include <stack>
//#include <map>
//#include <sstream>
//
//#include <NodeTree.hpp>
//#include <Time.hpp>
//
//////////////////////////////////////////////////////////////////////////////
//using namespace std;
//using namespace boost::spirit;
//using namespace phoenix;
//using namespace BOOST_SPIRIT_CLASSIC_NS;
//
//////////////////////////////////////////////////////////////////////////////
//// Error
//// Errors to check for during the parse:
//enum Errors {
//	  task_name_expected,
//	  meter_name_expected,
//      expression_expected
//};
//
//// Assertions to use during the parse:
//assertion<Errors> expect_task_name(task_name_expected);
//assertion<Errors> expect_meter_name(meter_name_expected);
//assertion<Errors> expect_expression(expression_expected);
//
//using namespace ecf;
//
//////////////////////////////////////////////////////////////////////////////
////  closures
//////////////////////////////////////////////////////////////////////////////
//struct event_closure : boost::spirit::classic::closure< event_closure, int, std::string >
//{
//    member1 number;
//    member2 name;
//};
//struct string_pair_closure : boost::spirit::classic::closure< string_pair_closure, std::string, std::string >
//{
//    member1 name;
//    member2 value;
//};
//struct label_closure : boost::spirit::classic::closure< label_closure, std::string, std::string >
//{
//    member1 name;
//    member2 value;
//};
//struct meter_closure : boost::spirit::classic::closure< meter_closure,std::string,int,int,int >
//{
//    member1 name;
//    member2 min;
//    member3 max;
//    member4 colorChange;
//};
//struct string_closure : boost::spirit::classic::closure<string_closure, std::string>
//{
//    member1 name;
//};
//
//struct limit_closure : boost::spirit::classic::closure<limit_closure, std::string, int>
//{
//    member1 name;
//    member2 value;
//};
//struct inlimit_closure : boost::spirit::classic::closure<inlimit_closure, std::string, std::string, int>
//{
//    member1 name;
//    member2 path;
//    member3 priority;
//};
//
//struct date_closure : boost::spirit::classic::closure<date_closure,int,int,int>
//{
//    member1 day;
//    member2 month;
//    member3 year;
//};
//
//struct repeat_date_closure : boost::spirit::classic::closure<repeat_date_closure,std::string,int,int,int>
//{
//	member1 varName;
//    member2 start;
//    member3 end;
//    member4 delta;
//};
//
//struct int_closure : boost::spirit::classic::closure<int_closure, int>
//{
//    member1 theInt;
//};
//struct time_struct {
//	time_struct() : relative(false), start_hour(-1), start_minute(-1),
//					finish_hour(-1),finish_minute(-1), incr_hour(-1),incr_minute(-1) {}
//	bool relative;
//    int start_hour;
//    int start_minute;
//    int finish_hour;
//    int finish_minute;
//    int incr_hour;
//    int incr_minute;
//};
//std::ostream& operator<<(std::ostream& os, const time_struct& tm) { return os << "time_struct";}
//
//struct time_closure : boost::spirit::classic::closure<time_closure, time_struct >
//{
//	member1 t;
//};
//
//struct hh_mm_closure : boost::spirit::classic::closure<hh_mm_closure,bool,int,int>
//{
//	member1 relative; // if + was entered
//    member2 hour;
//    member3 minute;
//};
//
//struct late_option_closure : boost::spirit::classic::closure<late_option_closure,int>
//{
//	member1 option; // 0=submitted, 1=active, 2=complete
//};
//
//
//////////////////////////////////////////////////////////////////////////////////////////////
//
//void doThrow(const std::string& duplicateType, const std::string& theDuplicate,Node* node)
//{
//   std::stringstream ss;
//   ss << "Duplicate "
//      <<  duplicateType << " "
//      << theDuplicate << " for " << node->debugType() << " " << node->name() << "\n";
//   throw std::runtime_error(ss.str());
//}
//
////////////////////////////////////////////////////////////////////////////////////////
//// The objects *MUST* be held by reference since spirit will call contructor/destructor
//// many many times.
//struct OldCheckPointGrammer : public grammar<OldCheckPointGrammer>
//{
//	// The parser object is copied a lot, so only use reference/pointer variables
//	OldCheckPointGrammer(Defs* d,int& lineNum, std::stack<Node*>& nodeStack,std::map<Node*,bool>& defStatusMap)
//	    :defsFile_(d), lineNum_(lineNum), nodeStack_(nodeStack), defStatusMap_(defStatusMap)
//	{
////		std::cout << "OldCheckPointGrammer::OldCheckPointGrammer\n";
//	}
//	~OldCheckPointGrammer()
//	{
////		std::cout << "OldCheckPointGrammer::~OldCheckPointGrammer\n";
//	}
//
//    template <typename ScannerT>
//    struct definition
//    {
//         rule<ScannerT> defs, suite, suiteContents, endSuite ;
//         rule<ScannerT> family, endFamily,familyContent;
//         rule<ScannerT> task, event , nextLine, endTask, comment,defsContent;
//         rule<ScannerT> event_k, label_k, defstatus_k,leaf_sms;
//         rule<ScannerT> text,clock,repeat;
//         rule<ScannerT> repeat_type, repeat_day,repeat_month,repeat_year,repeat_enumerated,
//                        repeat_integer,repeat_string,repeat_file,cron;
//         rule<ScannerT, repeat_date_closure::context_t> repeat_date;
//
//         rule<ScannerT> late;
//         rule<ScannerT, hh_mm_closure::context_t> hh_mm;
//         rule<ScannerT, late_option_closure::context_t> late_option;
//
//         rule<ScannerT, string_pair_closure::context_t> externRule,variable;
//         rule<ScannerT, inlimit_closure::context_t> inlimit;
//         rule<ScannerT, limit_closure::context_t> limit;
//         rule<ScannerT, date_closure::context_t> date;
//
//         rule<ScannerT, event_closure::context_t> eventContent1,eventContent2;
//         rule<ScannerT, time_closure::context_t> time;
//         rule<ScannerT, label_closure::context_t> label;
//         rule<ScannerT, meter_closure::context_t> meter;
//         rule<ScannerT, int_closure::context_t> eventNumber;
//         rule<ScannerT, string_closure::context_t> identifier,suiteName,trigger, complete, expression,
//                        familyName,taskName,eventName,varName,varValue,quotedString,
//                        tickQuotedString, defstatus, nodestate,
//                        absolutePath,dotPath,dotDotPath,nodePath;
//
//         typedef uint_parser<int, 10, 1, 1>  one_int_p;
//         typedef uint_parser<int, 10, 2, 2>  two_int_p;
//         typedef uint_parser<int, 10, 4, 4>  four_int_p;
//         typedef uint_parser<int, 10, 8, 8>  eight_int_p;
//         one_int_p theDigit;
//         two_int_p theTime;
//         four_int_p theYear;
//         eight_int_p YMD;
//
//         // *NOTE WELL*. The bind must be after the matched rule, If it placed after the +nextLine
//         //              it will match many times, and hence create duplicate tasks,labels, etc
//         definition(OldCheckPointGrammer const& self) {
//
//        	distinct_parser<> keyword_p("a-zA-Z0-9_");
//
//            identifier
//                = 	lexeme_d   [ (alnum_p || ch_p('_'))  >> *(alnum_p || ch_p('_'))  ]
//                  	           [ identifier.name = construct_<std::string>(arg1, arg2) ]
//                ;
//
//            comment = comment_p('#');
//            nextLine = (ch_p('\n')[increment_a(self.lineNum_)]  | comment [increment_a(self.lineNum_)]);
//            quotedString
//				= comment_p("\"","\"") [ quotedString.name =  construct_<std::string>(arg1, arg2) ]
//			    ;
//
//            tickQuotedString = lexeme_d [ch_p("'") >> *(print_p - nextLine) ]
//                                        [ tickQuotedString.name =  construct_<std::string>(arg1, arg2) ];
//
//            absolutePath
//   				= 	(	!(str_p("/"))
//   						>> identifier
//   						>> *(
//   								str_p("/")
//   								>> identifier
//   							)
//   					)
//   					[ absolutePath.name = construct_<std::string>(arg1, arg2) ]
//   				;
//
//           	 dotDotPath  // a kind of relative path
//   				 = 	  (  str_p("..")
//   						 >> +(
//   								str_p("/")
//   								>> identifier
//							 )
//   				   	  )
//   				   	  [ dotDotPath.name = construct_<std::string>(arg1, arg2) ]
//  				  ;
//           	 dotPath
//				 = 	(	str_p(".")
//						 >> +(
//								 str_p("/")
//								 >> identifier
//							  )
//					 )
//					 [ dotPath.name = construct_<std::string>(arg1, arg2) ]
//				 ;
//           	 nodePath
//				 =     absolutePath  [ nodePath.name = arg1 ]
//				       | dotDotPath  [ nodePath.name = arg1 ]
//					   | dotPath     [ nodePath.name = arg1 ]
//				;
//
//            // We restrict triggers to family and tasks. The extraction of expression is
//            // deferred since node path can reference Nodes that have not yet been parsed.
//            expression
//				= 	lexeme_d [ *(print_p - nextLine) ]
//				             [ expression.name = construct_<std::string>(arg1, arg2) ];
//            trigger
//				= 	keyword_p("trigger")
//					>> expression[ trigger.name = arg1 ]
//					             [ bind(&OldCheckPointGrammer::addTrigger)(self,trigger.name) ]
//					>> +nextLine
//				;
//            complete
//				= 	keyword_p("complete")
//					>> expression[ complete.name = arg1]
//					             [ bind(&OldCheckPointGrammer::addComplete)(self,complete.name )]
//					>> +nextLine
//         	    ;
//
//            varName =  identifier[ varName.name = arg1] ;
//            varValue
//				=  	    tickQuotedString     [ varValue.name = arg1 ]
//				   	|   quotedString         [ varValue.name = arg1 ]
//				    |   identifier           [ varValue.name = arg1 ]
//				;
//            variable
//				= 	keyword_p("edit")
//					>> varName  [ variable.name = arg1]
//					>> varValue [ variable.value = arg1]
//					            [ bind(&OldCheckPointGrammer::addVariable)(self,variable.name,variable.value )]
//					>> +nextLine
//             	;
//
//        	suite = keyword_p("suite") ;
//        	suiteName
//				= 	identifier[ suiteName.name = arg1 ]
//				  	          [ bind(&OldCheckPointGrammer::addSuite)(self,suiteName.name) ]
//				  	>> +nextLine
//				;
//        	endSuite
//				= 	keyword_p("endsuite") [ bind(&OldCheckPointGrammer::popNode)(self) ]
//					>> *nextLine
//				;
//
//           	family = keyword_p("family");
//            familyName
//				= 	identifier[ familyName.name = arg1 ]
//				  	          [ bind(&OldCheckPointGrammer::addFamily)(self,familyName.name) ]
//				  	>> +nextLine
//                ;
//        	endFamily
//				= 	keyword_p("endfamily") [ bind(&OldCheckPointGrammer::popFamily)(self) ]
//					>> +nextLine
//				;
//
//           	task = keyword_p("task");
//            taskName
//				= 	identifier  [ taskName.name = arg1 ]
//				  	            [ bind(&OldCheckPointGrammer::addTask)(self,taskName.name) ]
//				  	>> +nextLine
//            	;
//           	endTask /* optional */
//				=	keyword_p("endtask") [ bind(&OldCheckPointGrammer::popNode)(self) ]
//           	        >> +nextLine
//           	    ;
//
//           	label_k = keyword_p("label");
//           	label
//				= 	label_k
//					>> identifier   [ label.name =  arg1  ]
//					>> quotedString [ label.value =  arg1 ]
//					                [ bind(&OldCheckPointGrammer::addLabel)(self,label.name,label.value)]
//					>> +nextLine
//				;
//
//#define ASSIGN(member, what) bind(&time_struct::member)(time.t) = what
//           	time
//				=  	keyword_p("time")
//           	        >> !ch_p('+')                       [ ASSIGN(relative,true)      ]
//           	        >> (
//           	        		 (	theTime                 [ ASSIGN(start_hour,arg1)    ]
//                     	        		   >> ch_p(':')
//                     	        		   >> theTime   [ ASSIGN(start_minute,arg1)  ]
//           	        			>> theTime              [ ASSIGN(finish_hour,arg1)   ]
//           	        			           >> ch_p(':')
//           	           	        	 	   >> theTime   [ ASSIGN(finish_minute,arg1)  ]
//           	        			>> theTime              [ ASSIGN(incr_hour,arg1)      ]
//           	        			           >> ch_p(':')
//           	        			           >> theTime   [ ASSIGN(incr_minute,arg1)  ]
//           	        		 )
//           	        		 | theTime                  [ ASSIGN(start_hour,arg1)   ]
//           	        		   >> ch_p(':')
//           	        	 	   >> theTime               [ ASSIGN(start_minute,arg1) ]
//					   )
//           	           [ bind(&OldCheckPointGrammer::addTime)(self,time.t) ]
//           	        >> +nextLine
//           	        ;
//#undef ASSIGN
//
//           	defstatus_k = keyword_p("defstatus");
//           	nodestate
//				= 	str_p("complete")  [ nodestate.name = construct_<std::string>(arg1, arg2) ]
//				| 	str_p("unknown")   [ nodestate.name = construct_<std::string>(arg1, arg2) ]
//				|   str_p("queued")    [ nodestate.name = construct_<std::string>(arg1, arg2) ]
//				|   str_p("aborted")   [ nodestate.name = construct_<std::string>(arg1, arg2) ]
//				|   str_p("active")    [ nodestate.name = construct_<std::string>(arg1, arg2) ]
//				|   str_p("suspended") [ nodestate.name = construct_<std::string>(arg1, arg2) ]
//           	    ;
//           	defstatus
//				= 	defstatus_k
//					>> nodestate [ defstatus.name =  arg1]
//	             	             [ bind(&OldCheckPointGrammer::defStatus)(self,defstatus.name) ]
//					>> +nextLine
//				;
//
//           	event_k = keyword_p("event");
//            eventNumber =  uint_p [ eventNumber.theInt = arg1 ];
//            eventName =  identifier[ eventName.name =  arg1 ] ;
//            eventContent1.number = 0;
//            eventContent2.number = 0;
//            // Could not get closure initialisation to work. The work around was to define another
//            // action, where if only the name is specified we set the number to -1
//            eventContent1
//				= 	event_k
//					>> (
//							eventNumber     [ eventContent1.number = arg1 ]
//							|   eventName   [ eventContent1.name = arg1   ]
//            	             	            [ eventContent1.number = -1   ]
//					   )
//            	       [ bind(&OldCheckPointGrammer::addEvent)(self,eventContent1.number,eventContent1.name)]
//					>> +nextLine
//             	;
//            eventContent2
//				= 	event_k
//					>> (
//							eventNumber  [ eventContent2.number = arg1 ]
//							>> eventName [ eventContent2.name = arg1   ]
//	                   )
//					   [ bind(&OldCheckPointGrammer::addEvent)(self,eventContent2.number,eventContent2.name)]
//					>> +nextLine
//              	;
//            event = eventContent2 | eventContent1;
//
//
//
//            externRule
//  				= 	keyword_p("extern")
//  					>> (
//  							absolutePath            [ externRule.value = arg1 ]
//  							>> !(
//  									":"
//  									>> identifier   [ externRule.name = arg1 ]
//  							    )
//  					   )
//  					   [ bind(&OldCheckPointGrammer::addExtern)(self,externRule.value,externRule.name) ]
//  					>> +nextLine
//  				;
//
//
//            limit
//				= 	keyword_p("limit")
//					>> (
//							identifier       [ limit.name  = arg1 ]
//							>> uint_p        [ limit.value = arg1 ]
//				       )
//					   [ bind(&OldCheckPointGrammer::addLimit)(self,limit.name,limit.value) ]
//					>> +nextLine
//				;
//            inlimit
//				= 	keyword_p("inlimit")
// 					>> (
// 					      (
//							    (
//									   nodePath      [ inlimit.path =  arg1 ]
//									   >> ":"
// 								       >> identifier [ inlimit.name =  arg1  ] [ inlimit.priority =  -1 ]
//							    )
//							    |
//							    identifier  [ inlimit.name = arg1 ] [ inlimit.priority =  -1 ]
//					      )
//						  >> !uint_p [ inlimit.priority =  arg1 ]
// 					   )
// 					   [ bind(&OldCheckPointGrammer::addInLimit)(self,inlimit.name,inlimit.path,inlimit.priority) ]
//                    >> +nextLine
//                ;
//
//
//             meter
//				= 	keyword_p("meter")
//					>> expect_meter_name(identifier[ meter.name = arg1 ])
//					>> (
//							int_p     [ meter.min = arg1  ]
//					        >> int_p  [ meter.max = arg1  ]
//                  	                  [ meter.colorChange = arg1 ] // init in case meterColorChange is missing
//                            >> !int_p [ meter.colorChange = arg1 ]
//                       )
//                       [ bind(&OldCheckPointGrammer::addMeter)(self,meter.name, meter.min, meter.max, meter.colorChange)]
//                    >> +nextLine
//                 ;
//
//             hh_mm
//				 =  !(ch_p('+') [ hh_mm.relative = true ] )
//				    >> theTime  [ hh_mm.hour = arg1 ]
//				    >> ":"
//				    >> theTime  [ hh_mm.hour = arg1 ]
//				 ;
//             late_option
//				 = ( str_p("-c") >> hh_mm )
//				 | ( str_p("-s") >> hh_mm )
//				 | (str_p("-a") >> hh_mm) ;
//             late
//				 = 	 keyword_p("late")
//					 >> late_option
//					 >> !late_option
//					 >> !late_option
//	                 >> +nextLine
//				 ;
//
//             repeat_type
//				 = repeat_date
//                   | repeat_day | repeat_month | repeat_year | repeat_integer
//                   | repeat_enumerated | repeat_string | repeat_file;
//
//             repeat_day = str_p("day") >> uint_p >> !YMD;
//             repeat_month = str_p("month") >> uint_p >> !YMD;
//             repeat_year = str_p("year") >> uint_p >> !YMD;
//             repeat_integer = str_p("integer") >> identifier >> int_p >> int_p >> int_p;
//             repeat_enumerated = str_p("enumerated") >> identifier >> +identifier;
//             repeat_string = str_p("string") >> identifier >> +identifier;
//             repeat_file = str_p("file") >> identifier >> identifier;
//             repeat_date
//				 = 	(	str_p("date")
//						 >> identifier  [ repeat_date.varName =  arg1 ]
//						 >> YMD         [ repeat_date.start =  arg1 ]
//						 >> YMD         [ repeat_date.end =  arg1 ]
//					      >> uint_p      [ repeat_date.delta =  arg1 ]
//					 )
//					 [ bind(&OldCheckPointGrammer::addRepeatDate) (self,repeat_date.varName,
//				                	                                    repeat_date.start,
//				                	                                    repeat_date.end,
//				                	                                    repeat_date.delta)]
//				  ;
//             repeat  =  keyword_p("repeat") >> repeat_type >> +nextLine;
//
//
//             text = keyword_p("text") >> (tickQuotedString | quotedString ) >> +nextLine;
//
//             date
//				 =
//					 (  theTime       [ date.day = arg1]
//					 |  theDigit      [ date.day = arg1]
//					 )
//					 >> ch_p(".")
//					 >>  (  theTime   [ date.month = arg1]
//					     | theDigit   [ date.month = arg1]
//					     )
//					 >> ch_p(".")
//					 >> theYear       [ date.year = arg1]
//				  ;
//             clock
//				 = 	keyword_p("clock")
//					>> ( str_p("real") | str_p("hybrid") )
//					>> (
//							( date >> !(hh_mm | int_p) )
//							|
//							( hh_mm | int_p )
// 					   )
//					>> +nextLine
//				 ;
//             cron = keyword_p("cron") >> lexeme_d [ *(print_p - nextLine) ] >> +nextLine;
//
//             // Only one trigger/complete/def status is allowed, more easily check by data model/semantics
//             // This also allows trigger/complete/def status to appear in any order. This was not possible
//             // when we just defining it via EBNF.
//             leaf_sms
//				= 	*(
//						 variable
//						| trigger
//						| time
//						| defstatus
//					    | complete
//            		    | inlimit
//						| label
//						| event
//            		    | late
//            		    | text
//            		    | limit
//						| meter
//            		    | repeat
//            		    | cron
//            		 )
//                ;
//            familyContent
//				= 	(
//						task
//						>> 	expect_task_name(taskName)
//				    	>> 	leaf_sms
//				    	>> !endTask
//					)
//					| suiteContents
//				;
//        	suiteContents = family >> familyName >> !leaf_sms >> *familyContent >> endFamily ;
//
//        	defsContent
//				= 	suite
//					>> suiteName
//					>> *(
//							variable
//						    | inlimit
//							| defstatus
//						    | limit
//						    | late
//						    | clock
//						    | repeat
//						)
//					>> *suiteContents
//					>> endSuite
//				;
//
//        	defs
//				=  *(
//						nextLine | externRule
//					)
//					>> +defsContent
//					>> end_p;
//
//            BOOST_SPIRIT_DEBUG_NODE(suite);
//            BOOST_SPIRIT_DEBUG_NODE(identifier);
//            BOOST_SPIRIT_DEBUG_NODE(suiteName);
//            BOOST_SPIRIT_DEBUG_NODE(suiteContents);
//            BOOST_SPIRIT_DEBUG_NODE(family);
//            BOOST_SPIRIT_DEBUG_NODE(familyName);
//            BOOST_SPIRIT_DEBUG_NODE(familyContent);
//            BOOST_SPIRIT_DEBUG_NODE(endFamily);
//            BOOST_SPIRIT_DEBUG_NODE(leaf_sms);
//            BOOST_SPIRIT_DEBUG_NODE(task);
//            BOOST_SPIRIT_DEBUG_NODE(taskName);
//            BOOST_SPIRIT_DEBUG_NODE(endTask);
//            BOOST_SPIRIT_DEBUG_NODE(event);
//            BOOST_SPIRIT_DEBUG_NODE(eventContent1);
//            BOOST_SPIRIT_DEBUG_NODE(eventContent2);
//            BOOST_SPIRIT_DEBUG_NODE(eventName);
//            BOOST_SPIRIT_DEBUG_NODE(eventNumber);
//            BOOST_SPIRIT_DEBUG_NODE(meter);
//            BOOST_SPIRIT_DEBUG_NODE(nextLine);
//            BOOST_SPIRIT_DEBUG_NODE(comment);
//            BOOST_SPIRIT_DEBUG_NODE(endSuite);
//            BOOST_SPIRIT_DEBUG_NODE(defs);
//            BOOST_SPIRIT_DEBUG_NODE(defsContent);
//            BOOST_SPIRIT_DEBUG_NODE(variable);
//            BOOST_SPIRIT_DEBUG_NODE(varName);
//            BOOST_SPIRIT_DEBUG_NODE(label);
//            BOOST_SPIRIT_DEBUG_NODE(varValue);
//            BOOST_SPIRIT_DEBUG_NODE(quotedString);
//            BOOST_SPIRIT_DEBUG_NODE(tickQuotedString);
//            BOOST_SPIRIT_DEBUG_NODE(trigger);
//            BOOST_SPIRIT_DEBUG_NODE(complete);
//            BOOST_SPIRIT_DEBUG_NODE(expression);
//            BOOST_SPIRIT_DEBUG_NODE(defstatus);
//            BOOST_SPIRIT_DEBUG_NODE(time);
//            BOOST_SPIRIT_DEBUG_NODE(externRule);
//            BOOST_SPIRIT_DEBUG_NODE(limit);
//            BOOST_SPIRIT_DEBUG_NODE(inlimit);
//            BOOST_SPIRIT_DEBUG_NODE(late);
//            BOOST_SPIRIT_DEBUG_NODE(text);
//            BOOST_SPIRIT_DEBUG_NODE(clock);
//            BOOST_SPIRIT_DEBUG_NODE(date);
//            BOOST_SPIRIT_DEBUG_NODE(hh_mm);
//            BOOST_SPIRIT_DEBUG_NODE(repeat);
//            BOOST_SPIRIT_DEBUG_NODE(repeat_day);
//            BOOST_SPIRIT_DEBUG_NODE(repeat_month);
//            BOOST_SPIRIT_DEBUG_NODE(repeat_year);
//            BOOST_SPIRIT_DEBUG_NODE(repeat_integer);
//            BOOST_SPIRIT_DEBUG_NODE(repeat_string);
//            BOOST_SPIRIT_DEBUG_NODE(repeat_enumerated);
//            BOOST_SPIRIT_DEBUG_NODE(repeat_file);
//            BOOST_SPIRIT_DEBUG_NODE(repeat_date);
//         };
//
//        rule<ScannerT> const& start() const { return defs; }
//    };
//
//    Defs* defsFile_;
//    int &lineNum_;
//    std::stack<Node*>& nodeStack_;
//    std::map<Node*,bool>& defStatusMap_;
//
//
//    void dumpStackTop(const std::string& msg, const std::string& msg2 = "") const {
//#ifdef DEBUG_NODE_STACK
//    	std::cout << msg << "  '" << msg2 << "' ++++++++++++++++++++++++++++++++++++++++++++++++++\n";
//    	if (nodeStack_.empty()) std::cout << "nodeStack_ is EMPTY\n";
//    	else  std::cout << "TOP = " <<  nodeStack_.top()->debugType()
//    	                << " '" << nodeStack_.top()->name() << "'\n";
//#endif
//    }
//
//    ////////////////////////////////////////////////////////////////////////////
//    //  semantic actions
//    ////////////////////////////////////////////////////////////////////////////
//    void popNode( ) const
//    {
//    	dumpStackTop("popNode");
//     	nodeStack_.pop( );
//    }
//
//    void popFamily() const
//    {
//    	dumpStackTop("popFamily");
//
//    	// Compensate for the fact that Task don't have endtask, hence when we pop for a
//    	// family, the top should be a suite/family
// 		if ( dynamic_cast<Task*>( nodeStack_.top() )) {
// 	     	nodeStack_.pop(); // pop the task
// 	     	nodeStack_.pop(); // pop the family to get to suite/family
//   		}
// 		else {
// 	     	nodeStack_.pop(); // pop the family to get to suite/family
// 		}
//    }
//
//    void addSuite( const std::string& name) const
//    {
//    	dumpStackTop("addSuite",name);
//
//      	if (defsFile_->findSuite( name )) {
//       	    std::stringstream ss;
//        	ss << "Duplicate Suite " << name << "\n";
//            throw std::runtime_error(ss.str());
//      	}
//
//    	std::auto_ptr<Suite> suite(new Suite(name));
//    	nodeStack_.push( suite.get() );
//      	defsFile_->addSuite( suite );
//    }
//
//    void addFamily( const std::string& familyName) const
//    {
//    	dumpStackTop("addFamily",familyName);
//
//       	assert( !nodeStack_.empty() );
//        Suite* lastAddedSuite = dynamic_cast<Suite*>( nodeStack_.top() );
//        if (lastAddedSuite ) {
//
//        	std::auto_ptr<Family> family (new Family(familyName));
//        	nodeStack_.push( family.get() );
//
//        	if (!lastAddedSuite->addFamily( family )) {
//        		doThrow("family",familyName,lastAddedSuite);
//         	}
//        }
//        else {
//        	 // support hierarchical families
//             Family* lastAddedFamily = dynamic_cast<Family*>( nodeStack_.top() );
//             if ( lastAddedFamily ) {
//
//             	std::auto_ptr<Family> family (new Family(familyName));
//              	nodeStack_.push( family.get() );
//
//             	if (!lastAddedFamily->addFamily( family )) {
//            		doThrow("family",familyName,lastAddedFamily);
//             	}
//             }
//             else {
//                  Task* lastAddedTask = dynamic_cast<Task*>( nodeStack_.top() );
//                  if ( lastAddedTask ) {
//                	  // Pop the node, since tasks don't always have end task
//                	  popNode();
//                	  addFamily(familyName);
//                  }
//             }
//        }
//    }
//
//    void addTask( const std::string& taskName) const
//    {
//    	dumpStackTop("addTask",taskName);
//
//    	// bad test data can mean that last node is not a family or task, will fail parse
//    	if (nodeStack_.empty() ) return;
//
//        Family* lastAddedFamily = dynamic_cast<Family*>( nodeStack_.top() );
//        if ( lastAddedFamily ) {
//
//        	std::auto_ptr<Task> task(new Task(taskName));
//        	nodeStack_.push( task.get() );
//
//        	if(!lastAddedFamily->addTask( task )) {
//        		doThrow("task",taskName,lastAddedFamily);
//        	}
//        }
//        else {
//            if ( dynamic_cast<Task*>( nodeStack_.top() ) ) {
//
//            	// Task can _only_ be added to families.
//            	// pop the top Node to get to the Family, then call recursively to add task
//            	popNode();
//            	addTask( taskName );
//            }
//        }
//    }
//
//    void addEvent(int number, const std::string& name) const
//    {
//#ifdef DEBUG_NODE_STACK
//    	std::cout << "Event number = '" << number << "' name = '" << name << "'\n";
//#endif
//    	dumpStackTop("addEvent",name);
//
//    	assert( !nodeStack_.empty() );
//     	Task* lastAddedTask = dynamic_cast<Task*>( nodeStack_.top() );
//     	assert( lastAddedTask );
//
//     	if (!lastAddedTask->addEvent( Event(number, name) )) {
//     		doThrow("Event",name,lastAddedTask);
//     	}
//     }
//
//    void addVariable(const std::string& name, const std::string& value) const
//    {
//    	dumpStackTop("addVariable",name);
//
//     	if (!nodeStack_.empty()) {
//     		Node* node = nodeStack_.top();
//     		if (!node->addVariable( Variable(name, value) )) {
//     			doThrow("Variable",name,node);
//     		}
//     	}
//    }
//
//    void addTrigger(const std::string& exp) const {
//
//    	dumpStackTop("addTrigger",exp);
//
//     	if (!nodeStack_.empty()) {
//     		Node* node = nodeStack_.top();
//    		if (!node->add_trigger( exp )) {
//    			std::stringstream ss;
//    			ss << node->debugType() << " " << node->name() << " already has a trigger.";
//    			throw std::runtime_error(ss.str());
//    		}
//     	}
//    }
//
//    void addComplete(const std::string& exp) const {
//
//    	dumpStackTop("addComplete",exp);
//
//     	if (!nodeStack_.empty()) {
//     		Node* node = nodeStack_.top();
//    		if (!node->add_complete( exp )) {
//    			std::stringstream ss;
//    			ss << node->debugType() << " " << node->name() << " already has a complete.";
//    			throw std::runtime_error(ss.str());
//    		}
//     	}
//    }
//
//    void addLabel(const std::string& name, const std::string& value) const  {
//
//     	dumpStackTop("addLabel",name);
//
//    	assert( !nodeStack_.empty() );
//
//     	if (!nodeStack_.top()->addLabel( Label(name, value) )) {
//     		doThrow("Label",name,nodeStack_.top());
//     	}
//     }
//
//    void defStatus(const std::string& nodestate) const {
//
//      	dumpStackTop("updateState",nodestate);
//
//      	if (!nodeStack_.empty()) {
//     		Node* node = nodeStack_.top();
//
//     		// Check default status not already defined for this node.
//     	    std::map<Node*,bool>::const_iterator it =  defStatusMap_.find(node);
//     	    if (it != defStatusMap_.end()) {
//     	    	if ((*it).second) {
//       	   			std::stringstream ss;
//     	    		ss << node->debugType() << " " << node->name() << " already has a default status\n";
//     	    		throw std::runtime_error(ss.str());
//     	    	}
//     	    }
//     	    defStatusMap_[node] =  true;
//
//     		node->addDefStatus( NState::toState(nodestate)  );
//     	}
//    }
//
//    void addMeter(const std::string& name, int min, int max, int colorChange) const {
//    	assert( !nodeStack_.empty() );
//     	assert( !dynamic_cast<Suite*>( nodeStack_.top() ) );
//
//     	if (!nodeStack_.top()->addMeter( Meter(name,min,max,colorChange))) {
//     		doThrow("Meter",name,nodeStack_.top());
//     	}
//    }
//
//    void testTime(int hour, int minute) const {
//    	if (hour == -1 || minute == -1) {
//    		return;
//    	}
//     	if (hour < 0 || hour > 23) {
//     		Node* node = nodeStack_.top();
//	   		std::stringstream ss;
//	    	ss << node->debugType() << " " << node->name() << " time hour must be in range 0-23\n";
//	    	throw std::runtime_error(ss.str());
//      	}
//      	if (minute < 0 || minute > 59) {
//     		Node* node = nodeStack_.top();
//	   		std::stringstream ss;
//	    	ss << node->debugType() << " " << node->name() << " time minute must be in range 0-59\n";
//	    	throw std::runtime_error(ss.str());
//      	}
//    }
//
//    void addTime(const time_struct& t) const {
//      	dumpStackTop("addTime");
//      	testTime(t.start_hour,t.start_minute);
//      	testTime(t.finish_hour,t.finish_minute);
//      	testTime(t.incr_hour,t.incr_minute);
//
//      	TimeSlot start(t.start_hour,t.start_minute);
//      	TimeSlot finish(t.finish_hour,t.finish_minute);
//      	TimeSlot incr(t.incr_hour,t.incr_minute);
//      	Time time(start,finish,incr,t.relative);
//
//      	//cout << "time added ";
//      	//time.print(cout);
//
//  		nodeStack_.top()->addTime(time);
//    }
//
//    void addExtern(const std::string& path, const std::string& obj) const
//    {
//      	dumpStackTop("addExtern");
//      	// obj is one event,meter,limit, label.
//      	// cout << "add extern path = '" << path << "' obj = '" << obj << "'\n";
//      	defsFile_->addExtern(path,obj);
//    }
//
//    void addLimit(const std::string& limitName, int limitValue) const
//    {
//      	dumpStackTop("addLimit",limitName);
//
//      	if (!nodeStack_.empty()) {
//     		Node* node = nodeStack_.top();
//
//        	if (!node->addLimit( Limit(limitName, limitValue) )) {
//         		doThrow("Limit",limitName,node);
//         	}
//     	}
//    }
//
//    void addInLimit(const std::string& limitName, const std::string& path, int priority) const
//    {
//      	dumpStackTop("addLimit",limitName);
//
//      	if (!nodeStack_.empty()) {
//     		Node* node = nodeStack_.top();
//
//        	if (!node->addInLimit( InLimit(limitName, path, priority)) ) {
//         		doThrow("InLimit",limitName,node);
//         	}
//     	}
//    }
//
//    void addRepeatDate(const std::string& name, int start, int end, int delta) const {
//
//      	dumpStackTop("addRepeatDate",name);
//      	assert( !nodeStack_.empty() );
//#ifdef DEBUG_NODE_STACK
//      	cout << "addRepeatDate name= " << name << " start= " << start << " end= " << end << " delta= " << delta << "\n";
//#endif
//      	if (!nodeStack_.top()->addRepeat( Repeat( RepeatDate(name,start,end,delta) ))) {
//      		doThrow("Repeat",name,nodeStack_.top());
//      	}
//     }
//};
//
////----------------------------------------------------------------------------------------------
//
//OldCheckPtParser::OldCheckPtParser(Defs* defsfile, const std::string& path_to_defs) : defsfile_(defsfile),path_to_defs_(path_to_defs)  {}
//
//bool OldCheckPtParser::doParse(std::string& errorMsg) const
//{
//	std::ifstream in( path_to_defs.c_str(), std::ios_base::in );
//	if ( !in ) {
//	    std::stringstream ss;
//		ss << "Unable to open file! " << path_to_defs << "\n";
//		errorMsg = ss.str();
//		return false;
//	}
//
//	std::string file;              // We will read the contents here.
//	in.unsetf( std::ios::skipws ); // No white space skipping!
//	std::copy(  std::istream_iterator< char >( in ),
//				std::istream_iterator< char >(),
//				std::back_inserter( file ) );
//	std::string::const_iterator first = file.begin();
//	std::string::const_iterator last = file.end();
//
//	typedef std::string::const_iterator iterator_type;
//	//typedef OldCheckPointGrammer< iterator_type > OldCheckPointGrammer;
//
//	std::map<Node*,bool> defStatusMap; // ensure defstatus define once per node without bloating DM
//	std::stack<Node*> nodeStack;
//	int lineNum = 0;
// 	OldCheckPointGrammer grammer(defsfile_,lineNum,nodeStack,defStatusMap); //  Our Phrase level parser
//
//	/// Note: Our separator is blank_p i.e spaces and tabs. means that we need to deal with newline
//	/// **** This is significant as we need newline to disambiguate the grammar ****
//    BOOST_SPIRIT_DEBUG_NODE(grammer);
//    parse_info<iterator_type> info;
//
//    try {
//    	info = parse(first,last, grammer, blank_p);
//    }
//    catch (parser_error<Errors,iterator_type> x) {
//
//	    std::stringstream ss;
//        switch (x.descriptor) {
//            case task_name_expected:   ss << "Expected task name" << endl; break;
//            case meter_name_expected:  ss << "Expected meter name" << endl; break;
//            case expression_expected:  ss << "Expected operand" << endl; break;
//         }
//
//         // Only show last 200 characters, since some suites are massive
//        iterator_type theStart = first;
//         if (std::distance(first,x.where) > 200) theStart = x.where - 200;
//
//         while (theStart != x.where) { ss << *theStart++; }
//         ss << "----->| "  << endl;
// 		 errorMsg = ss.str();
//    }
//    catch (std::exception& e) {
//    	errorMsg =  e.what();
//    }
//
//	if (info.full) {
//
//		// Now parse the trigger/complete expressions and any further checks
// 		bool result = defsfile_->check(errorMsg);
//#ifdef DEBUG
// 		assert(defsfile_->checkInvariants());
//#endif
// 		return result;
//  	}
//	else {
//	    std::stringstream ss;
// 		ss << "Parsing failed: Around line: " << (lineNum + 1) << "\n";
//		ss << "length = " << std::dec << info.length << "\n";
//		//ss << "stopped at: \": " << info.stop << "\"\n";
//		errorMsg += ss.str();
// 	}
//	return false;
//}
