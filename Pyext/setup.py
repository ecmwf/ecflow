from distutils.core import setup, Extension
import os
import sys
import glob
from numpy.f2py.crackfortran import endifpattern
# ========================================================================
# Usage:
#   python setup.py build_ext
#   python setup.py install
# Test:
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
# When we manually add -G, sucessfull link(ecflow.so)
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

def get_ecflow_version():
    "This will extract ecFlow version from the source code."
    "The version is defined in the file VERSION.cmake"
    "expecting string of form:"
    "set( ECFLOW_RELEASE  \"4\" )"
    "set( ECFLOW_MAJOR    \"0\" )"
    "set( ECFLOW_MINOR    \"2\" )"
    "set( ${PROJECT_NAME}_VERSION_STR  \"${ECFLOW_RELEASE}.${ECFLOW_MAJOR}.${ECFLOW_MINOR}\" )"
    "will return a list of form `[4,0,2]`" 
    file = "../VERSION.cmake"
    ecflow_version = []
    if os.path.exists(file):
        version_cpp = open(file,'r')
        try :
           for line in version_cpp :
               first_quote = line.find('"')
               second_quote = line.find('"',first_quote+1)
               if first_quote != -1 and second_quote != -1 :
                   part = line[first_quote+1:second_quote]
                   ecflow_version.append(part);
                   if len(ecflow_version) == 3:  
                      break;
        finally:
            version_cpp.close();
        
        print "Extracted ecflow version '" + str(ecflow_version) + "' from " + file 
        return ecflow_version
    assert False, "could not determine version"
    
ecflow_version_list = get_ecflow_version()
assert len(ecflow_version_list) == 3, "Expected version to have release, major,minor"
ecflow_build_dir_version =  "ecflow_" + ecflow_version_list[0] + "_" + ecflow_version_list[1] + "_" + ecflow_version_list[2]
ecflow_version = ecflow_version_list[0] + "." + ecflow_version_list[1] + "." + ecflow_version_list[2]
print ecflow_build_dir_version;
print ecflow_version;

#===============================================================================================
# define the directories to search for include files
# to get this to work, you will need to include the path
# to your boost installation and  ecflow includes
boost_root=os.getenv("BOOST_ROOT") 
include_dirs = [ "../ACore/src", 
                 "../ANattr/src", 
                 "../ANode/src",
                 "../AParser/src",
                 "../Base/src",
                 "../Base/src/cts",
                 "../Base/src/stc",
                 "../CSim/src", 
                 "../Client/src",
                 "../Pyext/src",
                 boost_root,
               ]

# define the library directories to include any extra libraries that may be needed.
# Give preference to release libs   
boost_lib_dir = boost_root + "/stage/lib/"
if os.path.isdir("../ecbuild/release"):
    library_dirs = ['../ecbuild/release/ACore/',
                    '../ecbuild/release/ANattr/',
                    '../ecbuild/release/ANode/',
                    '../ecbuild/release/AParser/', 
                    '../ecbuild/release/Base/', 
                    '../ecbuild/release/CSim/', 
                    '../ecbuild/release/Client/', 
                    boost_lib_dir  
                   ]
else:
    library_dirs = ['../ecbuild/debug/ACore/',
                    '../ecbuild/debug/ANattr/',
                    '../ecbuild/debug/ANode/',
                    '../ecbuild/debug/AParser/', 
                    '../ecbuild/debug/Base/', 
                    '../ecbuild/debug/CSim/', 
                    '../ecbuild/debug/Client/', 
                    boost_lib_dir  
                   ]


# We link to boost python statically, hence no need to setup runtime_libs
# define any runtime dependencies. This will setup RPATH
# runtime_libs = [ boost_python_dir ]

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
extra_compile_args = []
extra_link_args = []
if sys.platform.startswith("aix"):
   #extra_compile_args =  [ '-qsuppress=1540-0198', '-O3', '-qstrict', '-qfuncsect', '-qeh', '-qrtti'  ]
   extra_compile_args = [ '-qsuppress=1540-0198',  '-qNOOPTimize', '-qnoinline', '-qfullpath', '-qfuncsect', '-qeh', '-qrtti' ]
   extra_link_args= [ '-lpthread', '-ldl', '-bbigtoc' ]  # '-G', '-noipath', 
 

# create the extension and add it to the python distribution
# The configuation below creates:
#  lib/python2.7/site-packages/ecflow/ecflow.so
#                                     __init__.py               
setup( name='ecflow', 
       version=ecflow_version, 
       packages = [ 'ecflow' ],
       package_dir={'ecflow': 'ecflow'},
       ext_modules=[ Extension( 
                              'ecflow.ecflow', 
                              glob.glob(os.path.join('src', '*.cpp')) , 
                              include_dirs=include_dirs, 
                              library_dirs=library_dirs, 
                              libraries=libraries,
                              extra_compile_args=extra_compile_args,
                              extra_link_args=extra_link_args,
                              language='c++'
                              )
                    ],
   )
