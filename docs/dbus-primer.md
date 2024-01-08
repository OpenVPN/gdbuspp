<!--
SPDX-License-Identifier: AGPL-3.0-only
SPDX-FileCopyrightText: OpenVPN Inc <sales@openvpn.net>
SPDX-FileCopyrightText: David Sommerseth <davids@openvpn.net>
-->

D-Bus Primer - Understanding the bus
====================================

D-Bus is first and foremost an Inter Process Communication (IPC) solution. But
it does a bit more than just passing a blob of information from one process to
another one. D-Bus is object oriented, where each object can have a set of
Methods, Properties and Signals. All the data passed through D-Bus is also
required to use the data types provided by D-Bus. This enforces a fairly
strict and predictable type checking, which means the application implementing
D-Bus will not need to care about parsing message protocols.

The D-Bus design is built around a server/client approach, where the server
side is most commonly referred to as a D-Bus service. And the client side
uses, what is often called, a client proxy to do operations on D-Bus objects
provided by a specific D-Bus service. Operations can be calling methods or
reading or writing object properties. D-Bus object are referenced through D-Bus
paths. And a D-Bus service is accessed through a destination reference, which
is a bus name and a bus type indicator.

Bus types
---------

D-Bus also operates with several types of buses. The most commonly used
implementations use either the _system bus_ or the _session bus_. The system
bus is by default fairly strictly controlled and locked down. Users can only
access services on the system bus if the policy for that service allows
it. The session bus is only reachable by a single user, and most commonly also
only in a specific sessions. The session bus is also not that strict
configured, as its availability is much more reduced. For example if you are
logged in through a graphical desktop login, you most likely have a session
bus running for that login. If you at the same time from another computer log
in via SSH to the same user account on the same graphical desktop host, that
SSH login will not have access to the session bus which the graphical desktop
login uses - despite the username and host being identical.

Bus names
---------

Each process implementing a D-Bus is required to have a unique bus name,
regardless if it has the role of a server or client. This is typically using
the `:X.Y` notation. Also beware that this unique bus name is provided by theimport dbus

# Get a connection to the system bus
bus_connection = dbus.SessionBus()

# Use the connection to attach to the example service and its main object
service_object = bus_connection.get_object('net.example.myservice',    # Well-known bus name
                                           '/example/myobject')    # Object path

# Retrieve access to the proper interface in the object
my_interface = dbus.Interface(service_object,
                              dbus_interface='net.example.myinterface')

result = my_interface.MethodWithArgs('a simple test string','and another string')
print(result)
D-Bus messaging daemon which is most likely running on your system
already. That means it is not a consistent value. In addition a service can
own a more generic and human readable service name, most commonly called a
well-known bus name. Such a well-known name can for example be
`net.openvpn.gdbuspp.test.simple`. This is used for the server side
implementations, to have a predictable destination reference from the client
side. The implementation of the service instructs the D-Bus message daemon
what should happen if there is conflicting well-known bus names; if new
services should be denied, if existing services should loose the ownership to
the well-known bus name or if exiting services should just ignore it and
continue to run only to be reachable via the unique bus name. But it is also
important to beware that many of the responses received may very well return
the unique bus name and not the well-known bus name you used when connecting
to a D-Bus service.

Getting unto the bus
--------------------

To reach a D-Bus object, you need to first connect to either the system or
session bus. Then you need to get in touch with the service where it is most
common to use the well-known bus name. A well-known bus name is basically a
human readable string name, organized as a reversed domain name. In this
walk-through we will use `net.example.myservice`, which is
a service provided by the `example-service` binary (source code in
[example-service.cpp](example-service.cpp)), and it uses the
 _session bus_ to provide its service.

When connecting to a specific service, the well-known name is referred to as
the _destination_. Within that service you will have one or more objects
available. Using D-Bus object paths, you will get access to one or more
interfaces, where each interface have their own set of Methods, Signals and
Properties.

D-Bus methods are functions you can call, which will be executed by the D-Bus
service. D-Bus properties are variables owned by an object which you can read
or write to, depending on their attributes. And D-Bus signals are events
happening inside a D-Bus service which it can use to get some attention to
changes. To receive such signals, you need to subscribe to them first.

Lets have a look at what the example program `example-service`, shipped in
this project, provides

```
$ gdbus introspect --session --session --dest net.example.myservice --object-path /example/myobject
node /example/myobject {
  interface org.freedesktop.DBus.Properties {
    methods:
      Get(in  s interface_name,
          in  s property_name,
          out v value);
      GetAll(in  s interface_name,
             out a{sv} properties);
      Set(in  s interface_name,
          in  s property_name,
          in  v value);
    signals:
      PropertiesChanged(s interface_name,
                        a{sv} changed_properties,
                        as invalidated_properties);
    properties:
  };
  interface org.freedesktop.DBus.Introspectable {
    methods:
      Introspect(out s xml_data);
    signals:
    properties:
  };
  interface org.freedesktop.DBus.Peer {
    methods:
      Ping();
      GetMachineId(out s machine_uuid);
    signals:
    properties:
  };
  interface net.example.myinterface {
    methods:
      MyMethod();
      MethodWithArgs(in  s string_1,
                     in  s string_2,
                     out s result);
    signals:
      MySignal(s message);
    properties:
      readwrite s my_property = 'My Initial Value';
  };
};
$
```

The command line used here, uses the D-Bus introspection feature which most
services provides. It gives an idea of what is available and the required
API.

In the call above, we ask for the introspection data of a service on the
session bus (The `--session` argument). Further, we connect to the
`net.example.myservice` service and ask for the introspection data of the
object located under `/net/example/myobject`.

The result provides a fairly comprehensive list which includes four different interfaces:

*   `org.freedesktop.DBus.Properties`
*   `org.freedesktop.DBus.Introspectable`
*   `org.freedesktop.DBus.Peer`
*   `net.example.myinterface`

In this section, we will only look at the last interface, `net.example.myinterface`.

This interface provides two methods (`MyMethod` and `MethodWithArgs`), a signal
(`MySignal`) and a property named `my_property` which can be both read and
modified.

The '`s`' reference in the variable declarations indicates that the data type
is a 'string'. There are several data types variables and data can use. You can
read more about the various data types D-Bus supports here:
[https://dbus.freedesktop.org/doc/dbus-specification.html#type-system](https://dbus.freedesktop.org/doc/dbus-specification.html#type-system)

The `MySignal` signal is quite simple in this example service.  It will send
a signal containing just a simple string, in a variable named 'message'.
Likewise, the `my_property` property is also declared as a simple string type.


Interacting with a D-Bus service
--------------------------------

Lets test this service with a very simple Python script first


```python
import dbus

# Get a connection to the system bus
bus_connection = dbus.SessionBus()

# Use the connection to attach to the example service and its main object
service_object = bus_connection.get_object('net.example.myservice',    # Well-known bus name
                                           '/example/myobject')    # Object path

# Retrieve access to the proper interface in the object
my_interface = dbus.Interface(service_object,
                              dbus_interface='net.example.myinterface')

result = my_interface.MethodWithArgs('a simple test string','and another string')
print(result)
```

When running this little Python script, the result will be something like
this:

```
a simple test string <=> and another string

```

We can also use other tools to interact with this D-Bus service, like using
the `dbus-send` utility:

```
$ dbus-send --print-reply --dest=net.example.myservice         \
      /example/myobject net.example.myinterface.MethodWithArgs \
      string:'Test string 1' string:'Another string'
method return time=1698795879.470667 sender=:1.5277 -> destination=:1.5530 serial=77 reply_serial=2
   string "Test string 1 <=> Another string"
$
```

Or using the `busctl`  utility (part of systemd)
```
$ busctl --user call net.example.myservice /example/myobject \
         net.example.myinterface MethodWithArgs ss "Test string 1" "Another string"
s "Test string 1 <=> Another string"

```

### Accessing object properties

Accessing properties in D-Bus objects need to happen via a D-Bus specific
interface. The needed methods to retrieve and modify properties are located
within the `org.freedesktop.DBus.Properties` interface in each available object.
First, lets look closer at that interface, again by using the introspection
possibility.

```
$ gdbus introspect --session --session --dest net.example.myservice --object-path /example/myobject
node /example/myobject {
  interface org.freedesktop.DBus.Properties {
    methods:
      Get(in  s interface_name,
          in  s property_name,
          out v value);
      GetAll(in  s interface_name,
             out a{sv} properties);
      Set(in  s interface_name,
          in  s property_name,
          in  v value);
    signals:
      PropertiesChanged(s interface_name,
                        a{sv} changed_properties,
                        as invalidated_properties);
    properties:
  };
  /\* Removed the rest, for clarity */
};
```

The method we are interested in is the `Get` method in the
`org.freedesktop.DBus.Properties` interface. This method requires two string
arguments, the first one is the _interface_ carrying the property we want to
read. The second argument is the _name_ _of the property_ we want to access.

```
$ dbus-send --session --print-reply --dest=net.example.myservice   \
            /example/myobject                                      \
            org.freedesktop.DBus.Properties.Get                    \
            string:'net.example.myinterface' string:'my_property'
method return sender=:1.2920 -> dest=:1.3560 reply_serial=2
   variant       string "Test config"
$
```

Conclusion
----------

To access a D-Bus object, you need to:

*   Connect to the bus where the service you want to access is available
    (system or session)
*   Know which destination service you are targeting
*   Know which object you want to work with
*   Know which interface within that particular object you want to access

With these four areas covered, you have direct access to all the available
methods and properties inside any object on the D-Bus.
