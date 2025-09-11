
Commands
========

User Commands
-------------

- :code:`CSyncCmd{CSyncCmd::NEWS, 0, 0, 0}` : News
- :code:`CSyncCmd{CSyncCmd::SYNC, 0, 0, 0}` : Sync
- :code:`CSyncCmd{0}` : Full Sync
- :code:`CSyncCmd{CSyncCmd::SYNC_CLOCK, 0, 0, 0}` : Sync Clock

- :code:`CtsNodeCmd{CtsNodeCmd::GET}` : Get definition
- :code:`CtsNodeCmd{CtsNodeCmd::GET_STATE}` : Get definition+state
- :code:`CtsNodeCmd{CtsNodeCmd::MIGRATE}` : Migrate

- :code:`CheckPtCmd{}` : Save Checkpoint file

- :code:`CtsCmd{CtsCmd::PING}` : Ping
- :code:`CtsCmd{CtsCmd::RESTORE_DEFS_FROM_CHECKPT}` : Load Checkpoint file
- :code:`CtsCmd{CtsCmd::RESTART_SERVER}` : Restart Server
- :code:`CtsCmd{CtsCmd::HALT_SERVER}` : Halt Server
- :code:`CtsCmd{CtsCmd::SHUTDOWN_SERVER}` : Shutdown Server
- :code:`CtsCmd{CtsCmd::TERMINATE_SERVER}` : Terminate Server
- :code:`CtsCmd{CtsCmd::RELOAD_WHITE_LIST_FILE}` : Reload whitelist file
- :code:`CtsCmd{CtsCmd::RELOAD_PASSWD_FILE}` : Reload password file
- :code:`CtsCmd{CtsCmd::RELOAD_CUSTOM_PASSWD_FILE}` : Reload custom password file
- :code:`CtsCmd{CtsCmd::FORCE_DEP_EVAL}` : Force dependency evaluation
- :code:`CtsCmd{CtsCmd::STATS}` : Retrieve server usage statistics
- :code:`CtsCmd{CtsCmd::STATS_SERVER}` : Retrieve server usage statistics (tests only)
- :code:`CtsCmd{CtsCmd::STATS_RESET}` : Reset server usage statistics
- :code:`CtsCmd{CtsCmd::DEBUG_SERVER_ON}` : Enable server debug
- :code:`CtsCmd{CtsCmd::DEBUG_SERVER_OFF}` : Disable server debug
- :code:`CtsCmd{CtsCmd::SERVER_LOAD}` : Generate Gnuplot files for server load

- :code:`CtsNodeCmd{CtsNodeCmd::JOB_GEN}` : Request job generation and immediate submission
- :code:`CtsNodeCmd{CtsNodeCmd::CHECK_JOB_GEN_ONLY}` : Test job generation (no submission)

- :code:`DeleteCmd{}` : Delete node definition

- :code:`PathsCmd{PathsCmd::SUSPEND}` : Suspend node
- :code:`PathsCmd{PathsCmd::RESUME}` : Resume node
- :code:`PathsCmd{PathsCmd::KILL} :` Kill job(s) associated with node
- :code:`PathsCmd{PathsCmd::STATUS}` : Retrieve node status
- :code:`PathsCmd{PathsCmd::CHECK}` : Validate node expression(s) and limits
- :code:`PathsCmd{PathsCmd::EDIT_HISTORY}`
- :code:`PathsCmd{PathsCmd::ARCHIVE}`
- :code:`PathsCmd{PathsCmd::RESTORE}`

- :code:`ZombieCmd{ecf::User::FOB}`
- :code:`ZombieCmd{ecf::User::FAIL}`
- :code:`ZombieCmd{ecf::User::ADOPT}`
- :code:`ZombieCmd{ecf::User::BLOCK}`
- :code:`ZombieCmd{ecf::User::REMOVE}`
- :code:`ZombieCmd{ecf::User::KILL}`

- :code:`CtsCmd{CtsCmd::GET_ZOMBIES}`
- :code:`CtsCmd{CtsCmd::SUITES}`

- :code:`ClientHandleCmd{ClientHandleCmd::REGISTER}`
- :code:`ClientHandleCmd{ClientHandleCmd::DROP}`
- :code:`ClientHandleCmd{ClientHandleCmd::DROP_USER}`
- :code:`ClientHandleCmd{ClientHandleCmd::ADD}`
- :code:`ClientHandleCmd{ClientHandleCmd::REMOVE}`
- :code:`ClientHandleCmd{ClientHandleCmd::AUTO_ADD}`
- :code:`ClientHandleCmd{ClientHandleCmd::SUITES}`

- :code:`LogCmd{}`

- :code:`ServerVersionCmd{}`

- :code:`LogMessageCmd{}`

- :code:`RequeueNodeCmd{}`

- :code:`OrderNodeCmd{}`

- :code:`RunNodeCmd{}`

- :code:`ForceCmd{}`

- :code:`FreeDepCmd{}`

- :code:`LoadDefsCmd{}`

- :code:`ReplaceNodeCmd{}`

- :code:`CFileCmd{}`

- :code:`EditScriptCmd{}`

- :code:`AlterCmd{}`

- :code:`QueryCmd{}`

- :code:`PlugCmd{}`

Task Commands
-------------

- :code:`BeginCmd{}`

- :code:`InitCmd{}`

- :code:`CompleteCmd{}`

- :code:`AbortCmd{}`

- :code:`CtsWaitCmd{}`

- :code:`EventCmd{}`

- :code:`MeterCmd{}`

- :code:`LabelCmd{}`

- :code:`QueueCmd{}`

Other Commands
--------------

- :code:`CtsNodeCmd{CtsNodeCmd::WHY}`

- :code:`GroupCTSCmd{}`
