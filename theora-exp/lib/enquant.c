#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>
#include "enquant.h"
#include "internal.h"


/*The default quantization parameters used by VP3.1.*/
static const int OC_VP31_RANGE_SIZES[1]={63};
static const th_quant_base OC_VP31_BASES_INTRA_Y[2]={
  {
     16, 11, 10, 16, 24, 40, 51, 61,
     12, 12, 14, 19, 26, 58, 60, 55,
     14, 13, 16, 24, 40, 57, 69, 56,
     14, 17, 22, 29, 51, 87, 80, 62,
     18, 22, 37, 58, 68,109,103, 77,
     24, 35, 55, 64, 81,104,113, 92,
     49, 64, 78, 87,103,121,120,101,
     72, 92, 95, 98,112,100,103, 99
  },
  {
     16, 11, 10, 16, 24, 40, 51, 61,
     12, 12, 14, 19, 26, 58, 60, 55,
     14, 13, 16, 24, 40, 57, 69, 56,
     14, 17, 22, 29, 51, 87, 80, 62,
     18, 22, 37, 58, 68,109,103, 77,
     24, 35, 55, 64, 81,104,113, 92,
     49, 64, 78, 87,103,121,120,101,
     72, 92, 95, 98,112,100,103, 99
  }
};
static const th_quant_base OC_VP31_BASES_INTRA_C[2]={
  {
     17, 18, 24, 47, 99, 99, 99, 99,
     18, 21, 26, 66, 99, 99, 99, 99,
     24, 26, 56, 99, 99, 99, 99, 99,
     47, 66, 99, 99, 99, 99, 99, 99,
     99, 99, 99, 99, 99, 99, 99, 99,
     99, 99, 99, 99, 99, 99, 99, 99,
     99, 99, 99, 99, 99, 99, 99, 99,
     99, 99, 99, 99, 99, 99, 99, 99
  },
  {
     17, 18, 24, 47, 99, 99, 99, 99,
     18, 21, 26, 66, 99, 99, 99, 99,
     24, 26, 56, 99, 99, 99, 99, 99,
     47, 66, 99, 99, 99, 99, 99, 99,
     99, 99, 99, 99, 99, 99, 99, 99,
     99, 99, 99, 99, 99, 99, 99, 99,
     99, 99, 99, 99, 99, 99, 99, 99,
     99, 99, 99, 99, 99, 99, 99, 99
  }
};
static const th_quant_base OC_VP31_BASES_INTER[2]={
  {
     16, 16, 16, 20, 24, 28, 32, 40,
     16, 16, 20, 24, 28, 32, 40, 48,
     16, 20, 24, 28, 32, 40, 48, 64,
     20, 24, 28, 32, 40, 48, 64, 64,
     24, 28, 32, 40, 48, 64, 64, 64,
     28, 32, 40, 48, 64, 64, 64, 96,
     32, 40, 48, 64, 64, 64, 96,128,
     40, 48, 64, 64, 64, 96,128,128
  },
  {
     16, 16, 16, 20, 24, 28, 32, 40,
     16, 16, 20, 24, 28, 32, 40, 48,
     16, 20, 24, 28, 32, 40, 48, 64,
     20, 24, 28, 32, 40, 48, 64, 64,
     24, 28, 32, 40, 48, 64, 64, 64,
     28, 32, 40, 48, 64, 64, 64, 96,
     32, 40, 48, 64, 64, 64, 96,128,
     40, 48, 64, 64, 64, 96,128,128
  }
};

const th_quant_info TH_VP31_QUANT_INFO={
  {
    220,200,190,180,170,170,160,160,
    150,150,140,140,130,130,120,120,
    110,110,100,100, 90, 90, 90, 80,
     80, 80, 70, 70, 70, 60, 60, 60,
     60, 50, 50, 50, 50, 40, 40, 40,
     40, 40, 30, 30, 30, 30, 30, 30,
     30, 20, 20, 20, 20, 20, 20, 20,
     20, 10, 10, 10, 10, 10, 10, 10
  },
  {
    500,450,400,370,340,310,285,265,
    245,225,210,195,185,180,170,160,
    150,145,135,130,125,115,110,107,
    100, 96, 93, 89, 85, 82, 75, 74,
     70, 68, 64, 60, 57, 56, 52, 50,
     49, 45, 44, 43, 40, 38, 37, 35,
     33, 32, 30, 29, 28, 25, 24, 22,
     21, 19, 18, 17, 15, 13, 12, 10
  },
  {
    30,25,20,20,15,15,14,14,
    13,13,12,12,11,11,10,10,
     9, 9, 8, 8, 7, 7, 7, 7,
     6, 6, 6, 6, 5, 5, 5, 5,
     4, 4, 4, 4, 3, 3, 3, 3,
     2, 2, 2, 2, 2, 2, 2, 2,
     0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0
  },
  {
    {
      {1,OC_VP31_RANGE_SIZES,OC_VP31_BASES_INTRA_Y},
      {1,OC_VP31_RANGE_SIZES,OC_VP31_BASES_INTRA_C},
      {1,OC_VP31_RANGE_SIZES,OC_VP31_BASES_INTRA_C}
    },
    {
      {1,OC_VP31_RANGE_SIZES,OC_VP31_BASES_INTER},
      {1,OC_VP31_RANGE_SIZES,OC_VP31_BASES_INTER},
      {1,OC_VP31_RANGE_SIZES,OC_VP31_BASES_INTER}
    }
  }
};



static const int OC_DEF_QRANGE_SIZES[3]={15,16,32};

static const th_quant_base OC_DEF_BASE_MATRICES[9][4]={
  /*Y' matrices, 4:4:4 subsampling.*/
  {
    /*qi=0.*/
    {
      255, 54, 54, 54, 55, 57, 63, 71,
       54, 54, 54, 54, 55, 58, 63, 71,
       54, 54, 54, 54, 55, 58, 63, 72,
       54, 54, 54, 55, 55, 58, 63, 72,
       55, 55, 55, 55, 56, 59, 64, 73,
       57, 58, 58, 58, 59, 62, 67, 76,
       63, 63, 63, 63, 64, 67, 73, 83,
       71, 71, 72, 72, 73, 76, 83, 94
    },
    /*qi=15.*/
    {
      255, 48, 48, 48, 49, 51, 56, 64,
       48, 48, 48, 48, 49, 51, 56, 65,
       48, 48, 48, 48, 49, 51, 57, 65,
       48, 48, 48, 48, 49, 52, 57, 65,
       49, 49, 49, 49, 50, 52, 58, 66,
       51, 51, 51, 52, 52, 55, 61, 69,
       56, 56, 57, 57, 58, 61, 67, 76,
       64, 65, 65, 65, 66, 69, 76, 87
    },
    /*qi=31.*/
    {
      255, 46, 46, 46, 47, 49, 54, 62,
       46, 46, 46, 46, 47, 50, 55, 63,
       46, 46, 46, 46, 47, 50, 55, 63,
       46, 46, 46, 47, 47, 50, 55, 63,
       47, 47, 47, 47, 48, 51, 56, 64,
       49, 50, 50, 50, 51, 53, 59, 67,
       54, 55, 55, 55, 56, 59, 65, 74,
       62, 63, 63, 63, 64, 67, 74, 86
    },
    /*qi=63.*/
    {
      255, 45, 45, 46, 46, 49, 54, 62,
       45, 46, 46, 46, 46, 49, 54, 62,
       45, 46, 46, 46, 47, 49, 54, 62,
       46, 46, 46, 46, 47, 49, 54, 62,
       46, 46, 47, 47, 47, 50, 55, 63,
       49, 49, 49, 49, 50, 53, 58, 67,
       54, 54, 54, 54, 55, 58, 64, 74,
       62, 62, 62, 62, 63, 67, 74, 85
    }
  },
  /*Cb matrices, 4:4:4 subsampling.*/
  {
    /*qi=0.*/
    {
      149, 34, 37, 41, 45, 49, 54, 60,
       34, 38, 41, 46, 50, 55, 61, 67,
       37, 41, 45, 49, 54, 60, 66, 73,
       41, 46, 49, 54, 60, 66, 72, 80,
       45, 50, 54, 60, 65, 72, 79, 88,
       49, 55, 60, 66, 72, 79, 87, 96,
       54, 61, 66, 72, 79, 87, 96,106,
       60, 67, 73, 80, 88, 96,106,117
    },
    /*qi=15.*/
    {
      149, 31, 34, 37, 41, 46, 51, 56,
       31, 35, 38, 42, 47, 52, 57, 63,
       34, 38, 42, 46, 51, 56, 62, 69,
       37, 42, 46, 51, 56, 62, 69, 76,
       41, 47, 51, 56, 62, 69, 76, 84,
       46, 52, 56, 62, 69, 76, 84, 93,
       51, 57, 62, 69, 76, 84, 93,103,
       56, 63, 69, 76, 84, 93,103,115
    },
    /*qi=31.*/
    {
      149, 30, 33, 36, 40, 45, 49, 55,
       30, 34, 37, 41, 46, 51, 56, 62,
       33, 37, 41, 45, 50, 55, 61, 68,
       36, 41, 45, 50, 55, 61, 68, 75,
       40, 46, 50, 55, 61, 68, 75, 83,
       45, 51, 55, 61, 68, 75, 83, 93,
       49, 56, 61, 68, 75, 83, 92,102,
       55, 62, 68, 75, 83, 93,102,114
    },
    /*qi=63.*/
    {
      149, 30, 33, 36, 40, 44, 49, 55,
       30, 34, 37, 41, 45, 50, 56, 62,
       33, 37, 40, 45, 50, 55, 61, 68,
       36, 41, 45, 50, 55, 61, 67, 75,
       40, 45, 50, 55, 61, 68, 75, 83,
       44, 50, 55, 61, 68, 75, 83, 92,
       49, 56, 61, 67, 75, 83, 92,102,
       55, 62, 68, 75, 83, 92,102,114
    }
  },
  /*Cb matrices, 4:2:2 subsampling.*/
  {
    /*qi=0.*/
    {
      142, 31, 32, 34, 36, 38, 40, 42,
       33, 35, 36, 38, 40, 42, 44, 47,
       35, 38, 39, 41, 43, 46, 48, 51,
       39, 41, 43, 45, 48, 50, 53, 55,
       43, 45, 47, 50, 52, 55, 58, 61,
       47, 50, 52, 55, 58, 61, 64, 67,
       52, 55, 57, 60, 63, 67, 70, 74,
       57, 61, 63, 67, 70, 74, 77, 81
    },
    /*qi=15.*/
    {
      142, 28, 29, 31, 33, 34, 36, 38,
       30, 32, 33, 35, 37, 39, 41, 43,
       32, 34, 36, 38, 40, 42, 45, 47,
       36, 38, 40, 42, 44, 47, 49, 52,
       39, 42, 44, 46, 49, 52, 54, 58,
       44, 46, 49, 51, 54, 57, 60, 64,
       48, 51, 54, 57, 60, 63, 67, 70,
       53, 57, 60, 63, 66, 70, 74, 78
    },
    /*qi=31.*/
    {
      142, 27, 28, 30, 32, 33, 35, 37,
       29, 31, 32, 34, 36, 38, 40, 42,
       31, 33, 35, 37, 39, 41, 44, 46,
       35, 37, 39, 41, 43, 46, 48, 51,
       38, 41, 43, 45, 48, 51, 54, 57,
       43, 45, 48, 50, 53, 56, 59, 63,
       47, 50, 53, 56, 59, 62, 66, 70,
       52, 56, 59, 62, 65, 69, 73, 77
    },
    /*qi=63.*/
    {
      142, 27, 28, 30, 31, 33, 35, 37,
       28, 30, 32, 34, 35, 38, 40, 42,
       31, 33, 35, 37, 39, 41, 43, 46,
       34, 37, 39, 41, 43, 45, 48, 51,
       38, 41, 43, 45, 48, 50, 53, 56,
       42, 45, 47, 50, 53, 56, 59, 63,
       47, 50, 52, 55, 59, 62, 65, 69,
       52, 56, 58, 62, 65, 69, 73, 77
    }
  },
  /*Cb matrices, subsampling in the Y direction.*/
  {
    /*qi=0.*/
    {
      142, 33, 35, 39, 43, 47, 52, 57,
       31, 35, 38, 41, 45, 50, 55, 61,
       32, 36, 39, 43, 47, 52, 57, 63,
       34, 38, 41, 45, 50, 55, 60, 67,
       36, 40, 43, 48, 52, 58, 63, 70,
       38, 42, 46, 50, 55, 61, 67, 74,
       40, 44, 48, 53, 58, 64, 70, 77,
       42, 47, 51, 55, 61, 67, 74, 81
    },
    /*qi=15.*/
    {
      142, 30, 32, 36, 39, 44, 48, 53,
       28, 32, 34, 38, 42, 46, 51, 57,
       29, 33, 36, 40, 44, 49, 54, 60,
       31, 35, 38, 42, 46, 51, 57, 63,
       33, 37, 40, 44, 49, 54, 60, 66,
       34, 39, 42, 47, 52, 57, 63, 70,
       36, 41, 45, 49, 54, 60, 67, 74,
       38, 43, 47, 52, 58, 64, 70, 78
    },
    /*qi=31.*/
    {
      142, 29, 31, 35, 38, 43, 47, 52,
       27, 31, 33, 37, 41, 45, 50, 56,
       28, 32, 35, 39, 43, 48, 53, 59,
       30, 34, 37, 41, 45, 50, 56, 62,
       32, 36, 39, 43, 48, 53, 59, 65,
       33, 38, 41, 46, 51, 56, 62, 69,
       35, 40, 44, 48, 54, 59, 66, 73,
       37, 42, 46, 51, 57, 63, 70, 77
    },
    /*qi=63.*/
    {
      142, 28, 31, 34, 38, 42, 47, 52,
       27, 30, 33, 37, 41, 45, 50, 56,
       28, 32, 35, 39, 43, 47, 52, 58,
       30, 34, 37, 41, 45, 50, 55, 62,
       31, 35, 39, 43, 48, 53, 59, 65,
       33, 38, 41, 45, 50, 56, 62, 69,
       35, 40, 43, 48, 53, 59, 65, 73,
       37, 42, 46, 51, 56, 63, 69, 77
    }
  },
  /*Cb matrices, 4:2:0 subsampling.*/
  {
    /*qi=0.*/
    {
      135, 30, 31, 33, 34, 36, 38, 40,
       30, 32, 33, 35, 36, 38, 40, 42,
       31, 33, 34, 36, 38, 40, 42, 44,
       33, 35, 36, 38, 40, 42, 44, 46,
       34, 36, 38, 40, 42, 44, 46, 49,
       36, 38, 40, 42, 44, 46, 49, 51,
       38, 40, 42, 44, 46, 49, 51, 54,
       40, 42, 44, 46, 49, 51, 54, 57
    },
    /*qi=15.*/
    {
      135, 27, 28, 29, 31, 33, 35, 36,
       27, 28, 30, 31, 33, 35, 37, 39,
       28, 30, 31, 33, 35, 37, 39, 41,
       29, 31, 33, 35, 37, 39, 41, 43,
       31, 33, 35, 37, 39, 41, 43, 45,
       33, 35, 37, 39, 41, 43, 45, 48,
       35, 37, 39, 41, 43, 45, 48, 51,
       36, 39, 41, 43, 45, 48, 51, 53
    },
    /*qi=31.*/
    {
      135, 26, 27, 28, 30, 32, 34, 36,
       26, 28, 29, 30, 32, 34, 36, 38,
       27, 29, 30, 32, 34, 36, 38, 40,
       28, 30, 32, 34, 36, 38, 40, 42,
       30, 32, 34, 36, 38, 40, 42, 44,
       32, 34, 36, 38, 40, 42, 44, 47,
       34, 36, 38, 40, 42, 44, 47, 50,
       36, 38, 40, 42, 44, 47, 50, 52
    },
    /*qi=63.*/
    {
      135, 25, 27, 28, 30, 31, 33, 35,
       25, 27, 29, 30, 32, 34, 36, 38,
       27, 29, 30, 32, 33, 35, 37, 40,
       28, 30, 32, 33, 35, 37, 39, 42,
       30, 32, 33, 35, 37, 39, 42, 44,
       31, 34, 35, 37, 39, 42, 44, 47,
       33, 36, 37, 39, 42, 44, 47, 49,
       35, 38, 40, 42, 44, 47, 49, 52
    }
  },
  /*Cr matrices, 4:4:4 subsampling.*/
  {
    /*qi=0.*/
    {
      195, 46, 52, 59, 67, 77, 88,101,
       46, 54, 60, 69, 78, 89,102,117,
       52, 60, 68, 77, 87,100,114,131,
       59, 69, 77, 87, 99,114,130,149,
       67, 78, 87, 99,113,130,148,170,
       77, 89,100,114,130,148,169,194,
       88,102,114,130,148,169,193,222,
      101,117,131,149,170,194,222,255
    },
    /*qi=15.*/
    {
      195, 42, 48, 55, 63, 72, 83, 96,
       42, 50, 56, 64, 74, 85, 98,113,
       48, 56, 63, 73, 83, 96,110,127,
       55, 64, 73, 83, 95,110,126,146,
       63, 74, 83, 95,110,126,145,167,
       72, 85, 96,110,126,145,166,192,
       83, 98,110,126,145,166,191,221,
       96,113,127,146,167,192,221,255
    },
    /*qi=31.*/
    {
      195, 41, 47, 53, 61, 71, 81, 94,
       41, 49, 55, 63, 73, 84, 96,111,
       47, 55, 62, 71, 82, 95,109,126,
       53, 63, 71, 82, 94,109,125,145,
       61, 73, 82, 94,108,125,144,166,
       71, 84, 95,109,125,144,166,192,
       81, 96,109,125,144,166,191,220,
       94,111,126,145,166,192,220,255
    },
    /*qi=63.*/
    {
      195, 41, 46, 53, 61, 70, 81, 94,
       41, 48, 55, 63, 72, 83, 96,111,
       46, 55, 62, 71, 82, 94,108,125,
       53, 63, 71, 82, 94,108,125,144,
       61, 72, 82, 94,108,125,144,166,
       70, 83, 94,108,125,144,165,191,
       81, 96,108,125,144,165,190,220,
       94,111,125,144,166,191,220,255
    }
  },
  /*Cr matrices, 4:2:2 subsampling.*/
  {
    /*qi=0.*/
    {
      183, 41, 43, 46, 49, 53, 57, 61,
       44, 47, 50, 54, 58, 62, 66, 71,
       49, 53, 56, 60, 64, 69, 74, 79,
       56, 60, 64, 68, 73, 78, 84, 90,
       63, 69, 73, 78, 83, 89, 96,103,
       72, 79, 83, 89, 95,102,109,118,
       83, 90, 95,102,109,117,125,134,
       95,103,109,117,125,134,144,154
    },
    /*qi=15.*/
    {
      183, 37, 39, 42, 45, 49, 53, 57,
       40, 43, 46, 50, 53, 58, 62, 67,
       45, 49, 52, 56, 60, 65, 70, 75,
       51, 56, 60, 64, 69, 74, 80, 86,
       59, 64, 69, 74, 79, 85, 92, 99,
       68, 74, 79, 85, 91, 98,106,114,
       78, 85, 91, 97,105,113,121,131,
       90, 98,105,112,121,130,140,151
    },
    /*qi=31.*/
    {
      183, 36, 38, 41, 44, 48, 51, 55,
       39, 42, 45, 48, 52, 56, 61, 66,
       44, 48, 51, 55, 59, 64, 69, 74,
       50, 55, 59, 63, 68, 73, 79, 85,
       58, 63, 67, 72, 78, 84, 91, 98,
       66, 73, 78, 83, 90, 97,104,113,
       76, 84, 89, 96,103,112,120,130,
       88, 97,103,111,120,129,139,150
    },
    /*qi=63.*/
    {
      183, 35, 38, 41, 44, 47, 51, 55,
       38, 42, 45, 48, 52, 56, 60, 65,
       43, 47, 51, 54, 59, 63, 68, 74,
       50, 54, 58, 63, 67, 73, 78, 85,
       57, 63, 67, 72, 78, 84, 90, 98,
       66, 72, 77, 83, 90, 97,104,112,
       76, 83, 89, 96,103,111,120,129,
       88, 96,103,111,119,129,139,150
    }
  },
  /*Cr matrices, subsampling in the Y direction.*/
  {
    /*qi=0.*/
    {
      183, 44, 49, 56, 63, 72, 83, 95,
       41, 47, 53, 60, 69, 79, 90,103,
       43, 50, 56, 64, 73, 83, 95,109,
       46, 54, 60, 68, 78, 89,102,117,
       49, 58, 64, 73, 83, 95,109,125,
       53, 62, 69, 78, 89,102,117,134,
       57, 66, 74, 84, 96,109,125,144,
       61, 71, 79, 90,103,118,134,154
    },
    /*qi=15.*/
    {
      183, 40, 45, 51, 59, 68, 78, 90,
       37, 43, 49, 56, 64, 74, 85, 98,
       39, 46, 52, 60, 69, 79, 91,105,
       42, 50, 56, 64, 74, 85, 97,112,
       45, 53, 60, 69, 79, 91,105,121,
       49, 58, 65, 74, 85, 98,113,130,
       53, 62, 70, 80, 92,106,121,140,
       57, 67, 75, 86, 99,114,131,151
    },
    /*qi=31.*/
    {
      183, 39, 44, 50, 58, 66, 76, 88,
       36, 42, 48, 55, 63, 73, 84, 97,
       38, 45, 51, 59, 67, 78, 89,103,
       41, 48, 55, 63, 72, 83, 96,111,
       44, 52, 59, 68, 78, 90,103,120,
       48, 56, 64, 73, 84, 97,112,129,
       51, 61, 69, 79, 91,104,120,139,
       55, 66, 74, 85, 98,113,130,150
    },
    /*qi=63.*/
    {
      183, 38, 43, 50, 57, 66, 76, 88,
       35, 42, 47, 54, 63, 72, 83, 96,
       38, 45, 51, 58, 67, 77, 89,103,
       41, 48, 54, 63, 72, 83, 96,111,
       44, 52, 59, 67, 78, 90,103,119,
       47, 56, 63, 73, 84, 97,111,129,
       51, 60, 68, 78, 90,104,120,139,
       55, 65, 74, 85, 98,112,129,150
    }
  },
  /*Cr matrices, 4:2:0 subsampling.*/
  {
    /*qi=0.*/
    {
      172, 38, 41, 44, 47, 50, 54, 58,
       38, 42, 44, 47, 51, 54, 58, 62,
       41, 44, 47, 50, 54, 57, 62, 66,
       44, 47, 50, 54, 57, 61, 66, 71,
       47, 51, 54, 57, 61, 66, 70, 76,
       50, 54, 57, 61, 66, 70, 75, 81,
       54, 58, 62, 66, 70, 75, 81, 87,
       58, 62, 66, 71, 76, 81, 87, 93
    },
    /*qi=15.*/
    {
      172, 35, 37, 40, 43, 46, 49, 53,
       35, 38, 40, 43, 47, 50, 54, 58,
       37, 40, 43, 46, 50, 53, 57, 62,
       40, 43, 46, 50, 53, 57, 62, 67,
       43, 47, 50, 53, 57, 62, 66, 72,
       46, 50, 53, 57, 62, 66, 71, 77,
       49, 54, 57, 62, 66, 71, 77, 83,
       53, 58, 62, 67, 72, 77, 83, 89
    },
    /*qi=31.*/
    {
      172, 34, 36, 38, 41, 45, 48, 52,
       34, 37, 39, 42, 45, 49, 53, 57,
       36, 39, 42, 45, 48, 52, 56, 61,
       38, 42, 45, 48, 52, 56, 61, 65,
       41, 45, 48, 52, 56, 61, 65, 70,
       45, 49, 52, 56, 61, 65, 70, 76,
       48, 53, 56, 61, 65, 70, 76, 82,
       52, 57, 61, 65, 70, 76, 82, 88
    },
    /*qi=63.*/
    {
      172, 33, 35, 38, 41, 44, 48, 52,
       33, 36, 39, 42, 45, 49, 52, 57,
       35, 39, 41, 45, 48, 52, 56, 60,
       38, 42, 45, 48, 52, 56, 60, 65,
       41, 45, 48, 52, 56, 60, 65, 70,
       44, 49, 52, 56, 60, 65, 70, 76,
       48, 52, 56, 60, 65, 70, 75, 81,
       52, 57, 60, 65, 70, 76, 81, 88
    }
  }
};

const th_quant_info OC_DEF_QUANT_INFO[4]={
  {
    {
        15,  14,  13,  13,  12,  12,  11,  11,
        10,  10,   9,   9,   8,   8,   8,   7,
         7,   7,   6,   6,   6,   6,   5,   5,
         5,   5,   4,   4,   4,   4,   4,   3,
         3,   3,   3,   3,   3,   3,   2,   2,
         2,   2,   2,   2,   2,   2,   2,   1,
         1,   1,   1,   1,   1,   1,   1,   1,
         1,   1,   1,   1,   1,   1,   1,   1
    },
    {
        71,  68,  65,  64,  62,  59,  57,  55,
        53,  51,  50,  48,  46,  44,  43,  41,
        40,  38,  36,  35,  33,  32,  31,  29,
        29,  27,  26,  25,  24,  23,  22,  21,
        20,  19,  19,  18,  17,  16,  16,  15,
        14,  14,  13,  12,  12,  11,  11,  10,
        10,  10,   9,   9,   8,   8,   8,   7,
         7,   7,   6,   6,   6,   6,   5,   1
    },
    {
        77,  74,  71,  68,  65,  62,  60,  57,
        55,  52,  50,  48,  46,  44,  42,  40,
        39,  37,  35,  34,  33,  31,  30,  29,
        27,  26,  25,  24,  23,  22,  21,  20,
        19,  19,  18,  17,  16,  16,  15,  14,
        14,  13,  13,  12,  12,  11,  11,  10,
        10,   9,   9,   8,   8,   8,   7,   7,
         7,   7,   6,   6,   6,   6,   5,   2
    },
    {
      {
        {3,OC_DEF_QRANGE_SIZES,OC_DEF_BASE_MATRICES[0]},
        {3,OC_DEF_QRANGE_SIZES,OC_DEF_BASE_MATRICES[4]},
        {3,OC_DEF_QRANGE_SIZES,OC_DEF_BASE_MATRICES[8]}
      },
      {
        {3,OC_DEF_QRANGE_SIZES,OC_DEF_BASE_MATRICES[0]},
        {3,OC_DEF_QRANGE_SIZES,OC_DEF_BASE_MATRICES[4]},
        {3,OC_DEF_QRANGE_SIZES,OC_DEF_BASE_MATRICES[8]}
      }
    }
  },
  {
    {
        15,  14,  13,  13,  12,  12,  11,  11,
        10,  10,   9,   9,   8,   8,   8,   7,
         7,   7,   6,   6,   6,   6,   5,   5,
         5,   5,   4,   4,   4,   4,   4,   3,
         3,   3,   3,   3,   3,   3,   2,   2,
         2,   2,   2,   2,   2,   2,   2,   1,
         1,   1,   1,   1,   1,   1,   1,   1,
         1,   1,   1,   1,   1,   1,   1,   1
    },
    {
        71,  68,  65,  64,  62,  59,  57,  55,
        53,  51,  50,  48,  46,  44,  43,  41,
        40,  38,  36,  35,  33,  32,  31,  29,
        29,  27,  26,  25,  24,  23,  22,  21,
        20,  19,  19,  18,  17,  16,  16,  15,
        14,  14,  13,  12,  12,  11,  11,  10,
        10,  10,   9,   9,   8,   8,   8,   7,
         7,   7,   6,   6,   6,   6,   5,   1
    },
    {
        77,  74,  71,  68,  65,  62,  60,  57,
        55,  52,  50,  48,  46,  44,  42,  40,
        39,  37,  35,  34,  33,  31,  30,  29,
        27,  26,  25,  24,  23,  22,  21,  20,
        19,  19,  18,  17,  16,  16,  15,  14,
        14,  13,  13,  12,  12,  11,  11,  10,
        10,   9,   9,   8,   8,   8,   7,   7,
         7,   7,   6,   6,   6,   6,   5,   2
    },
    {
      {
        {3,OC_DEF_QRANGE_SIZES,OC_DEF_BASE_MATRICES[0]},
        {3,OC_DEF_QRANGE_SIZES,OC_DEF_BASE_MATRICES[3]},
        {3,OC_DEF_QRANGE_SIZES,OC_DEF_BASE_MATRICES[7]}
      },
      {
        {3,OC_DEF_QRANGE_SIZES,OC_DEF_BASE_MATRICES[0]},
        {3,OC_DEF_QRANGE_SIZES,OC_DEF_BASE_MATRICES[3]},
        {3,OC_DEF_QRANGE_SIZES,OC_DEF_BASE_MATRICES[7]}
      }
    }
  },
  {
    {
        15,  14,  13,  13,  12,  12,  11,  11,
        10,  10,   9,   9,   8,   8,   8,   7,
         7,   7,   6,   6,   6,   6,   5,   5,
         5,   5,   4,   4,   4,   4,   4,   3,
         3,   3,   3,   3,   3,   3,   2,   2,
         2,   2,   2,   2,   2,   2,   2,   1,
         1,   1,   1,   1,   1,   1,   1,   1,
         1,   1,   1,   1,   1,   1,   1,   1
    },
    {
        71,  68,  65,  64,  62,  59,  57,  55,
        53,  51,  50,  48,  46,  44,  43,  41,
        40,  38,  36,  35,  33,  32,  31,  29,
        29,  27,  26,  25,  24,  23,  22,  21,
        20,  19,  19,  18,  17,  16,  16,  15,
        14,  14,  13,  12,  12,  11,  11,  10,
        10,  10,   9,   9,   8,   8,   8,   7,
         7,   7,   6,   6,   6,   6,   5,   1
    },
    {
        77,  74,  71,  68,  65,  62,  60,  57,
        55,  52,  50,  48,  46,  44,  42,  40,
        39,  37,  35,  34,  33,  31,  30,  29,
        27,  26,  25,  24,  23,  22,  21,  20,
        19,  19,  18,  17,  16,  16,  15,  14,
        14,  13,  13,  12,  12,  11,  11,  10,
        10,   9,   9,   8,   8,   8,   7,   7,
         7,   7,   6,   6,   6,   6,   5,   2
    },
    {
      {
        {3,OC_DEF_QRANGE_SIZES,OC_DEF_BASE_MATRICES[0]},
        {3,OC_DEF_QRANGE_SIZES,OC_DEF_BASE_MATRICES[2]},
        {3,OC_DEF_QRANGE_SIZES,OC_DEF_BASE_MATRICES[6]}
      },
      {
        {3,OC_DEF_QRANGE_SIZES,OC_DEF_BASE_MATRICES[0]},
        {3,OC_DEF_QRANGE_SIZES,OC_DEF_BASE_MATRICES[2]},
        {3,OC_DEF_QRANGE_SIZES,OC_DEF_BASE_MATRICES[6]}
      }
    }
  },
  {
    {
        15,  14,  13,  13,  12,  12,  11,  11,
        10,  10,   9,   9,   8,   8,   8,   7,
         7,   7,   6,   6,   6,   6,   5,   5,
         5,   5,   4,   4,   4,   4,   4,   3,
         3,   3,   3,   3,   3,   3,   2,   2,
         2,   2,   2,   2,   2,   2,   2,   1,
         1,   1,   1,   1,   1,   1,   1,   1,
         1,   1,   1,   1,   1,   1,   1,   1
    },
    {
        71,  68,  65,  64,  62,  59,  57,  55,
        53,  51,  50,  48,  46,  44,  43,  41,
        40,  38,  36,  35,  33,  32,  31,  29,
        29,  27,  26,  25,  24,  23,  22,  21,
        20,  19,  19,  18,  17,  16,  16,  15,
        14,  14,  13,  12,  12,  11,  11,  10,
        10,  10,   9,   9,   8,   8,   8,   7,
         7,   7,   6,   6,   6,   6,   5,   1
    },
    {
        77,  74,  71,  68,  65,  62,  60,  57,
        55,  52,  50,  48,  46,  44,  42,  40,
        39,  37,  35,  34,  33,  31,  30,  29,
        27,  26,  25,  24,  23,  22,  21,  20,
        19,  19,  18,  17,  16,  16,  15,  14,
        14,  13,  13,  12,  12,  11,  11,  10,
        10,   9,   9,   8,   8,   8,   7,   7,
         7,   7,   6,   6,   6,   6,   5,   2
    },
    {
      {
        {3,OC_DEF_QRANGE_SIZES,OC_DEF_BASE_MATRICES[0]},
        {3,OC_DEF_QRANGE_SIZES,OC_DEF_BASE_MATRICES[1]},
        {3,OC_DEF_QRANGE_SIZES,OC_DEF_BASE_MATRICES[5]}
      },
      {
        {3,OC_DEF_QRANGE_SIZES,OC_DEF_BASE_MATRICES[0]},
        {3,OC_DEF_QRANGE_SIZES,OC_DEF_BASE_MATRICES[1]},
        {3,OC_DEF_QRANGE_SIZES,OC_DEF_BASE_MATRICES[5]}
      }
    }
  }
};


void oc_quant_params_pack(oggpack_buffer *_opb,const th_quant_info *_qinfo){
  const th_quant_ranges *qranges;
  const th_quant_base   *base_mats[2*3*64];
  int                    indices[2][3][64];
  int                    nbase_mats;
  int                    nbits;
  int                    ci;
  int                    qi;
  int                    qri;
  int                    qti;
  int                    pli;
  int                    qtj;
  int                    plj;
  int                    bmi;
  int                    i;
  /*323 bits for the defaults.*/
  /*Unlike the scale tables, we can't assume the maximum value will be in
     index 0, so search for it here.*/
  i=_qinfo->loop_filter_limits[0];
  for(qi=1;qi<64;qi++)i=OC_MAXI(i,_qinfo->loop_filter_limits[qi]);
  nbits=oc_ilog(i);
  oggpackB_write(_opb,nbits,3);
  for(qi=0;qi<64;qi++){
    oggpackB_write(_opb,_qinfo->loop_filter_limits[qi],nbits);
  }
  /*452 bits for the defaults, 580 for VP3.*/
  nbits=OC_MAXI(oc_ilog(_qinfo->ac_scale[0]),1);
  oggpackB_write(_opb,nbits-1,4);
  for(qi=0;qi<64;qi++)oggpackB_write(_opb,_qinfo->ac_scale[qi],nbits);
  /*260 bits for the defaults, 516 for VP3.*/
  nbits=OC_MAXI(oc_ilog(_qinfo->dc_scale[0]),1);
  oggpackB_write(_opb,nbits-1,4);
  for(qi=0;qi<64;qi++)oggpackB_write(_opb,_qinfo->dc_scale[qi],nbits);
  /*Consolidate any duplicate base matrices.*/
  nbase_mats=0;
  for(qti=0;qti<2;qti++)for(pli=0;pli<3;pli++){
    qranges=_qinfo->qi_ranges[qti]+pli;
    for(qri=0;qri<=qranges->nranges;qri++){
      for(bmi=0;;bmi++){
        if(bmi>=nbase_mats){
          base_mats[bmi]=qranges->base_matrices+qri;
          indices[qti][pli][qri]=nbase_mats++;
          break;
        }
        else if(memcmp(base_mats[bmi][0],qranges->base_matrices[qri],
         sizeof(base_mats[bmi][0]))==0){
          indices[qti][pli][qri]=bmi;
          break;
        }
      }
    }
  }
  /*Write out the list of unique base matrices.
    6153 bits for our default matrices, 1545 bits for VP3 matrices.*/
  oggpackB_write(_opb,nbase_mats-1,9);
  for(bmi=0;bmi<nbase_mats;bmi++){
    for(ci=0;ci<64;ci++)oggpackB_write(_opb,base_mats[bmi][0][ci],8);
  }
  /*Now store quant ranges and their associated indices into the base matrix
     list.
    236 bits for our default matrices, 46 bits for VP3 matrices.*/
  nbits=oc_ilog(nbase_mats-1);
  for(i=0;i<6;i++){
    qti=i/3;
    pli=i%3;
    qranges=_qinfo->qi_ranges[qti]+pli;
    if(i>0){
      if(qti>0){
        if(qranges->nranges==_qinfo->qi_ranges[qti-1][pli].nranges&&
         memcmp(qranges->sizes,_qinfo->qi_ranges[qti-1][pli].sizes,
         qranges->nranges*sizeof(qranges->sizes[0]))==0&&
         memcmp(indices[qti][pli],indices[qti-1][pli],
         (qranges->nranges+1)*sizeof(indices[qti][pli][0]))==0){
          oggpackB_write(_opb,1,2);
          continue;
        }
      }
      qtj=(i-1)/3;
      plj=(i-1)%3;
      if(qranges->nranges==_qinfo->qi_ranges[qtj][plj].nranges&&
       memcmp(qranges->sizes,_qinfo->qi_ranges[qtj][plj].sizes,
       qranges->nranges*sizeof(qranges->sizes[0]))==0&&
       memcmp(indices[qti][pli],indices[qtj][plj],
       (qranges->nranges+1)*sizeof(indices[qti][pli][0]))==0){
        oggpackB_write(_opb,0,1+(qti>0));
        continue;
      }
      oggpackB_write(_opb,1,1);
    }
    oggpackB_write(_opb,indices[qti][pli][0],nbits);
    for(qi=qri=0;qi<63;qri++){
      oggpackB_write(_opb,qranges->sizes[qri]-1,oc_ilog(62-qi));
      qi+=qranges->sizes[qri];
      oggpackB_write(_opb,indices[qti][pli][qri+1],nbits);
    }
  }
}



/*See comments at oc_dequant_tables_init() for how the quantization tables'
   storage should be initialized.*/
void oc_enquant_tables_init(oc_quant_table *_dequant[2][3],
 oc_quant_table *_enquant[2][3],const th_quant_info *_qinfo){
  int qti;
  int pli;
  /*Initialize the dequantization tables first.*/
  oc_dequant_tables_init(_dequant,NULL,_qinfo);
  /*Derive the quantization tables directly from the dequantization tables.*/
  for(qti=0;qti<2;qti++)for(pli=0;pli<3;pli++){
    int qi;
    /*These simple checks help us improve cache coherency later.*/
    if(pli>0&&_dequant[qti][pli]==_dequant[qti][pli-1]){
      _enquant[qti][pli]=_enquant[qti][pli-1];
      continue;
    }
    if(qti>0&&_dequant[qti][pli]==_dequant[qti-1][pli]){
      _enquant[qti][pli]=_enquant[qti-1][pli];
      continue;
    }
    for(qi=0;qi<64;qi++){
      int ci;
      /*All of that VP3.2 floating point code has been implemented as integer
         code with exact precision.
        I.e., (ogg_uint16_t)(1.0/q*OC_FQUANT_SCALE+0.5) has become
         (2*OC_FQUANT_SCALE+q)/(q<<1).*/
      for(ci=0;ci<64;ci++){
        ogg_uint32_t q;
        q=_dequant[qti][pli][qi][ci];
        _enquant[qti][pli][qi][ci]=(ogg_uint16_t)(
         (2*OC_FQUANT_SCALE+q)/(q<<1));
      }
    }
  }
}
