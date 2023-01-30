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

const VJOULE_PATH : &str = "/etc/vjoule/results/";

/**
 * Wait for a result written by the service
 * This is the correct way of doing it as it is a passive wait
 */
fn wait_service_iteration () {
    let mut inotify = Inotify::init ()
	.expect ("Error while initializing inotify instance");

    inotify.add_watch (
	format!("{}/cpu", VJOULE_PATH),
	WatchMask::MODIFY
    ).expect ("Failed to add watch");

    let mut buffer = [0 ; 1024];
    inotify.read_events_blocking (&mut buffer)
	.expect ("Error while reading events");
}

/**
 * Reading the package and memory result file of a cgroup
 * @params:
 *    - dir: the directory of the cgroup result 
 *    - cpu_map: the map containing the old values of the cpu file (previous tick)
 *    - ram_map: the map containing the old values of the ram file (previous tick)
 *    - gpu_map: the map containing the old values of the gpu file (previous tick)
 */
fn read_cgroup_consumption (dir : &str, cpu_map : &mut HashMap<String, f64>, ram_map : &mut HashMap<String, f64>, gpu_map : &mut HashMap<String, f64>) {
    let package_path = format!("{}/cpu", dir);
    let ram_path = format!("{}/ram", dir);
    let gpu_path = format!("{}/gpu", dir);

    let cpu_s = fs::read_to_string(&package_path[..]).unwrap (); 
    let cpu = cpu_s [..cpu_s.len () - 1].parse::<f64> ().unwrap ();

    let ram_s = fs::read_to_string(&ram_path[..]).unwrap (); 
    let ram = ram_s [..ram_s.len () - 1].parse::<f64> ().unwrap ();

    let gpu_s = fs::read_to_string(&gpu_path[..]).unwrap (); 
    let gpu = gpu_s [..gpu_s.len () - 1].parse::<f64> ().unwrap ();

    let cgroup_name = String::from (&dir[VJOULE_PATH.len () .. ]);
    
    let old_cpu : f64 = *cpu_map.get(&cgroup_name).unwrap_or (&0.0);
    let old_ram : f64 = *ram_map.get(&cgroup_name).unwrap_or (&0.0);
    let old_gpu : f64 = *gpu_map.get(&cgroup_name).unwrap_or (&0.0);

    cpu_map.insert (cgroup_name.clone (), cpu);
    ram_map.insert (cgroup_name.clone (), ram);
    gpu_map.insert (cgroup_name.clone (), gpu);

    if cgroup_name == "" {
	println!("SYSTEM => CPU : {:.3}W, RAM : {:.3}W, GPU : {:.3}W", cpu - old_cpu, ram - old_ram, gpu - old_gpu);
    } else {
	println!("{} => CPU : {:.3}W, RAM : {:.3}W, GPU : {:.3}W", cgroup_name, cpu - old_cpu, ram - old_ram, gpu - old_gpu);
    }
}

/**
 * Traverse the result tree directory of the formula 
 * @params:
 *    - dir: the current directory to traverse
 *    - cpu_map: the map containing the old values of the cpu file (previous tick)
 *    - ram_map: the map containing the old values of the ram file (previous tick)
 *    - gpu_map: the map containing the old values of the gpu file (previous tick)
 */
fn read_cgroup_tree (dir : &str, cpu_map : &mut HashMap<String, f64>, ram_map : &mut HashMap<String, f64>, gpu_map : &mut HashMap<String, f64>) {
    let package_path = format!("{}/cpu", dir);
    if Path::new (&package_path[..]).exists () {
	read_cgroup_consumption (dir, cpu_map, ram_map, gpu_map);
    }

    for entry in fs::read_dir(dir).unwrap () {
        let entry = entry.unwrap ();
        let path = entry.path();
	
	let md = metadata (&path).unwrap ();
	if md.is_dir () {
	    read_cgroup_tree (&path.into_os_string().into_string().unwrap(), cpu_map, ram_map, gpu_map);
	}
    }
}

/**
 * Example of result pulling
 */
fn main() {
    let mut cpu_map = HashMap::new ();
    let mut ram_map = HashMap::new ();
    let mut gpu_map = HashMap::new ();
    
    loop {
	wait_service_iteration ();

	let now = Local::now ();
	
	println!("{}", now.format("%Y-%m-%d %H:%M:%S.%f"));
	read_cgroup_tree (VJOULE_PATH, &mut cpu_map, &mut ram_map, &mut gpu_map);
    }
}
