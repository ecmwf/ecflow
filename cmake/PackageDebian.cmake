#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "ecflow")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(DEPENDS "libboost-all-dev")
if(ENABLE_SSL)
  set(DEPENDS "${DEPENDS}, libssl-dev")
endif()
if(ENABLE_UI)
  if(QT_VERSION VERSION_LESS "6.0.0")
    set(DEPENDS "${DEPENDS}, libqt5widgets5, libqt5network5, libqt5gui5, libqt5core5a")
  else()
    set(DEPENDS "${DEPENDS}, libqt6widgets6, libqt6network6, libqt6gui6, libqt6core6")
  endif()
endif()
set(CPACK_DEBIAN_PACKAGE_DEPENDS "${DEPENDS}")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "software@ecmwf.int")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr/local")

include(CPack)
