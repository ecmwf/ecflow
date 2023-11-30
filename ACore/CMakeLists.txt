#
# Copyright 2023- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

configure_file(
  src/ecflow/core/ecflow_version.h.in
  ${CMAKE_BINARY_DIR}/generated/src/ecflow/core/ecflow_version.h)
configure_file(
  src/ecflow/core/ecflow_source_build_dir.h.in
  ${CMAKE_BINARY_DIR}/generated/src/ecflow/core/ecflow_source_build_dir.h)

set(srcs
  # Headers
  ${CMAKE_BINARY_DIR}/generated/src/ecflow/core/ecflow_version.h
  ${CMAKE_BINARY_DIR}/generated/src/ecflow/core/ecflow_source_build_dir.h
  src/ecflow/core/AssertTimer.hpp
  src/ecflow/core/Cal.hpp
  src/ecflow/core/Calendar.hpp
  src/ecflow/core/CalendarUpdateParams.hpp
  src/ecflow/core/CheckPt.hpp
  src/ecflow/core/Child.hpp
  src/ecflow/core/CommandLine.hpp
  src/ecflow/core/DState.hpp
  src/ecflow/core/DebugPerf.hpp
  src/ecflow/core/DurationTimer.hpp
  src/ecflow/core/Ecf.hpp
  src/ecflow/core/EcfPortLock.hpp
  src/ecflow/core/Extract.hpp
  src/ecflow/core/File.hpp
  src/ecflow/core/File_r.hpp
  src/ecflow/core/Host.hpp
  src/ecflow/core/Indentor.hpp
  src/ecflow/core/Log.hpp
  src/ecflow/core/LogVerification.hpp
  src/ecflow/core/NOrder.hpp
  src/ecflow/core/NState.hpp
  src/ecflow/core/NodePath.hpp
  src/ecflow/core/Passwd.hpp
  src/ecflow/core/PasswdFile.hpp
  src/ecflow/core/PasswordEncryption.hpp
  src/ecflow/core/Pid.hpp
  src/ecflow/core/PrintStyle.hpp
  src/ecflow/core/SState.hpp
  src/ecflow/core/Serialization.hpp
  src/ecflow/core/SerializationTest.hpp
  src/ecflow/core/Stl.hpp
  src/ecflow/core/Str.hpp
  src/ecflow/core/StringSplitter.hpp
  src/ecflow/core/TestUtil.hpp
  src/ecflow/core/TimeSeries.hpp
  src/ecflow/core/TimeSlot.hpp
  src/ecflow/core/TimeStamp.hpp
  src/ecflow/core/User.hpp
  src/ecflow/core/Version.hpp
  src/ecflow/core/WhiteListFile.hpp
  src/ecflow/core/cereal_boost_time.hpp
  src/ecflow/core/cereal_optional_nvp.hpp
  src/ecflow/core/perf_timer.hpp
  # Sources
  src/ecflow/core/AssertTimer.cpp
  src/ecflow/core/Cal.cpp
  src/ecflow/core/Calendar.cpp
  src/ecflow/core/Child.cpp
  src/ecflow/core/CommandLine.cpp
  src/ecflow/core/DState.cpp
  src/ecflow/core/DurationTimer.cpp
  src/ecflow/core/Ecf.cpp
  src/ecflow/core/Extract.cpp
  src/ecflow/core/File.cpp
  src/ecflow/core/File_r.cpp
  src/ecflow/core/Host.cpp
  src/ecflow/core/Indentor.cpp
  src/ecflow/core/Log.cpp
  src/ecflow/core/LogVerification.cpp
  src/ecflow/core/NOrder.cpp
  src/ecflow/core/NState.cpp
  src/ecflow/core/NodePath.cpp
  src/ecflow/core/Passwd.cpp
  src/ecflow/core/PasswdFile.cpp
  src/ecflow/core/Pid.cpp
  src/ecflow/core/PrintStyle.cpp
  src/ecflow/core/SState.cpp
  src/ecflow/core/Str.cpp
  src/ecflow/core/StringSplitter.cpp
  src/ecflow/core/TimeSeries.cpp
  src/ecflow/core/TimeSlot.cpp
  src/ecflow/core/TimeStamp.cpp
  src/ecflow/core/User.cpp
  src/ecflow/core/Version.cpp
  src/ecflow/core/WhiteListFile.cpp
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
    unit
    nightly
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
