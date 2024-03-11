#  GDBus++ - glib2 GDBus C++ wrapper
#
#  SPDX-License-Identifier: AGPL-3.0-only
#
#  Copyright (C)  OpenVPN Inc <sales@openvpn.net>
#  Copyright (C)  David Sommerseth <davids@openvpn.net>

project(
        'gdbuspp','cpp',
        default_options : [ 'cpp_std=c++17', 'werror=true' ],
        license: 'AGPL-3.0-only',
        version: run_command('scripts/get-git-ref', check: true).stdout().strip(),
        meson_version: '>= 0.58'
)

version_file = vcs_tag(
        command: 'scripts/get-git-ref',
        input:'version.txt.in',
        output:'version.txt',
        fallback:'git_unknown'
)
meson.add_dist_script('scripts/get-git-ref', '--dist')

if (get_option('debug'))
        add_global_arguments('-DGDBUSPP_DEBUG', language:'cpp')
endif

# Build Doxygen documentation?
if (get_option('doxygen'))
        subdir('doxygen')
endif

#
# glib2 dependencies
# This project makes use of the glib, gio and gio-unix components
#
glib2_deps = [
        dependency('glib-2.0'),
        dependency('gio-2.0'),
        dependency('gio-unix-2.0'),
        dependency('gthread-2.0'),
        dependency('threads')
]

#
# The main GDBus++ library
#
build_config_data = configuration_data({
        'GDBUSPP_INTERNAL_DEBUG': get_option('internal_debug'),
})
build_config_h = configure_file(
        output: 'build-config.h',
        configuration: build_config_data,
)


gdbuspp_lib = both_libraries(
        'gdbuspp',
        [
                'gdbuspp/async-process.cpp',
                'gdbuspp/authz-request.cpp',
                'gdbuspp/glib2/callbacks.cpp',
                'gdbuspp/glib2/utils.cpp',
                'gdbuspp/connection.cpp',
                'gdbuspp/credentials/exceptions.cpp',
                'gdbuspp/credentials/query.cpp',
                'gdbuspp/exceptions.cpp',
                'gdbuspp/features/idle-detect.cpp',
                'gdbuspp/mainloop.cpp',
                'gdbuspp/object/base.cpp',
                'gdbuspp/object/callbacklink.cpp',
                'gdbuspp/object/exceptions.cpp',
                'gdbuspp/object/manager.cpp',
                'gdbuspp/object/method.cpp',
                'gdbuspp/object/operation.cpp',
                'gdbuspp/object/path.cpp',
                'gdbuspp/object/property.cpp',
                'gdbuspp/proxy.cpp',
                'gdbuspp/proxy/utils.cpp',
                'gdbuspp/service.cpp',
                'gdbuspp/signals/emit.cpp',
                'gdbuspp/signals/exceptions.cpp',
                'gdbuspp/signals/group.cpp',
                'gdbuspp/signals/signal.cpp',
                'gdbuspp/signals/single-subscription.cpp',
                'gdbuspp/signals/subscriptionmgr.cpp'
        ],
        dependencies: [
                glib2_deps
        ],
        soversion: '0',
        install: true
)
gdbuspp_inc = include_directories('gdbuspp')
install_headers(
        'gdbuspp/async-process.hpp',
        'gdbuspp/authz-request.hpp',
        'gdbuspp/connection.hpp',
        'gdbuspp/exceptions.hpp',
        'gdbuspp/gen-constants.hpp',
        'gdbuspp/mainloop.hpp',
        'gdbuspp/proxy.hpp',
        'gdbuspp/service.hpp',
        subdir: 'gdbuspp'
)

install_headers(
        'gdbuspp/credentials/exceptions.hpp',
        'gdbuspp/credentials/query.hpp',
        subdir: 'gdbuspp/credentials'
)

install_headers(
        'gdbuspp/object/exceptions.hpp',
        'gdbuspp/object/manager.hpp',
        'gdbuspp/object/method.hpp',
        'gdbuspp/object/operation.hpp',
        'gdbuspp/object/path.hpp',
        'gdbuspp/object/property.hpp',
        'gdbuspp/object/base.hpp',
        subdir: 'gdbuspp/object'
)

install_headers(
        'gdbuspp/proxy/utils.hpp',
        subdir: 'gdbuspp/proxy'
)

install_headers(
        'gdbuspp/signals/emit.hpp',
        'gdbuspp/signals/event.hpp',
        'gdbuspp/signals/exceptions.hpp',
        'gdbuspp/signals/group.hpp',
        'gdbuspp/signals/signal.hpp',
        'gdbuspp/signals/single-subscription.hpp',
        'gdbuspp/signals/subscriptionmgr.hpp',
        'gdbuspp/signals/target.hpp',
        subdir: 'gdbuspp/signals'
)

install_headers(
        'gdbuspp/glib2/callbacks.hpp',
        'gdbuspp/glib2/utils.hpp',
        subdir: 'gdbuspp/glib2'
)


#
# Documentation
#
install_data(
        'README.md',
        install_dir: get_option('datadir') / 'doc/gdbuspp',
        install_mode: 'rw-r--r--',
        sources: [
                'docs/dbus-primer.md',
                'docs/example-service.cpp',
                'docs/example-proxy.cpp',
                'docs/example-proxy2.cpp'
        ]
)

#
# Create pkg-config files
#
pkg_mod = import('pkgconfig')
pkg_mod.generate(libraries : gdbuspp_lib,
                 version : run_command('scripts/get-git-ref', check: true).stdout().strip(),
                 name : 'libgdbuspp',
                 filebase : 'gdbuspp',
                 description : 'glib2 GDBus C++ wrapper')


#
# Simple test and example implementations
#
# These examples links statically with the GDBus++ library
#

# A very simple example service program
executable(
        'example-service',
        [
                'docs/example-service.cpp'
        ],
        link_with: [
                gdbuspp_lib.get_static_lib(),
        ],
        dependencies: [
                glib2_deps,
        ]
)


executable(
        'example-proxy',
        [
                'docs/example-proxy.cpp'
        ],
        link_with: [
                gdbuspp_lib.get_static_lib(),
        ],
        dependencies: [
                glib2_deps,
        ]
)

executable(
        'example-proxy2',
        [
                'docs/example-proxy2.cpp'
        ],
        link_with: [
                gdbuspp_lib.get_static_lib(),
        ],
        dependencies: [
                glib2_deps,
        ]
)

# A static library with shared code across all the example programs
tests_lib = static_library(
        'tests-lib',
        [
                'tests/test-utils.cpp'
        ],
        dependencies: [
                glib2_deps
        ]
)

subdir('tests/regression')

# A simple test service, exposing all the functionality
# of the GDBus++ library
simple_service = executable(
        'test_simple-service',
        [
                'tests/simple-service.cpp',
        ],
        link_with: [
                gdbuspp_lib.get_static_lib(),
        ],
        dependencies: [
                glib2_deps,
        ],
        install: get_option('install_testprogs'),
        install_dir: get_option('libexecdir') + '/gdbuspp/tests'
)

#  A simple and generic D-Bus proxy test program
#  Feature wise can somewhat do similar tasks as the gdbus program
test_proxy = executable(
        'test_proxy',
        [
                'tests/proxy-client.cpp'
        ],
        link_with: [
                gdbuspp_lib.get_static_lib(),
                tests_lib,
        ],
        dependencies: [
                glib2_deps,
        ],
        install: get_option('install_testprogs'),
        install_dir: get_option('libexecdir') + '/gdbuspp/tests'
)

test_proxy_utils = executable(
        'test_proxy_utils',
        [
                'tests/proxy-utils.cpp'
        ],
        link_with: [
                gdbuspp_lib.get_static_lib(),
                tests_lib,
        ],
        dependencies: [
                glib2_deps,
        ],
        install: get_option('install_testprogs'),
        install_dir: get_option('libexecdir') + '/gdbuspp/tests'
)

#  A test utility to retrieve D-Bus service credentials
test_credentials = executable(
        'test_credentials',
        [
                'tests/credentials.cpp'
        ],
        link_with: [
                gdbuspp_lib.get_static_lib(),
                tests_lib,
        ],
        dependencies: [
                glib2_deps,
        ],
        install: get_option('install_testprogs'),
        install_dir: get_option('libexecdir') + '/gdbuspp/tests'
)

#  Sends D-Bus signals in various ways
signal_emit = executable(
        'test_signal-emit',
        [
                'tests/signal-emit.cpp',
        ],
        link_with: [
                gdbuspp_lib.get_static_lib(),
                tests_lib,
        ],
        dependencies: [
                glib2_deps,
        ],
        install: get_option('install_testprogs'),
        install_dir: get_option('libexecdir') + '/gdbuspp/tests'
)

#  Tests the DBus::Signals::Group API
signal_group = executable(
        'test_signal-group',
        [
                'tests/signal-group.cpp',
        ],
        link_with: [
                gdbuspp_lib.get_static_lib(),
                tests_lib,
        ],
        dependencies: [
                glib2_deps,
        ],
        install: get_option('install_testprogs'),
        install_dir: get_option('libexecdir') + '/gdbuspp/tests'
)

#  Tests the DBus::Signals::Signal API
signal_signal = executable(
        'test_signal-signal',
        [
                'tests/signal-signal.cpp',
        ],
        link_with: [
                gdbuspp_lib.get_static_lib(),
                tests_lib,
        ],
        dependencies: [
                glib2_deps,
        ],
        install: get_option('install_testprogs'),
        install_dir: get_option('libexecdir') + '/gdbuspp/tests'
)

#  Tests subscribing to D-Bus signals
signal_subscribe = executable(
        'test_signal-subscribe',
        [
                'tests/signal-subscribe.cpp',
        ],
        link_with: [
                gdbuspp_lib.get_static_lib(),
                tests_lib,
        ],
        dependencies: [
                glib2_deps,
        ],
        install: get_option('install_testprogs'),
        install_dir: get_option('libexecdir') + '/gdbuspp/tests'
)

# Tests receving a file descriptor passed from test_simple-service
test_fd_recv = executable(
        'test_fd-receive-read',
        [
                'tests/fd-receive-read.cpp',
        ],
        link_with: [
                gdbuspp_lib.get_static_lib(),
                tests_lib,
        ],
        dependencies: [
                glib2_deps,
        ],
        install: get_option('install_testprogs'),
        install_dir: get_option('libexecdir') + '/gdbuspp/tests'
)

# Tests sending a file descriptor to test_simple-service
test_fd_send = executable(
        'test_fd-send-fstat',
        [
                'tests/fd-send-fstat.cpp',
        ],
        link_with: [
                gdbuspp_lib.get_static_lib(),
                tests_lib,
        ],
        dependencies: [
                glib2_deps,
        ],
        install: get_option('install_testprogs'),
        install_dir: get_option('libexecdir') + '/gdbuspp/tests'
)

# Simple test of various D-Bus vs C++ data type "conversions"
test_data_types = executable(
        'test_data-types',
        [
                'tests/test-data-types.cpp',
        ],
        link_with: [
                gdbuspp_lib.get_static_lib(),
                tests_lib,
        ],
        dependencies: [
                glib2_deps,
        ],
)

test_idle_detect = executable(
        'test_idle-detect',
        [
                'tests/idle-detect.cpp'
        ],
        link_with: [
                gdbuspp_lib.get_static_lib(),
                tests_lib,
        ],
        dependencies: [
                glib2_deps,
        ]
)

#
#
#   Define various test scripts
#
#

#  Tests all the functionality exposed by the test_simple-service
server_runner = find_program('tests/scripts/run-simple-server-test')
test('service-tests',
        server_runner,
        args: [find_program('tests/scripts/test-simple-service-1').full_path()],
        depends: [
                simple_service,
        ],
        priority: 98,
)

#  Tests the D-Bus proxy client aspect of GDBus++, by accessing
#  the test_simple-service service.  Not as comprehensive service testing
#  as 'service-tests', but the goal here is to test the client side proxy API.
test('proxy-tests',
        server_runner,
        args: [find_program('tests/scripts/test-proxy').full_path()],
        depends: [
                simple_service,
                test_proxy
        ],
        priority: 90,
        is_parallel: false
)

test('proxy-utils',
        server_runner,
        args: [test_proxy_utils.full_path()],
        depends: [
                simple_service,
                test_proxy_utils
        ],
        priority: 90,
        is_parallel: false
)

test('credentials',
        server_runner,
        args: [find_program('tests/scripts/test-credentials').full_path()],
        depends: [
                simple_service,
                test_credentials
        ],
        priority: 90,
        is_parallel: false
)

#  Tests requesting a file descriptor to a file opened by the
#  test_simple-service service and write the content of that to a new
#  file.  Check that the file content is identical.
test('pass-fd-receive',
        server_runner,
        args: [find_program('tests/scripts/test-fd-receive').full_path()],
        depends: [
                simple_service,
                test_fd_recv
        ],
        priority: 50,
        is_parallel: false
)

#  Tests sending a file descriptor to an opened file and request the
#  test_simple-service service to run fstat() on that fd.  The D-Bus method
#  should return the UID, GID and file size back to the caller.  This
#  is verified against a local run of 'stat'
test('pass-fd-send',
        server_runner,
        args: [find_program('tests/scripts/test-fd-send').full_path()],
        depends: [
                simple_service,
                test_fd_send
        ],
        priority: 50,
        is_parallel: false
)

#  Tests D-Bus signals, both subscribing to them and emitting them
test('signals',
        server_runner,
        args: [find_program('tests/scripts/test-signals').full_path()],
        depends: [
                signal_emit,
                signal_group,
                signal_subscribe
        ],
        priority: 90,
        timeout: 60,
        is_parallel: false
)

test('test-data-types-plain',
        test_data_types,
        priority: 100,
        timeout: 10,
        is_parallel: true,
        suite: 'standalone',
)

test('test-data-types-dbus',
        server_runner,
        args: [test_data_types.full_path(), '--simple-service'],
        depends: [
                simple_service,
        ],
        priority: 99,
        timeout: 10,
        is_parallel: false,
)

if (get_option('long_tests'))
    test('idle-detect',
        server_runner,
        args: [test_idle_detect.full_path()],
         priority: 50,
         timeout: 60,
         is_parallel: true
    )
endif
