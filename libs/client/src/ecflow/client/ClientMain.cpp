/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <cstdio>
#include <sys/wait.h>

#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/CommandLine.hpp"

static std::string json_escape(const std::string& input) {
    std::string out;
    out.reserve(input.size() + 16);
    for (char c : input) {
        switch (c) {
            case '\\':
                out += "\\\\";
                break;
            case '"':
                out += "\\\"";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                out += c;
                break;
        }
    }
    return out;
}

static std::string join_args(const std::vector<std::string>& args) {
    std::ostringstream ss;
    for (size_t i = 1; i < args.size(); ++i) {
        if (i > 1) {
            ss << ' ';
        }
        ss << args[i];
    }
    return ss.str();
}

static std::string shell_quote(const std::string& value) {
    std::string quoted;
    quoted.reserve(value.size() + 2);
    quoted += '\'';
    for (char c : value) {
        if (c == '\'') {
            quoted += "'\\''";
        }
        else {
            quoted += c;
        }
    }
    quoted += '\'';
    return quoted;
}

static bool requires_early_cli_exit(const std::vector<std::string>& args) {
    for (size_t i = 1; i < args.size(); ++i) {
        const std::string& arg = args[i];
        if (arg == "--version" || arg == "--help" || arg.rfind("--help=", 0) == 0) {
            return true;
        }
    }
    return false;
}

static int run_and_capture(const std::vector<std::string>& args, std::string& output) {
    std::ostringstream cmd;
    for (size_t i = 0; i < args.size(); ++i) {
        if (i) {
            cmd << ' ';
        }
        cmd << shell_quote(args[i]);
    }
    cmd << " 2>&1";

    FILE* pipe = popen(cmd.str().c_str(), "r");
    if (!pipe) {
        output = "Failed to launch subprocess for help/version";
        return 1;
    }

    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }

    int status = pclose(pipe);
    if (status == -1) {
        return 1;
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return 1;
}

static void print_json_response(bool ok,
                                int exit_code,
                                const std::vector<std::string>& args,
                                const std::string& reply,
                                const std::string& error) {
    std::cout << "{";
    std::cout << "\"ok\":" << (ok ? "true" : "false") << ",";
    std::cout << "\"exit_code\":" << exit_code << ",";
    std::cout << "\"command\":\"" << json_escape(join_args(args)) << "\",";
    std::cout << "\"reply\":\"" << json_escape(reply) << "\",";
    std::cout << "\"error\":\"" << json_escape(error) << "\"";
    std::cout << "}" << std::endl;
}

int main(int argc, char* argv[]) {

    bool json_output_mode = false;
    std::vector<std::string> args;
    args.reserve(static_cast<size_t>(argc));
    for (int i = 0; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--json") {
            json_output_mode = true;
            continue;
        }
        args.push_back(arg);
    }

    if (json_output_mode && requires_early_cli_exit(args)) {
        std::string subprocess_output;
        int rc = run_and_capture(args, subprocess_output);
        print_json_response((rc == 0), rc, args, subprocess_output, (rc == 0 ? "" : subprocess_output));
        return rc;
    }

    /// By default, error condition will throw exception.
    try {
        ClientInvoker client;
        client.set_cli(!json_output_mode); // avoid non-JSON command output in JSON mode
        int rc = client.invoke(CommandLine(args));
        if (json_output_mode) {
            print_json_response((rc == 0), rc, args, client.get_string(), client.errorMsg());
        }
        return rc;
    }
    catch (std::exception& e) {
        if (json_output_mode) {
            print_json_response(false, 1, args, "", e.what());
            return 1;
        }
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
