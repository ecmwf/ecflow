// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <map>
#include <set>
#include <string>
#include <string_view>

#include "ecflow/core/cfg/CTree.hpp"
#include "ecflow/core/cfg/Catalogue.hpp"
#include "ecflow/core/cfg/Configuration.hpp"

namespace ecf::cfg {

///
/// @brief Maps the tree of a configuration file onto a layer of settings of a catalogue.
///
/// The tree has the following layout:
///
///   - the root is an object holding a mandatory `"version": 1`, the version of the format;
///   - every other value is placed at the path naming its setting in the file (Names::ecf_cfg), e.g.
///     `/ecflow/server/variables/ECF_PORT` for `ecflow.server.variables@ECF_PORT`.
///
/// The paths of the nodes of the tree, as JSON Pointers, are compared with the names of the settings.
///
/// The mapper refuses the following, naming the origin and the path in the tree:
///
///   - a missing or unsupported version;
///   - a member that names no setting of the catalogue, including settings without a name in the file;
///   - a value that does not have the native type of its setting (integer, boolean, or string),
///     or does not pass the check of its setting;
///   - a value other than an object where the catalogue expects an object.
///
class Mapper {
public:
    ///
    /// @brief Creates a mapper.
    ///
    /// @param[in] catalogue The catalogue of the settings; it must outlive the mapper
    ///
    explicit Mapper(const Catalogue& catalogue);

    ///
    /// @brief Maps a tree onto a layer.
    ///
    /// @param[in] tree The tree of the configuration file
    /// @param[in] layer The name of the layer, e.g. layers::ecf_cfg
    /// @param[in] origin Where the tree comes from, e.g. the file name, used in error messages
    /// @return The layer, holding a value for each setting present in the tree.
    /// @throws ConfigurationError if the tree is refused; the message names the origin and the path in the tree.
    ///
    [[nodiscard]] Layer map(const CTree& tree, std::string_view layer, std::string_view origin) const;

private:
    void map_object(const CTree& tree, std::map<Key, Value>& entries, std::string_view origin) const;

    std::map<std::string, const Setting*> settings_; ///< The settings, by name in the file, e.g. "/a/b/c"
    std::set<std::string> objects_;                  ///< The paths holding settings, e.g. "/a" and "/a/b"
};

} // namespace ecf::cfg
