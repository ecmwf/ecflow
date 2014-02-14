# Note:
# If new src or test is added make sure you touch this file
#

file( GLOB srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*.cpp" )
ecbuild_add_library( TARGET core
                     NOINSTALL
                     TYPE     STATIC
                     SOURCES  ${srcs}
                     INCLUDES ${Boost_INCLUDE_DIRS} 
                    )


file( GLOB test_srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "test/*.cpp" )
ecbuild_add_test( TARGET   u_acore
                  SOURCES  ${test_srcs}
                  LIBS     core  
                           ${Boost_SYSTEM_LIBRARY}
                           ${Boost_SERIALIZATION_LIBRARY}
                           ${Boost_FILESYSTEM_LIBRARY}
                           ${Boost_DATE_TIME_LIBRARY}  
                           ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} 
                  INCLUDES src 
                           ${Boost_INCLUDE_DIRS} 
                )
