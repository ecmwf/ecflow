//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "DiagData.hpp"

#include <QString>
#include <QStringList>

#include "File.hpp"
#include "File_r.hpp"
#include "ModelColumn.hpp"
#include "NodePath.hpp"
#include "ServerHandler.hpp"
#include "Str.hpp"
#include "UIDebug.hpp"
#include "UiLog.hpp"
#include "VItemPathParser.hpp"
#include "VNode.hpp"

DiagData* DiagData::instance_ = nullptr;

DiagDataServerItem::DiagDataServerItem(const std::string& host, const std::string& port, size_t colNum)
    : host_(host),
      port_(port) {
    for (size_t i = 0; i < colNum; i++)
        data_.emplace_back();
}

const std::string& DiagDataServerItem::dataAt(int row, int column) const {
    return data_[column][row];
    // static std::string emptyStr;
    // return emptyStr;
}

int DiagDataServerItem::findRowByPath(const std::string& path) const {
    for (size_t i = 0; i < pathData_.size(); i++)
        if (pathData_[i] == path)
            return i;

    return -1;
}

bool DiagDataServerItem::checkSizes() const {
    size_t num = pathData_.size();
    for (const auto& d : data_) {
        if (d.size() != num)
            return false;
    }
    return true;
}

DiagData::DiagData() = default;

DiagData* DiagData::instance() {
    if (!instance_)
        instance_ = new DiagData();

    return instance_;
}

const std::string& DiagData::columnName(int i) const {
    if (i >= 0 && i < static_cast<int>(columnNames_.size()))
        return columnNames_[i];

    static std::string emptyStr;
    return emptyStr;
}

const std::string& DiagData::dataAt(VNode* vn, int column) const {
    static std::string emptyStr;
    if (!vn)
        return emptyStr;

    if (ServerHandler* sh = vn->server()) {
        if (DiagDataServerItem* d = findServerData(sh->host(), sh->port())) {
            int row = d->findRowByPath(vn->absNodePath());
            if (row > -1)
                return d->dataAt(row, column);
        }
    }

    return emptyStr;
}

DiagDataServerItem* DiagData::findServerData(const std::string& host, const std::string& port) const {
    if (host.empty() || port.empty())
        return nullptr;

    for (size_t i = 0; i < serverData_.size(); i++) {
        if (serverData_[i]->host_ == host && serverData_[i]->port_ == port)
            return serverData_[i];
    }

    return nullptr;
}

void DiagData::clear() {
    fileName_.clear();
    columnNames_.clear();
    for (size_t i = 0; i < serverData_.size(); i++) {
        delete serverData_[i];
    }
    serverData_.clear();
}

void DiagData::load() {
    if (const char* df = getenv("ECFLOWUI_DIAG_FILE"))
        loadFile(std::string(df));
}

void DiagData::loadFile(const std::string& fileName) {
    clear();

    fileName_ = fileName;

    /// The log file can be massive > 50Mb
    ecf::File_r diag_file(fileName_);
    if (!diag_file.ok()) {
        UiLog().warn() << "DiagData::loadFile: Could not open diagnostics file " + fileName;
        return;
    }

    std::string line;

    // The first line is the header
    diag_file.getline(line); // default delimiter is /n

    // No header, no diag data can be read
    if (line.empty())
        return;

    QString headerLine = QString::fromStdString(line);
    if (headerLine.startsWith("#"))
        headerLine = headerLine.mid(1);

    QStringList headerLst = headerLine.split(",");
    int pathColumnIndex   = -1;
    for (int i = 0; i < headerLst.count(); i++) {
        if (headerLst[i].compare("path", Qt::CaseInsensitive) == 0)
            pathColumnIndex = i;
        else
            columnNames_.push_back(headerLst[i].toStdString());
    }

    if (pathColumnIndex == -1) {
        UiLog().warn() << "DiagData::loadFile: could not find node path column in file. Diagnostics cannot be loaded!";
        return;
    }

    while (diag_file.good()) {
        diag_file.getline(line); // default delimiter is /n

        if (line.size() <= 1)
            continue;

        if (line[0] == '#')
            line = line.substr(1);

        // split by comma
        QStringList lst = QString::fromStdString(line).split(",");

        // When number of items does not match expected number we skip the line
        if (lst.count() != static_cast<int>(columnNames_.size()) + 1)
            continue;

        std::string path = lst[pathColumnIndex].toStdString();
        std::string host, port;
        DiagDataServerItem* data = nullptr;
        VItemPathParser parser(path, VItemPathParser::DiagFormat);
        if (parser.itemType() != VItemPathParser::NoType) {
            data = findServerData(parser.host(), parser.port());
            if (!data) {
                data = new DiagDataServerItem(parser.host(), parser.port(), columnNames_.size());
                serverData_.push_back(data);
            }
        }
        else {
            continue;
        }

        data->pathData_.push_back(parser.node());
        assert(data->data_.size() == columnNames_.size());

        for (int i = 0; i < lst.count(); i++) {
            QString val = lst[i];
#if 0
            QStringList vals=lst[i].split(":");
            QString val=lst[i];
            if(vals.count() == 2)
                columnData_[i].push_back(vals[1].toStdString());
            else
                columnData_[i].push_back(lst[i].toStdString());
#endif

            int idx = i;
            if (idx > pathColumnIndex)
                idx--;

            if (i != pathColumnIndex)
                data->data_[idx].push_back(val.toStdString());
        }
    }

    updateTableModelColumn();
}

void DiagData::updateTableModelColumn() {
    if (ModelColumn* mc = ModelColumn::def("table_columns")) {
        mc->setDiagData(this);
    }
}
