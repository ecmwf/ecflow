# =========================================================
# We do ***not** want to hard wire version in the definition
# This will allow us to change version without having to
# modify the definition
# use [[ ]] for string comparison
# =========================================================
if [[ "%LOCAL_HOST:%" != ""  ]] ; then
   export WK=%WK%
else
   # When building on remote platform.
   export WK=%ROOT_WK:%/ecflow
fi
