#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

##
## Notice: test_support is an INTERFACE-only test utility library.
##

ecbuild_add_library(
  TARGET
    test_support
  NOINSTALL
  TYPE INTERFACE
  SOURCES
    test/TestSerialisation.hpp
    test/TestNaming.hpp
  PUBLIC_INCLUDES
    test
)
target_clangformat(test_support)

set(test_srcs
  # Headers
  test/TestVersioning.hpp
  # Sources
  test/TestCalendar.cpp
  test/TestCereal.cpp
  test/TestCerealOptionalNVP.cpp
  test/TestCerealWithHierarchy.cpp
  test/TestChrono.cpp
  test/TestClassDataMemberInit.cpp
  test/TestCommandLine.cpp
  test/TestConverter.cpp
  test/TestCore_main.cpp # contains main() function for test driver
  test/TestExtract.cpp
  test/TestFile.cpp
  test/TestGetUserDetails.cpp
  test/TestLog.cpp
  test/TestMessage.cpp
  test/TestMigration.cpp
  test/TestNodePath.cpp
  test/TestPasswdFile.cpp
  test/TestPasswordEncryption.cpp
  test/TestPerfTimer.cpp
  test/TestRealCalendar.cpp
  test/TestSanitizerAS.cpp
  test/TestSanitizerUB.cpp
  test/TestSerialisation.cpp
  test/TestStr.cpp
  test/TestStringSplitPerf.cpp
  test/TestStringSplitter.cpp
  test/TestTimeSeries.cpp
  test/TestTimeSlot.cpp
  test/TestVersion.cpp
  test/TestVersioning.cpp
  test/TestWhiteListFile.cpp
)

ecbuild_add_test(
  TARGET
    u_core
  LABELS
    unit
    nightly
  SOURCES
    ${test_srcs}
  LIBS
    test_support
    ecflow_all
    Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
    Boost::timer
)
target_clangformat(u_core
  CONDITION ENABLE_TESTS
)
