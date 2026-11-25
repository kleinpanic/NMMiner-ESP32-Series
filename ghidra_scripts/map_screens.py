# Map all 6 screens
# @author 
# @category RE

from ghidra.program.model.address import Address

literals = [
    "400d1cfc", # Screen 0
    "400d1d00", # Screen 1
    "400d1d04", # Screen 2 (Miner320x240)
    "400d1d08", # Screen 3
    "400d1d10", # Screen 4
    "400d1d14"  # Screen 5
]

mem = currentProgram.getMemory()
listing = currentProgram.getListing()
funcManager = currentProgram.getFunctionManager()

for lit_str in literals:
    lit_addr = currentProgram.getAddressFactory().getAddress(lit_str)
    try:
        # Read the literal value to get the vtable address
        vtable_ptr = mem.getInt(lit_addr) & 0xFFFFFFFF
        vtable_addr = currentProgram.getAddressFactory().getAddress(hex(vtable_ptr).rstrip('L'))
        
        # getName() is at vtable[4]
        getname_ptr_addr = vtable_addr.add(0x10)
        getname_ptr = mem.getInt(getname_ptr_addr) & 0xFFFFFFFF
        getname_addr = currentProgram.getAddressFactory().getAddress(hex(getname_ptr).rstrip('L'))
        
        # setup/constructor is at vtable[3]
        setup_ptr_addr = vtable_addr.add(0x0c)
        setup_ptr = mem.getInt(setup_ptr_addr) & 0xFFFFFFFF
        
        # destructor is at vtable[0]
        dtor_ptr_addr = vtable_addr
        dtor_ptr = mem.getInt(dtor_ptr_addr) & 0xFFFFFFFF
        
        func = funcManager.getFunctionAt(getname_addr)
        
        # Analyze the function to find the returned string
        # It's usually `l32r a2, <literal>; ret.n`
        # We'll just scan the first few instructions for an l32r
        name = "Unknown"
        ins = listing.getInstructionAt(getname_addr)
        for i in range(5):
            if ins is None:
                break
            if ins.getMnemonicString() == "l32r":
                # Get the literal address from the operand
                ref = ins.getPrimaryReference(1)
                if ref:
                    str_lit_addr = ref.getToAddress()
                    # Read the string pointer
                    str_ptr = mem.getInt(str_lit_addr) & 0xFFFFFFFF
                    str_addr = currentProgram.getAddressFactory().getAddress(hex(str_ptr).rstrip('L'))
                    
                    # Read the string
                    string_data = ""
                    curr = str_addr
                    while True:
                        b = mem.getByte(curr)
                        if b == 0:
                            break
                        string_data += chr(b)
                        curr = curr.add(1)
                    name = string_data
                break
            ins = ins.getNext()
            
        print("Screen Class: {:15} | VTable: 0x{:08x} | Setup: 0x{:08x} | Dtor: 0x{:08x}".format(name, vtable_ptr, setup_ptr, dtor_ptr))
    except Exception as e:
        print("Error processing literal {}: {}".format(lit_str, e))
