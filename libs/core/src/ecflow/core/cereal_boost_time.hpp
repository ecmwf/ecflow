/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_core_cereal_boost_time_HPP
#define ecflow_core_cereal_boost_time_HPP

#include <cereal/archives/json.hpp>
#include <cereal/details/traits.hpp>

#include "ecflow/core/Chrono.hpp"

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
