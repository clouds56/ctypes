#[macro_use]
extern crate cti;
use cti::*;

#[derive(Debug)]
pub struct ExtTest<'lib> {
    handle: HandleType,
    is_owned: bool,
    lib: &'lib Lib,
}

impl_packed_ext!(ExtTest, 32, new="ext_new", release="ext_release");

impl<'lib> ExtTest<'lib> {
    pub fn name(&self) -> String {
        packed_call!(self.lib.registry_get("PackedFunc", "ext_get"), self)
    }

    pub fn transform(&mut self) -> Self {
        let f = self.lib.registry_get("PackedFunc", "ext_transform");
        let handle: PackedArg = packed_call!(f, self as &Self);
        Self::from_raw(self.lib, handle)
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
        let mut ext_test = ExtTest::new(&lib);
        println!("new_ext: {:?}", ext_test);
        println!("transform: {:?}", ext_test.transform());
        let name = ext_test.name();
        println!("name: {}", name);
        assert_eq!(name, "run!");
    }

    #[test]
    fn it_works() { test_ext() }
}

fn main() {
}
