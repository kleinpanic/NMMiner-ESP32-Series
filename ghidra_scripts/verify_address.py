# Verify address 0x400d1960
# @author 
# @category RE
# @keybinding 
# @menupath 
# @toolbar 

from ghidra.program.model.address import Address

addr = currentProgram.getAddressFactory().getAddress("400d1960")
func = getFunctionContaining(addr)

if func:
    print("Found function: {} at {}".format(func.getName(), func.getEntryPoint()))
else:
    print("No function at {}".format(addr))
    # Try to see what is there
    codeUnit = currentProgram.getListing().getCodeUnitAt(addr)
    if codeUnit:
        print("Code unit: {}".format(codeUnit.toString()))
    else:
        print("Nothing at {}".format(addr))
