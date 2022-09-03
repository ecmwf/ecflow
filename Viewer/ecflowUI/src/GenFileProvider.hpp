//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef GENFILEPROVIDER_HPP
#define GENFILEPROVIDER_HPP

#include <string>
#include <vector>

#include "FetchTask.hpp"

class VReply;

class GenFileProvider : public FetchQueueOwner
{
public:
    GenFileProvider();
    ~GenFileProvider();
    void clear() override;

    virtual void fetchFile(const std::string&);
    virtual void fetchFiles(const std::vector<std::string>&);
    VReply* theReply() const override {return reply_;}
    void fetchQueueSucceeded() override;
    void fetchQueueFinished(const std::string& filePath, VNode*) override;

protected:
    VReply* reply_{nullptr};
};

#endif // GENFILEPROVIDER_HPP
