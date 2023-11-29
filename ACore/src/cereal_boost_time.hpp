/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_cereal_boost_time_HPP
#define ecflow_core_cereal_boost_time_HPP

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/details/traits.hpp>

namespace cereal {

// ===================================================================================
// Handle boost::posix_time::time_duration
template <class Archive, traits::EnableIf<traits::is_text_archive<Archive>::value> = traits::sfinae>
inline void save(Archive& ar, boost::posix_time::time_duration const& d) {
    ar(cereal::make_nvp("duration", to_simple_string(d)));
}

//! Loading for std::map<std::string, std::string> for text based archives
template <class Archive, traits::EnableIf<traits::is_text_archive<Archive>::value> = traits::sfinae>
inline void load(Archive& ar, boost::posix_time::time_duration& d) {
    std::string value;
    ar(value);
    d = boost::posix_time::duration_from_string(value);
}

// ===================================================================================
// Handle boost::posix_time::ptime
template <class Archive, traits::EnableIf<traits::is_text_archive<Archive>::value> = traits::sfinae>
inline void save(Archive& ar, boost::posix_time::ptime const& d) {
    ar(cereal::make_nvp("ptime", to_simple_string(d)));
}

//! Loading for std::map<std::string, std::string> for text based archives
template <class Archive, traits::EnableIf<traits::is_text_archive<Archive>::value> = traits::sfinae>
inline void load(Archive& ar, boost::posix_time::ptime& d) {
    std::string value;
    ar(value);
    d = boost::posix_time::time_from_string(value);
}

// ===================================================================================
// Handle boost::gregorian::date
template <class Archive, traits::EnableIf<traits::is_text_archive<Archive>::value> = traits::sfinae>
inline void save(Archive& ar, boost::gregorian::date const& d) {
    ar(cereal::make_nvp("date", to_simple_string(d)));
}

//! Loading for std::map<std::string, std::string> for text based archives
template <class Archive, traits::EnableIf<traits::is_text_archive<Archive>::value> = traits::sfinae>
inline void load(Archive& ar, boost::gregorian::date& d) {
    std::string value;
    ar(value);
    d = boost::gregorian::from_simple_string(value);
}

} // namespace cereal

#endif /* ecflow_core_cereal_boost_time_HPP */
