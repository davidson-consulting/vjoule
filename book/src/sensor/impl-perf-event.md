# class sensor::perf::PerfEventWatcher

This class is configuring and polling the values of the `perf_event`
being watch for a given cgroup. If no cgroup is configured
(`PerfEventWatcher::_cgroupPath == ""`), then system events are watched.

## Configuration

The configuration `PerfEventWatcher::configure` is made with a list of
event names (e.g. `RAPL_ENERGY_PKG`, `LLC_MISSES`, ...). Multiple
`perf_event` are grouped, so there is only one reading to
make. Perf\_event watching for cgroup is made per cpu, as it is
impossible (apparently) for libperf to watch the event of multiple cpu
core for a cgroup in a single file descriptor. So there is one file
descriptor per cpu core, and this file descriptor is watching multiple
events at the same time.

Configuration uses utility functions
`PerfEventWatcher::findPerfEventAttrs`, that look up for the libpfm
attributes. Only available events are watched, meaning that a
`PerfEventWatcher` is working even if there are some undefined event
in the configuration. Undefined event are logged as errors.

## Polling events

The polling of event is made using the file descriptors opened during
the configuration.  There are two ways of polling events, with and
without cache. With cache is the prefered method
`PerfEventWatcher::poll (vector<Metric>&)` as it does not reallocate
memory, and is therefore more efficient.

The polling reset the metric values, the content of the metric is the
number of events since the last poll.


## Disposing 

`PerfEventWatcher` opens many file descriptor that must be closed. For
that the class was written using the `move` semantic. Thus it is
impossible to `copy` a `PerfEventWatcher`, it must be moved `std::move
(watcher)`. All disposing are made automatically by the destructor of
the instance that has the ownership.
