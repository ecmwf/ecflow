////============================================================================
//// Name        :
//// Author      : Avi
//// Revision    : $Revision: #5 $
////
//// Copyright 2009-2019 ECMWF.
//// This software is licensed under the terms of the Apache Licence version 2.0
//// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
//// In applying this licence, ECMWF does not waive the privileges and immunities
//// granted to it by virtue of its status as an intergovernmental organisation
//// nor does it submit to any jurisdiction.
////
//// Description :
////============================================================================
//#include <boost/test/unit_test.hpp>
//#include <boost/bind.hpp>
//
//#include <boost/test/exception_safety.hpp>
//#include <boost/test/mock_object.hpp>
//#include <boost/make_shared.hpp>
//#include <boost/shared_ptr.hpp>
//
//using namespace std;
//using namespace boost::itest;
//
//BOOST_AUTO_TEST_SUITE( CoreTestSuite )
//
//// Example of how to check for for exception safety
//// COMMENTED OUT SINCE THIS CAUSES THOUSANDS OF VALGRIND ERRORS
//
//template<class T1, class T2>
//void algo(
//  boost::shared_ptr<T1> x,
//  boost::shared_ptr<T2> y) {}
//
//typedef mock_object<> Mock;
//typedef boost::shared_ptr<Mock> SharedMock;
//
//BOOST_TEST_EXCEPTION_SAFETY( fail_test )
//{
//	algo( SharedMock( new Mock() ),
//	      SharedMock( new Mock() ));
//}
//
//BOOST_TEST_EXCEPTION_SAFETY( success_test )
//{
//	algo( boost::make_shared<Mock>(),
//	      boost::make_shared<Mock>());
//}
//
//BOOST_AUTO_TEST_SUITE_END()
