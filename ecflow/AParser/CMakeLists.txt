# Note:
# If new src or test cpp files are added make sure you touch this file
#

file( GLOB srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*.cpp" )

ecbuild_add_library( TARGET   libparser
                   TYPE     STATIC
                   SOURCES  ${srcs}
                   LIBS     core nodeattr node ${Ecflow_Boost_LIBS}
                   INCLUDES ../ACore/src 
                            ../ANattr/src
                            ../ANode/src
                            ${Boost_INCLUDE_DIRS} )

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
ecbuild_add_test( TARGET tparser
                SOURCES  ${test_srcs}
                LIBS     core nodeattr node libparser ${Ecflow_Test_LIBS}
                INCLUDES src
                         ../ACore/src 
                         ../ANattr/src
                         ../ANode/src
                         ${Boost_INCLUDE_DIRS} 
                 )



#
# Tests parser for a single defs file but with a range of tests
#
list( APPEND t2_src test/TestSingleDefsFile.cpp test/PersistHelper.cpp )                    
ecbuild_add_test( TARGET   tsingle
                SOURCES  ${t2_src}
                LIBS     core nodeattr node libparser ${Ecflow_Test_LIBS}
                INCLUDES  src
                          ../ACore/src 
                          ../ANattr/src
                          ../ANode/src
                          ../ANode/test
                          ${Boost_INCLUDE_DIRS} )

#
# Timer for arbitary defs file,  
#
list( APPEND t3_src test/ParseTimer.cpp  test/PersistHelper.cpp )                    
ecbuild_add_test( TARGET   parse_timer
                SOURCES  ${t3_src}
                LIBS     core nodeattr node libparser ${Ecflow_Test_LIBS}
                INCLUDES  src
                          ../ACore/src 
                          ../ANattr/src
                          ../ANode/src
                          ../ANode/test
                          ${Boost_INCLUDE_DIRS} )


#
# Tests parser for a single defs file.  
#
list( APPEND t4_src test/ParseOnly.cpp )                    
ecbuild_add_test( TARGET   parse_only
                SOURCES  ${t4_src}
                LIBS     core nodeattr node libparser ${Ecflow_Test_LIBS}
                INCLUDES  src
                          ../ACore/src 
                          ../ANattr/src
                          ../ANode/src
                          ../ANode/test
                          ${Boost_INCLUDE_DIRS} )

 
