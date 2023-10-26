#
# Copyright 2023- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

configure_file(ecflow_version.h.in ${CMAKE_BINARY_DIR}/generated/ecflow_version.h)
configure_file(ecflow_source_build_dir.h.in ${CMAKE_BINARY_DIR}/generated/ecflow_source_build_dir.h)

set(srcs
  # Headers
  ${CMAKE_BINARY_DIR}/generated/ecflow_version.h
  ${CMAKE_BINARY_DIR}/generated/ecflow_source_build_dir.h
  src/TimeStamp.hpp
  src/TimeSlot.hpp
  src/TestUtil.hpp
  src/StringSplitter.hpp
  src/PasswdFile.hpp
  src/NodePath.hpp
  src/perf_timer.hpp
  src/CommandLine.hpp
  src/Host.hpp
  src/Str.hpp
  src/Extract.hpp
  src/AssertTimer.hpp
  src/PrintStyle.hpp
  src/CheckPt.hpp
  src/Ecf.hpp
  src/File.hpp
  src/SState.hpp
  src/Serialization.hpp
  src/NOrder.hpp
  src/File_r.hpp
  src/Cal.hpp
  src/WhiteListFile.hpp
  src/User.hpp
  src/PasswordEncryption.hpp
  src/Version.hpp
  src/DurationTimer.hpp
  src/cereal_optional_nvp.hpp
  src/DebugPerf.hpp
  src/Indentor.hpp
  src/Calendar.hpp
  src/LogVerification.hpp
  src/DState.hpp
  src/NState.hpp
  src/EcfPortLock.hpp
  src/Log.hpp
  src/Passwd.hpp
  src/SerializationTest.hpp
  src/cereal_boost_time.hpp
  src/Pid.hpp
  src/Child.hpp
  src/Stl.hpp
  src/TimeSeries.hpp
  src/CalendarUpdateParams.hpp
  # Sources
  src/DState.cpp
  src/NState.cpp
  src/Log.cpp
  src/Passwd.cpp
  src/Child.cpp
  src/Pid.cpp
  src/TimeSeries.cpp
  src/Version.cpp
  src/DurationTimer.cpp
  src/Indentor.cpp
  src/Calendar.cpp
  src/LogVerification.cpp
  src/PrintStyle.cpp
  src/File.cpp
  src/Ecf.cpp
  src/SState.cpp
  src/NOrder.cpp
  src/File_r.cpp
  src/WhiteListFile.cpp
  src/Cal.cpp
  src/User.cpp
  src/TimeStamp.cpp
  src/TimeSlot.cpp
  src/StringSplitter.cpp
  src/PasswdFile.cpp
  src/NodePath.cpp
  src/CommandLine.cpp
  src/Str.cpp
  src/Host.cpp
  src/Extract.cpp
  src/AssertTimer.cpp
)

ecbuild_add_library(
  TARGET
    core
  NOINSTALL
  TYPE STATIC
  SOURCES
    ${srcs}
  PUBLIC_INCLUDES
    src
    ${CMAKE_BINARY_DIR}/generated
  PUBLIC_LIBS
    $<$<VERSION_LESS:${Boost_VERSION},1.69.0>:Boost::system>
    Boost::filesystem
    Boost::date_time
    Boost::program_options
    $<$<NOT:$<BOOL:${APPLE}>>:crypt>
  DEFINITIONS
    CMAKE
)
target_clangformat(core)

set(test_srcs
  # Headers
  test/TestVersioning.hpp
  # Sources
  test/TestCore_main.cpp # contains main() function for test driver
  test/TestCalendar.cpp
  test/TestSanitizerUB.cpp
  test/TestSanitizerAS.cpp
  test/TestStringSplitter.cpp
  test/TestFile.cpp
  test/TestWhiteListFile.cpp
  test/TestStr.cpp
  test/TestPasswordEncryption.cpp
  test/TestVersion.cpp
  test/TestPasswdFile.cpp
  test/TestVersioning.cpp
  test/TestTimeSeries.cpp
  test/TestCerealWithHierarchy.cpp
  test/TestLog.cpp
  test/TestGetUserDetails.cpp
  test/TestNodePath.cpp
  test/TestClassDataMemberInit.cpp
  test/TestStringSplitPerf.cpp
  test/TestCommandLine.cpp
  test/TestTimeSlot.cpp
  test/TestSerialisation.cpp
  test/TestRealCalendar.cpp
  test/TestStackTrace.cpp
  test/TestPerfTimer.cpp
  test/TestMigration.cpp
  test/TestCerealOptionalNVP.cpp
  test/TestCereal.cpp
)

ecbuild_add_test(
  TARGET
    u_acore
  SOURCES
    ${test_srcs}
  LIBS
    core
    Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
#
# Important!
#
# Boost::timer, Boost::chrono are required when certain definitions are manually turned on, e.g.
#  - FILE_PERF_CHECK_IMPLEMENTATIONS, at TestFile.cpp
#  - STRING_SPLIT_IMPLEMENTATIONS_PERF_CHECK_, at TestStringSplitPerf.cpp
#
)
target_clangformat(u_acore
  CONDITION ENABLE_TESTS
)
