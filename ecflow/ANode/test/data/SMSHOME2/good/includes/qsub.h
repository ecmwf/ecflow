:
# QSUB -q %QUEUE%
# QSUB -u %USER%
# QSUB -eo
# QSUB -ro
# QSUB -ko
# QSUB -nr
# QSUB -A %ACCOUNT%
# QSUB -s /bin/ksh
# QSUB -r %TASK%-%FAMILY1%
# QSUB -o %LOGDIR%%SMSNAME%.%SMSTRYNO%
