#!/bin/sh

## Copyright 2009-2012 ECMWF. 
## This software is licensed under the terms of the Apache Licence version 2.0 
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
## In applying this licence, ECMWF does not waive the privileges and immunities 
## granted to it by virtue of its status as an intergovernmental organisation 
## nor does it submit to any jurisdiction. 

# kill any running test
ps -ef | grep ma0 | grep gcc | grep log_level=message | cut -c9-14 | xargs kill -9
ps -ef | grep ma0 | grep test.sh | cut -c9-14 | xargs kill -9

# kill any existing test based server 
ps -ef | grep ma0 | grep ecfinterval | grep ecflow_server | cut -c9-14 | xargs kill -9
ps -ef | grep ma0 | grep dis_job_gen | grep ecflow_server | cut -c9-14 | xargs kill -9
ps -ef | grep ma0 | grep ecbuild | grep debug | grep ecflow_server | cut -c9-14 | xargs kill -9
ps -ef | grep ma0 | grep gcc | grep ecflow_server | cut -c9-14 | xargs kill -9
ps -ef | grep ma0 | grep gcc | grep ecflow_client | cut -c9-14 | xargs kill -9

ps -ef | grep ma0 | grep clientRoot | grep ecbuild | grep debug | cut -c9-14 | xargs kill -9
ps -ef | grep ma0 | grep clientRoot | grep ecbuild | grep release | cut -c9-14 | xargs kill -9

# kill any jobs
ps -ef | grep ma0  | grep job | grep test | cut -c9-14 | xargs kill -9

