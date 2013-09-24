wait                                            # wait for background process to stop
%ECF_CLIENT_EXE_PATH:ecflow_client% --complete  # Notify ECF of a normal end
trap 0                                          # Remove all traps
exit 0                                          # End the shell
