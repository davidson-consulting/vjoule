extern crate vjoule_api;

use vjoule_api::{VJouleAPI};
use vjoule_api::vjoule_api::diff::ConsumptionDiff;
use vjoule_api::vjoule_api::error::VJouleError;

fn compute_pi (prec : u64)-> f64 {
    let mut res = 0.0;
    for i in 0 .. prec {
        res += (4.0 / prec as f64) / (1.0 + ((i as f64 - 0.5) * (1.0 / prec as f64)) * ((i as f64 - 0.5) * (1.0 / prec as f64)))
    }
    
    res
}

fn test_pi (api : &mut VJouleAPI, prec : u64)-> Result<ConsumptionDiff, VJouleError> {
    let m_beg = api.get_current_machine_consumption ()?;
    
    let pi = compute_pi (prec);

    // Same here
    let m_end = api.get_current_machine_consumption_no_force ()?;

    let m_diff = m_end - m_beg;
    
    println!("PI : {}", pi);
    println!("{}", m_diff);

    Ok (m_diff)
}

fn run_pi (mut api : &mut VJouleAPI, prec : u64)-> Result<ConsumptionDiff, ()> {
    match test_pi (&mut api, prec) {
	    Err (s) => {
	        println!("Pi error : {}", s);
            Err (())
        }
	    Ok (x) => { Ok (x) }
    }
}

fn main() {
    let api = vjoule_api::VJouleAPI::new ();
    match api {
	    Ok (mut api) => {
	        let a = run_pi (&mut api, 100000000);
            let b = run_pi (&mut api, 200000000);

            match (a, b) {
                (Ok (x), Ok (y)) => {
                    println!("{}", x % y);
                }
                _ => {}
            }
	    }
	    Err (s) => {
	        println!("{}", s);
	    }
    }

}
