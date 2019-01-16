#!/bin/bash

#============================================================================
# Copyright 2009-2019 ECMWF.
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#============================================================================


echo "Create suite definition"

rm -rf ui_test_gen
mkdir -p ui_test_gen
cd ui_test_gen


# Figure out the directory of the exe
EXE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
echo "$EXE_DIR"
ETC_DIR="$EXE_DIR/../../share/ecflow/etc"
DEF_FILE="$ETC_DIR/ecflow_ui_test.def"

ECFTEST_SCRIPTS_HOME="."
TBPY=$EXE_DIR/../../Doc/online/cookbook/src/test_bench.py
unset WK # otherwise it will be used by the Python script
export ECF_PORT=4949
python $TBPY --port $ECF_PORT --ecf_home $ECFTEST_SCRIPTS_HOME $DEF_FILE

tar -pzcf ../ecflow_ui_test_server_scripts.tar.gz operation_suite includes

echo "  ** Now copy ecflow_ui_test_server_scripts.tar.gz into $ETC_DIR **"
