#!/bin/sh

## Copyright 2009-2019 ECMWF.
## This software is licensed under the terms of the Apache Licence version 2.0 
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
## In applying this licence, ECMWF does not waive the privileges and immunities 
## granted to it by virtue of its status as an intergovernmental organisation 
## nor does it submit to any jurisdiction. 

# Assumes boost version 1.57

# Fix 1:
# o https://svn.boost.org/trac/boost/ticket/10749
# 
#   This fix was implemented directly on the boost dir, then the tar recreated
#   modify : shared_ptr_helper.hpp 
#   to add
#     #include <boost/serialization/type_info_implementation.hpp>

# Fix 2: 
# o maintain compatibility with boost 1.53 server/archives
#   Modified Boost archive version is specified in: $BOOST_ROOT/libs/serialization/src/basic_archive.cpp
#   From 11 -> 10
#
# There has not been any changes to boost that affects, text archives.
# Testing with old version of the servers, and new clients, do not show any issues.
#

# Fix 1 notes:
# In file included from /var/tmp/ma0/boost/boost_1_57_0/boost/serialization/shared_ptr.hpp:29:0,
#                 from Base/src/cts/ClientToServerCmd.hpp:25,
#                 from Base/test/TestRequeueNodeCmd.cpp:17:
#/var/tmp/ma0/boost/boost_1_57_0/boost/serialization/shared_ptr_helper.hpp: In static member function 'static const boost::serialization::extended_type_info* boost::serialization::shared_ptr_helper<SPT>::non_polymorphic::get_object_type(U&)':
#/var/tmp/ma0/boost/boost_1_57_0/boost/serialization/shared_ptr_helper.hpp:108:39: error: 'type_info_implementation' in namespace 'boost::serialization' does not name a type
#/var/tmp/ma0/boost/boost_1_57_0/boost/serialization/shared_ptr_helper.hpp:108:63: error: expected template-argument before '<' token
#/var/tmp/ma0/boost/boost_1_57_0/boost/serialization/shared_ptr_helper.hpp:108:63: error: expected '>' before '<' token
#/var/tmp/ma0/boost/boost_1_57_0/boost/serialization/shared_ptr_helper.hpp:109:13: error: template argument 1 is invalid
#/var/tmp/ma0/boost/boost_1_57_0/boost/serialization/shared_ptr_helper.hpp:109:16: error: expected '(' before 'get_const_instance'
#/var/tmp/ma0/boost/boost_1_57_0/boost/serialization/shared_ptr_helper.hpp:109:16: error: expected ';' before 'get_const_instance'
#/var/tmp/ma0/boost/boost_1_57_0/boost/serialization/shared_ptr_helper.hpp:109:35: error: there are no arguments to 'get_const_instance' that depend on a template parameter, so a declaration of 'get_const_instance' must be available
#/var/tmp/ma0/boost/boost_1_57_0/boost/serialization/shared_ptr_helper.hpp:109:35: note: (if you use '-fpermissive', G++ will accept your code, but allowing the use of an undeclared name is deprecated)
#/var/tmp/ma0/boost/boost_1_57_0/boost/serialization/shared_ptr_helper.hpp: In static member function 'static const boost::serialization::extended_type_info* boost::serialization::shared_ptr_helper<SPT>::polymorphic::get_object_type(U&)':
#/var/tmp/ma0/boost/boost_1_57_0/boost/serialization/shared_ptr_helper.hpp:118:39: error: 'type_info_implementation' in namespace 'boost::serialization' does not name a type
#/var/tmp/ma0/boost/boost_1_57_0/boost/serialization/shared_ptr_helper.hpp:118:63: error: expected template-argument before '<' token
#/var/tmp/ma0/boost/boost_1_57_0/boost/serialization/shared_ptr_helper.hpp:118:63: error: expected '>' before '<' token
#/var/tmp/ma0/boost/boost_1_57_0/boost/serialization/shared_ptr_helper.hpp:119:13: error: template argument 1 is invalid
#/var/tmp/ma0/boost/boost_1_57_0/boost/serialization/shared_ptr_helper.hpp:119:16: error: expected '(' before 'get_const_instance'
#/var/tmp/ma0/boost/boost_1_57_0/boost/serialization/shared_ptr_helper.hpp:119:16: error: expected ';' before 'get_const_instance'
#/var/tmp/ma0/boost/boost_1_57_0/boost/serialization/shared_ptr_helper.hpp:119:35: error: there are no arguments to 'get_const_instance' that depend on a template parameter, so a declaration of 'get_const_instance' must be available
#/var/tmp/ma0/boost/boost_1_57_0/boost/serialization/shared_ptr_helper.hpp: In member function 'void boost::serialization::shared_ptr_helper<SPT>::reset(SPT<T>&, T*)':
#/var/tmp/ma0/boost/boost_1_57_0/boost/serialization/shared_ptr_helper.hpp:131:17: error: 'type_info_implementation' is not a member of 'boost::serialization'
#/var/tmp/ma0/boost/boost_1_57_0/boost/serialization/shared_ptr_helper.hpp:131:67: error: expected primary-expression before '>' token
#/var/tmp/ma0/boost/boost_1_57_0/boost/serialization/shared_ptr_helper.hpp:131:70: error: '::type' has not been declared
#...failed gcc.compile.c++ Base/bin/gcc-4.5/release/test/TestRequeueNodeCmd.o...