//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #24 $
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
//============================================================================
#include <iostream>
#include <fstream>

#include <boost/test/unit_test.hpp>
#include <boost/timer.hpp>
#include <boost/foreach.hpp>

#include "Str.hpp"
#include "StringSplitter.hpp"
#include "File.hpp"

using namespace std;
using namespace ecf;
using namespace boost;

// **************************************************************************************************************
// IMPORTANT: Str:split uses StringSplitter:
//            to run these tests, please switch off Str.cpp::USE_STRINGSPLITTER
// **************************************************************************************************************
//#define STRING_SPLIT_IMPLEMENTATIONS_PERF_CHECK_ 1;


BOOST_AUTO_TEST_SUITE( CoreTestSuite )

#ifdef STRING_SPLIT_IMPLEMENTATIONS_PERF_CHECK_
BOOST_AUTO_TEST_CASE( test_str_split_perf )
{
   cout << "ACore:: ...test_str_split_perf\n";
   // Method:              time
   // boost::split:        4.34
   // Str::split:          2.47
   // make_split_iterator  4.13
   // boost::string_view    1.86
   std::vector<std::string> result;
   std::string line = "This is a long string that is going to be used to test the performance of splitting with different Implementations   extra   empty tokens   ";
   size_t times = 1000000;
   cout << "This test will split a line " << times << " times: '" << line << "'\n";

   { // BOOST SPLIT
      boost::timer timer;  // measures CPU, replace with cpu_timer with boost > 1.51, measures cpu & elapsed
      for (size_t i = 0; i < times; i++) {
         result.clear();
         boost::algorithm::split(result, line, boost::is_any_of(" \t"),boost::algorithm::token_compress_on);

         string reconstructed;
         BOOST_FOREACH(const std::string& s, result) { reconstructed += s;  reconstructed += " "; }
         //cout << "boost::split: reconstructed " << reconstructed << "\n";
      }
      cout << "Time for boost::split " << times << " times = " << timer.elapsed() << "\n";
   }

   { // Str::split
      boost::timer timer;
      for (size_t i = 0; i < times; i++) {
         result.clear(); Str::split(line,result);

         string reconstructed;
         BOOST_FOREACH(const std::string& s, result) { reconstructed += s;  reconstructed += " "; }
         //cout << "Str::split reconstructed " << reconstructed << "\n";
      }
      cout << "Time for Str::split " << times << " times = " << timer.elapsed() << "\n";
   }

   typedef boost::split_iterator<string::const_iterator> split_iter_t;
   { // boost::make_split_iterator
      boost::timer timer;
      for (size_t i = 0; i < times; i++) {

         split_iter_t tokens = Str::make_split_iterator(line);

         std::stringstream ss;
         for(; !tokens.eof(); ++tokens ) {
             boost::iterator_range<string::const_iterator> range = *tokens;
             ss << range << " ";
         }
         string reconstructed = ss.str();
         //cout << "boost::make_split_iterator: reconstructed " << reconstructed << "\n";
      }
      cout << "Time for make_split_iterator::split " << times << " times = " << timer.elapsed() << "\n";
   }

   { // boost::string_view
      boost::timer timer;
      for (size_t i = 0; i < times; i++) {
         StringSplitter string_splitter(line);

         string reconstructed;
         while(!string_splitter.finished())  {
            reconstructed += std::string(string_splitter.next());
            reconstructed += " ";
         }
         //cout << "StringSplitter:: reconstructed " << reconstructed << "\n";
      }
      cout << "Time for boost::string_view " << times << " times = " << timer.elapsed() << "\n";
   }
}

BOOST_AUTO_TEST_CASE( test_str_split_perf_with_file )
{
   cout << "ACore:: ...test_str_split_perf_with_file\n";
//   Time for boost::split 2001774 times =               3.71848
//   Time for Str::split 2001774 times =                 2.06476
//   Time for boost::make_split_iterator 2001774 times = 4.24187
//   Time for boost::string_view 2001774 times =           1.66037

   // Now test performance of splitting with a big DEFS file
   std::string path = "/var/tmp/ma0/BIG_DEFS/vsms2.31415.def";
   std::vector<std::string> file_contents;
   if (File::splitFileIntoLines(path,file_contents,true/* ignore empty lines*/)) {

      cout << "This test will split each line in file " << path << "\n";

      std::vector<std::string> result;result.reserve(300);
      {
         boost::timer timer;
         for(size_t i = 0; i < file_contents.size(); i++) {
            result.clear();
            boost::algorithm::split(result,file_contents[i], boost::is_any_of(" \t"),boost::algorithm::token_compress_on);

            string reconstructed;
            BOOST_FOREACH(const std::string& s, result) { reconstructed += s;  reconstructed += " "; }
         }
         cout << "Time for boost::split " << file_contents.size() << " times = " << timer.elapsed() << "\n";
      }
      {
         boost::timer timer;
         for(size_t i = 0; i < file_contents.size(); i++) {
            result.clear();
            Str::split(file_contents[i],result);

            string reconstructed;
            BOOST_FOREACH(const std::string& s, result) { reconstructed += s;  reconstructed += " "; }
         }
         cout << "Time for Str::split " << file_contents.size() << " times = " << timer.elapsed() << "\n";
      }
      {
         typedef boost::split_iterator<string::const_iterator> split_iter_t;
         boost::timer timer;
         for(size_t i = 0; i < file_contents.size(); i++) {

            std::stringstream ss;
            split_iter_t tokens = Str::make_split_iterator(file_contents[i]);

            for(; !tokens.eof(); ++tokens ) {
                boost::iterator_range<string::const_iterator> range = *tokens;
                ss << range << " ";
            }
            string reconstructed = ss.str();
         }
         cout << "Time for boost::make_split_iterator " << file_contents.size() << " times = " << timer.elapsed() << "\n";
      }

      {
         boost::timer timer;
         for(size_t i = 0; i < file_contents.size(); i++) {

            StringSplitter string_splitter(file_contents[i]);

            string reconstructed;
            while(!string_splitter.finished())  {
               reconstructed += std::string(string_splitter.next());
               reconstructed += " ";
            }
         }
         cout << "Time for boost::string_view " << file_contents.size() << " times = " << timer.elapsed() << "\n";
      }
   }
}
#endif

BOOST_AUTO_TEST_SUITE_END()
