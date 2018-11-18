#-----------------------------------------------------------------------------
# The confidential and proprietary information contained in this file may
## only be used by a person authorised under and to the extent permitted
## by a subsisting licensing agreement from ARM Limited.
##
##            (C) COPYRIGHT 2007-2010 ARM Limited.
##                ALL RIGHTS RESERVED
##
## This entire notice must be reproduced on all copies of this file
## and copies of this file may only be made by a person if such person is
## permitted to do so under the terms of a subsisting license agreement
## from ARM Limited.
##
##      RCS Information
##
##      RCS Filename        : $RCSfile$
##
##      Checked In          : $Date$
##
##      Revision            : $Revision: 73865 $
##
##      Release Information : r3p0-00rel0
##-----------------------------------------------------------------------------
##
###-----------------------------------------------------------------------------
###  Find configuration of CPU
###  Variable PERIPH_BASE needs to be updated according to real mapping
###-----------------------------------------------------------------------------
#
##-------------------------------------------------------------------------------
## Init Code
##-------------------------------------------------------------------------------
        .text
        .ALIGN 6

.EQU    num_iteration,     	  0x60
.EQU    num_init,             0x0

## 0x00008000 --> 32K L1 memory copy
## lower 16K memory is copied to upper 16K memory
.EQU    COPY_SIZE,            0x00004000

.EQU    Cache_Line_Size,      0x40

//g_iTestMem_CPUx are buffers definded in .c
.extern g_iTestMem_CPU0_L1
.extern g_iTestMem_CPU1_L1
.extern g_iTestMem_CPU2_L1
.extern g_iTestMem_CPU3_L1
.extern g_iTestMem_CPU4_L1
.extern g_iTestMem_CPU5_L1
.extern g_iTestMem_CPU6_L1
.extern g_iTestMem_CPU7_L1

nADDR_CPU0:
    .dword g_iTestMem_CPU0_L1

nADDR_CPU1:
    .dword g_iTestMem_CPU1_L1

nADDR_CPU2:
    .dword g_iTestMem_CPU2_L1

nADDR_CPU3:
    .dword g_iTestMem_CPU3_L1

nADDR_CPU4:
    .dword g_iTestMem_CPU4_L1

nADDR_CPU5:
    .dword g_iTestMem_CPU5_L1

nADDR_CPU6:
    .dword g_iTestMem_CPU6_L1

nADDR_CPU7:
    .dword g_iTestMem_CPU7_L1
	
        .ALIGN   6

##*****************************************************************************
##***    start of main test
##*****************************************************************************
//x9 :CPUID  ; x10: loop times; x11: init-data; x13:copy_size;
.global fp2_memcpyL1_start
fp2_memcpyL1_start:
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


                .ALIGN 6  // align to $ line
				
                #Be commented in 8953
                MOV    x9, x0
                LDR    x10, =num_iteration
                LDR    x11, =num_init
			    LDR    x13, =COPY_SIZE
               
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
		
                LDR    x11, =num_init
 // every CPU uses full 32KB data, enable full bank access for every CPU ----begin
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

               
cpu0_init:
                LDR    x0, nADDR_CPU0        //initialize data address
                LDR    x0, [x0]
                MOV    x12, x0
                B    start_l1_test
cpu1_init:
                LDR    x0, nADDR_CPU1        // initialize data address
                LDR    x0, [x0]
                MOV    x12, x0
                B    start_l1_test
cpu2_init:
                LDR    x0, nADDR_CPU2        // initialize data address
                LDR    x0, [x0]
                MOV    x12, x0
                B    start_l1_test
cpu3_init:
                LDR    x0, nADDR_CPU3        // initialize data address
                LDR    x0, [x0]
                MOV    x12, x0
                B    start_l1_test
cpu4_init:
                LDR    x0, nADDR_CPU4        // initialize data address
                LDR    x0, [x0]
                MOV    x12, x0
                B    start_l1_test
cpu5_init:
                LDR    x0, nADDR_CPU5        // initialize data address
                LDR    x0, [x0]
                MOV    x12, x0
                B    start_l1_test
cpu6_init:
                LDR    x0, nADDR_CPU6        // initialize data address
                LDR    x0, [x0]
                MOV    x12, x0
                B    start_l1_test
cpu7_init:
                LDR    x0, nADDR_CPU7        // initialize data address
                LDR    x0, [x0]
                MOV    x12, x0
                B    start_l1_test
// every CPU uses full 32KB data, enable full bank access for every CPU ----end
start_l1_test:

//-------------------------begin
                //x0: temp init data0
		//x1: temp init data1
		//x2: temp init data2
		//x3: temp init data3
		//x5: temp control size
		//x6: temp init address
				
		MOV    x0, x11
		MOV    x6, x12
		SUB    x5, x13, #0x40
MEM_SET_64_INC:
		ADD    x1, x0, #1
		ADD    x2, x0, #2
		ADD    x3, x0, #3
				
		STP    x0, x1,[x6],#16
		STP    x2, x3,[x6],#16
		STP    x0, x1,[x6],#16
		STP    x2, x3,[x6],#16
		
		ADD            x0, x0, #4

		SUBS           x5, x5, #0x40
		BCS            MEM_SET_64_INC
				
//flush L1 cache data--------------------------begin
//cisw:Way, bits[31:32-A], the number of the way to operate on.
//    :Set, bits[B-1:L], the number of the set to operate on
//    :Bits[L-1:4] Bits[0] are RES0. 
//    :Level, bits [3:1]
//A = Log2(ASSOCIATIVITY)= 2, L = Log2(LINELEN) = 6, B = (L + S), S = Log2(NSETS) = 7.
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
                LSL     x4, x4, x7   //x4 set number,aligned to position in DC ops
                LSL     x5, x20, x7  //x5 decrement set number per iteration      

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

//--------------------------------------begin
//x19: temp loop counter ; x20: temp address
                LSR      x19, x13, #12
                SUB      x19, x19, #1
                MOV      x20, x12
_next_4k_load:

                #LOAD_NONSEQ_DATA_L1 r8
                LDR     x0, [x20]
                LDR     x1, [x20,#Cache_Line_Size]!
                LDR     x2, [x20,#Cache_Line_Size]!
                LDR     x3, [x20,#Cache_Line_Size]!
                LDR     x4, [x20,#Cache_Line_Size]!
                LDR     x5, [x20,#Cache_Line_Size]!
                LDR     x6, [x20,#Cache_Line_Size]!
                LDR     x7, [x20,#Cache_Line_Size]!
                LDR     x0, [x20,#Cache_Line_Size]!
                LDR     x1, [x20,#Cache_Line_Size]!
                LDR     x2, [x20,#Cache_Line_Size]!
                LDR     x3, [x20,#Cache_Line_Size]!
                LDR     x4, [x20,#Cache_Line_Size]!
                LDR     x5, [x20,#Cache_Line_Size]!
                LDR     x6, [x20,#Cache_Line_Size]!
                LDR     x7, [x20,#Cache_Line_Size]!
                LDR     x0, [x20,#Cache_Line_Size]!
                LDR     x1, [x20,#Cache_Line_Size]!
                LDR     x2, [x20,#Cache_Line_Size]!
                LDR     x3, [x20,#Cache_Line_Size]!
                LDR     x4, [x20,#Cache_Line_Size]!
                LDR     x5, [x20,#Cache_Line_Size]!
                LDR     x6, [x20,#Cache_Line_Size]!
                LDR     x7, [x20,#Cache_Line_Size]!
                LDR     x0, [x20,#Cache_Line_Size]!
                LDR     x1, [x20,#Cache_Line_Size]!
                LDR     x2, [x20,#Cache_Line_Size]!
                LDR     x3, [x20,#Cache_Line_Size]!
                LDR     x4, [x20,#Cache_Line_Size]!
                LDR     x5, [x20,#Cache_Line_Size]!
                LDR     x6, [x20,#Cache_Line_Size]!
                LDR     x7, [x20,#Cache_Line_Size]!
                LDR     x0, [x20,#Cache_Line_Size]!
                LDR     x1, [x20,#Cache_Line_Size]!
                LDR     x2, [x20,#Cache_Line_Size]!
                LDR     x3, [x20,#Cache_Line_Size]!
                LDR     x4, [x20,#Cache_Line_Size]!
                LDR     x5, [x20,#Cache_Line_Size]!
                LDR     x6, [x20,#Cache_Line_Size]!
                LDR     x7, [x20,#Cache_Line_Size]!
                LDR     x0, [x20,#Cache_Line_Size]!
                LDR     x1, [x20,#Cache_Line_Size]!
                LDR     x2, [x20,#Cache_Line_Size]!
                LDR     x3, [x20,#Cache_Line_Size]!
                LDR     x4, [x20,#Cache_Line_Size]!
                LDR     x5, [x20,#Cache_Line_Size]!
                LDR     x6, [x20,#Cache_Line_Size]!
                LDR     x7, [x20,#Cache_Line_Size]!
                LDR     x0, [x20,#Cache_Line_Size]!
                LDR     x1, [x20,#Cache_Line_Size]!
                LDR     x2, [x20,#Cache_Line_Size]!
                LDR     x3, [x20,#Cache_Line_Size]!
                LDR     x4, [x20,#Cache_Line_Size]!
                LDR     x5, [x20,#Cache_Line_Size]!
                LDR     x6, [x20,#Cache_Line_Size]!
                LDR     x7, [x20,#Cache_Line_Size]!
                LDR     x0, [x20,#Cache_Line_Size]!
                LDR     x1, [x20,#Cache_Line_Size]!
                LDR     x2, [x20,#Cache_Line_Size]!
                LDR     x3, [x20,#Cache_Line_Size]!
                LDR     x4, [x20,#Cache_Line_Size]!
                LDR     x5, [x20,#Cache_Line_Size]!
                LDR     x6, [x20,#Cache_Line_Size]!
                LDR     x7, [x20,#Cache_Line_Size]!

                MOV      x7, #0
                dsb sy

                # r8 is a variable
                # r0~r7 is in used
                ADD    x20,x20,#0x08
                SUBS    x19,x19,#1
                B.CS    _next_4k_load
                //--------------------------------------end
				
                //load data from part1 to part2--------------------begin
                //x1: temp data1
                //x2: temp data2
                //x5: loop counter
                //x6: temp address part1
                //x7: temp address part2
_next_iter_loop:
                MOV    x6, x12
                ADD    x7, x12, x13

                SUB    x5, x13, #0x40
MEM_COPY_DW_64:

                LDP    x0,x1,[x6],#0x10
                STP    x0,x1,[x7],#0x10

                LDP    x0,x1,[x6],#0x10
                STP    x0,x1,[x7],#0x10
				
                LDP    x0,x1,[x6],#0x10
                STP    x0,x1,[x7],#0x10
				
                LDP    x0,x1,[x6],#0x10
                STP    x0,x1,[x7],#0x10

                SUBS    x5, x5, #0x40
				
                B.CS    MEM_COPY_DW_64
				
                //--------------------------------------begin
				//x0:temp data; x4: temp init data; x5: temp compair data ; x6: dest address; x7: loop counter
                MOV    x4, x11
                MOV    x7, x12
                ADD    x6, x7, x13 //destination address
                SUB    x7, x13, #0x40 // address count
                
MEM_CMP2_64_INC:
                LDR           x0, [x6], #8
                ADD           x5, x4, #0
                CMP           x5, x0
                B.NE          error_judge
				
                LDR           x0, [x6], #8
                ADD           x5, x4, #1
                CMP           x5, x0
                B.NE          error_judge
				
                LDR           x0, [x6], #8
                ADD           x5, x4, #2
                CMP           x5, x0
                B.NE          error_judge
				
                LDR           x0, [x6], #8
                ADD           x5, x4, #3
                CMP           x5, x0
                B.NE          error_judge
				
                LDR           x0, [x6], #8
                ADD           x5, x4, #0
                CMP           x5, x0
                B.NE          error_judge
				
                LDR           x0, [x6], #8
                ADD           x5, x4, #1
                CMP           x5, x0
                B.NE          error_judge

                LDR           x0, [x6], #8
                ADD           x5, x4, #2
                CMP           x5, x0
                B.NE          error_judge
				
                LDR           x0, [x6], #8
                ADD           x5, x4, #3
                CMP           x5, x0
                B.NE          error_judge

                ADD           x4, x4, #4

                SUBS           x7, x7, #0x40
                BCS            MEM_CMP2_64_INC

                #r8, $size, $init, $fail_label --> all variable will not be changed
                # r0~r7 is in used
               //--------------------------------------end
                SUBS     x10, x10, #0x01
                BCS      _next_iter_loop

                B       test_pass
//
error_judge:

                //check L1 cache-----------------------begin
                SUB    x6, x6, #8
                MOV    x0, x5
                //x1 loop counter ;x2 temp data ;x0 correct data ,x6 address ; x4 error times
                MOV    x1, #10
                MOV    x4, #0
	
l1_check_loop:

                LDR    x2, [x6]
	
                CMP    x0, x2
                BEQ    l1_counter_operate
	
                ADD    x4, x4, #1	
l1_counter_operate:	
                //loop count check
                SUBS    x1, x1, #1
                BNE    l1_check_loop
	
                CMP    x4, #10
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

                LDR    x2, [x6]
                CMP    x0, x2
	
                BEQ    l2_counter_operate
	
                ADD    x4, x4, #1	
	
l2_counter_operate:				
                //loop count check
                SUBS x1, x1, #1
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
        
                LDNP x2, x3, [x6]
                CMP x0, x2
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
	
//Set return vaule functions. 0 no test fail; 1: test success; -1: coreindex error,not run. --------begin			
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
                MOV  x0, #-1
                B    test_end

test_pass:
                MOV  x0, #0x0
                B    test_end

test_end:
				ldp x1,x2,[sp],#16     
				ldp x3,x4,[sp],#16   
				ldp x5,x6,[sp],#16     
				ldp x7,x8,[sp],#16     
				ldp x9,x10,[sp],#16    
				ldp x11,x12,[sp],#16     
				ldp x13,x14,[sp],#16     
				ldp x15,x16,[sp],#16     
				ldp x17,x18,[sp],#16     
				ldp x19,x20,[sp],#16     
				ldp x21,x22,[sp],#16     
				ldp x23,x24,[sp],#16     
				ldp x25,x26,[sp],#16     
				ldp x27,x28,[sp],#16     
				ldp x29,x30,[sp],#16
				dsb sy
				isb
				ret   
		.data
                .ALIGN   6
PC_sync_L1_MEMORY:
                .dword 0

                .END
