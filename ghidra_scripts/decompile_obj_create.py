# Decompile FUN_400d53d4
# @author 
# @category RE

from ghidra.app.decompiler import DecompInterface
from ghidra.util.task import ConsoleTaskMonitor

func_addr = currentProgram.getAddressFactory().getAddress("400d53d4")
func = currentProgram.getFunctionManager().getFunctionAt(func_addr)

if func:
    decompInterface = DecompInterface()
    decompInterface.openProgram(currentProgram)
    results = decompInterface.decompileFunction(func, 60, ConsoleTaskMonitor())
    
    if results.decompileCompleted():
        print("Decompiled {}:".format(func.getName()))
        print("----------")
        print(results.getDecompiledFunction().getC())
        print("----------")
    else:
        print("Decompilation failed: {}".format(results.getErrorMessage()))
else:
    print("Function not found at {}".format(func_addr))
