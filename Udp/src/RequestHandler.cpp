/*
 * Copyright 2009-2023 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "RequestHandler.hpp"

#include <string>
#include <vector>

#include <boost/lexical_cast.hpp>
#include <nlohmann/json.hpp>

#include "ClientAPI.hpp"
#include "Trace.hpp"

namespace ecf {

namespace /* __anonymous__ */ {

/// The interface of a `command`
class BasicCommand {
public:
    virtual void execute(ClientAPI& ctx) = 0;
    virtual ~BasicCommand()              = default;
};

/// The default implementation of a `command`, enables CRTP-based mixins
template <typename COMMAND>
class DefaultCommand : public BasicCommand {
public:
    void execute(ClientAPI& client) final { static_cast<COMMAND*>(this)->actually_execute(client); }
};

/// The `command` to update an ecFlow meter (issued by an ecFlow task)
class CommandUpdateMeter : public DefaultCommand<CommandUpdateMeter> {
public:
    explicit CommandUpdateMeter(std::string path, std::string name, int value)
        : path_{std::move(path)},
          name_{std::move(name)},
          value_{value} {};
    void actually_execute(ClientAPI& client) const { client.child_update_meter(path_, name_, std::to_string(value_)); };

    static constexpr const char* COMMAND_TAG = "meter";

private:
    std::string path_;
    std::string name_;
    int value_;
};

/// The `command` to update an ecFlow label (issued by an ecFlow task)
class CommandUpdateLabel : public DefaultCommand<CommandUpdateLabel> {
public:
    explicit CommandUpdateLabel(std::string path, std::string name, std::string value)
        : path_{std::move(path)},
          name_{std::move(name)},
          value_{std::move(value)} {};
    void actually_execute(ClientAPI& client) const { client.child_update_label(path_, name_, value_); };

    static constexpr const char* COMMAND_TAG = "label";

private:
    std::string path_;
    std::string name_;
    std::string value_;
};

/// The `command` to update an ecFlow event (issued by an ecFlow task)
class CommandUpdateEvent : public DefaultCommand<CommandUpdateEvent> {
public:
    explicit CommandUpdateEvent(std::string path, std::string name, bool value)
        : path_{std::move(path)},
          name_{std::move(name)},
          value_{value} {};
    void actually_execute(ClientAPI& client) const {
        if (value_) {
            client.child_set_event(path_, name_);
        }
        else {
            client.child_clear_event(path_, name_);
        }
    };

    static constexpr const char* COMMAND_TAG = "event";

private:
    std::string path_;
    std::string name_;
    bool value_;
};

/// The actual (type erased) `command`
class Command {
public:
    Command(Command&& rhs) noexcept : impl_{std::move(rhs.impl_)} {}

    template <typename COMMAND, typename... ARGS>
    static Command make_command(ARGS&&... args) {
        return Command{std::make_unique<COMMAND>(std::forward<ARGS>(args)...)};
    }

    void execute(ClientAPI& client) const { impl_->execute(client); }

private:
    explicit Command(std::unique_ptr<BasicCommand>&& impl) : impl_{std::move(impl)} {}

    std::unique_ptr<BasicCommand> impl_;
};

/// The `command` build interface
class CommandBuilder {
public:
    virtual ~CommandBuilder(){};
    virtual const char* name()                = 0;
    virtual Command build(nlohmann::json obj) = 0;
};

/// The builder for a `command` to update an ecFlow label
class CommandUpdateLabelBuilder : public CommandBuilder {
public:
    const char* name() override { return CommandUpdateLabel::COMMAND_TAG; };
    Command build(nlohmann::json obj) override {
        assert(obj.at("command") == CommandUpdateLabel::COMMAND_TAG);

        std::string name  = obj.at("name");
        std::string path  = obj.at("path");
        std::string value = obj.at("value");

        return Command::make_command<CommandUpdateLabel>(path, name, value);
    }
};

/// The builder for a `command` to update an ecFlow meter
class CommandUpdateMeterBuilder : public CommandBuilder {
public:
    const char* name() override { return CommandUpdateMeter::COMMAND_TAG; };
    Command build(nlohmann::json obj) override {
        assert(obj.at("command") == CommandUpdateMeter::COMMAND_TAG);

        std::string name  = obj.at("name");
        std::string path  = obj.at("path");
        std::string value = obj.at("value");

        auto meter_value  = boost::lexical_cast<int>(value);

        return Command::make_command<CommandUpdateMeter>(path, name, meter_value);
    }
};

/// The builder for a `command` to update an ecFlow event
class CommandUpdateEventBuilder : public CommandBuilder {
public:
    const char* name() override { return CommandUpdateEvent::COMMAND_TAG; };
    Command build(nlohmann::json obj) override {
        assert(obj.at("command") == CommandUpdateEvent::COMMAND_TAG);

        std::string name  = obj.at("name");
        std::string path  = obj.at("path");
        std::string value = obj.at("value");

        bool event_value;
        if (value == "1") {
            event_value = true;
        }
        else if (value == "0") {
            event_value = false;
        }
        else {
            throw std::runtime_error("Unexpected Event value");
        }

        return Command::make_command<CommandUpdateEvent>(path, name, event_value);
    }
};

/// The factory of `command`; creates commands based on JSON data
class CommandFactory {
public:
    CommandFactory() : builders_{} {
        builders_.push_back(std::make_unique<CommandUpdateLabelBuilder>());
        builders_.push_back(std::make_unique<CommandUpdateMeterBuilder>());
        builders_.push_back(std::make_unique<CommandUpdateEventBuilder>());
    };

    Command make_command_from(nlohmann::json obj) {
        std::string name = obj.at("command");
        auto found       = std::find_if(std::begin(builders_), std::end(builders_), [&name](const auto& builder) {
            return builder->name() == name;
        });

        if (found == std::end(builders_)) {
            throw std::runtime_error("Unknown command detected");
        }

        return found->get()->build(obj);
    }

    std::vector<std::unique_ptr<CommandBuilder>> builders_;
};

class JSONRequestHandler {
public:
    JSONRequestHandler() : client_{} {}

    void handle(const nlohmann::json& request) {
        try {
            std::string method_type = request.at("method");

            // process "header"
            if (request.contains("header")) {
                nlohmann::json header = request.at("header");
                configure_authentication(header);
            }

            // process "data"
            if (method_type == "put") {
                auto payload    = request.at("payload");

                Command command = command_factory_.make_command_from(payload);
                command.execute(client_);
            }
            else {
                TRACE_ERR("JSONRequestHandler", "unknown method: ", method_type)
            }
        }
        catch (nlohmann::json::exception& e) {
            TRACE_ERR("JSONRequestHandler", "JSON format error: ", e.what())
            return;
        }
        catch (ClientAPIException& e) {
            TRACE_ERR("JSONRequestHandler", "Client invocation error: ", e.what())
            return;
        }
        catch (...) {
            TRACE_ERR("JSONRequestHandler", "Unknown error detected handling request")
            return;
        }

        TRACE_NFO("JSONRequestHandler", "request handled successfully");
    }

    void configure_authentication(const nlohmann::json& header) {
        for (const auto& [key, value] : header.items()) {

            if (key == "task_rid") {
                client_.child_set_remote_id(value);
            }
            else if (key == "task_password") {
                client_.child_set_password(value);
            }
            else if (key == "task_try_no") {
                client_.child_set_try_no(value);
            }
            else {
                TRACE_ERR("JSONRequestHandler", "unknown header: ", key, ", ignored.")
            }
        }
    }

private:
    ClientAPI client_;
    CommandFactory command_factory_;
};

} // namespace

void RequestHandler::handle(const RequestHandler::inbound_t& request) const {
    try {
        nlohmann::json inbound = nlohmann::json::parse(request);
        JSONRequestHandler handler;
        handler.handle(inbound);
    }
    catch (nlohmann::json::exception& e) {
        TRACE_ERR("RequestHandler", "Unable to parse JSON request");
    }
    catch (...) {
        TRACE_ERR("RequestHandler", "Unknown error detected");
    }
}

} // namespace ecf
