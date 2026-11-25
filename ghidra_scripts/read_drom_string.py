# Read string from DROM
# @author 
# @category RE

from ghidra.program.model.address import Address

addrs = ["3f4e4287", "3f52c4d9"]
mem = currentProgram.getMemory()

for addr_str in addrs:
    addr = currentProgram.getAddressFactory().getAddress(addr_str)
    string_data = ""
    curr = addr
    for _ in range(64):  # limit to 64 chars
        try:
            b = mem.getByte(curr)
            if b == 0:
                break
            string_data += chr(b)
            curr = curr.add(1)
        except:
            break
    print("String at {}: '{}'".format(addr_str, string_data))
