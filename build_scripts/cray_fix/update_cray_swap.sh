#!/bin/ksh

# ==================================================================
# Setup environment on CRAY, allow switch between cray,intel and gnu
# ==================================================================

scp $WK/build/cray_fix/swap.sh cca:/home/ma/ma0/.
 
echo "copied swap.sh to cca:/home/ma/ma0/."
