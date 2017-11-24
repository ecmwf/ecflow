#ifndef GLOSSARY_DOC_HPP_
#define GLOSSARY_DOC_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #32 $
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
#include <boost/noncopyable.hpp>

// ===========================================================================
// IMPORTANT: These appear as python doc strings.
//            Additionally they are auto documented using sphinx-poco
//            Hence the doc strings use reStructuredText markup.
// ===========================================================================
class GlossaryDoc : private boost::noncopyable {
public:
   static const char* list();
private:
   GlossaryDoc(){}
};
#endif
