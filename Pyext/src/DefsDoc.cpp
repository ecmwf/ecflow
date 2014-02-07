/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #73 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : This class is used as a helper class
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include "DefsDoc.hpp"

const char* DefsDoc::abs_node_path_doc()
{
   return  "returns a string which holds the path to the node\n\n";
}

const char* DefsDoc::part_expression_doc()
{
   return
            "PartExpression holds part of a :term:`trigger` or :term:`complete expression`.\n\n"
            "Expressions can contain references to :term:`event`, :term:`meter` s, user variables,\n"
            ":term:`repeat` variables and generated variables. The part expression allows us\n"
            "to split a large trigger or complete expression into smaller ones\n"
            "\nConstructor::\n\n"
            "  PartExpression(exp )\n"
            "      string   exp: This represents the *first* expression\n\n"
            "  PartExpression(exp, bool and_expr)\n"
            "      string   exp: This represents the expression\n"
            "      bool and_exp: If true the expression is to be anded, with a previously added expression\n"
            "                    If false the expression is to be 'ored', with a previously added expression\n"
            "\nUsage:\n"
            "To add simple expression this class can be by-passed, i.e. can use::\n\n"
            "  task = Task('t1')\n"
            "  task.add_trigger( 't2 == active' )\n"
            "  task.add_complete( 't2 == complete' )\n\n"
            "To add large triggers and complete expression::\n\n"
            "  exp1 = PartExpression('t1 == complete')\n  # a simple expression can be added as a string\n"
            "  ....\n"
            "  task2.add_part_trigger( PartExpression(\"t1 == complete or t4 == complete\") ) \n"
            "  task2.add_part_trigger( PartExpression(\"t5 == active\",True) )    # anded with first expression\n"
            "  task2.add_part_trigger( PartExpression(\"t7 == active\",False) )   # or'ed with last expression added\n\n"
            "The trigger for task2 is equivalent to\n"
            "'t1 == complete or t4 == complete and t5 == active or t7 == active'"
            ;
}

const char* DefsDoc::expression_doc()
{
   return
            "Expression holds :term:`trigger` or :term:`complete expression`.\n\n"
            "Expressions can contain references to events, meters, user variables,repeat variables and generated variables.\n"
            "Expressions hold a list of part expressions. This allows us to split a large trigger or complete\n"
            "expression into smaller ones.\n"
            "\nConstructor::\n\n"
            "   Expression( expression )\n"
            "      string expression  : This typically represents the complete expression\n"
            "                           however part expression can still be added\n"
            "   Expression( part )\n"
            "      PartExpression part: The first part expression should have no 'and/or' set\n"
            "\nUsage:\n"
            "To add simple expression this class can be by passed, i.e. can use::\n\n"
            "  task = Task('t1')\n"
            "  task.add_trigger( 't2 == active' )\n"
            "  task.add_complete( 't2 == complete' )\n"
            "\n"
            "  task = Task('t2')\n"
            "  task.add_trigger( 't1 == active' )\n"
            "  task.add_part_trigger( 't3 == active', True)\n\n"
            "To store and add large expressions use a Expression with PartExpression::\n\n"
            "  big_expr = Expression( PartExpression(\"t1 == complete or t4 == complete\") )\n"
            "  big_expr.add( PartExpression(\"t5 == active\",True) )\n"
            "  big_expr.add( PartExpression(\"t7 == active\",False) )\n"
            "  task.add_trigger( big_expr)\n\n"
            "In the example above the trigger for task is equivalent to\n"
            "'t1 == complete or t4 == complete and t5 == active or t7 == active'\n\n"
            "::\n\n"
            "  big_expr2 = Expression('t0 == complete'))\n"
            "  big_expr2.add( PartExpression(\"t1 == complete or t4 == complete\",True) )\n"
            "  big_expr2.add( PartExpression(\"t5 == active\",False) )\n"
            "  task2.add_trigger( big_expr2)\n\n"
            "Here the trigger for task2 is equivalent to\n"
            "'t0 == complete and t1 == complete or t4 == complete or t5 == active'"
   ;
}

const char* DefsDoc::add_trigger_doc()
{
   return
            "Add a :term:`trigger` or :term:`complete expression`.\n\n"
            "This defines a dependency for a :term:`node`.\n"
            "There can only be one :term:`trigger` or :term:`complete expression` dependency per node.\n"
            "A :term:`node` with a trigger can only be activated when the trigger has expired.\n"
            "A trigger holds a node as long as the expression returns false.\n"
            "\nException:\n\n"
            "- Will throw RuntimeError if multiple trigger or complete expression are added\n"
            "- Will throw RuntimeError if first expression is added as 'AND' or 'OR' expression\n"
            "  Like wise second and subsequent expression must have 'AND' or 'OR' booleans set\n"
            "\nUsage:\n\n"
            "Note we can not make multiple add_trigger(..) calls on the same :term:`task`!\n"
            "to add a simple trigger::\n\n"
            "  task1.add_trigger( \"t2 == active\" )\n"
            "  task2.add_trigger( \"t1 == complete or t4 == complete\" )\n"
            "  task3.add_trigger( \"t5 == active\" )\n"
            "\n"
            "Long expression can be broken up using add_part_trigger::\n\n"
            "  task2.add_part_trigger( \"t1 == complete or t4 == complete\")\n"
            "  task2.add_part_trigger( \"t5 == active\",True)  # True means  AND\n"
            "  task2.add_part_trigger( \"t7 == active\",False) # False means OR\n\n"
            "The trigger for task2 is equivalent to:\n"
            "'t1 == complete or t4 == complete and t5 == active or t7 == active'"
            ;
}

const char* DefsDoc::add_variable_doc()
{
   return
            "Adds a name value :term:`variable`.\n\n"
            "This defines a variable for use in :term:`variable substitution` in a :term:`ecf script` file.\n"
            "There can be any number of variables. The variables are names inside a pair of\n"
            "'%' characters in an :term:`ecf script`. The name are case sensitive.\n"
            "Special character in the value, must be placed inside single quotes if misinterpretation\n"
            "is to be avoided.\n"
            "The value of the variable replaces the variable name in the :term:`ecf script` at :term:`job creation` time.\n"
            "The variable names for any given node must be unique. If duplicates are added then the\n"
            "the last value added is kept.\n"
            "\nException:\n\n"
            "- Writes warning to standard output, if a duplicate variable name is added\n"
            "\nUsage::\n\n"
            "  task.add_variable( Variable(\"ECF_HOME\",\"/tmp/\"))\n"
            "  task.add_variable( \"TMPDIR\",\"/tmp/\")\n"
            "  task.add_variable( \"COUNT\",2)\n"
            "  a_dict = { \"name\":\"value\", \"name2\":\"value2\", \"name3\":\"value3\" }\n"
            "  task.add_variable(a_dict)\n"
            ;
}

const char* DefsDoc::add_label_doc()
{
   return
            "Adds a :term:`label` to a :term:`node`.\n\n"
            "Labels can be updated from the jobs files, via :term:`child command`\n"
            "\nException:\n\n"
            "- Throws RuntimeError if a duplicate label name is added\n"
            "\nUsage::\n\n"
            "  task.add_label( Label(\"TEA\",\"/me/\"))\n"
            "  task.add_label( \"Joe\",\"/me/\")\n\n"
            "The corresponding child command in the .ecf script file might be::\n"
            "  ecflow_client --label TEA time\n"
            "  ecflow_client --label Joe ninety\n"
            ;
}

const char* DefsDoc::add_limit_doc()
{
   return
            "Adds a :term:`limit` to a :term:`node` for simple load management.\n\n"
            "Multiple limits can be added, however the limit name must be unique.\n"
            "For a node to be in a limit, a :term:`inlimit` must be used.\n"
            "\nException:\n\n"
            "- Throws RuntimeError if a duplicate limit name is added\n"
            "\nUsage::\n\n"
            "  family.add_limit( Limit(\"load\",12) )\n"
            "  family.add_limit( \"load\",12 )\n"
            ;
}

const char* DefsDoc::add_inlimit_doc()
{
   return
            "Adds a :term:`inlimit` to a :term:`node`.\n\n"
            "InLimit reference a :term:`limit`/:py:class:`ecflow.Limit`. Duplicate InLimits are not allowed\n"
            "\nException:\n\n"
            "- Throws RuntimeError if a duplicate is added\n"
            "\nUsage::\n\n"
            "  task2.add_inlimit( InLimit(\"limitName\",\"/s1/f1\",2) )\n"
            "  task2.add_inlimit( \"limitName\",\"/s1/f1\",2 )\n"
            ;
}

const char* DefsDoc::suite_doc()
{
   return
            "A :term:`suite` is a collection of Families,Tasks,Variables, :term:`repeat` and :term:`clock` definitions\n\n"
            "Suite is the only node that can be started using the begin API.\n"
            "There are two ways of adding a suite, see example below and :py:class:`ecflow.Defs.add_suite`\n"
            "\nConstructor::\n\n"
            "  Suite(name)\n"
            "      string name : The Suite name. name must consist of alpha numeric characters or\n"
            "                    underscore or dot. The first character can not be a dot, as this\n"
            "                    will interfere with trigger expressions. Case is significant\n"
            "\nException:\n\n"
            "- Throws a RuntimeError if the name is not valid\n"
            "- Throws a RuntimeError if duplicate suite names added\n"
            "\nUsage::\n\n"
            "  defs = Defs(\"new.def\")         # create a defs\n"
            "  suite = Suite(\"suite_1\")       # create a suite\n"
            "  defs.add_suite(suite)          # add suite to definition\n"
            "  suite2 = defs.add_suite(\"s2\")  # create a suite and add it to the defs\n"
            ;
}

const char* DefsDoc::family_doc()
{
   return
            "Create a :term:`family` :term:`node`.A Family node lives inside a :term:`suite` or another :term:`family`\n\n"
            "A family is used to collect :term:`task` s together or to group other families.\n"
            "Typically you place tasks that are related to each other inside the same family\n"
            "analogous to the way you create directories to contain related files.\n"
            "There are two ways of adding a family, see example below.\n"
            "\nConstructor::\n\n"
            "  Family(name)\n"
            "      string name : The Family name. name must consist of alpha numeric characters or\n"
            "                    underscore or dot. The first character can not be dot, as this\n"
            "                    will interfere with trigger expressions. Case is significant\n"
            "\nException:\n\n"
            "- Throws a RuntimeError if the name is not valid\n"
            "- Throws a RuntimeError if a duplicate family is added\n"
            "\nUsage::\n\n"
            "  suite = Suite(\"suite_1\")       # create a suite\n"
            "  family = Family(\"family_1\")    # create a family\n"
            "  suite.add_family(family)       # add created family to a suite\n"
            "  f2 = suite.add_family(\"f2\")    # create a family r2 and add to suite\n"
            ;
}

const char* DefsDoc::task_doc()
{
   return
            "Creates a :term:`task` :term:`node`.Task is a child of a :term:`suite` or :term:`family` node.\n\n"
            "Multiple Tasks can be added, however the task names must be unique for a given parent.\n"
            "Note case is significant. Only Tasks can be submitted. A job inside a Task :term:`ecf script` (i.e .ecf file)\n"
            "should generally be re-entrant since a Task may be automatically submitted more than once if it aborts.\n"
            "There are two ways of adding a task, see example below\n"
            "\nConstructor::\n\n"
            "   Task(name)\n"
            "      string name : The Task name.Name must consist of alpha numeric characters or\n"
            "                    underscore or dot. First character can not be a dot.\n"
            "                    Case is significant\n"
            "\nException:\n\n"
            "- Throws a RuntimeError if the name is not valid\n"
            "- Throws a RuntimeError if a duplicate Task is added\n"
            "\nUsage::\n\n"
            "  task = Task(\"t1\")            # create a task\n"
            "  family.add_task(task)        # add to the family\n"
            "  t2 = family.add_task(\"t2\")   # create a task t2 and add to the family\n\n"
            ;
}

const char* DefsDoc::alias_doc()
{
   return
            "A Aliases is create by the GUI or via edit_script command\n\n"
            "Aliases provide a mechanism to edit/test task scripts without effecting the suite\n"
            "The Aliases parent is always a Task.Multiple Alias can be added\n"
            ;
}

const char* DefsDoc::add_suite_doc()
{
   return
            "Add a :term:`suite` :term:`node`. See :py:class:`ecflow.Suite`\n\n"
            "Only one suite should be added for ease of maintenance. If a new suite is added\n"
            "which matches the name of an existing suite, then an exception is thrown.\n"
            "\nException:\n\n"
            "- Throws RuntimeError is the suite name is not valid\n"
            "- Throws RuntimeError if duplicate suite is added\n"
            "\nUsage::\n\n"
            "  Defs defs(\"file.def\"         # create a defs)\n"
            "  suite = Suite(\"suite\")       # create a Suite \n"
            "  defs.add_suite(suite)        # add suite to defs\n"
            "  s2 = defs.add_suite(\"s2\")    # create a suite and add to defs\n"
            ;
}

const char* DefsDoc::add_extern_doc()
{
   return
            ":term:`extern` refer to nodes that have not yet been defined typically due to cross suite :term:`dependencies`\n\n"
            ":term:`trigger` and :term:`complete expression` s may refer to paths, and variables in other suites, that have not been\n"
            "loaded yet. The references to node paths and variable must exist, or exist as externs\n"
            "Externs can be added manually or automatically.\n\n"
            "Manual Method::\n\n"
            "  void add_extern(string nodePath )\n"
            "\nUsage::\n\n"
            "  defs = Defs(\"file.def\")\n"
            "  ....\n"
            "  defs.add_extern('/temp/bill:event_name')\n"
            "  defs.add_extern('/temp/bill:meter_name')\n"
            "  defs.add_extern('/temp/bill:repeat_name')\n"
            "  defs.add_extern('/temp/bill:edit_name')\n"
            "  defs.add_extern('/temp/bill')\n"
            "\n"
            "Automatic Method:\n"
            "  This will scan all trigger and complete expressions, looking for paths and variables\n"
            "  that have not been defined. The added benefit of this approach is that duplicates will not\n"
            "  be added. It is the user's responsibility to check that extern's are eventually defined\n"
            "  otherwise trigger expression will not evaluate correctly\n\n"
            "::\n\n"
            "  void auto_add_externs(bool remove_existing_externs_first )\n"
            "\nUsage::\n\n"
            "  defs = Defs(\"file.def\")\n"
            "  ... populate the defs\n"
            "  ...\n"
            "  defs.auto_add_externs(True)   # remove existing extern first.\n"
            ;
}


const char* DefsDoc::node_doc()
{
   return
             "A Node class is the abstract base class for Suite, Family and Task\n\n"
             "Every Node instance has a name, and a path relative to a suite"
             ;
}

const char* DefsDoc::node_container_doc()
{
   return
             "NodeContainer is the abstract base class for a Suite and Family\n\n"
             "A NodeContainer can have Families and Tasks as children"
             ;
}

const char* DefsDoc::submittable_doc()
{
   return
             "Submittable is the abstract base class for a Task and Alias\n\n"
             "It provides a process id, password and try number"
             ;
}

const char* DefsDoc::add_family_doc()
{
   return
            "Add a :term:`family`. See :py:class:`ecflow.Family`.\n\n"
            "Multiple families can be added. However family names must be unique.\n"
            "for a given parent. Families can be hierarchical.\n"
            "\nException:\n\n"
            "- Throws RuntimeError if a duplicate is added\n"
            "\nUsage::\n\n"
            "  suite = Suite(\"suite\")          # create a suite\n"
            "  f1 = Family(\"f1\")               # create a family\n"
            "  suite.add_family(f1)            # add family to suite\n"
            "  f2 = suite.add_family(\"f2\")     # create a family and add to suite\n"
            ;
}

const char* DefsDoc::add_task_doc()
{
   return
            "Add a :term:`task`. See :py:class:`ecflow.Task`\n\n"
            "Multiple Tasks can be added. However Task names must be unique,\n"
            "for a given parent. Task can be added to Familiy's or Suites.\n"
            "\nException:\n\n"
            "- Throws RuntimeError if a duplicate is added\n"
            "\nUsage::\n\n"
            "  f1 = Family(\"f1\")        # create a family\n"
            "  t1 = Task(\"t1\")          # create a task\n"
            "  f1.add_task(t1)          # add task to family\n"
            "  t2 = f1.add_task(\"t2\")   # create task 't2' and add to family"
            ;
}


const char* DefsDoc::add_definition_doc()
{
   return
            "The Defs class holds the :term:`suite definition` structure.\n\n"
            "It contains all the suites and hence acts like the root for suite node tree hierarchy.\n"
            "The definition can be kept as python code, alternatively it can be saved as a flat\n"
            "ASCII definition file.\n"
            "If a definition is read in from disk, it will by default, check the :term:`trigger` expressions.\n"
            "If however the definition is created in python, then checking should be done explicitly.\n"
            "The Defs class take one argument which represents the file name\n\n"
            "Example::\n\n"
            "  defs = Defs()                      # create an empty defs\n"
            "  suite = defs.add_suite(\"s1\")\n"
            "  family = suite.add_family(\"f1\")\n"
            "  for i in [ \"_1\", \"_2\", \"_3\" ]: family.add_task( \"t\" + i )\n"
            "  defs.save_as_defs('filename.def')  # save defs into file\n"
            "\n"
            "Create a Defs from an existing file on disk.\n"
            "\n"
            "  defs = Defs('filename.def')   #  Will open and parse the file and create the Definition\n"
            "  print defs\n"
            ;
}

const char* DefsDoc::add_event_doc()
{
   return
            "Add a :term:`event`. See :py:class:`ecflow.Event`\n"
            "Events can be referenced in :term:`trigger` and :term:`complete expression` s\n\n"
            "\nException:\n\n"
            "- Throws RuntimeError if a duplicate is added\n"
            "\nUsage::\n\n"
            "  t1 = Task(\"t1\")\n"
            "  t1.add_event( Event(10) )\n"
            "  t1.add_event( Event(11,\"Eventname\") )\n"
            "  t1.add_event( 12 )\n"
            "  t1.add_event( 13, \"name\")\n"
            "  t1.add_event(\"flag\")\n\n"
            "To reference in a trigger::\n\n"
            "  t2 = Task(\"t2\")\n"
            "  t2.add_trigger('t1:flag == set')\n"
            ;
}

const char* DefsDoc::add_meter_doc()
{
   return
            "Add a :term:`meter`. See :py:class:`ecflow.Meter`\n"
            "Meters can be referenced in :term:`trigger` and :term:`complete expression` s\n\n"
            "\nException:\n\n"
            "- Throws RuntimeError if a duplicate is added\n"
            "\nUsage::\n\n"
            "  t1 = Task(\"t1\")\n"
            "  t1.add_meter( Meter(\"metername\",0,100,50) )\n"
            "  t1.add_meter( \"meter\",0,200)\n\n"
            "To reference in a trigger::\n\n"
            "  t2 = Task(\"t2\")\n"
            "  t2.add_trigger('t1:meter >= 10')\n"
            ;
}

const char* DefsDoc::add_date_doc()
{
   return
            "Add a :term:`date` time dependency\n\n"
            "A value of zero for day,month,year means every day, every month, every year\n"
            "\nException:\n\n"
            "- Throws RuntimeError if an invalid date is added\n"
            "\nUsage::\n\n"
            "  t1 = Task(\"t1\")\n"
            "  t1.add_date( Date(1,1,2010) ) # day,month,year\n"
            "  t1.add_date( 2,1,2010)        # day,month,year\n"
            "  t1.add_date( 1,0,0)           # day,month,year, the first of each month for every year\n"
            ;
}

const char* DefsDoc::add_day_doc()
{
   return
            "Add a :term:`day` time dependency\n\n"
            "\nUsage::\n\n"
            "  t1 = Task(\"t1\")\n"
            "  t1.add_day( Day(Days.sunday) ) \n"
            "  t1.add_day( Days.monday)\n"
            "  t1.add_day( \"tuesday\")\n"
            ;
}

const char* DefsDoc::add_today_doc()
{
   return
            "Add a :term:`today` time dependency\n\n"
            "\nUsage::\n\n"
            "  t1 = Task(\"t1\")\n"
            "  t1.add_today( \"00:30\" )\n"
            "  t1.add_today( \"+00:30\" )\n"
            "  t1.add_today( \"+00:30 20:00 01:00\" )\n"
            "  t1.add_today( Today( 0,10 ))      # hour,min,relative =false\n"
            "  t1.add_today( Today( 0,12,True )) # hour,min,relative\n"
            "  t1.add_today( Today(TimeSlot(20,20),False))\n"
            "  t1.add_today( 0,1 ))              # hour,min,relative=false\n"
            "  t1.add_today( 0,3,False ))        # hour,min,relative=false\n"
            "  start = TimeSlot(0,0)\n"
            "  finish = TimeSlot(23,0)\n"
            "  incr = TimeSlot(0,30)\n"
            "  ts = TimeSeries( start, finish, incr, True)\n"
            "  task2.add_today( Today(ts) )\n"
            ;
}

const char* DefsDoc::add_time_doc()
{
   return
            "Add a :term:`time` dependency\n\n"
            "\nUsage::\n\n"
            "  t1 = Task(\"t1\")\n"
            "  t1.add_time( \"00:30\" )\n"
            "  t1.add_time( \"+00:30\" )\n"
            "  t1.add_time( \"+00:30 20:00 01:00\" )\n"
            "  t1.add_time( Time( 0,10 ))      # hour,min,relative =false\n"
            "  t1.add_time( Time( 0,12,True )) # hour,min,relative\n"
            "  t1.add_time( Time(TimeSlot(20,20),False))\n"
            "  t1.add_time( 0,1 ))              # hour,min,relative=false\n"
            "  t1.add_time( 0,3,False ))        # hour,min,relative=false\n"
            "  start = TimeSlot(0,0)\n"
            "  finish = TimeSlot(23,0)\n"
            "  incr = TimeSlot(0,30)\n"
            "  ts = TimeSeries( start, finish, incr, True)\n"
            "  task2.add_time( Time(ts) )\n"
            ;
}

const char* DefsDoc::add_cron_doc()
{
   return
            "Add a :term:`cron` time dependency\n\n"
            "\nUsage::\n\n"
            "  start = TimeSlot(0,0)\n"
            "  finish = TimeSlot(23,0)\n"
            "  incr = TimeSlot(0,30)\n"
            "  time_series = TimeSeries( start, finish, incr, True)\n"
            "  cron = Cron()\n"
            "  cron.set_week_days( [0,1,2,3,4,5,6] )\n"
            "  cron.set_days_of_month( [1,2,3,4,5,6] )\n"
            "  cron.set_months( [1,2,3,4,5,6] )\n"
            "  cron.set_time_series( time_series )\n"
            "  t1 = Task(\"t1\")\n"
            "  t1.add_cron( cron )\n"
            ;
}

const char* DefsDoc::add_late_doc()
{
   return
            "Add a :term:`late` attribute\n\n"
            "\nException:\n\n"
            "- Throws a RuntimeError if more than one late is added\n"
            "\nUsage::\n\n"
            "   late = Late()\n"
            "   late.submitted( 20,10 )     # hour,minute\n"
            "   late.active(    20,10 )     # hour,minute\n"
            "   late.complete(  20,10,True) # hour,minute,relative\n"
            "   t1 = Task(\"t1\")\n"
            "   t1.add_late( late )\n"
            ;
}

const char* DefsDoc::add_autocancel_doc()
{
   return
            "Add a :term:`autocancel` attribute.\n\n"
            "This will delete the node on completion. The deletion may be delayed by\n"
            "an amount of time in hours and minutes or expressed as days\n"
            "Node deletion is not immediate. The nodes are checked once a minute\n"
            "and expired auto cancel nodes are deleted\n"
            "A node may only have one auto cancel attribute\n"
            "\nException:\n\n"
            "- Throws a RuntimeError if more than one auto cancel is added\n"
            "\nUsage::\n\n"
            "  t1 = Task('t1')\n"
            "  t1.add_autocancel( Autocancel(20,10,False) )  # hour,min, relative\n"
            "  t2 = Task('t2')\n"
            "  t2.add_autocancel( 3 )                        # 3 days \n"
            "  t3 = Task('t3')\n"
            "  t3.add_autocancel( 20,10,True )               # hour,minutes,relative \n"
            "  t4 = Task('t4')\n"
            "  t4.add_autocancel( TimeSlot(20,10),True )     # hour,minutes,relative \n"
            ;
}

const char* DefsDoc::add_verify_doc()
{
   return
            "Add a Verify attribute.\n\n"
            "For DEBUG/test used to assert that a particular state was reached."
            ;
}

const char* DefsDoc::add_repeat_date_doc()
{
   return
            "Add a RepeatDate attribute.\n\n"
            "A node can only have one repeat\n"
            "\nException:\n\n"
            "- Throws a RuntimeError if more than one repeat is added\n"
            "\nUsage::\n\n"
            "  t1 = Task('t1')\n"
            "  t1.add_repeat( RepeatDate(\"testDate\",20100111,20100115) )\n"
            ;
}

const char* DefsDoc::add_repeat_integer_doc()
{
   return
            "Add a RepeatInteger attribute.\n\n"
            "A node can only have one :term:`repeat`\n"
            "\nException:\n\n"
            "- Throws a RuntimeError if more than one repeat is added\n"
            "\nUsage::\n\n"
            "  t1 = Task('t1')\n"
            "  t1.add_repeat( RepeatInteger(\"testInteger\",0,100,2) )\n"
            ;
}

const char* DefsDoc::add_repeat_string_doc()
{
   return
            "Add a RepeatString attribute.\n\n"
            "A node can only have one :term:`repeat`\n"
            "\nException:\n\n"
            "- Throws a RuntimeError if more than one repeat is added\n"
            "\nUsage::\n\n"
            "  t1 = Task('t1')\n"
            "  t1.add_repeat( RepeatString(\"test_string\",['a', 'b', 'c' ] ) )\n"
            ;
}

const char* DefsDoc::add_repeat_enumerated_doc()
{
   return
            "Add a RepeatEnumerated attribute.\n\n"
            "A node can only have one :term:`repeat`\n"
            "\nException:\n\n"
            "- Throws a RuntimeError if more than one repeat is added\n"
            "\nUsage::\n\n"
            "  t1 = Task('t1')\n"
            "  t1.add_repeat( RepeatEnumerated(\"test_string\", ['red', 'green', 'blue' ] ) )\n"
            ;
}

const char* DefsDoc::add_repeat_day_doc()
{
   return
            "Add a RepeatDay attribute.\n\n"
            "A node can only have one :term:`repeat`\n"
            "\nException:\n\n"
            "- Throws a RuntimeError if more than one repeat is added\n"
            ;
}

const char* DefsDoc::add_defstatus_doc()
{
   return
            "Set the default status( :term:`defstatus` ) of node at begin or re queue\n\n"
            "A :term:`defstatus` is useful in preventing suites from running automatically\n"
            "once begun, or in setting Task's complete so they can be run selectively\n"
            "\nUsage::\n\n"
            "  t1 = Task('t1')\n"
            "  t1.add_defstatus( DState.suspended )\n"
            ;
}

const char* DefsDoc::jobgenctrl_doc()
{
   return
            "The class JobCreationCtrl is used in :term:`job creation` checking\n\n"
            "Constructor::\n\n"
            "   JobCreationCtrl()\n\n"
            "\nUsage::\n\n"
            "   defs = Defs('my.def')                     # specify the definition we want to check, load into memory\n"
            "   job_ctrl = JobCreationCtrl()\n"
            "   job_ctrl.set_node_path('/suite/to_check') # will hierarchically check job creation under this node\n"
            "   defs.check_job_creation(job_ctrl)         # job files generated to ECF_JOB\n"
            "   print job_ctrl.get_error_msg()            # report any errors in job generation\n"
            "\n"
            "   job_ctrl = JobCreationCtrl()              # no set_node_path() hence check job creation for all tasks\n"
            "   job_ctrl.set_dir_for_job_creation(tmp)    # generate jobs file under this directory\n"
            "   defs.check_job_creation(job_ctrl)\n"
            "   print job_ctrl.get_error_msg()\n"
            "\n"
            "   job_ctrl = JobCreationCtrl()              # no set_node_path() hence check job creation for all tasks\n"
            "   job_ctrl.generate_temp_dir()              # automatically generate directory for job file\n"
            "   defs.check_job_creation(job_ctrl)\n"
            "   print job_ctrl.get_error_msg()\n"
            ;
}

const char* DefsDoc::check_job_creation_doc()
{
   return
            "Check :term:`job creation` .\n\n"
            "Will check the following:\n\n"
            "- :term:`ecf script` files and includes files can be located\n"
            "- recursive includes\n"
            "- manual and comments :term:`pre-processing`\n"
            "- :term:`variable substitution`\n\n"
            "Some :term:`task` s are dummy tasks have no associated :term:`ecf script` file.\n"
            "To disable error message for these tasks please add a variable called ECF_DUMMY_TASK to them.\n"
            "Checking is done in conjunction with the class :py:class:`ecflow.JobCreationCtrl`.\n"
            "If no node path is set on class JobCreationCtrl then all tasks are checked.\n"
            "In the case where we want to check all tasks, use the convenience function that take no arguments.\n"
            "\nUsage::\n\n"
            "   defs = Defs('my.def')                     # specify the defs we want to check, load into memory\n"
            "   ...\n"
            "   print defs.check_job_creation()           # Check job generation for all tasks\n"
            "   ...\n"
            "   job_ctrl = JobCreationCtrl()\n"
            "   defs.check_job_creation(job_ctrl)         # Check job generation for all tasks, same as above\n"
            "   print job_ctrl.get_error_msg()\n"
            "   ...\n"
            "   job_ctrl = JobCreationCtrl()\n"
            "   job_ctrl.set_node_path('/suite/to_check') # will hierarchically check job creation under this node\n"
            "   defs.check_job_creation(job_ctrl)         # job files generated to ECF_JOB\n"
            "   print job_ctrl.get_error_msg()\n"
            "   ...\n"
            "   job_ctrl = JobCreationCtrl()              # no set_node_path() hence check job creation for all tasks\n"
            "   job_ctrl.set_dir_for_job_creation(tmp)    # generate jobs file under this directory\n"
            "   defs.check_job_creation(job_ctrl)\n"
            "   print job_ctrl.get_error_msg()\n"
            "   ...\n"
            "   job_ctrl = JobCreationCtrl()              # no set_node_path() hence check job creation for all tasks\n"
            "   job_ctrl.generate_temp_dir()              # automatically generate directory for job file\n"
            "   defs.check_job_creation(job_ctrl)\n"
            "   print job_ctrl.get_error_msg()\n"
            ;
}

const char* DefsDoc::generate_scripts_doc()
{
   return
            "Automatically generate template :term:`ecf script` s for this definition\n"
            "Will automatically add :term:`child command` s for :term:`event`, :term:`meter` and :term:`label` s.\n"
            "This allows the definition to be refined with out worrying about the scripts.\n"
            "However it should be noted that, this will create a lot of *duplicated* script contents\n"
            "i.e in the absence of :term:`event` s, :term:`meter` s and :term:`label` s, most of generated :term:`ecf script` files will\n"
            "be the same. Hence should only be used an aid to debugging the definition.\n"
            "It uses the contents of the definition to parameterise what gets\n"
            "generated, and the location of the files. Will throw Exceptions for errors.\n"
            "\nRequires:\n\n"
            "- ECF_HOME: specified and accessible for all Tasks, otherwise RuntimeError is raised\n"
            "- ECF_INCLUDE: specifies location for head.h and tail.h includes, will use angle brackets,\n"
            "               i.e %include <head.h>, if the head.h and tail.h already exist they are used otherwise\n"
            "               they are generated\n"
            "\nOptional:\n\n"
            "- ECF_FILES: If specified, then scripts are generated under this directory otherwise ECF_HOME is used.\n"
            "             The missing directories are automatically created.\n"
            "- ECF_CLIENT_EXE_PATH: if specified child command will use this, otherwise will use ecflow_client\n"
            "                       and assume this accessible on the path.\n"
            "- ECF_DUMMY_TASK: Will not generated scripts for this task.\n"
            "- SLEEP: Uses this variable to delay time between calls to child commands, if not specified uses delay of one second\n\n"
            "\nUsage::\n\n"
            "   defs = ecflow.Defs()\n"
            "   suite = defs.add_suite('s1')\n"
            "   suite.add_variable(\"ECF_HOME\",\"/user/var/home\")\n"
            "   suite.add_variable(\"ECF_INCLUDE\",\"/user/var/home/includes\")\n"
            "   for i in range(1,7) :\n"
            "      fam = suite.add_family(\"f\" + str(i))\n"
            "      for t in ( \"a\", \"b\", \"c\", \"d\", \"e\" ) :\n"
            "        fam.add_task(t);\n"
            "   defs.generate_scripts()   # generate '.ecf' and head.h/tail.h if required\n"
            ;
}

const char* DefsDoc::check()
{
   return
            "Check :term:`trigger` and :term:`complete expression` s and :term:`limit` s\n\n"
            "* Client Side: The client side can specify externs. Hence all node path references\n"
            "  in :term:`trigger` expressions, and :term:`inlimit` references to :term:`limit` s, that are\n"
            "  unresolved and which do *not* appear in :term:`extern` s are reported as errors\n"
            "* Server Side: The server does not store externs. Hence all unresolved references\n"
            "  are reported as errors\n\n"
            "Returns a non empty string for any errors or warning\n"
            "\nUsage::\n\n"
            "   # Client side\n"
            "   defs = Defs('my.def')        # Load my.def from disk\n"
            "   ....\n"
            "   print defs.check() # do the check\n"
            "\n"
            "   # Server Side\n"
            "   try:\n"
            "       ci = Client()             # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       print ci.check('/suite')\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
            ;
}

const char* DefsDoc::simulate()
{
   return
            "Simulates a :term:`suite definition` for 1 year:\n\n"
            "Will disable infinite repeats at the suite level. i.e repeat day\n"
            "This allows simulation to progress faster.\n"
            "By default will run simulation for a year. If the simulation does not complete\n"
            "creates  .flat and .depth files. This provides clues as to the state of the definition\n"
            "at the end of the simulation\n"
            "\nUsage::\n\n"
            "   defs = Defs('my.def')        # specify the defs we want to simulate\n"
            "   ....\n"
            "   theResults = defs.simulate()\n"
            "   print theResults\n"
            ;
}


const char* DefsDoc::get_server_state()
{
   return
            "Returns the :term:`ecflow_server` state: See :term:`server states`\n\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()           # use default host(ECF_NODE) & port(ECF_PORT)\n"
            "       ci.shutdown_server()\n"
            "       ci.sync_local()\n"
            "       assert ci.get_defs().get_server_state() == SState.SHUTDOWN, \"Expected server to be shutdown\"\n"
            "   except RuntimeError, e:\n"
            "       print str(e)\n"
             ;
}
