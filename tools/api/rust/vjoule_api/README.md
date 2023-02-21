# VJoule API 

This is a library used to communicate with the vJoule service. This
API provides simplified functions to manage cgroups, and retreive the
estimate consumption.

# Simple example

```rust
extern crate vjoule_api;

use std::thread;
use std::time::Duration;

use vjoule_api::VJouleAPI;
use vjoule_api::vjoule_api::error::VJouleError;

fn compute_pi (prec : u64)-> f64 {
    let mut res = 0.0;
    for i in 0 .. prec {
        res += (4.0 / prec as f64) / (1.0 + ((i as f64 - 0.5) * (1.0 / prec as f64)) * ((i as f64 - 0.5) * (1.0 / prec as f64)))
    }
    
    res
}

fn test_pi (api : &mut VJouleAPI)-> Result<(), VJouleError> {
    let mut pg = api.get_group ("this")?;
    let m_beg = api.get_current_machine_consumption ()?;
    
    // No need to force the service iteration, it was already made by the previous call
    let p_beg = pg.get_current_consumption_no_force ()?;
    
    let pi = compute_pi (100000000);
    
    let p_end = pg.get_current_consumption ()?;
    
    // Same here
    let m_end = api.get_current_machine_consumption_no_force ()?;
   
    let m_diff = m_end - m_beg;
    let p_diff = p_end - p_beg;
    
    println!("PI : {}", pi);
    println!("{}", p_diff);
    println!("{}", m_diff);
    println!("{}", p_diff % m_diff);

    Ok (())
}

fn run_pi (mut api : &mut VJouleAPI) {
    match test_pi (&mut api) {
	Err (s) => {
	    println!("Pi error : {}", s);
	}
	Ok (_) => {}
    }
}


fn main() {
    let api = vjoule_api::VJouleAPI::new ();
    match api {
	Ok (mut api) => {
	    run_pi (&mut api);
	}
	Err (s) => {
	    println!("{}", s);
	}
    }
}
```

