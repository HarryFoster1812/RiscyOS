/*----------------------------------------------------------------------------*/
/* mmu_regs.v                                                                */
/* */
/* Provides kernel-writable registers to control the Instruction and Data     */
/* MMUs. Supports Base-and-Bound protection with a Virtual Start offset for   */
/* the data segment, allowing shared code and private data segments.          */
/* */
/* Mapped at PERIPH + P_MMU = 0x0001_09xx                                     */
/* */
/* Register map:                                                              */
/* offset 0x00  IMMU_BASE        [31:0] Physical base for instructions      */
/* offset 0x04  IMMU_LIMIT       [31:0] Upper physical limit for instr      */
/* offset 0x08  DMMU_BASE        [31:0] Physical base for data segment      */
/* offset 0x0C  DMMU_LIMIT       [31:0] Upper physical limit for data       */
/* offset 0x10  DMMU_VIRT_START  [31:0] Virtual address where data begins   */
/* offset 0x14  MMU_CTRL         [0]    Enable bit (1 = translation active) */
/* offset 0x18  MMU_STATUS       [0]    Current enable status (RO)          */
/* [1]    Current mode is user (RO)           */
/* */
/*----------------------------------------------------------------------------*/

module mmu_regs (
    input  wire        clk,
    input  wire        reset,

    /* Bus interface (connected via periph2 data mux) */
    input  wire        cs_i,
    input  wire        read_i,
    input  wire        write_i,
    input  wire [31:0] address_i,
    input  wire  [1:0] mode_i,
    input  wire  [1:0] size_i,
    output wire        stall_o,
    output wire  [2:0] abort_v_o,
    input  wire [31:0] data_in,
    output reg  [31:0] data_out,

    /* Translation outputs — routed back to subsystem for simple_mmu */
    output reg  [31:0] immu_base_o,   
    output reg  [31:0] immu_limit_o, 
    output reg  [31:0] dmmu_base_o, 
    output reg  [31:0] dmmu_limit_o,
    output reg  [31:0] dmmu_virt_start_o, 

    output reg         mmu_enable_o     /* 0 = translation disabled */
);

assign stall_o   =    cs_i   && 1'b0;
assign abort_v_o = {3{cs_i}} && 3'h0;

reg [5:0] addr_latch;

wire priv_write = cs_i && write_i && (mode_i != 2'b00);

always @ (posedge clk)
if (reset)
    begin
    /*
     * Default: base = 0x0004_0000 (start of user RAM area), disabled.
     * The kernel must explicitly enable translation once it has set up the
     * base for the current user process.
     */
    immu_base_o <= 32'h0004_0000;
    immu_limit_o <= 32'h0004_0000;

    // base - physical address
    dmmu_base_o <= 32'h0004_0000;
    // limit - physical address
    dmmu_limit_o <= 32'h0004_0000;
    // virt_start a new address
    dmmu_virt_start_o <= 32'h0004_0000;

    mmu_enable_o  <= 1'b0;
    end
else
    if (priv_write)
        case (address_i[4:2])
            3'h0: immu_base_o <= data_in; // 0x0
            3'h1: immu_limit_o <= data_in; // 0x4
            3'h2: dmmu_base_o <= data_in; // 0x8
            3'h3: dmmu_limit_o <= data_in; // 0xC
            3'h4: dmmu_virt_start_o <= data_in; // 0x10
            3'h5: mmu_enable_o <= data_in[0]; // 0x14

        endcase

/* Address latch for read mux timing */
always @ (posedge clk)
    if (read_i) addr_latch <= address_i[5:0];

always @ (*)
    case (addr_latch[4:2])
        3'h0:    data_out = immu_base_o;
        3'h1:    data_out = immu_limit_o;
        3'h2:    data_out = dmmu_base_o;
        3'h3:    data_out = dmmu_limit_o;
        3'h4:    data_out = dmmu_virt_start_o; // 0x10
        3'h5:    data_out = {31'h0000_0000, mmu_enable_o};
        3'h6:    data_out = {30'h0, (mode_i == 2'b00), mmu_enable_o};
        default: data_out = 32'h0000_0000;
    endcase

endmodule   // mmu_regs
