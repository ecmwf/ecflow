#!/usr/bin/env python
from distutils.core import setup, Extension
import os
import sys
import glob

# ========================================================================
# Usage:
#   cd $WK; cd ecbuild/release/; sh -x ../../cmake.sh release; make ; make install
#   calling 'sh -x ../../cmake.sh release' will generate setup.py from setup.py.in
# Issues:
#   Can not test cmake install, since it will always install to 
#     /usr/local/apps/python/current/lib/python2.7/site-packages/ecflow
#
# To test install manually, we can:
#   python3 setup.py --help-commands # help
#
#   cd $WK/Pyext
#   rm -rf build/                     # for a clean build
#   python setup.py build_ext         # build C/C++ extensions (compile/link to build directory)
#   python setup.py install --home=~  # ln -s ecflow.so ecflow.cpython-36m-x86_64-linux-gnu.so , otherwise puthon3 -c "import ecflow" not working
#
# See: http://docs.python.org/2/distutils/apiref.html?highlight=extension#distutils.core.Extension
#
# ==========================================================================
# Permissions: The permission of configured file are wrong: 2 choices
# o Make sure origin file, has the right permissions
# o Copy file to different directory, and change the permissions
#   since file(COPY) does rename files
#   configure_file(setup.py.in /tmp/ma0/workspace/bdir5/release/ecflow/CMakeFiles/setup.py)
#       now copy the temporary into the final destination, setting the permissions
#   file(COPY /tmp/ma0/workspace/bdir5/release/ecflow/CMakeFiles/setup.py
#     DESTINATION /tmp/ma0/workspace/bdir5/release/ecflow
#     FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
#     GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
# ============================================================================
        
#=============================================================================
# define the directories to search for include files
# to get this to work, you will need to include the path
# to your boost installation and  ecflow includes
boost_root=os.getenv("BOOST_ROOT") 
include_dirs = [ "/tmp/ma0/workspace/ecflow/Pyext/../cereal/include",
                 "/tmp/ma0/workspace/ecflow/Pyext/../ACore/src", 
                 "/tmp/ma0/workspace/ecflow/Pyext/../ANattr/src", 
                 "/tmp/ma0/workspace/ecflow/Pyext/../ANode/parser/src",
                 "/tmp/ma0/workspace/ecflow/Pyext/../ANode/src",
                 "/tmp/ma0/workspace/ecflow/Pyext/../Base/src",
                 "/tmp/ma0/workspace/ecflow/Pyext/../Base/src/cts",
                 "/tmp/ma0/workspace/ecflow/Pyext/../Base/src/stc",
                 "/tmp/ma0/workspace/ecflow/Pyext/../CSim/src", 
                 "/tmp/ma0/workspace/ecflow/Pyext/../Client/src",
                 "/tmp/ma0/workspace/ecflow/Pyext/src",
                 boost_root,
               ]

# define the library directories to include any extra libraries that may be needed.
# Give preference to release libs   
boost_lib_dir = boost_root + "/stage/lib/"
library_dirs = ['/tmp/ma0/workspace/bdir5/release/ecflow/ACore',
                '/tmp/ma0/workspace/bdir5/release/ecflow/ANattr/',
                '/tmp/ma0/workspace/bdir5/release/ecflow/ANode/',
                '/tmp/ma0/workspace/bdir5/release/ecflow/Base/', 
                '/tmp/ma0/workspace/bdir5/release/ecflow/CSim/', 
                '/tmp/ma0/workspace/bdir5/release/ecflow/Client/', 
                boost_lib_dir  
               ]

# define the libraries to link with this includes the boost lib
libraries = [ 'core' , 'nodeattr', 'node', 'base', 'libsimu', 'libclient',
              'boost_system',
              'boost_filesystem',
              'boost_program_options',
              'boost_date_time', 
              'boost_python36' ]

# Using gcc-4.8 with boost 1.53 linux requires these flags
extra_compile_args = ['-ftemplate-depth-512','-Wno-unused-variable','-Wno-deprecated-declarations','-Wno-unused-local-typedefs','-Wno-maybe-uninitialized' ]
extra_link_args = []

# create the extension and add it to the python distribution
# o glob.glob(os.path.join('src', '*.cpp'))
#   This expand the list of cpp files that need to be compiled
#
# The configuation below installs to:
#   o /usr/local/apps/python/current/lib/python2.7/site-packages/ecflow
#     when, "python setup.py install" is used:
#   lib/python2.7/site-packages/ecflow/ecflow.so
#                                     __init__.py               
setup( name='ecflow', 
       version='5.3.1', 
       author      = 'ECMWF',
       description = """ecflow Python interface""",
       packages = [ 'ecflow' ],
       url = 'https://confluence.ecmwf.int/display/ECFLOW/ecflow+home',
       license = 'Apache',
       package_dir={'ecflow': '/tmp/ma0/workspace/ecflow/Pyext/ecflow'},
       ext_modules=[ Extension( 
                              'ecflow.ecflow', 
                              glob.glob(os.path.join('/tmp/ma0/workspace/ecflow/Pyext/src', '*.cpp')), 
                              include_dirs=include_dirs, 
                              library_dirs=library_dirs, 
                              libraries=libraries,
                              extra_compile_args=extra_compile_args,
                              extra_link_args=extra_link_args
                              )
                    ],
   )
