/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_SessionAutoSave_HPP
#define ecflow_viewer_SessionAutoSave_HPP

#include <QObject>
#include <QTimer>

///
/// @brief Coalesces requests to persist the session layout into debounced writes.
///
/// The session snapshot (window geometry, tabs, dashboards, and panes) is expensive
/// to serialise relative to the rate at which layout events arrive, so callers never
/// write it directly. Instead, any change to persisted layout state calls request(),
/// which marks the session dirty and arms two timers:
///
/// - an idle timer, restarted on every request, that flushes shortly after the
///   last change;
/// - a maximum-delay timer, armed only by the first request, that bounds how long
///   a continuous stream of changes (for example a window drag) can postpone the
///   write.
///
/// Whichever timer fires first performs a single write through
/// MainWindow::autoSave(). Requests issued while settings are being restored
/// (see ScopedRestore) are ignored, since the partially reconstructed hierarchy
/// must not overwrite the snapshot it is being read from.
///
/// @invariant At most one write is outstanding at any time; timers are stopped
///            whenever the session is clean.
///
class SessionAutoSave : public QObject {
    Q_OBJECT

public:
    ///
    /// @brief Suppresses autosave requests for the lifetime of the object.
    ///
    /// Instances nest: requests resume only when the outermost guard is destroyed.
    ///
    class ScopedRestore {
    public:
        ScopedRestore();
        ~ScopedRestore();
        ScopedRestore(const ScopedRestore&)            = delete;
        ScopedRestore& operator=(const ScopedRestore&) = delete;
    };

    /// Delay, in milliseconds, between the last request and the write.
    static constexpr int idleIntervalMs = 1000;
    /// Upper bound, in milliseconds, between the first unsaved request and the write.
    static constexpr int maxIntervalMs = 10000;
    /// Delay, in milliseconds, before a failed write is retried.
    static constexpr int retryIntervalMs = 60000;

    ///
    /// @brief Returns the application-wide coordinator, creating it on first use.
    ///
    /// @return The singleton instance; never nullptr.
    ///
    static SessionAutoSave* instance();

    ///
    /// @brief Marks the session dirty and schedules a debounced write.
    ///
    /// Ignored while settings are being restored or after the application has
    /// started quitting.
    ///
    void request();

    ///
    /// @brief Writes the session immediately if it is dirty.
    ///
    /// Safe to call repeatedly; a clean session results in no output. A failed
    /// write keeps the session dirty and schedules a retry.
    ///
    void flush();

    ///
    /// @brief Discards pending requests and stops all timers.
    ///
    /// Used when the graceful-quit path takes over persistence, so that no timer
    /// fires after the window hierarchy has been torn down.
    ///
    void abandon();

    ///
    /// @brief Reports whether a write is pending.
    ///
    /// @return true when at least one request has not yet been written.
    ///
    bool isDirty() const { return dirty_; }

    ///
    /// @brief Reports whether requests are currently suppressed.
    ///
    /// @return true while at least one ScopedRestore is alive.
    ///
    bool isRestoring() const { return restoreDepth_ > 0; }

private:
    SessionAutoSave();

    void beginRestore();
    void endRestore();
    void stopTimers();

    QTimer idleTimer_;
    QTimer maxTimer_;
    bool dirty_{false};
    int restoreDepth_{0};
};

#endif /* ecflow_viewer_SessionAutoSave_HPP */
