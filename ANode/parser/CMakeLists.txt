# Note:
# If new src or test cpp files are added make sure you touch this file
#

# ===================================================================
# Use following to populate list: 
#  cd $WK/ANode/parser                      
#  find test -name \*.cpp | sort   
list( APPEND test_srcs
   test/PersistHelper.cpp
   test/TestAutoAddExterns.cpp
   test/TestDefsStructurePersistAndReload.cpp
   test/TestMementoPersistAndReload.cpp
   test/TestMigration.cpp
   test/TestParser.cpp
   test/TestVariableParsing.cpp
)
ecbuild_add_test( TARGET       u_aparser
                  SOURCES      ${test_srcs}
                  LIBS         node nodeattr core
                               ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_TEST_EXEC_MONITOR_LIBRARY}
                  INCLUDES     src 
                               ../../ACore/src 
                               ../../ANattr/src 
                               ../test       # ANode/test
                               ../src        # ANode/src
                               ${Boost_INCLUDE_DIRS}
                  TEST_DEPENDS u_anode
                )


if (ENABLE_ALL_TESTS)
	#
	# Tests parser for a single defs file but with a range of tests
	#
	list( APPEND t2_src test/TestSingleDefsFile.cpp test/PersistHelper.cpp )                    
	ecbuild_add_test( TARGET   perf_aparser
                      SOURCES  ${t2_src}
                      LIBS     node nodeattr core
                               ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_TEST_EXEC_MONITOR_LIBRARY}
                      INCLUDES src 
                               ../../ACore/src 
                               ../../ANattr/src 
                               ../test       # ANode/test
                               ../src        # ANode/src
                               ${Boost_INCLUDE_DIRS}
                    )

	#
	# Timer for arbitary defs file,  
	#
    list( APPEND t3_src test/ParseTimer.cpp  test/PersistHelper.cpp )                    
    ecbuild_add_test( TARGET   perf_aparser_timer
                      ARGS     ${CMAKE_CURRENT_SOURCE_DIR}/test/data/single_defs/mega.def
                      SOURCES  ${t3_src}
                      LIBS     node nodeattr core 
                      INCLUDES src 
                               ../../ACore/src 
                               ../../ANattr/src 
                               ../src        # ANode/src
                               ../test       # ANode/test
                               ${Boost_INCLUDE_DIRS} 
                    )


	#
	# Tests parser for a single defs file.  
	#
	list( APPEND t4_src test/ParseOnly.cpp )                    
	ecbuild_add_test( TARGET   perf_aparser_only
                      ARGS     ${CMAKE_CURRENT_SOURCE_DIR}/test/data/single_defs/mega.def
                      SOURCES  ${t4_src}
                      LIBS     node nodeattr core
                      INCLUDES src 
                               ../../ACore/src 
                               ../../ANattr/src 
                               ../src        # ANode/src
                               ../test       # ANode/test
                               ${Boost_INCLUDE_DIRS} 
                   )
endif()