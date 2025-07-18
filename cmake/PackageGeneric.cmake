#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

#
# Exclude specific content from sources tarball
#
#   note: by default ecbuild includes everything apart from the 'build' directory
#

ecbuild_dont_pack(
  DIRS
    # ignore local notes
    .scratch
    SCRATCH
    CUSTOMER
    # ignore source control directories
    .git
    # ignore build directories
    bamboo
    ecbuild
    build_scripts/nightly
    build_scripts/test_bench
    Debug
    bdir
    bdir_xcode
    bin
    # ignore Documentation directories
    libs/core/doc
    libs/attributes/doc
    libs/node/doc
    libs/client/doc
    libs/simulator/doc
    libs/pyext/doc
    libs/server/doc
    Doc/misc
    Doc/sphinx-examples
    # ignore IDE configuration directories
    .settings
  FILES
    # ignore IDE configuration files
    .project
    .cproject
    .pydevproject
    # ignore python ancillary files
    Pyext/.pydevproject
    Pyext/samples/test.py
    Pyext/samples/confluence_add_attachment.py
    build_scripts/.pydevproject
)

# prepares a tar.gz of your sources and/or binaries
ecbuild_install_project( NAME ecFlow )
