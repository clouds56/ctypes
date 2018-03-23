#[macro_use]
extern crate cti;
use cti::*;

pub trait ExtTestType<'lib> : PackedExt<'lib> {
    const _CODE: u32 = 32;
    // TODO: block on default type
    const _NEW: &'static str = "ext_new";
    const _RELEASE: &'static str = "ext_release";
}

impl_packed_ext!(pub struct ExtTest, ExtTestType);
impl_packed_ext!(pub managed struct ExtManagedTest, ExtTestType);

#[derive(Debug)]
pub struct ExtTestProxy<'lib, T> where T: ExtTestBase<'lib> {
    content: T,
    count: u32,
    _phantom: std::marker::PhantomData<&'lib bool>,
}

pub trait ExtTestBase<'lib> : PackedExt<'lib> {
    fn name(&self) -> String;
    fn transform(&mut self) -> ExtTest<'lib>;
}

impl<'lib, T: ExtTestType<'lib>> ExtTestBase<'lib> for T {
    fn name(&self) -> String {
        let f: PackedFunc<'lib> = self.lib().registry_get("PackedFunc", "ext_get");
        packed_call!(f, self)
    }

    fn transform(&mut self) -> ExtTest<'lib> {
        let f: PackedFunc<'lib> = self.lib().registry_get("PackedFunc", "ext_transform");
        packed_call!(f, self as &Self)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::path::PathBuf;

    const LIB_NAME: &'static str = "../build/libtest_ctypes.dylib";
    fn _find_lib() -> PathBuf {
        PathBuf::from(LIB_NAME)
    }

    pub fn test_ext() {
        let lib: Lib = Lib::open(_find_lib().as_ref()).unwrap();
        let _ext_test = ExtManagedTest::new(&lib);
        let mut ext_test = ExtTestProxy{ content: _ext_test, count: 2, _phantom: std::marker::PhantomData };
        println!("new_ext: {:?}", ext_test);
        println!("transform: {:?}",
                 (0..ext_test.count).fold(
                     ext_test.content.transform(),
                     |mut t,_| t.transform()));
        let name = ext_test.content.name();
        println!("name: {}", name);
        assert_eq!(name, "run!!!");
    }

    #[test]
    fn it_works() { test_ext() }
}

fn main() {
}
