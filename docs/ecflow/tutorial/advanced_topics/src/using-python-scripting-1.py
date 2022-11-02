# provides *examples* of add adding node attributes using the python API
# hence does *not* represent a real suite definition
from ecflow import *
    
if __name__ == "__main__":

    # adding variables 
    suite = Suite("s1");
    suite.add_variable(Variable("ECF_HOME", "/tmp/"))
    suite.add_variable("ECF_URL_CMD", "${BROWSER:=firefox} -remote 'openURL(%ECF_URL_BASE%/%ECF_URL%)'")
    suite.add_variable("NAME", 10)
    
    a_dict = { "name":"value", "name2":"value2", "name3":"value3", "name4":"value4" }
    suite.add_variable(a_dict)

     
    # adding limits
    suite.add_limit(  Limit("limitName1", 10) )
    suite.add_limit(  "limitName3", 10 )
  
    # adding inlimits
    suite.add_inlimit( InLimit("limitName1", "/s1/f1", 2) )
    suite.add_inlimit( "limitName3", "/s1/f1", 2)
 
    # add short triggers and complete
    task = Task("task")
    task.add_trigger( "t2 == active" )
    task.add_complete( "t2 == complete" )
      
    # add long triggers and complete, in example below 'True' mean AND and 'False' means OR
    task = Task("trigger")
    task.add_part_trigger(  "t1 == complete" )
    task.add_part_trigger(  "t2 == active", True )  #  for long and/or expressions, subsequent expr must be and/or
    task.add_part_complete( "t3 == complete" )
    task.add_part_complete( "t4 == active", False)  #  for long and/or expressions, subsequent expr must be and/or
    
    # add events
    task.add_event( Event(1) )
    task.add_event( 2 )
    task.add_event( Event(10, "Eventname") )
    task.add_event( 10, "Eventname2" )
    task.add_event( "fred" )

    # add meter
    task.add_meter( Meter("metername1", 0, 100, 50) )
    task.add_meter( "metername3", 0, 100 )
  
    # add label
    task.add_label( Label("label_name1", "value") )
    task.add_label( "label_name3", "value" )
 
    # add Repeat. A node can only have one repeat, hence we delete the repeat before, adding another
    task.add_repeat( RepeatInteger("integer", 0, 100, 2) )  
    task.delete_repeat()     
    task.add_repeat( RepeatEnumerated("enum", ["red", "green", "blue" ] ) )
    task.delete_repeat()     
    task.add_repeat( RepeatDate("date", 20100111, 20100115, 2) )
    task.delete_repeat()         
    task.add_repeat( RepeatString("string", ["a", "b", "c" ] ) )
    task.delete_repeat()         
    
    # create a time series, used for adding time and today and cron
    start = TimeSlot(0, 0)
    finish = TimeSlot(23, 0)
    incr = TimeSlot(0, 30)
    time_series = TimeSeries( start, finish, incr, True) # True means relative to suite start
  
    # add a today
    task.add_today( "00:30" )
    task.add_today( "+00:30" )
    task.add_today( "+00:30 20:00 01:00" )
    task.add_today( Today( time_series) )
    task.add_today( Today( 0, 10 ))
    task.add_today( 0, 59, True )
    task.add_today( Today(TimeSlot(20, 10)) )
    task.add_today( Today(TimeSlot(20, 20), False)) 
                    
    # add time
    task.add_time( "00:30" )
    task.add_time( "+00:30" )
    task.add_time( "+00:30 20:00 01:00" )
    task.add_time( Time(time_series ))
    task.add_time( Time( 0, 10 ))
    task.add_time( 0, 59, True)
    task.add_time( Time(TimeSlot(20, 10)) )
    task.add_time( Time(TimeSlot(20, 20), False)) 
 
    # add date
    for i in [ 1, 2, 4, 8, 16 ] :
        task.add_date( i, 0, 0)    # day,month,year, where corresponding 0 means any possible day,month, year
    task.add_date( Date(1, 1, 2010))

    # add day
    task.add_day( Day(Days.sunday))
    task.add_day( Days.monday)
    task.add_day( "tuesday")
    
    # create cron, showing different ways adding the time, to a cron attribute
    cron = Cron()
    cron.set_week_days( [0, 1, 2, 3, 4, 5, 6]  )
    cron.set_days_of_month( [1, 2, 3, 4, 5, 6] )
    cron.set_months( [1, 2, 3, 4, 5, 6] )
    start = TimeSlot(0, 0)
    finish = TimeSlot(23, 0)
    incr = TimeSlot(0, 30)
    ts = TimeSeries( start, finish, incr, True)  # True means relative to suite start
    cron.set_time_series( ts )

    cron1 = Cron()
    cron1.set_week_days( [0, 1, 2, 3, 4, 5, 6] )
    cron1.set_time_series( 1, 30,  True )
    
    cron2 = Cron()
    cron2.set_week_days(  [0, 1, 2, 3, 4, 5, 6] )
    cron2.set_time_series( "00:30 01:30 00:01" )
    
    cron3 = Cron()
    cron3.set_week_days( [0, 1, 2, 3, 4, 5, 6] )
    cron3.set_time_series( "+00:30" )
    
    # add auto cancel
    t1 = Task("t1");
    t3 = Task("t3")
    t4 = Task("t4")
    t5 = Task("t5")
    t1.add_autocancel( 3 )                       # 3 days
    t3.add_autocancel( 20, 10, True )            # hour,minutes,relative
    t4.add_autocancel( TimeSlot(10, 10), True )  # hour,minutes,relative
    t5.add_autocancel( Autocancel(1, 10, True) ) # hour,minutes,relative
    
    # add late
    late = Late()
    late.submitted( TimeSlot(20, 10))
    late.active(    TimeSlot(20, 10))
    late.complete(  TimeSlot(20, 10), True)
    task.add_late(  late )
    
    late = Late()
    late.submitted( 20, 10 )
    late.active(    20, 10 )
    late.complete(  20, 10, True)
    t1.add_late( late )
    
    # add defstatus, last one set takes effect
    task.add_defstatus( DState.complete )
    task.add_defstatus( DState.queued )
    task.add_defstatus( DState.aborted )
    task.add_defstatus( DState.submitted )
    task.add_defstatus( DState.suspended )
    task.add_defstatus( DState.active )
    
    # add clock
    clock = Clock(1, 1, 2010, False)    # day,month, year, hybrid(true), real(False)
    clock.set_gain(1, 10, True)         # True means positive gain
    suite =  Suite("suite")
    suite.add_clock(clock)
    
    clock = Clock(False)                #  Use the current time and date with real time clock
    clock.set_gain_in_seconds(12, True)
    s1 = Suite("s1")
    s1.add_clock(clock)    