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

static std::vector<aviso::ListenRequest> subscribe() {
    return {};
}

static void notify(const aviso::ConfiguredListener& listener, const aviso::Notification& notification) {
    std::cout << "Received:" << std::endl;
    std::cout << "Listener --> " << listener.path() << std::endl;
    std::cout << notification << std::endl;
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {

    // Aviso Schema configuration (provided in .json file, or maybe downloaded from Aviso server?)
    std::string listener_schema_location =
        ".aviso/service_configuration/event_listener_schema.json";
    auto listener_schema = aviso::ListenerSchema::load(listener_schema_location);

    // Aviso configuration (provided in .def file)
    std::vector<std::tuple<std::string, std::string, std::string>> aviso_attribute_cfgs = {

        {R"(/path/to/diss/COM/A)",
         R"(http://localhost:2379)",
         R"({ "event": "dissemination", "request": { "destination": "COM", "target": "T9", "stream": "eefh", "step": [0, 6, 12, 18] } })"},

        {R"(/path/to/diss/COM/B)",
         R"(http://localhost:2379)",
         R"({"event": "dissemination", "request": { "destination": "COM", "target": "T9", "step": [0, 6, 12, 18], "stream": "eefh" } })"},

        {R"(/path/to/diss/COM/C)",
         R"(http://localhost:2379)",
         R"({ "event": "dissemination", "request": { "destination": "COM", "step": 0 } })"},

        {R"(/path/to/diss/COM/D)",
         R"(http://localhost:2379)",
         R"({ "event": "dissemination", "request": { "destination": "COM", "step": 0 } })"},

        {R"(/path/to/diss/COM/D)",
         R"(http://localhost:2378)",
         R"({ "event": "dissemination", "request": { "destination": "COM", "step": 0 } })"},

        {R"(/path/to/mars/A)", R"(http://localhost:2379)", R"({ "event": "mars", "request": { "class": "od" } })"}};

    aviso::ListenService service{listener_schema, notify, subscribe};
    for (auto&& [path, address, listener_cfg] : aviso_attribute_cfgs) {
        aviso::ListenRequest request{path, address, listener_cfg};
        service.register_listener(request);
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
