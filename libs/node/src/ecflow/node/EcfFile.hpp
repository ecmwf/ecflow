// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <fstream>
#include <memory>

#include "ecflow/core/Filesystem.hpp"
#include "ecflow/node/NodeFwd.hpp"

namespace ecf {
class VariableLookup;
} // namespace ecf

// This class is used to minimise file I/0.
// When job processing the same include file can be opened and closed many time
// This cache serves to open the include file only once. It halves the job processing time.
class IncludeFileCache {
public:
    explicit IncludeFileCache(const std::string& path);

    // Disable copy (and move) semantics
    IncludeFileCache(const IncludeFileCache&)                  = delete;
    const IncludeFileCache& operator=(const IncludeFileCache&) = delete;
    IncludeFileCache(IncludeFileCache&&)                       = delete;
    IncludeFileCache& operator=(IncludeFileCache&&)            = delete;

    ~IncludeFileCache();

    const std::string& path() const { return path_; }
    bool lines(std::vector<std::string>&);

private:
    std::string path_;
    std::ifstream fp_; // is not copy constructable
    size_t no_of_lines_;
};

/// This class is used in the pre-processing of files( .ecf or .usr or .man typically)
/// It is used to create the job file.
///
///
/// Please note the %manual is only created on request from the file cmd.
/// Even then it is extracted as a string. i.e. no .man file is
/// created. This is left to the client.
/// When returning the manual we pre-process the files first
///
/// However for testing purpose this capability may be retained.

class EcfFile {
public:
    enum Origin {
        ECF_SCRIPT,    // Look for .ecf file, uses default algorithm to find %includes
        ECF_HOME,      // default directory for scripts
        ECF_FILES,     // User specified directory
        ECF_FETCH_CMD, // pre-process output of ECF_FETCH,     all %includes use same command
        ECF_SCRIPT_CMD // pre-process output of ECF_SCRIPT_CMD, uses default algorithm to find %includes
    };

    // Use to record how the ecf_file was located from ECF_FILES and ECF_HOME directories. Default is PRUNE_ROOT
    enum EcfFileSearchAlgorithm { PRUNE_ROOT, PRUNE_LEAF };

    ///
    /// @brief The kind of node whose file is processed, when the file is processed without the node.
    ///
    enum class NodeKind { TASK, ALIAS, FAMILY, SUITE };

    EcfFile();

    /// This preserves the caches, used to avoid opening/stat of include file more than once.
    EcfFile& operator=(const EcfFile& rhs);

    /// use default copy constructor, assignment, destructor
    /// ECF_FETCH  is used obtain the script from running a command  i.e.
    /// from the version control system.
    EcfFile(Node*,
            const std::string& path_to_script_or_fetch_cmd,
            EcfFile::Origin                 = ECF_SCRIPT,
            EcfFile::EcfFileSearchAlgorithm = PRUNE_ROOT);

    ///
    /// @brief Creates the file of a node from its variables only, without the node.
    ///
    /// The file is pre-processed as the server would pre-process it for the node: the includes are expanded and the
    /// variables substituted from @p lookup, and the messages name the node by @p kind and @p abs_node_path. Only
    /// manual() and script() are available; the operations that need the node (create_job(), pre_process(),
    /// edit_used_variables() and pre_process_user_file()) throw std::runtime_error.
    ///
    /// A file created this way never runs a command: the origins ECF_FETCH_CMD and ECF_SCRIPT_CMD, whose script is the
    /// output of a command taken from the variables, are refused.
    ///
    /// @param[in] kind The kind of the node.
    /// @param[in] abs_node_path The absolute path of the node, such as /suite/family/task.
    /// @param[in] lookup The variables as seen from the node, with the generated variables current and the server
    ///                   variables included (such as ECF_MICRO), as MapVariableLookup::from_node gives them; it must
    ///                   not be null.
    /// @param[in] path_to_script_or_cmd The path of the file, or the command that produces it, as located by
    ///                                  ecf::locate_script or ecf::locate_manual_in_ecf_files and
    ///                                  ecf::locate_manual_in_ecf_home.
    /// @param[in] origin Where the file comes from.
    /// @param[in] search_algorithm The search that found the file in ECF_FILES or ECF_HOME.
    /// @throws std::invalid_argument when @p lookup is null, or @p origin is ECF_FETCH_CMD or ECF_SCRIPT_CMD.
    /// @throws std::runtime_error when ECF_MICRO is not a single character.
    ///
    EcfFile(NodeKind kind,
            const std::string& abs_node_path,
            std::shared_ptr<const ecf::VariableLookup> lookup,
            const std::string& path_to_script_or_cmd,
            EcfFile::Origin origin                           = ECF_SCRIPT,
            EcfFile::EcfFileSearchAlgorithm search_algorithm = PRUNE_ROOT);

    // The path to the ecf file, empty path means that ecf file could not be located
    bool valid() const { return !script_path_or_cmd_.empty(); }

    /// This function will return the contents of %manual -> %end for the input file
    /// It will pre-process the file, then extract the manual form all the pre-processed files
    /// Will throw std::runtime_error for errors
    void manual(std::string& theManual);

    /// returns the script, The first line contains the origin of the file/cmd.
    /// Will throw std::runtime_error for errors
    void script(std::string& theScript) const;

    const std::string& script_path_or_cmd() const { return script_path_or_cmd_; }

    /// What is the origin of this ecf_file
    std::string ecf_file_origin_dump() const;

    ///
    /// @brief Gives the kind of a node, as the constructor without a node takes it.
    ///
    /// @param[in] node A task, an alias, a family or a suite.
    /// @return The kind of the node.
    ///
    static NodeKind node_kind(const Node& node);

    ///
    /// @brief Gives the extension of the script of a task or an alias from its variables, as Submittable does.
    ///
    /// For a task, the extension is the user variable ECF_EXTN, as for Task::script_extension, or `.ecf`; for an
    /// alias it is `.usr`; a family or a suite has no script, and an empty extension.
    ///
    /// @param[in] kind The kind of the node.
    /// @param[in] lookup The variables as seen from the node.
    /// @return The extension, for ecf::locate_script.
    ///
    static std::string script_extension(NodeKind kind, const ecf::VariableLookup& lookup);

    /// simple accessors
    Origin ecf_file_origin() const { return script_origin_; }
    EcfFileSearchAlgorithm ecf_file_search_algorithm() const { return ecf_file_search_algorithm_; }
    static std::string origin_str(Origin);
    static std::string search_algorithm_str(EcfFileSearchAlgorithm);

    /// Create the job file from with script of a task or alias.
    /// Note: the location of the ecf file may not be the same as the job file
    ///       For creating the job we must use ECF_JOB
    ///
    /// _IF_ ECF_CLIENT is specified provide Special support for migration. TEST ONLY
    /// The variable ECF_CLIENT is used to specify the path to client exe.
    /// This is then used to replace smsinit,smscomplete, smsevent,smsmeter.smslabel,smsabort
    /// This function will start the pre processing
    /// Will throw std::runtime_error for errors
    const std::string& create_job(JobsParam&);

    /// Process the script file, to add all the used variables, add the start of the file
    /// between %comment %end, The augmented script is returned in file_with_used_variables
    /// Will throw std::runtime_error for errors
    void edit_used_variables(std::string& file_with_used_variables);

    /// Pre-processing involves include expansion and variable substitution
    /// Will throw std::runtime_error if pre processing fails
    void pre_process(std::string& pre_processed_file);

    /// Does full pre-processing:
    ///   - include pre-processing
    ///   - variable substitution
    ///   - remove comment and manual sections
    ///   - ignore %nopp
    void pre_process_user_file(std::vector<std::string>& user_edit_file, std::string& pre_processed_file);

    /// Searches for the first %comment, then extracts all variables of the form
    ///   <name>  =  <value>
    /// and places into map
    static void extract_used_variables(NameValueMap& used_variables_as_map,
                                       const std::vector<std::string>& script_lines);

    static std::vector<std::string> get_ecf_include_paths(const EcfFile& ecf);

private:
    friend class PreProcessor;
    enum Type { SCRIPT, INCLUDE, MANUAL, COMMENT };
    static std::string fileType(EcfFile::Type);

    bool open_script_file(const std::string& file,
                          EcfFile::Type,
                          std::vector<std::string>& lines,
                          std::string& errormsg) const;
    bool open_include_file(const std::string& file, std::vector<std::string>& lines, std::string& errormsg) const;

    bool replaceSmsChildCmdsWithEcf(const std::string& clientPath, std::string& errormsg);
    void variableSubstitution(const JobsParam&);
    const std::string& doCreateJobFile(JobsParam&) const;
    bool doCreateManFile(std::string& errormsg);
    bool extractManual(const std::vector<std::string>& lines,
                       std::vector<std::string>& theManualLines,
                       std::string& errormsg) const;

    void remove_comment_manual_and_nopp_tokens();

    static int countEcfMicro(const std::string& line, const std::string& ecfMicro);
    static void dump_expanded_script_file(const std::vector<std::string>& lines); // for DEBUG

    /// returns the extension, i.e for task->.ecf for alias->.usr, will throw if the node is not task or alias
    const std::string& get_extn() const;

    bool
    do_popen(const std::string& the_cmd, EcfFile::Type, std::vector<std::string>& lines, std::string& errormsg) const;

    fs::path file_creation_path() const;
    std::string script_or_job_path() const;

    bool extract_ecfmicro(const std::string& line, std::string& ecfmicro, std::string& error_msg) const;

    /// User edit functions:
    void get_used_variables(std::string& used_variables) const;
    bool get_used_variables(NameValueMap& used_variables, std::string& errorMsg) const;
    void doCreateUsrFile() const;
    bool file_exists(const std::string& file_path) const;

    ///
    /// @brief Reads ECF_MICRO into the cache, checking that it is a single character.
    ///
    /// @throws std::runtime_error when ECF_MICRO is not a single character.
    ///
    void init_micro();

    ///
    /// @brief Gives the node, for the operations that need it.
    ///
    /// @param[in] operation The name of the operation, for the message.
    /// @return The node.
    /// @throws std::runtime_error when the file was created from variables only, without the node.
    ///
    Node& node(const char* operation) const;

    /// Whether the node is a task or an alias, rather than a family or a suite
    [[nodiscard]] bool is_submittable() const { return kind_ == NodeKind::TASK || kind_ == NodeKind::ALIAS; }

    ///
    /// @brief Gives the absolute path of the node: the current one of the node when there is one, else as given.
    ///
    /// @return The path.
    ///
    [[nodiscard]] std::string abs_node_path() const;

    ///
    /// @brief Gives the name of the node: from the node when there is one, else the last part of the path given.
    ///
    /// @return The name.
    ///
    [[nodiscard]] std::string node_name() const;

    ///
    /// @brief Gives the kind and path of the node, such as `TASK:/suite/task`, as Node::debugNodePath does.
    ///
    /// @return The kind and path, for messages.
    ///
    [[nodiscard]] std::string debug_node_path() const;

    Node* node_{nullptr}; // Task or Alias or Container when pre-processing the man files; null when created from
                          // variables only
    std::shared_ptr<const ecf::VariableLookup> lookup_; // The variables as seen from the node
    NodeKind kind_{NodeKind::TASK};                     // The kind of the node
    std::string abs_node_path_;                         // The absolute path of the node; only used without a node
    std::string extension_;             // The extension of the script, only used without a node; empty for a container
    std::string ecfMicroCache_;         // cache value of ECF_MICRO
    std::string script_path_or_cmd_;    // path to .ecf, .usr file or command
    std::vector<std::string> jobLines_; // Lines that will form the job file.

    mutable std::vector<std::shared_ptr<IncludeFileCache>> include_file_cache_; // only open include file once
    mutable std::vector<std::pair<std::string, bool>> file_stat_cache_;         // Minimise calls to stat/kernel calls
    mutable std::string job_size_;                       // to be placed in log file during job submission
    EcfFile::Origin script_origin_{EcfFile::ECF_SCRIPT}; // get script from a file, or from running a command
    EcfFile::EcfFileSearchAlgorithm ecf_file_search_algorithm_{
        EcfFile::PRUNE_ROOT}; // only used for ECF_FILES and ECF_HOME
};

// This class is used in expanding(pre-processing) the includes.
// The pre-processing is done in a depth first fashion (ECFLOW-673)
class PreProcessor {
public:
    explicit PreProcessor(EcfFile*, const char* error_context);
    // Disable copy (and move) semantics
    PreProcessor(const PreProcessor&)                  = delete;
    const PreProcessor& operator=(const PreProcessor&) = delete;

    ~PreProcessor();

    void preProcess(std::vector<std::string>& script_lines);

private:
    std::string error_context() const;

    // include pre-processing on the included file.
    // Note: include directives _in_ manual/comment should he handled.
    //       only include directives in %nopp/%end are ignored
    void preProcess_line();
    void preProcess_includes();
    std::string getIncludedFilePath(const std::string& include, const std::string& line);

private:
    EcfFile* ecfile_;
    const char* error_context_;

    std::string pp_nopp_;
    std::string pp_comment_;
    std::string pp_manual_;
    std::string pp_end_;
    std::string ecf_micro_; // constant until ecfmicro changes, then reset

    std::vector<std::string>& jobLines_;

    std::vector<std::pair<std::string, int>>
        globalIncludedFileSet_; // test for recursive includes, <no _of times it was included>
    std::vector<std::string> include_once_set_;

    bool nopp_{false};
    bool comment_{false};
    bool manual_{false};
};
