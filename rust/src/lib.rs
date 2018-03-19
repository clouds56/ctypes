extern crate libc;
use libc::{c_int};
use std::os::raw::c_char;
use std::ffi::{CStr, CString};
use std::slice;

extern "C" {
    fn CTIRegistryListNames(tag: *const c_char, ret_size: *mut c_int, ret_names: *mut*const*const c_char) -> c_int;
}

pub fn registry_list_names(tag: String) -> Vec<String> {
    let mut ret_size: c_int = 0;
    let mut ret_names: *const*const c_char = std::ptr::null();
    let mut ret: Vec<String> = Vec::new();
    let _tag = CString::new(tag).unwrap();
    unsafe {
        CTIRegistryListNames(_tag.as_ptr(), &mut ret_size, &mut ret_names);
        let rn = slice::from_raw_parts(ret_names, ret_size as usize);
        for n in rn.iter() {
            ret.push(CStr::from_ptr(*n).to_str().unwrap().to_owned());
        }
    }
    ret
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn it_works() {
        assert_eq!(2 + 2, 4);
        println!("list names: {:?}", registry_list_names(String::from("PackedFunc")));
    }
}

