set(srcs
    DirectoryHandler.cpp
    IconProvider.cpp
    LocalSocketServer.cpp
    LogData.cpp    
    LogModel.cpp
    LogTruncator.cpp
    LogView.cpp
    Palette.cpp   
    TextFormat.cpp
    UIDebug.cpp
    UiLog.cpp
    UserMessage.cpp   
    )

if(ECFLOW_LOGVIEW)
    list(APPEND srcs LogLoadWidget.cpp)
endif()

if(ECFLOW_QT5)
    include_directories(${ECFLOW_QT_INCLUDE_DIR})
else()
    include(${QT_USE_FILE})
    set(ECFLOW_QT_LIBRARIES ${QT_LIBRARIES})
endif()


set(moc_files
    LocalSocketServer.hpp
    LogModel.hpp
    LogTruncator.hpp
    LogView.hpp
)

if(ECFLOW_LOGVIEW)
    list(APPEND moc_files LogLoadWidget.hpp)
endif()

set(wrap_ui_files
    LogLoadWidget.ui
    )

if(ECFLOW_LOGVIEW)
    set(wrap_ui_files
        LogLoadWidget.ui
        )
endif()


if(ECFLOW_QT5)
  QT5_WRAP_CPP(LIBVIEWER_MOC ${moc_files} )
  #QT5_ADD_RESOURCES (LIBVIEWER_RES viewer.qrc)
  QT5_WRAP_UI       (LIBVIEWER_FORMS_HEADERS ${wrap_ui_files})
else()
  QT4_WRAP_CPP(LIBVIEWER_MOC ${moc_files} )
  #QT4_ADD_RESOURCES (LIBVIEWER_RES viewer.qrc)
  QT4_WRAP_UI       (LIBVIEWER_FORMS_HEADERS ${wrap_ui_files})
endif()


# add all the images as dependencies of the resource file so that it is
# automatically recompiled when an image changes
#file( GLOB image_files "${CMAKE_CURRENT_SOURCE_DIR}/../images/*.*" )
#ADD_CUSTOM_TARGET(Qt_resource_cpp DEPENDS ${VIEWER_RES})
#ADD_DEPENDENCIES(Qt_resource_cpp ${image_files})

add_definitions( -DECFLOW_SHARED_DIR="${CMAKE_INSTALL_PREFIX}/share/ecflow" )

ecbuild_add_library( TARGET viewer
                     NOINSTALL
                     TYPE     STATIC
                     SOURCES  ${srcs} ${LIBVIEWER_MOC} ${LIBVIEWER_RES} ${LIBVIEWER_FORMS_HEADERS}
                     INCLUDES .
                              ../../../ACore/src
                              ${Boost_INCLUDE_DIRS}
                              ${CMAKE_CURRENT_BINARY_DIR}
                    )


# This ensures that for debug config, we only link with debug boost libs, for other configs, we link with optimised boost libs
target_link_libraries(viewer debug ${Boost_REGEX_LIBRARY_DEBUG}   ${Boost_REGEX_LIBRARY_RELEASE} )
