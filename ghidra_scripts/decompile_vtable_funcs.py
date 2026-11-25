# Decompile vtable functions
# @author 
# @category RE

from ghidra.app.decompiler import DecompInterface
from ghidra.util.task import ConsoleTaskMonitor

funcs = ["400f37b0"]

decompInterface = DecompInterface()
decompInterface.openProgram(currentProgram)

for f_addr in funcs:
    addr = currentProgram.getAddressFactory().getAddress(f_addr)
    func = currentProgram.getFunctionManager().getFunctionAt(addr)
    if func:
        results = decompInterface.decompileFunction(func, 60, ConsoleTaskMonitor())
        if results.decompileCompleted():
            print("Decompiled {}:".format(func.getName()))
            print("----------")
            print(results.getDecompiledFunction().getC())
            print("----------")
        else:
            print("Decompilation failed for {}".format(f_addr))
    else:
        print("Function not found at {}".format(f_addr))
