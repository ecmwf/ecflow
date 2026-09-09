# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

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
