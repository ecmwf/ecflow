#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#


add_subdirectory( core )
add_subdirectory( service )
add_subdirectory( attribute )
add_subdirectory( node )
add_subdirectory( base )
add_subdirectory( simulator )
add_subdirectory( client )

if (ENABLE_SERVER)
  add_subdirectory( server )
  add_subdirectory( test )
endif()

if (ENABLE_PYTHON)
  if ( ENABLE_PYTHON_PTR_REGISTER )
    add_definitions( -DECF_ENABLE_PYTHON_PTR_REGISTER )
  endif()
  add_subdirectory( pyext )
endif()

if (ENABLE_HTTP)
  add_subdirectory( rest )
endif()

if (ENABLE_UDP)
  add_subdirectory( udp )
endif()
