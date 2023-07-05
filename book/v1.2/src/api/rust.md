# vJoule API rust

The vJoule API for rust is accessible from the crate `vjoule_api`
available in [crates.io](https://crates.io/crates/vjoule_api).

## Permissions

This API uses the vjoule tool `vjoule_cgutils` that can be executed
only by user from the user group `vjoule`. To be part of this group,
the user alice has to type the following command.

```bash
$ usermod -a -G vjoule alice
```

## Usage

The API is base on two important types : `VJouleAPI` and
`ProcessGroup`.  The VJouleAPI type is used to manage process group,
and to retreive the consumption of the whole machine, when process
groups are created by the VJouleAPI and are used to retreive the
consumption of a given cgroup.

The rust API uses the vjoule service, to retreive the consumptions of
the components and cgroups. The API communicates with it to avoid
waiting for service iterations to get the the next consumption, as we
have seen in the user guide.

Further information about the API is presented here [API
Documentation](https://docs.rs/vjoule_api/1.2.0/vjoule_api/struct.VJouleAPI.html). And
a complete example can be found
[here](https://github.com/davidson-consulting/vjoule/blob/main/tools/api/rust/example/src/main.rs).

### Create a new process group

The API can be used to create a new cgroup using a list of pids.

```rust
let mut api = VJouleApi::new ();
let mut pg = api.create_group ("my_group", &[123, 569]);

// attach new pids to the process group
pg = api.create_group ("my_group", &[786, 902]);

let beg = pg.get_current_consumption ()?;

// even if the prefered way is using the attach function
pg.attach (&[45, 897]);

thread::sleep (Duration::from_secs (2));
    
// Detach pids from the process group
pg.detach (&[45]);

let end = pg.get_current_consumption ()?;

println!("{}", end - beg);
```

### Retreive an already created cgroup

The API can retreive a cgroup that already exists. As for the cgroup
presented to the vjoule service, the cgroup accessible from the API
must be in a slice.

```rust
let mut api = VJouleApi::new ();
let mut pg = api.get_group ("slice/group");

let beg = pg.get_current_consumption ()?;

thread::sleep (Duration::from_secs (2));

let end = pg.get_current_consumption ()?;

println!("{}", end - beg);
```

A process group named `this` is created by the API when it is
initialized. This process group contains the pid of the current
process, enabling to retreive the estimated consumption of the running
program.
