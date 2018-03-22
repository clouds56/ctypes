extern crate libc;
#[macro_use]
extern crate shared_library;

use libc::{c_schar as c_char, c_int, c_uint, int64_t, c_double, c_void, size_t};
use std::ffi::{CStr, CString};
use std::slice;
use std::fmt::Debug;
use std::convert::{From, Into};

pub type FuncHandle = *const c_void;
pub type HandleType = *const c_void;
pub type _PackedType = c_uint;

trait PackedType : From<_PackedType> + Into<_PackedType> { }
pub trait PackedExt<'lib>: Drop {
    const CODE: u32;
    fn from_raw(lib: &'lib Lib, ptr: PackedArg) -> Self;
    fn new(lib: &'lib Lib) -> Self;
    fn release(&mut self);
    fn is_owned(&self) -> bool;
    fn handle(&self) -> HandleType;
}

pub trait _PackedExt<'lib>: Drop {
    fn _new(lib: &'lib Lib) -> HandleType;
    fn _release(&self);
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
pub struct ManagedPackedVec<'lib> {
    packed_vec: _PackedVec,
    values: Vec<_PackedValue>,
    managed: Vec<ManagedPackedArg<'lib>>,
}

#[repr(C)]
#[derive(Copy, Clone)]
pub union _PackedValue {
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
pub enum ManagedPackedArg<'lib> {
    Base(PackedArg<'lib>),
    String(CString),
    Vec(ManagedPackedVec<'lib>),
}

#[derive(Debug)]
pub enum PackedArg<'lib> {
    Unknown,
    Int64(i64),
    UInt64(u64),
    Float64(f64),
    Ptr(*const c_void),
    String(String),
    Func(PackedFunc<'lib>),
    Vec(Vec<PackedArg<'lib>>),
    Ext(c_uint, *const c_void),
}

#[derive(Debug)]
pub struct PackedFunc<'lib> {
    pub name: Option<String>,
    pub handle: FuncHandle,
    pub lib: &'lib Lib,
}

impl Debug for _PackedValue {
    #[inline]
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        unsafe {
            write!(f, "PackedValue {{ v_ptr: {:?} }}", self.v_ptr)
        }
    }
}

impl _PackedVec {
    fn into_vec<'lib>(self, lib: &'lib Lib) -> Vec<PackedArg<'lib>> {
        unsafe {
            let s = slice::from_raw_parts(self.data, self.size);
            s.iter().map(|&x| PackedArg::from_raw(lib, self.type_code, x)).collect()
        }
    }
}

macro_rules! packed_arg_type {
    ( $t:ty, $tt:ty, $n:ident ) => {
        impl<'lib> From<$t> for PackedArg<'lib> {
            #[inline]
            fn from(value: $t) -> Self {
                PackedArg::$n(value as $tt)
            }
        }
        impl<'lib> Into<$t> for PackedArg<'lib> {
            #[inline]
            fn into(self) -> $t {
                match self {
                    PackedArg::$n(value) => value as $t,
                    _ => panic!()
                }
            }
        }
        impl<'lib> From<$t> for ManagedPackedArg<'lib> {
            #[inline]
            fn from(value: $t) -> Self {
                ManagedPackedArg::Base(PackedArg::$n(value as $tt))
            }
        }
        impl<'lib> Into<$t> for ManagedPackedArg<'lib> {
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
packed_arg_type!(PackedFunc<'lib>, Func);

impl<'a, 'lib> From<&'a str> for PackedArg<'lib> {
    #[inline]
    fn from(value: &'a str) -> Self {
        PackedArg::String(String::from(value))
    }
}

impl<'lib> Into<String> for PackedArg<'lib> {
    #[inline]
    fn into(self) -> String {
        match self {
            PackedArg::String(value) => value,
            _ => panic!()
        }
    }
}

impl<'a, 'lib> From<&'a str> for ManagedPackedArg<'lib> {
    #[inline]
    fn from(value: &'a str) -> Self {
        ManagedPackedArg::String(CString::new(value).unwrap())
    }
}

impl<'lib> Into<String> for ManagedPackedArg<'lib> {
    #[inline]
    fn into(self) -> String {
        match self {
            ManagedPackedArg::String(value) => value.into_string().unwrap(),
            _ => panic!()
        }
    }
}

impl<'lib, T> From<Vec<T>> for ManagedPackedArg<'lib> where ManagedPackedArg<'lib>: std::convert::From<T> {
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

impl<'lib, T> Into<Vec<T>> for PackedArg<'lib> where PackedArg<'lib>: std::convert::Into<T> {
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

impl<'a, 'lib, T: PackedExt<'a>> From<&'a T> for ManagedPackedArg<'lib> {
    #[inline]
    fn from(value: &'a T) -> Self {
        ManagedPackedArg::Base(PackedArg::Ext(T::CODE, value.handle()))
    }
}

impl<'lib> Into<HandleType> for PackedArg<'lib> {
    #[inline]
    fn into(self) -> HandleType {
        match self {
            PackedArg::Ptr(value) => value as HandleType,
            PackedArg::Ext(_, value) => value as HandleType,
            _ => panic!(),
        }
    }
}

impl<'lib> Into<()> for PackedArg<'lib> {
    #[inline]
    fn into(self) { }
}

impl<'lib> PackedArg<'lib> {
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

impl<'lib> ManagedPackedArg<'lib> {
    #[inline]
    unsafe fn to_raw(&self) -> _PackedArg {
        match self {
            &ManagedPackedArg::Base(ref base) => base.to_raw(),
            &ManagedPackedArg::String(ref i) => _PackedArg{ type_code: PackedTypeCode::Str as _PackedType, value: _PackedValue{ v_str: i.as_ptr() } },
            &ManagedPackedArg::Vec(ref i) => _PackedArg{ type_code: PackedTypeCode::Vector as _PackedType, value: _PackedValue{ v_vec: &i.packed_vec } },
        }
    }
}

impl<'lib> PackedArg<'lib> {
    #[inline]
    unsafe fn from_raw(lib: &'lib Lib, type_code: _PackedType, value: _PackedValue) -> Self {
        match type_code.into() {
            PackedTypeCode::Int64 => PackedArg::Int64(value.v_int64),
            PackedTypeCode::Float64 => PackedArg::Float64(value.v_float64),
            PackedTypeCode::Str => PackedArg::String(CStr::from_ptr(value.v_str).to_owned().into_string().unwrap()),
            PackedTypeCode::Func => PackedArg::Func(PackedFunc{ handle: value.v_func, name: None, lib }),
            PackedTypeCode::Vector => PackedArg::Vec((*value.v_vec).into_vec(lib)),
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

shared_library!(Lib,
    fn CTIRegistryListNames(tag: *const c_char, ret_size: *mut size_t, ret_names: *mut*const*const c_char) -> c_int,
    fn CTIRegistryGet(tag: *const c_char, name: *const c_char, handle: *mut FuncHandle) -> c_int,
    fn CTIPackedFuncCall(name: FuncHandle, num_args: size_t,
                         type_codes: *const _PackedType, values: *const _PackedValue,
                         ret_type: *mut _PackedType, ret_val: *mut _PackedValue) -> c_int,
);

impl Lib {
    pub fn registry_list_names(&self, tag: &str) -> Vec<String> {
        let mut ret_size: size_t = 0;
        let mut ret_names: *const *const c_char = std::ptr::null();
        let mut ret = Vec::new();
        let _tag = CString::new(tag).unwrap();
        unsafe {
            (self.CTIRegistryListNames)(_tag.as_ptr(), &mut ret_size, &mut ret_names);
            let rn = slice::from_raw_parts(ret_names, ret_size as usize);
            for n in rn.iter() {
                ret.push(CStr::from_ptr(*n).to_str().unwrap().to_owned());
            }
        }
        ret
    }

    pub fn registry_get(&self, tag: &str, name: &str) -> PackedFunc {
        let _tag = CString::new(tag).unwrap();
        let _name = CString::new(name).unwrap();
        let mut handle: FuncHandle = std::ptr::null();
        unsafe {
            (self.CTIRegistryGet)(_tag.as_ptr(), _name.as_ptr(), &mut handle);
            PackedFunc { name: Some(String::from(name)), handle, lib: &self }
        }
    }

    pub unsafe fn func_call(&self, func: FuncHandle, args: Vec<ManagedPackedArg>) -> PackedArg {
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
        (self.CTIPackedFuncCall)(func, num_args, type_codes.as_ptr(), values.as_ptr(), &mut ret_type, &mut ret_val);
        PackedArg::from_raw(&self,ret_type, ret_val)
    }
}

impl<'lib> PackedFunc<'lib> {
    pub unsafe fn call(&self, args: Vec<ManagedPackedArg>) -> PackedArg {
        self.lib.func_call(self.handle, args)
    }
}

impl Debug for Lib {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        write!(f, "Lib {{ }}")
    }
}

#[macro_export]
macro_rules! vec_packed_arg {
    ($($x:expr),*) => (vec![$(ManagedPackedArg::from($x),)*])
}

#[macro_export]
macro_rules! packed_call {
    ($f:expr $(,$x:expr)*) => (unsafe{ $f.call(vec_packed_arg!($($x),*)) }.into());
}

#[macro_export]
macro_rules! impl_packed_ext {
    ( $n:ident, $c:expr ) => {
        impl<'lib> PackedExt<'lib> for $n<'lib> {
            const CODE: u32 = $c;

            fn from_raw(lib: &'lib Lib, ptr: PackedArg) -> Self {
                ExtTest{ handle: ptr.into(), is_owned: false, lib }
            }

            fn new(lib: &'lib Lib) -> Self {
                ExtTest{ handle: Self::_new(lib), is_owned: true, lib }
            }

            fn release(&mut self) {
                if self.is_owned {
                    self._release();
                    self.handle = std::ptr::null();
                    self.is_owned = false;
                }
            }

            fn is_owned(&self) -> bool { self.is_owned }
            fn handle(&self) -> HandleType { self.handle }
        }
    };
    ( $n:ident, new = $new:expr, release = $release:expr ) => {
        impl<'lib> _PackedExt<'lib> for $n<'lib> {
            fn _new(lib: &'lib Lib) -> HandleType {
                packed_call!(lib.registry_get("PackedFunc", $new))
            }

            fn _release(&self) {
                let () = packed_call!(self.lib.registry_get("PackedFunc", $release), self);
            }
        }
        impl<'lib> Drop for ExtTest<'lib> {
            fn drop(&mut self) {
                self.release()
            }
        }
    };
    ( $n:ident, $c:expr, new = $new:expr, release = $release:expr ) => {
        impl_packed_ext!($n, $c);
        impl_packed_ext!($n, new = $new, release = $release );
    }
}

mod tests;
