# Find references to a function
# @author 
# @category RE

from ghidra.program.model.address import Address

func_addr = currentProgram.getAddressFactory().getAddress("400f2f54")
refManager = currentProgram.getReferenceManager()

refs = refManager.getReferencesTo(func_addr)
found = False

for ref in refs:
    from_addr = ref.getFromAddress()
    print("Found reference from: {}".format(from_addr))
    found = True

if not found:
    print("No references found to {}".format(func_addr))
