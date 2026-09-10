/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_node_parser_test_PersistHelper_HPP
#define ecflow_node_parser_test_PersistHelper_HPP

#include <string>

#include "ecflow/core/PrintStyle.hpp"

class Defs;

/// Given a in memory defs file, this class will write it to disk
/// and reload the definition file structure and will then make a comparison
/// to ensure they are the same
class PersistHelper {
public:
    explicit PersistHelper(bool compare_edit_history = false)
        : compare_edit_history_(compare_edit_history) {}

    PersistHelper(const PersistHelper&)            = delete;
    PersistHelper& operator=(const PersistHelper&) = delete;
    PersistHelper(PersistHelper&&)                 = delete;
    PersistHelper& operator=(PersistHelper&&)      = delete;

    ~PersistHelper() = default;

    bool
    test_persist_and_reload(const Defs& theInMemoryDefs, PrintStyle::Type_t file_type_on_disk, bool do_compare = true);
    bool test_defs_checkpt_and_reload(const Defs& theInMemoryDefs, bool do_compare = true);
    bool test_cereal_checkpt_and_reload(const Defs& theInMemoryDefs, bool do_compare = true);
    bool test_state_persist_and_reload_with_checkpt(const Defs& theInMemoryDefs);
    const std::string& errorMsg() const { return errorMsg_; }

    /// returns the file size of the temporary file created by:
    ///   test_persist_and_reload(..) or test_cereal_checkpt_and_reload(..)
    size_t file_size() const { return file_size_; }

private:
    bool reload_from_defs_file(const Defs& theInMemoryDefs,
                               Defs& reloaded_defs,
                               const std::string& filename,
                               bool do_compare = true);
    bool reload_from_cereal_checkpt_file(const Defs& theInMemoryDefs, Defs& reloaded_defs, bool do_compare = true);

    std::string errorMsg_;
    size_t file_size_{0};
    bool compare_edit_history_;
};

#endif /* ecflow_node_parser_test_PersistHelper_HPP */
