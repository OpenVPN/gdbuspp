//  GDBus++ - glib2 GDBus C++ wrapper
//
//  SPDX-License-Identifier: AGPL-3.0-only
//
//  Copyright (C) 2026-  Karol Adrianek <karol.adrianek@openvpn.com>
//  Copyright (C) 2026-  OpenVPN Inc <sales@openvpn.net>
//

/**
 *  @file data.cpp
 *
 *  Functionality related to making the GVariant type
 *  to behave more C++ like
 */


#include "data.hpp"

namespace DBus::Data {

DBus::Data::UPtr MakeUPtr(GVariant *params)
{
    return {(params ? g_variant_ref_sink(params) : nullptr), g_variant_unref};
}

} // namespace DBus::Data