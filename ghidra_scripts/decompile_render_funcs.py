# Decompile render functions
# @author 
# @category RE

from ghidra.app.decompiler import DecompInterface
from ghidra.util.task import ConsoleTaskMonitor

vtables = {
    "Miner320x240": "3f4be98c",
    "Clock320x240": "3f4738b8"
}

mem = currentProgram.getMemory()
funcManager = currentProgram.getFunctionManager()
decompInterface = DecompInterface()
decompInterface.openProgram(currentProgram)

for name, vtable_str in vtables.items():
    vtable_addr = currentProgram.getAddressFactory().getAddress(vtable_str)
    
    # Read vtable[2]
    render_ptr_addr = vtable_addr.add(8)
    render_ptr = mem.getInt(render_ptr_addr) & 0xFFFFFFFF
    render_addr = currentProgram.getAddressFactory().getAddress(hex(render_ptr).rstrip('L'))
    
    print("Screen: {} -> render() is at 0x{:08x}".format(name, render_ptr))
    
    func = funcManager.getFunctionAt(render_addr)
    if func:
        results = decompInterface.decompileFunction(func, 60, ConsoleTaskMonitor())
        if results.decompileCompleted():
            print("Decompiled {}:".format(func.getName()))
            print("----------")
            print(results.getDecompiledFunction().getC())
            print("----------")
        else:
            print("Decompilation failed for 0x{:08x}".format(render_ptr))
    else:
        print("Function not found at 0x{:08x}".format(render_ptr))
