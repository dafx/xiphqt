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


#include <stdio.h>
#include "dkpltfrm.h"

#include "vfw_pb_interface.h"

typedef unsigned long RGBColorComponent;
extern RGBColorComponent *WK_ClampOriginR555;
extern RGBColorComponent *WK_ClampOriginG555;
extern RGBColorComponent *WK_ClampOriginB555;

extern RGBColorComponent *WK_ClampOriginR565;
extern RGBColorComponent *WK_ClampOriginG565;
extern RGBColorComponent *WK_ClampOriginB565;

extern unsigned char WK_YforY[];		
extern unsigned char WK_VforRG[];
extern unsigned char WK_UforBG[];

#define BYTE_THREE(X) 	((X & 0xFF000000) >> (24 - 2)	) 
#define BYTE_TWO(X)  	((X & 0x00FF0000) >> (16 - 2)	)
#define BYTE_ONE(X)  	((X & 0x0000FF00) >> (8 - 2)	) 
#define BYTE_ZERO(X) 	((X & 0x000000FF) << (0 + 2)    )

extern unsigned long xditherPats[];

/* params for blitters */
/*
typedef struct {
    int     YWidth;
    int     YHeight;
    int     YStride;

    int     UVWidth;
    int     UVHeight;
    int     UVStride;

    char *  YBuffer;
    char *  UBuffer;
    char *  VBuffer;

    unsigned char     *uvStart;
    int     uvDstArea;
    int     uvUsedArea;
} YUV_BUFFER_CONFIG;
*/

void bcs00_555_c(unsigned char *_ptrScreen, int thisPitch, YUV_BUFFER_CONFIG *src)
{
    unsigned char *ptrScrn = (unsigned char *)_ptrScreen;
    
    unsigned char *YBuffer = (unsigned char *)src->YBuffer;
    unsigned char *UBuffer = (unsigned char *)src->UBuffer;
    unsigned char *VBuffer = (unsigned char *)src->VBuffer;

    unsigned long pat;
    unsigned long temp2;
    unsigned long yTemp;
    unsigned short uTemp, vTemp;

    int i, j;

    long R, G, B;
    long R1, G1, B1;
    long RP,GP,BP;
    unsigned long Y0, Y1, CbforB, CbforG, CrforR, CrforG;

    typedef unsigned long tempColor;
    tempColor  *tR = (tempColor *) WK_ClampOriginR555;
    tempColor  *tG = (tempColor *) WK_ClampOriginG555;
    tempColor  *tB = (tempColor *) WK_ClampOriginB555;

    #define HOT_FUDGE(X) (*((long *) &(X)))   	

    for(i = 0; i < src->YHeight; i += 1)
    {
        int x;

        pat = xditherPats[i & 3];
        
    
        for(j = 0, x = 0; j < src->YWidth/4; j += 1, x += 2)
        {
        
            /* get 4 y's */
        	yTemp = ((unsigned long *) YBuffer)[j*1];

            /* get 2 uv's */
            uTemp = ((unsigned short *) UBuffer)[j*1];
            vTemp = ((unsigned short *) VBuffer)[j*1];
        	
            Y0 = HOT_FUDGE(WK_YforY[BYTE_ZERO(yTemp)]);
            Y1 = HOT_FUDGE(WK_YforY[BYTE_ONE(yTemp)]);
    
            CbforB 	= HOT_FUDGE(WK_UforBG[BYTE_ZERO(uTemp)<<1]);
            CbforG 	= HOT_FUDGE(WK_UforBG[(BYTE_ZERO(uTemp)<<1)+4]);
    
            CrforR 	= HOT_FUDGE(WK_VforRG[BYTE_ZERO(vTemp)<<1]);
            CrforG 	= HOT_FUDGE(WK_VforRG[(BYTE_ZERO(vTemp)<<1)+4]);
        
    
            R = Y0 + CrforR;
            B = Y0 + CbforB;
            G = Y0 - CbforG - CrforG;
            
            temp2 = (pat & 0xFF000000) >> 24;
            
            RP = tR[R + temp2];
            GP = tG[G + temp2];
            BP = tB[B + temp2];
            
    #if defined(_bigend_h)
            /* reuse Y0 */
            Y0 = ((RP | GP | BP) <<16);
    #else
            /* reuse Y0 */
            Y0 = ((RP | GP | BP));
    #endif
    
            R1 = Y1 + CrforR;
            B1 = Y1 + CbforB;
            G1 = Y1 - CbforG - CrforG;
            
            temp2 = (pat & 0x00FF0000) >> 16;
            RP = tR[R1 + temp2];
            GP = tG[G1 + temp2];
            BP = tB[B1 + temp2];
            
    #if defined(_bigend_h)
            ((unsigned long *) ptrScrn)[x] = (unsigned long)(Y0 | (RP | GP | BP));
    #else
            ((unsigned long *) ptrScrn)[x] = (unsigned long)(Y0 | (RP | GP | BP)<<16);
    #endif
    
        	Y0 = HOT_FUDGE(WK_YforY[BYTE_TWO(yTemp)]);
        	Y1 = HOT_FUDGE(WK_YforY[BYTE_THREE(yTemp)]);

       		CbforB 	= HOT_FUDGE(WK_UforBG[BYTE_ONE(uTemp)<<1]);
        	CbforG 	= HOT_FUDGE(WK_UforBG[(BYTE_ONE(uTemp)<<1)+4]);

        	CrforR 	= HOT_FUDGE(WK_VforRG[BYTE_ONE(vTemp)<<1]);
        	CrforG 	= HOT_FUDGE(WK_VforRG[(BYTE_ONE(vTemp)<<1)+4]);

        	R = Y0 + CrforR;
        	B = Y0 + CbforB;
        	G = Y0 - CbforG - CrforG;
        	
        	R1 = Y1 + CrforR;
        	B1 = Y1 + CbforB;
        	G1 = Y1 - CbforG - CrforG;
            
            
            temp2 = (pat & 0x0000FF00) >> 8;
            RP = tR[R + temp2];
            GP = tG[G + temp2];
            BP = tB[B + temp2];
            
    #if defined(_bigend_h)
            /* reuse Y0 */
            Y0 = ((RP | GP | BP) <<16);
    #else
            /* reuse Y0 */
            Y0 = ((RP | GP | BP));
    #endif
            
            temp2 = (pat & 0x000000FF);
            RP = tR[R1 + temp2];
            GP = tG[G1 + temp2];
            BP = tB[B1 + temp2];
           
     
    #if defined(_bigend_h)
            ((unsigned long *) ptrScrn)[x+1] = (unsigned long)(Y0 | (RP | GP | BP));
    #else
            ((unsigned long *) ptrScrn)[x+1] = (unsigned long)(Y0 | (RP | GP | BP)<<16);
    #endif
    
        } /* inner for */

        ptrScrn += thisPitch;
        YBuffer -= src->YStride;

        /* see mmx asm code on how to remove this branch */
        if(i & 1)
        {
            UBuffer -= src->UVStride;
            VBuffer -= src->UVStride;
        }
    } /* outer for */
}

/*---------------------------------------------*/
void bcs00_565_c(unsigned char *_ptrScreen, int thisPitch, YUV_BUFFER_CONFIG *src)
{
    unsigned char *ptrScrn = (unsigned char *)_ptrScreen;
    
    unsigned char *YBuffer = (unsigned char *)src->YBuffer;
    unsigned char *UBuffer = (unsigned char *)src->UBuffer;
    unsigned char *VBuffer = (unsigned char *)src->VBuffer;

    unsigned long pat;
    unsigned long temp2;
    unsigned long yTemp;
    unsigned short uTemp, vTemp;

    int i, j;

    long R, G, B;
    long R1, G1, B1;
    long RP,GP,BP;
    unsigned long Y0, Y1, CbforB, CbforG, CrforR, CrforG;

    typedef unsigned long tempColor;
    tempColor  *tR = (tempColor *) WK_ClampOriginR565;
    tempColor  *tG = (tempColor *) WK_ClampOriginG565;
    tempColor  *tB = (tempColor *) WK_ClampOriginB565;

    #define HOT_FUDGE(X) (*((long *) &(X)))   	

    for(i = 0; i < src->YHeight; i += 1)
    {
        int x;

        pat = xditherPats[i & 3];
        
    
        for(j = 0, x = 0; j < src->YWidth/4; j += 1, x += 2)
        {
        
            /* get 4 y's */
        	yTemp = ((unsigned long *) YBuffer)[j*1];

            /* get 2 uv's */
            uTemp = ((unsigned short *) UBuffer)[j*1];
            vTemp = ((unsigned short *) VBuffer)[j*1];
        	
            Y0 = HOT_FUDGE(WK_YforY[BYTE_ZERO(yTemp)]);
            Y1 = HOT_FUDGE(WK_YforY[BYTE_ONE(yTemp)]);
    
            CbforB 	= HOT_FUDGE(WK_UforBG[BYTE_ZERO(uTemp)<<1]);
            CbforG 	= HOT_FUDGE(WK_UforBG[(BYTE_ZERO(uTemp)<<1)+4]);
    
            CrforR 	= HOT_FUDGE(WK_VforRG[BYTE_ZERO(vTemp)<<1]);
            CrforG 	= HOT_FUDGE(WK_VforRG[(BYTE_ZERO(vTemp)<<1)+4]);
        
    
            R = Y0 + CrforR;
            B = Y0 + CbforB;
            G = Y0 - CbforG - CrforG;
            
            temp2 = (pat & 0xFF000000) >> 24;
            
            RP = tR[R + temp2];
            GP = tG[G + temp2];
            BP = tB[B + temp2];
            
    #if defined(_bigend_h)
            /* reuse Y0 */
            Y0 = ((RP | GP | BP) <<16);
    #else
            /* reuse Y0 */
            Y0 = ((RP | GP | BP));
    #endif
    
            R1 = Y1 + CrforR;
            B1 = Y1 + CbforB;
            G1 = Y1 - CbforG - CrforG;
            
            temp2 = (pat & 0x00FF0000) >> 16;
            RP = tR[R1 + temp2];
            GP = tG[G1 + temp2];
            BP = tB[B1 + temp2];
            
    #if defined(_bigend_h)
            ((unsigned long *) ptrScrn)[x] = (unsigned long)(Y0 | (RP | GP | BP));
    #else
            ((unsigned long *) ptrScrn)[x] = (unsigned long)(Y0 | (RP | GP | BP)<<16);
    #endif
    
        	Y0 = HOT_FUDGE(WK_YforY[BYTE_TWO(yTemp)]);
        	Y1 = HOT_FUDGE(WK_YforY[BYTE_THREE(yTemp)]);

       		CbforB 	= HOT_FUDGE(WK_UforBG[BYTE_ONE(uTemp)<<1]);
        	CbforG 	= HOT_FUDGE(WK_UforBG[(BYTE_ONE(uTemp)<<1)+4]);

        	CrforR 	= HOT_FUDGE(WK_VforRG[BYTE_ONE(vTemp)<<1]);
        	CrforG 	= HOT_FUDGE(WK_VforRG[(BYTE_ONE(vTemp)<<1)+4]);

        	R = Y0 + CrforR;
        	B = Y0 + CbforB;
        	G = Y0 - CbforG - CrforG;
        	
        	R1 = Y1 + CrforR;
        	B1 = Y1 + CbforB;
        	G1 = Y1 - CbforG - CrforG;
            
            
            temp2 = (pat & 0x0000FF00) >> 8;
            RP = tR[R + temp2];
            GP = tG[G + temp2];
            BP = tB[B + temp2];
            
    #if defined(_bigend_h)
            /* reuse Y0 */
            Y0 = ((RP | GP | BP) <<16);
    #else
            /* reuse Y0 */
            Y0 = ((RP | GP | BP));
    #endif
            
            temp2 = (pat & 0x000000FF);
            RP = tR[R1 + temp2];
            GP = tG[G1 + temp2];
            BP = tB[B1 + temp2];
           
     
    #if defined(_bigend_h)
            ((unsigned long *) ptrScrn)[x+1] = (unsigned long)(Y0 | (RP | GP | BP));
    #else
            ((unsigned long *) ptrScrn)[x+1] = (unsigned long)(Y0 | (RP | GP | BP)<<16);
    #endif
    
        } /* inner for */

        ptrScrn += thisPitch;
        YBuffer -= src->YStride;

        /* see mmx asm code on how to remove this branch */
        if(i & 1)
        {
            UBuffer -= src->UVStride;
            VBuffer -= src->UVStride;
        }
    } /* outer for */
}
