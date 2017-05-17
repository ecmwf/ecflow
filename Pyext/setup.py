#!/usr/bin/env python
from distutils.core import setup, Extension
import os
import sys
import glob

# ========================================================================
# Usage:
#   cd $WK; cd ecbuild/release/; sh -x ../../cmake.sh release; make ; make install
#   calling 'sh -x ../../cmake.sh release' will generate steup.py from setup.py.in
# Issues:
#   Can not test cmake install, since it will always install to 
#     /usr/local/apps/python/current/lib/python2.7/site-packages/ecflow
#
# To test install manually, we can:
#   cd $WK/Pyext
#   rm -rf build/   # for a clean build
#   python setup.py build_ext
#   python setup.py install --home=~
#
# See: http://docs.python.org/2/distutils/apiref.html?highlight=extension#distutils.core.Extension
#
# ========================================================================
# AIX: On AIX we need custom compile and link flags:
# The final link line is wrong: xlC_r xlc_r  , 
#                              /usr/local/lpp/vacpp11109/usr/vacpp/bin/.orig/xlC_r: 1501-228 (W) input file xlc_r not found
#           
# Removing the duplicated xlc_r, We then get a little further,
#         ld: 0711-317 ERROR: Undefined symbol: .main
#      Using -bnoquiet
#        .main                     [12]    ER PR crt0_64.s(/lib/crt0_64.o)
#                                          00000070 .text    R_RBR    [34]    .__start
# This is because it is missing -G (generate dynamic library)
# When we manually add -G, we sucessfully link(ecflow.so)
#
#      Both of these can be fixed, by overriding the Link line with:
#      export LDSHARED="xlC_r -G -Wl,-bI:/usr/local/apps/python/2.7.2-01/lib/python2.7/config/python.exp"
#
# To find out what python is going to use:
#         python -c "from distutils import sysconfig; print sysconfig.get_config_vars('LDSHARED')[0]"
# On AIX, it does not use this  value ? 
#
#      See python issue 18235  _sysconfigdata.py wrong on AIX installations
# ==========================================================================

# ==========================================================================
# Permissions: The permission of congfigured file are wrong: 2 choices
# o Make sure origin file, has the right permissions
# o Copy file to different directory, and change the permissions
#   since file(COPY) does rename files
#   configure_file(setup.py.in /tmp/ma0/workspace/ecflow/build/CMakeFiles/setup.py)
#       now copy the temporary into the final destination, setting the permissions
#   file(COPY /tmp/ma0/workspace/ecflow/build/CMakeFiles/setup.py
#     DESTINATION /tmp/ma0/workspace/ecflow/build
#     FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
#     GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
# ============================================================================
        
#=============================================================================
# define the directories to search for include files
# to get this to work, you will need to include the path
# to your boost installation and  ecflow includes
boost_root=os.getenv("BOOST_ROOT") 
include_dirs = [ "/tmp/ma0/workspace/ecflow/Pyext/../ACore/src", 
                 "/tmp/ma0/workspace/ecflow/Pyext/../ANattr/src", 
                 "/tmp/ma0/workspace/ecflow/Pyext/../ANode/src",
                 "/tmp/ma0/workspace/ecflow/Pyext/../ANode/parser/src",
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
library_dirs = ['/tmp/ma0/workspace/ecflow/build/ACore',
                '/tmp/ma0/workspace/ecflow/build/ANattr/',
                '/tmp/ma0/workspace/ecflow/build/ANode/',
                '/tmp/ma0/workspace/ecflow/build/ANode/parser/', 
                '/tmp/ma0/workspace/ecflow/build/Base/', 
                '/tmp/ma0/workspace/ecflow/build/CSim/', 
                '/tmp/ma0/workspace/ecflow/build/Client/', 
                boost_lib_dir  
               ]

# define the libraries to link with this includes the boost lib
libraries = [ 'core' , 'nodeattr', 'node', 'libparser', 'base', 'libsimu', 'libclient',
              'boost_system-mt',
              'boost_serialization-mt',
              'boost_filesystem-mt',
              'boost_program_options-mt',
              'boost_date_time-mt', 
              'boost_python-mt' ]

 
# extra compile flags needed for AIX only
# Note setup.py will add -q64 -qcpluscmt -DNDEBUG  automatically
# Note: two extra_compile_args, debug and release, use the debug for testing and faster compiles
extra_compile_args = ['-ftemplate-depth-512', '-Wno-unused-local-typedefs' ]
extra_link_args = []
if sys.platform.startswith("aix"):
   extra_compile_args =  [ '-qsuppress=1540-0198', '-O3', '-qstrict', '-qfuncsect', '-qeh', '-qrtti'  ]
   #extra_compile_args = [ '-qsuppress=1540-0198',  '-qNOOPTimize', '-qnoinline', '-qfullpath', '-qfuncsect', '-qeh', '-qrtti' ]
   extra_link_args= [ '-lpthread', '-ldl', '-bbigtoc' ]  # '-G', '-noipath', 
 

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
       version='4.0.8', 
       author      = 'ECMWF',
       description = """ecflow Python interface""",
       packages = [ 'ecflow' ],
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
