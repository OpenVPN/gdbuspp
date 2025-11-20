<!--
SPDX-License-Identifier: AGPL-3.0-only
SPDX-FileCopyrightText: OpenVPN Inc <sales@openvpn.net>
SPDX-FileCopyrightText: David Sommerseth <davids@openvpn.net>
-->

GDBus++ :: glib2 D-Bus C++ interface
====================================

This library provides a simpler C++ based interface to implement D-Bus into
applications in a more C++ approach, based on the C++17 standard.

How to build
------------

This project uses the [Meson Build System](https://mesonbuild.com/)
to compile and install the GDBus++ library and header files.


      $ meson setup --prefix=/usr _builddir
      $ cd _builddir
      $ meson compile
      $ meson test
      $ meson install
      $ sudo ldconfig

To see the configuration options available:

      $ meson configure

See the [meson documentation](https://mesonbuild.com/Commands.html#configure)
how to modify these settings.

### IMPORTANT NOTICE
GDBus++ supports **only 64-bit platforms**.


Basic concepts and the namespaces
---------------------------------
The GDBus++ API is split up across multiple C++ namespaces, all using `DBus`
as the root starting point.

All objects created in the `DBus` namespace should return a `Ptr` based object.
This is just a short form of a `std::shared_ptr<>`.  The `DBus::Connection::Ptr`
is the same as `std::shared_ptr<DBus::Connection>`.  It is not a requirement
to use the `Ptr` style, this is more a code style aspect.

The code excerpts used below can be found in
[docs/example-service.cpp](docs/example-service.cpp) and
[docs/example-proxy.cpp](docs/example-proxy.cpp).

#### `DBus::Connection`

This contains the class and declarations used to connect to the D-Bus on
the running system.

To create a connection to the session D-Bus:
```C++
  auto connection = DBus::Connection::Create(DBus::BusType::Session);
```

To create a connection to the system D-Bus:
```C++
  auto connection = DBus::Connection::Create(DBus::BusType::System);
```

No other bus connections are currently supported.  The `Create()` method
will return a `DBus::Connection::Ptr` object.  This object is used both
by the service providing side and the proxy client side.

#### `DBus::Service`
This provides the main building block to provide a service
on the D-Bus on the system.  This is a class which your own
implementation must extend.  The key elements for a service are:

```C++
class MyService : public DBus::Service
{
  public:
    MyService(DBus::Connection::Ptr connection)
        : DBus::Service(con, "net.example.myservice")
    {
    }

    void BusNameAcquired(const std::string &busname) override
    {
        // Code run when the name of your service is accepted
    }

    void BusNameLost(const std::string &busname) override
    {
        // Code run when your service could not get or keep the bus name
    }
};
```

In the `main()` function of your program, you initialize the service
like this; this example uses the Session Bus:

```C++
  auto connection = DBus::Connection::Create(DBus::BusType::SESSION);
  auto my_service = DBus::Service::Create<MyService>(connection);
```

#### `DBus::Object::Base`
The `Base` class in the `DBus::Object` namespace provides the needed
glue to bind your own C++ object into a D-Bus object.  Inheritance is
used here as well.

In relation to `DBus::Object`, there is also `DBus::Object::Path`
implementation available.  This behaves just like `std::string`, but
does have some additional checks to ensure it's a valid D-Bus object path.
It is recommended to use this as container for object paths, since the
GDBus++ APIs will also treat this as an object path instead of a string
data type when passing the data over the D-Bus.  Using `std::string` will
in these cases require overriding the data type if the D-Bus interface
expects an object path.

```C++
class MyObject : public DBus::Object::Base
{
public:
  MyObject()
      : DBus::Object::Base("/example/myobject",
                            "net.example.myinterface")
  {
  }

  const bool Authorize(const DBus::Authz::Request::Ptr request) override
  {
    // code to authorize a D-Bus proxy client accessing this object
  }
};

```

All D-Bus services must have a root D-Bus object.  This is referred to
as the "service handler" in GDBus++.  This service handler is a basic
`DBus::Object::Base` based object and is added to the D-Bus service using
the `DBus::Service::CreateServiceHandler<>()` method
```C++
  auto connection = DBus::Connection::Create(DBus::BusType::SESSION);
  auto my_service = DBus::Service::Create<MyService>(connection);
  my_service->CreateServiceHandler<MyObject>(connection);
```


##### Adding D-Bus methods
To add D-Bus methods, use the `DBus::Object::Base::AddMethod()` method,
where you define a callback function and the arguments it will receive
and return.  This must be setup during the initialization of the object.
```C++
  MyObject()
      : DBus::Object::Base("/example/myobject",
                            "net.example.myinterface")
  {
    AddMethod("MyMethod",
              [](DBus::Object::Method::Arguments::Ptr args)
              {
                // Code performed on access
              }
      );
  }

```
This adds a very simple method not receiving any input nor sending any
output back to the calling D-Bus proxy client.

To add arguments to a D-bus method, you use the `AddInput()` and
`AddOutput()` methods in the object the `AddMethod()` method returns:
```C++
  // ...
  auto args = AddMethod("MethodWithArgs",
                        [](DBus::Object::Method::Arguments::Ptr args)
                        {
                          // Code performed on access
                        }
      );
  args->AddInput("string_1", glib2::DataType::DBus<std::string>());
  args->AddInput("string_2", glib2::DataType::DBus<std::string>());
  args->AddOutput("result", glib2::DataType::DBus<std::string>());
```

With these two methods declared, the D-Bus object introspection will look
something like this:
```
$ gdbus introspect --session --dest net.example.myservice --object-path /example/myobject
node /example/myobject {
  // ...
  interface net.example.myinterface {
    methods:
      MyMethod();
      MethodWithArgs(in  s string_1,
                     in  s string_2,
                     out s result);
  // ...
  };
};
```

##### Adding D-Bus object properties
To add D-Bus properties (directly accessible variables), also set up via
the constructor:
```C++
  MyObject()
      : DBus::Object::Base("/example/myobject",
                            "net.example.myinterface")
  {
    // ...
    bool readwrite = true;
    AddProperty("my_property", my_property, readwrite);
  }

  // ...
  private:
    std::string my_property{"My Initial Value"};
```
This will add a direct mapping between a C++ variable and a D-Bus object
property.  In this case, the `readwrite` variable is `true`, which allows
the D-Bus proxy client to modify this variable.  The C++ variable is then
updated automatically.  If the C++ variable is modified by your program,
a "Get Property" call to your D-Bus object will reflect that as well.
The D-Bus data type is automatically identified via the C++ variable
data type.

The D-Bus introspection of this property will look something like this:
```
$ gdbus introspect --session --dest net.example.myservice --object-path /example/myobject
node /example/myobject {
  // ...
  interface net.example.myinterface {
    properties:
      readwrite s my_property = 'My Initial Value';
  // ...
  };
};
```

There are more ways to add more complex data types as well, by using the
`AddPropertyBySpec()` method instead of the `AddProperty()`.  Please see
the example code for more details how do make use of that.

##### Adding D-Bus object signals - via `DBus::Signals::Group`
The `Group` class in `DBus::Signals` is used to create an object sending
various types of D-Bus signals from a `DBus::Object::Base` based object.

This is done by creating your own "signal object":
```C++
class MySignalGroup : public DBus::Signals::Group
{
  public:
    using Ptr = std::shared_ptr<MySignalGroup>;

    MySignalGroup(DBus::Connection::Ptr connection)
        : DBus::Signals::Group(connection,
                               "/example/myobject",
                               "net.example.myinterface")
    {
        RegisterSignal("MySignal",
                       {{"message", glib2::DataType::DBus<std::string>()}});
    }

    void MySignal(const std::string &message_content)
    {
        GVariant *message = glib2::Value::Create(message_content);
        SendGVariant("MySignal", message);
    }
};

```
This class can be extended with specialised functions to satisfy your
need for sending D-Bus signals.  All you need in your code is to
instantiate this as an object in your `DBus::Object::Base` based
constructor:
```C++
  MyObject(DBus::Connection::Ptr connection)
      : DBus::Object::Base("/example/myobject",
                            "net.example.myinterface")
  {
    // ...
    my_signals = DBus::Signals::Group::Create<MySignalGroup>(connection);
    my_signals->AddTarget("")
    RegisterSignals(my_signals);
  }

  private:
    MySignals::Ptr signals = nullptr;

```
The `RegisterSignals()` call will add the signal details to the D-Bus
object introspection:
```
$ gdbus introspect --session --dest net.example.myservice --object-path /example/myobject
node /example/myobject {
  // ...
  interface net.example.myinterface {
    signals:
      MySignal(s message);
  // ...
  };
};
```

The any of the code inside the `MyObject` object can now call
`my_signals->MySignal("message");` to send the `MySignal` D-Bus signal.
Since the `AddTarget()` used `""` as the destination target, this will be
be sent to any application listening to the `MySignal signal on the session
bus.

##### D-Bus method arguments - via `DBus::Object::Method::Arguments`
A smart-pointer to an `Arguments` object will be returned when
calling the `DBus::Object::Base::AddMethod()` method.  This object
is used to add input and output arguments to a D-Bus method you
declare, as well as preparing for passing file descriptors via this
D-Bus method call.

In addition, this object is also passed to the D-Bus method callback
as the only argument.  This must be used to extract the arguments
passed to the D-Bus method by the proxy client, as well as the response
this method should return back to the caller.  This happens currently
via glib2's `GVariant` objects, which are generic value containers.

To retrieve the arguments passed to the callback function:
```C++
[](DBus::Object::Method::Arguments::Ptr args)
{
  GVariant *params = args->GetMethodParameters();
  std::string string1 = glib2::Value::Extract(params, 0);
  std::string string2 = glib2::Value::Extract(params, 1);
}
```

To return a value back to the caller:
```C++
[](DBus::Object::Method::Arguments::Ptr args)
{
  // ....
  std::string result = string1 + " <=> " + string2;
  GVariant *ret = glib2::Value::Create(result);
  args->SetMethodReturn(ret);
}
```

To return an error back to the caller, create an exception class based
on the `DBus::Exception` class and throw that as an exception.

#### D-Bus object management via `DBus::Object::Manager`
A D-Bus service may want to dynamically add or remove D-Bus objects
during its runtime.  This is functionality provided via the
`DBus::Object::Manager`.  When creating the `DBus::Service`, an object
manager is prepared for you.

```C++
  auto my_service = DBus::Service::Create<MyService>(connection);
  auto object_manager = my_service->GetObjectManager();
```

This manager provides two primary methods, first to create a new D-Bus
object in the D-Bus service the application is providing.  The second call
will remove the object from the service.

```C++
  auto my_new_object = object_manager->CreateObject<MY_CLASS>(MY_CLASS_CONSTRUCTOR_ARGUMENTS)
  object_manager->RemoveObject(DBus::Object::Path &object_path)
```

The D-Bus path this new object will use comes from the
`DBus::Object::Base()` constructor; the first argument here is the
D-Bus object path to use.  The `DBus::Object::Manager::CreateObject()` call
will return a `Ptr` (`std::shared_ptr<MY_CLASS>`) to the newly created object.
If you preserve a reference to this object, you can remove the object like this:

```C++
  object_manager->RemoveObject(my_new_object->GetPath());
```

A `DBus::Service` implementation wanting to provide access to the
`DBus::Object::Manager` in its object, can pass this via the class constructor:

```C++
  MyObject(DBus::Connection::Ptr connection, DBus::Object::Manager::Ptr obj_mgr)
      : DBus::Object::Base("/example/myobject",
                            "net.example.myinterface"),
        object_manager(obj_mgr)
  {
    // ...
    private:
      DBus::Object::Manager::Ptr object_manager = nullptr;
  };

```

See the [tests/simple-service.cpp](tests/simple-service.cpp) for a more
thorough implementation how to use this functionality.

#### `DBus::Proxy::Client`
This is the main class used to connect to a D-Bus service.  This can
be used both as a stand-alone class or being implemented into your
own class providing a more direct C++ API to a D-Bus service.

To call a D-Bus method:
```C++
auto connection = DBus::Connection::Create(DBus::BusType::SESSION);
auto proxy = Proxy::Client::Create(connection, "net.example.myservice");

auto preset = Proxy::TargetPreset::Create("/example/myobject",
                                          "net.example.myinterface");

GVariantBuilder *args_builder = glib2::Builder::Create("(ss)");
glib2::Builder::Add(args_builder, std::string("My first string"));
glib2::Builder::Add(args_builder, std::string("My Second String"));
GVariant *arguments = glib2::Builder::Finish(args_builder);

GVariant *response = proxy->Call(preset, "MethodWithArgs", arguments);
auto result = glib2::Value::Extract<std::string>(response, 0);
g_variant_unref(response);
```
This will call the `MethodWithArgs` D-Bus method with the required
arguments and extract the result from the response.

To retrieve a D-Bus object property:
```C++
std::string my_property = proxy->GetProperty<std::string>(preset, "my_property");
```

To modify a D-Bus object property:
```C++
std::string new_property_value = "A changed property";
proxy->SetProperty(preset, "my_property", new_property_value);
```

See the [example-proxy.cpp](docs/example-proxy.cpp) and
[example-proxy2.cpp](docs/example-proxy2.cpp) for the a full example of
the code snippets above.

#### `DBus::MainLoop`
This class contains the glib2 main loop logic.  Unless there is a need
to control the main loop directly, it is recommended to use the main loop
built into the `DBus::Service` object.

```C++
auto connection = DBus::Connection::Create(DBus::BusType::SESSION);
auto my_service = DBus::Service::Create<MyService>(connection);

my_service->Run();
```
To set up an independent main loop:
```C++
auto mainloop = DBus::MainLoop::Create();
mainloop->Run();
```

The advantage of using the `DBus::Service` provided main loop, is that
the service itself can more easily shut itself down, by calling the
`DBus::Service::Stop()` method.  Alternatively, the `DBus::Service`
implementation need to get the `DBus::MainLoop::Ptr` object provided via
its constructor or another approach.

#### `glib2` namespace - various low-level helper functions
The `glib2` namespace contains a set of sub-namespaces to help glue the
C based API with a more native C++ API.  This uses C++ templates heavily
and the type deduction happening at compile time.

When working with `GVariant` based glib2 objects, the `glib2::Value`
namespace is typically the most useful place to start looking.

##### `glib2::Value`
Functions to extract or retrieve values from `GVariant` objects to C++
variables are found in this namespace.  The `Extract<>()` group of
functions will extract values from variable lists (when the D-Bus data
type is encapsulated with `(` and `)`).  The `Get<>()` group of functions
will only read the value out of a `GVariant` object where the value is
accessible directly.

This namespace also contains the `ExtractVector()` function to convert
a `GVariant` based array to a C++ `std::vector<>`.  This can only be used
if all the values in the `GVariant` array is of the same data type.  The

The `Create<>()` set of functions will convert a C++ variable into a
`GVariant` object with the appropriate D-Bus data type.


##### `glib2::Builder`
In this namespace there are functions to work with the `GVariantBuilder`
API in glib2.  This is most commonly used to construct more complex data
types and processing data structures.  It also contains methods to
convert `std::vector<>` to `GVariantBuilder` objects.

##### `glib2::DataType`
This namespace contains several functions to retrieve the D-Bus identifier
for various C++ data types.


Examples code
-------------

Several test programs and a few example programs are shipped in this
project.  All the test programs are indirectly used when running
`meson test` and is expected to always work.

* [example-service.cpp](docs/example-service.cpp)
  A very simple D-Bus service.  All the service example code excerpts
  in this file comes from this example.

* [example-proxy.cpp](docs/example-proxy.cpp)
  A very simple D-Bus client proxy, accessing a D-Bus object provided
  by [example-service.cpp](docs/example-service.cpp).  All the client proxy
  examples comes in this README from this example code.

* [example-proxy2.cpp](docs/example-proxy2.cpp)
  An alternative `example-proxy.cpp` implementation, implementing the proxy
  as an object which is used directly from the `main()` function.  This also
  uses the service implemented in
  [example-service.cpp](docs/example-service.cpp).

* [simple-service.cpp](tests/simple-service.cpp)
  This implements a simple D-Bus test service using the Session Bus

* [proxy-client.cpp](tests/proxy-client.cpp)
  This implements a generic client side D-Bus proxy, which has similar
  functionality to the `gdbus` program.

* [signal-subscribe.cpp](tests/signal-subscribe.cpp)
  Implements a simple signal event handler, used for testing purposes.

* [signal-emit.cpp](tests/signal-emit.cpp)
  Provides a simple program to send one or more signals with
  various data types

* [signal-group.cpp](tests/signal-group.cpp)
  Similar to `signal-emit.cpp` but gives the possibility to wrap
  the signal emission of more signals into its own class.

* [fd-receive-read.cpp](tests/fd-receive-read.cpp)
  An test program for receiving a file descriptor over the D-Bus.
  This requires the D-Bus service to prepare a file descriptor
  intended to be sent back to the caller.

* [fd-send-fstat.cpp](tests/fd-send-fstat.cpp)
  Similar to `fd-receive-read.cpp`, but here the D-Bus method
  caller will send a file descriptor to the D-Bus service.


License
-------
This project is licensed under [AGPLv3](COPYRIGHT.md).
