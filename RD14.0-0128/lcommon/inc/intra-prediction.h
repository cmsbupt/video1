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
#include "../../ldecod/inc/global.h"
#include "commonVariables.h"
#include "defines.h"
#include "contributors.h"


/////////////////////////////////////////////////////////////////////////////
/// external function declaration
/////////////////////////////////////////////////////////////////////////////
#if EXTEND_BD
void predIntraLumaAdi( short* pSrc, int** piPredBuf,int uiDirMode, unsigned int uiBitSize, int bAbove, int bLeft,int bs_y,int bs_x, int sample_bit_depth );
void predIntraChromaAdi( short * pSrc, int** piPredBuf, int uiDirMode, unsigned int uiBitSize, int bAbove, int bLeft, int LumaMode, int sample_bit_depth );
#else
void predIntraLumaAdi( short* pSrc, int** piPredBuf,int uiDirMode, unsigned int uiBitSize, int bAbove, int bLeft,int bs_y,int bs_x );
void predIntraChromaAdi( short * pSrc, int** piPredBuf, int uiDirMode, unsigned int uiBitSize, int bAbove, int bLeft, int LumaMode );
#endif

#if BUGFIX_AVAILABILITY_INTRA
void getIntraNeighborAvailabilities(codingUnit *currMB, int maxCuSizeInBit, int img_x, int img_y, int bsx, int bsy, int *p_avail);
#endif