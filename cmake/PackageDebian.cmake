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
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libboost-all-dev, libssl-dev")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "software@ecmwf.int")
set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/ecmwf")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS "ON")

include(CPack)
