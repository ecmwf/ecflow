/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_test_scaffold_Serialisation_HPP
#define ecflow_test_scaffold_Serialisation_HPP

#include <exception>
#include <string>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/Serialization.hpp"

namespace ecf {

template <typename T>
void do_restore(const std::string& fileName, const T& saved) {
    T restored;
    try {
        ecf::restore(fileName, restored);
    }
    catch (std::exception& e) {
        BOOST_CHECK_MESSAGE(false, "Restore failed because: " << e.what());
    }
    BOOST_CHECK_MESSAGE(saved == restored, "save and restored don't match for " << fileName << "\n");
}

template <typename T>
void doRestore(const std::string& fileName, const T& saved) {
    do_restore(fileName, saved);
    std::remove(fileName.c_str());
}

template <typename T>
void doSave(const std::string& fileName, const T& saved) {
    try {
        ecf::save(fileName, saved);
    }
    catch (std::exception& e) {
        BOOST_CHECK_MESSAGE(false, "Save  failed because: " << e.what());
    }
}

template <typename T>
void doSave(const std::string& fileName) {
    T saved;
    doSave(fileName, saved);
}

template <typename T>
void doSaveAndRestore(const std::string& fileName, const T& saved) {
    doSave(fileName, saved);
    doRestore(fileName, saved);
}

template <typename T>
void doSaveAndRestore(const std::string& fileName) {
    T saved;
    doSaveAndRestore(fileName, saved);
}

} // namespace ecf

#endif /* ecflow_test_scaffold_Serialisation_HPP */
