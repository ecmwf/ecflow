
set(viewer_srcs   
    LogViewerMain.cpp
    #LogLoadWidget.cpp
    LogMainWindow.cpp
   )

if(ECFLOW_QT5)
    include_directories(${ECFLOW_QT_INCLUDE_DIR})
else()
    include(${QT_USE_FILE}) 
    set(ECFLOW_QT_LIBRARIES ${QT_LIBRARIES})
endif()


set(viewer_moc_files
    #LogLoadWidget.hpp
    LogMainWindow.hpp
)

set(viewer_wrap_ui_files   
    #LogLoadWidget.ui
    LogMainWindow.ui
)

if(ECFLOW_QT5)
  QT5_WRAP_CPP(VIEWER_MOC ${viewer_moc_files} )
  #QT5_ADD_RESOURCES (VIEWER_RES viewer.qrc)
  QT5_WRAP_UI       (VIEWER_FORMS_HEADERS ${viewer_wrap_ui_files})
else()
  QT4_WRAP_CPP(VIEWER_MOC ${viewer_moc_files} )
  #QT4_ADD_RESOURCES (VIEWER_RES viewer.qrc)
  QT4_WRAP_UI       (VIEWER_FORMS_HEADERS ${viewer_wrap_ui_files})
endif()


# add all the images as dependencies of the resource file so that it is
# automatically recompiled when an image changes
#file( GLOB image_files "${CMAKE_CURRENT_SOURCE_DIR}/../images/*.*" )
#ADD_CUSTOM_TARGET(Qt_resource_cpp DEPENDS ${VIEWER_RES})
#ADD_DEPENDENCIES(Qt_resource_cpp ${image_files})


add_definitions( -DECFLOW_SHARED_DIR="${CMAKE_INSTALL_PREFIX}/share/ecflow" )

ecbuild_add_executable( TARGET   ecflow_ui_log.x
    SOURCES  ${viewer_srcs} ${VIEWER_MOC} ${VIEWER_RES} ${VIEWER_FORMS_HEADERS}
    INCLUDES .
     ../../libViewer/src
     ../../../ACore/src
     ../../../ANattr/src
     ../../../ANode/src
     ../../../ANode/test
     ../../../Base/src
     ../../../Base/src/cts
     ../../../Base/src/stc
     ../../../Client/src    
     ${Boost_INCLUDE_DIRS} 
     ${CMAKE_CURRENT_BINARY_DIR}
    LIBS viewer core nodeattr node base libclient
         pthread  
         m dl
         ${ECFLOW_QT_LIBRARIES}
         ${OPENSSL_LIBRARIES}
)

# This ensures that for debug config, we only link with debug boost libs, for other configs, we link with optimised boost libs
target_link_libraries(ecflow_ui_log.x debug ${Boost_REGEX_LIBRARY_DEBUG}   ${Boost_REGEX_LIBRARY_RELEASE} )

