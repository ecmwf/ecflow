#ifndef PRINTSTYLE_HPP_
#define PRINTSTYLE_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #12 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================

#include <string>

class PrintStyle {
public:
	// Use to control the print defs/node output to string or file
   // Please note: with PrintStyle::NET is used to transfer Defs between client <->server
   //              as such there is no need extensive checking on recreating defs.
   //              i.e valid name, duplicate nodes, etc
	enum Type_t {
		NOTHING = 0,  // Does nothing
		DEFS = 1,     // Output the definition that is fully parse-able                       -> On reload *CHECK* everything
      STATE = 2,    // Output definition that includes Node state, and AST, fully parseable -> On reload *CHECK* everything
      MIGRATE = 3,  // Output the definition that is fully parse-able & includes state      -> On reload *CHECK* everything
      NET = 4       // Output the definition that is fully parse-able & includes state      -> On reload relax checking
	};

	explicit PrintStyle(Type_t t) : old_style_(getStyle()) { setStyle(t);}
	~PrintStyle() { setStyle(old_style_); } // reset to old style on destruction

	/// We want to control the output, so that we can dump in old style defs format
	/// or choose to dump for debug.
	static Type_t getStyle() ;
	static void setStyle(Type_t) ;

	static bool defsStyle();
   static bool persist_style();
   static bool is_persist_style(Type_t);

	// return current style as a string
   static std::string to_string();
   static std::string to_string(PrintStyle::Type_t);

private:
  PrintStyle(const PrintStyle&) = delete;
  const PrintStyle& operator=(const PrintStyle&) = delete;

private:
   Type_t old_style_;
};

#endif
