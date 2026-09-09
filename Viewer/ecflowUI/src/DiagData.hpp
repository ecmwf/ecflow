/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_DiagData_HPP
#define ecflow_viewer_DiagData_HPP

#include <string>
#include <vector>

class VNode;
class DiagData;

class DiagDataServerItem {
    friend class DiagData;

public:
    DiagDataServerItem(const std::string& host, const std::string& port, size_t);
    const std::string& dataAt(int row, int column) const;
    int findRowByPath(const std::string& path) const;

protected:
    bool checkSizes() const;
    std::string host_;
    std::string port_;
    std::vector<std::string> pathData_;
    std::vector<std::vector<std::string>> data_;
};

class DiagData {
public:
    static DiagData* instance();

    void load();
    void loadFile(const std::string&);
    int count() const { return static_cast<int>(columnNames_.size()); }
    const std::string& columnName(int i) const;
    const std::string& dataAt(VNode*, int column) const;

protected:
    DiagData();
    void clear();
    void updateTableModelColumn();
    DiagDataServerItem* findServerData(const std::string& host, const std::string& port) const;

    static DiagData* instance_;
    std::string fileName_;
    std::vector<std::string> columnNames_;
    std::vector<DiagDataServerItem*> serverData_;
};

#endif /* ecflow_viewer_DiagData_HPP */
