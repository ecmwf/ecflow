#ifndef BOOST_ARCHIVE_HPP_
#define BOOST_ARCHIVE_HPP_

//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #6 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : This class has come about due to a bug with boost archive
//               includes. i.e if the same set of includes are not defined
//               in different compilations units. Then _NO_ errors/warnings are
//               issued instead we get a crash at run time when serialising
//               via base pointer.
//
//               To get round this code will use this include to collate the
//               archives used in a single place.
//============================================================================

#if defined(TEXT_ARCHIVE) || !defined(BINARY_ARCHIVE) && !defined(PORTABLE_BINARY_ARCHIVE) && !defined(EOS_PORTABLE_BINARY_ARCHIVE)
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#endif

#if defined(BINARY_ARCHIVE)
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#endif

#if defined(PORTABLE_BINARY_ARCHIVE)
#include "portable_binary_oarchive.hpp"
#include "portable_binary_iarchive.hpp"
#endif

#if defined(EOS_PORTABLE_BINARY_ARCHIVE)
#include "portable_oarchive.hpp"
#include "portable_iarchive.hpp"
#endif

#endif
