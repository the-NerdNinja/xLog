use std::io::{self, Write};
//use std::process::Command;

pub fn promt(text: &str) -> String{
    print!("{}: ", text);
    io::stdout()
        .flush()
        .expect("Failed to flush stdout");

    let mut inp = String::new();
    io::stdin()
        .read_line(&mut inp)
        .expect("Failed to read input");

    inp.trim().to_string()
}
