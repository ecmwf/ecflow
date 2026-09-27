// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <system_error>
#include <unistd.h>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <sys/socket.h>
#include <sys/un.h>

#include "ecflow/core/Filesystem.hpp"
#include "ecflow/node/Signal.hpp"
#include "ecflow/node/System.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf;

namespace {

///
/// @brief Descriptors of every kind, opened in the test process, and closed when the fixture is destroyed.
///
/// The descriptors are those that a child spawned by the server must not inherit: the ends of a pipe, a file,
/// a pair of connected sockets, a listening socket, a descriptor marked close-on-exec, and descriptors moved to
/// high numbers, up to the one just below the soft limit on open files.
///
struct InheritableDescriptors
{
    InheritableDescriptors() {
        // A file or socket left by an interrupted run would make open or bind fail
        fs::remove(opened_.file);
        fs::remove(opened_.socket);

        int pipe_ends[2];
        BOOST_REQUIRE(::pipe(pipe_ends) == 0);
        add(pipe_ends[0], true);
        add(pipe_ends[1], true);

        add(::open(opened_.file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600), true);

        int pair[2];
        BOOST_REQUIRE(::socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
        add(pair[0], true);
        add(pair[1], true);

        int listener = ::socket(AF_UNIX, SOCK_STREAM, 0);
        BOOST_REQUIRE(listener >= 0);
        sockaddr_un address{};
        address.sun_family = AF_UNIX;
        BOOST_REQUIRE(opened_.socket.size() < sizeof(address.sun_path));
        std::snprintf(address.sun_path, sizeof(address.sun_path), "%s", opened_.socket.c_str());
        BOOST_REQUIRE_MESSAGE(::bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0,
                              "Could not bind " << opened_.socket << ": " << std::strerror(errno));
        BOOST_REQUIRE(::listen(listener, 1) == 0);
        add(listener, true);

        // A descriptor that exec closes by itself, which the child must not hold either
        int cloexec = ::open("/dev/null", O_RDONLY);
        BOOST_REQUIRE(cloexec >= 0);
        BOOST_REQUIRE(::fcntl(cloexec, F_SETFD, FD_CLOEXEC) == 0);
        add(cloexec, false);

        // Descriptors at high numbers: 1000, and the highest that the descriptor table allows (bounded, so that
        // the table of the test process stays small where the limit is very large). The size of the table is
        // that of getdtablesize(), which on macOS may be below the soft limit on open files (kern.maxfilesperproc)
        constexpr int bound = 1 << 16;
        const int table     = ::getdtablesize();
        const int highest   = std::min(table, bound) - 1;
        for (int target : {1000, highest}) {
            if (target > opened_.descriptors.back() && target < table) {
                BOOST_REQUIRE(::dup2(pipe_ends[0], target) == target);
                add(target, true);
            }
        }
    }

    InheritableDescriptors(const InheritableDescriptors&)            = delete;
    InheritableDescriptors& operator=(const InheritableDescriptors&) = delete;

    /// @brief The descriptors opened, in increasing order.
    const std::vector<int>& all() const { return opened_.descriptors; }

    /// @brief The descriptors that a process started with exec, and no closing, would hold.
    std::set<int> inheritable() const { return {inheritable_.begin(), inheritable_.end()}; }

private:
    void add(int fd, bool inheritable) {
        BOOST_REQUIRE_MESSAGE(fd >= 0, "Could not open a descriptor: " << std::strerror(errno));
        opened_.descriptors.push_back(fd);
        if (inheritable) {
            inheritable_.push_back(fd);
        }
    }

    // What the fixture opened and created, released by a member, so that it is released even when the constructor
    // fails; the paths are unique to the process, in the temporary directory of the system, so that concurrent runs,
    // and the file system of the working directory, do not interfere
    struct Opened
    {
        Opened()                         = default;
        Opened(const Opened&)            = delete;
        Opened& operator=(const Opened&) = delete;
        ~Opened() {
            for (int fd : descriptors) {
                ::close(fd);
            }
            std::error_code ignored;
            fs::remove(file, ignored);
            fs::remove(socket, ignored);
        }

        std::string file   = unique_path("file");
        std::string socket = unique_path("socket");
        std::vector<int> descriptors;
    };

    static std::string unique_path(const std::string& kind) {
        return (fs::temp_directory_path() / ("ecflow_descriptors_" + std::to_string(::getpid()) + "." + kind)).string();
    }

    Opened opened_;
    std::vector<int> inheritable_;
};

///
/// @brief Spawns a command through System, as the server spawns a job, and waits for it to terminate.
///
void spawn_and_wait(const std::string& cmd) {
    std::string errorMsg;
    BOOST_REQUIRE_MESSAGE(System::instance()->spawn(System::ECF_STATUS_CMD, cmd, "", errorMsg),
                          "System::instance()->spawn() failed: " << errorMsg);
    while (System::instance()->process() != 0) {
        // The child sends SIGCHLD when it terminates, which the parent catches while unblocked
        Signal unblock_on_destruction_then_reblock;
        System::instance()->processTerminatedChildren();
    }
}

///
/// @brief A shell command that writes, to the given file, the descriptors among those given that it holds.
///
/// The command runs in /bin/sh, the shell that the server uses, and checks each descriptor through /dev/fd,
/// which exists on Linux and macOS; the redirections to the file apply to each echo only, so that they do not
/// affect the descriptors being checked.
///
std::string report_held_descriptors(const std::vector<int>& descriptors, const std::string& file) {
    std::ostringstream cmd;
    cmd << ": > " << file << "; for fd in";
    for (int fd : descriptors) {
        cmd << ' ' << fd;
    }
    cmd << "; do if [ -e /dev/fd/$fd ]; then echo $fd >> " << file << "; fi; done";
    return cmd.str();
}

///
/// @brief A shell command that writes, to the given file, which of the descriptors 0, 1 and 2 refer to /dev/null.
///
std::string report_standard_descriptors(const std::string& file) {
    std::ostringstream cmd;
    cmd << ": > " << file << "; for fd in 0 1 2; do "
        << "if [ -e /dev/fd/$fd ]; then "
        << "if [ /dev/fd/$fd -ef /dev/null ]; then echo \"$fd null\" >> " << file << "; "
        << "else echo \"$fd other\" >> " << file << "; fi; "
        << "else echo \"$fd closed\" >> " << file << "; fi; done";
    return cmd.str();
}

std::set<int> read_descriptors(const std::string& file) {
    std::set<int> held;
    std::ifstream in(file);
    for (int fd; in >> fd;) {
        held.insert(fd);
    }
    return held;
}

std::vector<std::string> read_lines(const std::string& file) {
    std::vector<std::string> lines;
    std::ifstream in(file);
    for (std::string line; std::getline(in, line);) {
        lines.push_back(line);
    }
    return lines;
}

std::string to_string(const std::set<int>& descriptors) {
    std::ostringstream out;
    for (int fd : descriptors) {
        out << fd << ' ';
    }
    return out.str();
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_SystemDescriptors)

BOOST_AUTO_TEST_CASE(test_spawned_command_holds_no_inherited_descriptor) {
    ECF_NAME_THIS_TEST();

    InheritableDescriptors descriptors;
    const std::string file = "test_system_descriptors.held";
    const std::string cmd  = report_held_descriptors(descriptors.all(), file);

    // Control: a command started without closing (std::system) holds every descriptor not marked close-on-exec,
    // which shows that the check sees an inherited descriptor
    BOOST_REQUIRE(std::system(cmd.c_str()) == 0);
    BOOST_REQUIRE_MESSAGE(read_descriptors(file) == descriptors.inheritable(),
                          "The check does not see the inherited descriptors: expected "
                              << to_string(descriptors.inheritable()) << ", found "
                              << to_string(read_descriptors(file)));

    // A command spawned as the server spawns a job holds none of them
    spawn_and_wait(cmd);
    BOOST_CHECK_MESSAGE(read_descriptors(file).empty(),
                        "The spawned command holds inherited descriptors: " << to_string(read_descriptors(file)));

    fs::remove(file);
}

BOOST_AUTO_TEST_CASE(test_spawned_command_standard_descriptors_are_dev_null) {
    ECF_NAME_THIS_TEST();

    const std::string file = "test_system_descriptors.standard";
    spawn_and_wait(report_standard_descriptors(file));

    std::vector<std::string> expected{"0 null", "1 null", "2 null"};
    std::vector<std::string> found = read_lines(file);
    BOOST_CHECK_EQUAL_COLLECTIONS(found.begin(), found.end(), expected.begin(), expected.end());

    fs::remove(file);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
