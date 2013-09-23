#!/bin/ksh
:
# QSUB -q %QUEUE%
# QSUB -u %USER%
# QSUB -eo
# QSUB -ro
# QSUB -ko
# QSUB -nr
# QSUB -A %ACCOUNT%
# QSUB -s /bin/ksh
# QSUB -r %TASK%_%FAMILY1:NOT_DEF%
# QSUB -o %LOGDIR%%ECF_NAME%.%ECF_TRYNO%
# QSUB -lh %THREADS:1%
# QSUB -lMM 16000Mb
