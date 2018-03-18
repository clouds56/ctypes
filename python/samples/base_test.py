import sys
import os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
try:
    from .ext import ExtTest
    from .test_ctypes import lib
except:
    from ext import ExtTest
    from test_ctypes import lib


def test_ext():
    ext = ExtTest.new()
    print(lib.Get("ext_transform")(ext))
    # could not inline new like this
    #print(lib.Get("ext_transform")(ExtTest.new()))
    # unlike c++, the scope of temp var is not the statement
    del ext

def test_base():
    fs = {k: lib.Get(k) for k in lib.RegistryListNames()}
    print(fs.keys())
    print("hello: 1+2=", fs['hello'](1, 2))
    print("append_str:", fs['append_str']("hello", "world"))
    print("test_append_str:", fs['test_append_str'](fs['append_str'], "append", "str"))
    print("vector_add:", fs['vector_add']([[1,2,3],[4]], [1,2,3,4]))


if __name__ == "__main__":
    test_ext()
    test_base()
