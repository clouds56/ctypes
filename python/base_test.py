try:
    from .base import *
except:
    from base import *

def test_base():
    lib = Lib("test_ctypes", hint="../build")
    fs = {k: lib.Get(k) for k in lib.RegistryListNames()}
    print(fs.keys())
    print("hello: 1+2=", fs['hello'](1, 2))
    print("append_str:", fs['append_str']("hello", "world"))
    print("test_append_str:", fs['test_append_str'](fs['append_str'], "append", "str"))
    print("vector_add:", fs['vector_add']([[1,2,3],[4]], [1,2,3,4]))

if __name__ == "__main__":
    test_base()
