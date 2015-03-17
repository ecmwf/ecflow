# Note:
# If new src or test is added make sure you touch this file
#

# We place generated file in /ACore/src/ so that we can still use boost build
configure_file( ecflow_version.h.in ${CMAKE_SOURCE_DIR}/ACore/src/ecflow_version.h )

# Use transitive nature: i.e if any lib/exe uses lib core, they
# will also inherit the boost libs.
#
file( GLOB srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*.cpp" )
ecbuild_add_library( TARGET core
                     NOINSTALL
                     TYPE     STATIC
                     SOURCES  ${srcs}
                     INCLUDES src
                              ${Boost_INCLUDE_DIRS} 
                    )

# This ensures that for debug config, we only link with debug boost libs, for other configs, we link with optimised boost libs
target_link_libraries(core debug ${Boost_SERIALIZATION_LIBRARY_DEBUG}   ${Boost_SERIALIZATION_LIBRARY_RELEASE}
                           debug ${Boost_SYSTEM_LIBRARY_DEBUG}          ${Boost_SYSTEM_LIBRARY_RELEASE}  
                           debug ${Boost_FILESYSTEM_LIBRARY_DEBUG}      ${Boost_FILESYSTEM_LIBRARY_RELEASE}
                           debug ${Boost_DATE_TIME_LIBRARY_DEBUG}       ${Boost_DATE_TIME_LIBRARY_RELEASE})                
                   
#
# By use'ing BOOST tag we indicate this test should link with boost test libs
#
file( GLOB test_srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "test/*.cpp" )
ecbuild_add_test( TARGET   u_acore 
                  BOOST
                  SOURCES  ${test_srcs}
                  LIBS     core                           
                )
