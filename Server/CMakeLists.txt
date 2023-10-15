 # =======================================================
 # LIB
 # to list all sources to build use:
 #    cd $WK/Server
 #    find src -name \*.cpp  -print | sort
 # =======================================================
list( APPEND srcs
   # HEADERS
   src/BaseServer.hpp
   src/CheckPtSaver.hpp
   src/NodeTreeTraverser.hpp
   src/Server.hpp
   src/ServerEnvironment.hpp
   src/ServerOptions.hpp
   src/SslServer.hpp
   src/SslTcpServer.hpp
   src/TcpBaseServer.hpp
   src/TcpServer.hpp
   # SOURCES
   src/BaseServer.cpp
   src/CheckPtSaver.cpp
   src/NodeTreeTraverser.cpp
   src/Server.cpp
   src/ServerEnvironment.cpp
   src/ServerOptions.cpp
   src/TcpBaseServer.cpp
   src/TcpServer.cpp
)

if (OPENSSL_FOUND)
	list( APPEND srcs
          # HEADERS
          src/SslServer.hpp
          src/SslTcpServer.hpp
          # SOURCES
          src/SslServer.cpp
          src/SslTcpServer.cpp
        )
endif()


ecbuild_add_library( TARGET libserver
                     NOINSTALL
                     TYPE     STATIC
                     SOURCES  ${srcs}
                     )
target_link_libraries(libserver base node nodeattr core pthread )
target_include_directories(libserver PRIVATE src
                                             ../ACore/src 
                                             ../ANattr/src
                                             ../ANode/src
                                             ../Base/src
                                             ../Base/src/cts
                                             ../Base/src/stc 
)

target_clangformat(libserver)

# =======================================================
# EXE ecflow_server, if OpenSSL not enabled ${OPENSSL_LIBRARIES}, is empty

ecbuild_add_executable( TARGET   ecflow_server
                        SOURCES  src/ServerMain.cpp
                        LIBS     libserver ${OPENSSL_LIBRARIES}
                        INCLUDES ${Boost_INCLUDE_DIRS} 
                       )

# Override default behaviour that add RPATHS during install
# The only thing that seem to work is set INSTALL_RPATH to ""
# Using SKIP_BUILD_RPATH,BUILD_WITH_INSTALL_RPATH,INSTALL_RPATH_USE_LINK_PATH
# had no effect
#          
SET_TARGET_PROPERTIES(ecflow_server PROPERTIES 
                                    INSTALL_RPATH ""
                     )

target_clangformat(ecflow_server)

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
# libboost_unit_test_framework  undefined reference to `clock_gettime', ${LIBRT} needed for boost 1.71
ecbuild_add_test(
  TARGET
    u_server
  INCLUDES
    src
  SOURCES
    test/TestServerEnvironment.cpp
    test/TestServer1.cpp
  LIBS
    libserver
    $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
  DEFINITIONS
    ${BOOST_TEST_DYN_LINK}
  TEST_DEPENDS
    u_base
)

target_clangformat(u_server CONDITION ENABLE_TESTS)

# ===================================================================
# install
# ===================================================================
install (TARGETS ecflow_server DESTINATION bin)
