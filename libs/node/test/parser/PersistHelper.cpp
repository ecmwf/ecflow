/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "PersistHelper.hpp"

#include <sstream>

#include "TemporaryFile.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/node/Defs.hpp"

using namespace std;
using namespace ecf;

bool PersistHelper::test_persist_and_reload(const Defs& theInMemoryDefs,
                                            PrintStyle::Type_t file_type_on_disk,
                                            bool do_compare) {
    // Write parsed file to disk, and reload, then compare defs, they should be the same
    errorMsg_.clear();
    file_size_ = 0;

    TemporaryFile temporary("tmp_%%%%-%%%%-%%%%-%%%%.def");

    {
        // The file MUST be written in the *SAME* form that it was read, Otherwise they will not compare:
        theInMemoryDefs.write_to_file(temporary.path(), file_type_on_disk);
    }

    // Reload the file we just persisted and compare with in memory defs
    Defs savedDef;
    return reload_from_defs_file(theInMemoryDefs, savedDef, temporary.path(), do_compare);
}

bool PersistHelper::test_defs_checkpt_and_reload(const Defs& theInMemoryDefs, bool do_compare) {
    // Write parsed file to disk, and reload, then compare defs, they should be the same
    errorMsg_.clear();
    file_size_ = 0;

    TemporaryFile temporary("tmp_%%%%-%%%%-%%%%-%%%%.def");

    {
        // The file MUST be written in the *SAME* form that it was read, Otherwise they will not compare:
        theInMemoryDefs.save_as_checkpt(temporary.path());
    }

    // Reload the file we just persisted and compare with in memory defs
    Defs savedDef;
    bool reload_result = reload_from_defs_file(theInMemoryDefs, savedDef, temporary.path(), do_compare);
    if (reload_result)
        return savedDef.checkInvariants(errorMsg_);
    return false;
}

bool PersistHelper::test_cereal_checkpt_and_reload(const Defs& theInMemoryDefs, bool do_compare) {
    errorMsg_.clear();
    file_size_ = 0;
    if (!theInMemoryDefs.checkInvariants(errorMsg_)) {
        return false;
    }

    // Save in memory defs as a check pt file, then restore and compare
    Defs reloaded_defs;
    bool reload_result = reload_from_cereal_checkpt_file(theInMemoryDefs, reloaded_defs, do_compare);
    if (reload_result)
        return reloaded_defs.checkInvariants(errorMsg_);
    return false;
}

bool PersistHelper::test_state_persist_and_reload_with_checkpt(const Defs& theInMemoryDefs) {
    // Write Defs to disk, and reload, then compare defs reloaded checkpt file, they should be the same
    errorMsg_.clear();
    file_size_ = 0;
    if (!theInMemoryDefs.checkInvariants(errorMsg_)) {
        return false;
    }

    DebugEquality debug_equality; // only as affect in DEBUG build

    TemporaryFile temporary("tmp_%%%%-%%%%-%%%%-%%%%.def");

    {
        // The file MUST be written in the *SAME* form that it was read, Otherwise they will not compare:
        theInMemoryDefs.save_as_checkpt(temporary.path()); // will save edit history
    }

    Defs reload_strings_def;
    {
        // Open file, and parse as a string.
        std::string defs_as_string;
        if (!File::open(temporary.path(), defs_as_string)) {
            errorMsg_ += "Could not file file: " + temporary.path();
            return false;
        }
        std::string error_msg, warning;
        if (!reload_strings_def.restore_from_string(defs_as_string, error_msg, warning)) {
            errorMsg_ += error_msg;
            return false;
        }
        if (!reload_strings_def.checkInvariants(errorMsg_)) {
            return false;
        }
    }

    // Reload the file we just persisted and compare with in memory defs
    Defs reloaded_defs;
    if (!reload_from_defs_file(theInMemoryDefs, reloaded_defs, temporary.path())) {
        return false;
    }
    if (!reloaded_defs.checkInvariants(errorMsg_)) {
        return false;
    }

    // Save in memory defs as a check pt file, then restore and compare
    Defs reloaded_cereal_checkPt_defs;
    if (!reload_from_cereal_checkpt_file(theInMemoryDefs, reloaded_cereal_checkPt_defs, true)) {
        return false;
    }
    if (!reloaded_cereal_checkPt_defs.checkInvariants(errorMsg_)) {
        return false;
    }

    // Make sure reloading def's file with state is same as the checkpt file
    bool match = reloaded_defs == reloaded_cereal_checkPt_defs;

    if (!match) {
        std::stringstream ss;
        ss << "\nPersistHelper::test_state_persist_and_reload_with_checkpt\n";
        ss << "In reloaded_defs_file and reloaded_checkPt_defs don't match\n";
        ss << "+++++++++++++ in memory defs  ++++++++++++++++++++++++++++\n";
        PrintStyle style(PrintStyle::MIGRATE); // will save edit history
        ss << theInMemoryDefs;
        ss << "+++++++++++++ reloaded_defs  ++++++++++++++++++++++++++++\n";
        ss << reloaded_defs;
        ss << "++++++++++++++ reloaded_checkPt_defs  ++++++++++++++++++++++++++++\n";
        ss << reloaded_cereal_checkPt_defs;
        errorMsg_ += ss.str();
    }
    else {
        if (compare_edit_history_ && !reloaded_defs.compare_edit_history(reloaded_cereal_checkPt_defs)) {
            std::stringstream ss;
            ss << "\nPersistHelper::test_state_persist_and_reload_with_checkpt  compare_edit_history_\n";
            ss << "In reloaded_defs_file and reloaded_checkPt_defs edit history don't match\n";
            ss << "+++++++++++++ in memory defs  ++++++++++++++++++++++++++++\n";
            PrintStyle style(PrintStyle::MIGRATE); // will save edit history
            ss << theInMemoryDefs;
            ss << "+++++++++++++ reloaded_defs  ++++++++++++++++++++++++++++\n";
            ss << reloaded_defs;
            ss << "++++++++++++++ reloaded_checkPt_defs  ++++++++++++++++++++++++++++\n";
            ss << reloaded_cereal_checkPt_defs;
            errorMsg_ += ss.str();
        }
    }
    if (!reloaded_defs.compare_change_no(reloaded_cereal_checkPt_defs)) {
        errorMsg_ += "\nPersistHelper::test_state_persist_and_reload_with_checkpt: Change numbers don't compare "
                     "between reloaded_defs and reloaded_cereal_checkPt_defs \n";
    }

    // Make sure reloading def's file with state is same as the checkpt file
    match = reload_strings_def == reloaded_cereal_checkPt_defs;
    if (!match) {
        std::stringstream ss;
        ss << "\nPersistHelper::test_state_persist_and_reload_with_checkpt\n";
        ss << "In reloaded_defs file AS STRING and reloaded_checkPt_defs don't match\n";
        ss << "+++++++++++++ in memory defs  ++++++++++++++++++++++++++++\n";
        PrintStyle style(PrintStyle::MIGRATE); // will save edit history
        ss << theInMemoryDefs;
        ss << "+++++++++++++  reload_strings_def  ++++++++++++++++++++++++++++\n";
        ss << reload_strings_def;
        ss << "++++++++++++++ reloaded_checkPt_defs  ++++++++++++++++++++++++++++\n";
        ss << reloaded_cereal_checkPt_defs;
        errorMsg_ += ss.str();
    }
    else {
        if (compare_edit_history_ && !reload_strings_def.compare_edit_history(reloaded_cereal_checkPt_defs)) {
            std::stringstream ss;
            ss << "\nPersistHelper::test_state_persist_and_reload_with_checkpt  compare_edit_history_\n";
            ss << "In reloaded_defs_file and reloaded_checkPt_defs edit history don't match\n";
            ss << "+++++++++++++ in memory defs  ++++++++++++++++++++++++++++\n";
            PrintStyle style(PrintStyle::MIGRATE); // will save edit history
            ss << theInMemoryDefs;
            ss << "+++++++++++++ reload_strings_def ++++++++++++++++++++++++++++\n";
            ss << reload_strings_def;
            ss << "++++++++++++++ reloaded_checkPt_defs  ++++++++++++++++++++++++++++\n";
            ss << reloaded_cereal_checkPt_defs;
            errorMsg_ += ss.str();
        }
    }
    if (!reload_strings_def.compare_change_no(reloaded_cereal_checkPt_defs)) {
        errorMsg_ += "\nPersistHelper::test_state_persist_and_reload_with_checkpt: Change numbers don't compare "
                     "between reload_strings_def and reloaded_cereal_checkPt_defs\n";
    }

    return errorMsg_.empty();
}

bool PersistHelper::reload_from_defs_file(const Defs& theInMemoryDefs,
                                          Defs& reloaded_defs,
                                          const std::string& tmpFilename,
                                          bool do_compare) {
    DebugEquality debug_equality; // only as affect in DEBUG build

    std::string warningMsg;
    if (!reloaded_defs.restore(tmpFilename, errorMsg_, warningMsg)) {
        std::stringstream ss;
        ss << "RE-PARSE failed for " << tmpFilename << "\n";
        errorMsg_ += ss.str();
        return false;
    }

    if (do_compare) {
        // Make sure the file we just parsed match's the one we persisted
        bool match = reloaded_defs == theInMemoryDefs;

        if (!match) {
            std::stringstream ss;
            ss << "\nPersistHelper::reload_from_defs_file\n";
            ss << "In memory and reloaded def's don't match\n";
            ss << "+++++++++++++ Saved/reloaded_defs  ++++++++++++++++++++++++++++\n";
            PrintStyle style(PrintStyle::STATE);
            ss << reloaded_defs;
            ss << "++++++++++++++ In memory def ++++++++++++++++++++++++++++\n";
            ss << theInMemoryDefs;
            errorMsg_ += ss.str();
        }
        else {
            if (compare_edit_history_ && !reloaded_defs.compare_edit_history(theInMemoryDefs)) {
                std::stringstream ss;
                ss << "\nPersistHelper::reload_from_defs_file compare_edit_history_\n";
                ss << "In memory and reloaded def's don't match\n";
                ss << "+++++++++++++ Saved/reloaded_defs  ++++++++++++++++++++++++++++\n";
                PrintStyle style(PrintStyle::MIGRATE);
                ss << reloaded_defs;
                ss << "++++++++++++++ In memory def ++++++++++++++++++++++++++++\n";
                ss << theInMemoryDefs;
                errorMsg_ += ss.str();
            }
        }
        if (!reloaded_defs.compare_change_no(theInMemoryDefs)) {
            errorMsg_ += "\nPersistHelper::reload_from_defs_file: Change numbers don't compare between reloaded_defs  "
                         "and theInMemoryDefs  \n";
        }
    }

    file_size_ = fs::file_size(tmpFilename);
    std::remove(tmpFilename.c_str());
    return errorMsg_.empty();
}

bool PersistHelper::reload_from_cereal_checkpt_file(const Defs& theInMemoryDefs, Defs& reloaded_defs, bool do_compare) {
    // make sure edit history is stored
    TemporaryFile temporary("tmp.check_%%%%-%%%%-%%%%-%%%%");

    theInMemoryDefs.cereal_save_as_checkpt(temporary.path());

    DebugEquality debug_equality; // only as effect in DEBUG build

    try {
        // Parse the file we just persisted and load the defs file into memory.
        reloaded_defs.cereal_restore_from_checkpt(temporary.path());

        if (do_compare) {
            // Make sure the checkpoint the file we just parsed corresponds to the one we stored
            bool match = reloaded_defs == theInMemoryDefs;
            if (!match) {
                std::stringstream ss;
                ss << "\nPersistHelper::reload_from_cereal_checkpt_file\n";
                ss << "In memory and reloaded def's don't match\n";
                ss << "+++++++++++++ Saved/reloaded check pt file ++++++++++++++++++++++++++++\n";
                PrintStyle style(PrintStyle::STATE);
                ss << reloaded_defs;
                ss << "++++++++++++++ In memory def ++++++++++++++++++++++++++++\n";
                ss << theInMemoryDefs;
                errorMsg_ += ss.str();
            }
            else {
                if (compare_edit_history_ && !reloaded_defs.compare_edit_history(theInMemoryDefs)) {
                    std::stringstream ss;
                    ss << "\nPersistHelper::reload_from_cereal_checkpt_file  compare_edit_history_\n";
                    ss << "In reloaded_defs_file and reloaded_checkPt_defs edit history don't match\n";
                    ss << "+++++++++++++ Saved/reloaded check pt file ++++++++++++++++++++++++++++\n";
                    PrintStyle style(PrintStyle::MIGRATE);
                    ss << reloaded_defs;
                    ss << "++++++++++++++ theInMemoryDefs  ++++++++++++++++++++++++++++\n";
                    ss << theInMemoryDefs;
                    errorMsg_ += ss.str();
                }
            }
            if (!reloaded_defs.compare_change_no(theInMemoryDefs)) {
                errorMsg_ += "\nPersistHelper::reload_from_cereal_checkpt_file: Change numbers don't compare between "
                             "reloaded_defs and theInMemoryDefs  \n";
            }
        }
    }
    catch (std::exception& e) {
        errorMsg_ = "PersistHelper::reload_from_cereal_checkpt_file: " + string(e.what());
    }

    file_size_ = temporary.size();

    return errorMsg_.empty();
}
