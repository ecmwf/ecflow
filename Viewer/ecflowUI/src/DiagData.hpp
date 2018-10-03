//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef DIAGDATA_HPP
#define DIAGDATA_HPP

#include <string>
#include <vector>

class VNode;
class DiagData;

class DiagDataServerItem
{
    friend class DiagData;

public:
    DiagDataServerItem(const std::string& host,const std::string& port,size_t);
    const std::string& dataAt(int row,int column) const;
    int findRowByPath(const std::string& path) const;

protected:
    bool checkSizes() const;
    std::string host_;
    std::string port_;
    std::vector<std::string> pathData_;
    std::vector<std::vector<std::string> > data_;
};

class DiagData
{
public:
    static DiagData* instance();

    void load();
    void loadFile(const std::string&);
    int count() const {return static_cast<int>(columnNames_.size());}
    const std::string& columnName(int i) const;
    const std::string& dataAt(VNode*,int column) const;

protected:
    DiagData();
    void clear();
    void updateTableModelColumn();
    DiagDataServerItem* findServerData(const std::string& host,const std::string& port) const;

    static DiagData* instance_;
    std::string fileName_;
    std::vector<std::string> columnNames_;
    std::vector<DiagDataServerItem*> serverData_;
};

#endif // DIAGDATA_HPP
