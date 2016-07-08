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
* File name: header.h
* Function: Prototypes for header.c
*
*************************************************************************************
*/



#ifndef _HEADER_H_
#define _HEADER_H_
#include "global.h"
#include "../../lcommon/inc/commonVariables.h"
int bbv_check_times;

int slice_vertical_position_extension;
int slice_horizontal_position_extension;    //added by mz, 2008.04

int picture_coding_type;
#if !M3595_REMOVE_REFERENCE_FLAG
int picture_reference_flag;
#endif 
int frame_rate;

int marker_bit;

int  SliceHeader ( int slice_nr, int slice_qp );
int  IPictureHeader ( int frame );
int  PBPictureHeader();
int  CheckInterlaceSetting ( int idx );
void write_terminating_bit ( unsigned char );

void DeblockFrame ( byte **imgY, byte ***imgUV );
void getStatisticsSAO_one_SMB(int smb_index, int mb_y, int mb_x, int smb_mb_height, int smb_mb_width, SAOStatData** saostatData);
void getParaSAO_one_SMB(int smb_index,int mb_y, int mb_x, int smb_mb_height, int smb_mb_width, int *slice_sao_on, SAOStatData** saostatData, SAOBlkParam* saoBlkParam, SAOBlkParam** rec_saoBlkParam, int* num_off_sao, double sao_labmda);
void writeParaSAO_one_SMB(int smb_index,int mb_y, int mb_x, int smb_mb_height, int smb_mb_width, int *slice_sao_on, SAOBlkParam* saoBlkParam);

#endif
