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
* File name: commonVariables.h
* Function:  common variable definitions for for AVS encoder and decoder.
*
*************************************************************************************
*/
#include <stdio.h>                              //!< for FILE
#include "defines.h"
#include "commonStructures.h"
#ifndef _GLOBAL_COM_H_
#define _GLOBAL_COM_H_

extern CameraParamters *camera;
extern SNRParameters *snr;
extern ImageParameters *img;
FILE *p_log;                     //!< SNR file
FILE *p_trace;                   //!< Trace file
int  Bframe_ctr;
int tot_time;

// Tsinghua for picture_distance  200701
int picture_distance;

// M3178 PKU Reference Manage
int coding_order;

int   seq_header;
int **AVS_SCAN8x8;//=NULL;
int **AVS_SCAN16x16;//=NULL;
int **AVS_SCAN32x32;//=NULL;

int **AVS_SCAN4x4;//=NULL;
int **AVS_CG_SCAN8x8;//=NULL;
int **AVS_CG_SCAN16x16;//=NULL;
int **AVS_CG_SCAN32x32;//=NULL;
int **AVS_SCAN4x16;
int **AVS_SCAN16x4;
int **AVS_SCAN8x32;
int **AVS_SCAN32x8;
int **AVS_SCAN2x8;
int **AVS_SCAN8x2;

#if EXTRACT_PIC_YUV_PRED   //2016-5-23 提取整帧图像的经过预测后的数据
byte   **imgY_pred;               //!< Encoded/decoded luma images
byte   ***imgUV_pred;              //!< Encoded/decoded croma images
#endif
byte   **imgY;               //!< Encoded/decoded luma images
byte   ***imgUV;              //!< Encoded/decoded croma images
byte **imgY_sao;
byte **imgUV_sao[2];
byte *imgY_alf_Rec;
byte **imgUV_alf_Rec;
byte *imgY_alf_Org;
byte **imgUV_alf_Org;
byte **imgYPrev;
byte ***imgUVPrev;

int   **refFrArr;           //!< Array for reference frames of each block
int   **p_snd_refFrArr;     

#if EXTEND_BD
byte ***currentFrame;//[yuv][height][width]
#else
unsigned char ***currentFrame;//[yuv][height][width]
#endif

#ifdef AVS2_S2_BGLONGTERM
#if EXTEND_BD
byte **backgroundReferenceFrame[3];
byte ***background_ref;
#else
unsigned char **backgroundReferenceFrame[3];
unsigned char ***background_ref;
#endif
#endif
//cms DPB 存放参考帧的图像数据
#if EXTEND_BD
byte **referenceFrame[REF_MAXBUFFER][3];
byte ***ref[REF_MAXBUFFER];

int  total_frames;

#else
unsigned char **referenceFrame[REF_MAXBUFFER][3];
unsigned char ***ref[REF_MAXBUFFER];
#endif

#if EXTRACT_DPB_ColMV_B
int refPicTypebuf[256];//cms DPB 存放参考帧的图像类型，用来指示每个帧的类型，若是B帧，打印时域参考mv时，0改为1，1改为0，-1不变
#endif

int **refbuf[REF_MAXBUFFER];//cms DPB 存放参考帧的参考索引
int ***mvbuf[REF_MAXBUFFER];//cms DPB 存放参考帧的运动存储单元
#if SIMP_INTRA
int g_log2size[MAX_CU_SIZE+1];
#endif
// mv_range, 20071009
int  Min_V_MV;
int  Max_V_MV;
int  Min_H_MV;
int  Max_H_MV;

#define ET_SIZE 300      //!< size of error text buffer
char errortext[ET_SIZE]; //!< buffer for error message for exit with error()

double *saorate[REF_MAXBUFFER];

#if M3198_CU8
extern int g_Left_Down_Avail_Matrix64[16][16];
extern int g_Left_Down_Avail_Matrix32[8][8];
extern int g_Left_Down_Avail_Matrix16[4][4];
extern int g_Left_Down_Avail_Matrix8[2][2];
extern int g_Up_Right_Avail_Matrix64[16][16];
extern int g_Up_Right_Avail_Matrix32[8][8];
extern int g_Up_Right_Avail_Matrix16[4][4];
extern int g_Up_Right_Avail_Matrix8[2][2];
#else
extern int g_Left_Down_Avail_Matrix64[4][8][8];
extern int g_Left_Down_Avail_Matrix32[3][4][4];
extern int g_Left_Down_Avail_Matrix16[2][2][2];
extern int g_Up_Right_Avail_Matrix64[4][8][8];
extern int g_Up_Right_Avail_Matrix32[3][4][4];
extern int g_Up_Right_Avail_Matrix16[2][2][2];
#endif

extern int g_blk_size[20][2];
///encoder only
int **tmp;
int **tmp_block_88_inv;

#if SAO_CROSS_SLICE_FLAG
extern int sao_cross_slice;
#endif


#endif
