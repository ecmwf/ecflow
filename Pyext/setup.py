from distutils.core import setup, Extension
import os

# See: http://docs.python.org/2/distutils/apiref.html?highlight=extension#distutils.core.Extension

def get_ecflow_version():
    "This will extract ecFlow version from the source code."
    "The version is defined in the file VERSION.cmake"
    "expecting string of form:"
    "set( ECFLOW_RELEASE  \"4\" )"
    "set( ECFLOW_MAJOR    \"0\" )"
    "set( ECFLOW_MINOR    \"2\" )"
    "set( ${PROJECT_NAME}_VERSION_STR  \"${ECFLOW_RELEASE}.${ECFLOW_MAJOR}.${ECFLOW_MINOR}\" )"
    "will return a list of form `[4,0,2]`" 
    work_space=os.getenv("WK")
    file = work_space + "/VERSION.cmake"
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
# define the name of the extension to use
extension_name    = 'ecflow'
extension_version = ecflow_version
 
# define the directories to search for include files
# to get this to work, you will need to include the path
# to your boost installation.  
work_space=os.getenv("WK") 
boost_root=os.getenv("BOOST_ROOT") 
boost_python_dir = boost_root + "/stage/lib"

# Now all the ecflow includes
core_include     = work_space + "/ACore/src";
anattr_include   = work_space + "/ANattr/src";
anode_include    = work_space + "/ANode/src";
parser_include   = work_space + "/AParser/src";
base_include     = work_space + "/Base/src";
base_cts_include = work_space + "/Base/src/cts";
base_stc_include = work_space + "/Base/src/stc";
csim_include     = work_space + "/CSim/src";
client_include   = work_space + "/Client/src";
pyext_include    = work_space + "/Pyext/src";
include_dirs = [ core_include, 
                 anattr_include, 
                 anode_include,
                 parser_include,
                 base_include,
                 base_cts_include,
                 base_stc_include,
                 csim_include, 
                 client_include,
                 pyext_include,
                 boost_python_dir ]

# define the library directories to include any extra libraries that may be needed.   
boost_python_dir = boost_root + "/stage/lib/"
libcore_dir = work_space + '/ecbuild/debug/ACore/'
libnodeattr_dir = work_space + '/ecbuild/debug/ANattr/'
libnode_dir = work_space + '/ecbuild/debug/ANode/'
libparser_dir = work_space +  '/ecbuild/debug/AParser/'
libbase_dir = work_space +  '/ecbuild/debug/Base/'
libsim_dir = work_space + '/ecbuild/debug/CSim/'
libclient_dir = work_space + '/ecbuild/debug/Client/'
library_dirs = [  libcore_dir,libnodeattr_dir,libnode_dir,libparser_dir, libbase_dir, libsim_dir, libclient_dir, boost_python_dir  ]

# define any runtime dependencies. This will setup RPATH
runtime_libs = [ boost_python_dir ]

# define the libraries to link with this includes the boost lib
libcore =  'core'
libnodeattr = 'nodeattr'
libnode = 'node'
libparser =  'libparser'
libbase =   'base'
libsim =   'libsimu'
libclient = 'libclient'
libraries = [ libcore , libnodeattr, libnode, libparser, libbase, libsim, libclient,
              'boost_system-mt',
              'boost_serialization-mt',
              'boost_filesystem-mt',
              'boost_program_options-mt',
              'boost_date_time-mt', 
              'boost_python-mt' ]
 
# define the source files for the extension
source_files = [ 'src/BoostPythonUtil.cpp', 
                 'src/ClientDoc.cpp', 
                 'src/DefsDoc.cpp', 
                 'src/EcfExt.cpp', 
                 'src/ExportClient.cpp', 
                 'src/ExportCore.cpp',
                 'src/ExportNode.cpp',
                 'src/ExportNodeAttr.cpp',
                 'src/ExportSuiteAndFamily.cpp',
                 'src/ExportTask.cpp',
                 'src/NodeAttrDoc.cpp' ]
 
# create the extension and add it to the python distribution
setup( name=extension_name, version=extension_version, ext_modules=[Extension( extension_name, source_files, include_dirs=include_dirs, library_dirs=library_dirs, runtime_library_dirs=runtime_libs, libraries=libraries )] )