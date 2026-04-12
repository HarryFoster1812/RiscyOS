; ra is function where it invoked panic
kpanic:
la panic_str 
call k_dbg_print
j .

panic_str DEFB "KERNEL PANIC\0"
