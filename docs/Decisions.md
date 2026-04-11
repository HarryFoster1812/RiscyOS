On mhandler:
Store all of the regs onto the kernel stack and then copy sp into a0
a0 now becomes a "trap frame" storing the context of the program and we can do whatever without worring about overwriting regs

so now instead of the trap/ecall handler reciving nothing it gets passed a pointer to a trap frame struct. If a function wants to  return something it has to overwrite the trap frame a0 / a1 value

During a context switch, it recives the trap frame and a pid target (which is a index into the process array)
it will overwrite the trapfram in the current process pcb and also save other things such as the MEPC and MSTATUS ect and then overwrite the trap frame passed into it with the target context

This is not the most ideal thing to do. In the linux kernel, each running process has it own kernel stack and the context switch just switches the kernel stack pointer which is easier to do but having seperate kernel stacks is not feasble in memory constraints.

I decided to make a internal kmalloc. This will just be a simplistic header based heap. The main reason I did this is because I want to have dynamicly created items like PCB's and file handlers and I dont like having a fixed pool or any limitations. I know in the long run having a fixed pool is better and will save time and space but I like the dynamic approach.

Another thing i have decided is to create a slab-like allocator for the things like pcb and file handles so i can get the speed of O(1) (amortized) allocation.

For user memory I am going to be using a flat bit field allocator, I origonally planned for a buddy allocator but that would take up twice as much memory and for 512 bytes (4096 bits) the time to space saving is really not worth double the size

My plan is for on execve -> find program -> give it to the linker -> try to allocate memory -> if fails then check reason -> is not enough try to swap out a program?
On successful allocate -> parse program and modify all non-relocateable code (should be statically linked to a small libc)

Each program will be text + bss/static + stack (4KiB) which is also heap space where heap is managed by sbrk which is just going to be (check sp if current brk + inc >= current sp then deniy else allow) and is just also going to be a header based heap or maybe a more complex idk yet

Ok so how will swap space work? If we have a program that has non-relocateable code and we change it when linking/loading then when we swap it out it will then be in the wrong place

I added a very basic mmu (and i need to make it better) but for now the mmu is just a base offset so vaddress + offset = physical address (which is only active during user mode)
I plan to expand this so that there are 4 mmu registers (instruction base + limit, data base + limit) this should allow me to get individual process protection and it will let me use a very bad version of shared text sections so when a process forks it will be able to share the parent text section but have a different data section

Yesterday I accedentally swapped VCC and ground of the SD SPI breakout board and it killed my sd card but the board seemed fine. I went out and bought another sd card (18 quid) and then spent the rest of the day bent over a oscilliscope trying to debug the signals. Now i am home, i just tested it and maybe the problem is the board as well, i think i might of killed both the board and the sd although the new one is not killed when i plug it in. Anyway i have just bought more boards and hopefully they work

I think I need to make a mini polarity proctection circuit but I have no components 
