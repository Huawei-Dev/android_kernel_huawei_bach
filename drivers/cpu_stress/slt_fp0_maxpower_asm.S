//;-----------------------------------------------------------------------------
//; The confidential and proprietary information contained in this file may
//; only be used by a person authorised under and to the extent permitted
//; by a subsisting licensing agreement from ARM Limited.
//;
//;            (C) COPYRIGHT 2007-2010 ARM Limited.
//;                ALL RIGHTS RESERVED
//;
//; This entire notice must be reproduced on all copies of this file
//; and copies of this file may only be made by a person if such person is
//; permitted to do so under the terms of a subsisting license agreement
//; from ARM Limited.
//;
//;      RCS Information
//;
//;      RCS Filename        : $RCSfile$
//;
//;      Checked In          : $Date$
//;
//;      Revision            : $Revision: 73865 $
//;
//;      Release Information : r3p0-00rel0
//;-----------------------------------------------------------------------------
//;
//;;-----------------------------------------------------------------------------
//;;  Find configuration of CPU
//;;  Variable PERIPH_BASE needs to be updated according to real mapping
//;;-----------------------------------------------------------------------------


//AREA INIT, CODE, ALIGN=6
        .text

//num_iteration:the number of loop for "power_loop"
        .equ    num_iteration,    0x4000
    
//8k global buffer used by every cells.it is defined in maxpower.c 
        .extern    pMaxPowerTestMem       
test_mem:
        .dword    pMaxPowerTestMem
	
//data to be checked in the "power_loop"
        .equ    test_pattern,    0x5a5a5a5a5a5a5a5a         
    
//ALIGN   64
	.align 6

//;*****************************************************************************
//;***    start of main test
//;*****************************************************************************       
//EXPORT fp0_maxpower_start
        .global fp0_maxpower_start
	
fp0_maxpower_start:
        // save all register excecpt input argv "x0"
        STP    x29, x30, [sp, #-16]!
        STP    x27, x28, [sp, #-16]!     
        STP    x25, x26, [sp, #-16]!     
        STP    x23, x24, [sp, #-16]!     
        STP    x21, x22, [sp, #-16]!     
        STP    x19, x20, [sp, #-16]!     
        STP    x17, x18, [sp, #-16]!     
        STP    x15, x16, [sp, #-16]!     
        STP    x13, x14, [sp, #-16]!     
        STP    x11, x12, [sp, #-16]!     
        STP    x9, x10, [sp, #-16]!     
        STP    x7, x8, [sp, #-16]!     
        STP    x5, x6, [sp, #-16]!
        STP    x3, x4, [sp, #-16]!
        STP    x1, x2, [sp, #-16]!              

        //global register x9 = core_id
        mov x9, x0
		
//check: current core id as same as input argv (core id) ----begin     
  		
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
			
//check: current core id as same as input argv (core id) ----end  		

//loops of "no means operatetions of register" . check register value stability   --- begin
// x12:(the value of  "pMaxPowerTestMem+4k") ; x11: loop control counter;
// x10: standard value (test_pattern); x13: load memory value to compare with standard value
// x0~x7:temp operate value
        //set temp register x12. 
        LDR    x12, test_mem
        LDR    x12, [x12]
        ADD    x12, x12, #0x1000              

        //init the operate temp register
        ADD    x0, x12, #0
        ADD    x1, x12, #-3
        ADD    x2, x12, #-4
        ADD    x3, x12, #6
        ADD    x4, x12, #0
        ADD    x5, x12, #-3
        ADD    x6, x12, #-4
        ADD    x7, x12, #6
        
        //save test_pattern to address "x12+2"       
        LDR	x10, =test_pattern
        SUB     x7, x7, x12             //x7 = 6   
        ADD     x6, x6, x7              //x6 = address "x12+2"
        STR     x10, [x6]               //store value
        ADD     x7, x7, x12             //recovery x7 to "x12+6"	
		

        LDR     x11,=num_iteration        // init loops couter
//power_loop ----begin
power_loop:
	
        SUBS    x11, x11, #1        //decrease loop couter,set "cpsr" register

        //Group-1 (after operations every temp register have no change,but x13 = num_iteration)
        SUB    x1, x1, x12, LSR #1        //x1 = (x12/2) -3      
        SUB    x0, x0, x12                //x0 = 0   
        ADD    x0, x0, x1, LSL#1		  //x0 = x12 - 6 		
        LDR    x13, [x0]                  //load [x12-6] -> x13          
        MOV    x6, x2                     //x6 = x12 + 2
        ADD    x1, x1, x12, LSR# 1        //x1 = x12 - 3  

        SUB    x3, x3, x12                //x3 =  6		
        ADD    x2, x2, x3                 //x2 = x12 + 2
        LDR    x13, [x2]                  //load [x12+2] -> x13!!!!(test_pattern)
        ADD    x0, x1, #0x003             //x0 = x12
        ADD    x3, x3, x12                //x3 = x12 + 6

        SUB    x5, x5, x12, LSR #1        //x5 = (x12/2) - 3
        SUB    x4, x4, x12                //x4 = 0
        ADD    x4, x4, x5, LSL#1          //x4 = x12 - 6
        
        LDR    x13, [x4]            	  //load [x12-6] -> x13              
        SUB    x2, x3, #0x00a             //x2 = x12 - 4
        ADD    x5, x5, x12, LSR #1        //x5 = x12 - 3 

        SUB    x7, x7, x12                //x7 = 6
        ADD    x6, x6, x7                 //x6 = x12 + 2
        LDR    x13, [x6]                   //load [x12+2] -> x13!!!!(test_pattern)
        ADD    x4, x5, #0x003             //x4 = x12
        ADD    x7, x7, x12                //x7 = x12 + 6

        //Group-2 (after operations every temp register have no change,but x13 = num_iteration)
        SUB    x1, x1, x12, LSR #1        //x1 = (x12/2) -3      
        SUB    x0, x0, x12                //x0 = 0   
        ADD    x0, x0, x1, LSL#1		  //x0 = x12 - 6 		
        LDR    x13, [x0]                  //load [x12-6] -> x13          
        MOV    x6, x2                     //x6 = x12 + 2
        ADD    x1, x1, x12, LSR# 1        //x1 = x12 - 3  

        SUB    x3, x3, x12                //x3 =  6		
        ADD    x2, x2, x3                 //x2 = x12 + 2
        LDR    x13, [x2]                  //load [x12+2] -> x13!!!!(test_pattern)
        ADD    x0, x1, #0x003             //x0 = x12
        ADD    x3, x3, x12                //x3 = x12 + 6

        SUB    x5, x5, x12, LSR #1        //x5 = (x12/2) - 3
        SUB    x4, x4, x12                //x4 = 0
        ADD    x4, x4, x5, LSL#1          //x4 = x12 - 6
        
        LDR    x13, [x4]            	      //load [x12-6] -> x13              
        SUB    x2, x3, #0x00a             //x2 = x12 - 4
        ADD    x5, x5, x12, LSR #1        //x5 = x12 - 3 

        SUB    x7, x7, x12                //x7 = 6
        ADD    x6, x6, x7                 //x6 = x12 + 2
        LDR    x13, [x6]                   //load [x12+2] -> x13!!!!(test_pattern)
        ADD    x4, x5, #0x003             //x4 = x12
        ADD    x7, x7, x12                //x7 = x12 + 6

        //Group-3 (after operations every temp register have no change,but x13 = num_iteration)
        SUB    x1, x1, x12, LSR #1        //x1 = (x12/2) -3      
        SUB    x0, x0, x12                //x0 = 0   
        ADD    x0, x0, x1, LSL#1		  //x0 = x12 - 6 		
        LDR    x13, [x0]                  //load [x12-6] -> x13          
        MOV    x6, x2                     //x6 = x12 + 2
        ADD    x1, x1, x12, LSR# 1        //x1 = x12 - 3  

        SUB    x3, x3, x12                //x3 =  6		
        ADD    x2, x2, x3                 //x2 = x12 + 2
        LDR    x13, [x2]                  //load [x12+2] -> x13!!!!(test_pattern)
        ADD    x0, x1, #0x003             //x0 = x12
        ADD    x3, x3, x12                //x3 = x12 + 6

        SUB    x5, x5, x12, LSR #1        //x5 = (x12/2) - 3
        SUB    x4, x4, x12                //x4 = 0
        ADD    x4, x4, x5, LSL#1          //x4 = x12 - 6
        LDR    x13, [x4]            	      //load [x12-6] -> x13              
        SUB    x2, x3, #0x00a             //x2 = x12 - 4
        ADD    x5, x5, x12, LSR #1        //x5 = x12 - 3 

        SUB    x7, x7, x12                //x7 = 6
        ADD    x6, x6, x7                 //x6 = x12 + 2
        LDR    x13, [x6]                   //load [x12+2] -> x13!!!!(test_pattern)
        ADD    x4, x5, #0x003             //x4 = x12
        ADD    x7, x7, x12                //x7 =x12 + 6

        //Group-4 (after operations every temp register have no change,but x13 = num_iteration)
        SUB    x1, x1, x12, LSR #1        //x1 = (x12/2) -3      
        SUB    x0, x0, x12                //x0 = 0   
        ADD    x0, x0, x1, LSL#1		  //x0 = x12 - 6 		
        LDR    x13, [x0]                  //load [x12-6] -> x13          
        MOV    x6, x2                     //x6 = x12 + 2
        ADD    x1, x1, x12, LSR# 1        //x1 = x12 - 3  

        SUB    x3, x3, x12                //x3 =  6		
        ADD    x2, x2, x3                 //x2 = x12 + 2
        LDR    x13, [x2]                  //load [x12+2] -> x13!!!!(test_pattern)
        ADD    x0, x1, #0x003             //x0 = x12
        ADD    x3, x3, x12                //x3 = x12 + 6

        SUB    x5, x5, x12, LSR #1        //x5 = (x12/2) - 3
        SUB    x4, x4, x12                //x4 = 0
        ADD    x4, x4, x5, LSL#1          //x4 = x12 - 6
        LDR    x13, [x4]            	  //load [x12-6] -> x13              
        SUB    x2, x3, #0x00a             //x2 = x12 - 4
        ADD    x5, x5, x12, LSR #1        //x5 = x12 - 3 

        SUB    x7, x7, x12                //x7 = 6
        ADD    x6, x6, x7                 //x6 = x12 + 2
        LDR    x13, [x6]                   //load [x12+2] -> x13!!!!(test_pattern)
        ADD    x4, x5, #0x003             //x4 = x12
        ADD    x7, x7, x12                //x7 = x12 + 6

        BNE	   power_loop      //result of counter "x11" != 0
	//power_loop ---end 
	
        //check x2 == x12 - 4
        ADD    x1, x12, #-4        // x1 = x12 - 4
        CMP    x1, x2
        BNE    test_error         //x1 != x2 to test error
		
        //check x4 = x12               
        ADD    x3, x12, #0x0        //x3 = x12
        CMP    x3, x4
        BNE    test_error           //x3 != x4 to test error
		
        //check x6 = r12 + 2                 
        ADD     x5, x12, #0x2        //x5 = x12 + 2
        CMP     x6, x5
        BNE     test_error           //x3 != x4 to test error
		
        //check x8 = test_pattern
        LDR     x7,=test_pattern        //x7 =  test_pattern        
        CMP     x7, x13
        BNE     test_error        //x7 != x8 to test error

        B       test_pass        //test success
		
//loops of "no means operatetions of register" . check register value stability   --- end

cpuid_error:	 

        ISB
		
        MOV    x0, #-1        //return -1
        B    test_end
		
test_error:      

        ISB
        MOV  x0, #0x1        //return 1
        B    test_end

test_pass:       
		ISB

        MOV  x0, #0x0        //return 0
        B    test_end

test_end:            
        LDP    x1, x2, [sp], #16     
        LDP    x3, x4, [sp], #16   
        LDP    x5, x6, [sp], #16     
        LDP    x7, x8, [sp], #16     
        LDP    x9, x10, [sp], #16    
        LDP    x11, x12, [sp], #16     
        LDP    x13, x14, [sp], #16     
        LDP    x15, x16, [sp], #16     
        LDP    x17, x18, [sp], #16     
        LDP    x19, x20, [sp], #16     
        LDP    x21, x22, [sp], #16     
        LDP    x23, x24, [sp], #16     
        LDP    x25, x26, [sp], #16     
        LDP    x27, x28, [sp], #16     
        LDP    x29, x30, [sp], #16
        dsb    sy
        isb
        ret  

        //END
        .end
//;*****************************************************************************
//;***    end of main test
//;*****************************************************************************   