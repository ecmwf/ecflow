# ======================================================================
# Run LOCALLY
# ======================================================================
export WK=%WK%

# ======================================================================
# Determine version number: see ACore/doc/extracting_version_number.ddoc
# ======================================================================
cd $WK
ecflow_version=$(cat $WK/VERSION.cmake | awk '{print $3}'|sed 's/["]//g')
release=$(echo $ecflow_version | cut -d. -f1)
major=$(echo $ecflow_version   | cut -d. -f2)
minor=$(echo $ecflow_version   | cut -d. -f3)
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
      rsh %REMOTE_HOST% "cd $ROOT_DIR; rm -rf ecflow*"

      # cd to where tar file directory resides
      cd %ECFLOW_TAR_DIR%
      ecrcp $ECFLOWTAR.gz  %REMOTE_HOST%:$ROOT_DIR/
      rsh %REMOTE_HOST% "cd $ROOT_DIR; gunzip $ECFLOWTAR.gz; tar -xf $ECFLOWTAR; mv $ECFLOW ecflow"
   fi
fi
