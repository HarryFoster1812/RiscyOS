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

This allows me to get rid of the relocation problem since all programs will just be linked at 0x0 and virtual memory should be fine

Yesterday I accedentally swapped VCC and ground of the SD SPI breakout board and it killed my sd card but the board seemed fine. I went out and bought another sd card (18 quid) and then spent the rest of the day bent over a oscilliscope trying to debug the signals. Now i am home, i just tested it and maybe the problem is the board as well, i think i might of killed both the board and the sd although the new one is not killed when i plug it in. Anyway i have just bought more boards and hopefully they work

I think I need to make a mini polarity proctection circuit but I have no components 

The new boards just came i bought 3 of them so at least one of them work, I will go to uni on monday and solder them up

Ok I just modified the MMU and i added 4 more registers:
- IMMU Base
- IMMU Limit
- DMMU Base
- DMMU Limit
- DMMU Virtual Start

And I modified the logic to do target_address - virtual_start + base = physical
It should fault on: target_address < virtual start or (target_address-virtual + base >= physical)
NOTE: the >= so i need to make sure the last word is not accessable

The main reason for the DMMU virt start is because my origonal plan was just base + offset dmmu which is bad because say the data section was from 0x40_100 to 0x40_200 then for my base to work i would have to set the base to be 0x40_000 for all the addresses to align up but this exposes a vaulnerabliltiy because if i try to write to 0x90 it will work but it should not be able to since it is out side of the data section and could potentially modify the text section. 

I just though of something, on a swap if the  original program is swapped then the forked one will read incorrect memory addresses so i need to have some kind of dependency where if another pcb which relies on the text section then i need to load that text section somewhere and then modify both

Oh and if the original exits then i need to make sure the text section is saved somewhere otherwise the fork does not work. WHY ARE OS SO FUCKING COMPLICATED I LOVE AND HATE THIS

Ok so new plan, even more complicated, I am going to make a kind of poor mans paging system, instead of just pointing to the pentry instead i create a memory segment manager so i can count how many references and if they go to 0 then ufree else it will persist in memory, i will store two segments per PCB, one text and one data, on fork shallow copy the text (give it a pointer to the existing) and allocate a new  data segment then memcpy parent to child.

For process swapping, now that I have a mmu i can make sure that programs cant write outside of the program bounds. This means that I know 100% that no user program can do aything to the 1MiB frame store so if near the end i am very desperate i could rewrite kmalloc to use the frame buffer. I do want to avoid having the flat allocator inside of the kernel space for the frame buffer so i am going to re-use the flat alloctor logic and modify the functions to just take a pointer to the bit field and some metadata about the length, minium block len ect

At this point I have accpeted that I really do not have enough time for this, i really should of started earlier and worked more on this. This has been 100% my favourite module, I have learnt a lot about hardware. My new goal is to dial it back a bit and get file loading and maybe execution of one or two programs which are at a predefined location. I do really want to continue this project just for the sake of learning and understanding how the system comes together so I hope I can work on it over summer or maybe next year in industry.

I havent really updated this in while so here is where i am at:
- The SD card works. I can read from any address i want
- I need to implement a FAT Driver and it needs to be Non-Blocking
I dont really know the best way to do this, I do have a scheduler but it handles processes and not kernel tasks i have no idea how hard it would be to schedule kernel tasks or how long it would take to modify the things i have made and i dont really have enough time to find out

My plan for tasks is that each IO will have its own queue so: 
- Serial Queue
- SD queue
- Any thing else i cant think of

And if it is empty then there is nothing to do (obviously)
but if there is something then we need to set it off (since it will be a multi-step fsm) and then we can let it interrupt until the task is finished at which point we either start the next one or there are no more so just stop

The FAT driver seems the most annoying and i dont know how it should work when it is the kernel who wants something doting (maybe the pid should be 0?)
My though of the fat driver is to have a FAT_IO type where then each type would have its own fsm eg if it is a find file:
- Get current dir (root if non provided via the FILE struct or if it is a kernel operation) 
- Read the FAT DIR
- Read entries
- Parse name and try to match to string part eg test if name read matched FULLY x in example "/x/y"
- Read off cluster number
- Save it in a open file


I think i will have a special one for read ELF

Well, I am coming from a night out and I am on the tram currently and doing work (admitally whilst tipsy/drunk) which is quite sad but hey that is who I am. All I can really have to say is that in comparison to people that I have talked to/hear about is that my project is not that impressive I dont know if anyone except me is going to read this but what the hell I am just going to write this and forget anyway.

It is currently Monday 11th of May. Over the weekend I implemented:
-  File open (Involves directory parsing and walking)
-  Serial Write
-  Serial read
- File reading (although not as an ecall because I havent done file descriptors )
- ELF parsing / loading (only expecing my specific linker script)
- Memory Segment manager (for the mmu)
- File seeking 
I am on the way to uni and I know 100% that it will not work. I tried to test it using qemu and gdb but I cant seem to get a function to run so, all of the code I wrote is untested. I have made the decision that until after exams today is the last day I work on this. I think the theoretical paths that I have implemented (if they work) is more than enough. I really should of started this earlier and gotten a mvp first instead of rushing to implement everything in one desperate attempt. I would like to say that I am going to learn from this but I knew that I should of done this efore I even started. 

I also tried over the weekend to compile picoLibc and use that but I realised I was just procrastinating and wasting time. So i just made a very tiny terrible libc.
