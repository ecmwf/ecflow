#/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
# Name        :
# Author      : Avi
# Revision    : $Revision: #5 $
#
# Copyright 2009- ECMWF.
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

# wait for background process to stop.
# If we did not have background jobs, closly called foreround jobs
# may arrive out of order at the server, causing unnecessary zombies
# The wait should prevent this.
wait

# record shell time, see head.h
%ecfmicro !
finish_time=$(date +%s)
!ecfmicro %
echo "Job End: Time duration: $((finish_time - start_time)) secs."

# Notify ECF of a normal end
%ECF_CLIENT_EXE_PATH% --complete %COMPLETE_DEL_VARIABLES:%
trap 0                          # Remove all traps
exit 1                          # End the shell

