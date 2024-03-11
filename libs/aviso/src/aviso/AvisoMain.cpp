/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <cstdlib>
#include <iostream>

#include <nlohmann/json.hpp>

#include "aviso/Aviso.hpp"

void echo(const aviso::ConfiguredListener& listener, const aviso::Notification& notification) {
    std::cout << "Received:" << std::endl;
    std::cout << "Listener --> " << listener.path() << std::endl;
    std::cout << notification << std::endl;
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {

    // Aviso Schema configuration (provided in .json file, or maybe downloaded from Aviso server?)
    std::string listener_schema_location = "/workspace/aviso/client/service_configuration/event_listener_schema.json";
    auto listener_schema                 = aviso::load_listener_schema(listener_schema_location);

    // Aviso configuration (provided in .def file)
    std::vector<std::string> aviso_attribute_cfgs = {
        R"({ "path": "/path/to/diss/COM/A", "address": "http://localhost:2379", "event": "dissemination", "request": { "destination": "COM", "target": "T9", "stream": "eefh", "step": [0, 6, 12, 18] } })", // --> /ec/diss/COM
        R"({ "path": "/path/to/diss/COM/B", "address": "http://localhost:2379", "event": "dissemination", "request": { "destination": "COM", "target": "T9", "step": [0, 6, 12, 18], "stream": "eefh" } })", // --> /ec/diss/COM
        R"({ "path": "/path/to/diss/COM/C", "address": "http://localhost:2379", "event": "dissemination", "request": { "destination": "COM", "step": 0 } })",
        R"({ "path": "/path/to/diss/COM/D", "address": "http://localhost:2379", "event": "dissemination", "request": { "destination": "COM", "step": 0 } })",
        R"({ "path": "/path/to/diss/COM/D", "address": "http://localhost:2378", "event": "dissemination", "request": { "destination": "COM", "step": 0 } })",

        R"({ "path": "/path/to/mars/A", "address": "http://localhost:2379", "event": "mars", "request": { "class": "od" } })"};

    aviso::ListenService service{echo};
    for (const auto& cfg : aviso_attribute_cfgs) {
        auto listener = aviso::create_configured_listener(cfg, listener_schema);
        service.register_listener(listener);
    }

    service.start(std::chrono::seconds{15});

    std::this_thread::sleep_for(std::chrono::seconds(300));

    // service.stop();
    // --> no need to stop the service, it will be stopped (and the worker thread joined) when exiting

    // TODO: next steps
    //  - Persist the latest revision every time it is updated, so that it can be used to resume the service
    //  - Implement a way to resume the service from the latest revision
    //  - Implement a way to update the service configuration (listeners) without stopping the service
    //

    return EXIT_SUCCESS;
}
