# Find function entry
# @author 
# @category RE

from ghidra.program.model.address import Address

literal_addr = currentProgram.getAddressFactory().getAddress("400d1960")
funcManager = currentProgram.getFunctionManager()

# Find the function immediately preceding or containing the literal
func = funcManager.getFunctionContaining(literal_addr)
if func:
    print("Function containing literal: {} at {}".format(func.getName(), func.getEntryPoint()))
else:
    # Find the function right before it
    func_iter = funcManager.getFunctions(literal_addr, False)
    if func_iter.hasNext():
        func = func_iter.next()
        print("Nearest function before literal: {} at {}".format(func.getName(), func.getEntryPoint()))
    else:
        print("No function found before {}".format(literal_addr))
