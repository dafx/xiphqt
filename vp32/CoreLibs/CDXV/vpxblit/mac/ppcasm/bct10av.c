//==========================================================================
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1999 - 2001  On2 Technologies Inc. All Rights Reserved.
//
//--------------------------------------------------------------------------


#include "vfw_pb_interface.h"

extern vector signed short vp3_vConst1;


void bct10av(unsigned char *_ptrScreen, int thisPitch, YUV_BUFFER_CONFIG *src)
{
	register vector unsigned int alpha = (vector unsigned int)(0xff000000, 0xff000000, 0xff000000, 0xff000000);

	/* only use this loop if the dest starts on a 16byte boundary and the pitch is a multiple of 16 */
	if((!((unsigned long)_ptrScreen & 0xf)) && !(thisPitch & 0xf))
	{
		/* destination is 16 byte aligned */
		/* NOTE: compiler uses r25 for vrsave */	
		asm
		{
		    lwz         r26,vp3_vConst1
		    xor         r30,r30,r30             					//;clear hIndex
			vspltisb 	v30,15
		    
			lvx			v0,r26,r30
			vxor		v7,v7,v7
			vspltisb 	v31,2
			
		    lwz         r6, YUV_BUFFER_CONFIG.YBuffer(r5)
			vsplth		v1,v0,1										//1.596 * 32768
			
		    lwz         r7, YUV_BUFFER_CONFIG.UBuffer(r5)
			vsplth		v2,v0,2										//-0.813 * 32768

		    lwz         r8, YUV_BUFFER_CONFIG.VBuffer(r5)
			vsplth		v3,v0,3										//-0.392 * 32768

		    lwz         r9, YUV_BUFFER_CONFIG.YWidth(r5)
			vsplth		v4,v0,4										//2.017 * 32768

		    lwz         r31,YUV_BUFFER_CONFIG.YHeight(r5)
			vsplth		v5,v0,5										//16

		    lwz         r29,YUV_BUFFER_CONFIG.YStride(r5)
			vsplth		v6,v0,6										//128

		    lwz         r28,YUV_BUFFER_CONFIG.UVStride(r5)
			vsplth		v0,v0,0										//1.164 * 32768

		h_loop:
			xor			r10,r10,r10									//y width index
			xor			r27,r27,r27									//uv width index

			xor			r26,r26,r26									//store index


		w_loop:
			lvx			v10,r6,r10									//16 y's
			addi		r10,r10,16
			
			lvsl		v8,r7,r27									//u alignment vector
			vmrghb		v13,v7,v10									//8 unsigned short y's
	cmpw 		r9,r10

			lvsl		v9,r8,r27									//v alignment vector
			vmrglb		v14,v7,v10									//8 unsigned short y's

			lvx			v11,r7,r27									//only interested in 8 u's
			vsubuhm		v13,v13,v5									//y - 16
			
			lvx			v12,r8,r27									//only interested in 8 v's
			vsubuhm		v14,v14,v5									//y - 16
			addi		r27,r27,8

			vslh		v13,v13,v31									

			vslh		v14,v14,v31		

			vperm		v11,v11,v11,v8
			vsrh		v8,v13,v30									//get sign bit
			
			vperm		v12,v12,v12,v9		
			vsrh		v9,v14,v30									//get sign bit

			vmhaddshs	v13,v13,v0,v8								//(1.164 * 32768)(y - 16)
			vmrghb		v11,v7,v11									//convert to unsigned short

			vmhaddshs	v14,v14,v0,v9								//(1.164 * 32768)(y - 16)
			vmrghb		v12,v7,v12									//convert to unsigned short

			vsubuhm		v11,v11,v6									//(Cb - 128)

			vsubuhm		v12,v12,v6									//(Cr - 128)

			vslh		v11,v11,v31

			vslh		v12,v12,v31
			vmrghh		v15,v11,v11									//double (Cb - 128)'s
			
			vmrghh		v16,v12,v12									//double (Cr - 128)'s
			vsrh		v20,v15,v30									//get sign bits		

			vsrh		v21,v16,v30									//get sign bits		

			vmhaddshs	v9,v15,v4,v20								//CbforB							
			vmrglh		v11,v11,v11									//double (Cb - 128)'s
			
			vaddshs		v9,v9,v13									//b

			vmrglh		v12,v12,v12									//double (Cr - 128)'s
			vmhaddshs	v8,v16,v1,v21								//CrforR
			
			vaddshs		v8,v8,v13									//r

			vpkshus		v8,v8,v8									//8 r's
			vmhaddshs	v16,v16,v2,v21								//CrforG
			
			vpkshus		v9,v9,v9									//8 b's
			vmhaddshs	v15,v15,v3,v20								//CbforG
			
			vsubshs		v13,v13,v16									//almost g

			vsubshs		v13,v13,v15 								//g
								
			vsrh		v22,v11,v30									//get sign bits	(Cb - 128)	

			vsrh		v23,v12,v30									//get sign bits	(Cr - 128)	

			vpkshus		v13,v13,v13									//8 g's				
			vmhaddshs	v15,v12,v1,v23								//second CrforR's

			vaddshs		v15,v15,v14									//second r's
	 			
			vmrghb		v8,v7,v8									//0 r 0 r 0 r 0 r 0 r 0 r 0 r 0 r
			vmhaddshs	v16,v11,v4,v23								//second CbforB's
			
			vaddshs		v16,v16,v14									//second b's
			
			vmrghb		v20,v13,v9									//g b g b g b g b g b g b g b g b 
			vmhaddshs	v12,v12,v2,v23								//second CrforG's

			vmrghh		v21,v8,v20									//0 r g b 0 r g b 0 r g b 0 r g b
			vmhaddshs	v11,v11,v3,v22								//second CbforG's					

			vsubshs		v14,v14,v12									//almost second g's
			vmrglh		v20,v8,v20									//0 r g b 0 r g b 0 r g b 0 r g b

			vsubshs		v14,v14,v11									//second g's
			vmrghw		v10,v21,v21									//double h pixels
			
			vor         v10,v10,alpha
			
			stvx 		v10,r3,r26
			vmrglw		v21,v21,v21									//double l pixels
			addi		r26,r26,16
			
			vor         v21,v21,alpha
			
			stvx 		v21,r3,r26
			vpkshus		v15,v15,v15									//8 r's
			addi		r26,r26,16
			
			vpkshus		v16,v16,v16									//8 b's

			vpkshus		v14,v14,v14									//8 g's

			vmrghw		v10,v20,v20									//double h pixels

			vmrglw		v20,v20,v20									//double l pixels
			vor         v10,v10,alpha

			stvx 		v10,r3,r26
			vmrghb		v11,v7,v15									//0 r 0 r 0 r 0 r 0 r 0 r 0 r 0 r
			addi		r26,r26,16
			
			vor         v20,v20,alpha
			
			stvx 		v20,r3,r26
			vmrghb		v14,v14,v16									//g b g b g b g b g b g b g b g b 
			addi		r26,r26,16
			
			vmrghh		v12,v11,v14									//0 r g b 0 r g b 0 r g b 0 r g b
			
			vmrglh		v11,v11,v14									//0 r g b 0 r g b 0 r g b 0 r g b
			
			vmrghw		v10,v12,v12									//double h pixels
			
			vor         v10,v10,alpha

			stvx 		v10,r3,r26
			vmrglw		v12,v12,v12									//double l pixels
			addi		r26,r26,16
			
			vor         v12,v12,alpha

			stvx 		v12,r3,r26
			vmrghw		v10,v11,v11									//double h pixels
			addi		r26,r26,16
			
			vor         v10,v10,alpha

			stvx 		v10,r3,r26
			vmrglw		v11,v11,v11									//double l pixels
			addi		r26,r26,16
			
			vor         v11,v11,alpha

			stvx 		v11,r3,r26
			addi		r26,r26,16
			bne      	w_loop                   

		    slwi        r24,r30,31
		    addi        r30,r30,1               					//;inc line ptr

		    cmpw        r30,r31

		    srawi       r24,r24,31              					//;generate mask
		    subf        r6,r29,r6               					//;next line of y's

		    and         r24,r24,r28             					//;will be 0 or UVStride
		    add         r3,r3,r4                					//;next scan line
		    

		    subf        r7,r24,r7               
		    subf        r8,r24,r8
		    bne         h_loop
		} 
	}
	else
	{
		/* destination is NOT 16 byte aligned */
		/* NOTE: compiler uses r25 for vrsave */	
		asm
		{
		    lwz         r26,vp3_vConst1
		    xor         r30,r30,r30             					//;clear hIndex
			vspltisb 	v30,15
		    
			lvx			v0,r26,r30
			vxor		v7,v7,v7
			vspltisb 	v31,2
			
		    lwz         r6, YUV_BUFFER_CONFIG.YBuffer(r5)
			vsplth		v1,v0,1										//1.596 * 32768
			
		    lwz         r7, YUV_BUFFER_CONFIG.UBuffer(r5)
			vsplth		v2,v0,2										//-0.813 * 32768

		    lwz         r8, YUV_BUFFER_CONFIG.VBuffer(r5)
			vsplth		v3,v0,3										//-0.392 * 32768

		    lwz         r9, YUV_BUFFER_CONFIG.YWidth(r5)
			vsplth		v4,v0,4										//2.017 * 32768

		    lwz         r31,YUV_BUFFER_CONFIG.YHeight(r5)
			vsplth		v5,v0,5										//16

		    lwz         r29,YUV_BUFFER_CONFIG.YStride(r5)
			vsplth		v6,v0,6										//128

		    lwz         r28,YUV_BUFFER_CONFIG.UVStride(r5)
			vsplth		v0,v0,0										//1.164 * 32768

		h_loop_ual:
			xor			r10,r10,r10									//y width index
			xor			r27,r27,r27									//uv width index

			xor			r26,r26,r26									//store index


		w_loop_ual:
			lvx			v10,r6,r10									//16 y's
			addi		r10,r10,16
			
			lvsl		v8,r7,r27									//u alignment vector
			vmrghb		v13,v7,v10									//8 unsigned short y's
	cmpw 		r9,r10

			lvsl		v9,r8,r27									//v alignment vector
			vmrglb		v14,v7,v10									//8 unsigned short y's

			lvx			v11,r7,r27									//only interested in 8 u's
			vsubuhm		v13,v13,v5									//y - 16
			
			lvx			v12,r8,r27									//only interested in 8 v's
			vsubuhm		v14,v14,v5									//y - 16
			addi		r27,r27,8

			vslh		v13,v13,v31									

			vslh		v14,v14,v31		

			vperm		v11,v11,v11,v8
			vsrh		v8,v13,v30									//get sign bit
			
			vperm		v12,v12,v12,v9		
			vsrh		v9,v14,v30									//get sign bit

			vmhaddshs	v13,v13,v0,v8								//(1.164 * 32768)(y - 16)
			vmrghb		v11,v7,v11									//convert to unsigned short

			vmhaddshs	v14,v14,v0,v9								//(1.164 * 32768)(y - 16)
			vmrghb		v12,v7,v12									//convert to unsigned short

			vsubuhm		v11,v11,v6									//(Cb - 128)

			vsubuhm		v12,v12,v6									//(Cr - 128)

			vslh		v11,v11,v31

			vslh		v12,v12,v31
			vmrghh		v15,v11,v11									//double (Cb - 128)'s
			
			vmrghh		v16,v12,v12									//double (Cr - 128)'s
			vsrh		v20,v15,v30									//get sign bits		

			vsrh		v21,v16,v30									//get sign bits		

			vmhaddshs	v9,v15,v4,v20								//CbforB							
			vmrglh		v11,v11,v11									//double (Cb - 128)'s
			
			vaddshs		v9,v9,v13									//b

			vmrglh		v12,v12,v12									//double (Cr - 128)'s
			vmhaddshs	v8,v16,v1,v21								//CrforR
			
			vaddshs		v8,v8,v13									//r

			vpkshus		v8,v8,v8									//8 r's
			vmhaddshs	v16,v16,v2,v21								//CrforG
			
			vpkshus		v9,v9,v9									//8 b's
			vmhaddshs	v15,v15,v3,v20								//CbforG
			
			vsubshs		v13,v13,v16									//almost g

			vsubshs		v13,v13,v15 								//g
								
			vsrh		v22,v11,v30									//get sign bits	(Cb - 128)	

			vsrh		v23,v12,v30									//get sign bits	(Cr - 128)	

			vpkshus		v13,v13,v13									//8 g's				
			vmhaddshs	v15,v12,v1,v23								//second CrforR's

			vaddshs		v15,v15,v14									//second r's
	 			
			vmrghb		v8,v7,v8									//0 r 0 r 0 r 0 r 0 r 0 r 0 r 0 r
			vmhaddshs	v16,v11,v4,v23								//second CbforB's
			
			vaddshs		v16,v16,v14									//second b's
			
			vmrghb		v20,v13,v9									//g b g b g b g b g b g b g b g b 
			vmhaddshs	v12,v12,v2,v23								//second CrforG's

			vmrghh		v21,v8,v20									//0 r g b 0 r g b 0 r g b 0 r g b
			vmhaddshs	v11,v11,v3,v22								//second CbforG's					

			lvsr		v10,r3,r26									//store alignment vector
			vsubshs		v14,v14,v12									//almost second g's
//			vperm		v21,v21,v21,v10

			vsubshs		v14,v14,v11									//second g's
			vmrglh		v20,v8,v20									//0 r g b 0 r g b 0 r g b 0 r g b

			vmrglw		v8,v21,v21							

			vmrghw		v21,v21,v21							

			vperm		v21,v21,v21,v10
			vperm		v8,v8,v8,v10
			
			vor         v21,v21,alpha

			stvewx		v21,r3,r26									//pixel 0
//			vperm		v20,v20,v20,v10
			addi		r26,r26,4
			
			stvewx		v21,r3,r26									//pixel 1
			vpkshus		v15,v15,v15									//8 r's
			addi		r26,r26,4
			
			stvewx		v21,r3,r26									//pixel 2
			vpkshus		v16,v16,v16									//8 b's
			addi		r26,r26,4

			stvewx		v21,r3,r26									//pixel 3
			vpkshus		v14,v14,v14									//8 g's
			addi		r26,r26,4
			
			vor         v8,v8,alpha

			stvewx		v8,r3,r26									//pixel 4
			vmrghb		v11,v7,v15									//0 r 0 r 0 r 0 r 0 r 0 r 0 r 0 r
			addi		r26,r26,4
			
			stvewx		v8,r3,r26									//pixel 5
			vmrghb		v14,v14,v16									//g b g b g b g b g b g b g b g b 
			addi		r26,r26,4
			
			stvewx		v8,r3,r26									//pixel 6
			vmrghh		v12,v11,v14									//0 r g b 0 r g b 0 r g b 0 r g b
			addi		r26,r26,4
			
			stvewx		v8,r3,r26									//pixel 7
			vmrglh		v11,v11,v14									//0 r g b 0 r g b 0 r g b 0 r g b
			addi		r26,r26,4


			lvsr		v10,r3,r26									//store alignment vector
			vmrglw		v9,v20,v20							

			vmrghw		v20,v20,v20							

			vperm		v20,v20,v20,v10
			
			vor         v20,v20,alpha

			stvewx		v20,r3,r26									//pixel 8
			vperm		v9,v9,v9,v10
//			vperm		v12,v12,v12,v10
			addi		r26,r26,4

			stvewx		v20,r3,r26									//pixel 9
			vor         v9,v9,alpha
			addi		r26,r26,4

			stvewx		v20,r3,r26									//pixel 10
			addi		r26,r26,4

			stvewx		v20,r3,r26									//pixel 11
			addi		r26,r26,4

			stvewx		v9,r3,r26									//pixel 12
			addi		r26,r26,4

			stvewx		v9,r3,r26									//pixel 13
			addi		r26,r26,4

			stvewx		v9,r3,r26									//pixel 14
			vmrglw		v8,v12,v12							
			addi		r26,r26,4

			stvewx		v9,r3,r26									//pixel 15
			vmrghw		v12,v12,v12							
			addi		r26,r26,4

			lvsr		v10,r3,r26									//store alignment vector

			vperm		v12,v12,v12,v10

			vor         v12,v12,alpha
			
			stvewx		v12,r3,r26									//pixel 16
			vperm		v8,v8,v8,v10
//			vperm		v11,v11,v11,v10
			addi		r26,r26,4

			stvewx		v12,r3,r26									//pixel 17
			vmrglw		v9,v11,v11							
			addi		r26,r26,4
			
			stvewx		v12,r3,r26									//pixel 18
			vmrghw		v11,v11,v11							
			addi		r26,r26,4

			stvewx		v12,r3,r26									//pixel 19
			vor         v8,v8,alpha
			addi		r26,r26,4

			stvewx		v8,r3,r26									//pixel 20
			addi		r26,r26,4

			stvewx		v8,r3,r26									//pixel 21
			addi		r26,r26,4

			stvewx		v8,r3,r26									//pixel 22
			addi		r26,r26,4

			stvewx		v8,r3,r26									//pixel 23
			addi		r26,r26,4


			lvsr		v10,r3,r26									//store alignment vector

			vperm		v11,v11,v11,v10

			vor         v11,v11,alpha

			stvewx		v11,r3,r26									//pixel 24
			vperm		v9,v9,v9,v10
			addi		r26,r26,4

			stvewx		v11,r3,r26									//pixel 25
			vor         v9,v9,alpha
			addi		r26,r26,4

			stvewx		v11,r3,r26									//pixel 26
			addi		r26,r26,4

			stvewx		v11,r3,r26									//pixel 27
			addi		r26,r26,4

			stvewx		v9,r3,r26									//pixel 28
			addi		r26,r26,4

			stvewx		v9,r3,r26									//pixel 29
			addi		r26,r26,4

			stvewx		v9,r3,r26									//pixel 30
			addi		r26,r26,4

			stvewx		v9,r3,r26									//pixel 31
			addi		r26,r26,4
			bne      	w_loop_ual                   

		    slwi        r24,r30,31
		    addi        r30,r30,1               					//;inc line ptr

		    cmpw        r30,r31

		    srawi       r24,r24,31              					//;generate mask
		    subf        r6,r29,r6               					//;next line of y's

		    and         r24,r24,r28             					//;will be 0 or UVStride
		    add         r3,r3,r4                					//;next scan line
		    

		    subf        r7,r24,r7               
		    subf        r8,r24,r8
		    bne         h_loop_ual
		} 		
	}
}

