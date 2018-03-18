#!python3

from __future__ import absolute_import, division, print_function, unicode_literals
import ctypes
import os
import types

"""
CTI_EXPORT int CTIRegistryListNames(const char* tag, OUT int* ret_size, OUT const char*** ret_names);
CTI_EXPORT int CTIRegistryGet(const char* tag, const char* name, OUT func_handle* ret_handle);
CTI_EXPORT int CTIPackedFuncCall(const void* handle, int num_args, const unsigned* type_codes, const packedvalue_handle* values,
                                 OUT unsigned* ret_type, OUT packedvalue_handle* ret_val);
"""


packedtypecode = ctypes.c_uint
packedfunc_handle = ctypes.c_void_p


class packedvec_handle(ctypes.Structure):
    """
    struct PackedVector {
      PackedValue* data;
      size_t size;
      PackedType type_code;
    };
    """
    pass


class packedvalue_handle(ctypes.Union):
    """
    union PackedValue {
      int64_t v_int64;
      double v_float64;
      void* v_voidp;
      const char* v_str;
      const PackedFunc* v_func;
      const PackedVector* v_vec;
    };
    """
    _fields_ = [("v_int64", ctypes.c_int64),
                ("v_float64", ctypes.c_double),
                ("v_void_p", ctypes.c_void_p),
                ("v_str", ctypes.c_char_p),
                ("v_func", packedfunc_handle),
                ("v_vec", ctypes.POINTER(packedvec_handle))]


packedvec_handle._fields_ = [("data", ctypes.POINTER(packedvalue_handle)),
                             ("count", ctypes.c_size_t),
                             ("type_code", packedtypecode)]


class PackedArg:
    """
    enum PackedType_ {
      kUnknown = 0,
      kInt64 = 1,
      kFloat64 = 2,
      kPtr = 3,
      kStr = 4,
      kFunc = 5,
      kVector = 6,
    };
    """
    type_codes = {
        "kUnknown": 0,
        "kInt64": 1,
        "kFloat64": 2,
        "kPtr": 3,
        "kStr": 4,
        "kFunc": 5,
        "kVector": 6,
    }

    def __init__(self, value, type_code=None):
        if type_code is None:
            tp, value = PackedArg.from_(value)
            type_code = PackedArg.from_typecode(tp)
        self.type_code = type_code
        self.value = value

    @staticmethod
    def from_(value):
        packed_value = packedvalue_handle()
        if isinstance(value, int):
            packed_value.v_int64 = value
            return "kInt64", packed_value
        elif isinstance(value, float):
            packed_value.v_float64 = value
            return "kFloat64", packed_value
        elif isinstance(value, str):
            packed_value.v_str = value.encode()
            return "kStr", packed_value
        elif isinstance(value, PackedFunc):
            packed_value.v_func = value.func_handle
            return "kFunc", packed_value
        elif isinstance(value, list):
            tp, vec = PackedArg.make_vec(value)
            packed_value.v_vec = ctypes.pointer(vec)
            return tp, packed_value

    def to(self):
        type_code = self.type_code
        if type_code == PackedArg.type_codes["kUnknown"]:
            raise ValueError("Unknown type_code")
        elif type_code == PackedArg.type_codes["kInt64"]:
            return self.value.v_int64
        elif type_code == PackedArg.type_codes["kFloat64"]:
            return self.value.v_float64
        elif type_code == PackedArg.type_codes["kStr"]:
            return self.value.v_str.decode()
        elif type_code == PackedArg.type_codes["kFunc"]:
            return PackedFunc(None, self.value.v_func)
        elif type_code == PackedArg.type_codes["kVector"]:
            ret = []
            vec = self.value.v_vec.contents
            for i in range(vec.count):
                ret.append(PackedArg(vec.data[i], type_code=vec.type_code).to())
            return ret

    def __repr__(self):
        return "<PackedArg %d %s>" % (self.type_code, self.to())

    @staticmethod
    def from_typecode(tp):
        if isinstance(tp, tuple):
            return PackedArg.from_typecode(tp[0])
        elif isinstance(tp, str):
            return PackedArg.type_codes[tp]
        return PackedArg.type_codes["kUnknown"]

    @staticmethod
    def make_vec(value):
        packed_vec = packedvec_handle()
        packed_vec.count = len(value)
        packed_vec.type_code = PackedArg.type_codes["kUnknown"]
        type_code, data = None, (packedvalue_handle * len(value))()
        for i, x in enumerate(value):
            tp, v = PackedArg.from_(x)
            if type_code is None:
                type_code = tp
            elif type_code != tp:
                raise ValueError("Type code not consistent in vec")
            data[i] = v
        packed_vec.type_code = PackedArg.from_typecode(type_code)
        packed_vec.data = data
        return ("kVector", type_code), packed_vec



class PackedFunc:
    def __init__(self, lib, func_handle, name=""):
        self.lib = lib
        self.name = name
        self.func_handle = func_handle

    def __call__(self, *args):
        return self.lib.PackedFuncCall(self.func_handle, *args)

    def __repr__(self):
        return "<PackedFunc %s>" % self.name


class Lib:
    funcname_encoding = "ascii"

    def __init__(self, libname, hint=""):
        self.libpath = Lib._find_lib(libname, hint=hint)
        self.lib = Lib._load_lib(self.libpath)

    @staticmethod
    def _find_lib(libname, hint):
        for pre, post in [("lib", ".so"), ("", ".dll"), ("lib", ".dylib")]:
            filename = os.path.join(hint, pre + libname + post)
            if os.path.exists(filename):
                return filename

    @staticmethod
    def _load_lib(libpath):
        """Load libary by searching possible path."""
        from ctypes import c_int, c_char_p, c_void_p, POINTER
        lib = ctypes.CDLL(libpath, ctypes.RTLD_GLOBAL)
        # DMatrix functions
        # lib.TVMGetLastError.restype = ctypes.c_char_p
        lib.CTIRegistryListNames.argtypes = [c_char_p, POINTER(c_int), POINTER(POINTER(c_char_p))]
        lib.CTIRegistryListNames.restype = c_int

        lib.CTIRegistryGet.argtypes = [c_char_p, c_char_p, POINTER(packedfunc_handle)]
        lib.CTIRegistryGet.restype = c_int

        lib.CTIPackedFuncCall.argtypes = [packedfunc_handle, c_int,
                                          POINTER(packedtypecode), POINTER(packedvalue_handle),
                                          POINTER(packedtypecode), POINTER(packedvalue_handle)]
        lib.CTIPackedFuncCall.restype = c_int
        return lib

    def RegistryListNames(self, registry_name="PackedFunc"):
        ret_names = ctypes.POINTER(ctypes.c_char_p)()
        ret_size = ctypes.c_int()
        self.lib.CTIRegistryListNames(registry_name.encode(Lib.funcname_encoding), ctypes.byref(ret_size), ctypes.byref(ret_names))
        return [ret_names[i].decode(Lib.funcname_encoding) for i in range(ret_size.value)]

    def Get(self, name, registry_name="PackedFunc"):
        ret_handle = packedfunc_handle()
        self.lib.CTIRegistryGet(registry_name.encode(Lib.funcname_encoding), name.encode(Lib.funcname_encoding), ctypes.byref(ret_handle))
        return PackedFunc(self, ret_handle.value, name=name)

    def PackedFuncCall(self, func_handle, *args):
        num_args = len(args)
        packed_args = [PackedArg(x) for x in args]
        type_codes = (packedtypecode * num_args)()
        values = (packedvalue_handle * num_args)()
        for i, x in enumerate(packed_args):
            type_codes[i] = x.type_code
            values[i] = x.value
        ret_type = packedtypecode()
        ret_val = packedvalue_handle()
        self.lib.CTIPackedFuncCall(func_handle, len(args), type_codes, values, ctypes.byref(ret_type), ctypes.byref(ret_val))
        ret = PackedArg(ret_val, type_code=ret_type.value)
        return ret.to()
