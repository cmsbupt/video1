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
#include "commonVariables.h"
#include "defines.h"
#include "contributors.h"


/////////////////////////////////////////////////////////////////////////////
/// external function declaration
/////////////////////////////////////////////////////////////////////////////

void  get_b8_offset( int blocktype, int uiBitSize, int i, int j, int *start_x, int *start_y, int * width, int *height );
void  get_pix_offset( int blocktype, int uiBitSize, int i, int j, int *start_x, int *start_y, int * width, int *height );

void get_mb_pos ( int mb_addr, int *x, int*y, unsigned int uiBitSize );

//AEC
void CheckAvailabilityOfNeighbors ( codingUnit* currMB, unsigned int uiBitSize, unsigned int uiPositionInPic );

void getLuma8x8Neighbour ( int b8_x, int b8_y, int rel_x, int rel_y, PixelPos *pix, int uiPosition, int uiBitSize, codingUnit *currMB, int bw_flag );
#if TEXT_RD92_ALIGN
void getNeighbour ( int xN, int yN, int luma, PixelPos *pix, int uiPosition, int uiBitSize, codingUnit *currMB );
#endif 

#if HALF_PIXEL_COMPENSATION
int getDeltas(
    int *delt,			//delt for original MV
    int *delt2,			//delt for scaled MV
    int OriPOC, int OriRefPOC, int ScaledPOC, int ScaledRefPOC
    ); 
#endif