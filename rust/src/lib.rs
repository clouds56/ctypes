extern crate libc;
use libc::{c_schar as c_char, c_int, c_uint, int64_t, c_double, c_void, size_t};
use std::ffi::{CStr, CString};
use std::slice;
use std::fmt::Debug;
use std::convert::{From, Into};

pub type FuncHandle = *const c_void;
pub type HandleType = *const c_void;
type _PackedType = c_uint;

trait PackedType : From<_PackedType> + Into<_PackedType> { }
pub trait ExtBase: Drop {
    const CODE: u32;
    fn from_raw(ptr: PackedArg) -> Self;
    fn new() -> Self;
    fn release(&mut self);
    fn is_owned(&self) -> bool;
    fn handle(&self) -> HandleType;
}

#[derive(Debug)]
pub enum PackedTypeCode {
    Unknown = 0,
    Int64 = 1,
    Float64 = 2,
    Ptr = 3,
    Str = 4,
    Func = 5,
    Vector = 6,
}

impl Into<_PackedType> for PackedTypeCode {
    fn into(self) -> _PackedType {
        self as _PackedType
    }
}

impl From<_PackedType> for PackedTypeCode {
    fn from(type_code: _PackedType) -> Self {
        match type_code {
            i if i == (PackedTypeCode::Unknown as _PackedType) => PackedTypeCode::Unknown,
            i if i == (PackedTypeCode::Int64 as _PackedType) => PackedTypeCode::Int64,
            i if i == (PackedTypeCode::Float64 as _PackedType) => PackedTypeCode::Float64,
            i if i == (PackedTypeCode::Ptr as _PackedType) => PackedTypeCode::Ptr,
            i if i == (PackedTypeCode::Str as _PackedType) => PackedTypeCode::Str,
            i if i == (PackedTypeCode::Func as _PackedType) => PackedTypeCode::Func,
            i if i == (PackedTypeCode::Vector as _PackedType) => PackedTypeCode::Vector,
            _ => PackedTypeCode::Ptr,
        }
    }
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
struct _PackedVec {
    data: *const _PackedValue,
    size: size_t,
    type_code: _PackedType,
}

#[derive(Debug)]
pub struct ManagedPackedVec {
    packed_vec: _PackedVec,
    values: Vec<_PackedValue>,
    managed: Vec<ManagedPackedArg>,
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
pub enum ManagedPackedArg {
    Base(PackedArg),
    String(CString),
    Vec(ManagedPackedVec),
}

#[derive(Debug)]
pub enum PackedArg {
    Unknown,
    Int64(i64),
    UInt64(u64),
    Float64(f64),
    Ptr(*const c_void),
    String(String),
    Func(PackedFunc),
    Vec(Vec<PackedArg>),
    Ext(c_uint, *const c_void),
}

#[derive(Debug)]
pub struct PackedFunc {
    pub name: Option<String>,
    pub handle: FuncHandle,
}

impl Debug for _PackedValue {
    #[inline]
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        unsafe {
            write!(f, "PackedValue {{ v_ptr: {:?} }}", self.v_ptr)
        }
    }
}

impl Into<Vec<PackedArg>> for _PackedVec {
    fn into(self) -> Vec<PackedArg> {
        unsafe {
            let s = slice::from_raw_parts(self.data, self.size);
            s.iter().map(|&x| PackedArg::from_raw(self.type_code, x)).collect()
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
        impl From<$t> for ManagedPackedArg {
            #[inline]
            fn from(value: $t) -> Self {
                ManagedPackedArg::Base(PackedArg::$n(value as $tt))
            }
        }
        impl Into<$t> for ManagedPackedArg {
            #[inline]
            fn into(self) -> $t {
                match self {
                    ManagedPackedArg::Base(PackedArg::$n(value)) => value as $t,
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
packed_arg_type!(PackedFunc, Func);

impl<'a> From<&'a str> for PackedArg {
    #[inline]
    fn from(value: &'a str) -> Self {
        PackedArg::String(String::from(value))
    }
}

impl Into<String> for PackedArg {
    #[inline]
    fn into(self) -> String {
        match self {
            PackedArg::String(value) => value,
            _ => panic!()
        }
    }
}

impl<'a> From<&'a str> for ManagedPackedArg {
    #[inline]
    fn from(value: &'a str) -> Self {
        ManagedPackedArg::String(CString::new(value).unwrap())
    }
}

impl Into<String> for ManagedPackedArg {
    #[inline]
    fn into(self) -> String {
        match self {
            ManagedPackedArg::String(value) => value.into_string().unwrap(),
            _ => panic!()
        }
    }
}

impl<T> From<Vec<T>> for ManagedPackedArg where ManagedPackedArg: std::convert::From<T> {
    #[inline]
    fn from(value: Vec<T>) -> Self {
        let mut managed = Vec::new();
        for x in value {
            managed.push(ManagedPackedArg::from(x));
        }
        let mut values = Vec::new();
        let mut type_code = PackedTypeCode::Unknown as _PackedType;
        for a in managed.iter() {
            unsafe {
                let aa = a.to_raw();
                if type_code == PackedTypeCode::Unknown as _PackedType {
                    type_code = aa.type_code;
                } else if aa.type_code != type_code {
                    panic!();
                }
                values.push(aa.value);
            }
        }
        let packed_vec = _PackedVec{ data: values.as_ptr(), size: values.len(), type_code };
        let vec = ManagedPackedVec{ packed_vec, values, managed };
        ManagedPackedArg::Vec(vec)
    }
}

impl<T> Into<Vec<T>> for PackedArg where PackedArg: std::convert::Into<T> {
    #[inline]
    fn into(self) -> Vec<T> {
        match self {
            PackedArg::Vec(value) => {
                let mut ret = Vec::new();
                for x in value {
                    ret.push(x.into());
                }
                ret
            },
            _ => panic!()
        }
    }
}

impl<'a, T: ExtBase> From<&'a T> for ManagedPackedArg {
    #[inline]
    fn from(value: &'a T) -> Self {
        ManagedPackedArg::Base(PackedArg::Ext(T::CODE, value.handle()))
    }
}

impl Into<HandleType> for PackedArg {
    #[inline]
    fn into(self) -> HandleType {
        match self {
            PackedArg::Ptr(value) => value as HandleType,
            PackedArg::Ext(_, value) => value as HandleType,
            _ => panic!(),
        }
    }
}

impl Into<()> for PackedArg {
    #[inline]
    fn into(self) { }
}

impl PackedArg {
    #[inline]
    unsafe fn to_raw(&self) -> _PackedArg {
        match self {
            &PackedArg::Int64(i) => _PackedArg{ type_code: PackedTypeCode::Int64 as _PackedType, value: _PackedValue{ v_int64: i } },
            &PackedArg::UInt64(i) => _PackedArg{ type_code: PackedTypeCode::Int64 as _PackedType, value: _PackedValue{ v_int64: i as i64 } },
            &PackedArg::Float64(i) => _PackedArg{ type_code: PackedTypeCode::Float64 as _PackedType, value: _PackedValue{ v_float64: i } },
            &PackedArg::Ptr(i) => _PackedArg{ type_code: PackedTypeCode::Ptr as _PackedType, value: _PackedValue{ v_ptr: i } },
            &PackedArg::Func(ref i) => _PackedArg{ type_code: PackedTypeCode::Func as _PackedType, value: _PackedValue{ v_func: i.handle } },
            &PackedArg::Ext(c, i) => _PackedArg{ type_code: c, value: _PackedValue{ v_ptr: i } },
            _ => _PackedArg{ type_code: PackedTypeCode::Unknown as _PackedType, value: _PackedValue{ v_int64: 0 } }
        }
    }
}

impl ManagedPackedArg {
    #[inline]
    unsafe fn to_raw(&self) -> _PackedArg {
        match self {
            &ManagedPackedArg::Base(ref base) => base.to_raw(),
            &ManagedPackedArg::String(ref i) => _PackedArg{ type_code: PackedTypeCode::Str as _PackedType, value: _PackedValue{ v_str: i.as_ptr() } },
            &ManagedPackedArg::Vec(ref i) => _PackedArg{ type_code: PackedTypeCode::Vector as _PackedType, value: _PackedValue{ v_vec: &i.packed_vec } },
        }
    }
}

impl PackedArg {
    #[inline]
    unsafe fn from_raw(type_code: _PackedType, value: _PackedValue) -> Self {
        match type_code.into() {
            PackedTypeCode::Int64 => PackedArg::Int64(value.v_int64),
            PackedTypeCode::Float64 => PackedArg::Float64(value.v_float64),
            PackedTypeCode::Str => PackedArg::String(CStr::from_ptr(value.v_str).to_owned().into_string().unwrap()),
            PackedTypeCode::Func => PackedArg::Func(PackedFunc{ handle: value.v_func, name: None }),
            PackedTypeCode::Vector => PackedArg::Vec((*value.v_vec).into()),
            PackedTypeCode::Ptr => {
                if type_code == PackedTypeCode::Ptr as _PackedType {
                    PackedArg::Ptr(value.v_ptr)
                } else {
                    PackedArg::Ext(type_code, value.v_ptr)
                }

            },
            x => panic!("type code {:?} unknown", x)//PackedArg::Unknown,
        }
    }
}

extern "C" {
    fn CTIRegistryListNames(tag: *const c_char, ret_size: *mut size_t, ret_names: *mut*const*const c_char) -> c_int;
    fn CTIRegistryGet(tag: *const c_char, name: *const c_char, handle: *mut FuncHandle) -> c_int;
    fn CTIPackedFuncCall(name: FuncHandle, num_args: size_t,
                         type_codes: *const _PackedType, values: *const _PackedValue,
                         ret_type: *mut _PackedType, ret_val: *mut _PackedValue) -> c_int;
}

pub fn registry_list_names(tag: &str) -> Vec<String> {
    let mut ret_size: size_t = 0;
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
        PackedFunc { name: Some(String::from(name)), handle }
    }
}

impl PackedFunc {
    pub unsafe fn call(&self, args: Vec<ManagedPackedArg>) -> PackedArg {
        let num_args = args.len();
        let mut type_codes = Vec::new();
        let mut values = Vec::new();
        let mut ret_type: _PackedType = 0;
        let mut ret_val = _PackedValue { v_int64: 0 };
        for arg in args.iter() {
            let _arg = arg.to_raw();
            type_codes.push(_arg.type_code);
            values.push(_arg.value);
        }
        CTIPackedFuncCall(self.handle, num_args, type_codes.as_ptr(), values.as_ptr(), &mut ret_type, &mut ret_val);
        PackedArg::from_raw(ret_type, ret_val)
    }
}

#[macro_export]
macro_rules! vec_packed_arg {
    ($($x:expr),*) => (vec![$(ManagedPackedArg::from($x),)*])
}

#[macro_export]
macro_rules! packed_call {
    ($f:expr) => (unsafe{ $f.call(vec!()) }.into());
    ($f:expr, $($x:expr),*) => (unsafe{ $f.call(vec_packed_arg!($($x),*)) }.into());
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

    #[test]
    fn it_list() {
        let vector_add = registry_get("PackedFunc", "vector_add");
        let result: Vec<Vec<i32>> = packed_call!(vector_add, vec!(vec!(1,2,3), vec!(4)), vec!(1,2,3,4));
        println!("vector_add: {:?}", result);
        assert_eq!(result, vec!(vec!(11,12,13), vec!(14)));
    }
}
