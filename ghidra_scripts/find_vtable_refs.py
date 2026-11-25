# Find references to vtable
# @author 
# @category RE

from ghidra.program.model.address import Address

vtable_addr = currentProgram.getAddressFactory().getAddress("3f4be990")
refManager = currentProgram.getReferenceManager()

refs = refManager.getReferencesTo(vtable_addr)
found = False

for ref in refs:
    from_addr = ref.getFromAddress()
    print("Found reference from: {}".format(from_addr))
    
    func = currentProgram.getFunctionManager().getFunctionContaining(from_addr)
    if func:
        print("  Inside function: {} at {}".format(func.getName(), func.getEntryPoint()))
    found = True

if not found:
    print("No direct references to {}".format(vtable_addr))
    # We might need to look for literals containing 3f4be990
    print("Searching for literals...")
    mem = currentProgram.getMemory()
    start = currentProgram.getAddressFactory().getAddress("400d0020")
    end = currentProgram.getAddressFactory().getAddress("40200000")
    
    # Simple byte search for 90 e9 4b 3f
    bytes = [0x90, 0xe9, 0x4b, 0x3f]
    addr = start
    while addr.compareTo(end) < 0:
        val = mem.getInt(addr)
        val_hex = val & 0xFFFFFFFF
        if 0x3f4be980 <= val_hex <= 0x3f4be9a0:
            print("Found literal 0x{:08x} at: {}".format(val_hex, addr))
            # Now find references to THIS literal
            lit_refs = refManager.getReferencesTo(addr)
            for lref in lit_refs:
                l_from = lref.getFromAddress()
                print("  Referenced from: {}".format(l_from))
                func = currentProgram.getFunctionManager().getFunctionContaining(l_from)
                if func:
                    print("    Inside function: {} at {}".format(func.getName(), func.getEntryPoint()))
            found = True
        try:
            addr = addr.add(4)
        except:
            break
