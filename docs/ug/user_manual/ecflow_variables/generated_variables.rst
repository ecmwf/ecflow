.. _generated_variables:

Generated variables
///////////////////

ecFlow generates time and date variables in various formats from the
clock. There is a separate clock for every suite. Scripts can make use
of these generated variables. The variables are available at the suite
level and may be **overridden** by an **edit** keyword at the suite,
family, or task level. In ecflow_ui generated variables are bracketed,
e.g. (ECF_TRYNO = 0).

These variables are **generated** by ecFlow from the information in
the definition file and are available for use in ecFlow files.
Normally there is no need to **override** the value by using **edit**
statement in the definition file. 

The table below shows a list of generated variables.


.. list-table::
   :header-rows: 1

   * - Defined for
     - Variable name
     - Explanation
     - Example
   * - server
     - ECF_TRIES
     - The default number of tries for each task
     - 2
   * - server
     - ECF_PORT
     - The port number
     - 3141
   * - server
     - ECF_NODE
     - The hostname of the machine running ECF
     - host_1
   * - server
     - ECF_HOME
     - Home for all the ecFlow files
     - $CWD
   * - server
     - ECF_JOB_CMD
     - Command to be executed to send a job
     - %ECF_JOB% 1> %ECF_JOBOUT% 2>&1
   * - server
     - ECF_LISTS
     - Name of the ecFlow white-list file
     - ecf.lists
   * - server
     - ECF_PASS
     - Default password string to replace the real password, when communicating with the server.
     - DJP
   * - server
     - ECF_LOG
     - Name of the ecFlow history, or log file
     - ecf.log
   * - server
     - ECF_CHECK
     - Name of the checkpoint file
     - ecf.check
   * - server
     - ECF_CHECKOLD
     - Name of the backup of the checkpoint file
     - ecf.check.b
   * - Suite
     - SUITE
     - The name of the suite
     - Backarc
   * - server
     - DATE
     - Date of the suite in format DD.MM.YYYY
     - 21.02.2012
   * - server
     - DAY
     - Full name of the weekday
     - Sunday
   * - server
     - DD
     - Day of the month, with two digits
     - 07
   * - server
     - DOW
     - Day Of the Week
     - 0
   * - server
     - DOY
     - Day Of the Year
     - 52
   * - server
     - MM
     - The month of the year, with two digits
     - 02
   * - server
     - MONTH
     - Full name of the month
     - February
   * - server
     - YYYY
     - A year with four digits
     - 2012
   * - server
     - ECF_DATE
     - Single date in format YYYYMMDD
     - 20120221
   * - server
     - ECF_TIME
     - Time of the suites clock, HH:MM
     - 20:32
   * - server
     - ECF_CLOCK
     - Composite weekday, month, Day Of the Week, Day Of Year
     - sunday:february:0:52
   * - family
     - FAMILY
     - The name of the family, (avoid using)
     - get/ocean
   * - family
     - FAMILY1
     - The last part of the family, (avoid using)
     - Ocean
   * - task
     - TASK
     - The name of the task
     - Getobs
   * - task
     - ECF_RID
     - The Request ID of the job (only for running jobs)
     - PID
   * - task
     - ECF_TRYNO
     - The current try number for the task. After begin it is 1.The number is advanced if the job is re-run. Used for in output and job-file numbering.Since this variable must be numeric and is used in the file name generation, it should not be defined by the user.
     - 1
   * - task
     - ECF_NAME
     - Full name of the task
     - /backarc/get/getobs/getobs
   * - task
     - ECF_PASS
     - Password for the task to enable login to ECF
     - xyZ12Abx
   * - task
     - ECF_SCRIPT
     - The full pathname for the script(if ECFFILES was not used).The variable is composed as ECF_HOME/ECF_NAME.ecf,
     - /home/ma/map/ECF/back/ get/getobs/ getobs.ecf
   * - task
     - ECF_JOB
     - Name of the job file created by ECF.The variable is composed of ECF_HOME/ECF_NAME.jobECF_TRYNO.
     - /some/path/ back/get/ getobs/ getobs.job1
   * - task
     - ECF_JOBOUT
     - The filename of the job output. ecFlow makes the directory from ECF_HOME down to the last level.The variable is composed of ECF_HOME/ECF_NAME.ECF_TRYNO.
     - /some/path/ back/get/ getobs/ getobs.1


.. note::

  - **suite**: There are many variables derived from the **clock** of the suite.
  - **family**: For the variable **FAMILY** the value is generated from each family name by adding a slash, '/', in between.
  - **task**: The password exists only at submission time. During job execution, only the encrypted password is available in ECF. If a task does not have a variable **ECF_PASS**, ecFlow generates one. This is the only variable that is **not** searched in the normal way.
