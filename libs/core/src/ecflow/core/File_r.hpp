/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_core_File_r_HPP
#define ecflow_core_File_r_HPP

#include <fstream>
#include <string>

namespace ecf {

class File_r {
public:
    explicit File_r(const std::string& file_name);

    // Disable copy (and move) semantics
    File_r(const File_r&)                  = delete;
    const File_r& operator=(const File_r&) = delete;
    File_r(File_r&&)                       = delete;
    File_r& operator=(File_r&&)            = delete;

    ~File_r();

    bool ok() const { return (fp_) ? true : false; }
    std::streamoff pos() { return fp_.tellg(); }
    void setPos(std::streamoff pos) { fp_.seekg(pos, fp_.beg); }
    bool good() const { return fp_.good(); }
    void getline(std::string& line) { std::getline(fp_, line); }
    const std::string& file_name() const { return file_name_; }

private:
    std::string file_name_;
    std::ifstream fp_;
};

} // namespace ecf

#endif /* ecflow_core_File_r_HPP */
