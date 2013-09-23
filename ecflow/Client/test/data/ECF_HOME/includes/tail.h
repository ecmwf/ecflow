# wait for background process to stop.
# If we did not have background jobs, closly called foreround jobs
# may arrive out of order at the server, causing unnecessary zombies
# The wait should prevent this.

wait         # wait for background process to stop
smscomplete  # Notify ECF of a normal end
trap 0       # Remove all traps
exit 0       # End the shell
