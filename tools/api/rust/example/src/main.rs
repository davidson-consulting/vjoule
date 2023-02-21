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

fn test_extern (api : &mut VJouleAPI, pids : &[u64])-> Result <(), VJouleError> {
    let mut pg = api.create_group ("extern", pids)?;

    // Detach the second pid
    pg.detach (&[pids [1]])?;
    
    let m_beg = api.get_current_machine_consumption ()?;   
    let p_beg = pg.get_current_consumption_no_force ()?;
    
    thread::sleep (Duration::from_secs (2));

    // Reattach the second pid 
    pg.attach (&[pids [1]])?;

    thread::sleep (Duration::from_secs (2));
    
    let p_end = pg.get_current_consumption ()?;
    let m_end = api.get_current_machine_consumption_no_force ()?;
    
    let m_diff = m_end - m_beg;
    let p_diff = p_end - p_beg;
    
    println!("{}", p_diff);
    println!("{}", m_diff);
    println!("{}", p_diff % m_diff);

    Ok (())
}

fn run_extern (mut api : &mut VJouleAPI, pids : &[u64]) {
    match test_extern (&mut api, pids) {
	Err (s) => {
	    println!("Extern error : {}", s);
	}
	Ok (_) => {}
    }
}

fn test_existing (api : &mut VJouleAPI, name : &str)-> Result <(), VJouleError> {
    // Get a cgroup that already exist on the machine
    let mut pg = api.get_group (name)?;
    
    let m_beg = api.get_current_machine_consumption ()?;   
    let p_beg = pg.get_current_consumption_no_force ()?;
    
    thread::sleep (Duration::from_secs (5));
    
    let p_end = pg.get_current_consumption ()?;
    let m_end = api.get_current_machine_consumption_no_force ()?;

    let m_diff = m_end - m_beg;
    let p_diff = p_end - p_beg;

    println!("{}", p_diff);
    println!("{}", m_diff);
    println!("{}", p_diff % m_diff);

    Ok (())
}

fn run_existing (mut api : &mut VJouleAPI, name : &str) {
    match test_existing (&mut api, name) {
	Err (s) => {
	    println!("Existing error : {}", s);
	}
	Ok (_) => {}
    }
}

fn main() {
    let api = vjoule_api::VJouleAPI::new ();
    match api {
	Ok (mut api) => {
	    run_pi (&mut api);
	    println!("========");
	    run_extern (&mut api, &[196433, 196434]);
	    println!("========");
	    run_existing (&mut api, "stress.slice/stress");
	}
	Err (s) => {
	    println!("{}", s);
	}
    }

}
