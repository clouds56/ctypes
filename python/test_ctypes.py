try:
    from .base import Lib
except:
    from base import Lib

lib = Lib("test_ctypes", hint="../build")
