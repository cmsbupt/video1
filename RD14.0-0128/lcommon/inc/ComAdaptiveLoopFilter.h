/*
*****************************************************************************
* COPYRIGHT AND WARRANTY INFORMATION
*
* Copyright 2002-2012, Advanced Audio Video Coding Standard, Part II
*
* DISCLAIMER OF WARRANTY
*
* The contents of this file are subject to the Mozilla Public License
* Version 1.1 (the "License"); you may not use this file except in
* compliance with the License. You may obtain a copy of the License at
* http://www.mozilla.org/MPL/
*
* Software distributed under the License is distributed on an "AS IS"
* basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
* License for the specific language governing rights and limitations under
* the License.
*
* THIS IS NOT A GRANT OF PATENT RIGHTS - SEE THE AVS PATENT POLICY.
* The AVS Working Group doesn't represent or warrant that the programs
* furnished here under are free of infringement of any third-party patents.
* Commercial implementations of AVS, including shareware, may be
* subject to royalty fees to patent holders. Information regarding
* the AVS patent policy for standardization procedure is available at
* AVS Web site http://www.avs.org.cn. Patent Licensing is outside
* of AVS Working Group.
*
* The Initial Developer of the Original Code is Video subgroup of AVS
* Workinggroup.
* Contributors: Qin Yu,         Zhichu He,  Weiran Li,    Yong Ling,
*               Zhenjiang Shao, Jie Chen,   Junjun Si,    Xiaozhen Zheng, 
*               Jian Lou,       Qiang Wang, Jianwen Chen, Haiwu Zhao,
*               Guoping Li,     Siwei Ma,   Junhao Zheng, Zhiming Wang
*               Li Zhang,
******************************************************************************
*/


#ifndef _COMADAPTIVELOOPFILTER_H_
#define _COMADAPTIVELOOPFILTER_H_
#include "commonVariables.h"
#define ALF_MAX_NUM_COEF       9
#define NO_VAR_BINS            16 
#define LOG2_VAR_SIZE_H        2
#define LOG2_VAR_SIZE_W        2
#define NO_VAR_BINS            16 
#define ALF_FOOTPRINT_SIZE     7
#define DF_CHANGED_SIZE        3
#define ALF_NUM_BIT_SHIFT      6
#define MAX_DOUBLE             1.7e+308
extern int weightsShape1Sym[ALF_MAX_NUM_COEF+1];
extern unsigned int g_MaxSizeInbit;

void setFilterImage(byte* pDecY,byte** pDecUV,int stride,byte** imgY,byte*** imgUV,int img_width,int img_height);
void reconstructCoefficients(ALFParam* alfParam, int** filterCoeff);
void reconstructCoefInfo(int compIdx, ALFParam* alfParam, int** filterCoeff, int* varIndTab);
void checkFilterCoeffValue( int *filter, int filterLength, Boolean isChroma );
void copyALFparam(ALFParam* dst,ALFParam* src);
#if EXTEND_BD
void filterOneCompRegion(byte *imgRes, byte *imgPad, int stride, Boolean isChroma, int yPos, int lcuHeight, int xPos, int lcuWidth, int** filterSet, int* mergeTable, 
                         byte** varImg, int sample_bit_depth, int isLeftAvail, int isRightAvail, int isAboveAvail, int isBelowAvail, int isAboveLeftAvail, int isAboveRightAvail);
#else
void filterOneCompRegion(byte *imgRes, byte *imgPad, int stride, Boolean isChroma, int yPos, int yPosEnd, int xPos, int xPosEnd, int** filterSet, int* mergeTable, byte** varImg, 
						 int isLeftAvail, int isRightAvail, int isAboveAvail, int isBelowAvail);
#endif
void ExtendPicBorder(byte* img,int iHeight,int iWidth,int iMarginY,int iMarginX,byte* imgExt);
int getLCUCtrCtx_Idx(int ctu,int numLCUInPicWidth,int numLCUInPicHeight,int NumCUInFrame,int compIdx,Boolean** AlfLCUEnabled);

#endif