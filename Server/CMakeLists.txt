 # =======================================================
 # LIB
 # to list all sources to build use:
 #    cd $WK/Server
 #    find src -name \*.cpp  -print | sort
 # =======================================================
list( APPEND srcs
   src/CConnection.cpp
   src/CheckPtSaver.cpp
   src/NodeTreeTraverser.cpp
   src/Server.cpp
   src/ServerEnvironment.cpp
   src/ServerOptions.cpp
)
ecbuild_add_library( TARGET libserver
                     NOINSTALL
                     TYPE     STATIC
                     SOURCES  ${srcs}
                     LIBS     core nodeattr node libparser base  
                     INCLUDES ../ACore/src 
                              ../ANattr/src
                              ../ANode/src
                              ../AParser/src
                              ../Base/src
                              ../Base/src/cts
                              ../Base/src/stc
                              ${Boost_INCLUDE_DIRS} )

# =======================================================
# EXE ecflow_server
ecbuild_add_executable( TARGET   ecflow_server
                        SOURCES  src/ServerMain.cpp
                        LIBS     core nodeattr node libparser base libserver pthread ${Ecflow_Boost_LIBS}
                        INCLUDES src
                                 ${Boost_INCLUDE_DIRS} 
                      )
                          
# ================================================================================                         
# TEST
ecbuild_add_test( TARGET u_server
                SOURCES  test/TestServerEnvironment.cpp
                LIBS     core nodeattr node libparser base libserver pthread ${Ecflow_Test_LIBS}
                INCLUDES src
                         ../ACore/src 
                          ${Boost_INCLUDE_DIRS} 
              )

# ===================================================================
# install
# ===================================================================
install (TARGETS ecflow_server DESTINATION bin)
