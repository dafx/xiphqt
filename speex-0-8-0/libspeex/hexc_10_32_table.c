/* Copyright (C) 2002 Jean-Marc Valin 
   File: hexc_10_32_table.c
   Codebook for high-band excitation in SB-CELP mode (4000 bps)
  
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

   1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.  

   2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
   IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
   INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
   STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/

float hexc_10_32_table[32][10]={
{0.051364,0.004356,-0.004322,0.001384,0.120476,0.357375,0.785727,-1.508356,-0.450928,0.454636},
{-1.752247,-0.954283,-1.560063,-0.008506,-0.295434,0.296760,0.194227,0.395801,0.055508,-0.033656},
{0.466473,-0.431319,0.437330,-0.087312,0.203919,-0.331154,0.227619,-0.236355,0.348642,-0.331026},
{-0.016064,0.007879,0.021213,-0.014318,0.047478,0.060166,-0.393304,0.464035,-1.708652,1.684782},
{-0.392888,-0.953182,1.487882,-0.952867,0.481303,-0.044535,-0.261095,0.297167,-0.412005,0.240419},
{-0.034502,0.100662,-0.091294,-0.031752,-0.161733,-0.289738,-1.207797,1.074361,0.341492,0.380422},
{0.201441,-0.122678,-0.118435,0.500207,-0.880341,0.761293,-0.246244,-0.101848,1.033872,-0.898595},
{-0.047698,0.753306,0.070868,1.300748,-1.324289,0.020769,-0.215234,-0.067174,-0.036744,0.126553},
{0.012139,-0.024232,-0.041471,0.353640,0.627918,-0.472855,-0.992323,-1.010560,0.863156,0.698430},
{1.370679,0.196801,0.227321,-1.294604,-1.041896,-0.074951,0.292348,0.616852,0.440378,0.118910},
{-0.072268,0.006660,-0.028443,-0.034885,0.035330,-0.057347,0.528408,-0.830019,1.659012,-1.520934},
{-0.134400,0.005341,-0.030818,-0.061556,-0.111165,0.270597,0.603371,1.258344,-1.241643,-0.634452},
{1.539236,-0.381021,1.289311,0.481035,-0.620896,0.840511,-0.936456,0.528998,-0.513098,0.197301},
{1.968439,0.886318,-0.273808,0.671632,-0.057046,0.080523,-0.216307,-0.062329,-0.081718,0.048866},
{-1.260717,-1.440788,-0.320781,-0.114824,0.798784,0.810185,0.304607,-0.079885,-0.241416,-0.629623},
{-0.486179,1.416242,-0.832432,-0.004218,0.185920,-0.622372,0.489864,-0.521085,0.379386,-0.113544},
{-0.443210,0.419914,1.069873,1.428810,0.565141,0.136896,-0.249190,-0.580576,-0.651433,-0.293431},
{-0.253229,-0.453966,-0.400100,-0.984486,0.679340,-0.300030,1.538374,-0.257918,0.142489,-0.573646},
{0.538639,-0.580006,1.217292,-1.433887,1.418098,-1.034428,0.748440,-0.569872,0.300507,0.034179},
{0.024804,0.086340,-0.211142,0.290998,-0.798505,0.979495,-1.595515,1.922215,-0.951234,0.541487},
{0.559070,-1.699680,-0.404422,-0.270891,-0.065401,0.222026,0.029040,0.451988,-0.144311,0.224000},
{-0.241696,0.083297,0.004277,-0.703106,1.345270,-1.317389,0.252710,0.202348,-0.414938,0.487314},
{-0.009329,0.010869,-0.001348,-0.008489,-0.010858,0.039764,-0.031642,0.028523,-0.059353,0.062010},
{-0.487052,0.672704,-1.889003,1.325382,-1.187024,0.999405,-0.501302,0.340239,0.025195,-0.231435},
{0.957949,1.366740,1.375151,0.051362,0.508726,-0.024864,-0.382025,-0.378570,-0.286660,-0.017188},
{-0.359989,-0.316801,-1.213553,-0.536663,-0.472621,-0.101476,0.482269,0.325869,0.611524,-0.102750},
{-0.180824,0.001945,0.050018,0.099263,0.336777,-1.703052,1.615420,-1.369483,1.016551,-0.236258},
{-0.175850,0.001881,-0.299965,0.220411,-0.370262,1.936677,-1.135033,0.043155,-0.389060,-0.207132},
{2.409395,-0.727781,1.590088,-0.627343,0.587975,-0.178415,-0.247722,0.247041,-0.439136,0.492128},
{1.113168,1.025882,0.750539,0.196165,-0.798223,-1.368542,-0.203361,0.011581,0.294730,0.813502},
{-2.171338,-0.064544,-0.075549,-0.554025,0.159198,-0.173585,0.265976,-0.015812,0.122712,-0.061426},
{-2.047335,0.958457,-1.489224,0.855781,-0.334934,0.081884,0.340905,-0.509467,0.487732,-0.570032}};
