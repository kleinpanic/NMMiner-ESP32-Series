# Dump LAB_400d1af0
# @author 
# @category RE

from ghidra.program.model.address import Address

addr = currentProgram.getAddressFactory().getAddress("400d1af0")
mem = currentProgram.getMemory()

try:
    print("Dumping 16 words at 0x400d1af0:")
    target_addr = addr
    for i in range(16):
        try:
            w = mem.getInt(target_addr) & 0xFFFFFFFF
            print("  +0x{:02x}: 0x{:08x}".format(i*4, w))
            target_addr = target_addr.add(4)
        except:
            print("  <error reading>")
            break
except Exception as e:
    print("Error: {}".format(e))
