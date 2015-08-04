# Note:
# If new src or test cpp files are added make sure you touch this file
#

file( GLOB srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*.cpp" )

ecbuild_add_library( TARGET   libparser
                     NOINSTALL
                     TYPE     STATIC
                     SOURCES  ${srcs}
                     LIBS     node nodeattr core     
                     INCLUDES src
                              ../ACore/src 
                              ../ANattr/src
                              ../ANode/src
                     )

# ===================================================================
# Use following to populate list: 
#  cd $WK/AParser                      
#  find test -name \*.cpp | sort   
list( APPEND test_srcs
   test/PersistHelper.cpp
   test/TestAutoAddExterns.cpp
   test/TestDefsStructurePersistAndReload.cpp
   test/TestMigration.cpp
   test/TestParser.cpp
   test/TestVariableParsing.cpp
)
ecbuild_add_test( TARGET       u_aparser
                  BOOST
                  SOURCES      ${test_srcs}
                  LIBS         libparser 
                  INCLUDES     ../ANode/test
                  TEST_DEPENDS u_anode
                )


if (ENABLE_ALL_TESTS)
	#
	# Tests parser for a single defs file but with a range of tests
	#
	list( APPEND t2_src test/TestSingleDefsFile.cpp test/PersistHelper.cpp )                    
	ecbuild_add_test( TARGET   perf_aparser
                      BOOST
                      SOURCES  ${t2_src}
                      LIBS     libparser
                    )

	#
	# Timer for arbitary defs file,  
	#
    list( APPEND t3_src test/ParseTimer.cpp  test/PersistHelper.cpp )                    
    ecbuild_add_test( TARGET   perf_aparser_timer
                      ARGS     ${CMAKE_CURRENT_SOURCE_DIR}/test/data/single_defs/mega.def
                      SOURCES  ${t3_src}
                      LIBS     libparser 
                      INCLUDES ../ANode/test
                               ${Boost_INCLUDE_DIRS} 
                    )


	#
	# Tests parser for a single defs file.  
	#
	list( APPEND t4_src test/ParseOnly.cpp )                    
	ecbuild_add_test( TARGET   perf_aparser_only
                      ARGS     ${CMAKE_CURRENT_SOURCE_DIR}/test/data/single_defs/mega.def
                      SOURCES  ${t4_src}
                      LIBS     libparser
                      INCLUDES ../ANode/test
                               ${Boost_INCLUDE_DIRS} 
                   )
endif()