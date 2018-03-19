extern crate libc;
use libc::{c_schar as c_char, c_int, c_uint, int64_t, c_double, c_void, size_t};
use std::ffi::{CStr, CString};
use std::slice;
use std::fmt::Debug;
use std::convert::{From, Into};

type FuncHandle = *const c_void;
type _PackedType = c_uint;

pub enum PackedType {
    Unknown = 0,
    Int64 = 1,
    Float64 = 2,
    Ptr = 3,
    Str = 4,
    Func = 5,
    Vector = 6,
}

impl Into<_PackedType> for PackedType {
    fn into(self) -> _PackedType {
        self as _PackedType
    }
}

impl From<_PackedType> for PackedType {
    fn from(type_code: _PackedType) -> Self {
        match type_code {
            i if i == (PackedType::Int64 as _PackedType) => PackedType::Int64,
            i if i == (PackedType::Float64 as _PackedType) => PackedType::Float64,
            i if i == (PackedType::Ptr as _PackedType) => PackedType::Ptr,
            i if i == (PackedType::Str as _PackedType) => PackedType::Str,
            i if i == (PackedType::Func as _PackedType) => PackedType::Func,
            i if i == (PackedType::Vector as _PackedType) => PackedType::Vector,
            _ => PackedType::Unknown,
        }
    }
}

#[repr(C)]
#[derive(Debug)]
struct _PackedVec {
    data: *const _PackedValue,
    size: size_t,
    type_code: _PackedType,
}

#[repr(C)]
#[derive(Copy, Clone)]
union _PackedValue {
    v_int64: int64_t,
    v_float64: c_double,
    v_ptr: *const c_void,
    v_str: *const c_char,
    v_func: FuncHandle,
    v_vec: *const _PackedVec,
}

#[derive(Debug, Copy, Clone)]
struct _PackedArg {
    type_code: _PackedType,
    value: _PackedValue,
}

#[derive(Debug)]
pub enum PackedArg {
    Unknown,
    Int64(i64),
    UInt64(u64),
    Float64(f64),
    String(CString),
    Func(PackedFunc),
    Vec(Vec<PackedArg>),
}

#[derive(Debug)]
pub struct PackedFunc {
    name: String,
    handle: FuncHandle,
}

impl Debug for _PackedValue {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        unsafe {
            write!(f, "PackedValue {{ v_ptr: {:?} }}", self.v_ptr)
        }
    }
}

macro_rules! packed_arg_type {
    ( $t:ty, $tt:ty, $n:ident ) => {
        impl From<$t> for PackedArg {
            #[inline]
            fn from(value: $t) -> Self {
                PackedArg::$n(value as $tt)
            }
        }
        impl Into<$t> for PackedArg {
            #[inline]
            fn into(self) -> $t {
                match self {
                    PackedArg::$n(value) => value as $t,
                    _ => panic!()
                }
            }
        }
    };
    ( $t:ty, $n:ident ) => {
        packed_arg_type!($t, $t, $n);
    };
}

packed_arg_type!(i32, i64, Int64);
packed_arg_type!(i64, Int64);
packed_arg_type!(u64, UInt64);
packed_arg_type!(f64, Float64);
packed_arg_type!(CString, String);
packed_arg_type!(PackedFunc, Func);

impl<'a> From<&'a str> for PackedArg {
    #[inline]
    fn from(value: &'a str) -> Self {
        PackedArg::String(CString::new(value).unwrap())
    }
}

impl Into<String> for PackedArg {
    #[inline]
    fn into(self) -> String {
        match self {
            PackedArg::String(value) => value.into_string().unwrap(),
            _ => panic!()
        }
    }
}

impl PackedArg {
    #[inline]
    unsafe fn to_raw(&self) -> _PackedArg {
        match self {
            &PackedArg::Int64(i) => _PackedArg{ type_code: PackedType::Int64 as _PackedType, value: _PackedValue{ v_int64: i } },
            &PackedArg::UInt64(i) => _PackedArg{ type_code: PackedType::Int64 as _PackedType, value: _PackedValue{ v_int64: i as i64 } },
            &PackedArg::Float64(i) => _PackedArg{ type_code: PackedType::Float64 as _PackedType, value: _PackedValue{ v_float64: i } },
            &PackedArg::String(ref i) => _PackedArg{ type_code: PackedType::Str as _PackedType, value: _PackedValue{ v_str: i.as_ptr() } },
            &PackedArg::Func(ref i) => _PackedArg{ type_code: PackedType::Func as _PackedType, value: _PackedValue{ v_func: i.handle } },
            _ => panic!()//_PackedArg{ type_code: PackedType::Unknown as _PackedType, value: _PackedValue{ v_int64: 0 } }
        }
    }
}

impl PackedArg {
    unsafe fn from_raw(type_code: _PackedType, value: _PackedValue) -> Self {
        match PackedType::from(type_code) {
            PackedType::Int64 => PackedArg::Int64(value.v_int64),
            PackedType::Float64 => PackedArg::Float64(value.v_float64),
            PackedType::Str => PackedArg::String(CStr::from_ptr(value.v_str).to_owned()),
            PackedType::Func => PackedArg::Func(PackedFunc{ handle: value.v_func, name: String::from("") }),
            _ => panic!()//PackedArg::Unknown,
        }
    }
}

extern "C" {
    fn CTIRegistryListNames(tag: *const c_char, ret_size: *mut c_int, ret_names: *mut*const*const c_char) -> c_int;
    fn CTIRegistryGet(tag: *const c_char, name: *const c_char, handle: *mut FuncHandle) -> c_int;
    fn CTIPackedFuncCall(name: FuncHandle, num_args: c_int,
                         type_codes: *const _PackedType, values: *const _PackedValue,
                         ret_type: *mut _PackedType, ret_val: *mut _PackedValue) -> c_int;
}

pub fn registry_list_names(tag: &str) -> Vec<String> {
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

pub fn registry_get(tag: &str, name: &str) -> PackedFunc {
    let _tag = CString::new(tag).unwrap();
    let _name = CString::new(name).unwrap();
    let mut handle: FuncHandle = std::ptr::null();
    unsafe {
        CTIRegistryGet(_tag.as_ptr(), _name.as_ptr(), &mut handle);
        PackedFunc { name: name.to_owned(), handle }
    }
}

impl PackedFunc {
    pub fn call(&self, args: Vec<PackedArg>) -> PackedArg {
        let num_args = args.len() as i32;
        let mut type_codes = Vec::new();
        let mut values = Vec::new();
        let mut ret_type: _PackedType = 0;
        let mut ret_val = _PackedValue { v_int64: 0 };
        unsafe {
            for arg in args.iter() {
                let _arg = arg.to_raw();
                type_codes.push(_arg.type_code);
                values.push(_arg.value);
            }
            CTIPackedFuncCall(self.handle, num_args, type_codes.as_ptr(), values.as_ptr(), &mut ret_type, &mut ret_val);
            PackedArg::from_raw(ret_type, ret_val)
        }
    }
}

#[macro_export]
macro_rules! vec_packed_arg {
    ($($x:expr),*) => (vec![$(PackedArg::from($x),)*])
}

#[macro_export]
macro_rules! packed_call {
    ($f:expr, $($x:expr),*) => ($f.call(vec_packed_arg!($($x),*)).into())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn it_works() {
        println!("list names: {:?}", registry_list_names("PackedFunc"));
        let hello = registry_get("PackedFunc", "hello");
        println!("get func: {:?}", hello);
        println!("arg: {:?}", PackedArg::from(1.));
        let result: i64 = packed_call!(hello, 1, 2);
        println!("hello: 1+2={:?}", result);
        assert_eq!(result, 3);
    }

    #[test]
    fn it_appends() {
        let append_str = registry_get("PackedFunc", "append_str");
        let result_: String = packed_call!(append_str, "hello", "world");
        println!("append_str: {}", result_);

        let test_append_str = registry_get("PackedFunc", "test_append_str");
        let result: String = packed_call!(test_append_str, append_str, "append", "str");
        println!("test_append_str: {}", result);
        assert_eq!(result, "append str");

    }
}

