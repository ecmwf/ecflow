#////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
# Name        :
# Author      : Avi
# Revision    : $Revision: #10 $
#
# Copyright 2009-2017 ECMWF.
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
# SCRATCH test for ecflow python api

import ecflow

def find_variable_up_the_tree(node, varname):
    var = node.find_variable(varname)
    if not var.empty():
        return var.value()
            
    # search the generated variables
    variable_list = ecflow.VariableList()
    node.get_generated_variables(variable_list)
    for gen_var in variable_list:
        if gen_var.name() == varname:
            return gen_var.value() 
        
    parent = node.get_parent()
    if parent:
        return find_variable_up_the_tree(node=parent, varname=varname)
    else:
        return None

# Connect to server and get suite definition
client = ecflow.Client('localhost:4141')
client.ch_register(False, ['ecflow'])
client.sync_local()
defs = client.get_defs()

# Get a node
node = defs.find_abs_node('/ecflow/localhost/gnu/var_summary')
if node != None:
    node.update_generated_variables()  # does not seem to have any effect
else:
    print("Could not find node")
    exit(1)
    
# Find variables using two different methods:
#   a) Directly on the node using node.find_variable()
#   b) On the node and all parent nodes up the tree
task_vars = ['TASK', 'ECF_JOB', 'ECF_SCRIPT', 'ECF_JOBOUT', 'ECF_TRYNO',
             'ECF_RID', 'ECF_NAME']
family_vars = ['FAMILY', ]
suite_vars = ['SUITE', 'ECF_HOME', 'ECF_TIMEOUT', 'ECF_TRIES']
vars = task_vars + family_vars + suite_vars
 
print '\n=== b) ===\n'
for varname in vars:
    print '{0} = {1}'.format(varname, 
                             find_variable_up_the_tree(node, varname))
    