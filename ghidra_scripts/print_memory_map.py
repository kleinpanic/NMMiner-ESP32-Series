# Print memory map
# @author 
# @category RE
# @keybinding 
# @menupath 
# @toolbar 

for block in currentProgram.getMemory().getBlocks():
    print("Block: {} [0x{} - 0x{}] Size: {}".format(block.getName(), block.getStart(), block.getEnd(), block.getSize()))
