/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/http/TypeToJson.hpp"

#include "ecflow/core/Child.hpp"
#include "ecflow/core/SState.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"

using ecf::http::ojson;

void to_json(ojson& j, const ::Meter& a) {
    j = ojson({{"name", a.name()}, {"min", a.min()}, {"max", a.max()}, {"value", a.value()}});
}

void to_json(ojson& j, const ::Label& a) {
    j = ojson({{"name", a.name()}, {"value", a.new_value().empty() ? a.value() : a.new_value()}});
}

void to_json(ojson& j, const ::Variable& a) {
    j = ojson({{"name", a.name()}, {"value", a.theValue()}, {"const", false}});
}

void to_json(ojson& j, const ::Event& a) {
    j = ojson({{"name", a.name()}, {"value", a.value()}, {"initial_value", a.initial_value()}});
}

void to_json(ojson& j, const limit_ptr& a) {
    to_json(j, *a);
}

void to_json(ojson& j, const Limit& a) {
    j = ojson({{"name", a.name()}, {"value", a.value()}, {"limit", a.theLimit()}});
}

void to_json(ojson& j, const ::InLimit& a) {
    j = ojson({{"value", a.toString()}});
}

void to_json(ojson& j, const ::DateAttr& a) {
    std::string value = a.toString();
    value             = value.substr(5, std::string::npos); // remove prefix "date "
    j                 = ojson({{"value", value}, {"free", a.isSetFree()}});
}

void to_json(ojson& j, const ::DayAttr& a) {
    std::string value = a.toString();
    value             = value.substr(4, std::string::npos); // remove prefix "day "
    j                 = ojson({{"value", value}, {"free", a.isSetFree()}, {"expired", a.expired()}});
}

void to_json(ojson& j, const ::RepeatDateList& a) {
    std::vector<std::string> lst;
    for (int i = 0; i < a.indexNum(); i++) {
        lst.push_back(a.value_as_string(i));
    }
    j = ojson({{"name", a.name()}, {"index", a.index_or_value()}, {"value", a.valueAsString()}, {"values", lst}});
}

void to_json(ojson& j, const ::RepeatDate& a) {
    j = ojson({{"name", a.name()}, {"start", a.start()}, {"end", a.end()}, {"step", a.step()}, {"value", a.value()}});
}

void to_json(ojson& j, const RepeatDateTime& a) {
    j = ojson({{"name", a.name()},
               {"start", ecf::Instant::format(a.start_instant())},
               {"end", ecf::Instant::format(a.end_instant())},
               {"step", ecf::Duration::format(a.step_duration())},
               {"value", ecf::Instant::format(a.value_instant())}});
}

void to_json(ojson& j, const ::RepeatDay& a) {
    j = ojson({{"name", a.name()}, {"value", a.value()}, {"valid", a.valid()}});
}

void to_json(ojson& j, const ::RepeatInteger& a) {
    j = ojson({{"name", a.name()}, {"start", a.start()}, {"end", a.end()}, {"step", a.step()}, {"value", a.value()}});
}

void to_json(ojson& j, const ::RepeatEnumerated& a) {
    std::vector<std::string> lst;
    for (int i = 0; i < a.indexNum(); i++) {
        lst.push_back(a.value_as_string(i));
    }
    j = ojson({{"name", a.name()}, {"index", a.index_or_value()}, {"value", a.valueAsString()}, {"values", lst}});
}

void to_json(ojson& j, const ::RepeatString& a) {
    std::vector<std::string> lst;
    for (int i = 0; i < a.indexNum(); i++) {
        lst.push_back(a.value_as_string(i));
    }
    j = ojson({{"name", a.name()}, {"index", a.index_or_value()}, {"value", a.valueAsString()}, {"values", lst}});
}

namespace {

template <typename T, typename... Rest>
void repeat_to_json(ojson& j, const RepeatBase* b) {
    if (auto rr = dynamic_cast<const T*>(b)) {
        to_json(j, *rr);
        return;
    }

    if constexpr (sizeof...(Rest) >= 1) {
        repeat_to_json<Rest...>(j, b);
    }
}

} // namespace

void to_json(ojson& j, const Repeat& a) {
    const RepeatBase* b = a.repeatBase();
    repeat_to_json<RepeatDate,
                   RepeatDateTime,
                   RepeatDateList,
                   RepeatInteger,
                   RepeatDay,
                   RepeatString,
                   RepeatEnumerated>(j, b);
}

void to_json(ojson& j, const ::Stats& s) {
    j["version"]                   = s.version_;
    j["status"]                    = SState::to_string(s.status_);
    j["host"]                      = s.host_;
    j["port"]                      = s.port_;
    j["up_since"]                  = s.up_since_;
    j["job_sub_interval"]          = s.job_sub_interval_;
    j["ECF_HOME"]                  = s.ECF_HOME_;
    j["ECF_LOG"]                   = s.ECF_LOG_;
    j["ECF_CHECK"]                 = s.ECF_CHECK_;
    j["ECF_SSL"]                   = s.ECF_SSL_;
    j["checkpt_interval"]          = s.checkpt_interval_;
    j["checkpt_mode"]              = s.checkpt_mode_;
    j["checkpt_save_time_alarm"]   = s.checkpt_save_time_alarm_;
    j["no_of_suites"]              = s.no_of_suites_;
    j["request_stats"]             = s.request_stats_;
    j["locked_by_user"]            = s.locked_by_user_;
    j["checkpt"]                   = s.checkpt_;
    j["restore_defs_from_checkpt"] = s.restore_defs_from_checkpt_;
    j["restart_server"]            = s.restart_server_;
    j["shutdown_server"]           = s.shutdown_server_;
    j["halt_server"]               = s.halt_server_;
    j["ping"]                      = s.ping_;
    j["debug_server_on"]           = s.debug_server_on_;
    j["debug_server_off"]          = s.debug_server_off_;
    j["get_defs"]                  = s.get_defs_;
    j["server_version"]            = s.server_version_;
    j["sync"]                      = s.sync_;
    j["sync_full"]                 = s.sync_full_;
    j["sync_clock"]                = s.sync_clock_;
    j["news"]                      = s.news_;
    j["task_init"]                 = s.task_init_;
    j["task_complete"]             = s.task_complete_;
    j["task_wait"]                 = s.task_wait_;
    j["task_abort"]                = s.task_abort_;
    j["task_event"]                = s.task_event_;
    j["task_meter"]                = s.task_meter_;
    j["task_label"]                = s.task_label_;
    j["task_queue"]                = s.task_queue_;
    j["zombie_fob"]                = s.zombie_fob_;
    j["zombie_fail"]               = s.zombie_fail_;
    j["zombie_adopt"]              = s.zombie_adopt_;
    j["zombie_remove"]             = s.zombie_remove_;
    j["zombie_get"]                = s.zombie_get_;
    j["zombie_block"]              = s.zombie_block_;
    j["zombie_kill"]               = s.zombie_kill_;
    j["load_defs"]                 = s.load_defs_;
    j["begin_cmd"]                 = s.begin_cmd_;
    j["requeue_node"]              = s.requeue_node_;
    j["node_job_gen"]              = s.node_job_gen_;
    j["node_check_job_gen_only"]   = s.node_check_job_gen_only_;
    j["node_delete"]               = s.node_delete_;
    j["node_suspend"]              = s.node_suspend_;
    j["node_resume"]               = s.node_resume_;
    j["node_kill"]                 = s.node_kill_;
    j["node_status"]               = s.node_status_;
    j["node_edit_history"]         = s.node_edit_history_;
    j["node_archive"]              = s.node_archive_;
    j["node_restore"]              = s.node_restore_;
    j["log_cmd"]                   = s.log_cmd_;
    j["log_msg_cmd"]               = s.log_msg_cmd_;
    j["order_node"]                = s.order_node_;
    j["run_node"]                  = s.run_node_;
    j["replace"]                   = s.replace_;
    j["force"]                     = s.force_;
    j["free_dep"]                  = s.free_dep_;
    j["suites"]                    = s.suites_;
    j["edit_script"]               = s.edit_script_;
    j["alter_cmd"]                 = s.alter_cmd_;
    j["ch_cmd"]                    = s.ch_cmd_;
    j["plug"]                      = s.plug_;
    j["move"]                      = s.move_;
    j["group_cmd"]                 = s.group_cmd_;
    j["server_load_cmd"]           = s.server_load_cmd_;
    j["stats"]                     = s.stats_;
    j["check"]                     = s.check_;
    j["query"]                     = s.query_;
    j["reload_white_list_file"]    = s.reload_white_list_file_;
    j["reload_passwd_file"]        = s.reload_passwd_file_;
    j["file_ecf"]                  = s.file_ecf_;
    j["file_job"]                  = s.file_job_;
    j["file_jobout"]               = s.file_jobout_;
    j["file_cmdout"]               = s.file_cmdout_;
    j["file_manual"]               = s.file_manual_;
}

void to_json(ojson& j, const ::Expression* a) {
    if (!a) {
        return;
    }

    to_json(j, *a);
}

void to_json(ojson& j, const ::Expression& a) {
    const std::vector<PartExpression>& exprs = a.expr();
    std::vector<ojson> str;

    for (const auto& expr : exprs) {
        std::string typestr("FIRST");
        if (expr.andExpr()) {
            typestr = "AND";
        }
        else if (expr.orExpr()) {
            typestr = "OR";
        }

        ojson jj = ojson::object({{"expression", expr.expression()}, {"type", typestr}});

        str.push_back(jj);
    }

    std::string value;
    ecf::write_t(value, a, PrintStyle::DEFS, "complete");
    value = value.substr(11, value.size() - 12);
    j     = ojson::object({{"free", a.isFree()}, {"expressions", str}, {"value", value}});
}

void to_json(ojson& j, const ::QueueAttr& a) {
    j["name"]  = a.name();
    j["index"] = a.index();
    j["value"] = a.value();
    j["queue"] = a.list();
}

void to_json(ojson& j, const ::ZombieAttr& a) {
    j["type"]           = ecf::Child::to_string(a.zombie_type());
    j["action"]         = ecf::to_string(a.action());
    j["child_commands"] = ecf::Child::to_string(a.child_cmds());
    j["lifetime"]       = a.zombie_lifetime();
}

void to_json(ojson& j, const ::GenericAttr& a) {
    j["name"]   = a.name();
    j["values"] = a.values();
}

namespace ecf {

void to_json(ojson& j, const ecf::TimeAttr& a) {
    std::string value = a.toString();
    value             = value.substr(5, std::string::npos); // remove prefix "date "
    j                 = ojson({{"value", value}});
}

void to_json(ojson& j, const ecf::TodayAttr& a) {
    j = ojson(
        {{"value", a.time_series().toString()}, {"free", a.isSetFree()}, {"expired", !a.time_series().is_valid()}});
}

void to_json(ojson& j, const ecf::CronAttr& a) {
    std::string value = a.toString();
    value             = value.substr(5, std::string::npos); // remove prefix "cron "
    j                 = ojson({{"value", value}});
}

void to_json(ojson& j, const ecf::Flag& a) {
    j["value"] = a.to_string();
}

void to_json(ojson& j, const ecf::LateAttr* a) {
    if (!a) {
        return;
    }

    to_json(j, *a);
}

void to_json(ojson& j, const ecf::LateAttr& a) {
    j["submitted"]            = a.submitted();
    j["active"]               = a.active();
    j["complete"]             = a.complete();
    j["complete_is_relative"] = a.complete_is_relative();
    j["is_late"]              = a.isLate();
    const std::string value   = a.toString().substr(5, std::string::npos); // remove prefix "late "
    j["value"]                = value;
}

void to_json(ojson& j, const ecf::TimeSlot& a) {
    j["value"] = a.toString();
}

void to_json(ojson& j, const ecf::AutoCancelAttr* a) {
    if (!a) {
        return;
    }

    to_json(j, *a);
}

void to_json(ojson& j, const ecf::AutoCancelAttr& a) {
    j["relative"]     = a.relative();
    j["days"]         = a.days();
    j["time"]         = a.time();
    std::string value = a.toString().substr(11, std::string::npos);
    j["value"]        = value;
}

void to_json(ojson& j, const ecf::AutoRestoreAttr* a) {
    if (!a) {
        return;
    }

    to_json(j, *a);
}
void to_json(ojson& j, const ecf::AutoRestoreAttr& a) {
    std::string value = a.toString().substr(12, std::string::npos);
    j["value"]        = value;
}

void to_json(ojson& j, const ecf::AutoArchiveAttr* a) {
    if (!a) {
        return;
    }

    to_json(j, *a);
}
void to_json(ojson& j, const ecf::AutoArchiveAttr& a) {
    j["relative"]     = a.relative();
    j["days"]         = a.days();
    j["time"]         = a.time();
    std::string value = a.toString().substr(12, std::string::npos);
    j["value"]        = value;
}

void to_json(ojson& j, const ecf::AvisoAttr& a) {
    j["name"]     = a.name();
    j["listener"] = a.listener();
    j["url"]      = a.url();
    j["schema"]   = a.schema();
    j["polling"]  = a.polling();
}

void to_json(ojson& j, const ecf::MirrorAttr& a) {
    j["name"]        = a.name();
    j["remote_path"] = a.remote_path();
    j["remote_host"] = a.remote_host();
    j["remote_port"] = a.remote_port();
    j["polling"]     = a.polling();
    j["ssl"]         = a.ssl();
}

} // namespace ecf
