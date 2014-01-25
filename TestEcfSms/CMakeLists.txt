# =======================================================
# to list all sources to build use:
#    cd $WK/Base
#    find src -name \*.cpp -print | sort
# =======================================================
file( GLOB srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*.cpp" )
 
ecbuild_add_library( TARGET libecfsms
                   NOINSTALL
                   TYPE     STATIC
                   SOURCES  ${srcs}
                   LIBS     core nodeattr node libparser base libclient ${Ecflow_Boost_LIBS}
                   INCLUDES ../ACore/src 
                            ../ANattr/src
                            ../ANode/src
                            ../AParser/src
                            ../Base/src
                            ../Base/src/cts
                            ../Base/src/stc
                            ../Base/test
                            ../Client/src
                            ${Boost_INCLUDE_DIRS} )


# =================================================================
# test: *** takes too long hence commented out
# =================================================================
#ecbuild_add_test( TARGET   smsecftest
#                SOURCES  test/TestSmsEcf.cpp
#                LIBS     core nodeattr node libparser base libclient libecfsms pthread ${Ecflow_Test_LIBS}
#                INCLUDES  src
#                          ../ACore/src 
#                          ../ANattr/src
#                          ../ANode/src
#                          ../ANode/test
#                          ../Base/src
#                          ../Client/src
#                          ../Client/test
#                          ../TestEcfSms/test
#                          ${Boost_INCLUDE_DIRS} )


