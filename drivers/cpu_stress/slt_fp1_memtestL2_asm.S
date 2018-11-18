#;-----------------------------------------------------------------------------
#; The confidential and proprietary information contained in this file may
#; only be used by a person authorised under and to the extent permitted
#; by a subsisting licensing agreement from ARM Limited.
#;
#;            (C) COPYRIGHT 2007-2010 ARM Limited.
#;                ALL RIGHTS RESERVED
#;
#; This entire notice must be reproduced on all copies of this file
#; and copies of this file may only be made by a person if such person is
#; permitted to do so under the terms of a subsisting license agreement
#; from ARM Limited.
#;
#;      RCS Information
#;
#;      RCS Filename        : $RCSfile$
#;
#;      Checked In          : $Date$
#;
#;      Revision            : $Revision: 73865 $
#;
#;      Release Information : r3p0-00rel0
#;-----------------------------------------------------------------------------
#;
#;;-----------------------------------------------------------------------------
#;;  Find configuration of CPU
#;;  Variable PERIPH_BASE needs to be updated according to real mapping
#;;-----------------------------------------------------------------------------



    .text

//write_iteration: times of write operation 
    .EQU    write_iteration,    0x2
//read_iteration: times of  read operation  after write operation
    .EQU    read_iteration,     0x4
//num_init: data-in-line test vale0
    .equ    num_init0,    0x00000000FFFFFFFF
//num_init: data-in-line test vale1
    .equ    num_init1,    0x55555555AAAAAAAA
//num_init: data-in-line test vale2
    .equ    num_init2,    0x33333333CCCCCCCC
//num_init: data-in-line test vale3
    .equ    num_init3,    0x6699669999669966
//num_init: data-in-line test vale4
    .equ    num_init4,    0x0F0F0F0FF0F0F0F0
//num_init: data-in-line test vale5
    .equ    num_init5,    0x5A5A5A5AA5A5A5A5
//num_init: data-in-line test vale6
    .equ    num_init6,    0x3C3C3C3CC3C3C3C3
//num_init: data-in-line test vale7
    .equ    num_init7,    0x6969696996969696

//num_init: data-in-line test vale0_F
    .equ    num_init0_F,    0xFFFFFFFF00000000
//num_init: data-in-line test vale1_F
    .equ    num_init1_F,    0xAAAAAAAA55555555
//num_init: data-in-line test vale2_F
    .equ    num_init2_F,    0xCCCCCCCC33333333
//num_init: data-in-line test vale3_F
    .equ    num_init3_F,    0x9966996666996699
//num_init: data-in-line test vale4_F
    .equ    num_init4_F,    0xF0F0F0F00F0F0F0F
//num_init: data-in-line test vale5_F
    .equ    num_init5_F,    0xA5A5A5A55A5A5A5A
//num_init: data-in-line test vale6_F
    .equ    num_init6_F,    0xC3C3C3C33C3C3C3C
//num_init: data-in-line test vale7_F
    .equ    num_init7_F,    0x9696969669696969
	
	.equ    line_size,   0x40
	.equ    l2cache_size_s,    0x80000
    .equ    l2cache_size_b,    0x80000	

//ALIGN   64
    .ALIGN 6

    //g_iTestMem_CPUx are buffers definded in .c
    .extern g_iTestMem_CPU0_L2
    .extern g_iTestMem_CPU1_L2
    .extern g_iTestMem_CPU2_L2
    .extern g_iTestMem_CPU3_L2
	.extern g_iTestMem_CPU4_L2
    .extern g_iTestMem_CPU5_L2
    .extern g_iTestMem_CPU6_L2
    .extern g_iTestMem_CPU7_L2

nADDR_CPU0:
    .dword g_iTestMem_CPU0_L2

nADDR_CPU1:
    .dword g_iTestMem_CPU1_L2

nADDR_CPU2:
    .dword g_iTestMem_CPU2_L2

nADDR_CPU3:
    .dword g_iTestMem_CPU3_L2

nADDR_CPU4:
    .dword g_iTestMem_CPU4_L2

nADDR_CPU5:
    .dword g_iTestMem_CPU5_L2

nADDR_CPU6:
    .dword g_iTestMem_CPU6_L2

nADDR_CPU7:
    .dword g_iTestMem_CPU7_L2

//*****************************************************************************
//***    start of main test
//global register:
// x9: core index(0~7); x10: write_iteration;x11:read_iteration;x12: buffer_address
// x13: L2cachesize; x14:line_size;x15:init_vaule;x16:init_value_F
//*****************************************************************************

    .global fp1_memtestL2_start
	
fp1_memtestL2_start:

        STP    x29, x30, [sp,#-16]!
		STP    x27, x28, [sp,#-16]!     
		STP    x25, x26, [sp,#-16]!     
		STP    x23, x24, [sp,#-16]!     
		STP    x21, x22, [sp,#-16]!     
		STP    x19, x20, [sp,#-16]!     
		STP    x17, x18, [sp,#-16]!     
		STP    x15, x16, [sp,#-16]!     
		STP    x13, x14, [sp,#-16]!     
		STP    x11, x12, [sp,#-16]!     
		STP    x9, x10, [sp,#-16]!     
		STP    x7, x8, [sp,#-16]!     
		STP    x5, x6, [sp,#-16]!
		STP    x3, x4, [sp,#-16]!
		STP    x1, x2, [sp,#-16]!
				
    //ALIGN 64 ; align to $ line
    .align 6
	    MOV    x9, x0
		
//check: current core id as same as input argv (core id) ----------begin  
//x1: get coreindex from cpu register info. (0~7)
		//read CPU ID register  bit[7:0] level0 index  bit[15:8] level1 index
		MRS    x1, mpidr_el1
		AND    x1, x1, #0x1ff
				
		TST    x1, #0x100        // check bit[8]（level1 index）-- 1:core(4~7);0:cores(0~3)
		
		B.NE    cpuid_big        // bit[8] != 0
		
		B    cmp_current_cpuid	
		
cpuid_big:  
	    //convert x1 to value(4~7)             
		AND    x1, x1, #0x03        
		ADD    x1, x1, #4
				
cmp_current_cpuid:
		CMP    x1, x0
		//B.NE    cpuid_error        // x1 != x0
			
//check: current core id as same as input argv (core id) ---------end

//start -----------Buffer Init :get buffer address and cache size---------------------
//global register: 
//x10: loop control counter;  x11: init base value（x11+bufferaddress） 
//x12: current buffer address x13:CurrentCache size
        LDR    x10, =write_iteration       // num of iterations round power_loop, it should not be edited by other task
	LDR    x11, =read_iteration
init_buf:		
        CMP    x9, #0
        BEQ    cpu0_init
        CMP    x9, #1
        BEQ    cpu1_init
        CMP    x9, #2
        BEQ    cpu2_init
        CMP    x9, #3
        BEQ    cpu3_init
		
        CMP    x9, #4
        BEQ    cpu4_init
        CMP    x9, #5
        BEQ    cpu5_init
        CMP    x9, #6
        BEQ    cpu6_init
        CMP    x9, #7
        BEQ    cpu7_init

    // every CPU uses full 32KB data, enable full bank access for every CPU ----begin
cpu0_init:
        LDR    x0, nADDR_CPU0        //initialize data address
        LDR    x0, [x0]
        MOV    x12, x0
		LDR    x6, =l2cache_size_s
		LDR    x5, =line_size
		LSL    x11, x11, 1
        B    init_cache_size
cpu1_init:
        LDR    x0, nADDR_CPU1        // initialize data address
        LDR    x0, [x0]
        MOV    x12, x0
		LDR    x6, =l2cache_size_s
		LDR    x5, =line_size
		LSL    x11, x11, 1
        B    init_cache_size
cpu2_init:
        LDR    x0, nADDR_CPU2        // initialize data address
        LDR    x0, [x0]
        MOV    x12, x0
		LDR    x6, =l2cache_size_s
		LDR    x5, =line_size
		LSL    x11, x11, 1
        B    init_cache_size
cpu3_init:
        LDR    x0, nADDR_CPU3        // initialize data address
        LDR    x0, [x0]
        MOV    x12, x0
		LDR    x6, =l2cache_size_s
		LDR    x5, =line_size
		LSL    x11, x11, 1
        B    init_cache_size
cpu4_init:
        LDR    x0, nADDR_CPU4        // initialize data address
        LDR    x0, [x0]
        MOV    x12, x0
		LDR    x6, =l2cache_size_b
		LDR    x5, =line_size
        B    init_cache_size
cpu5_init:
        LDR    x0, nADDR_CPU5        // initialize data address
        LDR    x0, [x0]
        MOV    x12, x0
		LDR    x6, =l2cache_size_b
		LDR    x5, =line_size
        B    init_cache_size
cpu6_init:
        LDR    x0, nADDR_CPU6        // initialize data address
        LDR    x0, [x0]
        MOV    x12, x0
		LDR    x6, =l2cache_size_b
		LDR    x5, =line_size
        B    init_cache_size
cpu7_init:
        LDR    x0, nADDR_CPU7        // initialize data address
        LDR    x0, [x0]
        MOV    x12, x0
		LDR    x6, =l2cache_size_b
		LDR    x5, =line_size
        B    init_cache_size

init_cache_size:		
        MOV    x0, #2
        MSR    csselr_el1,x0    
        MRS    x0, CCSIDR_EL1
		AND    x1, x0, #7
		ADD    x1, x1, #4
		UBFX   x2, x0, #3, #10
		ADD    x2, x2, #1
		UBFX   x3, x0, #13, #15
		ADD    x3, x3, #1
		
		MUL    x13, x2, x3
		LSL    x13, x13, x1
		
		MOV    x14, #1
		LSL    x14, x14, x1
		
		CMP    x13, x6
		BNE    l2_size_error
		CMP    x14, x5
		BNE    l2_size_error
 
//-----------------Buffer Init :get buffer address and cache size------------------end  

//main process to run "num_iteration" times and check error occur-----begin

l2_write_loop:
//start -----------test value Init: x14 ------------------------
        AND    x0, x10,0x07
		
		CMP    x0, #0
		BEQ    value0_init
		CMP    x0, #1
		BEQ    value1_init
		CMP    x0, #2
		BEQ    value2_init
		CMP    x0, #3
		BEQ    value3_init
		CMP    x0, #4
		BEQ    value4_init
		CMP    x0, #5
		BEQ    value5_init
		CMP    x0, #6
		BEQ    value6_init
		CMP    x0, #7
		BEQ    value7_init

value0_init:
        LDR    x15, =num_init0
        LDR    x16, =num_init0_F		
		B    start_l2_test
value1_init:
        LDR    x15, =num_init1
        LDR    x16, =num_init1_F		
		B    start_l2_test
value2_init:
        LDR    x15, =num_init2
        LDR    x16, =num_init2_F		
		B    start_l2_test
value3_init:
        LDR    x15, =num_init3
        LDR    x16, =num_init3_F			
		B    start_l2_test
value4_init:
        LDR    x15, =num_init4
        LDR    x16, =num_init4_F		
		B    start_l2_test
value5_init:
        LDR    x15, =num_init5
        LDR    x16, =num_init5_F				
		B    start_l2_test
value6_init:
        LDR    x15, =num_init6
        LDR    x16, =num_init6_F		
		B      start_l2_test
value7_init:
        LDR    x15, =num_init7
        LDR    x16, =num_init7_F		
		B    start_l2_test
//-----------------test value Init: x14 ---------------------end

start_l2_test:
    //-----------------------------------------------------------------------------
    //init0: init nADDR_CPUn 32K bytes Buffer (size = (index_iteration + 1)*32) with "0"----begin
    //Register:
    //    x0: temp buffer address
    //    x1: loop counter (init with "0")
    //    x2: looptimes - 1 (0x3ff) each time 32 bytes
    //    x3: init value ("0")
    //    x4: init value ("0")
    //-----------------------------------------------------------------------------		

//begin-----------------write init value --------------------
        MOV    x0, x12
        MOV    x1, #0
        MOV    x2, x15
		MOV    x3, x15 
		
line_write_loop:
        STP    x2, x3, [x0],#0x10
		STP    x2, x3, [x0],#0x10
		STP    x2, x3, [x0],#0x10
		STP    x2, x3, [x0],#0x10
		
		ADD    x1, x1, #0x40
		CMP    x1, x13
		
		BNE   line_write_loop
	
//----------------------write init value-------------------end

 __flush_dcache_all:
         dmb     sy                              // ensure ordering with previous memory accesses
         mrs     x0, clidr_el1                   // read clidr
         and     x3, x0, #0x7000000              // extract loc from clidr
         lsr     x3, x3, #23                     // left align loc bit field
         cbz     x3, finished                    // if loc is 0, then no need to clean
         mov     x20, #0                         // start clean at cache level 0
loop1:
         add     x2, x20, x20, lsr #1            // work out 3x current cache level
        lsr     x1, x0, x2                      // extract cache type bits from clidr
        and     x1, x1, #7                      // mask of the bits for current cache only
         cmp     x1, #2                          // see what cache we have at this level
         b.lt    skip                            // skip if no cache, or just i-cache
//         save_and_disable_irqs x19                // make CSSELR and CCSIDR access atomic
         msr     csselr_el1, x20                 // select current cache level in csselr
         isb                                     // isb to sych the new cssr&csidr
         mrs     x1, ccsidr_el1                  // read the new ccsidr
//         restore_irqs x19
         and     x2, x1, #7                      // extract the length of the cache lines
         add     x2, x2, #4                      // add 4 (line length offset)
         mov     x4, #0x3ff
         and     x4, x4, x1, lsr #3              // find maximum number on the way size
         clz     w5, w4                          // find bit position of way size increment
         mov     x7, #0x7fff
         and     x7, x7, x1, lsr #13             // extract max number of the index size
loop2:
         mov     x19, x4                          // create working copy of max way size
loop3:
         lsl     x6, x19, x5
         orr     x21, x20, x6                    // factor way and cache number into x11
         lsl     x6, x7, x2
         orr     x21, x21, x6                    // factor index number into x11
         dc      cisw, x21                       // clean & invalidate by set/way
         subs    x19, x19, #1                      // decrement the way
         b.ge    loop3
         subs    x7, x7, #1                      // decrement the index
         b.ge    loop2
skip:
         add     x20, x20, #2                    // increment cache number
         cmp     x3, x20
         b.gt    loop1
finished:
         mov     x20, #0                         // swith back to cache level 0
         msr     csselr_el1, x20                 // select current cache level in csselr
         dsb     sy
         isb
//begin-----------------check init value --------------------
//x0 read address ; x1: read counter ; x2 temp data;x3: 4byte_offset read address;
// x4: x0 backup register; x5:read loop counter; x6:correct data
		MOV    x5, x11 
l2_read_loop:
		MOV    x0, x12
		MOV    x1, #0   
line_read_loop:		
		//set x3 
		ADD   x3, x0, #4
		MOV   x6, x15
		//get value from 0-offset address
		LDR   x2, [x0], #8
		CMP   x6, x2
        BNE   error_judge
		
		LDR   x2, [x0], #8
		CMP   x6, x2
        BNE   error_judge
		
		LDR   x2, [x0], #8
		CMP   x6, x2
        BNE   error_judge
		
		LDR   x2, [x0], #8
		CMP   x6, x2
        BNE   error_judge

		LDR   x2, [x0], #8
		CMP   x6, x2
        BNE   error_judge
		
		LDR   x2, [x0], #8
		CMP   x6, x2
        BNE   error_judge		

		LDR   x2, [x0], #8
		CMP   x6, x2
        BNE   error_judge
		
		LDR   x2, [x0], #8
		CMP   x6, x2
        BNE   error_judge			
		
		//save x0 to x4, and set x0 = 4byte_offset
		MOV   x4, x0
		MOV   x0, x3
		MOV   x6, x16
		//get value from 4bytes-offset address
		LDR   x2, [x0], #8
		CMP   x6, x2
        BNE   error_judge
		
		LDR   x2, [x0], #8
		CMP   x6, x2
        BNE   error_judge
		
		LDR   x2, [x0], #8
		CMP   x6, x2
        BNE   error_judge
		
		LDR   x2, [x0], #8
		CMP   x6, x2
        BNE   error_judge

		LDR   x2, [x0], #8
		CMP   x6, x2
        BNE   error_judge
		
		LDR   x2, [x0], #8
		CMP   x6, x2
        BNE   error_judge		

		LDR   x2, [x0], #8
		CMP   x6, x2
        BNE   error_judge

		MOV   x0, x4
		
		ADD   x1, x1, #0x40
		CMP   x1, x13
		BNE   line_read_loop
		
		SUBS  x5, x5, #1
		
		BCS   l2_read_loop
//----------------------check init value-------------------end

        SUBS  x10, x10, #1
		
		BCS  l2_write_loop
		
		B    test_pass
//--------------------------------------------------------end	     
//main process to run "num_iteration" times and check error occur-----end    
error_judge:

	//check L1 cache-----------------------begin
	SUB x0, x0, #8
	//x1 loop counter ;x2 temp data ;x6 correct data ,x0 address ; x4 error times
	MOV    x1, #5
	MOV    x4, #0
	
l1_check_loop:

    LDR    x2, [x0]
	
	CMP    x6, x2
	BEQ    l1_counter_operate
	
    ADD    x4, x4, #1	
	
l1_counter_operate:	
	//loop count check
	SUBS    x1, x1, #1
    BNE    l1_check_loop
	
	CMP    x4, #5
	BNE    error_line
	//check L1 cache-----------------------end
	
	//check L2 cache-----------------------begin
	MOV x1, #3
	MOV x4, #0
	
l2_check_loop:
        
//flush L1 cache data--------------------------begin
//cisw:Way, bits[31:32-A], the number of the way to operate on.
//    :Set, bits[B-1:L], the number of the set to operate on
//    :Bits[L-1:4] Bits[0] are RES0. 
//    :Level, bits [3:1]
//A = Log2(ASSOCIATIVITY)= 2, L = Log2(LINELEN) = 6, B = (L + S), S = Log2(NSETS) = 7.
//Register:
//    x21: cache index. 0 L1-Dcache 2-L2cache
//    x23: Cache Size ID register info of selected cache		
//    x22: way value of dc operation
//    x3: way decrease step of dc operation
//    x24: set value of dc operation    
//    x5: set decrease step of dc operation
//    x26: operation number of DC
//    x7: line size = 2^N(Bytes) x19 = N
//    x19: the number of bit'0' in the head of caceh id register
//    x20: temp data #1 
//-----------------------------------------------------------------------------		
flush_cache_start:
                MOV     x21, #0
flush_dcache:
                MSR     csselr_el1, x21
                MRS     x1, ccsidr_el1
                MOV     x20, #1

//flush cache to memory    
                AND     x7, x23, #7   
                ADD     x7, x7, #4      //x7 log2(linesize) x7=6  64B
                UBFX    x22, x23, #3, #10 
                CLZ     w19, w22         //DC OPS IS 32bits
                LSL     x22, x22, x19     //x2 way number,aligned to position in DC ops
                LSL     x3, x20, x19    //x3 decrement way number per iteration    

flush_way_loop:
                UBFX    x24, x23, #13, #15
                LSL     x24, x24, x7   //x4 set number,aligned to position in DC ops
                LSL     x5, x20, x7  //x5 decrement set number per iteration      

flush_set_loop:
                ORR     x26, x21, x22
                ORR     x26, x26, x24
                DC      CISW, x26
                SUBS    x24, x24, x5
                B.GE    flush_set_loop

                SUBS    x22,x22,x3
                B.GE    flush_way_loop
     
                ADD     x21,x21,#2
                CMP     x21,#2
                B.LS    flush_dcache

//flush L1 cache data--------------------------end

        LDR    x2, [x0]
	CMP    x6, x2
	
	BEQ    l2_counter_operate
	
    ADD    x4, x4, #1	
	
l2_counter_operate:	
	//loop count check
	SUBS x1,x1,#1
    BNE l2_check_loop

	CMP x4, #0
	BEQ error_l1cache
	
	CMP x4, #3
	BNE error_line
	
	//check L2 cache-----------------------end	

    //check memory------------------------------------------------------begin
	MOV x1, #5
	MOV x4, #0
	
memory_check_loop:

        //clear & invalid 
        DC CIVAC, x0
        
        LDNP x2, x3, [x0]
        
	CMP x6, x2
	BEQ m_counter_operate
	ADD x4, x4, #1
	
m_counter_operate:	
	//loop count check
	SUBS x1, x1, #1
        BNE memory_check_loop
   	
	CMP x4, #5
	BEQ error_memory
	
	CMP x4, #0
	BEQ error_l2cache
	
	B error_line
	
//Set return vaule functions. 0:test fail; 1: test success; -1: coreindex error,not run. --------begin			
error_memory:
	mov    x0, #2
    b    test_end
error_l1cache:
    mov    x0, #1
    b   test_end
error_l2cache:
    mov    x0, #1
    b    test_end
error_line:
    mov    x0, #1
    b    test_end
l2_size_error:
        MOV    x0, #-2		
        B    test_end	
cpuid_error:	
        MOV    x0, #-1		
        B    test_end
	
test_pass:
        MOV    x0, #0
        B    test_end

//Set return vaule functions. 0:test fail; 1: test success; -1: coreindex error,not run. --------end
test_end:      
        ldp    x1, x2, [sp], #16     
        ldp    x3, x4, [sp], #16   
        ldp    x5, x6, [sp], #16     
        ldp    x7, x8, [sp], #16     
        ldp    x9, x10, [sp], #16    
        ldp    x11, x12, [sp], #16     
        ldp    x13, x14, [sp], #16     
        ldp    x15, x16, [sp], #16     
        ldp    x17, x18, [sp], #16     
        ldp    x19, x20, [sp], #16     
        ldp    x21, x22, [sp], #16     
        ldp    x23, x24, [sp], #16     
        ldp    x25, x26, [sp], #16     
        ldp    x27, x28, [sp], #16     
        ldp    x29, x30, [sp], #16
        dsb    sy
        isb
        ret   
				
        //ALIGN   64
        .align 6

        //END
        .end
