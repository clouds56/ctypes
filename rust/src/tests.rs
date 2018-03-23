#![cfg(test)]

use super::*;
use std::path::PathBuf;

const LIB_PATH: &'static str = "../build/";
const LIB_NAME: &'static str = "test_ctypes";

fn _find_lib() -> PathBuf {
    Lib::find_lib(LIB_PATH, LIB_NAME).unwrap()
}

#[test]
fn it_works() {
    let lib: Lib = Lib::open(_find_lib().as_ref()).unwrap();
    println!("list names: {:?}", lib.registry_list_names("PackedFunc"));
    let hello = lib.registry_get("PackedFunc", "hello");
    println!("get func: {:?}", hello);
    println!("arg: {:?}", PackedArg::from(1.));
    let result: i64 = packed_call!(hello, 1, 2);
    println!("hello: 1+2={:?}", result);
    assert_eq!(result, 3);
}

#[test]
fn it_appends() {
    let lib: Lib = Lib::open(_find_lib().as_ref()).unwrap();
    let append_str = lib.registry_get("PackedFunc", "append_str");
    let result_: String = packed_call!(append_str, "hello", "world");
    println!("append_str: {}", result_);

    let test_append_str = lib.registry_get("PackedFunc", "test_append_str");
    let result: String = packed_call!(test_append_str, append_str, "append", "str");
    println!("test_append_str: {}", result);
    assert_eq!(result, "append str");
}

#[test]
fn it_list() {
    let lib: Lib = Lib::open(_find_lib().as_ref()).unwrap();
    let vector_add = lib.registry_get("PackedFunc", "vector_add");
    let result: Vec<Vec<i32>> = packed_call!(vector_add, vec!(vec!(1,2,3), vec!(4)), vec!(1,2,3,4));
    println!("vector_add: {:?}", result);
    assert_eq!(result, vec!(vec!(11,12,13), vec!(14)));
}