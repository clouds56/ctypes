extern crate cti;
use cti::*;

macro_rules! registry_func {
    ( $n:ident, $s:expr ) => {
        static mut $n: PackedFunc = PackedFunc{ name: None, handle: std::ptr::null() };
        static ONCE: std::sync::Once = std::sync::ONCE_INIT;
        unsafe {
            ONCE.call_once(|| $n = registry_get("PackedFunc", $s))
        }
    };
}

#[derive(Debug)]
pub struct ExtTest {
    handle: HandleType,
    is_owned: bool,
}

impl ExtBase for ExtTest {
    const CODE: u32 = 32;

    fn from_raw(ptr: PackedArg) -> Self {
        ExtTest{ handle: ptr.into(), is_owned: false }
    }

    fn new() -> Self {
        ExtTest{ handle: packed_call!(registry_get("PackedFunc", "ext_new")), is_owned: true }
    }

    fn release(&mut self) {
        registry_func!(EXT_RELEASE, "ext_release");
        if self.is_owned {
            let () = packed_call!(EXT_RELEASE, self as &Self);
            self.handle = std::ptr::null();
            self.is_owned = false;
        }
    }

    fn is_owned(&self) -> bool { self.is_owned }
    fn handle(&self) -> HandleType { self.handle }
}

impl Drop for ExtTest {
    fn drop(&mut self) {
        self.release()
    }
}

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
        println!("{:?}", ext_test);
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
