use std::fs::File;
use std::io::BufRead;

pub fn read_lines(filename: String) -> Vec<String> {
    // Open the file in read-only mode.
    let file = File::open(filename).unwrap(); 
    // Read the file line by line, and return an iterator of the lines of the file.
    let lines = std::io::BufReader::new(file).lines();
    let mut v = Vec::new ();
    
    for l in lines {
	    let mut i = String::from (&l.unwrap ()[..]);
	    if i.len () > 1 && &i [i.len () - 1 ..] == "\n" {
	        i = String::from (&i[.. i.len () - 1]);
	    }

	    if i != "" {
	        v.push (i);
	    }
    }

    v
}
