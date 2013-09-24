#!/bin/ksh
#=============================================================================================
# Name        : 
# Author      : 
# Revision    : $Revision: #3 $ 
#
# Copyright 2009-2012 ECMWF. 
# This software is licensed under the terms of the Apache Licence version 2.0 
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
# In applying this licence, ECMWF does not waive the privileges and immunities 
# granted to it by virtue of its status as an intergovernmental organisation 
# nor does it submit to any jurisdiction. 
#
# Description : 
#=============================================================================================

return
function call {
grep -l sms_node * > sms_node.list     

for f in `cat sms_node.list`; do 
   cat $f | sed -e 's:sms_node:ecf_node:' > ${f}.tmp; ; 
   mv ${f} ${f}.orig; 
   mv ${f}.tmp ${f}; 
done

 clear; ./env.sh cc 2>&1 | head -50

}
