# Note:
# If new src or test is added make sure you touch this file
#


# crypt is not needed on Apple machines
if(NOT DEFINED APPLE)
    set(CRYPT_LIB crypt)
endif()

# Only really needed for File.cpp
add_definitions( -DCMAKE )

# We place generated file in /ACore/src/ so that we can still use boost build
configure_file( ecflow_version.h.in ${CMAKE_SOURCE_DIR}/ACore/src/ecflow_version.h )

# place in binary directory since this is different for each user
configure_file( ecflow_source_build_dir.h.in ${CMAKE_BINARY_DIR}/ecflow_source_build_dir.h )
		
# Use transitive nature: i.e if any lib/exe uses lib core, they
# will also inherit the boost libs.
#
file( GLOB srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*.cpp" "src/*.hpp" "src/*.h" )
ecbuild_add_library( TARGET core
                     NOINSTALL
                     TYPE     STATIC
                     SOURCES  ${srcs}
                    )
target_include_directories(core PUBLIC src
                                       ${Boost_INCLUDE_DIRS}
                                       ${CMAKE_BINARY_DIR} )

# This ensures that for debug config, we only link with debug boost libs, for other configs, we link with optimised boost libs
if ( Boost_VERSION_STRING VERSION_LESS "1.69.0" )
   target_link_libraries(core ${Boost_FILESYSTEM_LIBRARY_RELEASE}
                              ${Boost_DATE_TIME_LIBRARY_RELEASE}
                              ${Boost_PROGRAM_OPTIONS_LIBRARY_RELEASE}
                              ${Boost_SYSTEM_LIBRARY_RELEASE}
                              ${CRYPT_LIB}
                           )   
else()
   # for boost version 1.69 or greater Boost.System is now header-only.
   target_link_libraries(core ${Boost_FILESYSTEM_LIBRARY_RELEASE}
                              ${Boost_DATE_TIME_LIBRARY_RELEASE}
                              ${Boost_PROGRAM_OPTIONS_LIBRARY_RELEASE}
                              ${CRYPT_LIB}
                           )   
endif()             

target_clangformat(core)

#  ${LIBRT} See: https://stackoverflow.com/questions/13653361/another-undefined-reference-error-when-linking-boost-libraries                   
file( GLOB test_srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "test/*.cpp" "test/*.hpp" )
ecbuild_add_test( TARGET   u_acore 
                  SOURCES  ${test_srcs}
                  INCLUDES src ${Boost_INCLUDE_DIRS}
                  LIBS     core ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_TEST_EXEC_MONITOR_LIBRARY}
                           ${Boost_TIMER_LIBRARY} ${Boost_CHRONO_LIBRARY} ${LIBRT}
                  DEFINITIONS ${BOOST_TEST_DYN_LINK}
                )

target_clangformat(u_acore CONDITION ENABLE_TESTS)
