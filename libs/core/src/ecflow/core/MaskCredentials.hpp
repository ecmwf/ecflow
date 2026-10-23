// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>
#include <string_view>

namespace ecf {

///
/// @brief Returns the given value with every part that looks like a credential replaced by `***`.
///
/// The parts replaced are:
///   - the password of a URL: `scheme://user:password@host` becomes `scheme://user:***@host`;
///   - the value of a credential name, given as `name=value` (in a URL query, the value ends at the next `&`);
///   - the value of a credential option, given as `-name value` or `--name value`.
///
/// A credential name has a part, split at `_`, `-` or `.`, equal to `password`, `passwd`, `token`, `secret` or
/// `key`, in any case (e.g. `API_KEY`, `db_passwd`, `access-token`, but not `keyfile` or `monkey`).
///
/// A value is a complete shell word: quoted segments, escaped characters and their combinations, e.g.
/// `"a b"'c'd\ e`; an unterminated quote extends the value to the end. The masking is a best effort: a secret
/// that is not given as the value of a credential name or option is left unchanged.
///
/// @param[in] value The value to mask, e.g. a command or a URL.
/// @return The masked value; the value itself when nothing looks like a credential.
///
std::string mask_credentials(std::string_view value);

} // namespace ecf
