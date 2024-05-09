/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_service_aviso_Listener_HPP
#define ecflow_service_aviso_Listener_HPP

#include <string>
#include <string_view>

namespace ecf::service::aviso {

class Listener {
public:
    Listener() = default;
    Listener(std::string_view name, std::string_view base, std::string_view stem)
        : name_{name},
          base_{base},
          stem_{stem} {}

    std::string_view name() const { return name_; }
    std::string_view base() const { return base_; }
    std::string_view stem() const { return stem_; }

    std::string full() const { return base_ + '/' + stem_; }

private:
    std::string name_{};
    std::string base_{};
    std::string stem_{};
};

} // namespace ecf::service::aviso

#endif /* ecflow_service_aviso_Listener_HPP */
