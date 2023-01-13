extern crate chrono;
extern crate inotify;

use std::path::Path;
use std::fs::metadata;
use std::fs;
use std::collections::HashMap;


use chrono::Local;
use inotify::{
    Inotify,
    WatchMask,
};

// Magic const, in practice the directory of the formula depends on the configuration of the formula
const VJOULE_PATH : &str = "/etc/vjoule/simple_formula";

/**
 * Wait for the signal of the formula 
 * This is the correct way of doing it as it is a passive wait
 */
fn wait_formula_signal () {
    let mut inotify = Inotify::init ()
	.expect ("Error while initializing inotify instance");

    inotify.add_watch (
	format!("{}/formula.signal", VJOULE_PATH),
	WatchMask::MODIFY
    ).expect ("Failed to add watch");

    let mut buffer = [0 ; 1024];
    inotify.read_events_blocking (&mut buffer)
	.expect ("Error while reading events");
}

/**
 * Reading the package and memory result file of a cgroup
 * There can be up to five different file depending on the formula (pp0, pp1, psys, package and memory)
 * package is a mandatory file, the other files are optionnal
 * @params:
 *    - dir: the directory of the cgroup result 
 *    - cpu_map: the map containing the old value of the package file (previous tick)
 *    - memory_map: the map containing the old value of the memory file (previous tick)
 */
fn read_cgroup_consumption (dir : &str, cpu_map : &mut HashMap<String, f64>, ram_map : &mut HashMap<String, f64>) {
    let package_path = format!("{}/package", dir);
    let ram_path = format!("{}/memory", dir);

    let cpu_s = fs::read_to_string(&package_path[..]).unwrap (); 
    let cpu = cpu_s [..cpu_s.len () - 1].parse::<f64> ().unwrap ();

    let ram_s = fs::read_to_string(&ram_path[..]).unwrap (); 
    let ram = ram_s [..ram_s.len () - 1].parse::<f64> ().unwrap ();

    let cgroup_name = String::from (&dir[VJOULE_PATH.len () .. ]);
    
    let old_cpu : f64 = *cpu_map.get(&cgroup_name).unwrap_or (&0.0);
    let old_ram : f64 = *ram_map.get(&cgroup_name).unwrap_or (&0.0);

    cpu_map.insert (cgroup_name.clone (), cpu);
    ram_map.insert (cgroup_name.clone (), ram);
    
    println!("{} => CPU : {}, Mem : {}", dir, cpu - old_cpu, ram - old_ram);
}

/**
 * Traverse the result tree directory of the formula 
 * @params:
 *    - dir: the current directory to traverse
 *    - cpu_map: the map containing the old values of the package file (previous tick)
 *    - memory_map: the map containing the old values of the memory file (previous tick)
 */
fn read_cgroup_tree (dir : &str, cpu_map : &mut HashMap<String, f64>, ram_map : &mut HashMap<String, f64>) {
    let package_path = format!("{}/package", dir);
    if Path::new (&package_path[..]).exists () {
	read_cgroup_consumption (dir, cpu_map, ram_map);
    }

    for entry in fs::read_dir(dir).unwrap () {
        let entry = entry.unwrap ();
        let path = entry.path();
	
	let md = metadata (&path).unwrap ();
	if md.is_dir () {
	    read_cgroup_tree (&path.into_os_string().into_string().unwrap(), cpu_map, ram_map);
	}
    }
}

/**
 * Example of result pulling
 */
fn main() {
    let mut cpu_map = HashMap::new ();
    let mut ram_map = HashMap::new ();
    
    loop {
	wait_formula_signal ();

	let now = Local::now ();
	
	println!("{}", now.format("%Y-%m-%d %H:%M:%S.%f"));
	read_cgroup_tree (VJOULE_PATH, &mut cpu_map, &mut ram_map);
    }
}
