// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ecflow/core/Message.hpp"

///
/// @file Log.hpp
/// @brief The server log: a process-wide singleton that writes one time-stamped record per line.
///
/// Every record written to the log has the form `XXX:[HH:MM:SS D.M.YYYY] <message>`, where `XXX` is one of the
/// Log::LogType markers. A consumer of the log (the log server, the viewer, site scripts) relies on one record per
/// line, so the writers below are designed around that property.
///
/// The class is split in two layers:
///
/// - Log is the singleton and the public API. It owns the file name, the write-failure recovery, the cached
///   time stamp and the lock that serialises all writers.
///
/// - LogImpl owns the actual `std::ofstream`. It is created lazily, and destroyed whenever the file must be closed
///   (Log::flush, Log::new_path, Log::clear, or after a write failure). Closing the file is the strongest hint the
///   operating system accepts that the data must reach the physical medium; a plain `flush()` on the stream is not
///   enough because of file caching. This matters for tests, which clear and copy the log file between runs, and
///   for the log server, which reads the file while the server writes it.
///
/// There is a single writer, Log::log, which always writes complete, terminated records. Nothing can leave a line
/// open, so a record can never be glued onto a previous one.
///

#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace ecf {

class LogImpl;

///
/// @brief Provides the process-wide log file, written as one time-stamped record per line.
///
/// The Log is a singleton, created once by the server (Log::create) and destroyed at exit (Log::destroy). All
/// writers are serialised by an internal recursive mutex, so the log can be written from any thread. The underlying
/// file is opened lazily and closed whenever the contents must be guaranteed on disk; see the file comment for the
/// rationale.
///
/// @invariant At most one instance exists at any time; Log::instance() is null before Log::create/after Log::destroy.
/// @see LogImpl, LogFlusher, LogToCout
///
class Log {
public:
    ///
    /// @brief Enumerates the record markers that introduce every log record.
    ///
    /// The marker is written as its three-letter name followed by a colon, for example `MSG:`.
    ///
    enum LogType {
        MSG, ///< A request received by the server (user or child command), or a server-generated message
        LOG, ///< A change of node state
        ERR, ///< An error, including a refused request
        WAR, ///< A warning
        DBG, ///< Debug output
        OTH  ///< Anything else
    };

    ///
    /// @brief Creates the singleton, bound to the given log file.
    ///
    /// The file is not opened until the first write. Calling this function when an instance already exists has no
    /// effect; the existing instance and its file name are kept.
    ///
    /// @param[in] filename Path of the log file; relative paths are resolved against the current working directory
    ///                     at the time of each open.
    ///
    static void create(const std::string& filename);

    ///
    /// @brief Flushes, closes and destroys the singleton.
    ///
    /// Safe to call when no instance exists. After this call Log::instance() returns null, and the free function
    /// ecf::log falls back to standard output (when LogToCout is active) or discards its message.
    ///
    static void destroy();

    ///
    /// @brief Returns the singleton, or null when none has been created.
    ///
    /// @return A non-owning pointer to the instance, or nullptr.
    ///
    static Log* instance() { return instance_; }

private:
    explicit Log(const std::string& fileName);

public:
    // Disable copy (and move) semantics
    Log(const Log&)            = delete;
    Log& operator=(const Log&) = delete;
    Log(Log&&)                 = delete;
    Log& operator=(Log&&)      = delete;

    ///
    /// @brief Writes a complete record, terminated by a newline.
    ///
    /// The record is written as `XXX:[HH:MM:SS D.M.YYYY] <message>`.
    ///
    /// When @p message contains newlines it is split into one record per line, each with the same marker and time
    /// stamp; empty lines are dropped, so a message consisting only of newlines writes nothing (see
    /// LogImpl::log).
    ///
    /// The time stamp is not the time of the write. MSG, LOG and OTH records carry the cached *request time*, the
    /// stamp last set by cache_time_stamp(), so that every record of one request carries the same stamp; ERR, WAR
    /// and DBG records carry the *current time*, and that refresh also replaces the cached stamp, so the MSG, LOG
    /// and OTH records that follow within the same request carry the time of the error rather than that of the
    /// request. The cache is also filled with the current time when it is empty, that is, for the first record
    /// after the file is opened.
    ///
    /// Consequently a MSG, LOG or OTH record written outside the paths that refresh the cache (see
    /// cache_time_stamp()) is stamped with the time of the last request or scheduler tick, however long ago that
    /// was.
    ///
    /// If the file is currently closed it is opened first.
    ///
    /// @param[in] lt      The record marker.
    /// @param[in] message The record body; may span several lines.
    /// @return true when the write succeeded; false when it failed, in which case the failure is described by
    ///         log_error(), the file has been closed and reopened, an ERR record describing the failure has been
    ///         written, and the original record has been written again.
    ///
    bool log(LogType, const std::string& message);

    ///
    /// @brief Sets the time stamp reused by subsequent MSG, LOG and OTH records to the current time.
    ///
    /// The stamp is a request time, not a write time, and is intended to be refreshed at three points only:
    ///  - once per client request, before the request is logged (ClientToServerCmd::handleRequest);
    ///  - when a task changes state outside any request, during tree traversal (Node::setStateOnly), so that the
    ///    resulting LOG records do not carry the time of the previous request;
    ///  - and, before the periodic check point is logged (CheckPtSaver).
    /// Any other MSG, LOG or OTH record reuses whatever stamp those calls last set.
    ///
    /// ERR, WAR or DBG records also replace the cached stamp with the current time as a side effect of being written.
    ///
    /// If the file is closed it is opened first.
    ///
    void cache_time_stamp();

    ///
    /// @brief Returns the time stamp last set by cache_time_stamp().
    ///
    /// Used to stamp the edit history of nodes with the same time as the log record of the request.
    ///
    /// @return The cached time stamp, or the empty string when the file is closed.
    ///
    const std::string& get_cached_time_stamp() const;

    ///
    /// @brief Returns the default number of lines requested by contents().
    ///
    /// @return 100.
    ///
    static int get_last_n_lines_default() { return 100; }

    ///
    /// @brief Returns the first or last lines of the log file.
    ///
    /// The file is flushed and closed before it is read, so the returned text is complete up to the last write.
    ///
    /// @param[in] get_last_n_lines A positive value returns that many lines from the end of the file; a negative
    ///                             value returns that many lines from the start; zero returns the empty string.
    /// @return The requested lines, or the empty string when the file cannot be read.
    ///
    std::string contents(int get_last_n_lines);

    ///
    /// @brief Flushes and closes the log file.
    ///
    /// Closing the file is the strongest available hint to the operating system that the contents must be written
    /// to the physical medium. The file is reopened, in append mode, by the next write.
    ///
    void flush();

    ///
    /// @brief Flushes the log file without closing it.
    ///
    /// Cheaper than flush(); used after every request (see LogFlusher) so that a reader of the log file sees the
    /// records of a request as soon as it is handled.
    ///
    void flush_only();

    ///
    /// @brief Truncates the log file to zero length.
    ///
    /// The file is closed first. Intended for tests.
    ///
    void clear();

    ///
    /// @brief Closes the current log file and directs subsequent writes to a new path.
    ///
    /// The current file is flushed and closed; the new file is opened, in append mode, by the next write.
    ///
    /// @param[in] the_new_path Path of the new log file; must not be empty, must not name a directory, and any
    ///                         directory part must already exist.
    /// @throws std::runtime_error when @p the_new_path does not satisfy the conditions above; the current log file
    ///         is left in place.
    ///
    void new_path(const std::string& the_new_path);

    ///
    /// @brief Returns the absolute path of the log file.
    ///
    /// A relative file name is resolved against the current working directory at the time of the call.
    ///
    /// @return The absolute path.
    ///
    std::string path() const;

    ///
    /// @brief Returns the description of the last failure to open or write the log file.
    ///
    /// @return The failure description, or the empty string when no failure has occurred.
    ///
    const std::string& log_error() const { return log_error_; }

    ///
    /// @brief Appends the names of all record markers to the given vector.
    ///
    /// @param[out] vec Receives, in order: MSG, LOG, ERR, WAR, DBG, OTH.
    ///
    static void get_log_types(std::vector<std::string>&);

private:
    // Handles a failed write: describes the failure, closes and reopens the file, and returns the description.
    std::string handle_write_failure();

    // Verifies that new_path is neither empty nor a directory, and that its parent directory exists.
    // Throws std::runtime_error otherwise.
    static void check_new_path(const std::string& new_path);

    // Opens the file, if it is closed.
    void create_logimpl();

private:
    static Log* instance_;

    std::unique_ptr<LogImpl> logImpl_;
    std::string fileName_;
    std::string log_error_;

    mutable std::recursive_mutex mx_;
};

///
/// @brief Flushes the log on destruction.
///
/// Declared at the start of a scope that handles a request, so that the records written while handling it reach the
/// file when the scope ends, whichever way it ends. Calls Log::flush_only(), so the file stays open. Does nothing
/// when no Log instance exists.
///
/// @see Log::flush_only
///
class LogFlusher {
public:
    LogFlusher() = default;

    // Disable copy (and move) semantics
    LogFlusher(const LogFlusher&)                  = delete;
    const LogFlusher& operator=(const LogFlusher&) = delete;

    ~LogFlusher();
};

///
/// @brief Owns the open log file and performs the actual writes.
///
/// A LogImpl is created by Log when the file must be open and destroyed when the file must be closed, so that
/// closing the file, the strongest hint to the operating system that the data must reach the physical medium, is
/// always possible. The class performs no locking and no failure recovery; both are the responsibility of Log.
///
/// The file is opened in append mode, so a LogImpl created after another one was destroyed continues at the end
/// of the file.
///
/// @invariant Between create_time_stamp() calls, every record written with MSG, LOG or OTH carries the same time
///            stamp; writing an ERR, WAR or DBG record counts as a create_time_stamp() call.
/// @see Log
///
class LogImpl {
public:
    ///
    /// @brief Opens the given file in append mode.
    ///
    /// Failure to open is not an error: it is recorded and returned by log_open_error(), so that the server can
    /// continue and report the problem, rather than die.
    ///
    /// @param[in] filename Path of the log file.
    ///
    explicit LogImpl(const std::string& filename);

    // Disable copy (and move) semantics
    LogImpl(const LogImpl&)                  = delete;
    const LogImpl& operator=(const LogImpl&) = delete;

    ~LogImpl();

    ///
    /// @brief Writes the marker, the time stamp and the message, as one or more terminated records.
    ///
    /// The time stamp is refreshed to the current time for ERR, WAR and DBG, and when none has been cached yet;
    /// otherwise the cached one is reused, whatever its age. A refresh replaces the cache, so it also affects the
    /// MSG, LOG and OTH records written afterwards.
    ///
    /// A message without newlines is written as a single record, `XXX:[stamp] <message>`. A message with newlines
    /// is split at every newline, using ecf::algorithm::split_at, and each piece is written as a separate record
    /// with the same marker and time stamp. The split drops empty pieces and ignores leading and trailing newlines,
    /// so
    ///   - `"a\n"` yields one record,
    ///   - `"a\n\nb"` yields two records with no blank line between them,
    ///   - and, a message consisting only of newlines yields no output at all
    ///     (the write counter is still incremented, so the next flush() flushes the stream).
    ///
    /// @param[in] lt      The record marker.
    /// @param[in] message The record body, possibly spanning several lines.
    /// @return true when the stream is still good after the write.
    ///
    bool log(Log::LogType lt, const std::string& message);

    ///
    /// @brief Sets the cached time stamp to the current time.
    ///
    void create_time_stamp();

    ///
    /// @brief Returns the cached time stamp, as written in the records.
    ///
    /// @return The time stamp, formatted as `[HH:MM:SS D.M.YYYY] `; empty until create_time_stamp() is first
    ///         called.
    ///
    const std::string& get_cached_time_stamp() const { return time_stamp_; }

    ///
    /// @brief Flushes the stream, when anything has been written since the last flush.
    ///
    void flush();

    ///
    /// @brief Returns the underlying stream, for error diagnosis.
    ///
    /// @return The output stream.
    ///
    const std::ofstream& stream() const { return file_; }

    ///
    /// @brief Returns the description of the failure to open the file, if any.
    ///
    /// @return The failure description, or the empty string when the file was opened.
    ///
    const std::string& log_open_error() const { return log_open_error_; }

private:
    std::string time_stamp_;
    std::string log_type_and_time_stamp_; // re-use memory
    std::string log_open_error_;
    mutable std::ofstream file_;
    unsigned int count_{0}; // writes since the last flush
};

///
/// @brief Mirrors log messages to standard output while an instance is alive.
///
/// A debugging aid: while any LogToCout object exists, the free function ecf::log also prints its message to
/// standard output when no Log instance exists, and write-failure diagnostics are printed as well. The flag is
/// global and not reference counted, so the destruction of one instance disables the mirroring for all.
///
class LogToCout {
public:
    LogToCout() { flag_ = true; }

    // Disable copy (and move) semantics
    LogToCout(const LogToCout&)                  = delete;
    const LogToCout& operator=(const LogToCout&) = delete;

    ~LogToCout() { flag_ = false; }

    ///
    /// @brief Reports whether mirroring to standard output is active.
    ///
    /// @return true while at least one LogToCout object has been constructed and none destroyed.
    ///
    static bool ok() { return flag_; }

private:
    static bool flag_;
};

///
/// @brief Writes a complete record to the log, if one exists.
///
/// Convenience wrapper over Log::log. When no Log instance exists, the message goes to standard output if
/// LogToCout is active, and is discarded otherwise.
///
/// @param[in] lt      The record marker.
/// @param[in] message The record body.
/// @return The result of Log::log, or true when no Log instance exists.
///
bool log(Log::LogType, const std::string& message);

///
/// @brief Reports a failed assertion and terminates the process when a log exists.
///
/// The failure is always printed to standard error. When a Log instance exists, it is also written as an ERR
/// record and the process exits with status 1; without a Log instance execution continues. Normally invoked through
/// the LOG_ASSERT macro.
///
/// @param[in] expr    Text of the asserted expression.
/// @param[in] file    Source file of the assertion.
/// @param[in] line    Source line of the assertion.
/// @param[in] message Additional context for the failure.
///
void log_assert(char const* expr, char const* file, long line, const std::string& message);

///
/// @brief Writes a record built from a stream expression, as `LOG(Log::MSG, "value: " << value)`.
///
#define LOG(level, EXPRESSION) ecf::log(level, MESSAGE(EXPRESSION))

///
/// @brief Asserts a condition, reporting the failure through ecf::log_assert with a stream-built message.
///
/// Unlike `assert`, the check is active in every build type.
///
#define LOG_ASSERT(expr, EXPRESSION) \
    ((expr) ? (static_cast<void>(0)) : ecf::log_assert(#expr, __FILE__, __LINE__, MESSAGE(EXPRESSION)))

} // namespace ecf
