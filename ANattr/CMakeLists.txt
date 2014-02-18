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

ecbuild_add_test( TARGET   u_anattr
                  BOOST
                  SOURCES  ${test_srcs}
                  LIBS     nodeattr
                  INCLUDES ${Boost_INCLUDE_DIRS} 
                 )
