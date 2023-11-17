#
# Copyright 2023- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

configure_file(src/ecflow_version.h.in ${CMAKE_BINARY_DIR}/generated/src/ecflow_version.h)
configure_file(src/ecflow_source_build_dir.h.in ${CMAKE_BINARY_DIR}/generated/src/ecflow_source_build_dir.h)

set(srcs
  # Headers
  ${CMAKE_BINARY_DIR}/generated/src/ecflow_version.h
  ${CMAKE_BINARY_DIR}/generated/src/ecflow_source_build_dir.h
  src/AssertTimer.hpp
  src/Cal.hpp
  src/Calendar.hpp
  src/CalendarUpdateParams.hpp
  src/CheckPt.hpp
  src/Child.hpp
  src/CommandLine.hpp
  src/DState.hpp
  src/DebugPerf.hpp
  src/DurationTimer.hpp
  src/Ecf.hpp
  src/EcfPortLock.hpp
  src/Extract.hpp
  src/File.hpp
  src/File_r.hpp
  src/Host.hpp
  src/Indentor.hpp
  src/Log.hpp
  src/LogVerification.hpp
  src/NOrder.hpp
  src/NState.hpp
  src/NodePath.hpp
  src/Passwd.hpp
  src/PasswdFile.hpp
  src/PasswordEncryption.hpp
  src/Pid.hpp
  src/PrintStyle.hpp
  src/SState.hpp
  src/Serialization.hpp
  src/SerializationTest.hpp
  src/Stl.hpp
  src/Str.hpp
  src/StringSplitter.hpp
  src/TestUtil.hpp
  src/TimeSeries.hpp
  src/TimeSlot.hpp
  src/TimeStamp.hpp
  src/User.hpp
  src/Version.hpp
  src/WhiteListFile.hpp
  src/cereal_boost_time.hpp
  src/cereal_optional_nvp.hpp
  src/perf_timer.hpp
  # Sources
  src/AssertTimer.cpp
  src/Cal.cpp
  src/Calendar.cpp
  src/Child.cpp
  src/CommandLine.cpp
  src/DState.cpp
  src/DurationTimer.cpp
  src/Ecf.cpp
  src/Extract.cpp
  src/File.cpp
  src/File_r.cpp
  src/Host.cpp
  src/Indentor.cpp
  src/Log.cpp
  src/LogVerification.cpp
  src/NOrder.cpp
  src/NState.cpp
  src/NodePath.cpp
  src/Passwd.cpp
  src/PasswdFile.cpp
  src/Pid.cpp
  src/PrintStyle.cpp
  src/SState.cpp
  src/Str.cpp
  src/StringSplitter.cpp
  src/TimeSeries.cpp
  src/TimeSlot.cpp
  src/TimeStamp.cpp
  src/User.cpp
  src/Version.cpp
  src/WhiteListFile.cpp
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
    ${CMAKE_BINARY_DIR}/generated/src
  PUBLIC_LIBS
    cereal::cereal # this needs to be public as it appears in public header files used downstream
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
  test/TestCalendar.cpp
  test/TestCereal.cpp
  test/TestCerealOptionalNVP.cpp
  test/TestCerealWithHierarchy.cpp
  test/TestClassDataMemberInit.cpp
  test/TestCommandLine.cpp
  test/TestCore_main.cpp # contains main() function for test driver
  test/TestFile.cpp
  test/TestGetUserDetails.cpp
  test/TestLog.cpp
  test/TestMigration.cpp
  test/TestNodePath.cpp
  test/TestPasswdFile.cpp
  test/TestPasswordEncryption.cpp
  test/TestPerfTimer.cpp
  test/TestRealCalendar.cpp
  test/TestSanitizerAS.cpp
  test/TestSanitizerUB.cpp
  test/TestSerialisation.cpp
  test/TestStackTrace.cpp
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
    u_acore
  LABELS
    unit nightly
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
