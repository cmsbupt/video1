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

#ifndef _RDOQH_
#define _RDOQH_

#include "global.h"
#include "../../lcommon/inc/commonVariables.h"
#include "block.h"
#include "rdopt_coding_state.h"
#include "codingUnit.h"

typedef struct level_data_struct
{
  int     level[3];    // candidate levels
  int     levelDouble; // coefficient before quantization
#if RDOQ_ROUNDING_ERR
  int     levelDouble2; // coefficient before quantization
#endif
  double  errLevel[3]; // quantization errors of each candidate
  int     noLevels;    // number of candidate levels
  int     xx;          // x coordinate of the coeff in transform block
  int     yy;          // y coordinate of the coeff in transform block
  int     scanPos;     // position in transform block zig-zag scan order
} levelDataStruct;


void quant_init   ( int qp, int mode, int **curr_blk, unsigned int uiBitSize, levelDataStruct *levelData );
void choose_level ( levelDataStruct *levelData, int **curr_blk, unsigned int uiBitSize, int mode, double lambda, int isChroma, unsigned int uiPositionInPic );
void rdoq_block   ( int qp, int mode, int **curr_blk, unsigned int uiBitSize, int isChroma, unsigned int uiPositionInPic, int intraPredMode, codingUnit* currMB );
int  estimate_bits( int **Quant_Coeff, unsigned int uiBitSize, int uiPositionInPic );

#endif