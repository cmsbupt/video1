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

#ifndef _AVS_TRANSFORM_H_
#define _AVS_TRANSFORM_H_

#include "global.h"
#include "../../lcommon/inc/commonVariables.h"


// functions

int  find_sad_8x8 ( int iMode, int iSizeX, int iSizeY, int iOffX, int iOffY, int resiY[MAX_CU_SIZE][MAX_CU_SIZE] );
int  sad_hadamard ( int iSizeX, int iSizeY, int iOffX, int iOffY, int resiY[MIN_CU_SIZE][MIN_CU_SIZE] );

int  sad_hadamard4x4 (  int resiY[MIN_CU_SIZE][MIN_CU_SIZE] );

void xUpdateCandList( int uiMode, double uiCost, int uiFullCandNum, int * CandModeList, double * CandCostList );
int calcHAD( int ** pi, int iWidth, int iHeight );
int  Mode_Decision_for_AVS_IntraBlocks ( codingUnit * currMB, unsigned int uiBitSize, unsigned int uiPositionInPic, int b8, double lambda, int *min_cost );
double RDCost_for_AVSIntraBlocks ( codingUnit * currMB, unsigned int uiBitSize, unsigned int uiPositionInPic, int *nonzero, int b8, int ipmode, double lambda, double  min_rdcost, int * mostProbableMode, int isChroma );
void quantization (int qp, int mode, int b8, int **curr_blk,  unsigned int uiBitSize, unsigned int uiPositionInPic, codingUnit * currMB, int isChroma, int intraPredMode);
int  inverse_quantization (int qp, int mode, int b8, int **curr_blk, int scrFlag, int *cbp,  unsigned int uiBitSize, unsigned int uiPositionInPic, codingUnit * currMB, int isChroma);
void inverse_transform(  int b8, int **curr_blk, unsigned int uiBitSize, unsigned int uiPositionInPic, codingUnit * currMB, int isChroma);

#if EXTEND_BD
extern short IQ_SHIFT[80];
extern unsigned short IQ_TAB[80];
extern unsigned short Q_TAB[80];
#else
extern short IQ_SHIFT[64];
extern unsigned short IQ_TAB[64];
extern unsigned short Q_TAB[64];
#endif

#endif // _AVS_TRANSFORM_H_
