# Note:
# If new src or test cpp files are added make sure you touch this file
#
file( GLOB srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*.cpp" )

ecbuild_add_library( TARGET   nodeattr
                   TYPE     STATIC
                   SOURCES  ${srcs}
                   LIBS     core ${Ecflow_Boost_LIBS}
                   INCLUDES ../ACore/src ${Boost_INCLUDE_DIRS} )

file( GLOB test_srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "test/*.cpp" )

ecbuild_add_test( TARGET   nodeattrtest
                SOURCES  ${test_srcs}
                LIBS     core nodeattr  ${Ecflow_Test_LIBS}
                INCLUDES ../ACore/src src ${Boost_INCLUDE_DIRS} )
