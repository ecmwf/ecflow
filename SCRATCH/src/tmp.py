import sys
import os

def get_installed_ecflow_version():
    "This will extract ecFlow version from /usr/local/apps/ecflow/current."
    "Will return a list of form `[2,0,24]`"
    ecflow_dir_ext = os.readlink("/usr/local/apps/ecflow/current")
    # print ecflow_dir_ext
    version_list = ecflow_dir_ext.split(".")
    assert len(version_list) == 3 ,"Expected 3 items,release,major,minor but found " + ecflow_dir_ext
    print "Extracted ecflow version from /usr/local/apps/ecflow/current: " + ecflow_dir_ext
    return version_list;
    
def get_ecflow_version( work_space ):
    "This will extract ecFlow version from the source code."
    "If that fails it will look at /usr/local/apps/ecflow/current directory"
    "The version is defined in the file VERSION.cmake"
    "expecting string of form:  'set( ${PROJECT_NAME}_VERSION_STR  "4.0.1" )' "
    "will return a list of form `[4,0,1]`"
    file = work_space + "/VERSION.cmake"
    ecflow_version = []
    if os.path.exists(file):
        version_cpp = open(file,'r')
        try :
           for line in version_cpp :
               first_quote = line.find('"')
               second_quote = line.find('"',first_quote+1)
               if first_quote != -1 and second_quote != -1:
                   part = line[first_quote+1:second_quote]
                   ecflow_version = part.split(".")
                   break;
        finally:
            version_cpp.close();
        
        print "Extracted ecflow version '" + str(ecflow_version) + "' from " + file 
        return ecflow_version
    else:
        return get_installed_ecflow_version()
    

if __name__ == "__main__":    
   print get_ecflow_version(os.getenv("WK"));