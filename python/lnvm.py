"""
  pylnvm - Wrapper for liblightnvm

"""
from __future__ import print_function
import ctypes

class Geo(ctypes.Structure):
    """Geometry of a Non-volatile memory device or subset thereof"""

    _fields_ = [
        ("nchannels", ctypes.c_size_t),
        ("nluns", ctypes.c_size_t),
        ("nplanes", ctypes.c_size_t),
        ("nblocks", ctypes.c_size_t),
        ("npages", ctypes.c_size_t),
        ("nsectors", ctypes.c_size_t),
        ("nbytes", ctypes.c_size_t),
        ("tbytes", ctypes.c_size_t),
        ("vblk_nbytes", ctypes.c_size_t),
        ("vpg_nbytes", ctypes.c_size_t),
    ]

    def __str__(self):
        """Returns a human readable string representation"""

        attrs = []
        for field, _ in self._fields_:
            attrs.append("%s(%d)" % (field, getattr(self, field)))

        fmt = []
        split = 3
        for i in xrange(0, len(attrs), split):
            fmt.append(", ".join(attrs[i:i+split]))

        return "geo {\n  %s\n}" % ",\n  ".join(fmt)

LLN = ctypes.cdll.LoadLibrary("liblightnvm.so")
LLN.nvm_dev_attr_geo.restype = Geo

def main():
    """Main entry-point for execution."""

    dev = LLN.nvm_dev_open("nvme0n1")

    if not dev:
        print("Failed opening dev")
        return

    geo = LLN.nvm_dev_attr_geo(dev)
    print(geo)

if __name__ == "__main__":
    main()
