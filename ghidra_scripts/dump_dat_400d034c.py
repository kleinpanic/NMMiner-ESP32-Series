# Dump DAT_400d034c
# @author 
# @category RE

from ghidra.program.model.address import Address

addr = currentProgram.getAddressFactory().getAddress("400d034c")
mem = currentProgram.getMemory()

try:
    val = mem.getInt(addr) & 0xFFFFFFFF
    print("DAT_400d034c = 0x{:08x}".format(val))
    
    # Try to read the structure at that address
    target_addr = currentProgram.getAddressFactory().getAddress(hex(val).rstrip('L'))
    print("Dumping 16 words at 0x{:08x}:".format(val))
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
