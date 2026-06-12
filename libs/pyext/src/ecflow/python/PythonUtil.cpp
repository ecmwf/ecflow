/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/python/PythonUtil.hpp"

#include <stdexcept>

#include "ecflow/attribute/Variable.hpp"
#include "ecflow/core/Converter.hpp"

void py_list_to_int_vec(const py::list& list, std::vector<int>& int_vec) {
    auto the_list_size = len(list);
    int_vec.reserve(the_list_size);
    for (size_t i = 0; i < the_list_size; ++i) {
        int_vec.push_back(list[i].cast<int>());
    }
}

void py_list_to_str_vec(const py::list& list, std::vector<std::string>& vec) {
    auto the_list_size = len(list);
    vec.reserve(the_list_size);
    for (size_t i = 0; i < the_list_size; ++i) {
        vec.push_back(list[i].cast<std::string>());
    }
}

void py_list_to_str_vec(const py::list& list, std::vector<Variable>& vec) {
    auto the_list_size = len(list);
    vec.reserve(the_list_size);
    for (size_t i = 0; i < the_list_size; ++i) {
        vec.push_back(list[i].cast<Variable>());
    }
}

void py_dict_to_str_vec(const py::dict& dict, std::vector<std::pair<std::string, std::string>>& vec) {

    for (auto entry : dict) {
        std::string first = entry.first.cast<std::string>();

        std::string second;
        if (auto found = py_extract<std::string>(entry.second); found) {
            second = found.value();
        }
        else if (auto found = py_extract<py::str>(entry.second); found) {
            second = found.value();
        }
        else if (auto found = py_extract<int>(entry.second); found) {
            int value = found.value();
            second    = ecf::convert_to<std::string>(value);
        }
        else if (auto found = py_extract<py::int_>(entry.second); found) {
            int value = found.value();
            second    = ecf::convert_to<std::string>(value);
        }
        else {
            throw std::runtime_error("PythonUtil::dict_to_str_vec: type not convertible to string or integer");
        }

        vec.emplace_back(first, second);
    }
}

void py_dict_to_str_vec(const py::dict& dict, std::vector<Variable>& vec) {

    for (auto entry : dict) {
        std::string first;
        if (auto found = py_extract<py::str>(entry.first); found) {
            first = found.value();
        }
        else {
            throw std::runtime_error("PythonUtil::dict_to_str_vec: key not convertible to string");
        }

        std::string second;
        if (auto found = py_extract<py::str>(entry.second); found) {
            second = found.value();
        }
        else if (auto found = py_extract<py::int_>(entry.second); found) {
            int value = found.value();
            second    = ecf::convert_to<std::string>(value);
        }
        else {
            throw std::runtime_error("PythonUtil::dict_to_str_vec: value not convertible to string or integer");
        }

        vec.emplace_back(first, second);
    }
}

py::object py_id(const py::object& self) {
    py::object builtins = py::module_::import("builtins");
    py::object id_func  = builtins.attr("id");
    return id_func(self);
}

ssize_t py_hash(const py::object& self) {
    return py::hash(py_id(self));
};

void py_finalize_enum(py::module& m, const char* class_name) {
    py::object cls = m.attr(class_name);

    // Use `NotImplemented` in __eq__ as return value when comparing enum with something not actually and enum/int
    static py::object NotImplemented = py::module_::import("builtins").attr("NotImplemented");

    // __str__ : return just the name (pybind11 default returns "ClassName.name")
    cls.attr("__str__") =
        py::cpp_function([](const py::object& self) -> std::string { return self.attr("name").cast<std::string>(); },
                         py::name("__str__"),
                         py::is_method(cls));

    // __repr__ : return module-qualified form
    cls.attr("__repr__") = py::cpp_function(
        [](const py::object& self) -> std::string {
            return "ecflow." + py::type::of(self).attr("__name__").cast<std::string>() + "." +
                   self.attr("name").cast<std::string>();
        },
        py::name("__repr__"),
        py::is_method(cls));

    // __hash__ : hash equals the underlying integer value
    cls.attr("__hash__") =
        py::cpp_function([](const py::object& self) -> py::ssize_t { return self.attr("value").cast<py::ssize_t>(); },
                         py::name("__hash__"),
                         py::is_method(cls));

    // __eq__ / __ne__ : compare by integer value; accept plain int on RHS
    auto get_int = [](const py::object& o) -> int {
        try {
            return o.attr("value").cast<int>();
        }
        catch (...) {
        }
        return o.cast<int>();
    };
    cls.attr("__eq__") = py::cpp_function(
        [get_int](const py::object& self, const py::object& other) -> py::object {
            try {
                return py::bool_(get_int(self) == get_int(other));
            }
            catch (...) {
                return NotImplemented;
            }
        },
        py::name("__eq__"),
        py::is_method(cls));
    cls.attr("__ne__") = py::cpp_function(
        [get_int](const py::object& self, const py::object& other) -> py::object {
            try {
                return py::bool_(get_int(self) != get_int(other));
            }
            catch (...) {
                return NotImplemented;
            }
        },
        py::name("__ne__"),
        py::is_method(cls));

    // Ordering operators (enum-to-enum only; tests don't mix enum and plain int here)
    auto lhs_int       = [](const py::object& o) -> int { return o.attr("value").cast<int>(); };
    cls.attr("__lt__") = py::cpp_function(
        [lhs_int](const py::object& s, const py::object& o) -> bool { return lhs_int(s) < lhs_int(o); },
        py::name("__lt__"),
        py::is_method(cls));
    cls.attr("__le__") = py::cpp_function(
        [lhs_int](const py::object& s, const py::object& o) -> bool { return lhs_int(s) <= lhs_int(o); },
        py::name("__le__"),
        py::is_method(cls));
    cls.attr("__gt__") = py::cpp_function(
        [lhs_int](const py::object& s, const py::object& o) -> bool { return lhs_int(s) > lhs_int(o); },
        py::name("__gt__"),
        py::is_method(cls));
    cls.attr("__ge__") = py::cpp_function(
        [lhs_int](const py::object& s, const py::object& o) -> bool { return lhs_int(s) >= lhs_int(o); },
        py::name("__ge__"),
        py::is_method(cls));

    // .values  {int -> member}   and   .names  {str -> member}
    py::dict values_dict, names_dict;
    for (auto item : cls.attr("__members__").attr("items")()) {
        auto kv                                                 = item.cast<py::tuple>();
        py::object key                                          = kv[0];
        py::object member                                       = kv[1];
        values_dict[py::int_(member.attr("value").cast<int>())] = member;
        names_dict[key]                                         = member;
    }
    py::setattr(cls, "values", values_dict);
    py::setattr(cls, "names", names_dict);
}
