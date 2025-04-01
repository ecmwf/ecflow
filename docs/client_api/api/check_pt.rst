
.. _check_pt_cli:

check_pt
////////

::

   
   check_pt
   --------
   
   Forces the definition file in the server to be written to disk *or* allow mode,
   interval and alarm to be changed.
   Whenever the check pt file is written to disk, it is measured.
   If the time to save to disk is greater than the default of 30 seconds,
   then an alarm is raised. This can be seen in the GUI as a late flag on the server.
   Once the late flag has been set it will need to manually cleared in the GUI
   or by using --alter functionality
   Note excessive save times can interfere with job scheduling.
   The alarm threshold can be changed. See below.
      arg1 = (optional) mode [ never | on_time | on_time:<integer> | always | <integer>]
        never     : Never check point the definition in the server
        on_time   : Turn on automatic check pointing at interval stored on server
        on_time<integer> : Turn on automatic check point, with the specified interval in seconds
        alarm<integer>   : Modify the alarm notification time for check pt saving to disk
        always    : Check point at any change in node tree, *NOT* recommended for large definitions
        <integer> : This specifies the interval in seconds when server should automatically check pt.
                    This will only take effect of mode is on_time/CHECK_ON_TIME
                    Should ideally be a value greater than 60 seconds, default is 120 seconds
   Usage:
     --check_pt
       Immediately check point the definition held in the server
     --check_pt=never
       Switch off check pointing
     --check_pt=on_time
       Start automatic check pointing at the interval stored in the server
     --check_pt=180
       Change the check pt interval to 180 seconds
     --check_pt=on_time:90
       Change mode and interval, to automatic check pointing every 90 seconds
     --check_pt=alarm:35
       Change the alarm time for check pt saves. i.e if saving the check pt takes longer than 35 seconds
       set the late flag on the server.
   
   The client reads in the following environment variables. These are read by user and child command
   
   |----------|----------|------------|-------------------------------------------------------------------|
   | Name     |  Type    | Required   | Description                                                       |
   |----------|----------|------------|-------------------------------------------------------------------|
   | ECF_HOST | <string> | Mandatory* | The host name of the main server. defaults to 'localhost'         |
   | ECF_PORT |  <int>   | Mandatory* | The TCP/IP port to call on the server. Must be unique to a server |
   | ECF_SSL  |  <any>   | Optional*  | Enable secure communication between client and server.            |
   |----------|----------|------------|-------------------------------------------------------------------|
   
   * The host and port must be specified in order for the client to communicate with the server, this can 
     be done by setting ECF_HOST, ECF_PORT or by specifying --host=<host> --port=<int> on the command line
   
