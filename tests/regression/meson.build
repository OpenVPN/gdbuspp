#  GDBus++ - glib2 GDBus C++ wrapper
#
#  SPDX-License-Identifier: AGPL-3.0-only
#
#  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
#  Copyright (C)  David Sommerseth <davids@openvpn.net>


regr_glib2_utils = executable(
    'glib2_utils',
    [
        'glib2_utils.cpp',
    ],
    link_with: [
        gdbuspp_lib.get_static_lib(),
    ],
    dependencies: [
            glib2_deps,
    ]
)

test('regression-glib2-utils',
    regr_glib2_utils,
    priority: 100,
    timeout: 5,
    is_parallel: true,
    suite: 'standalone'
)
