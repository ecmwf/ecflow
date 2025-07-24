/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_client_HostsFile_HPP
#define ecflow_client_HostsFile_HPP

#include <stdexcept>

#include "ecflow/core/Filesystem.hpp"

namespace ecf {

struct HostsFileFailure : public std::runtime_error
{
    explicit HostsFileFailure(const std::string& message) : std::runtime_error(message) {}
};

class HostsFile {
public:
    using host_t  = std::pair<std::string, std::string>; // host name and port
    using hosts_t = std::vector<host_t>;

    const hosts_t& hosts() const { return hosts_; }

    /**
     * @brief Loads the content of the given stream into a HostsFile.
     *
     * @see parse(const fs::path&, int) for the expected format.
     *
     * @param is the input stream to load
     * @param default_port the default port to use if not specified in the file
     * @return the loaded HostsFile
     */
    static HostsFile parse(std::istream& is, int default_port);

    /**
     * @brief Loads the content of the given file into a HostsFile.
     *
     * Each line is expected to have the following format:
     * ```
     *    # an optional comment
     * <host>          # another optional comment
     * <host>:<port>   # more optional comments
     * ```
     *
     * @remark An empty set of host/port in the file is *NOT* considered an error.
     *
     * @param file the file to load
     * @param default_port the default port to use if not specified in the file
     * @return the loaded HostsFile
     *
     * @throw HostsFileFailure if the file cannot be loaded or parsed.
     */
    static HostsFile parse(const fs::path& path, int default_port);

private:
    explicit HostsFile(hosts_t hosts) : hosts_(std::move(hosts)) {}
    hosts_t hosts_;
};

} // namespace ecf

#endif /* ecflow_client_ClientEnvironment_HPP */
