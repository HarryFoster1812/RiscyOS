On mhandler:
Store all of the regs onto the kernel stack and then copy sp into a0
a0 now becomes a "trap frame" storing the context of the program and we can do whatever without worring about overwriting regs

so now instead of the trap/ecall handler reciving nothing it gets passed a pointer to a trap frame struct. If a function wants to  return something it has to overwrite the trap frame a0 / a1 value

During a context switch, it recives the trap frame and a pid target (which is a index into the process array)
it will overwrite the trapfram in the current process pcb and also save other things such as the MEPC and MSTATUS ect and then overwrite the trap frame passed into it with the target context

This is not the most ideal thing to do. In the linux kernel, each running process has it own kernel stack and the context switch just switches the kernel stack pointer which is easier to do but having seperate kernel stacks is not feasble in memory constraints.
