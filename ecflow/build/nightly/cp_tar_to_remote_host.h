# ======================================================================
# Run LOCALLY
# ======================================================================
export WK=%WK%

# ======================================================================
# Determine version number: see ACore/doc/extracting_version_number.ddoc
# ======================================================================
cd $WK
release=$(grep "Version::release_" ACore/src/Version.cpp  | cut -d= -s -f2 | sed 's/;//g' | sed 's/ //g')
major=$(grep   "Version::major_"   ACore/src/Version.cpp  | cut -d= -s -f2 | sed 's/;//g' | sed 's/ //g')
minor=$(grep   "Version::minor_"   ACore/src/Version.cpp  | cut -d= -s -f2 | sed 's/;//g' | sed 's/ //g' | sed 's/"//g' )
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
      cd $WK
      cd ..
      scp $ECFLOWTAR.gz  %REMOTE_HOST%:$ROOT_DIR/
      ssh %USER%@%REMOTE_HOST% "cd $ROOT_DIR; gunzip $ECFLOWTAR.gz; tar -xf $ECFLOWTAR; mv $ECFLOW ecflow"

   else

      # Note: we deal with different version, by removing ecflow_* first
      rsh %REMOTE_HOST% "cd $ROOT_DIR; rm -rf ecflow*"

      # cd to where tar file directory resides
      cd $WK
      cd ..
      ecrcp $ECFLOWTAR.gz  %REMOTE_HOST%:$ROOT_DIR/
      rsh %REMOTE_HOST% "cd $ROOT_DIR; gunzip $ECFLOWTAR.gz; tar -xf $ECFLOWTAR; mv $ECFLOW ecflow"
   fi
fi
