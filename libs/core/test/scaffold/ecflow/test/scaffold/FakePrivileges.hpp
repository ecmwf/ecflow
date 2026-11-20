// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "ecflow/core/os/Privileges.hpp"

namespace ecf::test::scaffold {

///
/// @brief Privileges of a pretended server process, for tests of the checks made at start-up.
///
/// @details Each property is set by the test; the operations are recorded, and fail when told to.
///
class FakePrivileges : public ecf::os::Privileges {
public:
    uid_t uid{500};                                ///< The effective user of the process.
    bool supports_capabilities{true};              ///< Whether the platform has capabilities (Linux).
    bool has_capabilities{false};                  ///< Whether CAP_SETUID and CAP_SETGID are permitted.
    bool fail_raise{false};                        ///< Whether raising the capabilities fails.
    std::optional<std::string> trial_failure;      ///< The reason the trial switch fails, if it does.
    std::optional<std::string> executable_problem; ///< The reason the executable is refused, if it is.
    bool raised{false};                            ///< Set once the capabilities were raised.
    std::vector<ecf::os::OsIdentity> trials;       ///< The identities of the trial switches made.

    uid_t effective_uid() const override { return uid; }
    bool capabilities_supported() const override { return supports_capabilities; }
    bool has_identity_capabilities() const override { return supports_capabilities && has_capabilities; }

    void raise_identity_capabilities() override {
        if (fail_raise || !has_identity_capabilities()) {
            throw std::runtime_error("could not raise the capabilities");
        }
        raised = true;
    }

    void trial_switch(const ecf::os::OsIdentity& identity) override {
        trials.push_back(identity);
        if (trial_failure) {
            throw std::runtime_error(*trial_failure);
        }
    }

    std::optional<std::string> check_executable() const override { return executable_problem; }
};

} // namespace ecf::test::scaffold
