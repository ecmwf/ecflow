/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #73 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : This class is used as a helper class
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include "DefsDoc.hpp"

const char* DefsDoc::add()
{
   return  "add(..) provides a way to append Nodes and attributes\n\n"
           "This is best illustrated with an example::\n\n"
           " defs = Defs().add(\n"
           "     Suite('s1').add(\n"
           "         Clock(1, 1, 2010, False),\n"
           "         Autocancel(1, 10, True),\n"
           "         Task('t1').add(\n"
           "             Edit({'a':'12', 'b':'bb'}, c='v',d='b'),\n"
           "             Edit(g='d'),\n"
           "             Edit(h=1),\n"
           "             Event(1),\n"
           "             Event(11,'event'),\n"
           "             Meter('meter',0,10,10),\n"
           "             Label('label','c'),\n"
           "             Trigger('1==1'),\n"
           "             Complete('1==1'),\n"
           "             Limit('limit',10),Limit('limit2',10),\n"
           "             InLimit('limitName','/limit',2),\n"
           "             Defstatus(DState.complete),\n"
           "             Today(0,30),Today('00:59'),Today('00:00 11:30 00:01'),\n"
           "             Time(0,30),Time('00:59'),Time('00:00 11:30 00:01'),\n"
           "             Day('sunday'),Day(Days.monday),\n"
           "             Date(1,1,0),Date(28,2,1960),\n"
           "             Autocancel(3)\n"
           "             ),\n"
           "         [ Family('f{}'.format(i)) for i in range(1,6)]))\n\n"
           "We can also use '+=' with a list here are a few examples::\n\n"
           " defs = Defs();\n"
           " defs += [ Suite('s2'),Edit({ 'x1':'y', 'aa1':'bb'}, a='v',b='b') ]\n\n"
           "::\n\n"
           " defs += [ Suite('s{}'.format(i)) for i in range(1,6) ]\n\n"
           "::\n\n"
           " defs = Defs()\n"
           " defs += [ Suite('suite').add(\n"
           "              Task('x'),\n"
           "              Family('f').add( [ Task('t{}'.format(i)) for i in range(1,6)] ),\n"
           "              Task('y'),\n"
           "              [ Family('f{}'.format(i)) for i in range(1,6) ],\n"
           "              Edit(a='b'),\n"
           "              [ Task('t{}'.format(i)) for i in range(1,6) ],\n"
           "              )]\n\n"
           "It is also possible to use '+'\n\n"
           "::\n\n"
           " defs = Defs() + Suite('s1')\n"
           " defs.s1 += Autocancel(1, 10, True)\n"
           " defs.s1 += Task('t1') + Edit({ 'e':1, 'f':'bb'}) +\\ \n"
           "            Event(1) + Event(11,'event') + Meter('meter',0,10,10) + Label('label','c') + Trigger('1==1') +\\ \n"
           "            Complete('1==1') + Limit('limit',10) + Limit('limit2',10) + InLimit('limitName','/limit',2) +\\ \n"
           "            Defstatus(DState.complete) + Today(0,30) + Today('00:59') + Today('00:00 11:30 00:01') +\\ \n"
           "            Time(0,30) + Time('00:59') + Time('00:00 11:30 00:01') + Day('sunday') + Day(Days.monday) +\\ \n"
           "            Date(1,1,0) + Date(28,2,1960) + Autocancel(3)\n\n"
           ".. warning:: We can only use '+' when the left most object is a node, i.e Task('t1') in this case"
          ;
}

const char* DefsDoc::abs_node_path_doc()
{
   return  "returns a string which holds the path to the node\n\n";
}

const char* DefsDoc::part_expression_doc()
{
   return
            "PartExpression holds part of a `trigger`_ or `complete expression`_.\n\n"
            "Expressions can contain references to `event`_, `meter`_ s, user variables,\n"
            "`repeat`_ variables and generated variables. The part expression allows us\n"
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
            "  task2.add_part_trigger( PartExpression('t1 == complete or t4 == complete') ) \n"
            "  task2.add_part_trigger( PartExpression('t5 == active',True) )    # anded with first expression\n"
            "  task2.add_part_trigger( PartExpression('t7 == active',False) )   # or'ed with last expression added\n\n"
            "The trigger for task2 is equivalent to\n"
            "'t1 == complete or t4 == complete and t5 == active or t7 == active'"
            ;
}

const char* DefsDoc::expression_doc()
{
   return
            "Expression holds `trigger`_ or `complete expression`_. Also see :py:class:`ecflow.Trigger`\n\n"
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
            "  big_expr = Expression( PartExpression('t1 == complete or t4 == complete') )\n"
            "  big_expr.add( PartExpression('t5 == active',True) )\n"
            "  big_expr.add( PartExpression('t7 == active',False) )\n"
            "  task.add_trigger( big_expr)\n\n"
            "In the example above the trigger for task is equivalent to\n"
            "'t1 == complete or t4 == complete and t5 == active or t7 == active'\n\n"
            "::\n\n"
            "  big_expr2 = Expression('t0 == complete'))\n"
            "  big_expr2.add( PartExpression('t1 == complete or t4 == complete',True) )\n"
            "  big_expr2.add( PartExpression('t5 == active',False) )\n"
            "  task2.add_trigger( big_expr2)\n\n"
            "Here the trigger for task2 is equivalent to\n"
            "'t0 == complete and t1 == complete or t4 == complete or t5 == active'"
   ;
}

const char* DefsDoc::add_trigger_doc()
{
   return
            "Add a `trigger`_ or `complete expression`_.Also see :py:class:`ecflow.Trigger`\n\n"
            "This defines a dependency for a `node`_.\n"
            "There can only be one `trigger`_ or `complete expression`_ dependency per node.\n"
            "A `node`_ with a trigger can only be activated when the trigger has expired.\n"
            "A trigger holds a node as long as the expression returns false.\n"
            "\nException:\n\n"
            "- Will throw RuntimeError if multiple trigger or complete expression are added\n"
            "- Will throw RuntimeError if first expression is added as 'AND' or 'OR' expression\n"
            "  Like wise second and subsequent expression must have 'AND' or 'OR' booleans set\n"
            "\nUsage:\n\n"
            "Note we can not make multiple add_trigger(..) calls on the same `task`_!\n"
            "to add a simple trigger::\n\n"
            "  task1.add_trigger( 't2 == active' )\n"
            "  task2.add_trigger( 't1 == complete or t4 == complete' )\n"
            "  task3.add_trigger( 't5 == active' )\n"
            "\n"
            "Long expression can be broken up using add_part_trigger::\n\n"
            "  task2.add_part_trigger( 't1 == complete or t4 == complete')\n"
            "  task2.add_part_trigger( 't5 == active',True)  # True means  AND\n"
            "  task2.add_part_trigger( 't7 == active',False) # False means OR\n\n"
            "The trigger for task2 is equivalent to:\n"
            "'t1 == complete or t4 == complete and t5 == active or t7 == active'"
            ;
}

const char* DefsDoc::trigger()
{
   return
            "Add a `trigger`_ or `complete expression`_.\n\n"
            "This defines a dependency for a `node`_.\n"
            "There can only be one `trigger`_ or `complete expression`_ dependency per node.\n"
            "A `node`_ with a trigger can only be activated when the trigger has expired.\n"
            "Triggers can reference nodes, events, meters, variables, repeats, limits and late flag\n"
            "A trigger holds a node as long as the expression returns false.\n"
            "\nException:\n\n"
            "- Will throw RuntimeError if first expression is added as 'AND' or 'OR' expression\n"
            "  Like wise second and subsequent expression must have 'AND' or 'OR' booleans set\n"
            "\nUsage:\n\n"
            "Multiple trigger will automatically be *anded* together, If *or* is required please\n"
            "use bool 'False' as the last argument i.e ::\n\n"
            "  task1.add( Trigger('t2 == active' ),\n"
            "             Trigger('t1 == complete or t4 == complete' ),\n"
            "             Trigger('t5 == active',False))\n"
            "\n"
            "The trigger for task1 is equivalent to ::\n\n"
            "  t2 == active and t1 == complete or t4 == complete or t5 == active\n\n"
            "Since a large number of triggers are of the form `<node> == complete` there are\n"
            "are short cuts, these involves a use of a list ::\n\n"
            "  task1.add( Trigger( ['t2','t3'] )) #  This is same as t2 == complete and t3 == complete\n\n"
            "You can also use a node ::\n\n"
            "  task1.add( Trigger( ['t2',taskx] ))\n\n"
            "If the node 'taskx' has a parent, we use the full hierarchy, hence we will get a trigger\n"
            "of the form ::\n\n"
            "  t2 ==complete and /suite/family/taskx == complete\n\n"
            "If however node taskx has not yet been added to its parent, we use a relative name in the trigger ::\n\n"
            "  t2 ==complete and taskx == complete\n"
            ;
}

const char* DefsDoc::add_variable_doc()
{
   return
            "Adds a name value `variable`_. Also see :py:class:`ecflow.Edit`\n\n"
            "This defines a variable for use in `variable substitution`_ in a `ecf script`_ file.\n"
            "There can be any number of variables. The variables are names inside a pair of\n"
            "'%' characters in an `ecf script`_. The name are case sensitive.\n"
            "Special character in the value, must be placed inside single quotes if misinterpretation\n"
            "is to be avoided.\n"
            "The value of the variable replaces the variable name in the `ecf script`_ at `job creation` time.\n"
            "The variable names for any given node must be unique. If duplicates are added then the\n"
            "the last value added is kept.\n"
            "\nException:\n\n"
            "- Writes warning to standard output, if a duplicate variable name is added\n"
            "\nUsage::\n\n"
            "  task.add_variable( Variable('ECF_HOME','/tmp/'))\n"
            "  task.add_variable( 'TMPDIR','/tmp/')\n"
            "  task.add_variable( 'COUNT',2)\n"
            "  a_dict = { 'name':'value', 'name2':'value2', 'name3':'value3' }\n"
            "  task.add_variable(a_dict)\n"
            ;
}

const char* DefsDoc::add_label_doc()
{
   return
            "Adds a `label`_ to a `node`_. See :py:class:`ecflow.Label`\n\n"
            "Labels can be updated from the jobs files, via `child command`_\n"
            "\nException:\n\n"
            "- Throws RuntimeError if a duplicate label name is added\n"
            "\nUsage::\n\n"
            "  task.add_label( Label('TEA','/me/'))\n"
            "  task.add_label( 'Joe','/me/')\n\n"
            "The corresponding child command in the .ecf script file might be::\n\n"
            "  ecflow_client --label=TEA time\n"
            "  ecflow_client --label=Joe ninety\n"
            ;
}

const char* DefsDoc::add_limit_doc()
{
   return
            "Adds a `limit`_ to a `node`_ for simple load management. See :py:class:`ecflow.Limit`\n\n"
            "Multiple limits can be added, however the limit name must be unique.\n"
            "For a node to be in a limit, a `inlimit`_ must be used.\n"
            "\nException:\n\n"
            "- Throws RuntimeError if a duplicate limit name is added\n"
            "\nUsage::\n\n"
            "  family.add_limit( Limit('load',12) )\n"
            "  family.add_limit( 'load',12 )\n"
            ;
}

const char* DefsDoc::add_inlimit_doc()
{
   return
            "Adds a `inlimit`_ to a `node`_. See :py:class:`ecflow.InLimit`\n\n"
            "InLimit reference a `limit`_/:py:class:`ecflow.Limit`. Duplicate InLimits are not allowed\n"
            "\nException:\n\n"
            "- Throws RuntimeError if a duplicate is added\n"
            "\nUsage::\n\n"
            "  task2.add_inlimit( InLimit('limitName','/s1/f1',2) )\n"
            "  task2.add_inlimit( 'limitName','/s1/f1',2 )\n"
            ;
}

const char* DefsDoc::suite_doc()
{
   return
            "A `suite`_ is a collection of Families,Tasks,Variables, `repeat`_ and `clock`_ definitions\n\n"
            "Suite is the only node that can be started using the begin API.\n"
            "There are several ways of adding a suite, see example below and :py:class:`ecflow.Defs.add_suite`\n"
            "\nConstructor::\n\n"
            "  Suite(name, Nodes | attributes)\n"
            "      string name : The Suite name. name must consist of alpha numeric characters or\n"
            "                    underscore or dot. The first character can not be a dot, as this\n"
            "                    will interfere with trigger expressions. Case is significant\n"
            "      Nodes | Attributes:(optional)\n"
            "\nException:\n\n"
            "- Throws a RuntimeError if the name is not valid\n"
            "- Throws a RuntimeError if duplicate suite names added\n"
            "\nUsage::\n\n"
            "  defs = Defs()                  # create a empty definition. Root of all Suites\n"
            "  suite = Suite('suite_1')       # create a stand alone suite\n"
            "  defs.add_suite(suite)          # add suite to definition\n"
            "  suite2 = defs.add_suite('s2')  # create a suite and add it to the defs\n\n"
            "  defs = Defs(\n"
            "           Suite('s1',\n"
            "              Family('f1',\n"
            "                 Task('t1'))))   # create in in-place\n"
            ;
}

const char* DefsDoc::family_doc()
{
   return
            "Create a `family`_ `node`_.A Family node lives inside a `suite`_ or another `family`_\n\n"
            "A family is used to collect `task`_ s together or to group other families.\n"
            "Typically you place tasks that are related to each other inside the same family\n"
            "analogous to the way you create directories to contain related files.\n"
            "There are two ways of adding a family, see example below.\n"
            "\nConstructor::\n\n"
            "  Family(name, Nodes | Attributes)\n"
            "      string name : The Family name. name must consist of alpha numeric characters or\n"
            "                    underscore or dot. The first character can not be dot, as this\n"
            "                    will interfere with trigger expressions. Case is significant\n"
            "      Nodes | Attributes: (optional)\n"
            "\nException:\n\n"
            "- Throws a RuntimeError if the name is not valid\n"
            "- Throws a RuntimeError if a duplicate family is added\n"
            "\nUsage::\n\n"
            "  suite = Suite('suite_1')       # create a suite\n"
            "  family = Family('family_1')    # create a family\n"
            "  suite.add_family(family)       # add created family to a suite\n"
            "  f2 = suite.add_family('f2')    # create a family f2 and add to suite\n\n"
            "  # create in place\n"
            "  defs = Defs(\n"
            "           Suite('s1',\n"
            "              Family('f1',\n"
            "                 Task('t1',\n"
            "                     Edit(SLEEP='10')))))\n"
            ;
}

const char* DefsDoc::task_doc()
{
   return
            "Creates a `task`_ `node`_. Task is a child of a :py:class:`ecflow.Suite` or :py:class:`ecflow.Family` node.\n\n"
            "Multiple Tasks can be added, however the task names must be unique for a given parent.\n"
            "Note case is significant. Only Tasks can be submitted. A job inside a Task `ecf script`_ (i.e .ecf file)\n"
            "should generally be re-entrant since a Task may be automatically submitted more than once if it aborts.\n"
            "There are serveral ways of adding a task, see examples below\n"
            "\nConstructor::\n\n"
            "  Task(name, Attributes)\n"
            "     string name : The Task name.Name must consist of alpha numeric characters or\n"
            "                   underscore or dot. First character can not be a dot.\n"
            "                   Case is significant\n"
            "     attributes: optional, i.e like Meter, Event, Trigger etc\n"
            "\nException:\n\n"
            "- Throws a RuntimeError if the name is not valid\n"
            "- Throws a RuntimeError if a duplicate Task is added\n"
            "\nUsage::\n\n"
            "  task = Task('t1')            # create a stand alone task\n"
            "  family.add_task(task)        # add to the family\n"
            "  t2 = family.add_task('t2')   # create a task t2 and add to the family\n\n"
            "  # Create Task in place\n"
            "  defs = Defs(\n"
            "           Suite('s1',\n"
            "              Family('f1',\n"
            "                 Task('t1',\n"
            "                    Trigger('1==1'),\n"
            "                    Edit(SLEEP='10'))))) # add Trigger and Variables in place\n"
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
            "Add a `suite`_ `node`_. See :py:class:`ecflow.Suite`\n\n"
            "If a new suite is added which matches the name of an existing suite, then an exception is thrown.\n"
            "\nException:\n\n"
            "- Throws RuntimeError is the suite name is not valid\n"
            "- Throws RuntimeError if duplicate suite is added\n"
            "\nUsage::\n\n"
            "  defs = Defs()                # create a empty defs\n"
            "  suite = Suite('suite')       # create a stand alone Suite \n"
            "  defs.add_suite(suite)        # add suite to defs\n"
            "  s2 = defs.add_suite('s2')    # create a suite and add to defs\n\n"
            "  # Alternatively we can create Suite in place\n"
            "  defs = Defs(\n"
            "           Suite('s1',\n"
            "              Family('f1',\n"
            "                 Task('t1'))),\n"
            "           Suite('s2',\n"
            "              Family('f1',\n"
            "                 Task('t1'))))\n"
            ;
}

const char* DefsDoc::add_extern_doc()
{
   return
            "`extern`_ refer to nodes that have not yet been defined typically due to cross suite `dependencies`_\n\n"
            "`trigger`_ and `complete expression`_ s may refer to paths, and variables in other suites, that have not been\n"
            "loaded yet. The references to node paths and variable must exist, or exist as externs\n"
            "Externs can be added manually or automatically.\n\n"
            "Manual Method::\n\n"
            "  void add_extern(string nodePath )\n"
            "\nUsage::\n\n"
            "  defs = Defs('file.def')\n"
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
            "  defs = Defs('file.def')\n"
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
            "Add a `family`_. See :py:class:`ecflow.Family`.\n\n"
            "Multiple families can be added. However family names must be unique.\n"
            "for a given parent. Families can be hierarchical.\n"
            "\nException:\n\n"
            "- Throws RuntimeError if a duplicate is added\n"
            "\nUsage::\n\n"
            "  suite = Suite('suite')          # create a suite\n"
            "  f1 = Family('f1')               # create a family\n"
            "  suite.add_family(f1)            # add family to suite\n"
            "  f2 = suite.add_family('f2')     # create a family and add to suite\n"
            ;
}

const char* DefsDoc::add_task_doc()
{
   return
            "Add a `task`_. See :py:class:`ecflow.Task`\n\n"
            "Multiple Tasks can be added. However Task names must be unique,\n"
            "for a given parent. Task can be added to Familiy's or Suites.\n"
            "\nException:\n\n"
            "- Throws RuntimeError if a duplicate is added\n"
            "\nUsage::\n\n"
            "  f1 = Family('f1')      # create a family\n"
            "  t1 = Task('t1')          # create a task\n"
            "  f1.add_task(t1)          # add task to family\n"
            "  t2 = f1.add_task('t2') # create task 't2' and add to family"
            ;
}


const char* DefsDoc::add_definition_doc()
{
   return
            "The Defs class holds the `suite definition`_ structure.\n\n"
            "It contains all the :py:class:`ecflow.Suite` and hence acts like the root for suite node tree hierarchy.\n"
            "The definition can be kept as python code, alternatively it can be saved as a flat\n"
            "ASCII definition file.\n"
            "If a definition is read in from disk, it will by default, check the `trigger`_ expressions.\n"
            "If however the definition is created in python, then checking should be done explicitly:\n\n"
            "   Defs(string)\n"
            "      string - The Defs class take one argument which represents the file name\n"
            "   Defs(Suite | Edit )\n"
            "      :py:class:`ecflow.Suite`- One or more suites\n\n"
            "      :py:class:`ecflow.Edit` - specifies user defined server variables\n"
            "\nExample::\n\n"
            "  # Build definition using Constructor approach, This allows indentation, to show the structure\n"
            "  # This is a made up example to demonstrate suite construction:\n"
            " defs = Defs(\n"
            "     Edit(SLEEP=10,FRED='bill'),  # user defined server variables\n"
            "     Suite('s1'\n"
            "         Clock(1, 1, 2010, False),\n"
            "         Autocancel(1, 10, True),\n"
            "         Task('t1'\n"
            "             Edit({'a':'12', 'b':'bb'}, c='v',d='b'),\n"
            "             Edit(g='d'),\n"
            "             Edit(h=1),\n"
            "             Event(1),\n"
            "             Event(11,'event'),\n"
            "             Meter('meter',0,10,10),\n"
            "             Label('label','c'),\n"
            "             Trigger('1==1'),\n"
            "             Complete('1==1'),\n"
            "             Limit('limit',10),Limit('limit2',10),\n"
            "             InLimit('limitName','/limit',2),\n"
            "             Defstatus(DState.complete),\n"
            "             Today(0,30),Today('00:59'),Today('00:00 11:30 00:01'),\n"
            "             Time(0,30),Time('00:59'),Time('00:00 11:30 00:01'),\n"
            "             Day('sunday'),Day(Days.monday),\n"
            "             Date(1,1,0),Date(28,2,1960),\n"
            "             Autocancel(3)\n"
            "             ),\n"
            "         [ Family('f{}'.format(i)) for i in range(1,6)]))\n\n"
            "  defs.save_as_defs('filename.def')  # save defs into file\n\n"
            "  defs = Defs()                      # create an empty defs\n"
            "  suite = defs.add_suite('s1')\n"
            "  family = suite.add_family('f1')\n"
            "  for i in [ '_1', '_2', '_3' ]: family.add_task( 't' + i )\n"
            "  defs.save_as_defs('filename.def')  # save defs into file\n"
            "\n"
            "Create a Defs from an existing file on disk. ::\n\n"
            "  defs = Defs('filename.def')   #  Will open and parse the file and create the Definition\n"
            "  print(defs)\n"
            ;
}

const char* DefsDoc::add_event_doc()
{
   return
            "Add a `event`_. See :py:class:`ecflow.Event`\n"
            "Events can be referenced in `trigger`_ and `complete expression`_ s\n\n"
            "\nException:\n\n"
            "- Throws RuntimeError if a duplicate is added\n"
            "\nUsage::\n\n"
            "  t1 = Task('t1',\n"
            "            Event(12),\n"
            "            Event(11,'eventx'))             # Create events on Task creation\n\n"
            "  t1.add_event( Event(10) )                 # Create with function on Task\n"
            "  t1.add_event( Event(11,'Eventname') )\n"
            "  t1.add_event( 12 )\n"
            "  t1.add_event( 13, 'name')\n\n"
            "To reference event 'flag' in a trigger::\n\n"
            "  t1.add_event('flag')\n"
            "  t2 = Task('t2',\n"
            "            Trigger('t1:flag == set'))"
            ;
}

const char* DefsDoc::add_meter_doc()
{
   return
            "Add a `meter`_. See :py:class:`ecflow.Meter`\n"
            "Meters can be referenced in `trigger`_ and `complete expression`_ s\n\n"
            "\nException:\n\n"
            "- Throws RuntimeError if a duplicate is added\n"
            "\nUsage::\n\n"
            "  t1 = Task('t1',\n"
            "            Meter('met',0,50))                   # create Meter on Task creation\n"
            "  t1.add_meter( Meter('metername',0,100,50) )  # create Meter using function\n"
            "  t1.add_meter( 'meter',0,200)\n\n"
            "To reference in a trigger::\n\n"
            "  t2 = Task('t2')\n"
            "  t2.add_trigger('t1:meter >= 10')\n"
            ;
}

const char* DefsDoc::add_date_doc()
{
   return
            "Add a `date`_ time dependency. See :py:class:`ecflow.Date`\n\n"
            "A value of zero for day,month,year means every day, every month, every year\n"
            "\nException:\n\n"
            "- Throws RuntimeError if an invalid date is added\n"
            "\nUsage::\n\n"
            "  t1 = Task('t1',\n"
            "            Date('1.*.*'),\n"
            "            Date(1,1,2010)))    # Create Date in place\n\n"
            "  t1.add_date( Date(1,1,2010) ) # day,month,year\n"
            "  t1.add_date( 2,1,2010)        # day,month,year\n"
            "  t1.add_date( 1,0,0)           # day,month,year, the first of each month for every year\n"
            ;
}

const char* DefsDoc::add_day_doc()
{
   return
            "Add a `day`_ time dependency. See :py:class:`ecflow.Day`\n\n"
            "\nUsage::\n\n"
            "  t1 = Task('t1',\n"
            "            Day('sunday'))  # Create Day on Task creation\n\n"
            "  t1.add_day( Day(Days.sunday) )\n"
            "  t1.add_day( Days.monday)\n"
            "  t1.add_day( 'tuesday' )\n"
            ;
}

const char* DefsDoc::add_today_doc()
{
   return
            "Add a `today`_ time dependency. See :py:class:`ecflow.Today`\n\n"
            "\nUsage::\n\n"
            "  t1 = Task('t1',\n"
            "            Today('+00:30 20:00 01:00')) # Create Today in Task constructor\n\n"
            "  t1.add_today( '00:30' )\n"
            "  t1.add_today( '+00:30' )\n"
            "  t1.add_today( '+00:30 20:00 01:00' )\n"
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
            "Add a `time`_ dependency. See :py:class:`ecflow.Time`\n\n"
            "\nUsage::\n\n"
            "  t1 = Task('t1', Time('+00:30 20:00 01:00')) # Create Time in Task constructor\n"
            "  t1.add_time( '00:30' )\n"
            "  t1.add_time( '+00:30' )\n"
            "  t1.add_time( '+00:30 20:00 01:00' )\n"
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
            "Add a `cron`_ time dependency. See :py:class:`ecflow.Cron`\n\n"
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
            "  t1 = Task('t1')\n"
            "  t1.add_cron( cron )\n\n"
            "  # we can also create a Cron in the Task constructor like any other attribute\n"
            "  t2 = Task('t2',\n"
            "            Cron('+00:00 23:00 00:30',days_of_week=[0,1,2,3,4,5,6],days_of_month=[1,2,3,4,5,6],months=[1,2,3,4,5,6]))\n"
            ;
}

const char* DefsDoc::add_late_doc()
{
   return
            "Add a `late`_ attribute. See :py:class:`ecflow.Late`\n\n"
            "\nException:\n\n"
            "- Throws a RuntimeError if more than one late is added\n"
            "\nUsage::\n\n"
            "  late = Late()\n"
            "  late.submitted( 20,10 )     # hour,minute\n"
            "  late.active(    20,10 )     # hour,minute\n"
            "  late.complete(  20,10,True) # hour,minute,relative\n"
            "  t1 = Task('t1')\n"
            "  t1.add_late( late )\n\n"
            "  # we can also create a Late in the Task constructor like any other attribute\n"
            "  t2 = Task('t2',\n"
            "            Late(submitted='20:10',active='20:10',complete='+20:10'))\n"
           ;
}

const char* DefsDoc::add_autocancel_doc()
{
   return
            "Add a `autocancel` attribute. See :py:class:`ecflow.Autocancel`\n\n"
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
            "  t3.add_autocancel( 20,10,True )               # hour,minutes,relative\n"
            "  t4 = Task('t4')\n"
            "  t4.add_autocancel( TimeSlot(20,10),True )     # hour,minutes,relative\n\n"
            "  # we can also create a Autocancel in the Task constructor like any other attribute\n"
            "  t2 = Task('t2',\n"
            "            Autocancel(20,10,False))\n"
            ;
}

const char* DefsDoc::add_autoarchive_doc()
{
   return
            "Add a `autoarchive` attribute. See :py:class:`ecflow.Autoarchive`\n\n"
            "Provides a way to automatically archive a suite/family which has completed.(i.e remove children)\n"
            "This is required when dealing with super large suite/families, they can be archived off, and then restored later.\n"
            "The node can be recovered using 'autorestore',begin,re-queue and manually via ecflow_client --restore.\n"
            "The archived node is written to disk, as ECF_HOME/<host>.<port>.ECF_NAME.check,\n"
            "where '/' is replaced with ':' in ECF_NAME.\n"
            "The removal may be delayed by an amount of time in hours and minutes or expressed as days\n"
            "Node removal is not immediate. The nodes are checked once a minute\n"
            "A Node may only have one autoarchive attribute\n"
            "\nException:\n\n"
            "- Throws a RuntimeError if more than one auto archive is added\n"
            "\nUsage::\n\n"
            "  t1 = Task('t1')\n"
            "  t1.add_autoarchive( Autoarchive(20,10,False) )  # hour,min, relative\n"
            "  t2 = Task('t2')\n"
            "  t2.add_autoarchive( 3 )                        # 3 days \n"
            "  t3 = Task('t3')\n"
            "  t3.add_autoarchive( 20,10,True )               # hour,minutes,relative\n"
            "  t4 = Task('t4')\n"
            "  t4.add_autoarchive( TimeSlot(20,10),True )     # hour,minutes,relative\n\n"
            "  # we can also create a Autoarchive in the Task constructor like any other attribute\n"
            "  t2 = Task('t2',\n"
            "            Autoarchive(20,10,False))\n"
            ;
}

const char* DefsDoc::add_autorestore_doc()
{
   return
            "Add a `autorestore` attribute. See :py:class:`ecflow.Autorestore`\n\n"
            "Auto-restore is used to automatically restore a previously auto-archived node.\n"
            "The restore will fail if:\n"
            " - The node has not been archived\n"
            " - The node has children.\n"
            " - The file ECF_HOME/<host>.<port>.ECF_NAME.check does not exist\n"
            "\nException:\n\n"
            "- Throws a RuntimeError if more than one autorestore is added\n"
            "\nUsage::\n\n"
            "  t1 = Task('t1')\n"
            "  t1.add_autorestore( ['/s1/f1'] )   \n"
            "  t2 = Task('t2')\n"
            "  t2.add_autorestore( Autorestore(['/s2/f1','/s1/f2']) )  \n"
            "  # we can also create a Autorestore in the Task constructor like any other attribute\n"
            "  t2 = Task('t2', Autorestore(['/s2/f1','/s1/f2'] ))\n"
            ;
}

const char* DefsDoc::add_verify_doc()
{
   return
            "Add a Verify attribute.\n\n"
            "Used in python simulation used to assert that a particular state was reached."
            "  t2 = Task('t2',\n"
            "             Verify(State.complete, 6)) # verify task completes 6 times during simulation\n"
            ;
}

const char* DefsDoc::add_repeat_date_doc()
{
   return
            "Add a RepeatDate attribute. See :py:class:`ecflow.RepeatDate`\n\n"
            "A node can only have one repeat\n"
            "\nException:\n\n"
            "- Throws a RuntimeError if more than one repeat is added\n"
            "\nUsage::\n\n"
            "  t1 = Task('t1')\n"
            "  t1.add_repeat( RepeatDate('testDate',20100111,20100115) )\n\n"
            "  # we can also create a repeat in Task constructor like any other attribute\n"
            "  t2 = Task('t2',\n"
            "            RepeatDate('testDate',20100111,20100115))\n"
            ;
}

const char* DefsDoc::add_repeat_integer_doc()
{
   return
            "Add a RepeatInteger attribute. See :py:class:`ecflow.RepeatInteger`\n\n"
            "A node can only have one `repeat`_\n"
            "\nException:\n\n"
            "- Throws a RuntimeError if more than one repeat is added\n"
            "\nUsage::\n\n"
            "  t1 = Task('t1')\n"
            "  t1.add_repeat( RepeatInteger('testInteger',0,100,2) )\n\n"
            "  # we can also create a repeat in Task constructor like any other attribute\n"
            "  t2 = Task('t2',\n"
            "            RepeatInteger('testInteger',0,100,2))\n"
            ;
}

const char* DefsDoc::add_repeat_string_doc()
{
   return
            "Add a RepeatString attribute. See :py:class:`ecflow.RepeatString`\n\n"
            "A node can only have one `repeat`_\n"
            "\nException:\n\n"
            "- Throws a RuntimeError if more than one repeat is added\n"
            "\nUsage::\n\n"
            "  t1 = Task('t1')\n"
            "  t1.add_repeat( RepeatString('test_string',['a', 'b', 'c' ] ) )\n\n"
            "  # we can also create a repeat in Task constructor like any other attribute\n"
            "  t2 = Task('t2',\n"
            "            RepeatString('test_string',['a', 'b', 'c' ] ) )\n"
            ;
}

const char* DefsDoc::add_repeat_enumerated_doc()
{
   return
            "Add a RepeatEnumerated attribute. See :py:class:`ecflow.RepeatEnumerated`\n\n"
            "A node can only have one `repeat`_\n"
            "\nException:\n\n"
            "- Throws a RuntimeError if more than one repeat is added\n"
            "\nUsage::\n\n"
            "  t1 = Task('t1')\n"
            "  t1.add_repeat( RepeatEnumerated('test_string', ['red', 'green', 'blue' ] ) )\n\n"
            "  # we can also create a repeat in Task constructor like any other attribute\n"
            "  t2 = Task('t2',\n"
            "            RepeatEnumerated('test_string', ['red', 'green', 'blue' ] ) )\n"
            ;
}

const char* DefsDoc::add_repeat_day_doc()
{
   return
            "Add a RepeatDay attribute. See :py:class:`ecflow.RepeatDay`\n\n"
            "A node can only have one `repeat`_\n"
            "\nException:\n\n"
            "- Throws a RuntimeError if more than one repeat is added\n"
            "\nUsage::\n\n"
            "  t2 = Task('t2',\n"
            "            RepeatDay(1))\n"
            ;
}

const char* DefsDoc::add_defstatus_doc()
{
   return
            "Set the default status( `defstatus`_ ) of node at begin or re queue. See :py:class:`ecflow.Defstatus`\n\n"
            "A `defstatus`_ is useful in preventing suites from running automatically\n"
            "once begun, or in setting Task's complete so they can be run selectively\n"
            "\nUsage::\n\n"
            "  t1 = Task('t1') + Defstatus('complete')\n"
            "  t2 = Task('t2').add_defstatus( DState.suspended )\n\n"
            "  # we can also create a Defstatus in the Task constructor like any other attribute\n"
            "  t2 = Task('t3',\n"
            "            Defstatus('complete'))\n"
            ;
}

const char* DefsDoc::jobgenctrl_doc()
{
   return
            "The class JobCreationCtrl is used in `job creation` checking\n\n"
            "Constructor::\n\n"
            "   JobCreationCtrl()\n\n"
            "\nUsage::\n\n"
            "   defs = Defs('my.def')                     # specify the definition we want to check, load into memory\n"
            "   job_ctrl = JobCreationCtrl()\n"
            "   job_ctrl.set_node_path('/suite/to_check') # will hierarchically check job creation under this node\n"
            "   defs.check_job_creation(job_ctrl)         # job files generated to ECF_JOB\n"
            "   print(job_ctrl.get_error_msg())            # report any errors in job generation\n"
            "\n"
            "   job_ctrl = JobCreationCtrl()              # no set_node_path() hence check job creation for all tasks\n"
            "   job_ctrl.set_dir_for_job_creation(tmp)    # generate jobs file under this directory\n"
            "   defs.check_job_creation(job_ctrl)\n"
            "   print(job_ctrl.get_error_msg())\n"
            "\n"
            "   job_ctrl = JobCreationCtrl()              # no set_node_path() hence check job creation for all tasks\n"
            "   job_ctrl.generate_temp_dir()              # automatically generate directory for job file\n"
            "   defs.check_job_creation(job_ctrl)\n"
            "   print(job_ctrl.get_error_msg())\n"
            ;
}

const char* DefsDoc::check_job_creation_doc()
{
   return
            "Check `job creation` .\n\n"
            "Will check the following:\n\n"
            "- `ecf script`_ files and includes files can be located\n"
            "- recursive includes\n"
            "- manual and comments `pre-processing`_\n"
            "- `variable substitution`_\n\n"
            "Some `task`_ s are dummy tasks have no associated `ecf script`_ file.\n"
            "To disable error message for these tasks please add a variable called ECF_DUMMY_TASK to them.\n"
            "Checking is done in conjunction with the class :py:class:`ecflow.JobCreationCtrl`.\n"
            "If no node path is set on class JobCreationCtrl then all tasks are checked.\n"
            "In the case where we want to check all tasks, use the convenience function that take no arguments.\n"
            "\nUsage::\n\n"
            "   defs = Defs('my.def')                     # specify the defs we want to check, load into memory\n"
            "   ...\n"
            "   print(defs.check_job_creation())          # Check job generation for all tasks\n"
            "   ...\n\n"
            "   # throw on error and Output the tasks as they are being checked\n"
            "   defs.check_job_creation(throw_on_error=TrueTrue,verbose=True)\n\n"
            "   job_ctrl = JobCreationCtrl()\n"
            "   job_ctrl.set_verbose(True)                # Output the tasks as they are being checked\n"
            "   defs.check_job_creation(job_ctrl)         # Check job generation for all tasks, same as above\n"
            "   print(job_ctrl.get_error_msg())\n"
            "   ...\n"
            "   job_ctrl = JobCreationCtrl()\n"
            "   job_ctrl.set_node_path('/suite/to_check') # will hierarchically check job creation under this node\n"
            "   defs.check_job_creation(job_ctrl)         # job files generated to ECF_JOB\n"
            "   print(job_ctrl.get_error_msg())\n"
            "   ...\n"
            "   job_ctrl = JobCreationCtrl()              # no set_node_path() hence check job creation for all tasks\n"
            "   job_ctrl.set_dir_for_job_creation(tmp)    # generate jobs file under this directory\n"
            "   defs.check_job_creation(job_ctrl)\n"
            "   print(job_ctrl.get_error_msg())\n"
            "   ...\n"
            "   job_ctrl = JobCreationCtrl()              # no set_node_path() hence check job creation for all tasks\n"
            "   job_ctrl.generate_temp_dir()              # automatically generate directory for job file\n"
            "   defs.check_job_creation(job_ctrl)\n"
            "   print(job_ctrl.get_error_msg())\n"
            ;
}

const char* DefsDoc::generate_scripts_doc()
{
   return
            "Automatically generate template `ecf script`_ s for this definition\n"
            "Will automatically add `child command`_ s for `event`_, `meter`_ and `label`_ s.\n"
            "This allows the definition to be refined with out worrying about the scripts.\n"
            "However it should be noted that, this will create a lot of *duplicated* script contents\n"
            "i.e in the absence of `event`_ s, `meter`_ s and `label`_ s, most of generated `ecf script`_ files will\n"
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
            "   suite.add_variable('ECF_HOME','/user/var/home')\n"
            "   suite.add_variable('ECF_INCLUDE','/user/var/home/includes')\n"
            "   for i in range(1,7) :\n"
            "      fam = suite.add_family('f' + str(i))\n"
            "      for t in ( 'a', 'b', 'c', 'd', 'e' ) :\n"
            "        fam.add_task(t);\n"
            "   defs.generate_scripts()   # generate '.ecf' and head.h/tail.h if required\n"
            ;
}

const char* DefsDoc::check()
{
   return
            "Check `trigger`_ and `complete expression`_ s and `limit`_ s\n\n"
            "* Client Side: The client side can specify externs. Hence all node path references\n"
            "  in `trigger`_ expressions, and `inlimit`_ references to `limit`_ s, that are\n"
            "  unresolved and which do *not* appear in `extern`_ s are reported as errors\n"
            "* Server Side: The server does not store externs. Hence all unresolved references\n"
            "  are reported as errors\n\n"
            "Returns a non empty string for any errors or warning\n"
            "\nUsage::\n\n"
            "   # Client side\n"
            "   defs = Defs('my.def')        # Load my.def from disk\n"
            "   ....\n"
            "   print(defs.check()) # do the check\n"
            "\n"
            "   # Server Side\n"
            "   try:\n"
            "       ci = Client()             # use default host(ECF_HOST) & port(ECF_PORT)\n"
            "       print(ci.check('/suite'))\n"
            "   except RuntimeError, e:\n"
            "       print(str(e))\n"
            ;
}

const char* DefsDoc::simulate() {
   return
         "Simulates a suite definition, allowing you predict/verify the behaviour of your suite in few seconds\n\n"
         "The simulator will analyse the definition, and simulate the ecflow server.\n"
         "Allowing time dependencies that span several months, to be simulated in a few seconds.\n"
         "Ecflow allows the use of verify attributes. This example show how we can verify the number of times\n"
         "a task should run, given a start(optional) and end time(optional)::\n\n"
         "  suite cron3              # use real clock otherwise clock starts when the simulations starts.\n"
         "     clock real  1.1.2006  # define a start date for deterministic behaviour\n"
         "     endclock   13.1.2006  # When to finish. end clock is *only* used for the simulator\n"
         "     family cronFamily\n"
         "        task t\n"
         "           cron -d 10,11,12   10:00 11:00 01:00  # run on 10,11,12 of the month at 10am and 11am\n"
         "           verify complete:6                     # task should complete 6 times between 1.1.2006 -> 13.1.2006\n"
         "     endfamily\n"
         "  endsuite\n\n"
         "Please note, for deterministic behaviour, the start and end clock should be specified.\n"
         "However if no 'endclock' is specified the simulation will assume the following defaults.\n\n"
         "- No time dependencies: 24 hours\n"
         "- time || today       : 24 hours\n"
         "- day                 : 1 week\n"
         "- date                : 1 month\n"
         "- cron                : 1 year\n"
         "- repeat              : 1 year\n\n"
         "If there no time dependencies with an minute resolution, then the simulator will by default\n"
         "use 1 hour resolution. This needs to be taken into account when specifying the verify attribute\n"
         "If the simulation does not complete it creates  defs.flat and  defs.depth files.\n"
         "This provides clues as to the state of the definition at the end of the simulation\n"
         "\nUsage::\n\n"
         "   defs = Defs('my.def')        # specify the defs we want to simulate\n"
         "   ....\n"
         "   theResults = defs.simulate()\n"
         "   print(theResults)\n"
         ;
}

const char* DefsDoc::get_server_state()
{
   return
            "Returns the `ecflow_server`_ state: See `server states`_\n\n"
            "\nUsage::\n\n"
            "   try:\n"
            "       ci = Client()           # use default host(ECF_HOST) & port(ECF_PORT)\n"
            "       ci.shutdown_server()\n"
            "       ci.sync_local()\n"
            "       assert ci.get_defs().get_server_state() == SState.SHUTDOWN, 'Expected server to be shutdown'\n"
            "   except RuntimeError, e:\n"
            "       print(str(e))\n"
             ;
}
