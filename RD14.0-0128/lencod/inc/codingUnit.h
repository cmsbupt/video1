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



/*
*************************************************************************************
* File name:
* Function:
*
*************************************************************************************
*/

#ifndef _MACROBLOCK_H_
#define _MACROBLOCK_H_

#include "global.h"
#include "../../lcommon/inc/commonVariables.h"

int writeReferenceIndex ( codingUnit *currMB, unsigned int uiBitSize, unsigned int uiPositionInPic, int pos, int *rate_top, int *rate_bot );
int writeCBPandDqp ( codingUnit *currMB, int pos, int *rate_top, int *rate_bot, int uiPositionInPic );
int writeMVD ( codingUnit* currMB, int pos, int *rate_top, int *rate_bot, int uiPositionInPic );

void encode_golomb_word ( unsigned int symbol, unsigned int grad0, unsigned int max_levels, unsigned int *res_bits, unsigned int *res_len ); //returns symbol coded. (might be cropped if max_levels is too small)
void encode_multilayer_golomb_word ( unsigned int symbol, const unsigned int *grad, const unsigned int *max_levels, unsigned int *res_bits, unsigned int *res_len ); //terminate using a max_levels value of 30UL.
int writeSyntaxElement_GOLOMB ( SyntaxElement *se, Bitstream *bitstream );
int estLumaCoeff8x8   ( int **Quant_Coeff, codingUnit* currMB, unsigned int uiBitSize, int intra_mode, int uiPositionInPic );
int estChromaCoeff8x8 ( int **Quant_Coeff, codingUnit* currMB, unsigned int uiBitSize, int b8 , int uiPositionInPic );
void copyMBInfo( codingUnit *srcMB, codingUnit *dstMB );
int estCGLastRate    ( int iCG, BiContextTypePtr pCTX, int bitSize, int *CGLastX, int *CGLastY, int isChroma, int ctxmode, codingUnit *currMB, int intraPredMode );
int estLastRunRate ( int lastRun, BiContextTypePtr pCTX, int rank, int iCG, int isChroma, int ctxmode );
#if BBRY_CU8
int estRunRate   ( int run, BiContextTypePtr pCTX, int pos, int iCG, int remainingPos, int isChroma, int ctxmode ,int bitSize );
#else
int estRunRate   ( int run, BiContextTypePtr pCTX, int pos, int iCG, int remainingPos, int isChroma, int ctxmode );
#endif
int estLastPosRate ( int lastPosX, int lastPosY, BiContextTypePtr pCTX, int isLastCG, int CGLastX, int CGLastY , int iCG, int ctxmode, unsigned int uiBitSize, int isChroma);
int estLastCGLastPosRate ( int lastPosX, int lastPosY, BiContextTypePtr pCTX, int offset, int CGLastX, int CGLastY );
#if BBRY_CU8
int estLevelRate ( int absLevel, BiContextTypePtr pCTX, int rank, int pairsInCG, int iCG, int pos, int isChroma, int bitSize );
#else
int estLevelRate ( int absLevel, BiContextTypePtr pCTX, int rank, int pairsInCG, int iCG, int pos, int isChroma );
#endif
int estSignRate  ( int level );
int estSigCGFlagRate ( int sigCGFlag, BiContextTypePtr pCTX, int ctx );

#endif

