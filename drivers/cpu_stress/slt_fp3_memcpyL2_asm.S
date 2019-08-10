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

//num_iteration: 
    .EQU    num_iteration,    0x10
//num_init: 
    .equ    num_init,    0x0
    .equ    index_iteration,    0x3FF
//addr_iteration  EQU   (index_iteration + 1) << 5 ; 
    .equ    addr_iteration,    ((index_iteration + 1) << 5)
	
//Cache_Line_Size: cache  64 bytes in a line
    .equ    Cache_Line_Size,    0x40

//ALIGN   64
    .ALIGN 6

    //g_iTestMem_CPUx are buffers definded in .c
    .extern g_iTestMem_CPU0
    .extern g_iTestMem_CPU1
    .extern g_iTestMem_CPU2
    .extern g_iTestMem_CPU3
	.extern g_iTestMem_CPU4
    .extern g_iTestMem_CPU5
    .extern g_iTestMem_CPU6
    .extern g_iTestMem_CPU7


nADDR_CPU0:
    .dword g_iTestMem_CPU0

nADDR_CPU1:
    .dword g_iTestMem_CPU1

nADDR_CPU2:
    .dword g_iTestMem_CPU2

nADDR_CPU3:
    .dword g_iTestMem_CPU3

nADDR_CPU4:
    .dword g_iTestMem_CPU4

nADDR_CPU5:
    .dword g_iTestMem_CPU5

nADDR_CPU6:
    .dword g_iTestMem_CPU6

nADDR_CPU7:
    .dword g_iTestMem_CPU7

// *****************************************************************************
// ***    start of main test
//global register:
// x9: core index(0~7)
// *****************************************************************************

//   EXPORT fp3_memcpyL2_start
    .global fp3_memcpyL2_start
fp3_memcpyL2_start:

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

//main process to run "num_iteration" times and check error occur-----begin
//global register: 
//x10: loop control counter;  x11: init base value（x11+bufferaddress） 
//x12: current buffer address

        LDR    x10, =num_iteration       // num of iterations round power_loop, it should not be edited by other task
l2_loop:

        LDR    x11, =num_init

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
        ADD    x11, x0, x11        //initialize write data
        MOV    x12, x0
        B    start_l2_test
cpu1_init:
        LDR    x0, nADDR_CPU1        // initialize data address
        LDR    x0, [x0]
        ADD    x11, x0, x11        // initialize write data
        MOV    x12, x0
        B    start_l2_test
cpu2_init:
        LDR    x0, nADDR_CPU2        // initialize data address
        LDR    x0, [x0]
        ADD    x11, x0, x11        // initialize write data
        MOV    x12, x0
        B    start_l2_test
cpu3_init:
        LDR    x0, nADDR_CPU3        // initialize data address
        LDR    x0, [x0]
        ADD    x11, x0, x11        // initialize write data
        MOV    x12, x0
        B    start_l2_test
cpu4_init:
        LDR    x0, nADDR_CPU4        // initialize data address
        LDR    x0, [x0]
        ADD    x11, x0, x11        // initialize write data
        MOV    x12, x0
        B    start_l2_test
cpu5_init:
        LDR    x0, nADDR_CPU5        // initialize data address
        LDR    x0, [x0]
        ADD    x11, x0, x11        // initialize write data
        MOV    x12, x0
        B    start_l2_test
cpu6_init:
        LDR    x0, nADDR_CPU6        // initialize data address
        LDR    x0, [x0]
        ADD    x11, x0, x11        // initialize write data
        MOV    x12, x0
        B    start_l2_test
cpu7_init:
        LDR    x0, nADDR_CPU7        // initialize data address
        LDR    x0, [x0]
        ADD    x11, x0, x11        // initialize write data
        MOV    x12, x0
        B    start_l2_test
		
start_l2_test:
 
    // every CPU uses full 32KB data, enable full bank access for every CPU ----end

    //-----------------------------------------------------------------------------
    //init0: init nADDR_CPUn 32K bytes Buffer (size = (index_iteration + 1)*32) with "0"----begin
    //Register:
    //    x0: temp buffer address
    //    x1: loop counter (init with "0")
    //    x2: looptimes - 1 (0x3ff) each time 32 bytes
    //    x3: init value ("0")
    //    x4: init value ("0")
    //-----------------------------------------------------------------------------		
        MOV    x0, x12
        MOV    x1, #0
        LDR    x2, =index_iteration
		ADD    x2, x2, #1
        MOV    x3, #0
        MOV    x4, #0

init0:          
        STP    x3, x4, [x0], #16
        DMB    sy
        STP    x3, x4, [x0], #16
        
        ADD    x1, x1, #1		
        CMP    x1, x2
        
        BNE    init0
    //init0: init nADDR_CPUn 32K bytes Buffer (size = (index_iteration + 1)*32) with "0"----end
	
	//set_buffer deferent value:--------------------------------------------------begin
    // Set "long buff[n*4+0]~buf[n*4+3]" = "x11 + n"
    //Register:
    //    x0: temp buffer address
    //    x1: datainfo(init with "x11")
    //    x2: looptimes - 1 (0x3ff) each time 32 bytes
    //-----------------------------------------------------------------------------		
        MOV    x0, x12
        MOV    x1, x11

        LDR    x2, =index_iteration

set_dirty:
        STR    x1, [x0]
        STR    x1, [x0, #8]
        DMB    sy
        STR    x1, [x0, #0x10]
        STR    x1, [x0, #0x18]
				
        ADD    x0, x0, #0x20
				
        CMP    x2, #0
				
        ADD    x1, x1, #1
        SUB    x2, x2, #1
				
        BNE    set_dirty
    //set_buffer deferent value------------------------------------------------------end
      
//load memory data to cache L1 &L2 --------------------------------------------begin
// read 64bit on each line to load data(line size 64byte)
//Register:
//    x0: temp buffer address
//    x1: load temp data(no use)
//    x2: address of the temp buffer end  
//    x3: load temp data(no use)
//-----------------------------------------------------------------------------			

        //clear ECC Info	
        MOV    x0, x12
        LDR    x2, =addr_iteration
        ADD    x2, x0, x2
read_loop:
        LDR    x1, [x0], #0x40
        LDR    x3, [x0], #0x40
	    
	//load 4 line within one loop
        CMP    x0, x2
        BNE    read_loop
//load memory data to cache L1 &L2 -------------------------------------------end 

//check data on memory------------------------------------------------------begin
//data is load from L1. loop every 32byte data
//Register:
//    x0: temp buffer address
//    x1: result value  from memory
//    x2: address of the temp buffer end  
//    x3: temp data from memory 
//    x6: result value of correct data
//-----------------------------------------------------------------------------				
	MOV    x0,x12

	MOV    x6, x11

chk_every_line:
        //load long[n*4] long[n*4+1] to x1,x3 ; x6 = 2*(x11 + n)
        LDR    x1, [x0], #8
        CMP    x6, x1
        BNE    error_judge
		
        LDR    x1, [x0], #8
        CMP    x6, x1
        BNE    error_judge
		
        LDR    x1, [x0], #8
        CMP    x6, x1
        BNE    error_judge
		
        LDR    x1, [x0], #8
        CMP    x6, x1
        BNE    error_judge
		
        ADD    x6, x6, #1
		
		//quit while (x0 == x2)		
        CMP    x0, x2
        BNE    chk_every_line

//flush L1 cache data--------------------------begin
//cisw:Way, bits[31:32-A], the number of the way to operate on.
//    :Set, bits[B-1:L], the number of the set to operate on
//    :Bits[L-1:4] Bits[0] are RES0. 
//    :Level, bits [3:1]
//     A = Log2(ASSOCIATIVITY)= 2, L = Log2(LINELEN) = 6, B = (L + S), S = Log2(NSETS) = 7.
//Register:
//    x0: cache index. 0 L1-Dcache 2-L2cache
//    x1: Cache Size ID register info of selected cache		
//    x2: way value of dc operation
//    x3: way decrease step of dc operation
//    x4: set value of dc operation    
//    x5: set decrease step of dc operation
//    x6: operation number of DC
//    x7: line size = 2^N(Bytes) x19 = N
//    x19: the number of bit'0' in the head of caceh id register
//    x20: temp data #1 
//-----------------------------------------------------------------------------		
flush_cache_start:
                MOV     x0, #0
flush_dcache:
                MSR     csselr_el1, x0
                MRS     x1, ccsidr_el1
                MOV     x20, #1

//flush cache to memory    
                AND     x7, x1, #7   
                ADD     x7, x7, #4      //x7 log2(linesize) x7=6  64B
                UBFX    x2, x1, #3, #10 
                CLZ     w19, w2         //DC OPS IS 32bits
                LSL     x2, x2, x19     //x2 way number,aligned to position in DC ops
                LSL     x3, x20, x19    //x3 decrement way number per iteration    

flush_way_loop:
                UBFX    x4, x1, #13, #15
                LSL     x4, x4, x7     //x4 set number,aligned to position in DC ops
                LSL     x5, x20, x7    //x5 decrement set number per iteration      

flush_set_loop:
                ORR     x6, x0, x2
                ORR     x6, x6, x4
                DC      CISW, x6
                SUBS    x4, x4, x5
                B.GE    flush_set_loop

                SUBS    x2,x2,x3
                B.GE    flush_way_loop
     
                ADD     x0,x0,#2
                CMP     x0,#2
                B.LS    flush_dcache

//flush L1 cache data--------------------------end
//		
//load data from L2 cache--------------------------begin
//after L1 cache flush ,data is loaded to cache 1 from L2 cache 
//Register:
//    x0: temp buffer address
//    x2: address of the temp buffer end
//    x1: temp data 
//    x3: temp data
//    x4: temp data
//    x5: temp data	
//---------------------------------------------------------
        MOV    x0, x12
        LDR    x2, =addr_iteration
        ADD    x2, x0, x2
            
read2_loop:
        LDR    x1, [x0], #0x40
        LDR    x3, [x0], #0x40
        LDR    x4, [x0], #0x40
        LDR    x5, [x0],#0x40
        #;load 4 line within one loop
        CMP    x0, x2
        BNE    read2_loop
//load data from L2 cache--------------------------end
	
//check data on L2 cache------------------------------------------------------begin
//data is load from L2. loop every 32byte data
//Register:
//    x0: temp buffer address
//    x1: result value  from memory
//    x2: address of the temp buffer end  
//    x3: temp data from memory 
//    x6: result value of correct data
//-----------------------------------------------------------------------------		
        MOV    x0, x12
        MOV    x6, x11

chk2_every_line:
        //load long[n*4] long[n*4+1] to x1,x3 ; x6 = 2*(x11 + n)
        LDR    x1, [x0], #8
        CMP    x6, x1
        BNE    error_judge
		
        LDR    x1, [x0], #8
        CMP    x6, x1
        BNE    error_judge
		
        LDR    x1, [x0], #8
        CMP    x6, x1
        BNE    error_judge
		
        LDR    x1, [x0], #8
        CMP    x6, x1
        BNE    error_judge
				
        ADD    x6, x6, #1
		
		//quit while (x0 == x2)	
        CMP    x0, x2
        BNE    chk2_every_line
		
        //check data on L2 cache------------------------------------------------------end
        SUB    x10, x10, #1
        CMP    x10, #0
        BNE    l2_loop

        B    test_pass
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
flush_cache_start_C:
                MOV     x21, #0
flush_dcache_C:
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

flush_way_loop_C:
                UBFX    x24, x23, #13, #15
                LSL     x24, x24, x7   //x4 set number,aligned to position in DC ops
                LSL     x5, x20, x7  //x5 decrement set number per iteration      

flush_set_loop_C:
                ORR     x26, x21, x22
                ORR     x26, x26, x24
                DC      CISW, x26
                SUBS    x24, x24, x5
                B.GE    flush_set_loop_C

                SUBS    x22,x22,x3
                B.GE    flush_way_loop_C
     
                ADD     x21,x21,#2
                CMP     x21,#2
                B.LS    flush_dcache_C

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
	
//Set return vaule functions. 0：test fail; 1: test success; -1: coreindex error,not run. --------begin			
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
	
cpuid_error:	
        MOV    x0, #-1		
        B    test_end
	
test_pass:
        MOV    x0, #0
        B    test_end

//Set return vaule functions. 0：test fail; 1: test success; -1: coreindex error,not run. --------end
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
