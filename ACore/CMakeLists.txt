# Note:
# If new src or test is added make sure you touch this file
#

file( GLOB srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*.cpp" )

ecbuild_add_library( TARGET core
                     NOINSTALL
                     TYPE     STATIC
                     SOURCES  ${srcs}
                     LIBS     ${Ecflow_Boost_LIBS}
                     INCLUDES ${Boost_INCLUDE_DIRS} )

file( GLOB test_srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "test/*.cpp" )

ecbuild_add_test( TARGET   coretest
                SOURCES  ${test_srcs}
                LIBS     core  ${Ecflow_Test_LIBS}
                INCLUDES src ${Boost_INCLUDE_DIRS} )
