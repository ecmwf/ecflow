# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

import unittest

username = "<some-user>"
password = "<some-secret>"
workspace = "/path/to/workspace"

class TestConfluence(unittest.TestCase):
    def setUp(self):
        confluence_base_url = "https://confluence.ecmwf.int"

        import sys
        sys.path.append(f"{workspace}/admin/2.0")
        import rest
        self.c = rest.Confluence(confluence_base_url, username, password)

    def test_add_attachment_id(self):
        title = "Releases"
        space_key_list = [ "EMOS", "ECFLOW", "MAGP", "METV", "ECCUI", "ECC" ]
        for space_key in space_key_list:
            page_id = self.c.get_page_id(space_key,title) 
            if page_id is None:
                print("Could not find page id for space_key ",space_key," and title:",title)
            else:
                print("space key:",space_key," page id:",page_id)

        comment = "production release"
        file = f"{workspace}/ecflow/libs/pyext/samples/test.tar.gz"
        for space_key in space_key_list:
            page_id = self.c.get_page_id(space_key,title) 
            
            attachment = self.c.get_attachment_id(page_id,file)
            if attachment is not None:
                print("space key:",space_key," allready has an attachment for file ",file)
            else:
                self.c.create_attachment(page_id,comment,file)

        
if __name__ == '__main__':
    unittest.main()
    print("All Tests pass")
