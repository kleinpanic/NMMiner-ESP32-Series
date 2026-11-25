# Dump vtable
# @author 
# @category RE

from ghidra.program.model.address import Address

addr = currentProgram.getAddressFactory().getAddress("3f4be980")
listing = currentProgram.getListing()
mem = currentProgram.getMemory()

print("Dumping vtable around 3f4be980:")
for i in range(16):
    try:
        val = mem.getInt(addr)
        # Convert to unsigned 32-bit hex
        val_hex = val & 0xFFFFFFFF
        print("  {} : 0x{:08x}".format(addr, val_hex))
    except:
        print("  {} : <error>".format(addr))
    addr = addr.add(4)
