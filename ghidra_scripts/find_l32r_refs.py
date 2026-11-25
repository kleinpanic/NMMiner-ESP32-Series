# Find l32r references to an address
# @author 
# @category RE

from ghidra.program.model.address import Address

literal_addr = currentProgram.getAddressFactory().getAddress("400d1c14")
refManager = currentProgram.getReferenceManager()

refs = refManager.getReferencesTo(literal_addr)
found = False

for ref in refs:
    from_addr = ref.getFromAddress()
    print("Found reference from: {}".format(from_addr))
    
    # Try to find the function containing this reference
    func = currentProgram.getFunctionManager().getFunctionContaining(from_addr)
    if func:
        print("  Inside function: {} at {}".format(func.getName(), func.getEntryPoint()))
    else:
        print("  Not inside a known function. Trying to disassemble...")
        # Since we ran with -noanalysis or analysis missed it, it might not be disassembled
        # We can just print the address for manual inspection
    found = True

if not found:
    print("No references found to {}".format(literal_addr))
