/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "DirectoryHandler.hpp"

#include <regex>

#include <QFile>
#include <boost/foreach.hpp>

#include "UiLog.hpp"
#include "UserMessage.hpp"
#include "ecflow/core/File.hpp"

std::string DirectoryHandler::exeDir_;
std::string DirectoryHandler::shareDir_;
std::string DirectoryHandler::etcDir_;
std::string DirectoryHandler::configDir_;
std::string DirectoryHandler::logviewerConfigDir_;
std::string DirectoryHandler::rcDir_;
std::string DirectoryHandler::tmpDir_;
std::string DirectoryHandler::uiLogFile_;
std::string DirectoryHandler::uiEventLogFile_;
std::string DirectoryHandler::socketDir_;

static bool firstStartUp = false;

DirectoryHandler::DirectoryHandler() = default;

// -----------------------------------------------------------------------------
// DirectoryHandler::init
// We set all the directory paths containing any of the config files used by the viewer.
// If we tell this class where the executable started from, then it can work out
// where all the other directories are
// -----------------------------------------------------------------------------

void DirectoryHandler::init(const std::string& exeStr) {
    fs::path configDir;

    // The location of the config dir is specified by the user
    // This could be the case for a ui test! In this case we
    // must not use the rcDir!
    if (char* confCh = getenv("ECFLOWUI_CONFIG_DIR")) {
        std::string confStr(confCh);
        configDir = fs::path(confStr);
    }
    // By default the config dir is .ecflow_ui_v5 in $HOME,
    // in this case we might import older settings from $HOME/.ecflowrc
    else if (char* h = getenv("HOME")) {
        std::string home(h);
        fs::path homeDir(home);

        configDir = fs::path(homeDir);
        configDir /= ".ecflow_ui_v5";

        fs::path rcDir = homeDir;
        rcDir /= ".ecflowrc";
        rcDir_ = rcDir.string();
    }

    // Sets configDir_ and creates it if it does not exist
    if (!configDir.string().empty()) {
        configDir_ = configDir.string();
        if (!fs::exists(configDir)) {
            firstStartUp = true;

            try {
                fs::create_directory(configDir);
            }
            catch (const fs::filesystem_error& err) {
                UserMessage::message(
                    UserMessage::ERROR,
                    true,
                    std::string("Could not create configDir: " + configDir_ + " reason: " + err.what()));
                exit(1);
            }
        }
        else {
            // the config dir was copied from v4
            if (char* confCh = getenv("ECFLOWUI_CONFIG_DIR_COPIED"); confCh) {
                firstStartUp = true;
            }
        }
    }
    else {
        UserMessage::message(UserMessage::ERROR, true, std::string("Could not create configDir with an empty path!"));
        exit(1);
    }

    fs::path lgvConfigDir = configDir;
    lgvConfigDir /= "logviewer";
    logviewerConfigDir_ = lgvConfigDir.string();
    if (!fs::exists(lgvConfigDir)) {
        try {
            fs::create_directory(lgvConfigDir);
        }
        catch (const fs::filesystem_error& err) {
            UserMessage::message(
                UserMessage::ERROR,
                true,
                std::string("Could not create logviewer configDir: " + logviewerConfigDir_ + " reason: " + err.what()));
            exit(1);
        }
    }

    // Sets paths in the system directory
    fs::path exePath(exeStr);

    // If the executable path does not exist we
    // will use  the value of the ECFLOW_SHARED_DIR macro to get
    // the location of the "share/ecflow" dir.
    if (!fs::exists(exePath)) {
        fs::path shareDir(ECFLOW_SHARED_DIR);
        if (!fs::exists(shareDir)) {
            UserMessage::message(
                UserMessage::ERROR,
                true,
                std::string("Shared dir " + shareDir.string() + " does not exist! EcflowUI cannot be launched!"));
            exit(0);
        }

        fs::path etcDir = shareDir;
        etcDir /= "etc";

        shareDir_ = shareDir.string();
        etcDir_   = etcDir.string();
    }
    // If the executable path exits we probably use relative paths to it.
    else {
        exeDir_ = exePath.parent_path().string();

        // TODO: make it work when we run it from within "bin"
        fs::path shareDir = exePath.parent_path().parent_path();
        shareDir /= "share";
        shareDir /= "ecflow";

        // In some debugging environments the exe might be another level deeper
        if (!fs::exists(shareDir)) {
            shareDir = exePath.parent_path().parent_path().parent_path();
            shareDir /= "share";
            shareDir /= "ecflow";
        }

        fs::path etcDir = shareDir;
        etcDir /= "etc";

        shareDir_ = shareDir.string();
        etcDir_   = etcDir.string();
    }

    // Tmp dir
    if (char* h = getenv("ECFLOWUI_TMPDIR")) {
        tmpDir_ = std::string(h);
    }
    else if (char* h = getenv("TMPDIR")) {
        tmpDir_ = std::string(h);
        fs::path tmp(tmpDir_);
        tmp /= "ecflow_ui.tmp";
        tmpDir_ = tmp.string();
        if (!fs::exists(tmp)) {
            UiLog().warn()
                << "ECFLOWUI_TMPDIR env variable is not defined. ecFlowUI creates its tmp direcoty in TMPDIR as "
                << tmp.string();

            try {
                if (fs::create_directory(tmp)) {
                    UiLog().dbg() << "Tmp dir created: " << tmpDir_;
                }
            }
            catch (const fs::filesystem_error& e) {
                UserMessage::message(
                    UserMessage::ERROR, true, "Creating tmp directory failed:" + std::string(e.what()));
            }
        }
    }
    else {
        UserMessage::message(UserMessage::ERROR,
                             true,
                             "Neither of ECFLOWUI_TMPDIR and TMPDIR are defined. ecflowUI cannot be started up!");
        exit(1);
    }

    // Ui log. The ui logging either goes into the stdout or into a
    // file. The startup script desides on it.
    if (char* h = getenv("ECFLOWUI_LOGFILE")) {
        uiLogFile_ = std::string(h);
    }

    // Ui event log file. Ui event logging always goes into a file
    if (char* h = getenv("ECFLOWUI_UI_LOGFILE")) {
        uiEventLogFile_ = std::string(h);
    }
    else {
        fs::path tmp(tmpDir_);
        tmp /= "ecflowui_uilog.txt";
        uiEventLogFile_ = tmp.string();
    }

    // Ui event log file. Ui event logging always goes into a file
    if (char* h = getenv("ECFLOWUI_SOCKETDIR")) {
        socketDir_ = std::string(h);
    }
    else {
        fs::path tmp(tmpDir_);
        // tmp /= "sockets";
        socketDir_ = tmp.string();
    }
}

std::string DirectoryHandler::concatenate(const std::string& path1, const std::string& path2) {
    fs::path p1(path1);
    fs::path p2(path2);
    fs::path result = p1 /= p2;
    return result.string();
}

void DirectoryHandler::findDirContents(const std::string& dirPath,
                                       const std::string& filterStr,
                                       FileType type,
                                       std::vector<std::string>& res) {
    fs::path path(dirPath);
    fs::directory_iterator it(path), eod;

    const std::regex expr(filterStr);

    BOOST_FOREACH (fs::path const& p, std::make_pair(it, eod)) {
        std::smatch what;
        std::string fileName = p.filename().string();

        bool rightType = (type == File) ? is_regular_file(p) : is_directory(p); // file or directory?

        if (rightType && std::regex_match(fileName, what, expr)) {
            res.push_back(fileName);
        }
    }
}

void DirectoryHandler::findFiles(const std::string& dirPath,
                                 const std::string& filterStr,
                                 std::vector<std::string>& res) {
    findDirContents(dirPath, filterStr, File, res);
}

void DirectoryHandler::findDirs(const std::string& dirPath,
                                const std::string& filterStr,
                                std::vector<std::string>& res) {
    findDirContents(dirPath, filterStr, Dir, res);
}

bool DirectoryHandler::createDir(const std::string& path) {
    // Create configDir if if does not exist
    if (!fs::exists(path)) {
        try {
            fs::create_directory(path);
        }
        catch (const fs::filesystem_error& err) {
            UserMessage::message(UserMessage::ERROR, true, "Could not create dir: " + path + " reason: " + err.what());
            return false;
        }
    }

    return true; // will also return true if the directory already exists
}

bool DirectoryHandler::isFirstStartUp() {
    return firstStartUp;
}

// Return a unique non-existing tmp filename
std::string DirectoryHandler::tmpFileName() {
    fs::path tmp(tmpDir_);
    if (fs::exists(tmp)) {
        try {
            fs::path model = tmp;
            model /= "%%%%-%%%%-%%%%-%%%%";
            return ecf::fsx::unique_path(model).string();
        }
        catch (const fs::filesystem_error& err) {
            UiLog().warn() << "Could not generate tmp filename! Reason: " << err.what();
        }
    }

    return {};
}

// -----------------------------------------------------
// copyDir
// recursively copy a directory and its contents
// -----------------------------------------------------

bool DirectoryHandler::copyDir(const std::string& srcDir, const std::string& destDir, std::string& errorMessage) {
    fs::path src(srcDir);
    fs::path dest(destDir);

    // does the source directory exist (and is a directory)?
    if (!fs::exists(src) || !fs::is_directory(src)) {
        errorMessage = "Source directory (" + srcDir + ") does not exist";
        return false;
    }

    // create the destination directory if it does not already exist
    if (!fs::exists(dest)) {
        bool created = createDir(dest.string());
        if (!created) {
            errorMessage = "Could not create destination directory (" + destDir + ")";
            return false;
        }
    }

    // go through all the files/dirs in the dir
    bool ok = true;
    fs::directory_iterator it(src), eod;
    BOOST_FOREACH (fs::path const& p, std::make_pair(it, eod)) {
        std::string fileName = p.filename().string();
        std::string srcFile  = p.string();

        if (is_regular_file(p)) // file? then copy it into its new home
        {
            // The original boost based copy implementation did not work with newer compilers,
            // so we opted for a Qt based implementation. See ECFLOW-1207
            fs::path destPath    = dest / p.filename();
            std::string destFile = destPath.string();
            if (!copyFile(srcFile, destFile, errorMessage)) {
                return false;
            }

#if 0
            try
            {
                fs::copy_file(p, dest / p.filename());
            }
            catch (const fs::filesystem_error& err)
            {
                errorMessage = "Could not copy file " + fileName + " to " + destDir + "; reason: " + err.what();
                return false;
            }
#endif
        }
        else if (is_directory(p)) // directory? then copy it recursively
        {
            fs::path destSubDir(destDir);
            destSubDir /= p.filename();
            ok = ok && copyDir(p.string(), destSubDir.string(), errorMessage);
        }
    }

    return ok;
}

// --------------------------------------------------------
// removeDir
// Recursively removes the given directory and its contents
// --------------------------------------------------------

bool DirectoryHandler::removeDir(const std::string& dir, std::string& errorMessage) {
    try {
        fs::path d(dir);
        remove_all(d);
    }
    catch (const fs::filesystem_error& err) {
        errorMessage = "Could not remove directory " + dir + "; reason: " + err.what();
        return false;
    }

    return true;
}

// --------------------------------------------------------
// renameDir
// Same as a 'move' - renames the directory
// --------------------------------------------------------

bool DirectoryHandler::renameDir(const std::string& dir, const std::string& newName, std::string& errorMessage) {
    try {
        fs::path d1(dir);
        fs::path d2(newName);
        rename(d1, d2);
    }
    catch (const fs::filesystem_error& err) {
        errorMessage = "Could not rename directory " + dir + "; reason: " + err.what();
        return false;
    }

    return true;
}

// -------------------------------------------------------------
// copyFile
// Copies the given file to the given destintation (full paths).
// -------------------------------------------------------------

bool DirectoryHandler::copyFile(const std::string& srcFile, std::string& destFile, std::string& errorMessage) {
    // The original boost based copy implementation did not work with newer compilers,
    // so we opted for a Qt based implementation. See ECFLOW-1207

    if (srcFile == destFile) {
        return true;
    }

    QString src  = QString::fromStdString(srcFile);
    QString dest = QString::fromStdString(destFile);

    if (!QFile::exists(src)) {
        errorMessage = "Could not copy file " + srcFile + " to " + destFile + "; reason: source file does not exist";
        return false;
    }

    if (QFile::exists(dest)) {
        QFile::remove(dest);
    }
    if (!QFile::copy(src, dest)) {
        errorMessage = "Could not copy file " + srcFile + " to " + destFile;
        return false;
    }
    return true;

#if 0
    fs::path src(srcFile);
    fs::path dest(destFile);

    try
    {
        fs::copy_file(src, dest,  fs::copy_option::overwrite_if_exists);
    }
    catch (const fs::filesystem_error& err)
    {
        errorMessage = "Could not copy file " + srcFile + " to " + destFile + "; reason: " + err.what();
        return false;
    }

    return true;
#endif
}

// --------------------------------------------------------
// removeFile
// Removes the given file
// --------------------------------------------------------

bool DirectoryHandler::removeFile(const std::string& path, std::string& errorMessage) {
    try {
        fs::path f(path);
        remove(f);
    }
    catch (const fs::filesystem_error& err) {
        errorMessage = "Could not remove file " + path + "; reason: " + err.what();
        return false;
    }

    return true;
}

bool DirectoryHandler::truncateFile(const std::string& path, int lastLineNum, std::string& errorMessage) {
    std::string s = ecf::File::get_last_n_lines(path, lastLineNum, errorMessage);
    if (!errorMessage.empty()) {
        errorMessage = "Could not truncate file " + path + "; reason: " + errorMessage;
        return false;
    }

    if (!ecf::File::create(path, s, errorMessage)) {
        errorMessage = "Could not truncate file " + path + "; reason: " + errorMessage;
        return false;
    }

    return true;
}
