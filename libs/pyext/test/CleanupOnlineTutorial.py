#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

import os

# clean up any file after running the online tutorial examples
# Place in separate exception, otherwise not all files will be removed
try:
    os.remove("test.def")
except:
    pass

try:
    os.remove("TestSuite.def")
except:
    pass

try:
    os.remove("SuiteBuilder.def")
except:
    pass

try:
    os.remove("server.defs")
except:
    pass
