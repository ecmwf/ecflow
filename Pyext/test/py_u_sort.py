#////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
# Name        :
# Author      : Avi
# Revision    : $Revision: #10 $
#
# Copyright 2009-2020 ECMWF.
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

import unittest 
from ecflow import *

class Test_sort_api(unittest.TestCase):
    def test_sort_api(self):
        nodes = [ Defs(), Suite('s'), Family('f'), Task('t')]
        sort_attr = ['event','meter','label','variable','all']
        for attr in sort_attr:
            for node in nodes:
                node.sort_attributes(attr)
                node.sort_attributes(attr,True)
                node.sort_attributes(attr,True,['/path','/path2'])

        sort_attr = [AttrType.event, AttrType.meter, AttrType.label,AttrType.variable, AttrType.all  ]
        for attr in sort_attr:
            for node in nodes:
                node.sort_attributes(attr)
                node.sort_attributes(attr,True)
                node.sort_attributes(attr,True,['/path','/path2'])

# see ECFLOW-1642
class Test_sort(unittest.TestCase):
    def get_events(self):
        return [Event(name) for name in
            "foo buzz bar google zulu mason".split()]

    def sorted_events(self):
        return [Event(name) for name in
            "bar buzz foo google mason zulu".split()]

    def expected_def(self):
        defs = Defs(Suite("mysuite",
                Family("fam1",self.get_events()),
                Family("fam2",self.sorted_events(),
                   Task("task1",self.sorted_events()),
                ),
                Task("task2",self.sorted_events()),
                self.sorted_events()))
        return defs

    def defs_to_sort(self):
        defs = Defs(Suite("mysuite",
                Family("fam1",self.get_events()),
                Family("fam2",self.get_events(),
                   Task("task1",self.get_events()),
                ),
                Task("task2",self.get_events()),
                self.get_events()))
        return defs
    
    def test_no_sort(self):
        defs = self.defs_to_sort()
        defs.sort_attributes('event',True,['/mysuite/fam1'])
        self.assertEqual(defs,self.expected_def(),"Expected\n" + str(self.expected_def()) + "but found\n" + str(defs))

    def test_no_sort2(self):
        defs = self.defs_to_sort()
        defs.sort_attributes(AttrType.event,True,['/mysuite/fam1'])
        self.assertEqual(defs,self.expected_def(),"Expected\n" + str(self.expected_def()) + "but found\n" + str(defs))
    
class Test_defs_sort(unittest.TestCase):
    def test_defs_sort(self):
        defs = Defs()
        defs.add_variable("ZFRED", "/tmp/")
        defs.add_variable("YECF_URL_CMD", "${BROWSER:=firefox} -new-tab %ECF_URL_BASE%/%ECF_URL%")
        defs.add_variable("XECF_URL_BASE", "http://www.ecmwf.int")
        defs.add_variable("AECF_URL", "publications/manuals/sms")
        assert len(list(defs.user_variables)) == 4, "Expected *user* 4 variable"    
     
        # sort
        expected = ['AECF_URL','XECF_URL_BASE','YECF_URL_CMD','ZFRED']
        actual = []
        defs.sort_attributes("variable");
        for v in defs.user_variables: actual.append(v.name())
        assert actual == expected,"Attributes not sorted, expected:" + str(expected) + " but found:" + str(actual)
 
        expected = ['AECF_URL','XECF_URL_BASE','YECF_URL_CMD','ZFRED','ZZ']
        actual = []
        defs.add_variable("ZZ", "x")
        defs.sort_attributes(AttrType.variable);
        for v in defs.user_variables: actual.append(v.name())
        assert actual == expected,"Attributes not sorted, expected:" + str(expected) + " but found:" + str(actual)
     
        expected = ['AA', 'AECF_URL','XECF_URL_BASE','YECF_URL_CMD','ZFRED','ZZ']
        actual = []
        defs.add_variable("AA", "x")
        defs.sort_attributes("all");
        for v in defs.user_variables: actual.append(v.name())
        assert actual == expected,"Attributes not sorted, expected:" + str(expected) + " but found:" + str(actual)
     
        expected = ['AA', 'AECF_URL', 'BB','XECF_URL_BASE','YECF_URL_CMD','ZFRED','ZZ']
        actual = []
        defs.add_variable("BB", "x")
        defs.sort_attributes(AttrType.all );
        for v in defs.user_variables: actual.append(v.name())
        assert actual == expected,"Attributes not sorted, expected:" + str(expected) + " but found:" + str(actual)
 
if __name__ == "__main__":
    unittest.main()
    print("All Tests pass")