/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #36 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : This class is used as a helper class
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include "NodeAttrDoc.hpp"

const char* NodeAttrDoc::variable_doc()
{
   return
            "Defines a :term:`variable` on a :term:`node` for use in :term:`ecf script`.\n\n"
            "A Node can have a number of variables.\n"
            "These variables can be added at any node level: :term:`suite`, :term:`family` or :term:`task`.\n"
            "The variables are names inside a pair of '%' characters in an :term:`ecf script`.\n"
            "The content of a variable replaces the variable name in the :term:`ecf script` at\n"
            "job submission time. When a variable is needed at submission time, it is first\n"
            "sought in the task itself. If it is not found, it is sought from the tasks parent\n"
            "and so on, up through the node levels until found. See :term:`variable inheritance`\n"
            "A undefined variable in a :term:`ecf script`, causes the :term:`task` to be :term:`aborted`,\n"
            "without the job being submitted.\n"
            "\nConstructor::\n\n"
            "   Variable(name,value)\n"
            "      string name: the name of the variable\n"
            "      string value: The value of the variable\n"
            "\nUsage::\n\n"
            "   ..."
            "   var = Variable(\"ECF_JOB_CMD\",\"/bin/sh %ECF_JOB% &\")\n"
            "   task.add_variable(var)\n"
            "   task.add_variable('JOE','90')\n"
            ;
}

const char* NodeAttrDoc::zombie_doc()
{
   return
            "The :term:`zombie` attribute defines how a :term:`zombie` should be handled in an automated fashion\n\n"
            "Very careful consideration should be taken before this attribute is added\n"
            "as it may hide a genuine problem.\n"
            "It can be added to any :term:`node`. But is best defined at the :term:`suite` or :term:`family` level.\n"
            "If there is no zombie attribute the default behaviour is to block the init,complete,abort :term:`child command`.\n"
            "and *fob* the event,label,and meter :term:`child command`\n"
            "This attribute allows the server to make a automated response\n"
            "Please see: :py:class:`ecflow.ZombieType`, :py:class:`ecflow.ChildCmdType`, :py:class:`ecflow.ZombieUserActionType`\n"
            "\nConstructor::\n\n"
            "   ZombieAttr(ZombieType,ChildCmdTypes, ZombieUserActionType, lifetime)\n"
            "      ZombieType            : Must be one of ZombieType.ecf, ZombieType.path, ZombieType.user\n"
            "      ChildCmdType          : A list(ChildCmdType) of Child commands. Can be left empty in\n"
            "                              which case the action affect all child commands\n"
            "      ZombieUserActionType  : One of [ fob, fail, block, remove, adopt ]\n"
            "      int lifetime<optional>: Defines the life time in seconds of the zombie in the server.\n"
            "                              On expiration, zombie is removed automatically\n"
            "\nUsage::\n\n"
            "   # Add a zombie attribute so that child label commands(i.e ecflow_client --label)\n"
            "   # never block the job\n"
            "   s1 = ecflow.Suite('s1')\n"
            "   child_list = [ ChildCmdType.label ]\n"
            "   zombie_attr = ZombieAttr(ZombieType.ecf, child_list, ZombieUserActionType.fob)\n"
            "   s1.add_zombie(zombie_attr)\n"
            ;
}

const char* NodeAttrDoc::zombie_type_doc()
{
   return
            ":term:`zombie` s are running jobs that fail authentication when communicating with the :term:`ecflow_server`.\n\n"
            "See class :term:`zombie type` and :py:class:`ecflow.ZombieAttr` for further information.\n"
            ;
}

const char* NodeAttrDoc::zombie_user_action_type_doc()
{
   return
            "ZombieUserActionType is used define an automated response. See class :py:class:`ZombieAttr`\n\n"
            "This can be either on the client side or on the server side\n"
            "\nclient side:\n\n"
            "- fob:    The :term:`child command` always succeeds, i.e allowed to complete without blocking\n"
            "- fail:   The :term:`child command` is asked to fail.\n"
            "- block:  The :term:`child command` is asked to block. This is the default action for all child commands\n"
            "\nserver side:\n\n"
            "- adopt:  Allows the password supplied with the :term:`child command` s, to be adopted by the server\n"
            "- kill:   Kills the zombie process associated with the :term:`child command` using ECF_KILL_CMD.\n"
            "          path zombies will need to be killed manually. If kill is specified for path zombies\n"
            "          they will be fobed, i.e allowed to complete without blocking the job.\n"
            "- remove: :term:`ecflow_server` removes the :term:`zombie` from the zombie list.\n"
            "          The child continues blocking. The :term:`zombie` may well re-appear\n\n"
            "Note: Only adopt will allow the :term:`child command` to continue and change the :term:`node` tree\n"
            ;
}

const char* NodeAttrDoc::child_cmd_type_doc()
{
   return
            "ChildCmdType represents the different :term:`child command` s.\n"
            "This type is used as a parameter to the class :py:class:`ecflow.ZombieAttr`\n\n"
            "Child commands are called within a :term:`job file`::\n\n"
            "  ChildCmdType::init     corresponds to : ecflow_client --init=<process_id>\n"
            "  ChildCmdType::event    corresponds to : ecflow_client --event=<event_name | number>\n"
            "  ChildCmdType::meter    corresponds to : ecflow_client --meter=<meter_name>, <meter_value>\n"
            "  ChildCmdType::label    corresponds to : ecflow_client --label=<label_name>. <label_value>\n"
            "  ChildCmdType::wait     corresponds to : ecflow_client --wait=<expression>\n"
            "  ChildCmdType::abort    corresponds to : ecflow_client --abort=<reason>\n"
            "  ChildCmdType::complete corresponds to : ecflow_client --complete\n"
            ;
}

const char* NodeAttrDoc::label_doc()
{
   return
            "A :term:`label` has a name and value and provides a way of displaying information in a GUI.\n\n"
            "The value can be anything(ASCII) as it can not be used in triggers\n"
            "The value of the label is set to be the default value given in the definition\n"
            "when the :term:`suite` is begun. This is useful in repeated suites: A task sets the label\n"
            "to be something, e.g, the number of observations, and once the :term:`suite` is :term:`complete`\n"
            "and the next day starts) the number of observations is cleared.\n"
            "Labels can be set at any level: Suite,Family,Task\n"
            "There are two ways of updating the label\n"
            "- A :term:`child command` can be used to automatically update the label on a :term:`task`\n"
            "- By using the alter command, the labels on :term:`suite` :term:`family` and :term:`task` can be changed manually\n"
            "\nConstructor::\n\n"
            "   Label(name,value)\n"
            "      string name:  The name of the label\n"
            "      string value: The value of the label\n"
            "\nUsage::\n\n"
            "   t1 = Task('t1')\n"
            "   t1.add_label('l1','value')\n"
            "   t1.add_label(Label('l2','value2'))\n"
            "   for label in t1.labels:\n"
            "      print label\n"
            ;
}

const char* NodeAttrDoc::limit_doc()
{
   return
            ":term:`limit` provides a simple load management\n\n"
            "i.e. by limiting the number of :term:`task` s submitted by a server.\n"
            "Limits are typically defined at the :term:`suite` level, or defined in a\n"
            "separate suite, so that they can be used by multiple suites.\n"
            "Once a limit is defined in a :term:`suite definition`, you must also assign families/tasks to use\n"
            "this limit. See  :term:`inlimit` and :py:class:`ecflow.InLimit`\n"
            "\nConstructor::\n\n"
            "   Limit(name,value)\n"
            "      string name: the name of the limit\n"
            "      int   value: The value of the limit\n"
            "\nUsage::\n\n"
            "   limit = Limit(\"fast\", 10)\n"
            "    ...\n"
            "   suite.add_limit(limit)\n"
            ;
}

const char* NodeAttrDoc::inlimit_doc()
{
   return
            ":term:`inlimit` is used in conjunction with :term:`limit` to provide simple load management::\n\n"
            "   suite x\n"
            "      limit fast 1\n"
            "      family f\n"
            "         inlimit /x:fast\n"
            "         task t1\n"
            "         task t2\n\n"
            "Here 'fast' is the name of limit and the number defines the maximum number of tasks\n"
            "that can run simultaneously using this limit. Thats why you do not need a :term:`trigger`\n"
            "between tasks 't1' and 't2'. There is no need to change the tasks. The jobs are\n"
            "created in the order they are defined\n"
            "\nConstructor::\n\n"
            "   InLimit(name, optional<path = ''>, optional<token =  1>)\n"
            "      string name           : The name of the referenced Limit\n"
            "      string path<optional> : The path to the Limit, if this is left out, then Limit of 'name' must be specified\n"
            "                              some where up the parent hierarchy\n"
            "      int value<optional>   : The usage of the Limit. Each job submission will consume 'value' tokens\n"
            "                              from the Limit. defaults to 1 if no value specified.\n"
            "\nUsage::\n\n"
            "   inlimit = InLimit(\"fast\",\"/x/f\", 2)\n"
            "    ...\n"
            "   family.add_inlimit(inlimit)\n"
            ;
}

const char* NodeAttrDoc::event_doc()
{
   return
            ":term:`event` s are used as signal mechanism.\n\n"
            "Typically they would be used to signal partial completion of a :term:`task`\n"
            "and to be able to :term:`trigger` another job, which is waiting for this partial completion.\n"
            "Only tasks can have events that are automatically set via a :term:`child command` s, see below.\n"
            "Events are cleared automatically when a :term:`node` is re-queued or begun.\n"
            "Suites and Families can have tasks, but these events must be set via the Alter command\n"
            "Multiple events can be added to a task.\n"
            "An Event has a number and a optional name. Events are typically used\n"
            "in :term:`trigger` and :term:`complete expression` , to control job creation.\n"
            "Event are fired within a :term:`job file`, i.e.::\n\n"
            "   ecflow_client --init=$$\n"
            "   ecflow_client --event=foo\n"
            "   ecflow_client --complete\n\n"
            "Hence the defining of an event for a :term:`task`, should be followed with the addition of ecflow_client --event\n"
            ":term:`child command` in the corresponding :term:`ecf script` file.\n"
            "\nConstructor::\n\n"
            "   Event(number, optional<name = ''>)\n"
            "      int number            : The number must be >= 0\n"
            "      string name<optional> : If name is given, can only refer to Event by its name\n"
            "\nUsage::\n\n"
            "   event = Event(2,\"event_name\")\n"
            "   task.add_event(event)\n"
            "   task1.add_event(\"2\")      # create a event '1' and add to the task\n"
            "   task2.add_event(\"name\")   # create a event 'name' and add to task\n"
            ;
}

const char* NodeAttrDoc::meter_doc()
{
   return
            ":term:`meter` s can be used to indicate proportional completion of :term:`task`\n\n"
            "They are able to :term:`trigger` another job, which is waiting on this proportion.\n"
            "Can also be used to indicate progress of a job. Meters can be used in\n"
            ":term:`trigger` and :term:`complete expression`.\n"
            "\nConstructor::\n\n"
            "   Meter(name,min,max,<optional>color_change)\n"
            "      string name                : The meter name\n"
            "      int min                    : The minimum and initial meter value\n"
            "      int max                    : The maximum meter value. Must be greater than min value.\n"
            "      int color_change<optional> : default = max, Must be between min-max, used in the GUI\n"
            "\nExceptions:\n\n"
            "- raises IndexError when an invalid Meter is specified\n"
            "\nUsage:\n\n"
            "Using a meter requires:\n\n"
            "- Defining a meter on a :term:`task`::\n\n"
            "     meter = Meter(\"progress\",0,100,100)\n"
            "     task.add_meter(meter)\n\n"
            "- Updating the corresponding :term:`ecf script` file with the meter :term:`child command`::\n\n"
            "     ecflow_client --init=$$\n"
            "     for  i in 10 20 30 40 50 60 80 100; do\n"
            "         ecflow_client --meter=progress $i\n"
            "         sleep 2 # or do some work\n"
            "     done\n"
            "     ecflow_client --complete\n\n"
            "- Optionally addition in a :term:`trigger` or :term:`complete expression` for job control::\n\n"
            "     trigger task:progress ge 60\n\n"
            "  trigger and complete expression should *avoid* using equality i.e::\n\n"
            "     trigger task:progress == 60\n\n"
            "  Due to network issues the meter event's may **not** arrive in sequential order\n"
            "  hence the :term:`ecflow_server` will ignore meter value's, which are less than the current value\n"
            "  as a result triggers's which use meter equality may never evaluate\n"
            ;
}

const char* NodeAttrDoc::date_doc()
{
   return
            "Used to define a :term:`date` dependency.\n\n"
            "There can be multiple Date dependencies for a :term:`node`.\n"
            "Any of the 3 attributes, i.e. day, month, year can be wild carded using a zero\n"
            "If a hybrid :term:`clock` is defined on a suite, any node held by a date dependency\n"
            "will be set to :term:`complete` at the beginning of the :term:`suite`, without the\n"
            "task ever being dispatched otherwise, the suite would never complete.\n"
            "\nConstructor::\n\n"
            "   Date(day,month,year)\n"
            "      int day   : represents the day, zero means wild card. day >= 0 & day < 31\n"
            "      int month : represents the month, zero means wild card. month >= 0 & month < 12\n"
            "      int year  : represents the year, zero means wild card. year >= 0\n"
            "\nExceptions:\n\n"
            "- raises IndexError when an invalid date is specified\n"
            "\nUsage::\n\n"
            "   date = Date(11,12,2010)  # represent 11th of December 2010\n"
            "   date = Date(1,0,0);      # means the first day of every month of every year\n"
            ;
}

const char* NodeAttrDoc::day_doc()
{
   return
            "Defines a :term:`day` dependency.\n\n"
            "There can be multiple day dependencies. If a hybrid :term:`clock` is defined\n"
            "on a suite, any node held by a day dependency will be set to :term:`complete` at the\n"
            "beginning of the :term:`suite`, without the task ever being dispatched otherwise\n"
            "the suite would never complete.\n"
            "\nConstructor::\n\n"
            "   Day(Days)\n"
            "      Days day: Is an enumerator with represent the days of the week\n"
            "\nUsage::\n\n"
            "   day1 = Day(Days.sunday)\n"
            "   day2 = Day(Days.monday)\n"
            ;
}

const char* NodeAttrDoc::days_enum_doc()
{
   return
            "This enum is used as argument to a :py:class:`ecflow.Day` class.\n\n"
            "It represents the days of the week\n"
            "\nUsage::\n\n"
            "   day1 = Day(Days.sunday)\n"
            "   day2 = Day(Days.monday)\n"
            "   day3 = Day(Days.tuesday)\n"
            ;
}

const char* NodeAttrDoc::time_doc()
{
   return
            "Is used to define a :term:`time` dependency\n\n"
            "This can then control job submission.\n"
            "There can be multiple time dependencies for a node, however overlapping times may\n"
            "cause unexpected results. The time dependency can be made relative to the beginning\n"
            "of the suite or in repeated families relative to the beginning of the repeated family.\n"
            "\nConstructor::\n\n"
            "   Time(hour,minute,relative<optional> = false)\n"
            "      int hour:               hour in 24 clock\n"
            "      int minute:             minute <= 59\n"
            "      bool relative<optional>: default = False, Relative to suite start or repeated node.\n\n"
            "   Time(single,relative<optional> = false)\n"
            "      TimeSlot single:         A single time\n"
            "      bool relative:           Relative to suite start or repeated node. Default is false\n\n"
            "   Time(start,finish,increment,relative<optional> = false)\n"
            "      TimeSlot start:          The start time\n"
            "      TimeSlot finish:         The finish/end time\n"
            "      TimeSlot increment:      The increment\n"
            "      bool relative<optional>: default = False, relative to suite start or repeated node\n\n"
            "   Time(time_series)\n"
            "      TimeSeries time_series:Similar to constructor above\n"
            "\nExceptions:\n\n"
            "- raises IndexError when an invalid Time is specified\n"
            "\nUsage::\n\n"
            "   time = Time( 10,10 )                                                   #  time 10:10 \n"
            "   time = Time( TimeSlot(10,10), true)                                    #  time +10:10 \n"
            "   time = Time( TimeSlot(10,10), TimeSlot(20,10),TimeSlot(0,10), false )  #  time 10:10 20:10 00:10 \n"
            ;
}

const char* NodeAttrDoc::today_doc()
{
   return
            ":term:`today` is a time dependency that does not wrap to tomorrow.\n\n"
            "If the :term:`suite` s begin time is past the time given for the Today,\n"
            "then the node is free to run.\n"
            "\nConstructor::\n\n"
            "   Today(hour,minute,relative<optional> = false)\n"
            "      int hour               : hour in 24 clock\n"
            "      int minute             : minute <= 59\n"
            "      bool relative<optional>: Default = false,Relative to suite start or repeated node.\n\n"
            "   Today(single,relative<optional> = false)\n"
            "      TimeSlot single        : A single time\n"
            "      bool relative          : Relative to suite start or repeated node. Default is false\n\n"
            "   Today(start,finish,increment,relative<optional> = false)\n"
            "      TimeSlot start         : The start time\n"
            "      TimeSlot finish        : The finish/end time. This must be greater than the start time.\n"
            "      TimeSlot increment     : The increment\n"
            "      bool relative<optional>: Default = false, Relative to suite start or repeated node.\n\n"
            "   Today(time_series)\n"
            "      TimeSeries time_series: Similar to constructor above\n"
            "\nExceptions:\n\n"
            "- raises IndexError when an invalid Today is specified\n"
            "\nUsage::\n\n"
            "   today = Today( 10,10 )                                                   #  today 10:10 \n"
            "   today = Today( TimeSlot(10,10) )                                         #  today 10:10 \n"
            "   today = Today( TimeSlot(10,10), true)                                    #  today +10:10 \n"
            "   today = Today( TimeSlot(10,10), TimeSlot(20,10),TimeSlot(0,10), false )  #  time 10:10 20:10 00:10 \n"
            ;
}

const char* NodeAttrDoc::late_doc()
{
   return
            "Sets the :term:`late` flag.\n\n"
            "When a Node is classified as being late, the only action :term:`ecflow_server` can take\n"
            "is to set a flag. The GUI will display this alongside the :term:`node` name as a icon.\n"
            "Only one Late attribute can be specified on a Node.\n"
            "\nConstructor::\n\n"
            "   Late()\n"
            "\nUsage::\n\n"
            "   late = Late()\n"
            "   late.submitted( 0,15 )\n"
            "   late.active(   20,0 )\n"
            "   late.complete(  2,0, true )\n\n"
            "This is interpreted as: The node can stay :term:`submitted` for a maximum of 15 minutes\n"
            "and it must become :term:`active` by 20:00 and the run time must not exceed 2 hours"
            ;
}

const char* NodeAttrDoc::autocancel_doc()
{
   return
            "Provides a way to automatically delete/remove a node which has completed\n\n"
            "See :term:`autocancel`\n"
            "\nConstructor::\n\n"
            "   Autocancel(TimeSlot,relative)\n"
            "      TimeSlot single: A time\n"
            "      bool relative:   Relative to completion. False means delete the node at the real time specified.\n\n"
            "   Autocancel(hour,minute,relative)\n"
            "      int hour:        hour in 24 hrs\n"
            "      int minute:      minute <= 59\n"
            "      bool relative:   Relative to completion. False means delete the node at the real time specified.\n\n"
            "   Autocancel(days)\n"
            "      int days:        Delete the node 'days' after completion\n"
            "\nUsage::\n\n"
            "   attr = Autocancel( 1,30, true )              # delete node 1 hour and 30 minutes after completion\n"
            "   attr = Autocancel( TimeSlot(0,10), true )    # delete node 10 minutes after completion\n"
            "   attr = Autocancel( TimeSlot(10,10), false )  # delete node at 10:10 after completion\n"
            "   attr = Autocancel( 3  )                      # delete node 3 days after completion\n"
            ;
}

const char* NodeAttrDoc::repeat_doc()
{
   return
            "Represents one of RepeatString,RepeatEnumerated,RepeatInteger,RepeatDate,RepeatDay\n\n"
            ;
}

const char* NodeAttrDoc::repeat_date_doc()
{
   return
            "Allows a :term:`node` to be repeated using a yyyymmdd format\n\n"
            "A node can only have one :term:`repeat`.\n"
            "The repeat can be referenced in :term:`trigger` expressions.\n"
            "\nConstructor::\n\n"
            "   RepeatDate(variable,start,end,delta)\n"
            "      string variable:     The name of the repeat. The current date can referenced in\n"
            "                           in trigger expressions using the variable name\n"
            "      int start:           Start date, must have format: yyyymmdd\n"
            "      int end:             End date, must have format: yyyymmdd\n"
            "      int delta<optional>: default = 1, Always in days. The increment used to update the date\n"
            "\nException:\n\n"
            "- Throws a RuntimeError if start/end are not valid dates\n"
            "\nUsage::\n\n"
            "   rep = RepeatDate(\"YMD\", 20050130, 20050203 )\n"
            "   rep = RepeatDate(\"YMD\", 20050130, 20050203, 2 )\n"
            ;
}

const char* NodeAttrDoc::repeat_integer_doc()
{
   return
            "Allows a :term:`node` to be repeated using a integer range.\n\n"
            "A node can only have one :term:`repeat`.\n"
            "The repeat can be referenced in :term:`trigger` expressions.\n"
            "\nConstructor::\n\n"
            "   RepeatInteger(variable,start,end,step)\n"
            "      string variable:     The name of the repeat. The current integer value can be\n"
            "                           referenced in trigger expressions using the variable name\n"
            "      int start:           Start integer value\n"
            "      int end:             End end integer value\n"
            "      int step<optional>:  Default = 1, The step amount\n"
            "\nUsage::\n\n"
            "   rep = RepeatInteger(\"HOUR\", 6, 24, 6 )\n"
            ;
}

const char* NodeAttrDoc::repeat_enumerated_doc()
{
   return
            "Allows a node to be repeated using a enumerated list.\n\n"
            "A :term:`node` can only have one :term:`repeat`.\n"
            "The repeat can be referenced in :term:`trigger` expressions.\n"
            "\nConstructor::\n\n"
            "   RepeatEnumerated(variable,list)\n"
            "      string variable:     The name of the repeat. The current enumeration index can be\n"
            "                           referenced in trigger expressions using the variable name\n"
            "      vector list:         The list of enumerations\n"
            "\nUsage::\n\n"
            "   rep = RepeatEnumerated(\"COLOR\", [ 'red', 'green', 'blue' ] )\n"
            ;
}

const char* NodeAttrDoc::repeat_string_doc()
{
   return
            "Allows a :term:`node` to be repeated using a string list.\n\n"
            "A :term:`node` can only have one :term:`repeat`.\n"
            "The repeat can be referenced in :term:`trigger` expressions.\n"
            "\nConstructor::\n\n"
            "   RepeatString(variable,list)\n"
            "      string variable:     The name of the repeat. The current index of the string list can be\n"
            "                           referenced in trigger expressions using the variable name\n"
            "      vector list:         The list of enumerations\n"
            "\nUsage::\n\n"
            "   rep = RepeatString(\"COLOR\", [ 'red', 'green', 'blue' ] )\n"
            ;
}

const char* NodeAttrDoc::repeat_day_doc()
{
   return
            "A repeat that is infinite.\n\n"
            "A node can only have one :term:`repeat`.\n"
            "\nConstructor::\n\n"
            "   RepeatDay(step)\n"
            "      int step:     The step.\n"
            "\nUsage::\n\n"
            "   rep = RepeatDay( 1 )\n"
            ;
}

const char* NodeAttrDoc::cron_doc()
{
   return
            ":term:`cron` defines a time dependency for a node.\n\n"
            "Crons are repeated indefinitely.\n\n"
            "Avoid having a cron and :term:`repeat` at the same level,\n"
            "as both provide looping functionality\n"
            "\nConstructor::\n\n"
            "   Cron()\n"
            "\nExceptions:\n\n"
            "- raises IndexError when an invalid cron is specified\n"
            "\nUsage::\n\n"
            "    cron = ecflow.Cron()\n"
            "    cron.set_week_days([0, 1, 2, 3, 4, 5, 6])\n"
            "    cron.set_days_of_month([1, 2, 3, 4, 5, 6 ])\n"
            "    cron.set_months([1, 2, 3, 4, 5, 6])\n"
            "    start = ecflow.TimeSlot(0 , 0)\n"
            "    finish = ecflow.TimeSlot(23, 0)\n"
            "    incr = ecflow.TimeSlot(0, 30)\n"
            "    ts = ecflow.TimeSeries(start, finish, incr, True)  # True means relative to suite start\n"
            "    cron.set_time_series(ts)\n"
            "\n"
            "    cron1 = ecflow.Cron()\n"
            "    cron1.set_time_series(1, 30, True)  # same as cron +01:30\n"
            "\n"
            "    cron2 = ecflow.Cron()\n"
            "    cron2.set_week_days([0, 1, 2, 3, 4, 5, 6])\n"
            "    cron2.set_time_series(\"00:30 01:30 00:01\")\n"
            "\n"
            "    cron3 = ecflow.Cron()\n"
            "    cron3.set_week_days([0, 1, 2, 3, 4, 5, 6])\n"
            "    cron3.set_time_series(\"+00:30\")\n"
            ;
}

const char* NodeAttrDoc::clock_doc()
{
   return
            "Specifies the :term:`clock` type used by the :term:`suite`.\n\n"
            "Only suites can have a :term:`clock`.\n"
            "A gain can be specified to offset from the given date.\n"
            "\nConstructor::\n\n"
            "   Clock(day,month,year,hybrid)\n"
            "      int day              : Specifies the day of the month  1-31\n"
            "      int month            : Specifies the month 1-12\n"
            "      int year             : Specifies the year > 1400\n"
            "      bool hybrid<optional>: Default = False, true means hybrid, false means real\n"
            "                             by default the clock is not real\n\n"
            "      Time will be set to midnight, use set_gain() to alter\n"
            "\n"
            "   Clock(hybrid)\n"
            "      bool hybrid: true means hybrid, false means real\n"
            "                   by default the clock is real\n"
            "      Time will be set real time of the computer\n"
            "\n"
            "\nExceptions:\n\n"
            "- raises IndexError when an invalid Clock is specified\n"
            "\nUsage::\n\n"
            "   suite = Suite('s1')\n"
            "   clock = Clock(1,1,2012,False)\n"
            "   clock.set_gain(1,10,True)\n"
            "   suite.add_clock(clock)\n"
            ;
}
