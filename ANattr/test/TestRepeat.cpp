
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #16 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/time_formatters.hpp>  // requires boost date and time lib
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "RepeatAttr.hpp"
#include "Cal.hpp"

using namespace std;
using namespace boost::gregorian;
using namespace boost::posix_time;

BOOST_AUTO_TEST_SUITE( ANattrTestSuite )

BOOST_AUTO_TEST_CASE( test_repeat_invariants )
{
   cout << "ANattr:: ...test_repeat_invariants\n";

   std::vector<std::string> stringList; stringList.reserve(3);
   stringList.push_back("a");
   stringList.push_back("b");
   stringList.push_back("c");

   // Test the invariant that Non empty repeat must have a name
   {
      Repeat empty;
      Repeat empty2;
      BOOST_CHECK_MESSAGE(empty.empty(),"Construction");
      BOOST_CHECK_MESSAGE(empty.name().empty(),"Construction");
      BOOST_CHECK_MESSAGE(empty == empty2,"Equality failed");
   }
   {
      Repeat rep(RepeatDate("YMD",20090916,20090930,1));
      BOOST_CHECK_MESSAGE(!rep.empty()," Repeat should not be empty");
      BOOST_CHECK_MESSAGE(!rep.name().empty(),"name should not be empty");
      BOOST_CHECK_MESSAGE(rep.name() == "YMD","name not as expected");
      BOOST_CHECK_MESSAGE(rep.start() == 20090916,"Start should be 20090916");
      BOOST_CHECK_MESSAGE(rep.end() == 20090930,"end should be 20090930");
      BOOST_CHECK_MESSAGE(rep.step() == 1,"step should be 1");
      BOOST_CHECK_MESSAGE(rep.value() == 20090916,"value should be 20090916");
      BOOST_CHECK_MESSAGE(rep.last_valid_value() == 20090916,"last_valid_value should be 20090916");

      Repeat cloned = Repeat(rep);
      BOOST_CHECK_MESSAGE(cloned == rep,"Equality failed");
      BOOST_CHECK_MESSAGE(cloned.name() == "YMD","not as expected");
      BOOST_CHECK_MESSAGE(cloned.start() == 20090916,"not as expected");
      BOOST_CHECK_MESSAGE(cloned.end() == 20090930,"not as expected");
      BOOST_CHECK_MESSAGE(cloned.step() == 1,"not as expected");
      BOOST_CHECK_MESSAGE(cloned.value() == 20090916,"not as expected");
      BOOST_CHECK_MESSAGE(cloned.valueAsString() == "20090916","not as expected");
      BOOST_CHECK_MESSAGE(cloned.last_valid_value() == 20090916,"last_valid_value should be 20090916");

      RepeatDate empty;
      BOOST_CHECK_MESSAGE(empty.start() == 0,"Start should be 0");
      BOOST_CHECK_MESSAGE(empty.end() == 0,"end should be 0");
      BOOST_CHECK_MESSAGE(empty.step() == 0,"step should be 0");
      BOOST_CHECK_MESSAGE(empty.value() == 0,"delta should be 0");
      BOOST_CHECK_MESSAGE(empty.name().empty(),"name should be empty");
      BOOST_CHECK_MESSAGE(empty.name() == "","name not as expected");
   }
   {
      Repeat rep(RepeatDate("YMD",20090930,20090916,-1));
      BOOST_CHECK_MESSAGE(!rep.empty()," Repeat should not be empty");
      BOOST_CHECK_MESSAGE(!rep.name().empty(),"name should not be empty");
      BOOST_CHECK_MESSAGE(rep.name() == "YMD","name not as expected");
      BOOST_CHECK_MESSAGE(rep.start() == 20090930,"Start should be 20090930");
      BOOST_CHECK_MESSAGE(rep.end() == 20090916,"end should be 20090916");
      BOOST_CHECK_MESSAGE(rep.step() == -1,"step should be -1");
      BOOST_CHECK_MESSAGE(rep.value() == 20090930,"value should be 20090930");
      BOOST_CHECK_MESSAGE(rep.last_valid_value() == 20090930,"last_valid_value should be 20090930");

      Repeat cloned = Repeat(rep);
      BOOST_CHECK_MESSAGE(cloned == rep,"Equality failed");
      BOOST_CHECK_MESSAGE(cloned.name() == "YMD","not as expected");
      BOOST_CHECK_MESSAGE(cloned.start() == 20090930,"not as expected");
      BOOST_CHECK_MESSAGE(cloned.end() == 20090916,"not as expected");
      BOOST_CHECK_MESSAGE(cloned.step() == -1,"not as expected");
      BOOST_CHECK_MESSAGE(cloned.value() == 20090930,"not as expected");
      BOOST_CHECK_MESSAGE(cloned.valueAsString() == "20090930","not as expected");
      BOOST_CHECK_MESSAGE(cloned.last_valid_value() == 20090930,"last_valid_value should be 20090930");
   }
   {
      Repeat rep(RepeatDate("YMD",20090916,20090916,1));
      BOOST_CHECK_MESSAGE(rep.start() == 20090916,"Start should be 20090916");
      BOOST_CHECK_MESSAGE(rep.end() == 20090916,"end should be 20090916");
      BOOST_CHECK_MESSAGE(rep.step() == 1,"step should be 1");
      BOOST_CHECK_MESSAGE(rep.value() == 20090916,"value should be 20090916");
      BOOST_CHECK_MESSAGE(rep.last_valid_value() == 20090916,"delta should be 20090916");
      rep.increment();
      BOOST_CHECK_MESSAGE(!rep.valid(),"RepeatDate should not be valid");
      BOOST_CHECK_MESSAGE(rep.value() == 20090917,"value should be 20090916");
      BOOST_CHECK_MESSAGE(rep.last_valid_value() == 20090916,"last_valid_value should be 20090916");
   }
   {
      Repeat rep(RepeatDate("YMD",20090916,20090916,-1));
      BOOST_CHECK_MESSAGE(rep.start() == 20090916,"Start should be 20090916");
      BOOST_CHECK_MESSAGE(rep.end() == 20090916,"end should be 20090916");
      BOOST_CHECK_MESSAGE(rep.step() == -1,"step should be -1");
      BOOST_CHECK_MESSAGE(rep.value() == 20090916,"value should be 20090916");
      BOOST_CHECK_MESSAGE(rep.last_valid_value() == 20090916,"last_valid_value should be 20090916");
      rep.increment();
      BOOST_CHECK_MESSAGE(!rep.valid(),"RepeatDate should not be valid");
      BOOST_CHECK_MESSAGE(rep.value() == 20090915,"value should be 20090915");
      BOOST_CHECK_MESSAGE(rep.last_valid_value() == 20090916,"last_valid_value should be 20090916");
   }


   {
      Repeat rep(RepeatEnumerated("AEnum",stringList));
      BOOST_CHECK_MESSAGE(!rep.empty()," Repeat should not be empty");
      BOOST_CHECK_MESSAGE(!rep.name().empty(),"name should not be empty");
      BOOST_CHECK_MESSAGE(rep.name() == "AEnum","name not as expected");

      Repeat cloned = Repeat(rep);
      BOOST_CHECK_MESSAGE(cloned == rep,"Equality failed");
      BOOST_CHECK_MESSAGE(cloned.name() == "AEnum","not as expected");
      BOOST_CHECK_MESSAGE(cloned.start() == 0,"not as expected");
      BOOST_CHECK_MESSAGE(cloned.step() == 1,"not as expected");
      BOOST_CHECK_MESSAGE(cloned.value() == 0,"not as expected");
      BOOST_CHECK_MESSAGE(cloned.valueAsString() == "a","not as expected");

      RepeatEnumerated empty;
      BOOST_CHECK_MESSAGE(empty.start() == 0,"Start should be 0");
      BOOST_CHECK_MESSAGE(empty.end() == 0,"end should be 0");
      BOOST_CHECK_MESSAGE(empty.step() == 1,"default step should be 1");
      BOOST_CHECK_MESSAGE(empty.value() == 0,"delta should be 0");
      BOOST_CHECK_MESSAGE(empty.name().empty(),"name should be empty");
      BOOST_CHECK_MESSAGE(empty.name() == "","name not as expected");
   }
   {
      Repeat rep(RepeatInteger("rep",0,100,1));
      BOOST_CHECK_MESSAGE(!rep.empty()," Repeat should not be empty");
      BOOST_CHECK_MESSAGE(!rep.name().empty(),"name should not be empty");
      BOOST_CHECK_MESSAGE(rep.name() == "rep","name not as expected");

      Repeat cloned = Repeat(rep);
      BOOST_CHECK_MESSAGE(cloned == rep,"Equality failed");
      BOOST_CHECK_MESSAGE(cloned.name() == "rep","not as expected");
      BOOST_CHECK_MESSAGE(cloned.start() == 0,"not as expected");
      BOOST_CHECK_MESSAGE(cloned.end() == 100,"not as expected");
      BOOST_CHECK_MESSAGE(cloned.step() == 1,"not as expected");
      BOOST_CHECK_MESSAGE(cloned.value() == 0,"not as expected");

      RepeatInteger empty;
      BOOST_CHECK_MESSAGE(empty.start() == 0,"Start should be 0");
      BOOST_CHECK_MESSAGE(empty.end() == 0,"end should be 0");
      BOOST_CHECK_MESSAGE(empty.step() == 0,"default step should be 0 but found" <<  empty.step());
      BOOST_CHECK_MESSAGE(empty.value() == 0,"delta should be 0");
      BOOST_CHECK_MESSAGE(empty.name().empty(),"name should be empty");
      BOOST_CHECK_MESSAGE(empty.name() == "","name not as expected");
   }
   {
      Repeat rep(RepeatDay(2));
      BOOST_CHECK_MESSAGE(!rep.empty()," Repeat should not be empty");
      BOOST_CHECK_MESSAGE(!rep.name().empty(),"name should not be empty");
      BOOST_CHECK_MESSAGE(rep.name() == "day","name not as expected");

      Repeat cloned = Repeat(rep);
      BOOST_CHECK_MESSAGE(cloned == rep,"Equality failed");
      BOOST_CHECK_MESSAGE(cloned.name() == "day","name not as expected");
      BOOST_CHECK_MESSAGE(cloned.step() == 2,"step not as expected");

      RepeatDay empty;
      BOOST_CHECK_MESSAGE(empty.start() == 0,"Start should be 0");
      BOOST_CHECK_MESSAGE(empty.end() == 0,"end should be 0");
      BOOST_CHECK_MESSAGE(empty.step() == 1,"default step should be 0 but found " <<  empty.step());
      BOOST_CHECK_MESSAGE(empty.value() == 1,"value should be 0 but found " << empty.value());
      BOOST_CHECK_MESSAGE(empty.name().empty(),"name should be empty");
      BOOST_CHECK_MESSAGE(empty.name() == "","name not as expected");
   }
}

BOOST_AUTO_TEST_CASE( test_repeat )
{
	cout << "ANattr:: ...test_repeat \n";

	std::vector<std::string> stringList; stringList.reserve(3);
	stringList.push_back("a");
	stringList.push_back("b");
	stringList.push_back("c");

	Repeat empty;
	Repeat empty2;

	{
 		Repeat l1(RepeatDate("YMD",20090916,20090930,1));
		Repeat l2(RepeatDate("YMD",20090916,20090930,1));
 		BOOST_CHECK_MESSAGE(!l1.empty(),"Construction failed");
 		BOOST_CHECK_MESSAGE(!l2.empty(),"Construction failed");
 		BOOST_CHECK_MESSAGE(l1 == l2,"Equality failed");
 		BOOST_CHECK_MESSAGE(!(l1 == empty),"Equality failed");

      Repeat l1a(RepeatDate("YMD",20090930,20090916,-1));
      Repeat l2a(RepeatDate("YMD",20090930,20090916,-1));
      BOOST_CHECK_MESSAGE(!l1a.empty(),"Construction failed");
      BOOST_CHECK_MESSAGE(!l2a.empty(),"Construction failed");
      BOOST_CHECK_MESSAGE(l1a == l2a,"Equality failed");
      BOOST_CHECK_MESSAGE(!(l1a == empty),"Equality failed");

 		Repeat la(RepeatEnumerated("AEnum",stringList));
		Repeat lb(RepeatEnumerated("AEnum",stringList));
		BOOST_CHECK_MESSAGE(!la.empty(),"Construction failed");
 		BOOST_CHECK_MESSAGE(!lb.empty(),"Construction failed");
 		BOOST_CHECK_MESSAGE(la == lb,"Equality failed");
 		BOOST_CHECK_MESSAGE(!(la == empty),"Equality failed");

 		Repeat lc(RepeatString("RepeatString",stringList));
		Repeat ld(RepeatString("RepeatString",stringList));
		BOOST_CHECK_MESSAGE(!lc.empty(),"Construction failed");
 		BOOST_CHECK_MESSAGE(!ld.empty(),"Construction failed");
 		BOOST_CHECK_MESSAGE(lc == lc,"Equality failed");
 		BOOST_CHECK_MESSAGE(!(lc == empty),"Equality failed");

 		Repeat le(RepeatInteger("rep",0,100,1));
		Repeat lf(RepeatInteger("rep",0,100,1));
		BOOST_CHECK_MESSAGE(!le.empty(),"Construction failed");
 		BOOST_CHECK_MESSAGE(!lf.empty(),"Construction failed");
 		BOOST_CHECK_MESSAGE(le == lf,"Equality failed");
 		BOOST_CHECK_MESSAGE(!(le == empty),"Equality failed");

		l1.clear();
 		l2.clear();
 		la.clear();
 		lb.clear();
 		lc.clear();
 		ld.clear();
 		le.clear();
 		lf.clear();
 		BOOST_CHECK_MESSAGE(l1 == empty,"Clear failed");
 		BOOST_CHECK_MESSAGE(l2 == empty,"Clear failed");
 		BOOST_CHECK_MESSAGE(la == empty,"Clear failed");
 		BOOST_CHECK_MESSAGE(lb == empty,"Clear failed");
 		BOOST_CHECK_MESSAGE(lc == empty,"Clear failed");
 		BOOST_CHECK_MESSAGE(ld == empty,"Clear failed");
 		BOOST_CHECK_MESSAGE(le == empty,"Clear failed");
 		BOOST_CHECK_MESSAGE(lf == empty,"Clear failed");
	}

	{
 		Repeat l1(RepeatDate("YMD",20090916,20090930,1));
		Repeat l2;
		l2 = l1;
 		BOOST_CHECK_MESSAGE(l1 == l2,"Assignment failed");

 		l2 = empty;
 		BOOST_CHECK_MESSAGE(l2 == empty,"Assignment failed");

 		Repeat la(RepeatEnumerated("AEnum",stringList));
 		Repeat lb;
 		lb = la;
 		BOOST_CHECK_MESSAGE(la == lb,"Assignment failed");

 		Repeat lc(RepeatString("RepeatString",stringList));
 		Repeat ld;
 		ld = lc;
 		BOOST_CHECK_MESSAGE(lc == ld,"Assignment failed");

 		Repeat le(RepeatInteger("rep",0,100,1));
		Repeat lf;
		lf = le;
 		BOOST_CHECK_MESSAGE(le == lf,"Assignment failed");
 	}

	{
		Repeat l1(RepeatDate("YMD",20090916,20090930,1));

		Repeat l2 = l1;
  		BOOST_CHECK_MESSAGE(l1 == l2,"Copy construction failed");

 		Repeat la(RepeatEnumerated("AEnum",stringList));
 		Repeat lb = la;
 		BOOST_CHECK_MESSAGE(la == lb,"Copy construction failed");

 		Repeat lc(RepeatString("RepeatString",stringList));
 		Repeat ld = lc;
 		BOOST_CHECK_MESSAGE(lc == ld,"Copy construction failed");

 		Repeat le(RepeatInteger("rep",0,100,1));
		Repeat lf = le;
 		BOOST_CHECK_MESSAGE(le == lf,"Copy construction failed");

		Repeat empty_1( empty );
 		BOOST_CHECK_MESSAGE(empty_1 == empty,"Copy construction failed");
 	}

	{
 		BOOST_CHECK_MESSAGE(empty.name() == string()," empty  failed");
 		BOOST_CHECK_MESSAGE(empty.valid() == false," empty  failed");
 		BOOST_CHECK_MESSAGE(empty.value() == 0," empty  failed");
 		empty.setToLastValue();
 		BOOST_CHECK_MESSAGE(empty.valueAsString() == string()," empty  failed");
 		empty.reset();
 		empty.increment();
 		empty.change("fred");
 		BOOST_CHECK_MESSAGE(empty.valueAsString() == string()," empty  failed");
 		empty.changeValue(10);
 		BOOST_CHECK_MESSAGE(empty.valueAsString() == string()," empty  failed");
 		BOOST_CHECK_MESSAGE(empty.isInfinite() == false," empty  failed");
 		BOOST_CHECK_MESSAGE(empty.toString() == string()," empty  failed");
 		BOOST_CHECK_MESSAGE(empty.state_change_no() ==0," empty  failed");
	}

	{
 		Repeat day(RepeatDay(2));
 		Repeat day2 = day;
      BOOST_CHECK_MESSAGE(day == day2,"Copy construction failed");

		day.reset();
 		BOOST_CHECK_MESSAGE(day.valid(),"Should return true after reset");
	}
}


BOOST_AUTO_TEST_CASE( test_repeat_last_value )
{
   cout << "ANattr:: ...test_repeat_last_value \n";

   {
      Repeat rep(RepeatDate("YMD",20090916,20090930,1));
      rep.setToLastValue();
      BOOST_CHECK_MESSAGE(rep.value() == 20090930,"Set to last value did not work, expected 20090930 but found " << rep.value());
   }
   {
      Repeat rep(RepeatDate("YMD",20090930,20090916,-1));
      rep.setToLastValue();
      BOOST_CHECK_MESSAGE(rep.value() == 20090916,"Set to last value did not work, expected 20090916 but found " << rep.value());
   }

   std::vector<std::string> stringList; stringList.reserve(3);
   stringList.push_back("a");
   stringList.push_back("b");
   stringList.push_back("c");
   {
      Repeat rep(RepeatEnumerated("AEnum",stringList));
      rep.setToLastValue();
      BOOST_CHECK_MESSAGE(rep.value() == 2,"Set to last value did not work, expected 2 but found " << rep.value());
      BOOST_CHECK_MESSAGE(rep.valueAsString() == "c","Set to last value did not work, expected 'c' but found " << rep.valueAsString());
      BOOST_CHECK_MESSAGE(rep.value_as_string(0) == "a"," Expected 'a' but found " << rep.value_as_string(0));
      BOOST_CHECK_MESSAGE(rep.value_as_string(1) == "b"," Expected 'b' but found " << rep.value_as_string(1));
      BOOST_CHECK_MESSAGE(rep.value_as_string(2) == "c"," Expected 'c' but found " << rep.value_as_string(2));
   }
   {
      Repeat rep(RepeatString("Str",stringList));
      rep.setToLastValue();
      BOOST_CHECK_MESSAGE(rep.value() == 2,"Set to last value did not work, expected 2 but found " << rep.value());
      BOOST_CHECK_MESSAGE(rep.valueAsString() == "c","Set to last value did not work, expected 'c' but found " << rep.valueAsString());
      BOOST_CHECK_MESSAGE(rep.value_as_string(0) == "a"," Expected 'a' but found " << rep.value_as_string(0));
      BOOST_CHECK_MESSAGE(rep.value_as_string(1) == "b"," Expected 'b' but found " << rep.value_as_string(1));
      BOOST_CHECK_MESSAGE(rep.value_as_string(2) == "c"," Expected 'c' but found " << rep.value_as_string(2));
   }

   {
      Repeat rep(RepeatInteger("integer",0,10,1));
      rep.setToLastValue();
      BOOST_CHECK_MESSAGE(rep.value() == 10,"Set to last value did not work, expected 10 but found " << rep.value());
      BOOST_CHECK_MESSAGE(rep.value_as_string(0) == "0"," Expected '0' but found " << rep.value_as_string(0));
      BOOST_CHECK_MESSAGE(rep.value_as_string(1) == "1"," Expected '1' but found " << rep.value_as_string(1));
      BOOST_CHECK_MESSAGE(rep.value_as_string(2) == "2"," Expected '2' but found " << rep.value_as_string(2));
   }
   {
      Repeat rep(RepeatInteger("integer",10,0,-1));
      rep.setToLastValue();
      BOOST_CHECK_MESSAGE(rep.value() == 0,"Set to last value did not work, expected 0 but found " << rep.value());
      BOOST_CHECK_MESSAGE(rep.value_as_string(0) == "0"," Expected '0' but found " << rep.value_as_string(0));
      BOOST_CHECK_MESSAGE(rep.value_as_string(1) == "1"," Expected '1' but found " << rep.value_as_string(1));
      BOOST_CHECK_MESSAGE(rep.value_as_string(2) == "2"," Expected '2' but found " << rep.value_as_string(2));
   }
}

BOOST_AUTO_TEST_CASE( test_repeat_enumerated_as_string_integers )
{
   cout << "ANattr:: ...test_repeat_enumerated_as_string_integers\n";

   std::vector<std::string> stringList; stringList.reserve(3);
   stringList.push_back("20130101");
   stringList.push_back("20130102");
   stringList.push_back("20130103");
   {
      Repeat rep(RepeatEnumerated("AEnum",stringList));
      // Note: valueAsString should return string at the last valid index

      BOOST_CHECK_MESSAGE(rep.valid(),"Expected rep to be valid");
      BOOST_CHECK_MESSAGE(rep.value() == 20130101," Expected 20130101 but found " << rep.value());
      BOOST_CHECK_MESSAGE(rep.last_valid_value() == 20130101," Expected 20130101 but found " << rep.last_valid_value());
      BOOST_CHECK_MESSAGE(rep.valueAsString() == "20130101"," Expected '20130101' but found " << rep.valueAsString());
      BOOST_CHECK_MESSAGE(rep.value_as_string(0) == "20130101"," Expected '20130101' but found " << rep.value_as_string(0));

      rep.increment();
      BOOST_CHECK_MESSAGE(rep.valid(),"Expected rep to be valid");
      BOOST_CHECK_MESSAGE(rep.value() == 20130102," Expected 20130102 but found " << rep.value());
      BOOST_CHECK_MESSAGE(rep.last_valid_value() == 20130102," Expected 20130102 but found " << rep.last_valid_value());
      BOOST_CHECK_MESSAGE(rep.valueAsString() == "20130102"," Expected '20130102' but found " << rep.valueAsString());
      BOOST_CHECK_MESSAGE(rep.value_as_string(1) == "20130102"," Expected '20130102' but found " << rep.value_as_string(1));

      rep.increment();
      BOOST_CHECK_MESSAGE(rep.value() == 20130103," Expected 20130103 but found " << rep.value());
      BOOST_CHECK_MESSAGE(rep.last_valid_value() == 20130103," Expected 20130103 but found " << rep.last_valid_value());
      BOOST_CHECK_MESSAGE(rep.valueAsString() == "20130103"," Expected '20130103' but found " << rep.valueAsString());
      BOOST_CHECK_MESSAGE(rep.value_as_string(2) == "20130103"," Expected '20130103' but found " << rep.value_as_string(2));

      rep.increment();
      BOOST_CHECK_MESSAGE(!rep.valid(),"Expected rep to be in-valid");
      BOOST_CHECK_MESSAGE(rep.last_valid_value() == 20130103," Expected 20130103 but found " << rep.last_valid_value());
      BOOST_CHECK_MESSAGE(rep.valueAsString() == "20130103"," Expected '20130103' but found " << rep.valueAsString());

      rep.increment();
      BOOST_CHECK_MESSAGE(!rep.valid(),"Expected rep to be in-valid");
      BOOST_CHECK_MESSAGE(rep.last_valid_value() == 20130103," Expected 20130103 but found " << rep.last_valid_value());
      BOOST_CHECK_MESSAGE(rep.valueAsString() == "20130103"," Expected '20130103' but found " << rep.valueAsString());
   }
}

BOOST_AUTO_TEST_CASE( test_repeat_increment )
{
   cout << "ANattr:: ...test_repeat_increment \n";

   {
      Repeat rep(RepeatDate("YMD",20090916,20090920,1));
      while( rep.valid()) { rep.increment(); }
      BOOST_CHECK_MESSAGE(rep.value() == 20090921,"expected 20090921 but found " << rep.value());
      BOOST_CHECK_MESSAGE(rep.last_valid_value() == 20090920,"expected 20090920 but found " << rep.last_valid_value());
   }
   {
      Repeat rep(RepeatDate("YMD",20090920,20090916,-1));
      while( rep.valid()) { rep.increment(); }
      BOOST_CHECK_MESSAGE(rep.value() == 20090915,"expected 20090915 but found " << rep.value());
      BOOST_CHECK_MESSAGE(rep.last_valid_value() == 20090916,"expected 20090916 but found " << rep.last_valid_value());
   }

   {
      Repeat rep(RepeatDate("YMD",20150514,20150730,7));
      while( rep.valid()) { rep.increment();
         //cout << "YMD: " << rep.value() << "\n";
      }
      BOOST_CHECK_MESSAGE(rep.value() == 20150806,"expected 20150806 but found " << rep.value());
      BOOST_CHECK_MESSAGE(rep.last_valid_value() == 20150730,"expected 20150730 but found " << rep.last_valid_value());
   }
   {
      Repeat rep(RepeatDate("YMD",20150730,20150514,-7));
      while( rep.valid()) {
          rep.increment();
          //cout << "YMD: " << rep.value() << "\n";
      }
      BOOST_CHECK_MESSAGE(rep.value() == 20150507,"expected 20150507 but found " << rep.value());
      BOOST_CHECK_MESSAGE(rep.last_valid_value() == 20150514,"expected 20150514 but found " << rep.last_valid_value());
   }

   std::vector<std::string> stringList; stringList.reserve(3);
   stringList.push_back("a");
   stringList.push_back("b");
   stringList.push_back("c");
   {
      Repeat rep(RepeatEnumerated("AEnum",stringList));
      while( rep.valid()) { rep.increment(); }
      BOOST_CHECK_MESSAGE(rep.value() == 3," Expected 3 but found " << rep.value());
      BOOST_CHECK_MESSAGE(rep.last_valid_value() == 2," Expected 2 but found " << rep.last_valid_value());
   }
   {
      Repeat rep(RepeatString("Str",stringList));
      while( rep.valid()) { rep.increment(); }
      BOOST_CHECK_MESSAGE(rep.value() == 3," Expected 3 but found " << rep.value());
      BOOST_CHECK_MESSAGE(rep.last_valid_value() == 2," Expected 2 but found " << rep.last_valid_value());
   }
   {
      Repeat rep(RepeatInteger("integer",0,10,1));
      while( rep.valid()) { rep.increment(); }
      BOOST_CHECK_MESSAGE(rep.value() == 11," Expected 11 but found " << rep.value());
      BOOST_CHECK_MESSAGE(rep.last_valid_value() == 10," Expected 10 but found " << rep.last_valid_value());
   }
}

BOOST_AUTO_TEST_CASE( test_repeat_date_change_value )
{
   cout << "ANattr:: ...test_repeat_date_change_value \n";
   {
      Repeat rep2(RepeatDate("YMD",20150514,20150730,7));
      Repeat rep(RepeatDate("YMD",20150514,20150730,7));
      BOOST_CHECK_MESSAGE(rep.valid(),"expected valid at start ");

      while( rep.valid() ) {
         rep2.change(boost::lexical_cast<std::string>(rep.value()));
         BOOST_CHECK_MESSAGE(rep.value() == rep2.value(),"expected same value, but found " << rep.value() << "  " << rep2.value());
         rep.increment();
      }
   }
   {
      Repeat rep2(RepeatDate("YMD",20150730,20150514,-7));
      Repeat rep(RepeatDate("YMD",20150730,20150514,-7));
      BOOST_CHECK_MESSAGE(rep.valid(),"expected valid at start ");

      while( rep.valid() ) {
         rep2.change(boost::lexical_cast<std::string>(rep.value()));
         BOOST_CHECK_MESSAGE(rep.value() == rep2.value(),"expected same value, but found " << rep.value() << "  " << rep2.value());
         rep.increment();
      }
   }
}


BOOST_AUTO_TEST_CASE( test_repeat_date_errors )
{
   cout << "ANattr:: ...test_repeat_date_errors \n";

   std::vector<std::string> empty;
   std::vector<std::string> stringList;stringList.push_back("a");stringList.push_back("b");
   BOOST_REQUIRE_THROW( RepeatEnumerated("",stringList),std::runtime_error); // empty name
   BOOST_REQUIRE_THROW( RepeatEnumerated(" ",stringList),std::runtime_error); // empty name
   BOOST_REQUIRE_THROW( RepeatEnumerated("*",stringList),std::runtime_error); // illegal name
   BOOST_REQUIRE_THROW( RepeatString("",stringList),std::runtime_error);      // empty name
   BOOST_REQUIRE_THROW( RepeatString(" ",stringList),std::runtime_error);     // empty name
   BOOST_REQUIRE_THROW( RepeatString("!£$%^&*()",stringList),std::runtime_error);     // illegal name
   BOOST_REQUIRE_THROW( RepeatEnumerated("AEnum",empty),std::runtime_error); // empty enumerations
   BOOST_REQUIRE_THROW( RepeatString("AEnum",empty),std::runtime_error);     // empty string list


   BOOST_REQUIRE_THROW( RepeatDate("",20090916,20090920,1),std::runtime_error);
   BOOST_REQUIRE_THROW( RepeatDate("YMD",200909161,20090920,1),std::runtime_error); // start > 8
   BOOST_REQUIRE_THROW( RepeatDate("YMD",20090916,200909201,1),std::runtime_error); // end > 8
   BOOST_REQUIRE_THROW( RepeatDate("YMD",20090016,200909201,1),std::runtime_error); // invalid start month
   BOOST_REQUIRE_THROW( RepeatDate("YMD",20090900,20090920,1),std::runtime_error);  // invalid start day
   BOOST_REQUIRE_THROW( RepeatDate("YMD",20090916,20090020,1),std::runtime_error);  // invalid end month
   BOOST_REQUIRE_THROW( RepeatDate("YMD",20090916,20090900,1),std::runtime_error);  // invalid end day
   BOOST_REQUIRE_THROW( RepeatDate("YMD",20090916,20090920,0),std::runtime_error);  // delta can not be zero

   BOOST_REQUIRE_THROW( RepeatDate("YMD",20090920,20090916,1),std::runtime_error);  //  start day > end day, and delta > 0
   BOOST_REQUIRE_THROW( RepeatDate("YMD",20090916,20090920,-1),std::runtime_error);  //  start day < end day, and delta < 0


   RepeatDate date("YMD",20150514,20150730,7);
   BOOST_REQUIRE_THROW( date.changeValue(20150513),std::runtime_error);   // outside of range
   BOOST_REQUIRE_THROW( date.changeValue(20150731),std::runtime_error);   // outside of range
   BOOST_REQUIRE_THROW( date.changeValue(20150801),std::runtime_error);   // outside of range

   BOOST_REQUIRE_THROW( date.changeValue(20150515),std::runtime_error);   // not a valid step
   BOOST_REQUIRE_THROW( date.changeValue(20150516),std::runtime_error);   // not a valid step
   BOOST_REQUIRE_THROW( date.changeValue(20150517),std::runtime_error);   // not a valid step
   BOOST_REQUIRE_THROW( date.changeValue(20150518),std::runtime_error);   // not a valid step
   BOOST_REQUIRE_THROW( date.changeValue(20150519),std::runtime_error);   // not a valid step
   BOOST_REQUIRE_THROW( date.changeValue(20150520),std::runtime_error);   // not a valid step
   BOOST_REQUIRE_THROW( date.changeValue(20150522),std::runtime_error);   // not a valid step

   RepeatDate date1("YMD",20150730,20150514,-7);
   BOOST_REQUIRE_THROW( date1.changeValue(20150731),std::runtime_error);   // outside of range
   BOOST_REQUIRE_THROW( date1.changeValue(20150813),std::runtime_error);   // outside of range
   BOOST_REQUIRE_THROW( date1.changeValue(20150513),std::runtime_error);   // outside of range
   BOOST_REQUIRE_THROW( date1.changeValue(20150413),std::runtime_error);   // outside of range

   BOOST_REQUIRE_THROW( date.changeValue(20150729),std::runtime_error);    // not a valid step
   BOOST_REQUIRE_THROW( date.changeValue(20150728),std::runtime_error);    // not a valid step
   BOOST_REQUIRE_THROW( date.changeValue(20150515),std::runtime_error);    // not a valid step
}

static void check_date(int start, int end, int delta)
{
   boost::gregorian::date bdate(from_undelimited_string(boost::lexical_cast<std::string>(start)));

   Repeat rep(RepeatDate("YMD",start,end,delta));
   Repeat rep2(RepeatDate("YMD",start,end,delta));
   while (rep.valid()) {

      // xref repeat date with boost date, essentially checking bdate with rep
      string str_value = boost::lexical_cast<std::string>(rep.value());
      boost::gregorian::date date2(from_undelimited_string( str_value ));
      BOOST_CHECK_MESSAGE(bdate == date2 ,"expected same value, but found " << bdate << "  " << date2);

      // check change value
      rep2.change(str_value);
      BOOST_CHECK_MESSAGE(rep.value() == rep2.value(),"expected same value, but found " << rep.value() << "  " << rep2.value());

      // increment repeat and boost date
      rep.increment();
      bdate += days(delta);
   }
}

BOOST_AUTO_TEST_CASE( test_repeat_date_xref_to_boost_date )
{
   cout << "ANattr:: ...test_repeat_date_xref_to_boost_date \n";

   check_date(19800101,20621231,1);
   check_date(19800101,20621231,7);
   check_date(20621231,19800101,-7);
   check_date(20150514,20150730,7);
}

BOOST_AUTO_TEST_CASE( test_repeat_date_generated_variables )
{
   cout << "ANattr:: ...test_repeat_date_generated_variables\n";

    Repeat rep(RepeatDate("YMD",20090916,20090930,1));
    BOOST_CHECK_MESSAGE(!rep.empty()," Repeat should not be empty");
    BOOST_CHECK_MESSAGE(!rep.name().empty(),"name should not be empty");
    BOOST_CHECK_MESSAGE(rep.name() == "YMD","name not as expected");
    BOOST_CHECK_MESSAGE(rep.start() == 20090916,"Start should be 20090916");
    BOOST_CHECK_MESSAGE(rep.end() == 20090930,"end should be 20090930");
    BOOST_CHECK_MESSAGE(rep.step() == 1,"step should be 1");
    BOOST_CHECK_MESSAGE(rep.value() == 20090916,"value should be 20090916");
    BOOST_CHECK_MESSAGE(rep.last_valid_value() == 20090916,"last_valid_value should be 20090916");
    rep.update_repeat_genvar();
    std::vector<Variable> vec;
    rep.gen_variables(vec);
    BOOST_CHECK_MESSAGE(vec.size() == 6,"expected 6 generated variables but found " << vec.size());

    {const Variable& var = rep.find_gen_variable("YMD");
    BOOST_CHECK_MESSAGE(!var.empty(),"Did not find generated variable YMD_YYYY");
    BOOST_CHECK_MESSAGE(var.theValue() == "20090916", "expected year to be 20090916  but found " << var.theValue() );}

    {const Variable& var = rep.find_gen_variable("YMD_YYYY");
    BOOST_CHECK_MESSAGE(!var.empty(),"Did not find generated variable YMD_YYYY");
    BOOST_CHECK_MESSAGE(var.theValue() == "2009", "expected year to be 2009  but found " << var.theValue() );}

    {const Variable& var = rep.find_gen_variable("YMD_MM");
    BOOST_CHECK_MESSAGE(!var.empty(),"Did not find generated variable YMD_MM");
    BOOST_CHECK_MESSAGE(var.theValue() == "9", "expected month to be 9  but found " << var.theValue() );}

    {const Variable& var = rep.find_gen_variable("YMD_DD");
    BOOST_CHECK_MESSAGE(!var.empty(),"Did not find generated variable YMD_DD");
    BOOST_CHECK_MESSAGE(var.theValue() == "16", "expected day of month to be 16 but found " << var.theValue() );}

    {const Variable& var = rep.find_gen_variable("YMD_DOW");
    BOOST_CHECK_MESSAGE(!var.empty(),"Did not find generated variable YMD_DOW");
    BOOST_CHECK_MESSAGE(var.theValue() == "3", "expected day of week to be 3 but found " << var.theValue() );}

    {const Variable& var = rep.find_gen_variable("YMD_JULIAN");
    BOOST_CHECK_MESSAGE(!var.empty(),"Did not find generated variable YMD_JULIAN");
    std::string expected = boost::lexical_cast<std::string>(Cal::date_to_julian(20090916));
    BOOST_CHECK_MESSAGE(var.theValue() == expected, "expected " << expected << " but found " << var.theValue() );}
}

BOOST_AUTO_TEST_CASE( test_repeat_date_generated_variables2 )
{
   cout << "ANattr:: ...test_repeat_date_generated_variables\n";

   int start = 20161231;
   int end = 20170106;
   Repeat rep(RepeatDate("YMD",start,end,1));
   BOOST_CHECK_MESSAGE(!rep.empty()," Repeat should not be empty");
   BOOST_CHECK_MESSAGE(!rep.name().empty(),"name should not be empty");
   BOOST_CHECK_MESSAGE(rep.name() == "YMD","name not as expected");
   BOOST_CHECK_MESSAGE(rep.start() == start ,"Start should be " << start);
   BOOST_CHECK_MESSAGE(rep.end() == end ,"end should be " << end << " but found " << rep.end());
   BOOST_CHECK_MESSAGE(rep.step() == 1,"step should be 1");
   BOOST_CHECK_MESSAGE(rep.value() == start,"value should be " << start << " but found " << rep.value());
   BOOST_CHECK_MESSAGE(rep.last_valid_value() == start,"last_valid_value should be " << start << " but found " << rep.last_valid_value());

   std::vector<std::string> expected_YMD;
   expected_YMD.push_back("20161231");
   expected_YMD.push_back("20170101");
   expected_YMD.push_back("20170102");
   expected_YMD.push_back("20170103");
   expected_YMD.push_back("20170104");
   expected_YMD.push_back("20170105");
   expected_YMD.push_back("20170106");

   std::vector<std::string> expected_year;
   expected_year.push_back("2016");
   expected_year.push_back("2017");
   expected_year.push_back("2017");
   expected_year.push_back("2017");
   expected_year.push_back("2017");
   expected_year.push_back("2017");
   expected_year.push_back("2017");

   std::vector<std::string> expected_MM;
   expected_MM.push_back("12");
   expected_MM.push_back("1");
   expected_MM.push_back("1");
   expected_MM.push_back("1");
   expected_MM.push_back("1");
   expected_MM.push_back("1");
   expected_MM.push_back("1");

   std::vector<std::string> expected_day_of_month;
   expected_day_of_month.push_back("31");
   expected_day_of_month.push_back("1");
   expected_day_of_month.push_back("2");
   expected_day_of_month.push_back("3");
   expected_day_of_month.push_back("4");
   expected_day_of_month.push_back("5");
   expected_day_of_month.push_back("6");

   std::vector<std::string> expected_day_of_week;
   expected_day_of_week.push_back("6");
   expected_day_of_week.push_back("0");
   expected_day_of_week.push_back("1");
   expected_day_of_week.push_back("2");
   expected_day_of_week.push_back("3");
   expected_day_of_week.push_back("4");
   expected_day_of_week.push_back("5");

   std::vector<std::string> expected_julian;
   expected_julian.push_back(boost::lexical_cast<std::string>(Cal::date_to_julian(20161231)));
   expected_julian.push_back(boost::lexical_cast<std::string>(Cal::date_to_julian(20170101)));
   expected_julian.push_back(boost::lexical_cast<std::string>(Cal::date_to_julian(20170102)));
   expected_julian.push_back(boost::lexical_cast<std::string>(Cal::date_to_julian(20170103)));
   expected_julian.push_back(boost::lexical_cast<std::string>(Cal::date_to_julian(20170104)));
   expected_julian.push_back(boost::lexical_cast<std::string>(Cal::date_to_julian(20170105)));
   expected_julian.push_back(boost::lexical_cast<std::string>(Cal::date_to_julian(20170106)));

   for(int i = 0; i < 7; i++) {

      rep.update_repeat_genvar();
      std::vector<Variable> vec;
      rep.gen_variables(vec);
      BOOST_CHECK_MESSAGE(vec.size() == 6,"expected 6 generated variables but found " << vec.size());

      {const Variable& var = rep.find_gen_variable("YMD");
      BOOST_CHECK_MESSAGE(!var.empty(),"Did not find generated variable YMD_YYYY");
      BOOST_CHECK_MESSAGE(var.theValue() == expected_YMD[i], "expected YMD " << expected_YMD[i] << " but found " << var.theValue() );}

      {const Variable& var = rep.find_gen_variable("YMD_YYYY");
      BOOST_CHECK_MESSAGE(!var.empty(),"Did not find generated variable YMD_YYYY");
      BOOST_CHECK_MESSAGE(var.theValue() == expected_year[i], "expected year to be " << expected_year[i] << " but found " << var.theValue() );}

      {const Variable& var = rep.find_gen_variable("YMD_MM");
      BOOST_CHECK_MESSAGE(!var.empty(),"Did not find generated variable YMD_MM");
      BOOST_CHECK_MESSAGE(var.theValue() == expected_MM[i], "expected month to be " << expected_MM[i] << " but found " << var.theValue() );}

      {const Variable& var = rep.find_gen_variable("YMD_DD");
      BOOST_CHECK_MESSAGE(!var.empty(),"Did not find generated variable YMD_DD");
      BOOST_CHECK_MESSAGE(var.theValue() == expected_day_of_month[i], "expected day of month to be " << expected_day_of_month[i] << " but found " << var.theValue() );}

      {const Variable& var = rep.find_gen_variable("YMD_DOW");
      BOOST_CHECK_MESSAGE(!var.empty(),"Did not find generated variable YMD_DOW");
      BOOST_CHECK_MESSAGE(var.theValue() == expected_day_of_week[i], "expected day of week to be " << expected_day_of_week[i] << " but found " << var.theValue() );}

      {const Variable& var = rep.find_gen_variable("YMD_JULIAN");
      BOOST_CHECK_MESSAGE(!var.empty(),"Did not find generated variable YMD_JULIAN");
      BOOST_CHECK_MESSAGE(var.theValue() == expected_julian[i], "expected " <<  expected_julian[i] << " but found " << var.theValue() );}

      rep.increment();
   }
}

BOOST_AUTO_TEST_SUITE_END()

