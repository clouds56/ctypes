try:
    from .test_ctypes import lib, ExtBaseCls, PackedArg, classproperty
except:
    from test_ctypes import lib, ExtBaseCls, PackedArg, classproperty


class ExtTest(ExtBaseCls):
    ext_get = lib.Get("ext_get")
    ext_new = lib.Get("ext_new")
    ext_release = lib.Get("ext_release")

    @classproperty
    def ext_name(cls):
        return "ExtTest"

    @classproperty
    def type_code(cls):
        return 32

    def __init__(self, lib, handle):
        super(ExtTest, self).__init__(lib, handle)

    @property
    def name(self):
        return self.ext_get(self)

    @classmethod
    def _new(cls):
        return cls.ext_new()

    def _release(self):
        self.ext_release(self)
        self.is_py = False

    def __repr__(self):
        return "<%s 0x%x %s>" % (self.ext_name, self.handle, self.name)


PackedArg.RegisterExt(ExtTest)
