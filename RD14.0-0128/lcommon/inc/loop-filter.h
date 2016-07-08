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
#ifndef _LOOP_FILTER_H_
#define _LOOP_FILTER_H_

#include "commonVariables.h"
#include "defines.h"
#include "contributors.h"

void CreateEdgeFilter();//ZHAOHAIWU
void xSetEdgeFilter_One_SMB( unsigned int uiBitSize, unsigned int uiPositionInPic);//ZHAOHAIWU
void Copy_SMB_for_SAO(int smb_index, int mb_y, int mb_x, int smb_mb_height, int smb_mb_width);//ZHAOHAIWU
void Deblock_One_SMB(int smb_index, int mb_y, int mb_x, int smb_mb_height, int smb_mb_width);//ZHAOHAIWU

extern int saoclip[NUM_SAO_OFFSET][3];
void off_sao(SAOBlkParam* saoblkparam);
void copySAOParam_for_blk(SAOBlkParam* saopara_dst,SAOBlkParam* saopara_src);
void copySAOParam_for_blk_onecomponent(SAOBlkParam* saopara_dst,SAOBlkParam* saopara_src);
void Copy_frame_for_SAO();
void Copy_frame_for_file(FILE * p_pic_yuv, byte   **imgY, byte   ***imgUV);
void Copy_frame_for_file_pred(FILE * p_pic_yuv, int   *imgY, int   *imgUV[]);
void getMergeNeighbor(int smb_index, int pix_y, int pix_x, int smb_pix_width, int smb_pix_height, int input_MaxsizeInBit, int input_slice_set_enable,SAOBlkParam** rec_saoBlkParam, int* MergeAvail, SAOBlkParam sao_merge_param[][NUM_SAO_COMPONENTS] );
void checkBoundaryAvail(int mb_y, int mb_x, int smb_pix_height, int smb_pix_width, int input_slice_set_enable, int* smb_available_left, int* smb_available_right, int* smb_available_up, int* smb_available_down, int* smb_available_upleft, int* smb_available_upright, int* smb_available_leftdown, int* smb_available_rightdwon, int filter_on);
#if SAO_SLICE_BOUND_PRO
#if SAO_MULSLICE_FIX
void checkBoundaryProc(int pix_y, int pix_x, int smb_pix_height, int smb_pix_width, int comp, int input_slice_set_enable, int* smb_process_left, int* smb_process_right, int* smb_process_up, int* smb_process_down, int* smb_process_upleft, int* smb_process_upright, int* smb_process_leftdown, int* smb_process_rightdwon, int filter_on);
#else
void checkBoundaryProc(int mb_y, int mb_x, int smb_pix_height, int smb_pix_width, int input_slice_set_enable, int* smb_process_left, int* smb_process_right, int* smb_process_up, int* smb_process_down, int* smb_process_upleft, int* smb_process_upright, int* smb_process_leftdown, int* smb_process_rightdwon, int filter_on);
#endif
void checkBoundaryPara(int mb_y, int mb_x, int smb_pix_height, int smb_pix_width, int input_slice_set_enable, int* smb_process_left, int* smb_process_right, int* smb_process_up, int* smb_process_down, int* smb_process_upleft, int* smb_process_upright, int* smb_process_leftdown, int* smb_process_rightdwon, int filter_on);
#endif
#if EXTEND_BD
void SAOFrame(int input_MaxSizeInBit, int input_slice_set_enable, SAOBlkParam** rec_saoBlkParam, int *slice_sao_on, int sample_bit_depth);
void SAO_on_smb(int smb_index,int pix_y, int pix_x, int smb_pix_width, int smb_pix_height, int input_slice_set_enable, SAOBlkParam* saoBlkParam, int sample_bit_depth);
void SAO_on_block(SAOBlkParam* saoBlkParam, int compIdx, int smb_index, int smb_y, int smb_x, int smb_pix_height, int smb_pix_width, int isLeftAvail, int isRightAvail, int isAboveAvail, int isBelowAvail, int isAboveLeftAvail, int isAboveRightAvail, int isBelowLeftAvail, int isBelowRightAvail, int sample_bit_depth);
#else
void SAOFrame(int input_MaxSizeInBit, int input_slice_set_enable, SAOBlkParam** rec_saoBlkParam, int *slice_sao_on);
void SAO_on_smb(int smb_index,int pix_y, int pix_x, int smb_pix_width, int smb_pix_height, int input_slice_set_enable, SAOBlkParam* saoBlkParam);
void SAO_on_block(SAOBlkParam* saoBlkParam, int compIdx, int smb_index, int smb_y, int smb_x, int smb_pix_height, int smb_pix_width, int isLeftAvail, int isRightAvail, int isAboveAvail, int isBelowAvail, int isAboveLeftAvail, int isAboveRightAvail, int isBelowLeftAvail, int isBelowRightAvail);
#endif

#endif
