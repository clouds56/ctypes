#[macro_use]
extern crate cti;
use cti::*;

#[derive(Debug)]
pub struct ExtTest {
    handle: HandleType,
    is_owned: bool,
}

impl_packed_ext!(ExtTest, 32, new="ext_new", release="ext_release");

impl ExtTest {
    pub fn name(&self) -> String {
        registry_func!(EXT_GET, "ext_get");
        packed_call!(EXT_GET, self)
    }

    pub fn transform(&mut self) -> Self {
        registry_func!(EXT_TRANSFORM, "ext_transform");
        Self::from_raw(packed_call!(EXT_TRANSFORM, self as &Self))
    }
}

mod tests {
    use super::*;

    pub fn test_ext() {
        let mut ext_test = ExtTest::new();
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
    tests::test_ext()
}
