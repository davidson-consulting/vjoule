# Files of interest

Once vJoule is installed, you should find a directory located in
`/etc/vjoule` that contains configurations, logs and results. For
example, the service configuration is located in
`/etc/vjoule/config.toml`.

## Configuration files and logs

- `/etc/vjoule/config.toml`: the configuration file of the `vjoule_service`. It
  can be modified to change the log level and some other options that depends on
  the core plugin used.
- `/etc/vjoule/service.log`: the log file of the `vjoule_service`,
