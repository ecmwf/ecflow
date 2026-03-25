
# Wait for background process to stop.
# If we did not have background jobs, closly called foreround jobs
# may arrive out of order at the server, causing unnecessary zombies
# The wait should prevent this.
wait

# Record time between init and complete see head.h
%ecfmicro !
finish_time=$(date +%s)
!ecfmicro %
echo "Job End: Time duration: $((finish_time - start_time)) secs *BETWEEN* init and complete"

# Notify ecflow server of a normal task completion
%ECF_CLIENT_EXE_PATH% --complete %COMPLETE_DEL_VARIABLES:%

%ecfmicro !
job_finish_time=$(date +%s)
!ecfmicro %
echo "Job End: *COMPLETE*  took    : $((job_finish_time - finish_time)) secs"
echo "Job End: *JOB* duration took : $((job_finish_time - job_start_time)) secs"

trap 0                          # Remove all traps
exit 0                          # End the shell
