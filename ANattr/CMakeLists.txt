# Note:
# If new src or test cpp files are added make sure you touch this file
#
file( GLOB srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*.cpp" )

ecbuild_add_library( TARGET   nodeattr
                     NOINSTALL
                     TYPE     STATIC
                     SOURCES  ${srcs}
                     LIBS     core  
                     INCLUDES src
                              ../ACore/src 
                    )

file( GLOB test_srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "test/*.cpp" )

ecbuild_add_test( TARGET       u_anattr
                  SOURCES      ${test_srcs}
                  INCLUDES     ${Boost_INCLUDE_DIRS}
                  LIBS         nodeattr ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_TEST_EXEC_MONITOR_LIBRARY}
                  TEST_DEPENDS u_acore
                 )
