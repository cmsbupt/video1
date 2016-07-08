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

#ifndef _MV_SEARCH_H_
#define _MV_SEARCH_H_

void BackwardPred ( int *best_bw_ref, int *bw_mcost, int mode, int block, double lambda_motion, int max_ref, int adjust_ref , int mb_nr, int bit_size );//ZHAOHAIWU

int check_mvd ( int mvd_x, int mvd_y );
int check_mv_range ( unsigned int uiBitSize, int mv_x, int mv_y, int pix_x, int pix_y, int blocktype );
int check_mv_range_sym ( unsigned int uiBitSize, int mv_x, int mv_y, int pix_x, int pix_y, int blocktype, int ref );
void SetMotionVectorPredictor ( unsigned int uiBitSize, unsigned int uiPositionInPic, int  pmv[2], int  **refFrArr
							   , int  ***tmp_mv , int  ref_frame, int  mb_pix_x, int  mb_pix_y, int  blockshape_x, int  blockshape_y, int  ref, int  direct_mv );

void SetSkipMotionVectorPredictor ( unsigned int uiBitSize, unsigned int uiPositionInPic, int  **fw_refFrArr, int **bw_refFrArr, int  ***tmp_fwmv , int ***tmp_bwmv, 
	int  ref_frame, int  mb_pix_x, int  mb_pix_y, int  blockshape_x, int  blockshape_y,  int  direct_mv );

#endif