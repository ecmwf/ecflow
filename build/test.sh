#!/bin/sh

## Copyright 2009-2012 ECMWF. 
## This software is licensed under the terms of the Apache Licence version 2.0 
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
## In applying this licence, ECMWF does not waive the privileges and immunities 
## granted to it by virtue of its status as an intergovernmental organisation 
## nor does it submit to any jurisdiction. 

#===================================================================
# Run all the ecflow tests, note use !/bin/bash to keep rpm happy, it uses bash
#===================================================================

if [ "$#" -gt 3 ] ; then
   echo "Maximum of 3 arguments expected"
   echo " arg1-mode    (optional) default = debug,    valid values = [ default | release ]"
   echo " arg2-compiler(optional) default = linux/gcc-4.2.1"
   echo " arg3-safe    (optional) default = no, valid values = [ no | safe ],"
   echo "                                   safe means only run deterministic tests"
   exit 1
fi

mode=debug
compiler_arg=
safe=no
while [ "$#" != 0 ] ; do   
   if [ "$1" = debug -o "$1" = release -o "$1" = profile ] ; then
      mode=$1
   elif [ "$1" = safe ] ; then  
      safe=yes
   else
      compiler_arg=$1
   fi
   # shift remove last argument
   shift
done

if [ ${#mode} -eq 0 ] ; then
   echo "test expects mode i.e. debug or release"
   exit 1
fi

echo "mode=$mode compiler=$compiler_arg safe=$safe"
#exit 1;

#======================================================================
# remove python test, so that they are rerun
cd $WK
rm -rf Pyext/bin/*.test   

# Remove any lock file create by tests which used EcfPortLock.hpp
# ** However DO *NOT* remove this locks for in ecflow test.ecf
# ** as they allow the debug and release test to run at the same time.
rm -rf *.lock
 
# Check that a command is in the PATH.
test_path ()
{
    if `command -v command 1>/dev/null 2>/dev/null`; then
        command -v $1 1>/dev/null 2>/dev/null
    else
        hash $1 1>/dev/null 2>/dev/null
    fi
}

# Check that the OS name, as returned by "uname", is as given.
test_uname ()
{
    if test_path uname; then
        test `uname` = $*
    fi
}

if test_uname Linux ; then

   TOOLSET=
   CXXFLAGS=
   compiler=gcc-$(gcc -dumpversion)
   
   # When on cray check PE_ENV for environment
   if [ "$PE_ENV" = INTEL -o "$PE_ENV" = CRAY -o "$PE_ENV" = GNU ]
   then
       CXXFLAGS=cxxflags=-fPIC
       TOOLSET=toolset=gcc
       if [ "$PE_ENV" = INTEL ] ; then
          compiler=intel-linux
          TOOLSET=toolset=intel
       fi
       if [ "$PE_ENV" = CRAY ] ; then
          compiler=cray
          TOOLSET=toolset=cray
       fi
   fi   
   
   # Allow the compiler to be overridden on linux
   if [ ${#compiler_arg} -ne 0 ] ; then
      compiler=$compiler_arg
   fi

   echo "*****************************************"
   echo "Testing: variant=$mode compiler=$compiler"
   echo "*****************************************"

   ACore/bin/$compiler/$mode/coretest  --log_level=message $TEST_OPTS
   ANattr/bin/$compiler/$mode/nodeattrtest  --log_level=message $TEST_OPTS
   ANode/bin/$compiler/$mode/nodetest  --log_level=message $TEST_OPTS
   AParser/bin/$compiler/$mode/tparser  --log_level=message $TEST_OPTS
   if [ "$safe" = no ] ; then
      AParser/bin/$compiler/$mode/tsingle          --log_level=message $TEST_OPTS
   fi
   Base/bin/$compiler/$mode/basetest     --log_level=message $TEST_OPTS
   Client/bin/$compiler/$mode/tclient     --log_level=message $TEST_OPTS
   Server/bin/$compiler/$mode/tserver       --log_level=message $TEST_OPTS
   CSim/bin/$compiler/$mode/testsimulator  --log_level=message $TEST_OPTS
   if [ "$safe" = no ] ; then
      Test/bin/$compiler/$mode/server-test  --log_level=message $TEST_OPTS
      Test/bin/$compiler/$mode/test-zombies  --log_level=message $TEST_OPTS
   fi
   
   if [ "$safe" = no ] ; then
      # run python/C++ test
      cd Pyext
      $BOOST_ROOT/bjam $TOOLSET $CXXFLAGS variant=$mode test-all $TEST_OPTS
      cd ..
   fi
   
   if [ x$DISPLAY = x  ]; then
       echo "DISPLAY variable is not defined, ecflowview is not tested..."
   else
       view/bin/$compiler/$mode/test-view  --log_level=message $TEST_OPTS
   fi
   
   if [ "$safe" = no ] ; then
      # Allow this test to be disabled it can take a while
      if [ -n "$ECF_DISABLE_SMS_TEST" ] ; then   
         echo "Ignore SMS to ecf comparison test"
      else
         TestEcfSms/bin/$compiler/$mode/smsecftest  --log_level=message $TEST_OPTS
      fi
   fi
   
elif test_uname HP-UX ; then

   echo "Testing: variant=$mode"
   ACore/bin/acc/$mode/threading-multi/coretest  --log_level=message $TEST_OPTS
   ANattr/bin/acc/$mode/threading-multi/nodeattrtest  --log_level=message $TEST_OPTS
   ANode/bin/acc/$mode/threading-multi/nodetest  --log_level=message $TEST_OPTS
   AParser/bin/acc/$mode/threading-multi/tparser  --log_level=message $TEST_OPTS
   if [ "$safe" = no ] ; then
      AParser/bin/acc/$mode/threading-multi/tsingle  --log_level=message $TEST_OPTS
   fi
   Base/bin/acc/$mode/threading-multi/basetest  --log_level=message $TEST_OPTS
   Client/bin/acc/$mode/threading-multi/tclient  --log_level=message $TEST_OPTS
   Server/bin/acc/$mode/threading-multi/tserver  --log_level=message $TEST_OPTS
   CSim/bin/acc/$mode/threading-multi/testsimulator  --log_level=message $TEST_OPTS
   if [ "$safe" = no ] ; then
      Test/bin/acc/$mode/threading-multi/server-test  --log_level=message $TEST_OPTS
      Test/bin/acc/$mode/threading-multi/test-zombies  --log_level=message $TEST_OPTS
   fi
   
   if [ "$safe" = no ] ; then
      # run python/C++ test, use test to bypass 'with' statement tests
      cd Pyext
      $BOOST_ROOT/bjam variant=$mode test $TEST_OPTS
      cd ..
   fi

   if [ "$safe" = no ] ; then
      # Allow this test to be dissabled it can take a while
      if [ -n "$ECF_DISABLE_SMS_TEST" ] ; then   
         echo "Ignore SMS to ecf comparison test"
      else
         TestEcfSms/bin/acc/$mode/threading-multi/smsecftest  --log_level=message $TEST_OPTS
      fi
   fi
 
elif test_uname AIX ; then

   echo "Testing: $ARCH variant=$mode"
   
   ACore/bin/vacpp/$mode/threading-multi/coretest  --log_level=message $TEST_OPTS
   ANattr/bin/vacpp/$mode/threading-multi/nodeattrtest  --log_level=message $TEST_OPTS
   ANode/bin/vacpp/$mode/threading-multi/nodetest  --log_level=message $TEST_OPTS
   AParser/bin/vacpp/$mode/threading-multi/tparser  --log_level=message $TEST_OPTS
   if [ "$safe" = no ] ; then
      AParser/bin/vacpp/$mode/threading-multi/tsingle  --log_level=message $TEST_OPTS
   fi
   Base/bin/vacpp/$mode/threading-multi/basetest  --log_level=message $TEST_OPTS
   Client/bin/vacpp/$mode/threading-multi/tclient  --log_level=message $TEST_OPTS
   Server/bin/vacpp/$mode/threading-multi/tserver  --log_level=message $TEST_OPTS
   CSim/bin/vacpp/$mode/threading-multi/testsimulator  --log_level=message $TEST_OPTS
   if [ "$safe" = no ] ; then
      Test/bin/vacpp/$mode/threading-multi/server-test  --log_level=message $TEST_OPTS
      Test/bin/vacpp/$mode/threading-multi/test-zombies  --log_level=message $TEST_OPTS
   fi

   if [ "$safe" = no ] ; then
      # run python/C++ test
      cd Pyext
      $BOOST_ROOT/bjam variant=$mode test-all $TEST_OPTS
      cd ..
   fi
   
   if [ "$safe" = no ] ; then
      # Allow this test to be dissabled it can take a while
      if [ -n "$ECF_DISABLE_SMS_TEST" ] ; then   
         echo "Ignore SMS to ecf comparison test"
      else
         TestEcfSms/bin/vacpp/$mode/threading-multi/smsecftest  --log_level=message $TEST_OPTS
      fi
   fi
fi
