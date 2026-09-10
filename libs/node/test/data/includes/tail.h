# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

# ================================== start of tail.h ================================
#smscomplete  # Notify ECF of a normal end
%ECF_CLIENT_EXE_PATH:ecflow_client% --complete
trap 0       # Remove all traps
exit 0       # End the shell
%manual
#This is the manual from the tail.h file
%end
%comment
#This is the comment from the tail.h file
%end
# ================================== end of tail.h ================================
