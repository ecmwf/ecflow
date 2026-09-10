# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

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
