# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

#
# Exclude specific content from sources tarball
#
#   note: by default ecbuild includes everything apart from the 'build' directory
#

ecbuild_dont_pack(
  DIRS
    # ignore local notes
    .scratch
    .sandbox
    .scratch
    SCRATCH
    CUSTOMER
    # ignore source control directories
    .git
    # ignore build directories
    .deploy
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
    libs/test/simulator/doc
    libs/pyext/doc
    libs/server/doc
    # ignore IDE configuration directories
    .settings
    .vscode
  FILES
    # ignore IDE configuration files
    .project
    .cproject
    .pydevproject
    # ignore python ancillary files
    libs/pyext/samples/test.py
    libs/pyext/samples/confluence_add_attachment.py
)

# prepares a tar.gz of your sources and/or binaries
ecbuild_install_project( NAME ecFlow )
