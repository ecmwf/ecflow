# Used to build ecflow, incrementally and nightly
# In order to run on a new platform, we need to manually install a
# version on that platform first, and update head.h, so that standard
# path will includes path to ecflow_client
import os
import ecflow 

# Notice we need to make a distinction between installed ecflow version and the version currently being built
# The two have to be separate. The installed version is used to test.

def get_ecflow_version( work_space ):
    "This will extract ecFlow version from the source code."
    "The version is defined in the file VERSION.cmake"
    "expecting string of form:"
    "set( ECFLOW_RELEASE  \"4\" )"
    "set( ECFLOW_MAJOR    \"0\" )"
    "set( ECFLOW_MINOR    \"2\" )"
    "set( ${PROJECT_NAME}_VERSION_STR  \"${ECFLOW_RELEASE}.${ECFLOW_MAJOR}.${ECFLOW_MINOR}\" )"
    "will return a list of form `[4,0,2]`"
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
                   print "part = " + part
                   ecflow_version.append(part);
                   if len(ecflow_version) == 3:  
                      break;
        finally:
            version_cpp.close();
        
        print "Extracted ecflow version '" + str(ecflow_version) + "' from " + file 
        return ecflow_version
    assert False, "could not determine version"
    
        
WK=os.getenv("WK") 

ecflow_version_list = get_ecflow_version( WK )
assert len(ecflow_version_list) == 3, "Expected version to have release, major,minor"
ecflow_build_dir_version =  "ecflow_" + ecflow_version_list[0] + "_" + ecflow_version_list[1] + "_" + ecflow_version_list[2]
#ecflow_version = ecflow_version_list[0] + "." + ecflow_version_list[1] + "." + ecflow_version_list[2]
print ecflow_build_dir_version;
#print ecflow_version;
#exit(1)
latest_version = ecflow_version_list[0] + "." + ecflow_version_list[1] + "." + ecflow_version_list[2]

# We must use SCRATCH even though the disk is horrible slow, as it allows the output to be shown for ecgate
# Or we could start a log server on ecgate, (this is what is done for c1a/b)
def add_local_job_variables( node ):
    node.add_variable("ECF_KILL_CMD","kill -15 %ECF_RID%")
    node.add_variable("ECF_INCLUDE",os.getenv("SCRATCH") + "/nightly")   
    node.add_variable("ECF_JOB_CMD","rsh %LOCAL_HOST% -l %USER%  '%ECF_JOB% > %ECF_JOBOUT%  2>&1'")
    node.add_variable("LOCAL_HOST",os.uname()[1])
    node.add_variable("ECF_OUT","") # unset so we use ECF_HOME
 
def add_localhost_variables( localhost ):
    localhost.add_variable("COMPILER_TEST_PATH","gcc-4.5/$mode")
    localhost.add_variable("COMPILER_VERSION","gcc-4.5")
    localhost.add_variable("TOOLSET","gcc")
    localhost.add_variable("BOOTSTRAP_TOOLSET","gcc")
    localhost.add_variable("ARCH","opensuse113")
    localhost.add_variable("SITE_CONFIG","$WK/build/site_config/site-config-Linux64.jam")
    add_local_job_variables( localhost )
    
def add_localhost_clang_variables( localhost_clang ):
    localhost_clang.add_variable("COMPILER_TEST_PATH","clang-linux-3.2/$mode")
    localhost_clang.add_variable("COMPILER_VERSION","clang-linux-3.2")
    localhost_clang.add_variable("TOOLSET","clang")
    localhost_clang.add_variable("BOOTSTRAP_TOOLSET","gcc")  # can't seem to build jam with clang, using gcc instead
    localhost_clang.add_variable("REMOTE_COPY","cp")
    localhost_clang.add_variable("ARCH","opensuse113")
    localhost_clang.add_variable("SITE_CONFIG","$WK/build/site_config/site-config-Linux64-clang.jam")
    localhost_clang.add_variable("BOOST_DIR","/var/tmp/ma0/boost/clang")
    add_local_job_variables( localhost_clang )

def add_remote_linux_64_variables( linux_64 ): 
    linux_64.add_variable("ECF_KILL_CMD","rsh %REMOTE_HOST% \"kill -15 %ECF_RID%\"") 
    linux_64.add_variable("ECF_JOB_CMD","rsh %REMOTE_HOST% -l %USER%  '%ECF_JOB% > %ECF_JOBOUT%  2>&1'")
    linux_64.add_variable("COMPILER_TEST_PATH","gcc-4.3/$mode")
    linux_64.add_variable("COMPILER_VERSION","gcc-4.3")
    linux_64.add_variable("TOOLSET","gcc")
    linux_64.add_variable("BOOTSTRAP_TOOLSET","gcc")
    linux_64.add_variable("NO_OF_CORES","8")
    
def add_remote_linux_64_intel_variables( linux_64 ): 
    linux_64.add_variable("ECF_KILL_CMD","rsh %REMOTE_HOST% \"kill -15 %ECF_RID%\"") 
    linux_64.add_variable("ECF_JOB_CMD","rsh %REMOTE_HOST% -l %USER%  '%ECF_JOB% > %ECF_JOBOUT%  2>&1'")
    linux_64.add_variable("COMPILER_TEST_PATH","intel-linux/$mode")
    linux_64.add_variable("COMPILER_VERSION","intel-linux")
    linux_64.add_variable("TOOLSET","intel")  # intel-linux
    linux_64.add_variable("BOOTSTRAP_TOOLSET","gcc")
    linux_64.add_variable("NO_OF_CORES","8")

def add_linux_64_variables( linux_64 ): 
    linux_64.add_variable("REMOTE_HOST","lxb")
    linux_64.add_variable("ROOT_WK","/vol/ecf/cluster")
    linux_64.add_variable("BOOST_DIR","/vol/ecf/cluster/boost")
    linux_64.add_variable("ARCH","linux64")
    linux_64.add_variable("SITE_CONFIG","$WK/build/site_config/site-config-Linux64.jam")
    
def add_linux_64_intel_variables( linux_64_intel ): 
    linux_64_intel.add_variable("REMOTE_HOST","lxb")
    linux_64_intel.add_variable("ROOT_WK","/vol/ecf/cluster/intel")
    linux_64_intel.add_variable("BOOST_DIR","/vol/ecf/cluster/intel/boost")
    linux_64_intel.add_variable("ARCH","linux64intel")
    linux_64_intel.add_variable("SITE_CONFIG","$WK/build/site_config/site-config-Linux64-intel.jam")

def add_remote_opensuse113_variables( opensuse113 ):
    opensuse113.add_variable("ECF_KILL_CMD","ssh  %USER%@%REMOTE_HOST% \"kill -15 %ECF_RID%\"") 
    opensuse113.add_variable("ECF_JOB_CMD","ssh  %USER%@%REMOTE_HOST% '%ECF_JOB% > %ECF_JOBOUT%  2>&1'")
    opensuse113.add_variable("COMPILER_TEST_PATH","gcc-4.5/$mode")
    opensuse113.add_variable("COMPILER_VERSION","gcc-4.5")
    opensuse113.add_variable("TOOLSET","gcc")
    opensuse113.add_variable("BOOTSTRAP_TOOLSET","gcc")
    opensuse113.add_variable("REMOTE_COPY","scp")

def add_opensuse113_variables( opensuse113 ):
    opensuse113.add_variable("REMOTE_HOST","opensuse113")
    opensuse113.add_variable("ROOT_WK","/vol/ecf/opensuse113")
    opensuse113.add_variable("BOOST_DIR","/vol/ecf/opensuse113/boost")
    opensuse113.add_variable("ARCH","opensuse113")
    opensuse113.add_variable("SITE_CONFIG","$WK/build/site_config/site-config-Linux64.jam")

def add_remote_opensuse131_variables( opensuse131 ):
    opensuse131.add_variable("ECF_KILL_CMD","ssh  %USER%@%REMOTE_HOST% \"kill -15 %ECF_RID%\"") 
    opensuse131.add_variable("ECF_JOB_CMD","ssh  %USER%@%REMOTE_HOST% '%ECF_JOB% > %ECF_JOBOUT%  2>&1'")
    opensuse131.add_variable("COMPILER_TEST_PATH","gcc-4.8/$mode")
    opensuse131.add_variable("COMPILER_VERSION","gcc-4.8")
    opensuse131.add_variable("TOOLSET","gcc")
    opensuse131.add_variable("BOOTSTRAP_TOOLSET","gcc")
    opensuse131.add_variable("REMOTE_COPY","scp")

def add_opensuse131_variables( opensuse131 ):
    opensuse131.add_variable("REMOTE_HOST","opensuse131")
    opensuse131.add_variable("ROOT_WK","/vol/ecf/opensuse131")
    opensuse131.add_variable("BOOST_DIR","/vol/ecf/opensuse131/boost")
    opensuse131.add_variable("ARCH","opensuse131")
    opensuse131.add_variable("SITE_CONFIG","$WK/build/site_config/site-config-Linux64.jam")
    opensuse131.add_variable("CUSTOM_BJAM_ARGS","c++-template-depth=512")   # needed for gcc 4.8.1

def is_cray_cct( node ):
    if (node.name() == "cray_cct"):
        return True
    parent = node.get_parent();
    while parent:
        if (parent.name() == "cray_cct"):
            return True
        parent = parent.get_parent()
    return False
    
def add_cray_gnu_compiler_variables( cray_gnu ):
    if is_cray_cct( cray_gnu ):
        cray_gnu.add_variable("COMPILER_TEST_PATH","gcc-4.6.3/$mode")
        cray_gnu.add_variable("COMPILER_VERSION","gcc-4.6.3")
    else:
        cray_gnu.add_variable("COMPILER_TEST_PATH","gcc-4.8.2/$mode")
        cray_gnu.add_variable("COMPILER_VERSION","gcc-4.8.2")
        
    cray_gnu.add_variable("TOOLSET","gcc")
    cray_gnu.add_variable("BOOTSTRAP_TOOLSET","gcc")
    cray_gnu.add_variable("LAYOUT","versioned")
    cray_gnu.add_variable("PRGENV","PrgEnv-gnu")
    cray_gnu.add_variable("SITE_CONFIG","$WK/build/site_config/site-config-cray.jam")
    cray_gnu.add_variable("ROOT_WK","/perm/ma/ma0/workspace/GNU")
    # cray_gnu.add_variable("CUSTOM_BJAM_ARGS","toolset=gcc cxxflags=-fPIC c++-template-depth=512") # needed for gnu/4.8.1
    cray_gnu.add_variable("CUSTOM_BJAM_ARGS","toolset=gcc cxxflags=-fPIC")  

def add_cray_intel_compiler_variables( cray_intel ):
    cray_intel.add_variable("COMPILER_TEST_PATH","intel-linux/$mode")
    cray_intel.add_variable("COMPILER_VERSION","intel-linux")
    cray_intel.add_variable("TOOLSET","intel")
    cray_intel.add_variable("BOOTSTRAP_TOOLSET","intel")
    cray_intel.add_variable("LAYOUT","versioned")
    cray_intel.add_variable("PRGENV","PrgEnv-intel")
    cray_intel.add_variable("SITE_CONFIG","$WK/build/site_config/site-config-cray.jam")
    cray_intel.add_variable("ROOT_WK","/perm/ma/ma0/workspace/INTEL")
    cray_intel.add_variable("CUSTOM_BJAM_ARGS","toolset=intel cxxflags=-fPIC")  

def add_cray_cray_compiler_variables( cray_cray ):
    cray_cray.add_variable("COMPILER_TEST_PATH","cray/$mode")
    cray_cray.add_variable("COMPILER_VERSION","cray")
    cray_cray.add_variable("TOOLSET","cray")
    cray_cray.add_variable("BOOTSTRAP_TOOLSET","cray")
    cray_cray.add_variable("LAYOUT","versioned")
    cray_cray.add_variable("PRGENV","PrgEnv-cray")
    cray_cray.add_variable("SITE_CONFIG","$WK/build/site_config/site-config-cray.jam")
    cray_cray.add_variable("ROOT_WK","/perm/ma/ma0/workspace/CRAY")
    cray_cray.add_variable("CUSTOM_BJAM_ARGS","toolset=cray cxxflags=-fPIC")  

def add_remote_cray_variables( cray ):
    # re use axel scripts for trap.h. rcp.eh etc,  
    # However this re-use requires definitions for SMSNAME and SMSPASS
    cray.add_variable("SMSNAME", "0")
    cray.add_variable("SMSPASS", "0")
    cray.add_variable("ECF_INCLUDE", "/home/ma/emos/def/cray/include")
    
    cray.add_variable("REMOTE_COPY","scp")

    # for cray we need to use logsrvr in order to see the job output
    if is_cray_cct(cray):
        cray.add_variable("ECF_LOGHOST","cct")  # cctdtn1
    else:
        cray.add_variable("ECF_LOGHOST","cca-il2")  # cctdtn1

    cray.add_variable("ECF_LOGPORT","9316")   
    
    # Set the remote location for output, LOGDIR needed by queing system
    # See function ECF_RCP, where the remote file system, is copied to local file system, at the end of the job
    cray.add_variable("LOGDIR", "/scratch/ma/ma0/nightly")
    cray.add_variable("ECF_OUT","/scratch/ma/ma0/nightly")

    cray.add_variable("ECF_KILL_CMD",   "/home/ma/emos/bin/smssubmit.cray %USER% %SCHOST% %ECF_RID% %ECF_JOB% %ECF_JOBOUT% kill")
    cray.add_variable("ECF_STATUS_CMD", "/home/ma/emos/bin/smssubmit.cray %USER% %SCHOST% %ECF_RID% %ECF_JOB% %ECF_JOBOUT% stat")
    cray.add_variable("ECF_JOB_CMD",    "/home/ma/emos/bin/smssubmit.cray %USER% %SCHOST% %ECF_JOB% %ECF_JOBOUT%")

    cray.add_variable("QUEUE","ns")
    cray.add_variable("ACCOUNT","ecodmdma")
    #cray.add_variable("STHOST","/s2o1")  # Needed by qsub.h
    if is_cray_cct(cray):
        cray.add_variable("SCHOST","cct")    # Super Computer HOST
    else:
        cray.add_variable("SCHOST","cca")    # Super Computer HOST
        
    cray.add_variable("WSHOST",os.uname()[1])  # Work Space HOST

def add_cray_variables( cray ):
    # Look for includes in ECF_INCLUDES, and the ECF_HOME
    cray.add_variable("ECF_HOME", os.getenv("SCRATCH") + "/nightly/suite/cray")
    cray.add_variable("BOOST_DIR","/perm/ma/ma0/boost")
    cray.add_variable("ARCH","cray")
    cray.add_variable("MODULE_LOAD_CRAY_COMPILER","module load cce/8.3.0.186")
    if is_cray_cct( cray ):
        cray.add_variable("REMOTE_HOST","cct")
        cray.add_variable("MODULE_LOAD_GCC","module load gcc/4.6.3")
    else:
        cray.add_variable("REMOTE_HOST","cca")
        cray.add_variable("MODULE_LOAD_GCC","module load gcc/4.8.2")
    
    
def add_remote_redhat_variables( redhat ):
    redhat.add_variable("ECF_KILL_CMD","ssh  %USER%@%REMOTE_HOST% \"kill -15 %ECF_RID%\"") 
    redhat.add_variable("ECF_JOB_CMD","ssh  %USER%@%REMOTE_HOST% '%ECF_JOB% > %ECF_JOBOUT%  2>&1'")
    redhat.add_variable("COMPILER_TEST_PATH","gcc-4.4.7/$mode")
    redhat.add_variable("COMPILER_VERSION","gcc-4.4.7")
    redhat.add_variable("TOOLSET","gcc")
    redhat.add_variable("BOOTSTRAP_TOOLSET","gcc")
    redhat.add_variable("REMOTE_COPY","scp")

def add_redhat_variables( redhat ):
    redhat.add_variable("REMOTE_HOST","ecgb")
    redhat.add_variable("ROOT_WK","/vol/ecf/redhat")
    redhat.add_variable("BOOST_DIR","/vol/ecf/redhat/boost")
    redhat.add_variable("ARCH","redhat")
    redhat.add_variable("SITE_CONFIG","$WK/build/site_config/site-config-Linux64.jam")
    redhat.add_variable("CUSTOM_BJAM_ARGS","c++-template-depth=512")
 
def add_remote_opensuse103_variables( opensuse103 ):
    opensuse103.add_variable("ECF_KILL_CMD","rsh %REMOTE_HOST% \"kill -15 %ECF_RID%\"") 
    opensuse103.add_variable("ECF_JOB_CMD","rsh %REMOTE_HOST% -l %USER%  '%ECF_JOB% > %ECF_JOBOUT%  2>&1'")
    opensuse103.add_variable("COMPILER_TEST_PATH","gcc-4.2.1/$mode")
    opensuse103.add_variable("COMPILER_VERSION","gcc-4.2.1")
    opensuse103.add_variable("TOOLSET","gcc")
    opensuse103.add_variable("BOOTSTRAP_TOOLSET","gcc")

def add_opensuse103_variables( opensuse103 ):
    opensuse103.add_variable("REMOTE_HOST","opensuse103")
    opensuse103.add_variable("ROOT_WK","/vol/ecf/opensuse103")
    opensuse103.add_variable("BOOST_DIR","/vol/ecf/opensuse103/boost")
    opensuse103.add_variable("ARCH","opensuse103")
    opensuse103.add_variable("SITE_CONFIG","$WK/build/site_config/site-config-Linux.jam")
     
def add_remote_aix_power7_variables( aix_power7 ) :
    # for c2a we need to use logsrvr in order to see the job output
    aix_power7.add_variable("ECF_LOGHOST","c2a")
    aix_power7.add_variable("ECF_LOGPORT","9316")
    
    # Set the remote location for output, LOGDIR needed by queing system
    # See function ECF_RCP, where the remote file system, is copied to local file system, at the end of the job
    aix_power7.add_variable("LOGDIR", "/s2o1/emos_data/ecflow/log")
    aix_power7.add_variable("ECF_OUT","/s2o1/emos_data/ecflow/log")

    aix_power7.add_variable("ECF_INCLUDE",  WK + "/build/nightly/suite/aix_power7/includes")
    aix_power7.add_variable("ECF_JOB_CMD",    "/home/ma/emos/bin/smssubmit.c2a %USER% %SCHOST% %ECF_JOB% %ECF_JOBOUT%")
    aix_power7.add_variable("ECF_KILL_CMD",   "/home/ma/emos/bin/smskill   %USER% %SCHOST% %ECF_RID% %ECF_JOB% > %ECF_JOB%.kill 2>&1")
    aix_power7.add_variable("ECF_STATUS_CMD", "/home/ma/emos/bin/smsstatus %USER% %SCHOST% %ECF_RID% %ECF_JOB% > %ECF_JOB%.stat 2>&1")

    aix_power7.add_variable("QUEUE","ns")
    aix_power7.add_variable("ACCOUNT","ecodmdma")
    aix_power7.add_variable("STHOST","/s2o1")  # Needed by qsub.h
    aix_power7.add_variable("SCHOST","c2a")    # Super Computer HOST
    aix_power7.add_variable("WSHOST",os.uname()[1])  # Work Space HOST

    aix_power7.add_variable("COMPILER_VERSION","c++/vacpp/12.1.0.0")  # c++/vacpp/11.1.0.1 c++/vacpp/12.1.0.0
    aix_power7.add_variable("COMPILER_TEST_PATH","vacpp/$mode/threading-multi")
    aix_power7.add_variable("TOOLSET","vacpp")
    aix_power7.add_variable("BOOTSTRAP_TOOLSET","vacpp")

def add_aix_power7_variables( aix_power7 ) :
    aix_power7.add_variable("REMOTE_HOST","c2a")
    aix_power7.add_variable("ROOT_WK","/s2o1/emos_data/ecflow")
    aix_power7.add_variable("BOOST_DIR","/s2o1/emos_data/ecflow/boost")
    aix_power7.add_variable("ARCH","ibm_power7")
    aix_power7.add_variable("SITE_CONFIG","$WK/build/site_config/site-config-AIX.jam")
    aix_power7.add_variable("BUILD_ECFLOWVIEW","false")  # dont build on this platform


def add_build_debug( parent ): 
    f = parent.add_family("build_debug")
    f.add_trigger("cp_site_config == complete and git_clone_ecflow == complete and git_clone_ecbuild == complete")
    f.add_variable("MODE","debug")
    f.add_task("build")
    task_test = f.add_task("test")
    # on IBM we do the debug build first
    if parent.name() == "aix_power7" :
        task_test.add_trigger("build == complete")
    else:
        task_test.add_trigger("build == complete")
        #task_test.add_trigger("build == complete and (../build_release/test == complete or ../build_release/test == aborted)")
    task_test.add_label("progress","")
    task_test.add_meter("progress",0,100,100)
     

def add_build_release( parent ):
    f = parent.add_family("build_release")
    f.add_trigger("cp_site_config == complete and git_clone_ecflow == complete and git_clone_ecbuild == complete")
    f.add_variable("MODE","release")
    task_build = f.add_task("build")
    
    task_test = f.add_task("test")
    # on IBM we do the debug build first, it's a *lot* faster
    if parent.name() == "aix_power7" :
        task_test.add_trigger("build == complete")
        #task_test.add_trigger("build == complete and (../build_debug/test == complete or ../build_debug/test == aborted)")
    elif parent.name() == "linux64intel":
        # Both linux64 and linux64intel are same machine, to avoid starting server on same port add a dependency
        task_test.add_trigger("build == complete")
        #task_test.add_trigger("build == complete and ( /suite/build/linux64/build_debug/test == complete or /suite/build/linux64/build_debug/test == aborted)")
    else :
        task_test.add_trigger("build == complete")
  
    task_test.add_label("progress","")
    task_test.add_meter("progress",0,100,100)
    
def add_git_tasks( parent , set_git_clone_def_status = False ) :
    git_clone_ecflow = parent.add_task("git_clone_ecflow")
    if set_git_clone_def_status :
        git_clone_ecflow.add_defstatus( ecflow.DState.complete )
    git_pull_ecflow = parent.add_task("git_pull_ecflow")
    git_pull_ecflow.add_defstatus( ecflow.DState.complete )

    git_clone_ecbuild = parent.add_task("git_clone_ecbuild")
    if set_git_clone_def_status :
        git_clone_ecbuild.add_defstatus( ecflow.DState.complete )
    git_clone_ecbuild = parent.add_task("git_pull_ecbuild")
    git_clone_ecbuild.add_defstatus( ecflow.DState.complete )

            
def add_build_profile( parent ):
    f = parent.add_family("build_profile")
    f.add_trigger("cp_site_config ==  complete")
    f.add_variable("MODE","profile")
    task_build = f.add_task("build")


def add_build_and_test_tasks( parent ) :  
    clean_task = parent.add_task("clean")
    clean_task.add_defstatus( ecflow.DState.complete )
    
    if parent.name() != "localhost" and parent.name() != "localhost_clang"  and parent.name() != "localhost_cmake":
        cp_install = parent.add_task("cp_install")
        cp_install.add_defstatus( ecflow.DState.complete )
        add_local_job_variables(cp_install) # run locally
        
    cp_site_config = parent.add_task("cp_site_config")
    add_local_job_variables(cp_site_config) # run locally
            
    # On aix do the debug build first, its a lot faster, than build release
    if parent.name() == "aix_power7" :
        add_build_debug( parent )
        add_build_release( parent )
    else:
        add_build_release( parent )
        add_build_debug( parent )

def build_localhost( parent ) :
    localhost = parent.add_family("localhost")
    if (parent.name() == "local") :
        localhost.add_trigger("../tar/create_tar == complete")
    add_localhost_variables(localhost)
    
    add_git_tasks( localhost , True)

    localhost.add_task("cppcheck") 
    add_build_and_test_tasks( localhost )
    add_build_profile( localhost )
    
    localhost.add_task("test_memory_leaks").add_trigger("build_release == complete and build_debug == complete")
    localhost.add_task("test_server_memory").add_trigger("test_memory_leaks == complete or test_memory_leaks == aborted")
    localhost.add_task("test_client_performance").add_trigger("test_server_memory == complete or test_server_memory == aborted")
    localhost.add_task("test_server_performance").add_trigger("test_client_performance == complete or test_client_performance == aborted")
    localhost.add_task("test_performance").add_trigger("test_server_performance == complete or test_server_performance == aborted")
    localhost.add_task("test_migration").add_trigger("test_performance == complete or test_server_performance == aborted")
    task = localhost.add_task("test_new_client_old_server")
    task.add_trigger("test_migration == complete or test_migration == aborted")
    task.add_variable("OLD_VERSION","4.0.0")

def build_localhost_cmake( parent ) :
    # Hence left out test_client_performance and test_server_performance
    localhost_cmake = parent.add_family("localhost_cmake")
    localhost_cmake.add_variable("BUILD_TYPE","cmake")
    
    if (parent.name() == "local") :
        localhost_cmake.add_trigger("localhost == complete || localhost == aborted")
    add_localhost_variables(localhost_cmake)
    
    add_git_tasks( localhost_cmake , True)

    add_build_and_test_tasks( localhost_cmake )

def build_localhost_clang( parent ) :
    # Currently clang based profile builds, have a memory fault. 
    # Hence left out test_client_performance and test_server_performance
    localhost_clang = parent.add_family("localhost_clang")
    add_localhost_clang_variables(localhost_clang)

    if (parent.name() == "local") :
        localhost_clang.add_trigger("localhost_cmake == complete || localhost_cmake == aborted")
    
    add_git_tasks( localhost_clang , True)

    add_build_and_test_tasks( localhost_clang )
    add_build_profile( localhost_clang )
    localhost_clang.add_task("test_memory_leaks").add_trigger("build_release == complete and build_debug == complete")
    localhost_clang.add_task("test_server_memory").add_trigger("test_memory_leaks == complete or test_memory_leaks == aborted")
    localhost_clang.add_task("test_performance").add_trigger("test_server_memory == complete or test_server_memory == aborted")
    localhost_clang.add_task("test_migration").add_trigger("test_performance == complete or test_performance == aborted")


def build_linux_64( parent ) :
    linux_64 = parent.add_family("linux64")
    add_linux_64_variables(linux_64)
    add_remote_linux_64_variables(linux_64)
    add_git_tasks( linux_64 )
    add_build_and_test_tasks( linux_64 )
    
def build_linux_64_intel( parent ) :
    linux_64 = parent.add_family("linux64intel")
    add_linux_64_intel_variables(linux_64)
    add_remote_linux_64_intel_variables(linux_64)
    add_git_tasks( linux_64 )
    add_build_and_test_tasks( linux_64 )
    
def build_opensuse113( parent ) :
    opensuse113 = parent.add_family("opensuse113")
    add_opensuse113_variables(opensuse113)
    add_remote_opensuse113_variables(opensuse113)
    add_git_tasks( opensuse113 )
    add_build_and_test_tasks( opensuse113 )
    
def build_opensuse131( parent ) :
    opensuse131 = parent.add_family("opensuse131")
    opensuse131.add_variable("GIT","git")   # hack 
    add_opensuse131_variables(opensuse131)
    add_remote_opensuse131_variables(opensuse131)
    add_git_tasks( opensuse131 )
    add_build_and_test_tasks( opensuse131 )
    
def build_redhat( parent ) :
    redhat = parent.add_family("redhat")
    add_redhat_variables(redhat)
    add_remote_redhat_variables(redhat)
    add_git_tasks( redhat )
    add_build_and_test_tasks( redhat )

def build_cray_gnu( parent ) :
    cray = parent.add_family("cray_gnu")
    add_cray_variables(cray)
    add_cray_gnu_compiler_variables(cray)
    add_remote_cray_variables(cray)
    add_git_tasks( cray )
    add_build_and_test_tasks( cray )
    
def build_cray_intel( parent ) :
    cray = parent.add_family("cray_intel")
    cray.add_trigger("cray_gnu == complete or cray_gnu == aborted")
    add_cray_variables(cray)
    add_cray_intel_compiler_variables(cray)
    add_remote_cray_variables(cray)
    add_git_tasks( cray )
    add_build_and_test_tasks( cray )
    
def build_cray_cray( parent ) :
    cray = parent.add_family("cray_cray")
    cray.add_defstatus( ecflow.DState.complete ) # complete since does not compile
    cray.add_trigger("cray_intel == complete or cray_intel == aborted")
    add_cray_variables(cray)
    add_cray_cray_compiler_variables(cray)
    add_remote_cray_variables(cray)
    add_git_tasks( cray )
    add_build_and_test_tasks( cray )
    
def build_cray( parent ) :
    cray = parent.add_family("cray_cct")
    cray.add_variable("NO_OF_CORES","2") # temp until things get sorted on cray
    build_cray_gnu( cray)
    build_cray_intel( cray)
    build_cray_cray( cray)
    
    cray = parent.add_family("cray_cca")
    cray.add_variable("NO_OF_CORES","2") # temp until things get sorted on cray
    build_cray_gnu( cray)
    build_cray_intel( cray)
    build_cray_cray( cray)
    
def build_opensuse103( parent ) :
    opensuse103 = parent.add_family("opensuse103")
    add_opensuse103_variables(opensuse103)
    add_remote_opensuse103_variables(opensuse103)
    add_git_tasks( opensuse103 )
    add_build_and_test_tasks( opensuse103 )

def build_aix_power7( parent ) :
    aix_power7 = parent.add_family("aix_power7")
    add_aix_power7_variables( aix_power7 )
    add_remote_aix_power7_variables( aix_power7 )
    add_git_tasks( aix_power7 )
    add_build_and_test_tasks( aix_power7 )
    

def add_boost_tasks( family ):
    boost_remove = family.add_task("boost_remove")  
    boost_copy_gzip = family.add_task("boost_copy_gzip")  
    boost_copy_gzip.add_trigger("boost_remove == complete")
    add_local_job_variables(boost_copy_gzip)  # this is run locally
    boost_untar = family.add_task("boost_untar")
    boost_untar.add_trigger("boost_copy_gzip == complete")
    boost_bjam = family.add_task("boost_bjam")
    boost_bjam.add_trigger("boost_untar == complete")
    boost_fix = family.add_task("boost_fix")
    boost_fix.add_trigger("boost_bjam == complete")
    boost_site_config = family.add_task("boost_site_config")
    boost_site_config.add_trigger("boost_fix == complete")
    boost_build = family.add_task("boost_build")
    boost_build.add_trigger("boost_site_config == complete")
        
def add_cray_boost_tasks( family ):
    boost_remove = family.add_task("boost_remove")  
    boost_copy_gzip = family.add_task("boost_copy_gzip")  
    boost_copy_gzip.add_trigger("boost_remove == complete")
    add_local_job_variables(boost_copy_gzip)  # this is run locally
    boost_untar = family.add_task("boost_untar")
    boost_untar.add_trigger("boost_copy_gzip == complete")
    boost_bjam = family.add_task("boost_bjam")
    boost_bjam.add_trigger("boost_untar == complete")
    boost_fix = family.add_task("boost_fix")
    boost_fix.add_trigger("boost_bjam == complete")
    boost_site_config = family.add_task("boost_site_config")
    boost_site_config.add_trigger("boost_fix == complete")
    
    family_cray_gnu = family.add_family("cray_gnu")
    add_cray_gnu_compiler_variables(family_cray_gnu)
    boost_build = family_cray_gnu.add_task("boost_build")
    boost_build.add_trigger("../boost_site_config == complete")
    
    family_cray_intel = family.add_family("cray_intel")
    add_cray_intel_compiler_variables(family_cray_intel)
    boost_build = family_cray_intel.add_task("boost_build")
    boost_build.add_trigger("../boost_site_config == complete")
    
    family_cray_cray = family.add_family("cray_cray")
    add_cray_cray_compiler_variables(family_cray_cray)
    boost_build = family_cray_cray.add_task("boost_build")
    boost_build.add_trigger("../boost_site_config == complete")


def build_boost( boost ):
    boost.add_defstatus( ecflow.DState.suspended );
    boost.add_variable("LAYOUT","tagged")
    
    family = boost.add_family("localhost")
    add_localhost_variables(family)
    add_boost_tasks( family )

    family = boost.add_family("localhost_clang")
    add_localhost_clang_variables(family)
    add_boost_tasks( family )
     
    family = boost.add_family("linux64")
    add_linux_64_variables(family)
    add_remote_linux_64_variables(family)
    add_boost_tasks( family )

    family = boost.add_family("linux64intel")
    add_linux_64_intel_variables(family)
    add_remote_linux_64_intel_variables(family)
    add_boost_tasks( family )

    family = boost.add_family("opensuse113")
    add_opensuse113_variables(family)
    add_remote_opensuse113_variables(family)
    add_boost_tasks( family )
    
    family = boost.add_family("opensuse131")
    family.add_variable("GIT","git")   # hack 
    add_opensuse131_variables(family)
    add_remote_opensuse131_variables(family)
    add_boost_tasks( family )

    family = boost.add_family("redhat")
    add_redhat_variables(family)
    add_remote_redhat_variables(family)
    add_boost_tasks( family )
    
    family = boost.add_family("cray_cct")
    add_cray_variables(family)
    add_remote_cray_variables(family)
    add_cray_gnu_compiler_variables(family)
    add_cray_boost_tasks(family)

    family = boost.add_family("cray_cca")
    add_cray_variables(family)
    add_remote_cray_variables(family)
    add_cray_gnu_compiler_variables(family)
    add_cray_boost_tasks(family)

 
    family = boost.add_family("opensuse103")
    add_opensuse103_variables(family)
    add_remote_opensuse103_variables(family)
    add_boost_tasks( family )
    
    family = boost.add_family("aix_power7")
    add_aix_power7_variables(family)
    add_remote_aix_power7_variables(family)
    add_boost_tasks( family )
    
    
def add_suite_variables( suite ):
    suite.add_variable("ECFLOW_TAR_DIR","/var/tmp/ma0/clientRoot/workspace")
    suite.add_variable("ECF_HOME", os.getenv("SCRATCH") + "/nightly")
    suite.add_variable("ECF_INCLUDE",os.getenv("SCRATCH") + "/nightly")
    suite.add_variable("USER","ma0")
    suite.add_variable("ECFLOW_LAST_INSTALLED_VERSION","/usr/local/apps/ecflow/current") 
    suite.add_variable("NO_OF_CORES","2")
    suite.add_variable("WK",WK)
    suite.add_variable("BOOST_DIR","/var/tmp/ma0/boost")
    suite.add_variable("PYTHON_VERSION","2.7")
    suite.add_variable("CMAKE","/usr/local/apps/cmake/current/bin/cmake")
    suite.add_variable("GIT","/usr/local/apps/git/current/bin/git")
    suite.add_variable("SET_TO_TEST_SCRIPT","false") 
    suite.add_variable("BUILD_ECFLOWVIEW","true")
    suite.add_variable("ECFLOW_GIT_BRANCH","develop")  # when makeing a relase switch to release/<release version>, otherwise develop
    suite.add_variable("ECBUILD_GIT_BRANCH","develop")   

    # automatically fob all zombies when compiling ecflow 
    child_list = []
    suite.add_zombie(ecflow.ZombieAttr(ecflow.ZombieType.ecf,  child_list, ecflow.ZombieUserActionType.fob, 0))
    suite.add_zombie(ecflow.ZombieAttr(ecflow.ZombieType.user, child_list, ecflow.ZombieUserActionType.fob, 0))
    suite.add_zombie(ecflow.ZombieAttr(ecflow.ZombieType.path, child_list, ecflow.ZombieUserActionType.fob, 0))
    
# ================================================================================
# Defs
# ================================================================================    
defs = ecflow.Defs()
defs.add_variable("ECFLOW_TAR_DIR","/var/tmp/ma0/clientRoot/workspace")

print "build experiment"
with defs.add_suite("experiment") as experiment:
    experiment.add_defstatus( ecflow.DState.suspended )
    experiment.add_variable("ECF_HOME", os.getenv("SCRATCH") + "/nightly")
    experiment.add_variable("ECF_INCLUDE",os.getenv("SCRATCH") + "/nightly")
    experiment.add_variable("ECF_FILES",os.getenv("SCRATCH") + "/nightly/experiment")
    experiment.add_variable("ECF_JOB_CMD","python %ECF_JOB% 1> %ECF_JOBOUT% 2>&1")
    with experiment.add_task("exp") as task:
        task.add_event("event_fred")
        task.add_meter("meter", 0, 100)
        task.add_label("label_name", "value")
    with experiment.add_task("exp2") as task:
        task.add_event("event_fred")
        task.add_meter("meter", 0, 100)
        task.add_label("label_name", "value")

print "build boost"
with defs.add_suite("boost_suite") as boost_suite:
    boost_suite.add_variable("BOOST_VERSION","boost_1_53_0")
    boost_suite.add_variable("REMOTE_COPY","rcp")
    boost_suite.add_variable("ECF_FILES",os.getenv("SCRATCH") + "/nightly/boost_suite")
    add_suite_variables(boost_suite)
    build_boost(boost_suite)

print "incremental and full builds on all platforms"
with defs.add_suite("suite") as suite:
    suite.add_variable("REMOTE_COPY","rcp")
    suite.add_variable("ECF_FILES",os.getenv("SCRATCH") + "/nightly/suite")
    suite.add_variable("BOOST_VERSION","boost_1_53_0")
    add_suite_variables(suite)

    with suite.add_family("build") as build:
        build.add_repeat( ecflow.RepeatDay() )
        build.add_time("18:15")
        build.add_defstatus( ecflow.DState.suspended );
    
        git_pull_ecflow = build.add_task("git_pull_ecflow")
        git_pull_ecflow.add_variable("ARCH","opensuse113")
        git_pull_ecflow.add_variable("LOCAL_HOST",os.uname()[1]) # run this locally
        git_pull_ecbuild = build.add_task("git_pull_ecbuild")
        git_pull_ecbuild.add_variable("ARCH","opensuse113")
        git_pull_ecbuild.add_variable("LOCAL_HOST",os.uname()[1]) # run this locally

        tar_fam = build.add_family("tar")
        tar_fam.add_trigger("git_pull_ecflow == complete and git_pull_ecbuild == complete")
    
        create_tar = tar_fam.add_task("create_tar")
        create_tar.add_variable("ARCH","opensuse113")
    
        with build.add_family("local") as local:
            build_localhost( local )
            build_localhost_cmake( local )
            build_localhost_clang( local )

        with build.add_family("remote") as remote:
            remote.add_variable("BUILD_TYPE","boost")  # choose between [  cmake | boost ]
            build_linux_64( remote )
            build_linux_64_intel( remote )
            build_opensuse113( remote )
            build_opensuse131( remote )
            build_redhat( remote )
            build_cray( remote )
            build_opensuse103( remote )
            build_aix_power7(remote)
        
#ecflow.PrintStyle.set_style(ecflow.Style.STATE)
#print defs

# ====================================================================================
print "Check job generation"
# ====================================================================================
job_ctrl = ecflow.JobCreationCtrl()                   # no set_node_path() hence check job generation for all tasks
defs.check_job_creation(job_ctrl)
assert len(job_ctrl.get_error_msg()) == 0, job_ctrl.get_error_msg()

# ====================================================================================
print "Write defs to disk. This will we loaded onto the server separately"
# ====================================================================================
defs.save_as_defs("build.def")
