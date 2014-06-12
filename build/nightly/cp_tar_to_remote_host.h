# ======================================================================
# Run LOCALLY
# ======================================================================
export WK=%WK%

# ======================================================================
# Determine version number: see ACore/doc/extracting_version_number.ddoc
# ======================================================================
cd $WK
release=$(cat VERSION.cmake | grep 'set( ECFLOW_RELEASE' | awk '{print $3}'| sed 's/["]//g')
major=$(cat VERSION.cmake   | grep 'set( ECFLOW_MAJOR'   | awk '{print $3}'| sed 's/["]//g')
minor=$(cat VERSION.cmake   | grep 'set( ECFLOW_MINOR'   | awk '{print $3}'| sed 's/["]//g')
ECFLOW=ecflow_${release}_${major}_${minor}
ECFLOWTAR=$ECFLOW.tar

if [[ %SET_TO_TEST_SCRIPT% = true ]] ; then
   echo "Test only"
else
   # ===============================================================================================
   # %REMOTE_HOST%
   # ===============================================================================================
   ROOT_DIR=%ROOT_WK%

   if [[ %ARCH% = opensuse113 || %ARCH% = redhat ]] ; then

      # ===================================================================================
      # opensuse113 and redhat use SSH
      # ===================================================================================
      # Note: we deal with different version, by removing ecflow_*
      ssh %USER%@%REMOTE_HOST% "cd $ROOT_DIR; rm -rf ecflow*"

      # cd to where tar file directory resides
      cd %ECFLOW_TAR_DIR%
      scp $ECFLOWTAR.gz  %REMOTE_HOST%:$ROOT_DIR/
      ssh %USER%@%REMOTE_HOST% "cd $ROOT_DIR; gunzip $ECFLOWTAR.gz; tar -xf $ECFLOWTAR; mv $ECFLOW ecflow"

   else

      # Note: we deal with different version, by removing ecflow_* first
      if [[ %ARCH% = hpux ]] ; then
         # hpux is on scratch, no need to rsh
         cd $ROOT_DIR; rm -rf ecflow*

        # cd to where tar file directory resides
        cd %ECFLOW_TAR_DIR%
        scp $ECFLOWTAR.gz $ROOT_DIR/
        cd $ROOT_DIR; gunzip $ECFLOWTAR.gz; tar -xf $ECFLOWTAR; mv $ECFLOW ecflow

      else
         rsh %REMOTE_HOST% "cd $ROOT_DIR; rm -rf ecflow*"

         # cd to where tar file directory resides
         cd %ECFLOW_TAR_DIR%
         ecrcp $ECFLOWTAR.gz  %REMOTE_HOST%:$ROOT_DIR/
         rsh %REMOTE_HOST% "cd $ROOT_DIR; gunzip $ECFLOWTAR.gz; tar -xf $ECFLOWTAR; mv $ECFLOW ecflow"
      fi
   fi
fi
