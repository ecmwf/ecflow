
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #49 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : This class is used as a helper class
//============================================================================
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "StringSplitter.hpp"
#include "Str.hpp"

using namespace std;

namespace ecf {

const char* VALID_NODE_CHARS = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.";

const char* Str::CHILD_CMD() { static const char* CHILD_CMD = "chd:"; return CHILD_CMD; }
const char* Str::USER_CMD() { static const char* USER_CMD = "--"; return USER_CMD; }
const char* Str::SVR_CMD() { static const char* SVR_CMD = "svr:"; return SVR_CMD; } // Only for automatic check_pt

const std::string& Str::EMPTY() { static std::string empty = std::string(); return empty;}
const std::string& Str::ROOT_PATH() { static std::string root_path  = "/"; return root_path;}
const std::string& Str::PATH_SEPERATOR() { static std::string  path_sep = "/"; return path_sep;}
const std::string& Str::COLON() { static std::string colon = ":"; return colon;}


const std::string& Str::STATE_CHANGE() { static std::string state_change = "   state change "; return state_change;}

const std::string& Str::TASK()   { static std::string task = "TASK"; return task; }
const std::string& Str::FAMILY() { static std::string family = "FAMILY"; return family;}
const std::string& Str::SUITE()  { static std::string suite = "SUITE"; return suite;}
const std::string& Str::ALIAS()  { static std::string alias = "ALIAS"; return alias; }

const std::string& Str::DEFAULT_PORT_NUMBER() { static std::string port_number = "3141"; return port_number;}
const std::string& Str::LOCALHOST()           { static std::string localhost = "localhost"; return localhost;}

const std::string& Str::ECF_PORT()       { static std::string ECF_PORT = "ECF_PORT"; return ECF_PORT;}
const std::string& Str::ECF_RID()        { static std::string ECF_RID = "ECF_RID"; return ECF_RID;}
const std::string& Str::ECF_TRYNO()      { static std::string ECF_TRYNO = "ECF_TRYNO"; return ECF_TRYNO;}
const std::string& Str::ECF_TRIES()      { static std::string ECF_TRIES = "ECF_TRIES"; return ECF_TRIES;}
const std::string& Str::ECF_NAME()       { static std::string ECF_NAME = "ECF_NAME"; return ECF_NAME; }
const std::string& Str::ECF_HOST()       { static std::string ECF_HOST = "ECF_HOST"; return ECF_HOST;}
const std::string& Str::ECF_PASS()       { static std::string ECF_PASS = "ECF_PASS";return ECF_PASS;}
const std::string& Str::ECF_JOB()        { static std::string ECF_JOB = "ECF_JOB"; return ECF_JOB;}
const std::string& Str::ECF_JOBOUT()     { static std::string ECF_JOBOUT = "ECF_JOBOUT"; return ECF_JOBOUT;}
const std::string& Str::ECF_SCRIPT()     { static std::string ECF_SCRIPT = "ECF_SCRIPT"; return ECF_SCRIPT;}
const std::string& Str::ECF_DUMMY_TASK() { static std::string ECF_DUMMY_TASK = "ECF_DUMMY_TASK"; return ECF_DUMMY_TASK;}
const std::string& Str::ECF_NO_SCRIPT()  { static std::string ECF_NO_SCRIPT = "ECF_NO_SCRIPT"; return ECF_NO_SCRIPT;}
const std::string& Str::ECF_MICRO()      { static std::string ECF_MICRO = "ECF_MICRO"; return ECF_MICRO;}
const std::string& Str::ECF_FILES()      { static std::string ECF_FILES = "ECF_FILES"; return ECF_FILES;}
const std::string& Str::ECF_FETCH()      { static std::string ECF_FETCH = "ECF_FETCH"; return ECF_FETCH;}
const std::string& Str::ECF_KILL_CMD()   { static std::string ECF_KILL_CMD = "ECF_KILL_CMD"; return ECF_KILL_CMD;}
const std::string& Str::ECF_STATUS_CMD() { static std::string ECF_STATUS_CMD = "ECF_STATUS_CMD"; return ECF_STATUS_CMD;}

const std::string& Str::ECF_HOME()    { static std::string ECF_HOME = "ECF_HOME"; return ECF_HOME;}
const std::string& Str::ECF_INCLUDE() { static std::string ECF_INCLUDE = "ECF_INCLUDE"; return ECF_INCLUDE;}
const std::string& Str::ECF_JOB_CMD() { static std::string ECF_JOB_CMD = "ECF_JOB_CMD"; return ECF_JOB_CMD;}
const std::string& Str::ECF_OUT()     { static std::string ECF_OUT = "ECF_OUT"; return ECF_OUT;}
const std::string& Str::ECF_EXTN()    { static std::string ECF_EXTN = "ECF_EXTN"; return ECF_EXTN;}
const std::string& Str::ECF_LOG()     { static std::string ECF_LOG = "ECF_LOG"; return ECF_LOG;}


const std::string& Str::WHITE_LIST_FILE() { static std::string WHITE_LIST_FILE = "ecf.lists"; return WHITE_LIST_FILE;}
const std::string& Str::ECF_PASSWD() { static std::string ECF_PASSWD = "ecf.passwd"; return ECF_PASSWD;}

const std::string& Str::ALPHANUMERIC_UNDERSCORE()
{
   static string ALPHANUMERIC_UNDERSCORE = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
   return ALPHANUMERIC_UNDERSCORE;
}

const std::string& Str::NUMERIC() { static string NUMERIC = "0123456789"; return NUMERIC; }

void Str::removeQuotes(std::string& s)
{
	if (!s.empty()) {
		if (s[0] == '"' && s[s.size()-1] == '"') {
			s.erase(s.begin());
			s.erase(s.begin() + s.size()-1);
		}
	}
}
// 047 is octal for '
void Str::removeSingleQuotes(std::string& s)
{
	if (!s.empty()) {
		if (s[0] == 047 && s[s.size()-1] == 047) {
			s.erase(s.begin());
			s.erase(s.begin() + s.size()-1);
		}
	}
}

bool Str::replace(std::string& jobLine, const std::string& stringToFind, const std::string& stringToRplace)
{
	size_t pos = jobLine.find(stringToFind);
	if (pos != string::npos) {
		jobLine.replace(pos,stringToFind.length(),stringToRplace);
		return true;
	}
	return false;
}

bool Str::replace_all(std::string& subject, const std::string& stringToFind, const std::string& stringToReplace)
{
   bool replaced = false ;
   size_t pos = 0;
   while ((pos = subject.find(stringToFind, pos)) != std::string::npos) {
      subject.replace(pos, stringToFind.length(), stringToReplace);
      pos += stringToReplace.length();
      replaced = true;
   }
   return replaced;
}

bool Str::extract_data_member_value(const std::string& str, const std::string& data_member_name, std::string& data_member_value)
{
   //        012345678901234567,
   //    str=cmd 1 user:mao
   //    data_member_name=user:
   //    data_member_value=ma0
   std::string::size_type start = str.find(data_member_name);
   if (start != std::string::npos) {
      start += data_member_name.size();
      data_member_value.clear();
      for(size_t i = start; i < str.size(); i++) {
         if (str[i] == ' ') break;
         data_member_value += str[i];
      }
      return true;
   }
   return false;
}


void Str::replaceall(std::string& subject, const std::string& search, const std::string& replace)
{
   boost::replace_all(subject, search, replace);
}

#define USE_STRINGSPLITTER 1
void Str::split(const std::string& line, std::vector< std::string >& tokens,const std::string& delimiters )
{
#ifdef USE_STRINGSPLITTER
   StringSplitter::split(line,tokens,delimiters);
#else
   Str::split_orig(line,tokens,delimiters);
#endif

//	// ***************************************************************************
//	// Time for split 1000000 times = 11.66
//
//    // FAILS: with line = "\n", since I expect at least one token of "\n"
//    int i = 0;
//	char ch;
//	string word;
//	while ( (ch = line[i++]) ) {
//		if ( isspace( ch ) ) {
//			if ( !word.empty() )  tokens.push_back( word );
//			word = "";
//		}
//		else  word += ch;
//	}
//	if ( !word.empty() )  tokens.push_back( word );

// // ******************************************************************************
// // Time for boost split > 20 seconds, this
//		boost::algorithm::split(result, line, std::bind2nd(std::equal_to<char>(), ' '));
//
// 	// ***************************************************************************
//	// Time for split 1000000 times = 30.38
//
//    // FAILS: with line = "\n", since I expect at least one token of "\n"
//	char_separator< char > sep( " ",0, boost::drop_empty_tokens );
//	typedef boost::tokenizer< boost::char_separator< char > > tokenizer;
//	tokenizer theTokenizer( line, sep );
//	// std::copy( theTokenizer.begin(), theTokenizer.end(), back_inserter( tokens ) );
//
//	for(tokenizer::iterator beg=theTokenizer.begin(); beg!=theTokenizer.end();++beg){
//		string token = *beg;
//		boost::algorithm::trim(token);
//	    if (token.empty()) continue;
//		tokens.push_back(token);
//	 }
}

void Str::split_orig(const std::string& line, std::vector< std::string >& tokens,const std::string& delimiters )
{
   // Skip delimiters at beginning.
   string::size_type lastPos = line.find_first_not_of( delimiters, 0 );

   // Find first "non-delimiter".
   string::size_type pos = line.find_first_of( delimiters, lastPos );

   while ( string::npos != pos || string::npos != lastPos ) {
      tokens.push_back( line.substr( lastPos, pos - lastPos ) ); // Found a token, add it to the vector.
      lastPos = line.find_first_not_of( delimiters, pos );       // Skip delimiters.  Note the "not_of"
      pos = line.find_first_of( delimiters, lastPos );           // Find next "non-delimiter"
   }
}

boost::split_iterator<std::string::const_iterator> Str::make_split_iterator(const std::string& line,const std::string& delimiters)
{
   return boost::make_split_iterator(line, boost::algorithm::token_finder(boost::is_any_of(delimiters),boost::algorithm::token_compress_on));
}

static bool caseInsCharCompare(char a, char b) { return  (toupper(a) == toupper(b)); }

bool Str::caseInsCompare( const std::string& s1, const std::string& s2)
{
	return ( (s1.size() == s2.size()) && equal(s1.begin(),s1.end(), s2.begin(), caseInsCharCompare));
}


struct case_insensitive_less : public std::binary_function< char,char,bool >
{
    bool operator () (char x, char y) const {
        if (toupper(x) == toupper(y)) {
           return x > y;
        }
        return toupper( static_cast< unsigned char >(x)) <
               toupper( static_cast< unsigned char >(y));
    }
};
bool Str::caseInsLess( const std::string& a, const std::string& b)
{
   return std::lexicographical_compare( a.begin(),a.end(),
           b.begin(),b.end(), case_insensitive_less() );
}


struct case_insensitive_greater : public std::binary_function< char,char,bool >
{
    bool operator () (char x, char y) const {
        if (toupper(x) == toupper(y)) {
           return x < y;
        }
        return toupper( static_cast< unsigned char >(x)) >
               toupper( static_cast< unsigned char >(y));
    }
};
bool Str::caseInsGreater( const std::string& a, const std::string& b)
{
   return std::lexicographical_compare( a.begin(),a.end(),
           b.begin(),b.end(), case_insensitive_greater() );
}


bool Str::valid_name(const std::string& name, std::string &msg)
{
   // valid names are alphabetic (alphanumeric | underscore | .)
   // however we can't have a leading '.' as that can interfere with trigger expressions

   // verify that the string is not empty
   if ( name.empty() ) {
	   msg = "Invalid name. Empty string.";
	   return false;
   }

   // verify that the first character is alphabetic or has underscore
 	bool result = Str::ALPHANUMERIC_UNDERSCORE().find( name[0], 0 ) != string::npos;
	if ( !result ) {
      msg = "Valid names can only consist of alphanumeric characters "
            ",underscores and dots. The first character can not be a dot: ";
      msg += name;
		return false;
	}

   // verify that any other characters are alphanumeric or underscore
   if (name.size() > 1) {
      result = name.find_first_not_of(VALID_NODE_CHARS, 1) == string::npos;
      if ( !result ) {
         msg = "Valid names can only consist of alphanumeric characters "
               ",underscores and dots. The first character can not be a dot. ";
         if (name.find('\r') != string::npos)  msg += "Window's line ending ? ";
         msg += "'";
         msg += name;
         msg += "'"; // use '<name>' to show if PC format. i.e carriage return
      }
   }

   return result;
}

bool Str::valid_name(const std::string& name)
{
   // valid names are alphabetic (alphanumeric | underscore | .)
   // however we can't have a leading '.' as that can interfere with trigger expressions

   // verify that the string is not empty
   if ( name.empty() ) {
      return false;
   }

   // verify that the first character is alphabetic or has underscore
   bool result = Str::ALPHANUMERIC_UNDERSCORE().find( name[0], 0 ) != string::npos;
   if ( !result ) {
      return false;
   }

   // verify that any other characters are alphanumeric or underscore
   if (name.size() > 1) {
      result = name.find_first_not_of(VALID_NODE_CHARS, 1) == string::npos;
   }

   return result;
}

int Str::to_int( const std::string& the_str, int error_return)
{
	if ( the_str.find_first_of( Str::NUMERIC(), 0 ) != std::string::npos ) {
		try {
			return boost::lexical_cast< int >( the_str );
		}
		catch ( boost::bad_lexical_cast&) {}
	}
	return error_return;
}

bool Str::truncate_at_start(  std::string& fileContents, size_t max_lines)
{
	if (fileContents.empty()) return false;

	/// Truncate from the front
	size_t no_of_new_lines = 0;
	for(size_t i =fileContents.size()-1; i >0; --i) {
		if (fileContents[i] == '\n') no_of_new_lines++;
		if ( no_of_new_lines >=  max_lines) {
			fileContents.erase(fileContents.begin(),fileContents.begin() + i +1); //skip new line at start of file
			return true;
		}
	}
	return false;
}

bool Str::truncate_at_end(  std::string& fileContents, size_t max_lines)
{
   if (fileContents.empty()) return false;

   /// Truncate from the back
   size_t no_of_new_lines = 0;
   size_t the_size = fileContents.size();
   for(size_t i =0; i < the_size; ++i) {
      if (fileContents[i] == '\n') no_of_new_lines++;
      if ( no_of_new_lines >=  max_lines) {
         fileContents.erase(fileContents.begin()+i+1,fileContents.end()); //skip new line at end of file
         return true;
      }
   }
   return false;
}

}
