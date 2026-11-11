// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/server/OwnershipStartup.hpp"

#include <stdexcept>

#include "ecflow/core/os/OwnerEligibility.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Ownership.hpp"
#include "ecflow/node/Suite.hpp"

namespace ecf {

namespace {

[[noreturn]] void refuse(const std::string& reason) {
    throw std::runtime_error("Ownership: the server cannot start: " + reason);
}

} // namespace

std::string OwnershipStatus::summary() const {
    if (!enabled || !server_owner) {
        return "disabled";
    }
    return "enabled, server Owner " + server_owner->value() + ", " + privileges;
}

OwnershipStatus check_ownership_at_startup(const OwnershipSettings& settings,
                                           const os::AccountDatabase& accounts,
                                           os::Privileges& privileges,
                                           bool available_in_build) {
    OwnershipStatus status;
    if (!settings.owner) {
        return status;
    }
    if (!available_in_build) {
        refuse("a server Owner is configured ('" + *settings.owner +
               "'), but ownership cannot be enabled in this build of ecFlow");
    }

    // The server Owner: an eligible account (R4, R5, R8)
    OwnerAttr owner;
    try {
        owner = OwnerAttr::create(*settings.owner);
    }
    catch (const std::exception& e) {
        refuse("Invalid server Owner '" + *settings.owner + "': " + e.what());
    }
    auto eligible = os::OwnerEligibility(accounts, privileges.effective_uid()).check(owner.user(), owner.group());
    if (!eligible.ok()) {
        refuse("Invalid server Owner: " + eligible.reason());
    }
    const os::OsIdentity identity = eligible.value();

    // The privilege to change identity (R9, D14-D16)
    if (privileges.effective_uid() == 0) {
        status.privileges = "running as root";
        status.warnings.emplace_back("Ownership: the server runs as root, which is not recommended; prefer a "
                                     "dedicated account with the capabilities CAP_SETUID and CAP_SETGID");
        if (!privileges.capabilities_supported()) {
            status.warnings.emplace_back("Ownership: on this platform, only the commands are run as the owner; files "
                                         "are accessed as the server account");
        }
    }
    else if (privileges.capabilities_supported()) {
        if (!privileges.has_identity_capabilities()) {
            refuse("changing identity requires the capabilities CAP_SETUID and CAP_SETGID, or running as root");
        }
        try {
            privileges.raise_identity_capabilities();
        }
        catch (const std::exception& e) {
            refuse(e.what());
        }
        status.privileges = "capabilities CAP_SETUID and CAP_SETGID";
    }
    else {
        refuse("changing identity requires running as root on this platform");
    }
    try {
        privileges.trial_switch(identity);
    }
    catch (const std::exception& e) {
        refuse(e.what());
    }

    // The executable must not hand the privileges to other accounts (R58, D23)
    if (auto problem = privileges.check_executable(); problem) {
        refuse("the server executable is unsafe: " + *problem);
    }

    status.enabled      = true;
    status.server_owner = owner;
    return status;
}

std::optional<std::string> owners_without_ownership_warning(const Defs& defs, const OwnershipStatus& status) {
    if (status.enabled) {
        return std::nullopt;
    }
    for (const auto& suite : defs.suites()) {
        // Archived nodes included, read from their archive files (an unreadable one is skipped, with a warning)
        if (!ownership::owners_including_archives(*suite).empty()) {
            return std::string("Ownership is disabled, but the definitions hold Owners: their commands run as the "
                               "server account");
        }
    }
    return std::nullopt;
}

} // namespace ecf
