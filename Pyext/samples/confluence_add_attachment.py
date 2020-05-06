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

class TestConfluence(unittest.TestCase):
    def setUp(self):
        conflunce_base_url = "https://software-test.ecmwf.int/wiki"
#         conflunce_base_url = "https://software.ecmwf.int/wiki"
        user = "deploy"  
        password = "deploy2013"  
#         user = "ma0"
#         password = "chacchok1"
        
        import sys
        sys.path.append("/var/tmp/ma0/workspace/admin/2.0")
        import rest
        self.c = rest.Confluence(conflunce_base_url,user,password)

    def test_add_attachment_id(self):
        title = "Releases"
        space_key_list = [ "EMOS", "ECFLOW", "MAGP", "METV", "ECCUI", "ECC" ]
        for space_key in space_key_list:
            page_id = self.c.get_page_id(space_key,title) 
            if page_id == None:
                print("Could not find page id for space_key ",space_key," and title:",title)
            else:
                print("space key:",space_key," page id:",page_id)

        comment = "production release"
        file = "/var/tmp/ma0/workspace/ecflow/Pyext/samples/test.tar.gz"
        for space_key in space_key_list:
            page_id = self.c.get_page_id(space_key,title) 
            
            attachment = self.c.get_attachment_id(page_id,file)
            if attachment != None:
                print("space key:",space_key," allready has an attachment for file ",file)
            else:
                self.c.create_attachment(page_id,comment,file)

        
if __name__ == '__main__':
    unittest.main()
    print("All Tests pass")
    
