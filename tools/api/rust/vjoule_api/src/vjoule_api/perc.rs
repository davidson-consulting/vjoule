
use std::fmt;


/**
 * A consumption percentage between two consumption diffs
 */
#[derive(Debug, Copy, Clone)]
pub struct ConsumptionPerc {
    pub duration : f64,
    pub cpu : f64,
    pub gpu : f64,
    pub ram : f64
}

impl fmt::Display for ConsumptionPerc {
    
    fn fmt (&self, formatter : &mut fmt::Formatter)-> fmt::Result {
	let r = format!("diff ({:.2}%, cpu: {:.2}%, ram: {:.2}%, gpu: {:.2}%)", self.duration, self.cpu, self.ram, self.gpu);
	formatter.write_str (&r)
    }	
}
