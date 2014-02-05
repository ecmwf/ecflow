from distutils.core import setup, Extension
import os

# See: http://docs.python.org/2/distutils/apiref.html?highlight=extension#distutils.core.Extension

def get_ecflow_version():
    "This will extract version from the source code"
    "expecting string of form: const int Version::release_ = 1;"
    "expecting string of form: const int Version::major_ = 9;"
    "expecting string of form: const int Version::minor_ = 0;"
    "will return string of form `ecflow_1_9_15`"
    work_space=os.getenv("WK") 
    file = work_space + "/ACore/src/Version.cpp"
    version_cpp = open(file,'r')
    try :
        release = "";  major = "";  minor = ""
        # search for release, major, minor
        for line in version_cpp :
            equal_pos = line.find("=")
            semi_colon_pos = line.find(";")
            if equal_pos != -1 and semi_colon_pos != -1:
                part = line[equal_pos+1:semi_colon_pos]
                part = part.strip()
                if line.find("Version::release_") != -1: release = part; continue
                if line.find("Version::major_")   != -1: major = part;   continue
                if line.find("Version::minor_")   != -1: minor = part.replace('"',''); break; # minor is a string, remove quotes
    finally:
        version_cpp.close();
        
    ecflow_version = []
    ecflow_version.append(release)
    ecflow_version.append(major)
    ecflow_version.append(minor)
    return ecflow_version
        
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