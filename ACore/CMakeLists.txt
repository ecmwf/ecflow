# Note:
# If new src or test is added make sure you touch this file
#

# Use transitive nature: i.e if any lib/exe uses lib core, they
# will also inherit the boost libs.
#
file( GLOB srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*.cpp" )
ecbuild_add_library( TARGET core
                     NOINSTALL
                     TYPE     STATIC
                     SOURCES  ${srcs}
                     INCLUDES ${Boost_INCLUDE_DIRS} 
                     LIBS     ${Boost_SYSTEM_LIBRARY}
                              ${Boost_SERIALIZATION_LIBRARY}
                              ${Boost_FILESYSTEM_LIBRARY}
                              ${Boost_DATE_TIME_LIBRARY}  
                    )

#
# By use'ing BOOST tag we indicate this test should link with boost test libs
#
file( GLOB test_srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "test/*.cpp" )
ecbuild_add_test( TARGET   u_acore 
                  BOOST
                  SOURCES  ${test_srcs}
                  LIBS     core                           
                  INCLUDES src 
                )
