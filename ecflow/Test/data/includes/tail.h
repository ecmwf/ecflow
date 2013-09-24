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
%ECF_CLIENT_EXE_PATH% --complete
trap 0                          # Remove all traps
exit 0                          # End the shell

