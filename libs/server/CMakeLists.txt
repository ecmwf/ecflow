#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

# =======================================================
# EXE ecflow_server, if OpenSSL not enabled ${OPENSSL_LIBRARIES}, is empty

ecbuild_add_executable(
  TARGET
    ecflow_server
  SOURCES
    src/ecflow/server/ServerMain.cpp
  LIBS
    ecflow_all
    $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
    Threads::Threads
)
target_clangformat(ecflow_server)

# Override default behaviour that add RPATHS during install
# The only thing that seem to work is set INSTALL_RPATH to ""
# Using SKIP_BUILD_RPATH,BUILD_WITH_INSTALL_RPATH,INSTALL_RPATH_USE_LINK_PATH
# had no effect
#          
SET_TARGET_PROPERTIES(ecflow_server
  PROPERTIES
    INSTALL_RPATH ""
)

# use, i.e. don't skip the full RPATH for the build tree
#SET(CMAKE_SKIP_BUILD_RPATH  FALSE)

# when building, don't use the install RPATH already
# (but later on when installing)
#SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) 

# the RPATH to be used when installing
#SET(CMAKE_INSTALL_RPATH "")

# don't add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
#SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)


# ================================================================================                         
# TEST
ecbuild_add_test(
  TARGET
    u_server
  LABELS
    unit nightly
  INCLUDES
    src
  SOURCES
    test/TestCheckPtSaver.cpp
    test/TestServerEnvironment.cpp
    test/TestServer_main.cpp # test entry point
    test/TestServer.cpp
  LIBS
    ecflow_all
    Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
    $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
    Threads::Threads
    test_support
  TEST_DEPENDS
    u_base
)

target_clangformat(u_server CONDITION ENABLE_TESTS)

# ================================================================================
# TEST
ecbuild_add_test(
  TARGET
    s_periodic_scheduler
  LABELS
    integration nightly
  INCLUDES
    src
  SOURCES
    test/TestPeriodicScheduler.cpp
    test/TestPeriodicScheduler_main.cpp # test entry point
  LIBS
    ecflow_all
    Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
    $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
    Threads::Threads
    test_support
  TEST_DEPENDS
    u_base
)

target_clangformat(s_periodic_scheduler CONDITION ENABLE_TESTS)

# ===================================================================
# install
# ===================================================================
install (TARGETS ecflow_server DESTINATION bin)
