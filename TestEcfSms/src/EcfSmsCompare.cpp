//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision$ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================

#include <set>
#include "boost/filesystem.hpp"
#include "boost/filesystem/operations.hpp"
#include <boost/lexical_cast.hpp>

#include "EcfSmsCompare.hpp"
#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "File.hpp"
#include "Str.hpp"
#include "Extract.hpp"

using namespace std;
using namespace ecf;
namespace fs = boost::filesystem;

bool EcfSmsCompare::run(const defs_ptr& defs, std::string& errorMsg )
{
//	cout << "...EcfSmsCompare::run\n";
	if (defs->suiteVec().size() != 1) {
		errorMsg = "EcfSmsCompare::run  The defs should only have one suite";
		return false;
	}

	// Open the file.
	std::vector<std::string> lines;
 	if (!File::splitFileIntoLines(comparisonFile_,lines)) {
		errorMsg = "EcfSmsCompare::run Could not open file " + comparisonFile_;
		return false;
 	}

 	// format is:
 	//   	<Node path> <int> <type> <int> <value>
 	//  if type = <event | meter | variable | repeat >
 	//  Node_path = <node path> : <name>
 	//
 	// 		/test:ECF_INCLUDE 3 variable 0 /home/ma/ma0/course
 	// 		/test:ECF_HOME 3 variable 0 /home/ma/ma0/course
	// 		/test/f1 11 family 3 queued
 	// 		/test/f1:SLEEP 3 variable 0 5
 	// 		/test/f1/t1 10 task 3 queued
 	// 		/test/f1/t1:progress 24 meter 1
  	// 		/test/f1/t2:a 9 event 0 clear
 	// 		/test/f1/t2:b 9 event 1 set
 	//
 	//  If the type is limit | repeat then we only have 4 tokens
 	//		/test:l2            33 limit  0
  	//  	/test/f4/f5/t1:DATE 22 repeat 19991230

 	std::set<std::string> nodePathsInComparisonFile;
 	for(size_t i = 0; i < lines.size(); i++) {

// 		cout << i << ": " << lines[i] << "\n";

 		if (lines[i].empty()) continue;

 		// Split the lines,
 		std::vector< std::string > lineTokens;
 		Str::split(lines[i], lineTokens );
 		if ( lineTokens.size() > 5) {
 			std::stringstream ss; ss << "EcfSmsCompare::run expected 5 or less tokens per line but found " << lineTokens.size() << " per line\n";
 			errorMsg = ss.str();
 			addLineToErrorMsg(i,lines[i],errorMsg);
 			return false;
 		}
 		if ( lineTokens.empty()) continue;

 		string path, name, value;
 		if (!Extract::pathAndName(lineTokens[0],path,name) || path.empty()) {
 			errorMsg = "EcfSmsCompare::run Could not extract path\n";
 			addLineToErrorMsg(i,lines[i],errorMsg);
 			return false;
 		}
		if (path.empty()) {
 			errorMsg = "EcfSmsCompare::run Could not extract path ";
 			return false;
 		}

		node_ptr node = defs->findAbsNode(path);
		if (!node.get()) {
			errorMsg += "EcfSmsCompare::run could not find node " + path + "\n";
 			addLineToErrorMsg(i,lines[i],errorMsg);
			continue;
		}
		nodePathsInComparisonFile.insert(path);

		string type = lineTokens[2];
		if (type == "task" || type == "family" || type == "suite") {
			value = lineTokens[4];
 			NState::State state = NState::toState( value );
 			if (node->state() != state) {
 	 			std::stringstream ss;
 	 			ss << "EcfSmsCompare::run For path " << lineTokens[0] << " expected state " << value << " but found " << NState::toString(node->state()) << "\n";
				errorMsg += ss.str();
	 			addLineToErrorMsg(i,lines[i],errorMsg);
 				continue;
 			}
		}
		else if (type == "variable") {
			value = lineTokens[4];
			const Variable& var = node->findVariable( name ) ;
			if (var.empty()) {
				errorMsg += "EcfSmsCompare::run variable name " + name + " not found on node " + node->debugNodePath() + "\n";
	 			addLineToErrorMsg(i,lines[i],errorMsg);
				continue;
			}
			// IGNORE variable ECF_HOME and ECF_INCLUSE as these were change by ECF, to run locally
			if ( var.name() == Str::ECF_HOME() || var.name() == Str::ECF_INCLUDE()) continue;
			if (var.theValue() != value) {
 	 			std::stringstream ss;
 	 			ss << "EcfSmsCompare::run For path " << lineTokens[0] << " expected variable " << name << " to have a value of  " <<  value << " but found " << var.theValue() << "\n";
				errorMsg += ss.str();
	 			addLineToErrorMsg(i,lines[i],errorMsg);
 				continue;
			}
		}
		else if (type == "limit") {
			value = lineTokens[3];

			limit_ptr lim = node->find_limit( name );
 			if (!lim.get()) {
				errorMsg += "EcfSmsCompare::run limit name " + name + " not found on node " + node->debugNodePath() + "\n";
	 			addLineToErrorMsg(i,lines[i],errorMsg);
				continue;
			}

 			int intVaue = 0;
			try { intVaue = boost::lexical_cast< int >(value) ; }
			catch (boost::bad_lexical_cast&) {
	 			std::stringstream ss;
 	 			ss << "EcfSmsCompare::run For path " << lineTokens[0] << " could not convert limit value " << value << " to an integer\n";
				errorMsg += ss.str();
	 			addLineToErrorMsg(i,lines[i],errorMsg);
				continue;
			}

			if (lim->value() != intVaue) {
 	 			std::stringstream ss;
 	 			ss << "EcfSmsCompare::run For path " << lineTokens[0] << " expected limit " << name << " to have a value of  " <<  intVaue << " but found " << lim->value() << "\n";
				errorMsg += ss.str();
	 			addLineToErrorMsg(i,lines[i],errorMsg);
 				continue;
			}
		}
		else if (type == "repeat") {
			value = lineTokens[3];
			const Repeat& repeat = node->findRepeat( name ) ;
			if (repeat.empty()) {
				errorMsg += "EcfSmsCompare::run Repeat name " + name + " not found on node " + node->debugNodePath() + "\n";
	 			addLineToErrorMsg(i,lines[i],errorMsg);
				continue;
			}

			int repeatValue = 0;
			try { repeatValue = boost::lexical_cast<int>(value); }
			catch (boost::bad_lexical_cast&){
	 			std::stringstream ss;
 	 			ss << "EcfSmsCompare::run For path " << lineTokens[0] << " could not convert repeat value " << value << " to an integer\n";
				errorMsg += ss.str();
	 			addLineToErrorMsg(i,lines[i],errorMsg);
				continue;
			}

			if (repeat.value() != repeatValue) {
 	 			std::stringstream ss;
 	 			ss << "EcfSmsCompare::run For path " << lineTokens[0] << " expected repeat " << name << " to have a value of  " <<  repeatValue << " but found " << repeat.value() << "\n";
				errorMsg += ss.str();
	 			addLineToErrorMsg(i,lines[i],errorMsg);
 				continue;
			}
		}
		else if (type == "event") {
			value = lineTokens[4];

			if (name.empty()) {
				errorMsg += "EcfSmsCompare::run event name not specified \n";
	 			addLineToErrorMsg(i,lines[i],errorMsg);
				return false;
			}
			const Event& event = node->findEventByNameOrNumber(name);
			if (event.empty()) {
				errorMsg += "EcfSmsCompare::run event name " + name + " not found on node " + node->debugNodePath() + "\n";
	 			addLineToErrorMsg(i,lines[i],errorMsg);
				return false;
			}
			if (event.value() && value == Event::CLEAR()) {
	 			std::stringstream ss;
 	 			ss << "EcfSmsCompare::run For path " << lineTokens[0] << " expected event state of 'clear' but found 'set'\n";
				errorMsg += ss.str();
	 			addLineToErrorMsg(i,lines[i],errorMsg);
 				continue;
			}
			if (!event.value() && value == Event::SET()) {
	 			std::stringstream ss;
 	 			ss << "EcfSmsCompare::run For path " << lineTokens[0] << " expected event state of 'set' but found 'clear'\n";
				errorMsg += ss.str();
	 			addLineToErrorMsg(i,lines[i],errorMsg);
 				continue;
			}
		}
		else if (type == "meter") {
			value = lineTokens[3];

			if (name.empty()) {
				errorMsg += "EcfSmsCompare::run meter name not specified \n";
	 			addLineToErrorMsg(i,lines[i],errorMsg);
				return false;
			}
			const Meter& meter = node->findMeter(name);
			if (meter.empty()) {
				errorMsg += "EcfSmsCompare::run meter name " + name + " not found on node " + node->debugNodePath() + "\n";
	 			addLineToErrorMsg(i,lines[i],errorMsg);
				return false;
			}

			int metervalue = 0;
			try { metervalue = boost::lexical_cast<int>(value); }
			catch (boost::bad_lexical_cast&){
	 			std::stringstream ss;
 	 			ss << "EcfSmsCompare::run For path " << lineTokens[0] << " could not convert meter value " << value << " to an integer\n";
				errorMsg += ss.str();
	 			addLineToErrorMsg(i,lines[i],errorMsg);
				continue;
			}

			if (meter.value()  != metervalue) {
	 			std::stringstream ss;
 	 			ss << "EcfSmsCompare::run For path " << lineTokens[0] << " expected meter value of '" << metervalue << "' but found '" << meter.value()  << "'\n";
				errorMsg += ss.str();
	 			addLineToErrorMsg(i,lines[i],errorMsg);
				continue;
			}
		}
		else {
			errorMsg += "Can not recognise type " + type + "\n";
 			addLineToErrorMsg(i,lines[i],errorMsg);
 			continue;
		}
 	}

 	// Now cross-check that all tasks/family paths in the defs file exist in the comparison file
 	// Note: Suites do not exist in the comparison file
 	if (errorMsg.empty() ) {
 		std::vector<Family*> fvec;
 		defs->getAllFamilies(fvec);
 		for(size_t i = 0; i < fvec.size(); i++) {
 			if ( nodePathsInComparisonFile.find(fvec[i]->absNodePath()) == nodePathsInComparisonFile.end() ) {
 				errorMsg += "Could not find Family " + fvec[i]->absNodePath() + " in file " + comparisonFile_ + "\n";
 			}
 		}
 		std::vector<Task*> tvec;
 		defs->getAllTasks( tvec );
		for(size_t i = 0; i < tvec.size(); i++) {
 			if ( nodePathsInComparisonFile.find(tvec[i]->absNodePath()) == nodePathsInComparisonFile.end() ) {
 				errorMsg += "Could not find Task " + tvec[i]->absNodePath() + " in file " + comparisonFile_ + "\n";
 			}
 		}
 	}

 	return errorMsg.empty();
}

void EcfSmsCompare::addLineToErrorMsg(size_t i,const std::string& line, std::string& errorMsg ) const
{
	std::stringstream ss;
	ss << i+1 << ": " << line << "\n";
	errorMsg += ss.str();
}

