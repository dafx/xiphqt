;//==========================================================================
;//
;//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
;//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
;//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
;//  PURPOSE.
;//
;//  Copyright (c) 1999 - 2001  On2 Technologies Inc. All Rights Reserved.
;//
;//--------------------------------------------------------------------------


;
; **-CC_RGB24toYV12_MMX
; 
; This function will convert a RGB24 buffer to planer YV12 output.
; Alpha is ignored.
;
; See C version of function for description of how this function behaves.
; The C version of the function is called CC_RGB24toYV12 and can be found
; in colorconversions.c
;
; Although this function will run on Pentium processors with MMX built it,
; performance may not be as good as anticipated.  This function was written
; for the Pentium II processor.  As such no attempt was made to pair the 
; instructions properly for the Pentium processor.
;
;
 
        .586
        .387
        .MODEL  flat, SYSCALL, os_dos
        .MMX

        .DATA
TORQ_CX_DATA SEGMENT PAGE PUBLIC USE32 'DATA' 

        ALIGN 32

    Zeros           QWORD   00000000000000000h      ; All Zeros for conversion from 8-bits to 16-bits
    YBZRYGYB        QWORD   00C8B000040830C8Bh      ; 0.504 * ScaleFactor, 0.504 * ScaleFactor, 0.098 * ScaleFactor
    YRYGZRYR        QWORD   020E54083000020E5h      ; 0.257 * ScaleFactor, 0.504 * ScaleFactor, zero, 0.257 * ScaleFactor
    AvgRoundFactor  QWORD   00002000200020002h      ; Round factor used for dividing by 2
    YARYAR          QWORD   00008400000084000h      ; (16 * ScaleFactor) + (ScaleFactor / 2), (16 * ScaleFactor) + (ScaleFactor / 2)
    ZRVRUGUB        QWORD   000003831DAC13831h      ; Zero, 0.439 * ScaleFactor, -0.291 * ScaleFactor, 0.439 * ScaleFactor
    ZRZRZRUR        QWORD   0000000000000ED0Fh      ; Zero, Zero, Zero, -0.148 * ScaleFactor
    VGVBZRZR        QWORD   0D0E6F6EA00000000h      ; -0.368 * ScaleFactor, -0.071 * ScaleFactor, Zero, Zero
    VARUAR          QWORD   00040400000404000h      ; (128 * ScaleFactor) + (ScaleFactor / 2), (128 * ScaleFactor) + (ScaleFactor / 2)    



        .CODE

NAME x86ColorConversions

PUBLIC CC_RGB24toYV12_MMX_
PUBLIC _CC_RGB24toYV12_MMX
 

INCLUDE colorconversions.ash

;------------------------------------------------
; local vars
LOCAL_SPACE     EQU 0


;------------------------------------------------
; int CC_RGB32toYV12( unsigned char *RGBABuffer, int ImageWidth, int ImageHeight,
;                     unsigned char *YBuffer, unsigned char *UBuffer, unsigned char *VBuffer )
;
CC_RGB24toYV12_MMX_:
_CC_RGB24toYV12_MMX:
    push    esi
    push    edi
    push    ebp
    push    ebx 
    push    ecx
    push    edx


;
; ESP = Stack Pointer                      MM0 = Free
; ESI = Free                               MM1 = Free
; EDI = Free                               MM2 = Free
; EBP = Free                               MM3 = Free
; EBX = Free                               MM4 = Free
; ECX = Free                               MM5 = Free
; EDX = Image width * 3                    MM6 = Free
; EAX = Free                               MM7 = Free
;

    mov         esi,(CConvParams PTR [esp]).ImageHeight             ; Load Image height
    shr         esi,1                                               ; divide by 2, we process image two lines at a time
    mov         (CConvParams PTR [esp]).ImageHeight,esi             ; Load Image height
    mov         edi,(CConvParams PTR [esp]).RGBABuffer              ; Input Buffer Ptr
    mov         esi,(CConvParams PTR [esp]).ImageWidth              ; Load Image width
    mov         eax,(CConvParams PTR [esp]).YBuffer                 ; Load Y buffer ptr
    mov         edx,esi                                             ; prepare to multiply width by 3
    add         edx,esi                                             ; multi by 2
    add         edx,esi                                             ; add in to get 3

;
; ESP = Stack Pointer                      MM0 = Free
; ESI = ImageWidth                         MM1 = Free
; EDI = Input Buffer Ptr                   MM2 = Free
; EBP = Width Ctr                          MM3 = Free
; EBX = Free                               MM4 = Free
; ECX = Free                               MM5 = Free
; EDX = Image width * 3                    MM6 = Free
; EAX = Y output buff ptr                  MM7 = Free
;

;

HLoopStart:
    xor         ebp,ebp                                             ; setup width loop ctr

WLoopStart:
;
; ESP = Stack Pointer                      MM0 = Free
; ESI = ImageWidth                         MM1 = Free
; EDI = Input Buffer Ptr                   MM2 = Free
; EBP = Width Ctr                          MM3 = Free
; EBX = Scratch                            MM4 = Free
; ECX = Free                               MM5 = Free
; EDX = Image width * 3                    MM6 = Free
; EAX = Y output buff ptr                  MM7 = Used to accumulate sum of R,G,B values so that they can be averaged
;

; process two pixels from first image row
    movq        mm0,QWORD PTR 0[edi]                                ; G2,B2,R1,G1,B1,R0,G0,B0
    movq        mm1,QWORD PTR 0[edi+edx]                            ; G2,B2,R1,G1,B1,R0,G0,B0 (2nd row)

    movq        mm2,mm0                                             ; G2,B2,R1,G1,B1,R0,G0,B0
    movq        mm4,mm1                                             ; G2,B2,R1,G1,B1,R0,G0,B0 (2nd row)

    movq        mm5,mm0                                             ; G2,B2,R1,G1,B1,R0,G0,B0
    movq        mm6,mm1                                             ; G2,B2,R1,G1,B1,R0,G0,B0 (2nd row)

    punpcklbw   mm0,Zeros                                           ; B1,R0,G0,B0
    punpcklbw   mm1,Zeros                                           ; B1,R0,G0,B0 (2nd row)

    psllq       mm5,8                                               ; B2,R1,G1,B1,R0,G0,B0,00
    psllq       mm6,8                                               ; B2,R1,G1,B1,R0,G0,B0,00 (2nd row)

    psrlq       mm2,16                                              ; 00,00,G2,B2,R1,G1,B1,R0
    psrlq       mm4,16                                              ; 00,00,G2,B2,R1,G1,B1,R0 (2nd row)

    punpckhbw   mm5,Zeros                                           ; B2,R1,G1,B1
    punpckhbw   mm6,Zeros                                           ; B2,R1,G1,B1 (2nd row)

    movq        mm7,mm0                                             ; B1,R0,G0,B0

    punpcklbw   mm2,Zeros                                           ; R1,G1,B1,R0
    punpcklbw   mm4,Zeros                                           ; R1,G1,B1,R0 (2nd row)

    paddd       mm7,mm1                                             ; B1 + B1 , R0 + R0, G0 + G0 ,B0 + B0

    pmaddwd     mm0,YBZRYGYB                                        ; (B1*YB + R0*0) * 8000h, (G0*YB + B0*YB) * 8000h
    paddd       mm7,mm5                                             ; B2+B1+B1, R1+R0+R0, G1+G0+G0, B1+B0+B0

    pmaddwd     mm1,YBZRYGYB                                        ; (B1*YB + R0*0) * 8000h, (G0*YB + B0*YB) * 8000h (2nd row)
    paddd       mm7,mm6                                             ; B2+B2+B1+B1, R1+R1+R0+R0, G1+G1+G0+G0, B1+B1+B0+B0

    pmaddwd     mm2,YRYGZRYR                                        ; (R1*YR + G1*YG) * 8000h, (B1*0 + R0*YR) * 8000h
    pmaddwd     mm4,YRYGZRYR                                        ; (R1*YR + G1*YG) * 8000h, (B1*0 + R0*YR) * 8000h (2nd row)

    paddd       mm7,AvgRoundFactor                                  ; 2+B2+B2+B1+B1, 2+R1+R1+R0+R0, 2+G1+G1+G0+G0, 2+B1+B1+B0+B0    
    paddd       mm0,YARYAR                                          ; (B1*YB + R0*0 + 16 + 4000h) * 8000h, (G0*YB + B0*YB + 16 + 4000h) * 8000h
    paddd       mm1,YARYAR                                          ; (B1*YB + R0*0 + 16 + 4000h) * 8000h, (G0*YB + B0*YB + 16 + 4000h) * 8000h (2nd row)

    psrlw       mm7,2                                               ; Avg(B), Avg(R), Avg(G), Avg(B)

    paddd       mm0,mm2                                             ; (R1*YR + G1*YG + B1*YB + 16 + 4000h) * 8000h, (R0*YR + G0*YB + B0*YB + 16 + 4000h) * 8000h
    paddd       mm1,mm4                                             ; (R1*YR + G1*YG + B1*YB + 16 + 4000h) * 8000h, (R0*YR + G0*YB + B0*YB + 16 + 4000h) * 8000h (2nd row)

;--
    movq        mm2,mm7                                             ; Avg(B), Avg(R), Avg(G), Avg(B)
    movq        mm3,mm7                                             ; Avg(B), Avg(R), Avg(G), Avg(B)

    psrld       mm0,15                                              ; ((R1*YR + G1*YG + B1*YB + 16 + 4000h) * 8000h) / 8000h, ((R0*YR + G0*YB + B0*YB + 16 + 4000h) * 8000h) / 8000h
    psrld       mm1,15                                              ; ((R1*YR + G1*YG + B1*YB + 16 + 4000h) * 8000h) / 8000h, ((R0*YR + G0*YB + B0*YB + 16 + 4000h) * 8000h) / 8000h (2nd row)

    pmaddwd     mm7,ZRVRUGUB                                        ; (B*0 + R*VR) * 8000h, (G*UG + B*UB) * 8000h

    packssdw    mm0,mm1                                             ; Y1, Y0, Y1, Y0
    
    psrlq       mm2,32                                              ; 0, 0, B, R
    psllq       mm3,32                                              ; G, B, 0, 0

    pmaddwd     mm2,ZRZRZRUR                                        ; (0*0 + 0*0) * 8000h, (B*0 + R*UR) * 8000h
    packuswb    mm0,mm6                                             ; X, X, X, X, Y1, Y0, Y1, Y0

    pmaddwd     mm3,VGVBZRZR                                        ; (G*VG + B*VB) * 8000h, (0*0 + 0*0) * 8000h
    movd        ebx,mm0                                             ; Y1, Y0, Y1, Y0

    paddd       mm7,VARUAR                                          ; (R*VR + 128 + 4000h) * 8000h, (G*UG + B*UB + 128 + 4000h) * 8000h
    paddd       mm2,mm3                                             ; (G*VG + B*VB) * 8000h, (R*UR) * 8000h

    mov         WORD PTR 0[eax],bx                                  ; write first line of y's to memory
    shr         ebx,16                                              ; 0, 0, Y1, Y0 (2nd line)

    paddd       mm7,mm2                                             ; (G*VG + B*VB + R*VR + 128 + 4000h) * 8000h, (R*UR + G*UG + B*UB + 128 + 4000h) * 8000h
    mov         WORD PTR 0[eax+esi],bx                              ; write 2nd line of y's to memory

    psrld       mm7,15                                              ; ((G*VG + B*VB + R*VR + 128 + 4000h) * 8000h) / 8000h, ((R*UR + G*UG + B*UB + 128 + 4000h) * 8000h) / 8000h

    mov         ecx,(CConvParams PTR [esp]).UBuffer                 ;

    packssdw    mm7,mm0                                             ; X, X, V, U
    add         ebp,2                                               ; increment loop ctr
    
    packuswb    mm7,mm0                                             ; X, X, X, X, X, X, V, U
    add         eax,2                                               ; increment Y output buffer ptr

    movd        ebx,mm7                                             ; X, X, V, U
    
    mov         BYTE PTR 0[ecx],bl                                  ; write u to output
    inc         ecx                                                 ; point to next U

    mov         (CConvParams PTR [esp]).UBuffer,ecx                 ; preserve pointer
    mov         ecx,(CConvParams PTR [esp]).VBuffer                 ; 
    add         edi,6                                               ; point to next block of input buffers

    mov         BYTE PTR 0[ecx],bh                                  ; write v to output buffer
    inc         ecx                                                 ; point to next V

    mov         (CConvParams PTR [esp]).VBuffer,ecx                 ; preserve pointer

    cmp         ebp,esi                                             ; are we done yet?
    jne         WLoopStart    

WLoopEnd:
    add         edi,edx                                             ; Increment input buffer ptr by image width to setp over
                                                                    ; line already processed
    add         eax,esi                                             ; Increment Y output buffer ptr by image widht to step over
                                                                    ; line already processed
    dec         (CConvParams PTR [esp]).ImageHeight                 ; decrement image height ctr
    jne         HLoopStart                                          ; are we done yet?

HLoopEnd:


theExit:

    emms

    pop     edx
    pop     ecx
    pop     ebx
    pop     ebp
    pop     edi
    pop     esi

    ret

;************************************************
        END

