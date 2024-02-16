# vJoule API rust

The vJoule API for rust is accessible from the crate `vjoule_api`
available in [crates.io](https://crates.io/crates/vjoule_api).

## Usage
The API is base on the type `vjoule_api::VJouleAPI`. This type is used to
retreive the consumption of the machine from different components (enabled by the
configuration of the vjoule_service).

The Rust API uses the vjoule service to retrieve the consumption of the
components. The service must be running, using the `simple` core plugin. The
`getCurrentMachineConsumption` function triggers a consumption reading by the
service, and retrieves the values of each enabled component (disabled components
are set to 0). So there is no need to wait for the next iteration of the service
to read a value. In fact, the service can be configured with a frequency of 0
(i.e., no iteration at all).

Further information about the API is presented here [API
Documentation](https://docs.rs/vjoule_api/1.3.1/vjoule_api/struct.VJouleAPI.html). And
a complete example can be found
[here](https://github.com/davidson-consulting/vjoule/blob/main/tools/api/rust/example/src/main.rs).

```rust
let api = vjoule_api::VJouleAPI::new ();

let beg = api.get_current_machine_consumption ()?;
foo ();
let end = api.get_current_machine_consumption ()?;

let diff = end - beg;
println!("Foo took {}", diff);
```
