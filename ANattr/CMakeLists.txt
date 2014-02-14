# Note:
# If new src or test cpp files are added make sure you touch this file
#
file( GLOB srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*.cpp" )

ecbuild_add_library( TARGET   nodeattr
                     NOINSTALL
                     TYPE     STATIC
                     SOURCES  ${srcs}
                     LIBS     core  
                     INCLUDES ../ACore/src 
                              ${Boost_INCLUDE_DIRS} 
                    )

file( GLOB test_srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "test/*.cpp" )

ecbuild_add_test( TARGET   u_anattr
                  SOURCES  ${test_srcs}
                  LIBS     core nodeattr
                           ${Boost_SYSTEM_LIBRARY}
                           ${Boost_SERIALIZATION_LIBRARY}
                           ${Boost_FILESYSTEM_LIBRARY}
                           ${Boost_DATE_TIME_LIBRARY}  
                           ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} 
                  INCLUDES ../ACore/src src 
                           ${Boost_INCLUDE_DIRS} 
                 )
