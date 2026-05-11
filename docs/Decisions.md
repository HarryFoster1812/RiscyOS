To whomever might read this enjoy (or don't) the ramblings of a man slowly going insane.


On mhandler:
Store all of the regs onto the kernel stack and then copy sp into a0
a0 now becomes a "trap frame" storing the context of the program and we can do whatever without worrying about overwriting regs

so now instead of the trap/ecall handler receiving nothing it gets passed a pointer to a trap frame struct. If a function wants to  return something it has to overwrite the trap frame a0 / a1 value

During a context switch, it receives the trap frame and a pid target (which is a index into the process array)
it will overwrite the trapframe in the current process pcb and also save other things such as the MEPC and MSTATUS ect and then overwrite the trap frame passed into it with the target context

This is not the most ideal thing to do. In the linux kernel, each running process has it own kernel stack and the context switch just switches the kernel stack pointer which is easier to do but having separate kernel stacks is not feasible in memory constraints.

I decided to make a internal kmalloc. This will just be a simplistic header based heap. The main reason I did this is because I want to have dynamically created items like PCB's and file handlers and I don't like having a fixed pool or any limitations. I know in the long run having a fixed pool is better and will save time and space but I like the dynamic approach.

Another thing i have decided is to create a slab-like allocator for the things like pcb and file handles so i can get the speed of O(1) (amortized) allocation.

For user memory I am going to be using a flat bit field allocator, I originally planned for a buddy allocator but that would take up twice as much memory and for 512 bytes (4096 bits) the time to space saving is really not worth double the size

My plan is for on execve -> find program -> give it to the linker -> try to allocate memory -> if fails then check reason -> is not enough try to swap out a program?
On successful allocate -> parse program and modify all non-relocatable code (should be statically linked to a small libc)

Each program will be text + bss/static + stack (4KiB) which is also heap space where heap is managed by sbrk which is just going to be (check sp if current brk + inc >= current sp then deniy else allow) and is just also going to be a header based heap or maybe a more complex idk yet

Ok so how will swap space work? If we have a program that has non-relocatable code and we change it when linking/loading then when we swap it out it will then be in the wrong place

I added a very basic mmu (and i need to make it better) but for now the mmu is just a base offset so vaddress + offset = physical address (which is only active during user mode)
I plan to expand this so that there are 4 mmu registers (instruction base + limit, data base + limit) this should allow me to get individual process protection and it will let me use a very bad version of shared text sections so when a process forks it will be able to share the parent text section but have a different data section

This allows me to get rid of the relocation problem since all programs will just be linked at 0x0 and virtual memory should be fine

Yesterday I accidentally swapped VCC and ground of the SD SPI breakout board and it killed my sd card but the board seemed fine. I went out and bought another sd card (18 quid) and then spent the rest of the day bent over a oscilloscope trying to debug the signals. Just got home, tested it and maybe the problem is the board as well, i think i might of killed both the board and the sd although the new one is not dead when i plug it in. Anyway i have just bought more boards and hopefully they work. One thing I do want to mention is that Jim spent a large part of an afternoon helping me debug the SD card interface using the oscilloscope. He really did not have to spend that amount of time helping, and I genuinely appreciated it. It was one of the nicest and most encouraging parts of the project.

Every interaction I have had with Jim has been memorable. He is always enthusiastic, always speaks his mind. Even asking a simple question can somehow become a half-hour tangent only vaguely related to the original topic, but I always come away having learned something new.

I think I need to make a mini polarity protection circuit but I have no components 

The new boards just came i bought 3 of them so at least one of them work, I will go to uni on monday and solder them up. I might as well admit something stupid but this entire time I have been using 5V logic from the arduino (because i thought that the ardunio outputted 3V logic if the 3V out was used...) and that is the reason why the boards were working like once or twice but not again. Just built a voltage divider for each pin and the boards all work as expected every time. But not the original SD card that is dead.

Ok I just modified the MMU and i added 4 more registers:
- IMMU Base
- IMMU Limit
- DMMU Base
- DMMU Limit
- DMMU Virtual Start

And I modified the logic to do target_address - virtual_start + base = physical
It should fault on: target_address < virtual start or (target_address-virtual + base >= physical)
NOTE: the >= so i need to make sure the last word is not accessible

The main reason for the DMMU virt start is because my original plan was just base + offset dmmu which is bad because say theoretically, the data section was from 0x40_100 to 0x40_200 then for my data MMU base offset to work i would have to set the base to be 0x40_000 for all the addresses to align up but this exposes a vaulnerability (or however that is actually spelt) because if i try to write to 0x90 it will succeed but it should not be able to since it is out side of the data section and could potentially modify the text section. 

I just though of something, on a swap if the  original program is swapped then the forked one will read incorrect memory addresses so i need to have some kind of dependency where if another pcb which relies on the text section then i need to load that text section somewhere and then modify both

Oh and if the original exits then i need to make sure the text section is saved somewhere otherwise the fork does not work.

Ok so new plan, even more complicated, I am going to make a kind of poor mans paging system, instead of just pointing to a pentry, which is a static address, instead i create a memory segment manager so i can count how many references and if they go to 0 then i call ufree else it will persist in memory, i will store two segments per PCB, one text and one data, on fork shallow copy the text (give it a pointer to the existing) and allocate a new  data segment then memcpy parent to child.

For process swapping, now that I have a mmu i can make sure that programs cant write outside of the program bounds. This means that I know 100% that no user program can do anything to the 1MiB frame store so if near the end i am very desperate i could rewrite kmalloc to use the frame buffer. I do want to avoid having the flat allocator inside of the kernel space for the frame buffer so i am going to re-use the flat alloctor logic and modify the functions to just take a pointer to the bit field and some metadata about the length, minimum block len ect

I have made a lot of decision but i forgot/am too lazy to record them. Of the ones i can remember is that instead of an idle "process" which has its own pcb I am just going to switch everything to use a kidle flag. Another thing I decided was that instead of taking up space on the stack the trapframe is just directly saved to the pcb. I don't remember why I did it this way to start with. 

At this point I have accepted that I really do not have enough time for this, i really should of started earlier and worked more on this. This has been 100% my favourite module, I have learnt a lot about hardware. My new goal is to dial it back a bit and get file loading and maybe execution of one or two programs which are at a predefined location. I do really want to continue this project just for the sake of learning and understanding how the system comes together so I hope I can work on it over summer or maybe next year in industry.

I have been procrastinating from doing real work so for now I have made a few scripts, one which was reversed engineered from the bennett source code. And the other which uses the protocol to emulate the bennett terminal which is just implemented by polling the dummy uart and sending is of course by sending a command and then the bytes. This is better then bennett since it will not poll / read the memory state and just poll for (dummy) serial so overall printing should be a lot faster. The script can also give me a semi-automatic profiler which should give stats about the most critical paths which need to optimised (Future me reading over this, I did nothing with this)

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

The FAT driver seems the most annoying and i don't know how it should work when it is the kernel who wants something doing (maybe the pid should be 0?)
My thoughts for the fat driver is to have a FAT_IO type where then each type would have its own fsm eg if it is a find file:
- Get current dir (root if non provided via the FILE struct or if it is a kernel operation) 
- Read the FAT DIR
- Read entries
- Parse name and try to match to string part eg test if name read matched FULLY x in example "/x/y"
- Read off cluster number
- Save it in a open file


I think i will have a special one for read ELF

It is currently Monday 11th of May. Over the weekend I implemented:
-  File open (Involves directory parsing and walking)
-  Serial Write
-  Serial read
- File reading (although not as an ecall because I havent done file descriptors )
- ELF parsing / loading (only expecing my specific linker script)
- Memory Segment manager (for the mmu)
- File seeking 
I am on the way to uni and I know 100% that it will not work. I tried to test it using qemu and gdb but I cant seem to get a function to run so, all of the code I wrote is untested. I have made the decision that until after exams today is the last day I work on this. I think the theoretical paths that I have implemented (if they work) is more than enough. I really should of started this earlier and gotten a mvp first instead of rushing to implement everything in one desperate attempt. I would like to say that I am going to learn from this but I knew that I should of done this before I even started. 

I also tried over the weekend to compile picoLibc and use that but I realised I was just procrastinating and wasting time. So i just made a very tiny terrible libc.


So Today was the very last day that I said I will work on this. After a long day of debugging, the final product kind of works, the elf loading does work and I can load any arbitrary elf file on the SD card (as long as it was linked using my script). I also found out that the tail instruction is not correctly encoded using Jim's assembler. Anyway, once a process is loaded it does context switch into it but it seems the actual instructions are not executed correctly  which I can't really do anything about that all I can do is set the PC to the start address and let it run.

Things that I was meant to implement (but didnt because of time):
- process killing (especially on bad traps)
- Swapping (using the 1 MiB framestore)
- A useable shell so: pwd, chdir, which connects to serial IO
- File writing which also includes mkdir

I do feel sorry for whoever marks this because it is not well documented and doesnt really have a lot of comments

Here is the final run down of what this actually does:
- Given a path (the kernel uses "/init" or "/hello") it will interact with the SD card and using FAT32 it will find and open the file
- It will then read and parse this (which is an ELF) and produce two memory segments (text and data) which it will then read the instructions and data sections into the user memory which is allocated by a flat-bit allocator
- After it is done it will "unblock" the process and add it to the ready queue
- The scheduler will then schedule the process and it will be ran
- The process can print and receive data from the dummy uart using write and read syscall. These go to a tty interface which will govern how and which process should get the data as well handle stuff like back space and new lines automatically.
- Processes can fork which will result in a new process which is isolated being created and added to the schedule queue.
- Processes can perform execve (with no argument passing) to transform it into a different process.
- More general implementation details:
    - The kernel uses a header based heap with coalescing
    - The kernel also has support for a (kind-slab) allocator which is used for pcbs
- Obviously there is the SPI peripheral which interacts with the SD card
- There is a better MMU which allows for process isolation

A significant amount of time was spent on tooling of which I use:
- The RISCV cross compiler
- A custom GNU assembly to Jim conversion script (which works well enough but has issues especially with GAS defined variables and structs)
- Some time was spent learning about the inner working of LibC and making a very small one

Since this is the final day I can now get the final scores:
- I have spent 72 Hours on this project alone (this does inlcude having the editor open and talking to other people though)
- I have (apparently) spent 30 hours in the riscv system (this includes reading through the processor code) and the countless hours where I have had the editor open while debugging the SPI peripheral
Of course these numbers are not accurate they are just an indication of how much time I could of dedicated elsewhere (not to mention the amount of videos and blog articles I have read about how SD cards work, the inner workings of linux and the basics of operating systems)

Even though it doesnt do as much as I planned for it to do, I am proud of what I have achieved. I dont know if this the end of the road for RiscyOS and it just goes into the graveyard of all of my unfinished projects, I have alot of fun making it but the worst thing by far was the lack of testability. I tried to setup qemu on my laptop but it would just take too long to implement all of the peripherals and make sure they work the exact same as the board. What would be quite nice to do is buy a FPGA dev board and download the riscv bitfile on there and then hopefully everything should work. 
