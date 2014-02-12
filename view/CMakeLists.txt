# =======================================================
# LIB
# to list all sources to build use:
#    cd $WK/View
#    find src -name \icon_*.cc  -print | sort
# =======================================================

# Files to exclude
#       src/lister.cc 
#       src/extent.cc
#       src/node_alert.cc 
#       src/array.cc 
#       src/option.cc 
#       src/dialog.cc 
#       src/ecflowview.cc
#
#       src/menul.c 
#       src/tmpnam.c 
#       src/line.c 
#       src/x.c 
       
list( APPEND srcs_cc
   src/aborted.cc
   src/alerts.cc
   src/alias.cc
   src/ask.cc
   src/auto_alarm.cc
   src/base.cc
   src/collector.cc
   src/colors_prefs.cc
   src/configurable.cc
   src/confirm.cc
   src/counted.cc
   src/date.cc
   src/depend.cc
   src/directory.cc
   src/dummy_node.cc
   src/ecf_node.cc
   src/ecflow.cc
   src/edit.cc
   src/edit_label.cc
   src/edit_limit.cc
   src/edit_meter.cc
   src/edit_repeat.cc
   src/edit_variable.cc
   src/editor.cc
   src/error.cc
   src/event_node.cc
   src/external.cc
   src/find.cc
   src/flags.cc
   src/fonts_prefs.cc
   src/fsb.cc
   src/globals.cc
   src/graph_layout.cc
   src/gui.cc
   src/history.cc
   src/host.cc
   src/host_prefs.cc
   src/html_lister.cc
   src/http.cc
   src/hyper_lister.cc
   src/icon_Josstatus3.cc
   src/icon_W.cc
   src/icon_byrule.cc
   src/icon_cmd_failed.cc
   src/icon_edit_failed.cc
   src/icon_force_abort.cc
   src/icon_killed.cc
   src/icon_no_script.cc
   src/icon_queuelimit.cc
   src/icon_task_aborted.cc
   src/icon_user_edit.cc
   src/info.cc
   src/init.cc
   src/inlimit_node.cc
   src/input.cc
   src/interface.cc
   src/job.cc
   src/jobcheck_panel.cc
   src/jobstatus.cc
   src/label.cc
   src/late.cc
   src/late_node.cc
   src/layout.cc
   src/limit_node.cc
   src/log_event.cc
   src/logsvr.cc
   src/mail.cc
   src/manual.cc
   src/menu_prefs.cc
   src/menus.cc
   src/messages.cc
   src/meter_node.cc
   src/node.cc
   src/node_editor.cc
   src/node_list.cc
   src/node_window.cc
   src/not_enqueued.cc
   src/observable.cc
   src/observer.cc
   src/option_panel.cc
   src/output.cc
   src/panel.cc
   src/panel_window.cc
   src/parser.cc
   src/passwrd.cc
   src/persist.cc
   src/pixmap.cc
   src/pref_editor.cc
   src/pref_window.cc
   src/prefs.cc
   src/reach.cc
   src/relation.cc
   src/repeat_node.cc
   src/resource.cc
   src/restart.cc
   src/result.cc
   src/runnable.cc
   src/script_panel.cc
   src/scripting.cc
   src/search.cc
   src/searchable.cc
   src/selection.cc
   src/server.cc
   src/servers_prefs.cc
   src/show.cc
   src/simple_node.cc
   src/str.cc
   src/substitute.cc
   src/suites_panel.cc
   src/super_node.cc
   src/task_node.cc
   src/text_layout.cc
   src/text_window.cc
   src/time.cc
   src/timeout.cc
   src/timetable_panel.cc
   src/tip.cc
   src/tmp_file.cc
   src/top.cc
   src/translator.cc
   src/tree.cc
   src/trigger_node.cc
   src/trigger_panel.cc
   src/url.cc
   src/user_prefs.cc
   src/users.cc
   src/variable_node.cc
   src/variables.cc
   src/viewer.cc
   src/why.cc
   src/window.cc
   src/xdxmdialog.cc
   src/xdxtclass.cc
   src/xmstring.cc
   src/xnode.cc
   src/zombies_panel.cc
)

list( APPEND srcs_c
   src/Hyper.c
   src/SimpleBase.c
   src/SimpleGraph.c
   src/SimpleTime.c
   src/SimpleTree.c
   src/Tab.c
   src/menuy.c
)

file( GLOB srcs_cpp     RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*.cpp"        )
file( GLOB libicon_srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/libicon/*.cc" )
file( GLOB libxec_srcs  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/libxec/*.c"   )
file( GLOB libui_srcs   RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/libui/*.cc"   )

# ======================================================================================

find_package( X11 )
debug_var( X11_INCLUDE_DIR )
debug_var( X11_LIBRARIES )  
debug_var( X11_Xt_INCLUDE_PATH )
debug_var( X11_Xt_LIB )
debug_var( X11_Xpm_INCLUDE_PATH )
debug_var( X11_Xpm_LIB )

find_package( Motif )
debug_var( MOTIF_INCLUDE_DIR )
debug_var( MOTIF_LIBRARIES )  
if ( NOT MOTIF_FOUND )
   MESSAGE( "Overriding motif library" )
   set(MOTIF_LIBRARIES "/usr/lib64/libXm.so.4" )
endif()
debug_var( MOTIF_LIBRARIES )  

# =======================================================================================
find_package( CMath )

set( CMAKE_THREAD_PREFER_PTHREAD TRUE )
find_package( Threads )


# =========================================================================================
# libecflowview needs to be compiled as dynamic lin  otherwise the icons do not appear.
#
add_definitions( -D_GNU_SOURCE -DUNIX -Dunix -Dlinux )

ecbuild_add_library(TARGET  libecflowview
				       CONDITION X11_FOUND AND CMAKE_THREAD_LIBS_INIT
                   SOURCES  
                             ${srcs_cc} ${srcs_c} ${srcs_cpp} ${libicon_srcs} ${libxec_srcs} ${libui_srcs}
                   TEMPLATES
                             src/array.cc
                   INCLUDES 
                            src/libicon
                            src/libxec
                            src/libui
                            ../ACore/src 
                            ../ANattr/src
                            ../ANode/src
                            ../AParser/src
                            ../Base/src
                            ../Base/src/cts
                            ../Base/src/stc
                            ../Client/src
                            ${Boost_INCLUDE_DIRS} 
                            ${X11_INCLUDE_DIR}
                            ${MOTIF_INCLUDE_DIR}
                   )

# =======================================================================
# EXE ecflowview
#    o crypt    Does not appear to be used           ?
#    o Xp       libxprint does not appear to be used ?
#    o fl       fast lexical analyser generator, not used ?
# ========================================================================

ecbuild_add_executable( TARGET    ecflowview
                        SOURCES   src/ecflowview.cc
						      CONDITION X11_FOUND AND CMAKE_THREAD_LIBS_INIT
                        LIBS      libecflowview core nodeattr node libparser base libclient 
                                  ${CMAKE_THREAD_LIBS_INIT} 
                                  ${Ecflow_Boost_LIBS}
                                  ${X11_LIBRARIES} ${X11_Xext_LIB} ${MOTIF_LIBRARIES} ${X11_Xpm_LIB} ${X11_Xt_LIB} 
                                  ${CMATH_LIBRARIES}
                        INCLUDES  src
                                  ${X11_INCLUDE_DIR}
                                  ${MOTIF_INCLUDE_DIR}
                                  ${Boost_INCLUDE_DIRS} 
                     )


# ===================================================================
# test
# ===================================================================

ecbuild_add_test( TARGET  test-view
                SOURCES   test/TestRunner.cpp test/TestView.cpp
                LIBS      core nodeattr node libparser base libclient libharness ${CMAKE_THREAD_LIBS_INIT} ${Ecflow_Test_LIBS}
                INCLUDES  src
                          ../ACore/src 
                          ../ANattr/src
                          ../ANode/src
                          ../Base/src
                          ../Base/test
                          ../Test/src
                          ${Boost_INCLUDE_DIRS} )
                          

# ===================================================================
# install
# ===================================================================
                     
install (FILES  ${CMAKE_SOURCE_DIR}/view/src/ecflowview.menu 
                ${CMAKE_SOURCE_DIR}/build/servers
         DESTINATION lib
         PERMISSIONS OWNER_READ GROUP_READ WORLD_READ OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE
        )
         
