#
# Copyright 2023- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

set(srcs
  # Headers
  DirectoryHandler.hpp
  IconProvider.hpp
  LocalSocketServer.hpp
  LogConsumer.hpp
  LogData.hpp
  LogModel.hpp
  LogTruncator.hpp
  LogView.hpp
  MessageLabel.hpp
  Palette.hpp
  TextFormat.hpp
  UIDebug.hpp
  UiLog.hpp
  UserMessage.hpp
  $<$<BOOL:${ECFLOW_LOGVIEW}>:LogLoadData.hpp>
  # Sources
  DirectoryHandler.cpp
  IconProvider.cpp
  LocalSocketServer.cpp
  LogConsumer.hpp
  LogData.cpp
  LogModel.cpp
  LogTruncator.cpp
  LogView.cpp
  MessageLabel.cpp
  Palette.cpp
  TextFormat.cpp
  UIDebug.cpp
  UiLog.cpp
  UserMessage.cpp
  $<$<BOOL:${ECFLOW_LOGVIEW}>:LogLoadData.cpp>
)

set(moc_files
  LocalSocketServer.hpp
  LogModel.hpp
  LogTruncator.hpp
  LogView.hpp
  MessageLabel.hpp
)

if(ECFLOW_LOGVIEW)
  list(APPEND moc_files LogLoadData.hpp)
endif()

if(ECFLOW_QT6)
  qt6_wrap_cpp(LIBVIEWER_MOC ${moc_files} TARGET viewer)
  qt6_wrap_ui(LIBVIEWER_FORMS_HEADERS ${wrap_ui_files})
elseif(ECFLOW_QT5)
  qt5_wrap_cpp(LIBVIEWER_MOC ${moc_files} TARGET viewer)
  qt5_wrap_ui(LIBVIEWER_FORMS_HEADERS ${wrap_ui_files})
endif()

ecbuild_add_library(
  TARGET
    viewer
  NOINSTALL
  TYPE STATIC
  SOURCES
    ${srcs}
    ${LIBVIEWER_MOC}
    ${LIBVIEWER_RES}
    ${LIBVIEWER_FORMS_HEADERS}
  PUBLIC_INCLUDES
    .
  PRIVATE_INCLUDES
    ${CMAKE_CURRENT_BINARY_DIR}
  PRIVATE_DEFINITIONS
    ECFLOW_SHARED_DIR="${CMAKE_INSTALL_PREFIX}/share/ecflow"
  PRIVATE_LIBS
    core
    Boost::boost
    $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
    # Qt5
    $<$<TARGET_EXISTS:Qt5::Core>:Qt5::Core>
    $<$<TARGET_EXISTS:Qt5::Gui>:Qt5::Gui>
    $<$<TARGET_EXISTS:Qt5::Svg>:Qt5::Svg>
    $<$<TARGET_EXISTS:Qt5::Widgets>:Qt5::Widgets>
    $<$<TARGET_EXISTS:Qt5::Network>:Qt5::Network>
    $<$<TARGET_EXISTS:Qt5::Charts>:Qt5::Charts>
    # Qt6
    $<$<TARGET_EXISTS:Qt6::Core>:Qt6::Core>
    $<$<TARGET_EXISTS:Qt6::Gui>:Qt6::Gui>
    $<$<TARGET_EXISTS:Qt6::Svg>:Qt6::Svg>
    $<$<TARGET_EXISTS:Qt6::Widgets>:Qt6::Widgets>
    $<$<TARGET_EXISTS:Qt6::Network>:Qt6::Network>
    $<$<TARGET_EXISTS:Qt6::Charts>:Qt6::Charts>
    $<$<TARGET_EXISTS:Qt6::Core5Compat>:Qt6::Core5Compat>
)
target_clangformat( viewer )
