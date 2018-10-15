//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VNodeStateDiag.hpp"

#include "ServerHandler.hpp"

#include "DirectoryHandler.hpp"
#include "UiLog.hpp"
#include "VFile.hpp"
#include "VNode.hpp"
#include "ShellCommand.hpp"

VNodeStateDiag::VNodeStateDiag(VInfo_ptr info)
{
    if(!info)
        return;

    if(info->isNode() && info->node())
    {
        VNode *node=info->node();
        ServerHandler* s=info->server();

        if(node && s)
        {
            VFile_ptr tmpFile=VFile::createTmpFile(false);
            VFile_ptr tmpFile2=VFile::createTmpFile(false);

            s->writeDefs(info,tmpFile->path());
            std::string cmd="sh node_state_diag.sh \'" + tmpFile->path() + "\' " +
                    s->host() + " " + s->port() + " \'" +
                    info->nodePath() + "\' \'" + tmpFile2->path() + "\'";

            ShellCommand::run(cmd,"");
        }
    }
}
