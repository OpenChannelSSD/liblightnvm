"""
  pyLightNVM - Python I/O library for Open-Channel SSDs

"""
from __future__ import print_function
import ctypes

class AddrGDesc(ctypes.Structure):
    """Wrapper for anonymous member NVM_ADDR.g """

    _fields_ = [
        ("blk", ctypes.c_uint64, 16),
        ("pg", ctypes.c_uint64, 16),
        ("sec", ctypes.c_uint64, 8),
        ("pl", ctypes.c_uint64, 8),
        ("lun", ctypes.c_uint64, 8),
        ("ch", ctypes.c_uint64, 7),
        ("rsvd", ctypes.c_uint64, 1),
    ]

class AddrCDesc(ctypes.Structure):
    """Wrapper for anonymous member NVM_ADDR.c """

    _fields_ = [
        ("line", ctypes.c_uint64, 63),
        ("is_cached", ctypes.c_uint64, 1)
    ]

class AddrUDesc(ctypes.Union):
    """Wrapper for anonymous union NVM_ADDR"""

    _anonymous_ = ("g", "c",)
    _fields_ = [
        ("g", AddrGDesc),
        ("c", AddrCDesc),
        ("ppa", ctypes.c_uint64)
    ]

class Addr(ctypes.Structure):
    """Wrapper for NVM_ADDR"""

    _anonymous_ = ("u", )
    _fields_ = [
        ("u", AddrUDesc)
    ]

class Geometry(ctypes.Structure):
    """Geometry of a Non-Volatile Memory device or subset thereof"""

    _fields_ = [
        ("nchannels", ctypes.c_size_t),
        ("nluns", ctypes.c_size_t),
        ("nplanes", ctypes.c_size_t),
        ("nblocks", ctypes.c_size_t),
        ("npages", ctypes.c_size_t),
        ("nsectors", ctypes.c_size_t),

        ("page_nbytes", ctypes.c_size_t),
        ("sector_nbytes", ctypes.c_size_t),
        ("meta_nbytes", ctypes.c_size_t),

        ("tbytes", ctypes.c_size_t),
        ("vblk_nbytes", ctypes.c_size_t),
        ("vpg_nbytes", ctypes.c_size_t),
    ]

LLN = ctypes.cdll.LoadLibrary("liblightnvm.so")
LLN.nvm_dev_get_geo.restype = ctypes.POINTER(Geometry)

class Device(object):

    def __init__(self, dev_path):
        self._dev = None
        self.dev_path = dev_path

    def open(self):
        if self._dev:
            return True

        self._dev = LLN.nvm_dev_open(self.dev_path)

        return self._dev

    def close(self):
        if not self._dev:
            return

        LLN.nvm_dev_close(self._dev)
        self._dev = None

    def geo(self):
        if not self._dev:
            return None

        return LLN.nvm_dev_get_geo(self._dev)

    def pr(self):
        if not self._dev:
            return

        LLN.nvm_dev_pr(self._dev)

class Version(object):

    def __init__(self):
        self.major = LLN.nvm_ver_major()
        self.minor = LLN.nvm_ver_minor()
        self.patch = LLN.nvm_ver_patch()

    def major(self):
        return self.major

    def minor(self):
        return self.minor

    def patch(self):
        return self.patch

    def pr(self):
        LLN.nvm_ver_pr()

    def __str__(self):
        return "%d.%d.%d" % (self.major, self.minor, self.patch)

def main():
    """Main entry-point for execution."""

    dev = Device("/dev/nvme0n1")
    dev.open()

    geo = dev.geo()

    addrs = []

    for ch in xrange(0, geo.contents.nchannels):
        for lun in xrange(0, geo.contents.nluns):
            addr = Addr()
            addr.u.g.ch = ch
            addr.u.g.lun = lun
            addr.u.g.blk = 21

            ahex = "0x%016x" % addr.u.ppa
            addrs.append(ahex)

    print(addrs)

    dev.close()

if __name__ == "__main__":
    main()
