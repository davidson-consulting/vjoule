# class `sensor::Sensor`

This class contains the main part of the sensor. It runs 3 threads
(main, and two separate threads).

## Main thread

The main thread is responsible of the creation of the configuration,
and is able to start/stop the other two threads if needed (by a
configuration update for example). It waits a notification from
`sensor::Notifier`, that is triggered by a modification of the context
(creation or deletion of a cgroup, modification of the configuration
file, etc.).

![Sensor diagram](images/sensor_diagram_main_thread.png)

Listing, and configuring the watchers is made in the function
`Sensor::configure`. This function should be called within `this->
_mutex` locking, as it is not done inside the function itself.

Methods `Sensor::createHeader` and `Sensor::createPacket` pre
allocates the memory of the packet for the memory polling, to gain
some time, by avoiding allocation during the event polling. The packet
that is created is used in the report thread.

Method `Sensor::onContextChange` is the slot connected to the
`sensor::Notifier`. This function updates everything (middle and right
parts of the above diagram).

## Accept formula thread

This thread is pretty simple, it simply wait for new incoming
connexions from formulas, and register them as clients. This thread
uses a `net::TcpListener`, and create `net::TcpStream` when clients
are connected.

This is in this thread that `net::Header` is transmitted to the
clients.

## Report thread

The report thread uses the perf watcher
`sensor::perf::PerfEventWatcher` created by the main thread, and sent
the result to the client connected in the accept thread
`net::TcpStream`. Client deconnexion is managed in this thread.

![Sensor report thread](images/sensor_diagram_report_thread.png)

This thread send `net::Packet` to clients.

