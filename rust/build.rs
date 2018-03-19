use std::path::Path;
use std::env;

fn main() {
    let dir = env::var("CARGO_MANIFEST_DIR").unwrap();
    let ld_dir =  Path::new(&dir).join("..").join("build").to_str().unwrap().to_owned();
    println!("cargo:rustc-link-search=native={}", ld_dir);
    println!("cargo:rustc-link-lib={}", "test_ctypes");
    // println!("cargo:rustc-env=RUSTFLAGS=-Clink-args=-Xlinker -rpath -Xlinker {}", ld_dir);
}