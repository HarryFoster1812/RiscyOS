/*----------------------------------------------------------------------------*/
/* simple_mmu.v — Address translation MMU                                    */
/*                                                                            */
/* Replaces the existing `mmu` module stub.  When the processor is in        */
/* user mode (mode_i == 2'b00) and MMU translation is enabled, every virtual */
/* address is translated to a physical address by adding base_offset_i:      */
/*                                                                            */
/*   physical_address = virtual_address + base_offset                        */
/*                                                                            */
/* In M-mode and S-mode the address passes through unchanged.                */
/*                                                                            */
/* Alignment faults use the VIRTUAL address (bit positions are identical).   */
/* Access faults check the PHYSICAL address against machine-reserved space.  */
/*                                                                            */
/*----------------------------------------------------------------------------*/



module simple_mmu 
    (
    /* Virtual address from processor */
    input  wire [31:0] address_i,
    input  wire  [1:0] mode_i,
    input  wire  [1:0] size_i,
    input  wire        read_i,
    input  wire        write_i,

    /* Translation inputs from mmu_regs */
    input  wire [31:0] base_offset_i,
    input  wire [31:0] limit_offset_i,
    input  wire [31:0] virtual_start_i,
    input  wire        mmu_enable_i,

    /* Outputs */
    output wire [31:0] phys_address_o, /* Physical address (after translation) */
    output reg   [2:0] abort_o,
    output reg         read_o,
    output reg         write_o
    );

// Translation Logic 
wire user_mode  = (mode_i == 2'b00);
wire translate  = user_mode && mmu_enable_i;

assign phys_address_o = translate ? (address_i - virtual_start_i + base_offset_i) : address_i;

// Fault detection
reg abort_align;
reg abort_access;

always @ (*)
begin
    /* Alignment check */
    if (read_i || write_i)
        case (size_i)
            2'h1:    abort_align = (address_i[0]   != 1'h0); /* halfword */
            2'h2:    abort_align = (address_i[1:0] != 2'h0); /* word     */
            2'h3:    abort_align = (address_i[2:0] != 3'h0); /* dword    */
            default: abort_align = 1'b0;
        endcase
    else
        abort_align = 1'b0;

    /* Access fault check (uses physical address) */
    /*
     * With translation enabled a user program should only reach addresses
     * at or above base_offset (the user RAM region).  If the physical
     * address ends up in machine-reserved space (0x0000_xxxx) after
     * addition, report an access fault.
     *
     * Without translation, the existing rule applies: user mode may not
     * directly address the machine RAM / peripheral region below 0x0004_0000.
     */
    if (read_i || write_i)
        begin
        if (translate)
            /*
             * Physical range check: reject if the translated address lands
             * out side of range
             */
            abort_access = (address_i < virtual_start_i || ((address_i - virtual_start_i + base_offset_i)>=limit_offset_i));
        else
            /*
             * Pass-through mode: user mode cannot address below 0x0004_0000.
             */
            abort_access = user_mode && (address_i[31:18] == 14'h0000);
        end
    else
        abort_access = 1'b0;

    // Encode abort code
    if (abort_align)
        abort_o = read_i ? `ABORT_LD_ALGN : `ABORT_ST_ALGN;
    else if (abort_access)
        abort_o = read_i ? `ABORT_LD_ACC  : `ABORT_ST_ACC;
    else
        abort_o = `ABORT_NONE;

    // Gate read/write enables: suppress when aborting
    read_o  = read_i  && (abort_o == `ABORT_NONE);
    write_o = write_i && (abort_o == `ABORT_NONE);
end

endmodule
