## Copyright 2009-2012 ECMWF.
## This software is licensed under the terms of the Apache Licence version 2.0
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
## In applying this licence, ECMWF does not waive the privileges and immunities
## granted to it by virtue of its status as an intergovernmental organisation
## nor does it submit to any jurisdiction.

# ================================== start of tail.h ================================
#smscomplete  # Notify ECF of a normal end
%ECF_CLIENT_EXE_PATH% --complete
trap 0       # Remove all traps
exit 0       # End the shell
%manual
#This is the manual from the tail.h file
%end
%comment
#This is the comment from the tail.h file
%end
# ================================== end of tail.h ================================
