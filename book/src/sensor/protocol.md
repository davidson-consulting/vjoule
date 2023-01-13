# Report protocol 

Reports are sent to all the formulas connected to the address of the
sensor. The protocol is in raw data, so it should be only be read
using a formula written with the same protocol. There are two
different kind of packet that can be sent. The header, sent when a new
connexion is made. And the packet sent whenever a report is available.

Protocol implementation is made in the `common::net` namespace.

### Header 

The byte description of the header : 

| offset | size | type | content |
| --- | --- | --- | --- |
| 0 | 4 | int | the size of the whole packet `packet_size`|
| 4 | 4 | int | nb entries |
| 8 | `packet_size` - 8 | [(short, int, [char])] | content of the header |

The content of the header is a map of metric names (list of pairs) : 

| offset | size | type | content |
| --- | --- | --- | --- |
| 0 | 2 | short | The id of the metric |
| 2 | 4 | int | size of the metric names `metric_name_size` |
| 6 | `metric_name_size` | [char] | the metric name |


An example of header packet (int are written in full text to be readable, but are sent in raw by the sensor) : 

```
| packet size | nb entries | id | name size |   name          | id | name size | name | id | name size |  name   | id | name size |   name   | 
  40             4            0     15        RAPL_ENERGY_PKG    1     3           TSC   2     5         APERF      3    5           MPERF
```

## Report 

The byte description of a report packet : 

| offset | size | type | content |
| --- | --- | --- | --- |
| 0 | 4 | int | the size of the whole packet `packet_size` |
| 4 | 4 | float | energy pp0 in Joules |
| 8 | 4 | float | energy pp1 in Joules |
| 12 | 4 | float | energy pkg in Joules |
| 16 | 4 | float | energy dram in Joules |
| 20 | 4 | float | energy psys in Joules |
| 24 | 4 | int | number of global metric `nb_system_metrics` |
| 28 | `nb_system_metrics` * (2 + 8) | [(short, long)] | list of system metrics values |
| | 4 | int | nb cgroups `nbcgroups` |
| | `packet_size` - (`nb_system_metrics` * (2 + 8) + 4) | [cgroup content] | cgroup metrics |

The cgroup metrics is a list of cgroup names and metrics values : 

| offset | size | type | content |
| --- | --- | --- | --- |
| 0 | 4 | int | cgroup name size `name_size` |
| 4 | `name_size` | [char] | the name of the cgroup |
| `name_size` + 4 | 4 | int | nb metrics `nb_metrics` | 
| `name_size` + 8 | `nb_metrics` * (2 + 8) | [(short, long)] | metrics values |


An example of report packet : 

```
| packet size | energy pp0 | energy pp1 | energy pkg | energy dram | energy psys | nb system metrics | id |    value   | id |    value  | nb cgroups | name size | name | nb metrics | id |   value  | id |  value   | name size |  name | nb metrics | id |  value    | id |  value     | 
     113          12.1         0.001        10.92        1.4           0       2                0   98108989378   7   189082738       2          4         test      2         3    17878329    9  189789326     5        test2     2           3   8717896178    9   1878718897
```
