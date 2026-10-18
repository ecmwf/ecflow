# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

##
## Notice: test_scaffold is an INTERFACE-only test utility library.
##

if(${SELECTED_BOOST_VERSION} VERSION_GREATER_EQUAL 1.86.0)
  set(ADDITIONAL_BOOST_COMPONENTS Boost::process)
endif()

ecflow_add_library(
  TARGET
    test_scaffold
  NOINSTALL
  TYPE STATIC
  SOURCES
    test/scaffold/ecflow/test/scaffold/EcfPortLock.hpp
    test/scaffold/ecflow/test/scaffold/LockFile.hpp
    test/scaffold/ecflow/test/scaffold/Naming.hpp
    test/scaffold/ecflow/test/scaffold/Process.hpp
    test/scaffold/ecflow/test/scaffold/Provisioning.hpp
    test/scaffold/ecflow/test/scaffold/Serialisation.hpp
    test/scaffold/ecflow/test/scaffold/TestLog.hpp
    test/scaffold/ecflow/test/scaffold/Process.cpp
  PUBLIC_INCLUDES
    test/scaffold
  PUBLIC_LIBS
    Boost::boost
    ${ADDITIONAL_BOOST_COMPONENTS}
  FORMAT
)

set(test_srcs
  # Headers
  test/TestVersioning.hpp
  # Sources
  test/TestBase64.cpp
  test/TestCalendar.cpp
  test/TestCereal.cpp
  test/TestCerealOptionalNVP.cpp
  test/TestCerealWithHierarchy.cpp
  test/TestChrono.cpp
  test/TestClassDataMemberInit.cpp
  test/TestCollections.cpp
  test/TestCommandLine.cpp
  test/TestConverter.cpp
  test/TestCore_main.cpp # contains main() function for test driver
  test/TestExtract.cpp
  test/TestFile.cpp
  test/TestFilesystem.cpp
  test/TestGetUserDetails.cpp
  test/TestIdentity.cpp
  test/TestLog.cpp
  test/TestMaskCredentials.cpp
  test/TestMessage.cpp
  test/TestMigration.cpp
  test/TestNodeNameValidity.cpp
  test/TestNodePath.cpp
  test/TestPasswdFile.cpp
  test/TestPasswordEncryption.cpp
  test/TestPerfTimer.cpp
  test/TestPTree.cpp
  test/TestRealCalendar.cpp
  test/TestResources.cpp
  test/TestResult.cpp
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
  test/TestWhiteListIncludes.cpp
)

ecflow_add_test(
  TARGET
    u_core
  LABELS
    unit nightly fast
  SOURCES
    ${test_srcs}
  LIBS
    ecflow_all
    test_scaffold
    Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
  DEFINITIONS
    ECF_TEST_EXPECTED_AVISO=\"${found_aviso_ffi_version}\"
  FORMAT
)

ecflow_add_test(
  TARGET
    u_core_generate_git_commit
  LABELS
    unit nightly fast
  COMMAND
    ${CMAKE_COMMAND}
  ARGS
    -DSCRIPT=${ecflow_SOURCE_DIR}/cmake/GenerateGitCommit.cmake
    -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/u_core_generate_git_commit
    -DGIT_EXECUTABLE=${GIT_EXECUTABLE}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/test/TestGenerateGitCommit.cmake
)
