
use std::time::{Duration, SystemTime};

use std::fmt;
use std::ops;

use crate::vjoule_api::diff::{ConsumptionDiff};

/**
 * A consumption stamp is the consumption read for a process group, or the whole machine at a given timestamp
 */
#[derive(Debug, Copy, Clone)]
pub struct ConsumptionStamp {
    pub time : SystemTime,
    pub cpu : f64,
    pub gpu : f64,
    pub ram : f64    
}


impl fmt::Display for ConsumptionStamp {

    fn fmt (&self, formatter : &mut fmt::Formatter)-> fmt::Result {
	let r = format!("stamp ({:?}, cpu: {:.2}J, ram: {:.2}J, gpu: {:.2}J)", self.time, self.cpu, self.ram, self.gpu);
	formatter.write_str (&r)
    }	
    
}

impl ops::Sub<ConsumptionStamp> for ConsumptionStamp {
    type Output = ConsumptionDiff;
    
    fn sub (self, other : ConsumptionStamp)-> ConsumptionDiff {
	ConsumptionDiff {
	    duration : match self.time.duration_since (other.time) {
		Ok (dur) => { dur }
		_ => { Duration::new (0, 0) }
	    },
	    cpu : self.cpu - other.cpu,
	    ram : self.ram - other.ram,
	    gpu : self.gpu - other.gpu	    
	}
    }    
}
