/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/Stats.hpp"

#include <iomanip>
#include <memory>
#include <sstream>
#include <string>

#include <boost/range/adaptors.hpp>

#include "ecflow/core/SState.hpp"

Stats::Stats() = default;

Stats::Stats(const Stats& rhs)
    : locked_by_user_(rhs.locked_by_user_),
      host_(rhs.host_),
      port_(rhs.port_),
      up_since_(rhs.up_since_),
      version_(rhs.version_),
      request_stats_(rhs.request_stats_),
      ECF_HOME_(rhs.ECF_HOME_),
      ECF_CHECK_(rhs.ECF_CHECK_),
      ECF_LOG_(rhs.ECF_LOG_),
      ECF_SSL_(rhs.ECF_SSL_),
      status_(rhs.status_),
      request_count_(rhs.request_count_),
      job_sub_interval_(rhs.job_sub_interval_),
      checkpt_interval_(rhs.checkpt_interval_),
      checkpt_save_time_alarm_(rhs.checkpt_save_time_alarm_),
      checkpt_mode_(rhs.checkpt_mode_),
      no_of_suites_(rhs.no_of_suites_),
      checkpt_(rhs.checkpt_),
      restore_defs_from_checkpt_(rhs.restore_defs_from_checkpt_),
      server_version_(rhs.server_version_),
      restart_server_(rhs.restart_server_),
      shutdown_server_(rhs.shutdown_server_),
      halt_server_(rhs.halt_server_),
      reload_white_list_file_(rhs.reload_white_list_file_),
      reload_passwd_file_(rhs.reload_passwd_file_),
      ping_(rhs.ping_),
      debug_server_on_(rhs.debug_server_on_),
      debug_server_off_(rhs.debug_server_off_),
      get_defs_(rhs.get_defs_),
      sync_(rhs.sync_),
      sync_full_(rhs.sync_full_),
      sync_clock_(rhs.sync_clock_),
      news_(rhs.news_),
      node_job_gen_(rhs.node_job_gen_),
      node_check_job_gen_only_(rhs.node_check_job_gen_only_),
      node_delete_(rhs.node_delete_),
      node_suspend_(rhs.node_suspend_),
      node_resume_(rhs.node_resume_),
      node_kill_(rhs.node_kill_),
      node_status_(rhs.node_status_),
      node_edit_history_(rhs.node_edit_history_),
      node_archive_(rhs.node_archive_),
      node_restore_(rhs.node_restore_),
      log_cmd_(rhs.log_cmd_),
      log_msg_cmd_(rhs.log_msg_cmd_),
      begin_cmd_(rhs.begin_cmd_),
      task_init_(rhs.task_init_),
      task_complete_(rhs.task_complete_),
      task_wait_(rhs.task_wait_),
      task_abort_(rhs.task_abort_),
      task_event_(rhs.task_event_),
      task_meter_(rhs.task_meter_),
      task_label_(rhs.task_label_),
      task_queue_(rhs.task_queue_),
      zombie_fob_(rhs.zombie_fob_),
      zombie_fail_(rhs.zombie_fail_),
      zombie_adopt_(rhs.zombie_adopt_),
      zombie_remove_(rhs.zombie_remove_),
      zombie_get_(rhs.zombie_get_),
      zombie_block_(rhs.zombie_block_),
      zombie_kill_(rhs.zombie_kill_),
      requeue_node_(rhs.requeue_node_),
      order_node_(rhs.order_node_),
      run_node_(rhs.run_node_),
      load_defs_(rhs.load_defs_),
      replace_(rhs.replace_),
      force_(rhs.force_),
      free_dep_(rhs.free_dep_),
      suites_(rhs.suites_),
      edit_script_(rhs.edit_script_),
      alter_cmd_(rhs.alter_cmd_),
      ch_cmd_(rhs.ch_cmd_),
      file_ecf_(rhs.file_ecf_),
      file_job_(rhs.file_job_),
      file_jobout_(rhs.file_jobout_),
      file_cmdout_(rhs.file_cmdout_),
      file_manual_(rhs.file_manual_),
      plug_(rhs.plug_),
      move_(rhs.move_),
      group_cmd_(rhs.group_cmd_),
      server_load_cmd_(rhs.server_load_cmd_),
      stats_(rhs.stats_),
      check_(rhs.check_),
      query_(rhs.query_),
      process_meter_(nullptr),
      request_vec_(rhs.request_vec_) {
    if (rhs.process_meter_) {
        process_meter_ = std::make_unique<ecf::resources::ProcessMeter>(*rhs.process_meter_);
    }
}

Stats& Stats::operator=(const Stats& rhs) {
    if (this == &rhs) {
        return *this;
    }

    Stats tmp(rhs);
    using std::swap;
    swap(locked_by_user_, tmp.locked_by_user_);
    swap(host_, tmp.host_);
    swap(port_, tmp.port_);
    swap(up_since_, tmp.up_since_);
    swap(version_, tmp.version_);
    swap(request_stats_, tmp.request_stats_);
    swap(ECF_HOME_, tmp.ECF_HOME_);
    swap(ECF_CHECK_, tmp.ECF_CHECK_);
    swap(ECF_LOG_, tmp.ECF_LOG_);
    swap(ECF_SSL_, tmp.ECF_SSL_);
    swap(status_, tmp.status_);
    swap(request_count_, tmp.request_count_);
    swap(job_sub_interval_, tmp.job_sub_interval_);
    swap(checkpt_interval_, tmp.checkpt_interval_);
    swap(checkpt_save_time_alarm_, tmp.checkpt_save_time_alarm_);
    swap(checkpt_mode_, tmp.checkpt_mode_);
    swap(no_of_suites_, tmp.no_of_suites_);
    swap(checkpt_, tmp.checkpt_);
    swap(restore_defs_from_checkpt_, tmp.restore_defs_from_checkpt_);
    swap(server_version_, tmp.server_version_);
    swap(restart_server_, tmp.restart_server_);
    swap(shutdown_server_, tmp.shutdown_server_);
    swap(halt_server_, tmp.halt_server_);
    swap(reload_white_list_file_, tmp.reload_white_list_file_);
    swap(reload_passwd_file_, tmp.reload_passwd_file_);
    swap(ping_, tmp.ping_);
    swap(debug_server_on_, tmp.debug_server_on_);
    swap(debug_server_off_, tmp.debug_server_off_);
    swap(get_defs_, tmp.get_defs_);
    swap(sync_, tmp.sync_);
    swap(sync_full_, tmp.sync_full_);
    swap(sync_clock_, tmp.sync_clock_);
    swap(news_, tmp.news_);
    swap(node_job_gen_, tmp.node_job_gen_);
    swap(node_check_job_gen_only_, tmp.node_check_job_gen_only_);
    swap(node_delete_, tmp.node_delete_);
    swap(node_suspend_, tmp.node_suspend_);
    swap(node_resume_, tmp.node_resume_);
    swap(node_kill_, tmp.node_kill_);
    swap(node_status_, tmp.node_status_);
    swap(node_edit_history_, tmp.node_edit_history_);
    swap(node_archive_, tmp.node_archive_);
    swap(node_restore_, tmp.node_restore_);
    swap(log_cmd_, tmp.log_cmd_);
    swap(log_msg_cmd_, tmp.log_msg_cmd_);
    swap(begin_cmd_, tmp.begin_cmd_);
    swap(task_init_, tmp.task_init_);
    swap(task_complete_, tmp.task_complete_);
    swap(task_wait_, tmp.task_wait_);
    swap(task_abort_, tmp.task_abort_);
    swap(task_event_, tmp.task_event_);
    swap(task_meter_, tmp.task_meter_);
    swap(task_label_, tmp.task_label_);
    swap(task_queue_, tmp.task_queue_);
    swap(zombie_fob_, tmp.zombie_fob_);
    swap(zombie_fail_, tmp.zombie_fail_);
    swap(zombie_adopt_, tmp.zombie_adopt_);
    swap(zombie_remove_, tmp.zombie_remove_);
    swap(zombie_get_, tmp.zombie_get_);
    swap(zombie_block_, tmp.zombie_block_);
    swap(zombie_kill_, tmp.zombie_kill_);
    swap(requeue_node_, tmp.requeue_node_);
    swap(order_node_, tmp.order_node_);
    swap(run_node_, tmp.run_node_);
    swap(load_defs_, tmp.load_defs_);
    swap(replace_, tmp.replace_);
    swap(force_, tmp.force_);
    swap(free_dep_, tmp.free_dep_);
    swap(suites_, tmp.suites_);
    swap(edit_script_, tmp.edit_script_);
    swap(alter_cmd_, tmp.alter_cmd_);
    swap(ch_cmd_, tmp.ch_cmd_);
    swap(file_ecf_, tmp.file_ecf_);
    swap(file_job_, tmp.file_job_);
    swap(file_jobout_, tmp.file_jobout_);
    swap(file_cmdout_, tmp.file_cmdout_);
    swap(file_manual_, tmp.file_manual_);
    swap(plug_, tmp.plug_);
    swap(move_, tmp.move_);
    swap(group_cmd_, tmp.group_cmd_);
    swap(server_load_cmd_, tmp.server_load_cmd_);
    swap(stats_, tmp.stats_);
    swap(check_, tmp.check_);
    swap(query_, tmp.query_);
    swap(process_meter_, tmp.process_meter_);
    swap(request_vec_, tmp.request_vec_);

    return *this;
}

void Stats::update_stats(int poll_interval) {
    // Called at poll time, ie just before node tree traversal
    request_vec_.emplace_back(request_count_, poll_interval);
    request_count_ = 0;
    request_stats_.clear();

    // To avoid excessive memory usage, we store only a limited number of:
    //  - requests per poll period
    // Since we're polling every 60 seconds, sample cover the last hour
    if (request_vec_.size() > 60) {
        request_vec_.pop_front();
    }
}

void Stats::update_for_serialisation() {
    /// This *ONLY* computes the data when this function is called
    /// >>> Hence the server load is only valid for last hour <<<

    // Retrieve Process resources information
    try {
        using namespace ecf::resources;
        process_meter_ = std::make_unique<ProcessMeter>(Machine::make().get_process_meter());
    }
    catch (const ecf::resources::UnsupportedPlatform& e) {
        process_meter_ = nullptr;
    }
    catch (const ecf::resources::ResourceUnavailable& e) {
        process_meter_ = nullptr;
    }

    if (!request_stats_.empty()) {
        // Found request statistics in 'cache'. Nothing further to do...
        return;
    }

    if (request_vec_.empty()) {
        // No data to create statistics...
        return;
    }

    std::stringstream ss;
    ss << std::setiosflags(std::ios::fixed) << std::setprecision(2);

    int count      = 0;
    double request = 0.0;
    double seconds = 0.0;
    for (const auto& entry : boost::adaptors::reverse(request_vec_)) {
        count++;
        request += entry.first;
        seconds += entry.second;
        double request_per_second = request / seconds;

        switch (count) {
            case 5:
            case 15:
            case 30:
            case 60: {
                ss << " ";
                [[fallthrough]];
            }
            case 1: {
                ss << request_per_second;
            }
            default: {
                // nothing to do...
            }
        }
    }
    request_stats_ = ss.str();
}

void Stats::reset() {
    checkpt_                   = 0;
    restore_defs_from_checkpt_ = 0;

    server_version_         = 0;
    restart_server_         = 0;
    shutdown_server_        = 0;
    halt_server_            = 0;
    reload_white_list_file_ = 0;
    reload_passwd_file_     = 0;
    ping_                   = 0;
    debug_server_on_        = 0;
    debug_server_off_       = 0;
    get_defs_               = 0;
    sync_                   = 0;
    sync_full_              = 0;
    sync_clock_             = 0;
    news_                   = 0;

    node_job_gen_            = 0;
    node_check_job_gen_only_ = 0;
    node_delete_             = 0;
    node_suspend_            = 0;
    node_resume_             = 0;
    node_kill_               = 0;
    node_status_             = 0;
    node_edit_history_       = 0;
    node_archive_            = 0;
    node_restore_            = 0;

    log_cmd_     = 0;
    log_msg_cmd_ = 0;
    begin_cmd_   = 0;

    task_init_     = 0;
    task_complete_ = 0;
    task_wait_     = 0;
    task_abort_    = 0;
    task_event_    = 0;
    task_meter_    = 0;
    task_label_    = 0;
    task_queue_    = 0;

    zombie_fob_    = 0;
    zombie_fail_   = 0;
    zombie_adopt_  = 0;
    zombie_remove_ = 0;
    zombie_get_    = 0;
    zombie_block_  = 0;
    zombie_kill_   = 0;

    requeue_node_ = 0;
    order_node_   = 0;
    run_node_     = 0;
    load_defs_    = 0;
    replace_      = 0;
    force_        = 0;
    free_dep_     = 0;
    suites_       = 0;
    edit_script_  = 0;

    alter_cmd_   = 0;
    ch_cmd_      = 0;
    file_ecf_    = 0;
    file_job_    = 0;
    file_jobout_ = 0;
    file_cmdout_ = 0;
    file_manual_ = 0;

    plug_            = 0;
    move_            = 0;
    group_cmd_       = 0;
    server_load_cmd_ = 0;
    stats_           = 0;
    check_           = 0;
    query_           = 0;
}

static std::string show_checkpt_mode(ecf::CheckPt::Mode m) {
    switch (m) {
        case ecf::CheckPt::NEVER:
            return "CHECK_NEVER";
            break;
        case ecf::CheckPt::ON_TIME:
            return "CHECK_ON_TIME";
            break;
        case ecf::CheckPt::ALWAYS:
            return "CHECK_ON_ALWAYS";
            break;
        case ecf::CheckPt::UNDEFINED:
            return "UNDEFINED";
            break;
    }
    return std::string{};
}

namespace {
struct display_stats_helper
{
    display_stats_helper(std::ostream& os, uint32_t width)
        : os(os),
          width(width) {}

    void operator()(const ecf::resources::NamedValue& value) {
        os << std::left << "  " << std::setw(width) << value.name() << std::fixed << value;
        if (auto& u = value.unit(); !u.empty()) {
            os << " (" << u << ")";
        }
        os << "\n";
    };

    void operator()(const std::string& label, const std::string& value, const std::string& unit = "") {
        if (value.empty()) {
            return;
        }

        os << std::left << "  " << std::setw(width) << label << value;
        if (!unit.empty()) {
            os << " (" << unit << ")";
        }
        os << "\n";
    }

    void operator()(const std::string& label, unsigned int value, const std::string& unit = "") {
        if (value == 0) {
            return;
        }

        os << std::left << "  " << std::setw(width) << label << value;
        if (!unit.empty()) {
            os << " (" << unit << ")";
        }
        os << "\n";
    }

    std::ostream& os;
    uint32_t width;
};

template <typename... Args>
bool found_any(Args... args) {
    return (false || ... || args);
};

} // namespace

bool Stats::has_resources() const {
    return process_meter_ != nullptr;
}

bool Stats::has_user_commands_server() const {
    return found_any(checkpt_,
                     restore_defs_from_checkpt_,
                     server_version_,
                     restart_server_,
                     shutdown_server_,
                     halt_server_,
                     ping_,
                     debug_server_on_,
                     debug_server_off_,
                     get_defs_,
                     sync_,
                     sync_full_,
                     sync_clock_,
                     news_);
}

bool Stats::has_task_commands() const {
    return found_any(
        task_init_, task_complete_, task_wait_, task_abort_, task_event_, task_meter_, task_label_, task_queue_);
}

bool Stats::has_user_commands_zombie() const {
    return found_any(
        zombie_fob_, zombie_fail_, zombie_adopt_, zombie_remove_, zombie_get_, zombie_block_, zombie_kill_);
}
bool Stats::has_user_commands_node() const {
    return found_any(load_defs_,
                     begin_cmd_,
                     requeue_node_,
                     node_job_gen_,
                     node_check_job_gen_only_,
                     node_delete_,
                     node_suspend_,
                     node_resume_,
                     node_kill_,
                     node_status_,
                     node_edit_history_,
                     log_cmd_,
                     log_msg_cmd_,
                     order_node_,
                     run_node_,
                     replace_,
                     force_,
                     free_dep_,
                     suites_,
                     edit_script_,
                     alter_cmd_,
                     ch_cmd_,
                     plug_,
                     move_,
                     group_cmd_,
                     reload_white_list_file_,
                     server_load_cmd_,
                     stats_,
                     check_,
                     query_,
                     reload_passwd_file_,
                     node_archive_,
                     node_restore_);
}

bool Stats::has_user_commands_file() const {
    return found_any(file_ecf_, file_job_, file_jobout_, file_manual_, file_cmdout_);
}

void Stats::show(std::ostream& os) const {
    auto display = display_stats_helper(os, Stats::width);

    os << "Server Statistics\n";
    display("Version", version_);
    display("Status", SState::to_string(status_));
    display("Host", host_);
    display("Port", port_);
    display("Up since", up_since_);
    display("Job sub' interval", job_sub_interval_, "s");
    display("ECF_HOME", ECF_HOME_);
    display("ECF_LOG", ECF_LOG_);
    display("ECF_CHECK", ECF_CHECK_);
    display("ECF_SSL", ECF_SSL_);
    display("Check pt interval", checkpt_interval_, "s");
    display("Check pt mode", show_checkpt_mode(checkpt_mode_));
    display("Check pt save time alarm", checkpt_save_time_alarm_, "s");
    display("Number of Suites", std::to_string(no_of_suites_));
    display("Request/s per 1,5,15,30,60 min", request_stats_);
    os << "\n";

    if (has_resources()) {
        os << "Server Resources\n";
        for (auto const& nv : process_meter_->values_) {
            display(nv);
        }
        os << "\n";
    }

    if (has_user_commands_server()) {
        os << "User Commands (Server-related)\n";
        display("Locked by user", locked_by_user_);
        display("Store to Check point", checkpt_);
        display("Restore from Check point", restore_defs_from_checkpt_);
        display("Restart server", restart_server_);
        display("Shutdown server", shutdown_server_);
        display("Halt server", halt_server_);
        display("Ping", ping_);
        display("Debug server on", debug_server_on_);
        display("Debug server off", debug_server_off_);
        display("Get full definition", get_defs_);
        display("Server version", server_version_);
        display("Sync", sync_);
        display("Sync full", sync_full_);
        display("Sync suite clock", sync_clock_);
        display("News", news_);
        os << "\n";
    }

    if (has_task_commands()) {
        os << "Task Commands\n";
        display("Task init", task_init_);
        display("Task complete", task_complete_);
        display("Task wait", task_wait_);
        display("Task abort", task_abort_);
        display("Task event", task_event_);
        display("Task meter", task_meter_);
        display("Task label", task_label_);
        display("Task queue", task_queue_);
        os << "\n";
    }

    if (has_user_commands_zombie()) {
        os << "User Commands (Zombie-related)\n";
        display("Zombie fob", zombie_fob_);
        display("Zombie fail", zombie_fail_);
        display("Zombie adopt", zombie_adopt_);
        display("Zombie remove", zombie_remove_);
        display("Zombie get", zombie_get_);
        display("Zombie block", zombie_block_);
        display("Zombie kill", zombie_kill_);
        os << "\n";
    }

    if (has_user_commands_node()) {
        os << "User Commands (Node-related)\n";
        display("Load definition", load_defs_);
        display("Begin", begin_cmd_);
        display("Requeue", requeue_node_);
        display("Job generation", node_job_gen_);
        display("Check Job generation", node_check_job_gen_only_);
        display("Node delete", node_delete_);
        display("Node suspend", node_suspend_);
        display("Node resume", node_resume_);
        display("Node kill", node_kill_);
        display("Node status", node_status_);
        display("Node edit history", node_edit_history_);
        display("Node archive", node_archive_);
        display("Node restore", node_restore_);
        display("Log cmd", log_cmd_);
        display("Log message", log_msg_cmd_);
        display("Order", order_node_);
        display("Run", run_node_);
        display("Replace", replace_);
        display("Force", force_);
        display("Free dependencies", free_dep_);
        display("Suites", suites_);
        display("Edit script", edit_script_);
        display("Alter", alter_cmd_);
        display("Client handle", ch_cmd_);
        display("Plug", plug_);
        display("Move", move_);
        display("Group", group_cmd_);
        display("Server load cmd", server_load_cmd_);
        display("Stats cmd", stats_);
        display("Check cmd", check_);
        display("Query cmd", query_);
        display("Reload white list file", reload_white_list_file_);
        display("Reload password file", reload_passwd_file_);
        os << "\n";
    }

    if (found_any(file_ecf_, file_job_, file_jobout_, file_manual_, file_cmdout_)) {
        os << "User Commands (File-related)\n";
        display("File ECF", file_ecf_);
        display("File job", file_job_);
        display("File job out", file_jobout_);
        display("File Cmd out", file_cmdout_);
        display("File manual", file_manual_);
        os << "\n";
    }

    os << std::flush;
}
