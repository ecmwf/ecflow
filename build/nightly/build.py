# Used to build ecflow, incrementally and nightly
# In order to run on a new platform, we need to manually install a
# version on that platform first, and update head.h, so that standard
# path will includes path to ecflow_client
import os
import ecflow 

# Notice we need to make a distinction between installed ecflow version and the version currently being built
# The two have to be separate. The installed version is used to test.

def get_ecflow_version( work_space ):
    "This will extract version from the source code"
    "expecting string of form: const int Version::release_ = 1;"
    "expecting string of form: const int Version::major_ = 9;"
    "expecting string of form: const int Version::minor_ = 0;"
    "will return string of form `ecflow_1_9_15`"
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
        
WK=os.getenv("WK") 
BOOST_VERSION="boost_1_47_0"

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
    localhost.add_variable("ARCH","opensuse113")
    localhost.add_variable("SITE_CONFIG","$WK/build/site_config/site-config-Linux64.jam")
    add_local_job_variables( localhost )
    
def add_localhost_clang_variables( localhost_clang ):
    localhost_clang.add_variable("COMPILER_TEST_PATH","clang-linux-3.2/$mode")
    localhost_clang.add_variable("COMPILER_VERSION","clang-linux-3.2")
    localhost_clang.add_variable("TOOLSET","clang")
    localhost_clang.add_variable("ARCH","opensuse113")
    localhost_clang.add_variable("SITE_CONFIG","$WK/build/site_config/site-config-Linux64-clang.jam")
    add_local_job_variables( localhost_clang )

def add_remote_linux_64_variables( linux_64 ): 
    linux_64.add_variable("ECF_KILL_CMD","rsh %REMOTE_HOST% \"kill -15 %ECF_RID%\"") 
    linux_64.add_variable("ECF_JOB_CMD","rsh %REMOTE_HOST% -l %USER%  '%ECF_JOB% > %ECF_JOBOUT%  2>&1'")
    linux_64.add_variable("COMPILER_TEST_PATH","gcc-4.3/$mode")
    linux_64.add_variable("COMPILER_VERSION","gcc-4.3")
    linux_64.add_variable("TOOLSET","gcc")
    linux_64.add_variable("NO_OF_CORES","8")
    
def add_remote_linux_64_intel_variables( linux_64 ): 
    linux_64.add_variable("ECF_KILL_CMD","rsh %REMOTE_HOST% \"kill -15 %ECF_RID%\"") 
    linux_64.add_variable("ECF_JOB_CMD","rsh %REMOTE_HOST% -l %USER%  '%ECF_JOB% > %ECF_JOBOUT%  2>&1'")
    linux_64.add_variable("COMPILER_TEST_PATH","intel-linux/$mode")
    linux_64.add_variable("COMPILER_VERSION","intel-linux")
    linux_64.add_variable("TOOLSET","intel")  # intel-linux
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

def add_opensuse113_variables( opensuse113 ):
    opensuse113.add_variable("REMOTE_HOST","opensuse113")
    opensuse113.add_variable("ROOT_WK","/vol/ecf/opensuse113")
    opensuse113.add_variable("BOOST_DIR","/vol/ecf/opensuse113/boost")
    opensuse113.add_variable("ARCH","opensuse113")
    opensuse113.add_variable("SITE_CONFIG","$WK/build/site_config/site-config-Linux64.jam")

def add_cray_gnu_compiler_variables( cray ):
    cray.add_variable("COMPILER_TEST_PATH","gcc-4.6.3/$mode")
    cray.add_variable("COMPILER_VERSION","gcc-4.6.3")
    cray.add_variable("TOOLSET","gcc")
    cray.add_variable("CRAY_COMPILER_TOOLSET","gnu")
    cray.add_variable("CRAY_COMPILER_TOOLSET_VERSION","46")  # for install
    cray.add_variable("SITE_CONFIG","$WK/build/site_config/site-config-cray-gcc.jam")
    # cray.add_variable("TEMPLATE_DEPTH","c++-template-depth=512") # needed for gnu/4.8.1

def add_cray_intel_compiler_variables( cray ):
    cray.add_variable("COMPILER_TEST_PATH","intel-linux/$mode")
    cray.add_variable("COMPILER_VERSION","intel-linux")
    cray.add_variable("TOOLSET","intel")
    cray.add_variable("CRAY_COMPILER_TOOLSET","intel")
    cray.add_variable("CRAY_COMPILER_TOOLSET_VERSION","14")  # for install
    cray.add_variable("SITE_CONFIG","$WK/build/site_config/site-config-cray-intel.jam")

def add_cray_cray_compiler_variables( cray ):
    cray.add_variable("COMPILER_TEST_PATH","cray/$mode")
    cray.add_variable("COMPILER_VERSION","cray")
    cray.add_variable("TOOLSET","cray")
    cray.add_variable("CRAY_COMPILER_TOOLSET","cray")
    cray.add_variable("CRAY_COMPILER_TOOLSET_VERSION","14")  # for install
    cray.add_variable("SITE_CONFIG","$WK/build/site_config/site-config-cray-cray.jam")
    
def add_remote_cray_variables( cray ):
    # re use axel scripts for trap.h. rcp.eh etc,  
    # However this re-use requires definitions for SMSNAME and SMSPASS
    cray.add_variable("SMSNAME", "0")
    cray.add_variable("SMSPASS", "0")
    cray.add_variable("ECF_INCLUDE", "/home/ma/emos/def/cray/include")
    
    # for cray we need to use logsrvr in order to see the job output
    cray.add_variable("ECF_LOGHOST","cctdtn1")
    cray.add_variable("ECF_LOGPORT","10016")  #  revert to 9316 when log output umask setting fixed
    
    # Set the remote location for output, LOGDIR needed by queing system
    # See function ECF_RCP, where the remote file system, is copied to local file system, at the end of the job
    cray.add_variable("LOGDIR", "/scratch/ma/ma0/nightly")
    cray.add_variable("ECF_OUT","/scratch/ma/ma0/nightly")

    cray.add_variable("ECF_KILL_CMD",   "/home/ma/emos/bin/smssubmit.cray %USER% %SCHOST% %ECF_RID% %ECF_JOB% %ECF_JOBOUT% kill")
    cray.add_variable("ECF_STATUS_CMD", "/home/ma/emos/bin/smssubmit.cray %USER% %SCHOST% %ECF_RID% %ECF_JOB% %ECF_JOBOUT% stat")
    cray.add_variable("ECF_JOB_CMD",   "/home/ma/emos/bin/smssubmit.cray  %USER% %SCHOST% %ECF_JOB% %ECF_JOBOUT%")

    cray.add_variable("QUEUE","ns")
    cray.add_variable("ACCOUNT","ecodmdma")
    #cray.add_variable("STHOST","/s2o1")  # Needed by qsub.h
    cray.add_variable("SCHOST","cctdtn1")    # Super Computer HOST
    cray.add_variable("WSHOST",os.uname()[1])  # Work Space HOST

def add_cray_variables( cray ):
    # Look for includes in ECF_INCLUDES, and the ECF_HOME
    cray.add_variable("ECF_HOME", os.getenv("SCRATCH") + "/nightly/suite/cray")
    cray.add_variable("REMOTE_HOST","cctdtn1")
    cray.add_variable("ROOT_WK","/home/ma/ma0")
    cray.add_variable("BOOST_DIR","/home/ma/ma0/boost")
    cray.add_variable("ARCH","cray")
    
    
def add_remote_redhat_variables( redhat ):
    redhat.add_variable("ECF_KILL_CMD","ssh  %USER%@%REMOTE_HOST% \"kill -15 %ECF_RID%\"") 
    redhat.add_variable("ECF_JOB_CMD","ssh  %USER%@%REMOTE_HOST% '%ECF_JOB% > %ECF_JOBOUT%  2>&1'")
    redhat.add_variable("COMPILER_TEST_PATH","gcc-4.4.7/$mode")
    redhat.add_variable("COMPILER_VERSION","gcc-4.4.7")
    redhat.add_variable("TOOLSET","gcc")

def add_redhat_variables( redhat ):
    redhat.add_variable("REMOTE_HOST","ecgb")
    redhat.add_variable("ROOT_WK","/vol/ecf/redhat")
    redhat.add_variable("BOOST_DIR","/vol/ecf/redhat/boost")
    redhat.add_variable("ARCH","redhat")
    redhat.add_variable("SITE_CONFIG","$WK/build/site_config/site-config-Linux64.jam")
    redhat.add_variable("TEMPLATE_DEPTH","c++-template-depth=512")
 
def add_remote_opensuse103_variables( opensuse103 ):
    opensuse103.add_variable("ECF_KILL_CMD","rsh %REMOTE_HOST% \"kill -15 %ECF_RID%\"") 
    opensuse103.add_variable("ECF_JOB_CMD","rsh %REMOTE_HOST% -l %USER%  '%ECF_JOB% > %ECF_JOBOUT%  2>&1'")
    opensuse103.add_variable("COMPILER_TEST_PATH","gcc-4.2.1/$mode")
    opensuse103.add_variable("COMPILER_VERSION","gcc-4.2.1")
    opensuse103.add_variable("TOOLSET","gcc")

def add_opensuse103_variables( opensuse103 ):
    opensuse103.add_variable("REMOTE_HOST","opensuse103")
    opensuse103.add_variable("ROOT_WK","/vol/ecf/opensuse103")
    opensuse103.add_variable("BOOST_DIR","/vol/ecf/opensuse103/boost")
    opensuse103.add_variable("ARCH","opensuse103")
    opensuse103.add_variable("SITE_CONFIG","$WK/build/site_config/site-config-Linux.jam")
     
def add_remote_hpux_variables( hpux ):
    hpux.add_variable("ECF_KILL_CMD","rsh %REMOTE_HOST% \"kill -s 15 %ECF_RID%\"") 
    hpux.add_variable("ECF_JOB_CMD","rsh %REMOTE_HOST% -l %USER%  '%ECF_JOB% > %ECF_JOBOUT%  2>&1'")
    hpux.add_variable("COMPILER_TEST_PATH","acc/$mode/threading-multi")
    hpux.add_variable("TOOLSET","acc")
    
def add_hpux_variables( hpux ):
    hpux.add_variable("REMOTE_HOST","itanium")
    hpux.add_variable("ROOT_WK","/scratch/ma/emos/ma0/hpia64")
    hpux.add_variable("BOOST_DIR","/scratch/ma/emos/ma0/hpia64/boost")
    hpux.add_variable("ARCH","hpux")
    hpux.add_variable("SITE_CONFIG","$WK/build/site_config/site-config-HPUX.jam")
    hpux.add_variable("BUILD_ECFLOWVIEW","false")  # dont build on this platform

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
    
def add_aix_power7_variables( aix_power7 ) :
    aix_power7.add_variable("REMOTE_HOST","c2a")
    aix_power7.add_variable("ROOT_WK","/s2o1/emos_data/ecflow")
    aix_power7.add_variable("BOOST_DIR","/s2o1/emos_data/ecflow/boost")
    aix_power7.add_variable("ARCH","ibm_power7")
    aix_power7.add_variable("SITE_CONFIG","$WK/build/site_config/site-config-AIX.jam")
    aix_power7.add_variable("BUILD_ECFLOWVIEW","false")  # dont build on this platform


def add_remote_aix_rs6000_variables( aix_rs6000 ) :
    aix_rs6000.add_variable("ECF_JOB_CMD",    "/home/ma/emos/bin/smssubmit %USER% %SCHOST% %ECF_JOB% %ECF_JOBOUT%")
    aix_rs6000.add_variable("ECF_KILL_CMD",   "/home/ma/emos/bin/smskill   %USER% %SCHOST% %ECF_RID% %ECF_JOB% > %ECF_JOB%.kill 2>&1")
    aix_rs6000.add_variable("ECF_STATUS_CMD", "/home/ma/emos/bin/smsstatus %USER% %SCHOST% %ECF_RID% %ECF_JOB% > %ECF_JOB%.stat 2>&1")
    aix_rs6000.add_variable("QUEUE","ns")
    aix_rs6000.add_variable("ACCOUNT","ecodmdma")
    aix_rs6000.add_variable("SCHOST","ecgate")    # Super Computer HOST
    aix_rs6000.add_variable("WSHOST",os.uname()[1])     # Work Space HOST
    aix_rs6000.add_variable("ECF_TRIES","7")      # Since ecgate times out with CPU limit exceeded
    aix_rs6000.add_variable("ECF_INCLUDE",  WK + "/build/nightly/suite/aix_rs6000_xlc/includes")
    aix_rs6000.add_variable("COMPILER_VERSION","vacpp12100")
    aix_rs6000.add_variable("COMPILER_TEST_PATH","vacpp/$mode/threading-multi")
    aix_rs6000.add_variable("TOOLSET","vacpp")
    
def add_aix_rs6000_variables( aix_rs6000 ) :
    aix_rs6000.add_variable("REMOTE_HOST","ecgate")
    aix_rs6000.add_variable("ROOT_WK","/emos_data/ecflow/rs6000/xlc")
    aix_rs6000.add_variable("BOOST_DIR","/emos_data/ecflow/rs6000/xlc/boost")
    aix_rs6000.add_variable("ARCH","rs6000")
    aix_rs6000.add_variable("SITE_CONFIG","$WK/build/site_config/site-config-AIX-rs6000.jam")


def add_remote_aix_gcc_variables( aix_gcc ) :
    aix_gcc.add_variable("ECF_JOB_CMD",    "/home/ma/emos/bin/smssubmit %USER% %SCHOST% %ECF_JOB% %ECF_JOBOUT%")
    aix_gcc.add_variable("ECF_KILL_CMD",   "/home/ma/emos/bin/smskill   %USER% %SCHOST% %ECF_RID% %ECF_JOB% > %ECF_JOB%.kill 2>&1")
    aix_gcc.add_variable("ECF_STATUS_CMD", "/home/ma/emos/bin/smsstatus %USER% %SCHOST% %ECF_RID% %ECF_JOB% > %ECF_JOB%.stat 2>&1")
    aix_gcc.add_variable("QUEUE","ns")
    aix_gcc.add_variable("ACCOUNT","ecodmdma")
    aix_gcc.add_variable("SCHOST","ecgate")    # Super Computer HOST
    aix_gcc.add_variable("WSHOST",os.uname()[1])     # Work Space HOST
    aix_gcc.add_variable("ECF_TRIES","7")      # Since ecgate times out with CPU limit exceeded
    aix_gcc.add_variable("COMPILER_TEST_PATH","gcc-4.5.0/$mode")
    aix_gcc.add_variable("TOOLSET","gcc")
     
def add_aix_gcc_variables( aix_gcc ) :
    aix_gcc.add_variable("REMOTE_HOST","ecgate")
    aix_gcc.add_variable("ROOT_WK","/emos_data/ecflow/rs6000/gcc")
    aix_gcc.add_variable("BOOST_DIR","/emos_data/ecflow/rs6000/gcc/boost")
    aix_gcc.add_variable("ARCH","rs6000")
    aix_gcc.add_variable("SITE_CONFIG","$WK/build/site_config/site-config-AIX-gcc.jam")

def add_build_debug( parent ): 
    f = parent.add_family("build_debug")
    git_clone = parent.find_node_up_the_tree("git_clone")
    assert git_clone != None
    f.add_trigger("cp_site_config == complete and " + git_clone.get_abs_node_path() + " == complete")
    f.add_variable("MODE","debug")
    f.add_task("build")
    task_test = f.add_task("test")
    # on IBM we do the debug build first
    if parent.name() == "aix_power7" or parent.name() == "aix_rs6000_xlc" or parent.name() == "hpux" :
        task_test.add_trigger("build == complete")
    else:
        task_test.add_trigger("build == complete and (../build_release/test == complete or ../build_release/test == aborted)")
    task_test.add_label("progress","")
    task_test.add_meter("progress",0,100,100)
     

def add_build_release( parent ):
    f = parent.add_family("build_release")
    git_clone = parent.find_node_up_the_tree("git_clone")
    assert git_clone != None
    f.add_trigger("cp_site_config == complete and " + git_clone.get_abs_node_path() + " == complete")
    f.add_variable("MODE","release")
    task_build = f.add_task("build")
    
    task_test = f.add_task("test")
    # on IBM/HPUX we do the debug build first, it's a *lot* faster
    if parent.name() == "aix_power7" or parent.name() == "aix_rs6000_xlc" or parent.name() == "hpux" :
        task_test.add_trigger("build == complete and (../build_debug/test == complete or ../build_debug/test == aborted)")
    elif parent.name() == "linux64intel":
        # Both linux64 and linux64intel are same machine, to avoid starting server on same port add a dependency
        task_test.add_trigger("build == complete and ( /suite/build/linux64/build_debug/test == complete or /suite/build/linux64/build_debug/test == aborted)")
    else :
        task_test.add_trigger("build == complete")
  

    task_test.add_label("progress","")
    task_test.add_meter("progress",0,100,100)
    
def add_git_tasks( parent , set_git_clone_def_status = False ) :
    git_clone = parent.add_task("git_clone")
    if set_git_clone_def_status :
        git_clone.add_defstatus( ecflow.DState.complete )
    git_pull = parent.add_task("git_pull")
    git_pull.add_defstatus( ecflow.DState.complete )

            
def add_build_profile( parent ):
    f = parent.add_family("build_profile")
    f.add_trigger("cp_site_config ==  complete")
    f.add_variable("MODE","profile")
    task_build = f.add_task("build")


def add_build_and_test_tasks( parent ) :  
    clean_task = parent.add_task("clean")
    clean_task.add_defstatus( ecflow.DState.complete )
    
    if parent.name() != "localhost" and  parent.name() != "localhost_clang" :
        cp_install = parent.add_task("cp_install")
        cp_install.add_defstatus( ecflow.DState.complete )
        add_local_job_variables(cp_install) # run locally
        
    cp_site_config = parent.add_task("cp_site_config")
    add_local_job_variables(cp_site_config) # run locally
            
    # On aix/xlc do the debug build first, its a lot faster, than build release
    if parent.name() == "aix_power7" or parent.name() == "aix_rs6000_xlc" or parent.name() == "hpux" :
        add_build_debug( parent )
        add_build_release( parent )
    else:
        add_build_release( parent )
        add_build_debug( parent )

def build_localhost( parent ) :
    localhost = parent.add_family("localhost")
    if (parent.name() == "build") :
        localhost.add_trigger("tar/create_tar == active or tar/create_tar == complete")
    add_localhost_variables(localhost)
    
    add_git_tasks( localhost , True)

    add_build_and_test_tasks( localhost )
    add_build_profile( localhost )
    
    localhost.add_task("test_memory_leaks").add_trigger("build_release == complete and build_debug == complete")
    localhost.add_task("test_server_memory").add_trigger("test_memory_leaks == complete or test_memory_leaks == aborted")
    localhost.add_task("test_client_performance").add_trigger("test_server_memory == complete or test_server_memory == aborted")
    localhost.add_task("test_server_performance").add_trigger("test_client_performance == complete or test_client_performance == aborted")
    localhost.add_task("test_performance").add_trigger("test_server_performance == complete or test_server_performance == aborted")
    localhost.add_task("test_migration").add_trigger("test_performance == complete or test_server_performance == aborted")

def build_localhost_clang( parent ) :
    # Currently clang based profile builds, have a memory fault. 
    # Hence left out test_client_performance and test_server_performance
    localhost_clang = parent.add_family("localhost_clang")
    localhost_clang.add_variable("BOOST_DIR","/var/tmp/ma0/boost/clang")
    localhost_clang.add_variable("BOOST_VERSION","boost_1_53_0")

    if (parent.name() == "build") :
        localhost_clang.add_trigger("localhost == complete || localhost == aborted")
    add_localhost_clang_variables(localhost_clang)
    
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
    
def build_redhat( parent ) :
    redhat = parent.add_family("redhat")
    add_redhat_variables(redhat)
    add_remote_redhat_variables(redhat)
    add_git_tasks( redhat )
    add_build_and_test_tasks( redhat )

def build_cray_gnu( parent ) :
    cray = parent.add_family("cray_gnu")
    cray.add_trigger("git_clone == complete")
    add_cray_variables(cray)
    add_cray_gnu_compiler_variables(cray)
    add_remote_cray_variables(cray)
    add_build_and_test_tasks( cray )
    
def build_cray_intel( parent ) :
    cray = parent.add_family("cray_intel")
    cray.add_variable("BOOST_VERSION","boost_1_53_0")
    cray.add_trigger("git_clone == complete and cray_gnu == complete ")
    add_cray_variables(cray)
    add_cray_intel_compiler_variables(cray)
    add_remote_cray_variables(cray)
    add_build_and_test_tasks( cray )
    
def build_cray_cray( parent ) :
    cray = parent.add_family("cray_cray")
    cray.add_variable("BOOST_VERSION","boost_1_53_0")
    cray.add_trigger("git_clone == complete and cray_intel == complete ")
    add_cray_variables(cray)
    add_cray_cray_compiler_variables(cray)
    add_remote_cray_variables(cray)
    add_build_and_test_tasks( cray )
    
def build_cray( parent ) :
    cray = parent.add_family("cray")
    cray.add_variable("NO_OF_CORES","8")
    add_cray_variables(cray)
    add_remote_cray_variables(cray)
    add_git_tasks( cray )
    build_cray_gnu( cray)
    build_cray_intel( cray)
    build_cray_cray( cray)
    
def build_opensuse103( parent ) :
    opensuse103 = parent.add_family("opensuse103")
    add_opensuse103_variables(opensuse103)
    add_remote_opensuse103_variables(opensuse103)
    add_git_tasks( opensuse103 )
    add_build_and_test_tasks( opensuse103 )

def build_hpux( parent ):
    hpux = parent.add_family("hpux")
    if (parent.name() == "build") :
        hpux.add_trigger("tar/cp_tar_to_hpux == complete")
    else :
        hpux.add_trigger("/suite/build_incr/incr_tar_and_cp  == complete")
    add_hpux_variables( hpux )
    add_remote_hpux_variables( hpux )
    add_git_tasks( hpux , True) #  git not available on HP-UX, hence set git_clone to def-status complete
    add_build_and_test_tasks( hpux )

def build_aix_power7( parent ) :
    aix_power7 = parent.add_family("aix_power7")
    add_aix_power7_variables( aix_power7 )
    add_remote_aix_power7_variables( aix_power7 )
    add_git_tasks( aix_power7 )
    add_build_and_test_tasks( aix_power7 )
    
def build_aix_rs6000_xlc( parent ) :
    aix_rs6000 = parent.add_family("aix_rs6000_xlc")
    add_aix_rs6000_variables(aix_rs6000)
    add_remote_aix_rs6000_variables(aix_rs6000)
    add_git_tasks( aix_rs6000 )  
    add_build_and_test_tasks( aix_rs6000 )

def build_aix_gcc(parent) :
    aix_gcc = parent.add_family("aix_gcc")
    add_aix_gcc_variables( aix_gcc )
    add_remote_aix_gcc_variables( aix_gcc )
    add_git_tasks( aix_gcc )
    add_build_and_test_tasks( aix_gcc )
  
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
        
def build_boost( boost ):
    boost.add_defstatus( ecflow.DState.suspended );
    boost.add_variable("BOOST_VERSION",BOOST_VERSION)
    
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
    family.add_variable("BOOST_VERSION","boost_1_53_0") # intel compiler
    add_linux_64_intel_variables(family)
    add_remote_linux_64_intel_variables(family)
    add_boost_tasks( family )

    family = boost.add_family("opensuse113")
    add_opensuse113_variables(family)
    add_remote_opensuse113_variables(family)
    add_boost_tasks( family )

    family = boost.add_family("redhat")
    add_redhat_variables(family)
    add_remote_redhat_variables(family)
    add_boost_tasks( family )

    family = boost.add_family("opensuse103")
    add_opensuse103_variables(family)
    add_remote_opensuse103_variables(family)
    add_boost_tasks( family )
    
    family = boost.add_family("hpux");
    add_hpux_variables(family)
    add_remote_hpux_variables(family)
    add_boost_tasks( family )
 
    family = boost.add_family("aix_power7")
    add_aix_power7_variables(family)
    add_remote_aix_power7_variables(family)
    add_boost_tasks( family )
    
    family = boost.add_family("aix_rs6000")
    add_aix_rs6000_variables(family)
    add_remote_aix_rs6000_variables(family)
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
    suite.add_variable("SET_TO_TEST_SCRIPT","false")
    suite.add_variable("BUILD_ECFLOWVIEW","true")
    suite.add_variable("GIT_BRANCH","develop")  # when makeing a relase switch to master
    suite.add_variable("CRAY_COMPILER_TOOLSET","gnu")
    suite.add_variable("CRAY_COMPILER_TOOLSET_VERSION","43")

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
 
print "build boost"
with defs.add_suite("boost_suite") as boost_suite:
    boost_suite.add_variable("ECF_FILES",os.getenv("SCRATCH") + "/nightly/boost_suite")
    add_suite_variables(boost_suite)
    build_boost(boost_suite)

print "incremental and full builds on all platforms"
with defs.add_suite("suite") as suite:
    suite.add_variable("ECF_FILES",os.getenv("SCRATCH") + "/nightly/suite")
    suite.add_variable("BOOST_VERSION",BOOST_VERSION)
    add_suite_variables(suite)

    with suite.add_family("build") as build:
        build.add_repeat( ecflow.RepeatDay() )
        build.add_time("18:15")
        build.add_defstatus( ecflow.DState.suspended );
    
        git_pull = build.add_task("git_pull")
        git_pull.add_variable("ARCH","opensuse113")
        git_pull.add_variable("LOCAL_HOST",os.uname()[1]) # run this locally

    
        tar_fam = build.add_family("tar")
        tar_fam.add_trigger("git_pull == complete")
    
        create_tar = tar_fam.add_task("create_tar")
        create_tar.add_variable("ARCH","opensuse113")
    
        cp_tar_to_hpux = tar_fam.add_task("cp_tar_to_hpux")
        cp_tar_to_hpux.add_trigger("create_tar == complete")
        add_hpux_variables( cp_tar_to_hpux )
    
        build_localhost( build )
        build_localhost_clang( build )
        build_linux_64( build )
        build_linux_64_intel( build )
        build_opensuse113( build )
        build_redhat( build )
        build_cray( build )
        build_opensuse103( build )
        build_hpux(build)
        build_aix_power7(build)
        build_aix_rs6000_xlc(build)
        #build_aix_gcc(build)

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
