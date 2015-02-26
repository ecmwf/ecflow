# Determine the python unicode

import sys
if sys.maxunicode > 65535:
...print 'UCS4 build'
else:
...print 'UCS2 build'