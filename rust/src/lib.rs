extern crate libc;
use libc::{c_schar as c_char, c_int, c_uint, int64_t, c_double, c_void, size_t};
use std::ffi::{CStr, CString};
use std::slice;
use std::fmt::Debug;
use std::convert::{From, Into};

pub type FuncHandle = *const c_void;
pub type PackedType = c_uint;

pub enum PackedTypeCode {
    Unknown = 0,
    Int64 = 1,
    Float64 = 2,
    Ptr = 3,
    Str = 4,
    Func = 5,
    Vector = 6,
}

impl Into<PackedType> for PackedTypeCode {
    fn into(self) -> PackedType {
        self as PackedType
    }
}


#[repr(C)]
#[derive(Debug)]
pub struct _PackedVec {
    data: *const PackedValue,
    size: size_t,
    type_code: PackedType,
}

#[repr(C)]
pub union PackedValue {
    v_int64: int64_t,
    v_float64: c_double,
    v_ptr: *const c_void,
    v_str: *const c_char,
    v_func: FuncHandle,
    v_vec: *const _PackedVec,
}

#[derive(Debug)]
pub struct PackedArg {
    type_code: PackedType,
    value: PackedValue,
}

impl Debug for PackedValue {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        unsafe {
            write!(f, "PackedValue {{ v_ptr: {:?} }}", self.v_ptr)
        }
    }
}


impl PackedArg {
    pub fn check(&self, type_code: PackedType) {
        assert_eq!(type_code, self.type_code)
    }
}

macro_rules! packed_arg_type {
    ( $t:ty, $c:expr, $n:ident ) => {
        impl From<$t> for PackedArg {
            #[inline]
            fn from(value: $t) -> Self {
                PackedArg{ type_code: $c as PackedType, value: PackedValue{ $n: value } }
            }
        }
        impl Into<$t> for PackedArg {
            #[inline]
            fn into(self) -> $t {
                self.check($c as PackedType);
                unsafe {
                    self.value.$n
                }
            }
        }
    };
}

packed_arg_type!(i64, PackedTypeCode::Int64, v_int64);
packed_arg_type!(f64, PackedTypeCode::Float64, v_float64);
impl PackedArg {
    pub fn from_raw(type_code: PackedType, value: PackedValue) -> Self {
        PackedArg { type_code, value }
    }
}

#[derive(Debug)]
pub struct PackedFunc {
    name: String,
    handle: FuncHandle,
}

extern "C" {
    fn CTIRegistryListNames(tag: *const c_char, ret_size: *mut c_int, ret_names: *mut*const*const c_char) -> c_int;
    fn CTIRegistryGet(tag: *const c_char, name: *const c_char, handle: *mut FuncHandle) -> c_int;
    fn CTIPackedFuncCall(name: FuncHandle, num_args: c_int,
                         type_codes: *const PackedType, values: *const PackedValue,
                         ret_type: *mut PackedType, ret_val: *mut PackedValue) -> c_int;
}

pub fn registry_list_names(tag: String) -> Vec<String> {
    let mut ret_size: c_int = 0;
    let mut ret_names: *const*const c_char = std::ptr::null();
    let mut ret = Vec::new();
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

pub fn registry_get(tag: String, name: String) -> PackedFunc {
    let _tag = CString::new(tag).unwrap();
    let _name = CString::new(name.clone()).unwrap();
    let mut handle: FuncHandle = std::ptr::null();
    unsafe {
        CTIRegistryGet(_tag.as_ptr(), _name.as_ptr(), &mut handle);
        PackedFunc { name, handle }
    }
}

impl PackedFunc {
    fn call(&self, args: Vec<PackedArg>) -> PackedArg {
        let num_args = args.len() as i32;
        let mut type_codes = Vec::new();
        let mut values = Vec::new();
        for arg in args {
            type_codes.push(arg.type_code);
            values.push(arg.value);
        }
        let mut ret_type: PackedType = 0;
        let mut ret_val = PackedValue{ v_int64: 0 };
        unsafe {
            CTIPackedFuncCall(self.handle, num_args, type_codes.as_ptr(), values.as_ptr(), &mut ret_type, &mut ret_val);
        }
        PackedArg::from_raw(ret_type, ret_val)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn it_works() {
        println!("list names: {:?}", registry_list_names(String::from("PackedFunc")));
        let hello = registry_get(String::from("PackedFunc"), String::from("hello"));
        println!("get func: {:?}", hello);
        println!("arg: {:?}", PackedArg::from(1.));
        let result: i64 = hello.call(vec!(PackedArg::from(1), PackedArg::from(2))).into();
        println!("hello: 1+2={:?}", result);
        assert_eq!(result, 3);
    }
}

