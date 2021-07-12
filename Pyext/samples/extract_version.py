import sys, os

def get_installed_ecflow_version():
    "This will extract ecFlow version from /usr/local/apps/ecflow/current."
    "Will return a list of form `[2,0,24]`"
    ecflow_dir_ext = os.readlink("/usr/local/apps/ecflow/current")
    # print(ecflow_dir_ext)
    version_list = ecflow_dir_ext.split(".")
    assert len(version_list) == 3 ,"Expected 3 items,release,major,minor but found " + ecflow_dir_ext
    print("Extracted ecflow version from /usr/local/apps/ecflow/current: " + ecflow_dir_ext)
    return version_list;
    
def get_ecflow_version( work_space ):
    "This will extract ecFlow version *list* from the source code."
    "The version is defined in the file CMakeList.txt"
    "expecting string of form:"
    "project( ecflow LANGUAGES CXX VERSION 5.7.1 )"
    file = work_space + "/CMakeLists.txt11"
    ecflow_version = []
    if os.path.exists(file):
        cmake_file = open(file,'r')
        try :
            for line in cmake_file :
                project = line.find('project(')
                ecflow = line.find('ecflow')
                cxx = line.find('CXX')
                version_t = line.find('VERSION')
                if project != -1 and ecflow != -1 and cxx != -1 and version_t != -1 :
                    tokens = line.split()
                    version_index = tokens.index("VERSION")
                    version = tokens[version_index+1]
                    if version[-1] == ')':
                        version = version[:-1]
                    ecflow_version = version.split(".")
        finally:
            cmake_file.close();
        
        print("Extracted ecflow version '" + str(ecflow_version) + "' from " + file)
        return ecflow_version
    else:
        return get_installed_ecflow_version()
    
    
print(get_ecflow_version("/Users/avibahra/git/ecflow"))
    
