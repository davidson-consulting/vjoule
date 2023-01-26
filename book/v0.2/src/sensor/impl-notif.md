# class sensor::Notifier

This class is used to wait for modification in term of cgroup
filesytem, or configuration file. The idea being, that by using system
waits, less cpu usage is required, and it is therefore not necessary
to read all the cgroup topology at each report iteration.

This class uses the `inotify` system, to wait for modification of both
`config::cgroups::mnt` (defined in the configuration file, or
`/sys/fs/cgroup` by default), and the configuration file
`/etc/vjoule/config.toml`. This is set by calling
`Notifier::configuration` method.

When a modification happens a signal is emitted. This signal can be
connected to any slots. `Notifier::onUpdate ().connect (slot)`.

The notifier can be executed in `sync` mode (on the current thread),
or by spawning a thread, using the methods `Notifier::startSync`,
`Notifier::start`.

Disposing the notifier clear the `inotify` watchers.
