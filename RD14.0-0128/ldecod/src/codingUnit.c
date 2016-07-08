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
* File name: codingUnit.c
* Function: Decode a codingUnit
*
*************************************************************************************
*/



#include "../../lcommon/inc/contributors.h"

#include <math.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "global.h"
#include "../../lcommon/inc/commonVariables.h"
#include "codingUnit.h"
#include "vlc.h"
#include "../../lcommon/inc/transform.h"
#include "../../lcommon/inc/defines.h"
#include "../../lcommon/inc/intra-prediction.h"
#include "../../lcommon/inc/inter-prediction.h"
#include "../../lcommon/inc/block_info.h"
#include "../../lcommon/inc/memalloc.h"
#include "block.h"
#include "AEC.h"
#define max(a, b)                   ((a) > (b) ? (a) : (b))  //!< Macro returning max value 
#define min(a, b)                   ((a) < (b) ? (a) : (b))  //!< Macro returning min value 
// Adaptive frequency weighting quantization  
#if FREQUENCY_WEIGHTING_QUANTIZATION
#include "wquant.h"
#endif

#if EXTRACT
//0,1,2,3,4 ->Sym=0,  bipred=1,  double_fwd=2,  bck=3,  single_fwd=4
int  B_skip_dir[5]={0,1,3,0,4};//表87 图像的预测方向表
int  B_direct_dir[5]={0,1,3,0,4};//表87 图像的预测方向表

int  B_2N_dir[4]={4,3,0,1};//表88 图像的预测方向表

int  B_2PU_dir_CU8x8[4][2]={{4,4},{4,3},{3,3},{3,4}};//表89 图像的预测方向表  CU=8x8
int  B_2PU_dir_CUNo8x8[16][2]={{1,1},{1,3},{1,4},{1,0},\
								{3,3},{3,1},{3,4},{3,0},\
								{4,4},{4,1},{4,3},{4,0},\
								{0,0},{0,1},{0,3},{0,4}\
								};//表89 图像的预测方向表  CU=8x8

int  B_NxN_dir[5]={0,4,3,0,1};//{对称，单前向，后向，对称，双向}
							//表90 图像的预测方向表


//wighted_skip_mode=0时
int  F_skip_dir[5]={4,2,2,4,4};//表87 图像的预测方向表
int  F_direct_dir[5]={4,2,2,4,4};//表87 图像的预测方向表
int  F_2PU_dir_CUNo8x8[4][2]={{4,4},{4,2},{2,4},{2,2}};//表94 图像的预测方向表  CU=8x8

char * aInterPredModeStr[14]={"PSKIPDIRECT","P2NX2N","P2NXN","PNX2N","PHOR_UP","PHOR_DOWN","PVER_LEFT","PVER_RIGHT","PNXN","I8MB","I16MB","IBLOCK","InNxNMB","INxnNMB"};
char * aInterPredDirStr[5]={"FORWARD","BACKWARD","SYM","BID","DUAL"};

int cuCoeffY [MAX_CU_SIZE][ MAX_CU_SIZE];
int cuCoeffCb[MAX_CU_SIZE][ MAX_CU_SIZE];
int cuCoeffCr[MAX_CU_SIZE][ MAX_CU_SIZE];
void initCUInfoExtract()
{
  int i=0;
  //int  cuInfoIntra[8];//0-3是亮度预测模式，4是色度预测模式，  
  memset(cuInfoIntra,0,8*sizeof(int));  
  //int  cuInfoInter[8];//
  memset(cuInfoInter,0,8*sizeof(int)); 
  //int  puInfoInter[4][16];//
  memset(puInfoInter,0,4*16*sizeof(int)); 
  
  //int  EcuInfoInter[8];//
  memset(EcuInfoInter,0,8*sizeof(int));
  //int  EcuInfoInterSyntax[64];//在每个CU 开始时进行初始化
  memset(EcuInfoInterSyntax,0,64*sizeof(int));
  //1.6 MV解码数据
  //int EcuInfoInterMv[64];//用于存放解码后的MV 信息
  memset(EcuInfoInterMv,0,64*sizeof(int));
  for(i=0;i<8;i++)
    EcuInfoInterMv[14+i]=-1;
  
  puInfoInter[0][10]=-1;
  puInfoInter[0][11]=-1;
  puInfoInter[1][10]=-1;
  puInfoInter[1][11]=-1;
  puInfoInter[2][10]=-1;
  puInfoInter[2][11]=-1;
  puInfoInter[3][10]=-1;
  puInfoInter[3][11]=-1;

  //初始化 CU 的DCT 系数，码流中读取的
  memset(cuCoeffY,0,sizeof(int)*MAX_CU_SIZE*MAX_CU_SIZE);
  memset(cuCoeffCb,0,sizeof(int)*MAX_CU_SIZE*MAX_CU_SIZE);
  memset(cuCoeffCr,0,sizeof(int)*MAX_CU_SIZE*MAX_CU_SIZE);
}
#endif	

#if EXTEND_BD
extern  short IQ_SHIFT[80];
extern  unsigned  short IQ_TAB[80];
#else
extern  short IQ_SHIFT[64];
extern  unsigned  short IQ_TAB[64];
#endif

void readMotionVector ( unsigned int uiBitSize, unsigned int uiPositionInPic );
void readReferenceIndex ( unsigned int uiBitSize, unsigned int uiPositionInPic );

extern int DCT_Pairs;


  int dmh_pos[DMH_MODE_NUM+DMH_MODE_NUM-1][2][2] = {
    { { 0,  0},  {0,  0 } }, 
    { {-1,  0 }, {1,  0 } }, 
    { { 0, -1 }, {0,  1 } }, 
    { {-1,  1 }, {1, -1 } }, 
    { {-1, -1 }, {1,  1 } },
    { {-2,  0 }, {2,  0 } }, 
    { { 0, -2 }, {0,  2 } }, 
    { {-2,  2 }, {2, -2 } }, 
    { {-2, -2 }, {2,  2 } }
  };

void pmvr_mv_derivation(int mv[2], int mvd[2], int mvp[2]);

/*
*************************************************************************
* Function:Set motion vector predictor
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
#define MEDIAN(a,b,c)  (a + b + c - min(a, min(b, c)) - max(a, max(b, c)));

void SetMotionVectorPredictor ( unsigned int uiBitSize, unsigned int uiPositionInPic, int  pmv[2], int  **refFrArr,
                                int  ***tmp_mv, int  ref_frame, int  mb_pix_x, int  mb_pix_y, int  blockshape_x, int  blockshape_y, int  ref, int  direct_mv ) //Lou 1016//qyu 0816 
{
  int mb_nr                = uiPositionInPic;
  int mb_width             = img->width / MIN_CU_SIZE;
  int mb_x = mb_nr % mb_width;
  int mb_y = mb_nr / mb_width;
  int mb_pix_x_temp = mb_pix_x % MIN_BLOCK_SIZE == 0 ? mb_pix_x : MIN_BLOCK_SIZE;
  int mb_pix_y_temp = mb_pix_y % MIN_BLOCK_SIZE == 0 ? mb_pix_y : MIN_BLOCK_SIZE;
  int pic_block_x          = ( mb_x << 1 ) + ( mb_pix_x_temp >> MIN_BLOCK_SIZE_IN_BIT);
  int pic_block_y          = ( mb_y << 1 ) + ( mb_pix_y_temp >> MIN_BLOCK_SIZE_IN_BIT );

  int mb_available_up;
  int mb_available_left;
  int mb_available_upleft;
  int mb_available_upright;
  int block_available_up, block_available_left, block_available_upright, block_available_upleft;
  int mv_a, mv_b, mv_c, mv_d, pred_vec = 0;
  int mvPredType, rFrameL, rFrameU, rFrameUR;
  int hv;
  int mva[3] , mvb[3], mvc[3];
  int y_up = 1, y_upright = 1, y_upleft = 1, off_y = 0;
  int rFrameUL;

  int upright;

#ifdef AVS2_S2_BGLONGTERM
  int mvc_temp;
#endif

  blockshape_x = blockshape_x % MIN_BLOCK_SIZE == 0 ? blockshape_x : MIN_BLOCK_SIZE;
  blockshape_y = blockshape_y % MIN_BLOCK_SIZE == 0 ? blockshape_y : MIN_BLOCK_SIZE;

  if ( input->slice_set_enable ) //added by mz, 2008.04
  {
    mb_available_up      = ( mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr - mb_width  ].slice_set_index );
    mb_available_left    = ( mb_x == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr - 1         ].slice_set_index );
    mb_available_upleft  = ( mb_x == 0 || mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr - mb_width - 1].slice_set_index );
    mb_available_upright = ( mb_x >= mb_width - ( 1 << ( uiBitSize - MIN_CU_SIZE_IN_BIT ) ) || mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr - mb_width + ( 1 << ( uiBitSize - MIN_CU_SIZE_IN_BIT ) ) ].slice_set_index );
  }
  else
  {
    mb_available_up      = ( mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr - mb_width  ].slice_nr );      //jlzheng 6.23
    mb_available_left    = ( mb_x == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr - 1         ].slice_nr );      // jlzheng 6.23
    mb_available_upleft  = ( mb_x == 0 || mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr - mb_width - 1].slice_nr );

    if ( ( pic_block_y == 0 ) || ( ( pic_block_x << MIN_BLOCK_SIZE_IN_BIT ) + blockshape_x == img->width ) )
    {
      mb_available_upright = 1;
    }
    else if ( mb_pix_y > 0 )
    {
      mb_available_upright = 0;
    }
    else
    {
      mb_available_upright = ( img->mb_data[uiPositionInPic].slice_nr != img->mb_data[uiPositionInPic - img->PicWidthInMbs + ( mb_pix_x_temp + blockshape_x ) / MIN_CU_SIZE].slice_nr );
    }
  }

  block_available_up   = mb_available_up   || ( mb_pix_y > 0 );
  block_available_left = mb_available_left || ( mb_pix_x > 0 );

  if ( input->g_uiMaxSizeInBit == B64X64_IN_BIT )
  {
    upright = g_Up_Right_Avail_Matrix64[pic_block_y - img->block8_y][pic_block_x - img->block8_x + blockshape_x / MIN_BLOCK_SIZE - 1];
  }
  else if ( input->g_uiMaxSizeInBit == B32X32_IN_BIT )
  {
    upright = g_Up_Right_Avail_Matrix32[pic_block_y - img->block8_y][pic_block_x - img->block8_x + blockshape_x / MIN_BLOCK_SIZE - 1];
  }
  else if ( input->g_uiMaxSizeInBit == B16X16_IN_BIT )
  {
    upright = g_Up_Right_Avail_Matrix16[pic_block_y - img->block8_y][pic_block_x - img->block8_x + blockshape_x / MIN_BLOCK_SIZE - 1];
  }
#if M3198_CU8
  else if ( input->g_uiMaxSizeInBit == B8X8_IN_BIT )
  {
    upright = g_Up_Right_Avail_Matrix8[pic_block_y - img->block8_y][pic_block_x - img->block8_x + blockshape_x / MIN_BLOCK_SIZE - 1];
  }
#endif

  if ( ( pic_block_x << MIN_BLOCK_SIZE_IN_BIT ) + blockshape_x >= img->width )
  {
    upright = 0;
  }

  block_available_upright = upright && ( !mb_available_upright );

  if ( mb_pix_x > 0 )
  {
    block_available_upleft = ( mb_pix_y > 0 ? 1 : block_available_up );
  }
  else if ( mb_pix_y > 0 )
  {
    block_available_upleft = block_available_left;
  }
  else
  {
    block_available_upleft = mb_available_upleft;
  }


#if EXTRACT_lcu_info_debug
  fprintf(p_lcu_debug, "MVP  block_available_left=%d up=%d up-right=%d\n", block_available_left, block_available_up, block_available_upright);
#endif
  /*Lou 1016 Start*/
#if XY_MIN_PMV
  mvPredType = MVPRED_xy_MIN;
#else
  mvPredType = MVPRED_MEDIAN;
#endif
#if EXTRACT_lcu_info_debug
  fprintf(p_lcu_debug, "MVP  pic_block_y=%d pic_block_x=%d \n", pic_block_y, pic_block_x);
  fprintf(p_lcu_debug, "MVP  off_y=%d y_up=%d \n", off_y, y_up);
  fprintf(p_lcu_debug, "MVP  y_upright=%d y_upleft=%d \n", y_upright, y_upleft);
  if(block_available_left)
    fprintf(p_lcu_debug, "MVP  refFrArr[pic_block_y  - off_y]  [pic_block_x - 1]=%d \n", refFrArr[pic_block_y - off_y][pic_block_x - 1]);
  if(block_available_up)
    fprintf(p_lcu_debug, "MVP  refFrArr[pic_block_y - y_up][pic_block_x]=%d \n", refFrArr[pic_block_y - y_up][pic_block_x]);
  //fprintf(p_lcu_debug, "MVP  refFrArr[pic_block_y  - off_y]  [pic_block_x - 1]=%d refFrArr[pic_block_y - y_up][pic_block_x]=%d \n", refFrArr[pic_block_y - off_y][pic_block_x - 1], refFrArr[pic_block_y - y_up][pic_block_x]);
#endif
  rFrameL    = block_available_left    ? refFrArr[pic_block_y  - off_y]  [pic_block_x - 1] : -1;
  rFrameU    = block_available_up      ? refFrArr[pic_block_y - y_up][pic_block_x]   : -1;
  rFrameUR   = block_available_upright ? refFrArr[pic_block_y - y_upright][pic_block_x + ( blockshape_x / MIN_BLOCK_SIZE ) ] : block_available_upleft  ? refFrArr[pic_block_y - y_upleft][pic_block_x - 1] : -1;
  rFrameUL   = block_available_upleft  ? refFrArr[pic_block_y - y_upleft][pic_block_x - 1] : -1;
#if EXTRACT_lcu_info_debug
  fprintf(p_lcu_debug, "MVP  rFrameL=%d rFrameU=%d rFrameUR=%d \n", rFrameL, rFrameU, rFrameUR);
  fprintf(p_lcu_debug, "MVP  ref_frame=%d background_picture_enable=%d img->num_of_references=%d \n", ref_frame, background_picture_enable, img->num_of_references);
#endif
#ifdef AVS2_S2_BGLONGTERM
  if(rFrameL!=-1)
  {
#if AVS2_SCENE_CD
	  if((ref_frame==img->num_of_references-1&&rFrameL!=img->num_of_references-1||ref_frame!=img->num_of_references-1&&rFrameL==img->num_of_references-1)&&(img->type==INTER_IMG || img->type == F_IMG)&&background_picture_enable)
#else
	  if((ref_frame==img->num_of_references-1&&rFrameL!=img->num_of_references-1||ref_frame!=img->num_of_references-1&&rFrameL==img->num_of_references-1)&&img->type==INTER_IMG&&background_picture_enable&&input->profile_id==0x50)
#endif
	  {
		  rFrameL=-1; 
#if EXTRACT_lcu_info_debug
        fprintf(p_lcu_debug, "MVP  AVS2_SCENE_CD rFrameL=%d rFrameU=%d rFrameUR=%d \n", rFrameL, rFrameU, rFrameUR);     
#endif
	  }
#ifdef AVS2_S2_S
#if AVS2_SCENE_CD
	  if(img->typeb == BP_IMG )
#else
	  if(img->typeb == BP_IMG && input->profile_id == 0x50)
#endif
	  {
		  rFrameL=-1;
#if EXTRACT_lcu_info_debug
      fprintf(p_lcu_debug, "MVP  img->typeb == BP_IMG rFrameL=%d rFrameU=%d rFrameUR=%d \n", rFrameL, rFrameU, rFrameUR);
#endif
	  }
#endif
  }
  if(rFrameU!=-1)
  {
#if AVS2_SCENE_CD
	  if((ref_frame==img->num_of_references-1&&rFrameU!=img->num_of_references-1||ref_frame!=img->num_of_references-1&&rFrameU==img->num_of_references-1)&&(img->type==INTER_IMG || img->type == F_IMG)&&background_picture_enable)
#else
	  if((ref_frame==img->num_of_references-1&&rFrameU!=img->num_of_references-1||ref_frame!=img->num_of_references-1&&rFrameU==img->num_of_references-1)&&img->type==INTER_IMG&&background_picture_enable&&input->profile_id==0x50)
#endif
	  {
		  rFrameU=-1;
	  }
#ifdef AVS2_S2_S
#if AVS2_SCENE_CD
	  if(img->typeb == BP_IMG )
#else
	  if(img->typeb == BP_IMG && input->profile_id == 0x50)
#endif
	  {
		  rFrameU=-1;
	  }
#endif
  }
  if(rFrameUR!=-1)
  {
#if AVS2_SCENE_CD
	  if((ref_frame==img->num_of_references-1&&rFrameUR!=img->num_of_references-1||ref_frame!=img->num_of_references-1&&rFrameUR==img->num_of_references-1)&&(img->type==INTER_IMG || img->type == F_IMG)&&background_picture_enable)
#else
	  if((ref_frame==img->num_of_references-1&&rFrameUR!=img->num_of_references-1||ref_frame!=img->num_of_references-1&&rFrameUR==img->num_of_references-1)&&img->type==INTER_IMG&&background_picture_enable&&input->profile_id==0x50)
#endif
	  {
		  rFrameUR=-1;
	  }
#ifdef AVS2_S2_S
#if AVS2_SCENE_CD
	  if(img->typeb == BP_IMG )
#else
	  if(img->typeb == BP_IMG && input->profile_id == 0x50)
#endif
	  {
		  rFrameUR=-1;
	  }
#endif
  }
  if(rFrameUL!=-1)
  {
#if AVS2_SCENE_CD
	  if((ref_frame==img->num_of_references-1&&rFrameUL!=img->num_of_references-1||ref_frame!=img->num_of_references-1&&rFrameUL==img->num_of_references-1)&&(img->type==INTER_IMG || img->type == F_IMG)&&background_picture_enable)
#else
	  if((ref_frame==img->num_of_references-1&&rFrameUL!=img->num_of_references-1||ref_frame!=img->num_of_references-1&&rFrameUL==img->num_of_references-1)&&img->type==INTER_IMG&&background_picture_enable&&input->profile_id==0x50)
#endif
	  {
		  rFrameUL=-1;
	  }
#ifdef AVS2_S2_S
#if AVS2_SCENE_CD
	  if(img->typeb == BP_IMG )
#else
	  if(img->typeb == BP_IMG && input->profile_id == 0x50)
#endif
	  {
		  rFrameUL=-1;
	  }
#endif
  }
#endif

  if ( ( rFrameL != -1 ) && ( rFrameU == -1 ) && ( rFrameUR == -1 ) )
  {
    mvPredType = MVPRED_L;
  }
  else if ( ( rFrameL == -1 ) && ( rFrameU != -1 ) && ( rFrameUR == -1 ) )
  {
    mvPredType = MVPRED_U;
  }
  else if ( ( rFrameL == -1 ) && ( rFrameU == -1 ) && ( rFrameUR != -1 ) )
  {
    mvPredType = MVPRED_UR;
  }
  else if ( blockshape_x < blockshape_y  )
  {
    if ( mb_pix_x == 0 )
    {
      if ( rFrameL == ref_frame )
      {
        mvPredType = MVPRED_L;
      }
    }
    else
    {
      if ( rFrameUR == ref_frame )
      {
        mvPredType = MVPRED_UR;
      }
    }
  }
  else if ( blockshape_x > blockshape_y )
  {
    if ( mb_pix_y == 0 )
    {
      if ( rFrameU == ref_frame )
      {
        mvPredType = MVPRED_U;
      }
    }
    else
    {
      if ( rFrameL == ref_frame )
      {
        mvPredType = MVPRED_L;
      }
    }
  }

#if EXTRACT_lcu_info_debug
  fprintf(p_lcu_debug, "MVP  mvPredType=%d MVPRED_L=1 MVPRED_U=2 MVPRED_UR=3 \n", mvPredType);
  fprintf(p_lcu_debug, "MVP  rFrameL=%d rFrameU=%d rFrameUR=%d\n", rFrameL, rFrameU, rFrameUR);
#endif
  for (hv = 0; hv < 2; hv++)
  {

    mva[hv] = mv_a = block_available_left ? tmp_mv[pic_block_y - off_y][pic_block_x - 1][hv] : 0;
    mvb[hv] = mv_b = block_available_up ? tmp_mv[pic_block_y - y_up][pic_block_x][hv] : 0;
    mv_d = block_available_upleft ? tmp_mv[pic_block_y - y_upleft][pic_block_x - 1][hv] : 0;
    mvc[hv] = mv_c = block_available_upright ? tmp_mv[pic_block_y - y_upright][pic_block_x + (blockshape_x / MIN_BLOCK_SIZE)][hv] : mv_d;

#if EXTRACT_lcu_info_debug
    fprintf(p_lcu_debug, "MVP  hv=%d mva=%d mvb=%d mvc=%d mvd=%d \n", hv, mva[hv], mvb[hv], mvc[hv], mv_d);
#endif
#ifdef AVS2_S2_BGLONGTERM
#ifdef AVS2_S2_S
#if AVS2_SCENE_CD
    if ((rFrameL == -1 && (img->type == INTER_IMG || img->type == F_IMG) && background_picture_enable) || (rFrameL == -1 && img->typeb == BP_IMG))
    {
#else
    if ((rFrameL == -1 && img->type == INTER_IMG && background_picture_enable && input->profile_id == 0x50) || (rFrameL == -1 && img->typeb == BP_IMG && input->profile_id == 0x50))
#endif
#else
#if AVS2_SCENE_CD
    if (rFrameL == -1 && img->type == INTER_IMG && background_picture_enable)
#else
    if (rFrameL == -1 && img->type == INTER_IMG && background_picture_enable && input->profile_id == 0x50)
#endif
#endif
      mva[hv] = 0;
#if EXTRACT_lcu_info_debug
    fprintf(p_lcu_debug, "MVP  a hv=%d mva[0]=%d  ref_frame=%d rFrameL=%d ref=%d\n", hv, mva[0], ref_frame, rFrameL, ref);
#endif
  }
	else
#endif
#if HALF_PIXEL_COMPENSATION_PMV
    {
        if (img->is_field_sequence && hv==1 && rFrameL!=-1 )
        {
            int delta, delta2, oriPOC, oriRefPOC, scaledPOC, scaledRefPOC;
            int mult_distance = calculate_distance ( ref_frame, ref );
            int devide_distance = calculate_distance ( rFrameL, ref );

            oriPOC = 2*picture_distance; 
            oriRefPOC = oriPOC - devide_distance;
            scaledPOC = 2*picture_distance;
            scaledRefPOC = scaledPOC - mult_distance;
            getDeltas( &delta, &delta2, oriPOC, oriRefPOC, scaledPOC, scaledRefPOC);
            mva[hv] = scale_motion_vector ( mva[hv]+delta, ref_frame, rFrameL, ref ); //, smbtypecurr, smbtypeL, pic_block_y-off_y, pic_block_y, ref, direct_mv);
            mva[hv] -= delta2;
        }
        else
        {
            mva[hv] = scale_motion_vector ( mva[hv], ref_frame, rFrameL, ref ); //, smbtypecurr, smbtypeL, pic_block_y-off_y, pic_block_y, ref, direct_mv);
#if EXTRACT_lcu_info_debug
          fprintf(p_lcu_debug, "MVP  scale_motion_vector a hv=%d mva[0]=%d  ref_frame=%d rFrameL=%d ref=%d\n", hv, mva[0], ref_frame, rFrameL, ref);
#endif
        }

    }
#else
		mva[hv] = scale_motion_vector ( mva[hv], ref_frame, rFrameL, ref ); //, smbtypecurr, smbtypeL, pic_block_y-off_y, pic_block_y, ref, direct_mv);
#endif

#ifdef AVS2_S2_BGLONGTERM
#ifdef AVS2_S2_S
#if AVS2_SCENE_CD
	if((rFrameU == -1 && (img->type==INTER_IMG || img->type == F_IMG) && background_picture_enable) || (rFrameU == -1 && img->typeb == BP_IMG ))
#else
	if((rFrameU == -1 && img->type==INTER_IMG && background_picture_enable && input->profile_id==0x50) || (rFrameU == -1 && img->typeb == BP_IMG && input->profile_id == 0x50))
#endif
#else
#if AVS2_SCENE_CD
	if(rFrameU == -1 && img->type==INTER_IMG && background_picture_enable)
#else
	if(rFrameU == -1 && img->type==INTER_IMG && background_picture_enable && input->profile_id==0x50)
#endif
#endif
		mvb[hv]=0;
	else
#endif
#if HALF_PIXEL_COMPENSATION_PMV
    {
        if (img->is_field_sequence && hv==1 && rFrameU!=-1 )
        {
            int delta, delta2, oriPOC, oriRefPOC, scaledPOC, scaledRefPOC;
            int mult_distance = calculate_distance ( ref_frame, ref );
            int devide_distance = calculate_distance ( rFrameU, ref );

            oriPOC = 2*picture_distance;  
            oriRefPOC = oriPOC - devide_distance;
            scaledPOC = 2*picture_distance;
            scaledRefPOC = scaledPOC - mult_distance;
            getDeltas( &delta, &delta2, oriPOC, oriRefPOC, scaledPOC, scaledRefPOC);
            mvb[hv] = scale_motion_vector ( mvb[hv]+delta, ref_frame, rFrameU, ref ); //, smbtypecurr, smbtypeU, pic_block_y-y_up, pic_block_y, ref, direct_mv);
            mvb[hv] -= delta2;
        }
        else
            mvb[hv] = scale_motion_vector ( mvb[hv], ref_frame, rFrameU, ref ); //, smbtypecurr, smbtypeU, pic_block_y-y_up, pic_block_y, ref, direct_mv);
    }
#else
		mvb[hv] = scale_motion_vector ( mvb[hv], ref_frame, rFrameU, ref ); //, smbtypecurr, smbtypeU, pic_block_y-y_up, pic_block_y, ref, direct_mv);
#endif

#ifdef AVS2_S2_BGLONGTERM
#ifdef AVS2_S2_S
#if AVS2_SCENE_CD
	if((rFrameUL == -1 && (img->type==INTER_IMG || img->type == F_IMG) && background_picture_enable) || (rFrameUL == -1 && img->typeb == BP_IMG ))
#else
	if((rFrameUL == -1 && img->type==INTER_IMG && background_picture_enable && input->profile_id==0x50) || (rFrameUL == -1 && img->typeb == BP_IMG && input->profile_id == 0x50))
#endif
#else
#if AVS2_SCENE_CD
	if(rFrameUL == -1 && img->type==INTER_IMG && background_picture_enable)
#else
	if(rFrameUL == -1 && img->type==INTER_IMG && background_picture_enable && input->profile_id==0x50)
#endif
#endif
		mv_d=0;
	else
#endif	
#if HALF_PIXEL_COMPENSATION_PMV
    {
        if (img->is_field_sequence && hv==1 && rFrameUL!=-1 )
        {
            int delta, delta2, oriPOC, oriRefPOC, scaledPOC, scaledRefPOC;
            int mult_distance = calculate_distance ( ref_frame, ref );
            int devide_distance = calculate_distance ( rFrameUL, ref );

            oriPOC = 2*picture_distance;  
            oriRefPOC = oriPOC - devide_distance;
            scaledPOC = 2*picture_distance;
            scaledRefPOC = scaledPOC - mult_distance;
            getDeltas( &delta, &delta2, oriPOC, oriRefPOC, scaledPOC, scaledRefPOC);
            mv_d = scale_motion_vector ( mv_d+delta, ref_frame, rFrameUL, ref ); //, smbtypecurr, smbtypeUL, pic_block_y-y_upleft, pic_block_y, ref, direct_mv);
            mv_d -= delta2;
        }
        else
            mv_d = scale_motion_vector ( mv_d, ref_frame, rFrameUL, ref ); //, smbtypecurr, smbtypeUL, pic_block_y-y_upleft, pic_block_y, ref, direct_mv);
    }
#else
		mv_d = scale_motion_vector ( mv_d, ref_frame, rFrameUL, ref ); //, smbtypecurr, smbtypeUL, pic_block_y-y_upleft, pic_block_y, ref, direct_mv);
#endif

#ifdef AVS2_S2_BGLONGTERM
#ifdef AVS2_S2_S
#if AVS2_SCENE_CD
	if((rFrameUR == -1 && (img->type== INTER_IMG || img->type == F_IMG) && background_picture_enable) || (rFrameUR == -1 && img->typeb == BP_IMG ))
#else
	if((rFrameUR == -1 && img->type== INTER_IMG && background_picture_enable && input->profile_id==0x50) || (rFrameUR == -1 && img->typeb == BP_IMG && input->profile_id == 0x50))
#endif
#else
#if AVS2_SCENE_CD
	if(rFrameUR == -1 && img->type== INTER_IMG && background_picture_enable)
#else
	if(rFrameUR == -1 && img->type== INTER_IMG && background_picture_enable && input->profile_id==0x50)
#endif
#endif
		mvc_temp=0;
	else
#if HALF_PIXEL_COMPENSATION_PMV
    {
        if (img->is_field_sequence && hv==1 && rFrameUR!=-1 )
        {
            int delta, delta2, oriPOC, oriRefPOC, scaledPOC, scaledRefPOC;
            int mult_distance = calculate_distance ( ref_frame, ref );
            int devide_distance = calculate_distance ( rFrameUR, ref );

            oriPOC = 2*picture_distance;  
            oriRefPOC = oriPOC - devide_distance;
            scaledPOC = 2*picture_distance;
            scaledRefPOC = scaledPOC - mult_distance;
            getDeltas( &delta, &delta2, oriPOC, oriRefPOC, scaledPOC, scaledRefPOC);
            mvc_temp=scale_motion_vector ( mvc[hv]+delta, ref_frame, rFrameUR, ref );
            mvc_temp -= delta2;
        }
        else
            mvc_temp=scale_motion_vector ( mvc[hv], ref_frame, rFrameUR, ref );
    }
#else
		mvc_temp=scale_motion_vector ( mvc[hv], ref_frame, rFrameUR, ref );
#endif
	mvc[hv] = block_available_upright ? mvc_temp : mv_d;
#else	
	mvc[hv] = block_available_upright ? scale_motion_vector ( mvc[hv], ref_frame, rFrameUR, ref ) : mv_d; //, smbtypecurr, smbtypeUR, pic_block_y-y_upright, pic_block_y, ref, direct_mv);
#endif

    switch ( mvPredType )
    {
#if XY_MIN_PMV
	case MVPRED_xy_MIN:
		if(hv == 1)
		{
#if EXTRACT_lcu_info_debug
      fprintf(p_lcu_debug, "MVP  hv=%d mva[0]=%d  mvb[0]=%d mvc[0]=%d\n", hv, mva[0],  mvb[0], mvc[0]);
#endif
			//for x component
			if( ((mva[0]<0) && (mvb[0]>0) && (mvc[0]>0)) || (mva[0]>0) && (mvb[0]<0) && (mvc[0]<0) )
			{
				pmv[0] = (mvb[0] + mvc[0])/2;
#if EXTRACT_lcu_info_debug
        fprintf(p_lcu_debug, "MVP  hv=%d pmv[0]=%d pmv[0] = (mvb[0] + mvc[0])/2; \n", hv, pmv[0]);
#endif

			}
			else if( ((mvb[0]<0) && (mva[0]>0) && (mvc[0]>0))||((mvb[0]>0) && (mva[0]<0) && (mvc[0]<0)) )
			{
				pmv[0] = (mvc[0] + mva[0])/2;
#if EXTRACT_lcu_info_debug
        fprintf(p_lcu_debug, "MVP  hv=%d pmv[0]=%d pmv[0] = (mvc[0] + mva[0])/2; \n", hv, pmv[0]);
#endif

			}
			else if( ((mvc[0]<0) && (mva[0]>0) && (mvb[0]>0))||((mvc[0]>0) && (mva[0]<0) && (mvb[0]<0)) )
			{
				pmv[0] = (mva[0] + mvb[0])/2;
#if EXTRACT_lcu_info_debug
        fprintf(p_lcu_debug, "MVP  hv=%d pmv[0]=%d pmv[0] = (mva[0] + mvb[0])/2; \n", hv, pmv[0]);
#endif

			}
			else
			{
				// !! for Ax
				mva[2] = abs(mva[0] - mvb[0]);
				// !! for Bx
				mvb[2] = abs(mvb[0] - mvc[0]);
				// !! for Cx
				mvc[2] = abs(mvc[0] - mva[0]);

				pred_vec = min(mva[2],min(mvb[2],mvc[2]));

#if EXTRACT_lcu_info_debug
        fprintf(p_lcu_debug, "MVP  hv=%d pred_vec=%d pred_vec = min(mva[2],min(mvb[2],mvc[2])); \n", hv, pred_vec);
#endif
				if(pred_vec == mva[2]){
					pmv[0] = (mva[0] + mvb[0])/2;
				}
				else if(pred_vec == mvb[2]){
					pmv[0] = (mvb[0] + mvc[0])/2;
				}
				else{
					pmv[0] = (mvc[0] + mva[0])/2;
				}
			}

			//for y component
			if( ((mva[1]<0) && (mvb[1]>0) && (mvc[1]>0)) || (mva[1]>0) && (mvb[1]<0) && (mvc[1]<0) )
			{
				pmv[1] = (mvb[1] + mvc[1])/2;

			}
			else if( ((mvb[1]<0) && (mva[1]>0) && (mvc[1]>0))||((mvb[1]>0) && (mva[1]<0) && (mvc[1]<0)) )
			{
				pmv[1] = (mvc[1] + mva[1])/2;

			}
			else if( ((mvc[1]<0) && (mva[1]>0) && (mvb[1]>0))||((mvc[1]>0) && (mva[1]<0) && (mvb[1]<0)) )
			{
				pmv[1] = (mva[1] + mvb[1])/2;

			}
			else
			{
				// !! for Ay
				mva[2] =  abs(mva[1] - mvb[1]);
				// !! for By
				mvb[2] =  abs(mvb[1] - mvc[1]);
				// !! for Cy
				mvc[2] =  abs(mvc[1] - mva[1]);

				pred_vec = min(mva[2],min(mvb[2],mvc[2]));

				if(pred_vec == mva[2]){
					pmv[1] = (mva[1] + mvb[1])/2;
				}
				else if(pred_vec == mvb[2]){
					pmv[1] = (mvb[1] + mvc[1])/2;
				}
				else{
					pmv[1] = (mvc[1] + mva[1])/2;
				}
			}

#if EXTRACT_lcu_info_debug
      fprintf(p_lcu_debug, "MVP  hv=%d pmv[0]=%d pmv[1]=%d \n", hv, pmv[0],pmv[1]);
#endif
		}
		break;
#else
    case MVPRED_MEDIAN:

      if ( hv == 1 )
      {
        // jlzheng 7.2
        // !! for A
        //
        mva[2] = abs ( mva[0] - mvb[0] ) + abs ( mva[1] - mvb[1] ) ;
        // !! for B
        //
        mvb[2] = abs ( mvb[0] - mvc[0] ) + abs ( mvb[1] - mvc[1] );
        // !! for C
        //
        mvc[2] = abs ( mvc[0] - mva[0] ) + abs ( mvc[1] - mva[1] ) ;

        pred_vec = MEDIAN ( mva[2], mvb[2], mvc[2] );

        if ( pred_vec == mva[2] )
        {
          pmv[0] = mvc[0];
          pmv[1] = mvc[1];
        }

        else if ( pred_vec == mvb[2] )
        {
          pmv[0] = mva[0];
          pmv[1] = mva[1];
        }
        else
        {
          pmv[0] = mvb[0];
          pmv[1] = mvb[1];
        }// END

      }

      break;
#endif
#if !TEXT_RD92_ALIGN
    case MVPRED_L:
      pred_vec = mv_a;
      break;
    case MVPRED_U:
      pred_vec = mv_b;
      break;
    case MVPRED_UR:
      pred_vec = mv_c;
      break;
#else 
	case MVPRED_L:
		pred_vec = mva[hv];
		break;
	case MVPRED_U:
		pred_vec = mvb[hv];
		break;
	case MVPRED_UR:
		pred_vec = mvc[hv];
		break;
#endif 
    default:
      break;
    }
#if EXTRACT_lcu_info_BAC_inter
    fprintf(p_lcu,"mb_pix_x,y=%d %d hv=%d pred_vec=%d\n",mb_pix_x,mb_pix_y,hv,pred_vec);//F pu_reference_index
#endif 	

#if EXTRACT_lcu_info_debug
    fprintf(p_lcu_debug, "MVP  hv=%d pred_vec=%d \n", hv, pred_vec);
#endif
#if XY_MIN_PMV
	if ( mvPredType != MVPRED_xy_MIN )
#else
    if ( mvPredType != MVPRED_MEDIAN )
#endif
    {
      pmv[hv] = pred_vec;
    }
  }
}
void SetSkipMotionVectorPredictor ( unsigned int uiBitSize, unsigned int uiPositionInPic, int  fw_pmv[2], int bw_pmv[2],
	int  ref_frame, int  mb_pix_x, int  mb_pix_y, int  blockshape_x, int  blockshape_y, int num_skip_dir )
{
	int mb_nr                = uiPositionInPic;
	int mb_width             = img->width / MIN_CU_SIZE;
	int mb_x = mb_nr % mb_width;
	int mb_y = mb_nr / mb_width;
#if !M3476_BUF_FIX
	int mb_pix_x_temp = mb_pix_x % 8 == 0 ? mb_pix_x : 8;
	int mb_pix_y_temp = mb_pix_y % 8 == 0 ? mb_pix_y : 8;
#else 
	int mb_pix_x_temp = mb_pix_x % MIN_BLOCK_SIZE == 0 ? mb_pix_x : MIN_BLOCK_SIZE;
	int mb_pix_y_temp = mb_pix_y % MIN_BLOCK_SIZE == 0 ? mb_pix_y : MIN_BLOCK_SIZE;
#endif 
#if !M3476_BUF_FIX
	int pic_block_x          = ( mb_x << 1 ) + ( mb_pix_x_temp >> 3 );
	int pic_block_y          = ( mb_y << 1 ) + ( mb_pix_y_temp >> 3 );
#else 
	int pic_block_x          = ( mb_x << 1 ) + ( mb_pix_x_temp >> MIN_BLOCK_SIZE_IN_BIT );
	int pic_block_y          = ( mb_y << 1 ) + ( mb_pix_y_temp >> MIN_BLOCK_SIZE_IN_BIT );
#endif 
	int mb_available_up;
	int mb_available_left;
	int mb_available_upleft;
	int mb_available_upright;
	int block_available_left1, block_available_up1;
	int block_available_up, block_available_left, block_available_upright, block_available_upleft;
	int rFrameL[2], rFrameU[2], rFrameUR[2],rFrameUL[2],rFrameL1[2], rFrameU1[2];
	int hv;
	int mva[2][2] , mvb[2][2], mvc[2][2];
	int mva1[2][2], mvb1[2][2], mve[2][2];
	int y_up = 1, y_upright = 1, y_upleft = 1, off_y = 0;
	int upright;
	int mb_nb_L,mb_nb_L1, mb_nb_U,mb_nb_U1,mb_nb_UR,mb_nb_UL;
	int bRefFrame[2][6];
	int pmv[2][2][6];
	int i,j,dir;
	int bid_flag=0, bw_flag=0, fw_flag=0, sym_flag=0, bid2;

	int tmp_bwBSkipMv[5][2];
	int tmp_fwBSkipMv[5][2];
#if MOTION_BUGFIX
	int mode_info[6];
	PixelPos block_L, block_U,block_UR, block_UL,block_L1, block_U1;
	codingUnit *neighborMB;
	int sizeOfNeighborBlock_x,sizeOfNeighborBlock_y;
#endif 


#if !M3476_BUF_FIX
	blockshape_x = blockshape_x % 8 == 0 ? blockshape_x : 8;
	blockshape_y = blockshape_y % 8 == 0 ? blockshape_y : 8;
#else 
	blockshape_x = blockshape_x % MIN_BLOCK_SIZE == 0 ? blockshape_x : MIN_BLOCK_SIZE;
	blockshape_y = blockshape_y % MIN_BLOCK_SIZE == 0 ? blockshape_y : MIN_BLOCK_SIZE;
#endif 

	if ( input->slice_set_enable ) 
	{
		mb_available_up      = ( mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr - mb_width  ].slice_set_index );
		mb_available_left    = ( mb_x == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr - 1         ].slice_set_index );
		mb_available_upleft  = ( mb_x == 0 || mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr - mb_width - 1].slice_set_index );
		mb_available_upright = ( mb_x >= mb_width - ( 1 << ( uiBitSize - 4 ) ) || mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr - mb_width + ( 1 << ( uiBitSize - 4 ) ) ].slice_set_index );
	}
	else
	{
		mb_available_up      = ( mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr - mb_width  ].slice_nr );      
		mb_available_left    = ( mb_x == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr - 1         ].slice_nr );      
		mb_available_upleft  = ( mb_x == 0 || mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr - mb_width - 1].slice_nr );

#if !M3476_BUF_FIX
		if ( ( pic_block_y == 0 ) || ( ( pic_block_x << 3 ) + blockshape_x == img->width ) )
#else 
		if ( ( pic_block_y == 0 ) || ( ( pic_block_x << MIN_BLOCK_SIZE_IN_BIT ) + blockshape_x == img->width ) )
#endif 
		{
			mb_available_upright = 1;
		}
		else if ( mb_pix_y > 0 )
		{
			mb_available_upright = 0;
		}
		else
		{
			mb_available_upright = ( img->mb_data[uiPositionInPic].slice_nr != img->mb_data[uiPositionInPic - img->PicWidthInMbs + ( mb_pix_x_temp + blockshape_x ) / MIN_CU_SIZE].slice_nr );
		}
	}

	block_available_up   = mb_available_up   || ( mb_pix_y > 0 );
	block_available_left = mb_available_left || ( mb_pix_x > 0 );
#if !M3476_BUF_FIX
	block_available_left1 = block_available_left && (blockshape_y > 8);
	block_available_up1 = block_available_up && (blockshape_x > 8);
#else 
	block_available_left1 = block_available_left && (blockshape_y > MIN_BLOCK_SIZE);
	block_available_up1 = block_available_up && (blockshape_x > MIN_BLOCK_SIZE);
#endif 

	if ( input->g_uiMaxSizeInBit == B64X64_IN_BIT )
	{
		upright = g_Up_Right_Avail_Matrix64[pic_block_y - img->block8_y][pic_block_x - img->block8_x + blockshape_x / MIN_BLOCK_SIZE - 1];
	}
	else if ( input->g_uiMaxSizeInBit == B32X32_IN_BIT )
	{
		upright = g_Up_Right_Avail_Matrix32[pic_block_y - img->block8_y][pic_block_x - img->block8_x + blockshape_x / MIN_BLOCK_SIZE - 1];
	}
	else if ( input->g_uiMaxSizeInBit == B16X16_IN_BIT )
	{
		upright = g_Up_Right_Avail_Matrix16[pic_block_y - img->block8_y][pic_block_x - img->block8_x + blockshape_x / MIN_BLOCK_SIZE - 1];
	}
#if M3198_CU8
	else if ( input->g_uiMaxSizeInBit == B8X8_IN_BIT )
	{
		upright = g_Up_Right_Avail_Matrix8[pic_block_y - img->block8_y][pic_block_x - img->block8_x + blockshape_x / MIN_BLOCK_SIZE - 1];;
	}
#endif

	if ( ( pic_block_x << MIN_BLOCK_SIZE_IN_BIT ) + blockshape_x >= img->width )
	{
		upright = 0;
	}

	block_available_upright = upright && ( !mb_available_upright );

	if ( mb_pix_x > 0 )
	{
		block_available_upleft = ( mb_pix_y > 0 ? 1 : block_available_up );
	}
	else if ( mb_pix_y > 0 )
	{
		block_available_upleft = block_available_left;
	}
	else
	{
		block_available_upleft = mb_available_upleft;
	}
#if !M3476_BUF_FIX
	mb_nb_L1 = mb_width * ((pic_block_y - off_y + blockshape_y / 8 - 1) >> 1) + ((pic_block_x - 1)>>1);
	mb_nb_L = mb_width * ((pic_block_y  - off_y) >> 1) + ((pic_block_x - 1)>>1);
	mb_nb_U = mb_width * ((pic_block_y - y_up) >> 1) + ((pic_block_x)>>1);
	mb_nb_U1 = mb_width * ((pic_block_y - y_up) >> 1) + ((pic_block_x + blockshape_x / 8 -1)>>1);
	mb_nb_UR = mb_width * ((pic_block_y - y_upright)>>1) + ((pic_block_x + blockshape_x / 8 )>>1);
	mb_nb_UL = mb_width * ((pic_block_y - y_upleft)>>1) + ((pic_block_x - 1)>>1);

	rFrameL[0]    = block_available_left    ? img->bw_refFrArr[pic_block_y  - off_y]  [pic_block_x - 1] : -1;
	rFrameU[0]    = block_available_up      ? img->bw_refFrArr[pic_block_y - y_up][pic_block_x]   : -1;
	rFrameUR[0]   = block_available_upright ? img->bw_refFrArr[pic_block_y - y_upright][pic_block_x + ( blockshape_x / 8 ) ] :  -1;
	rFrameUL[0]   = block_available_upleft  ? img->bw_refFrArr[pic_block_y - y_upleft][pic_block_x - 1] : -1;
	rFrameL1[0] = block_available_left1 ? img->bw_refFrArr[pic_block_y - off_y + blockshape_y / 8 - 1][pic_block_x - 1] : -1;  
	rFrameU1[0] = block_available_up1 ? img->bw_refFrArr[pic_block_y - y_up][pic_block_x + blockshape_x / 8 -1] : -1;

	rFrameL[1]    = block_available_left    ? img->fw_refFrArr[pic_block_y  - off_y]  [pic_block_x - 1] : -1;
	rFrameU[1]    = block_available_up      ? img->fw_refFrArr[pic_block_y - y_up][pic_block_x]   : -1;
	rFrameUR[1]   = block_available_upright ? img->fw_refFrArr[pic_block_y - y_upright][pic_block_x + ( blockshape_x / 8 ) ] :  -1;
	rFrameUL[1]   = block_available_upleft  ? img->fw_refFrArr[pic_block_y - y_upleft][pic_block_x - 1] : -1;
	rFrameL1[1] = block_available_left1 ? img->fw_refFrArr[pic_block_y - off_y + blockshape_y / 8 - 1][pic_block_x - 1] : -1;
	rFrameU1[1] = block_available_up1 ? img->fw_refFrArr[pic_block_y - y_up][pic_block_x + blockshape_x / 8 -1] : -1;
#else 
	mb_nb_L1 = mb_width * ((pic_block_y - off_y + blockshape_y / MIN_BLOCK_SIZE - 1) >> 1) + ((pic_block_x - 1)>>1);
	mb_nb_L = mb_width * ((pic_block_y  - off_y) >> 1) + ((pic_block_x - 1)>>1);
	mb_nb_U = mb_width * ((pic_block_y - y_up) >> 1) + ((pic_block_x)>>1);
	mb_nb_U1 = mb_width * ((pic_block_y - y_up) >> 1) + ((pic_block_x + blockshape_x /MIN_BLOCK_SIZE -1)>>1);
	mb_nb_UR = mb_width * ((pic_block_y - y_upright)>>1) + ((pic_block_x + blockshape_x / MIN_BLOCK_SIZE )>>1);
	mb_nb_UL = mb_width * ((pic_block_y - y_upleft)>>1) + ((pic_block_x - 1)>>1);

	rFrameL[0]    = block_available_left    ? img->bw_refFrArr[pic_block_y  - off_y]  [pic_block_x - 1] : -1;
	rFrameU[0]    = block_available_up      ? img->bw_refFrArr[pic_block_y - y_up][pic_block_x]   : -1;
	rFrameUR[0]   = block_available_upright ? img->bw_refFrArr[pic_block_y - y_upright][pic_block_x + ( blockshape_x / MIN_BLOCK_SIZE ) ] :  -1;
	rFrameUL[0]   = block_available_upleft  ? img->bw_refFrArr[pic_block_y - y_upleft][pic_block_x - 1] : -1;
	rFrameL1[0] = block_available_left1 ? img->bw_refFrArr[pic_block_y - off_y + blockshape_y / MIN_BLOCK_SIZE - 1][pic_block_x - 1] : -1;  
	rFrameU1[0] = block_available_up1 ? img->bw_refFrArr[pic_block_y - y_up][pic_block_x + blockshape_x / MIN_BLOCK_SIZE -1] : -1;

	rFrameL[1]    = block_available_left    ? img->fw_refFrArr[pic_block_y  - off_y]  [pic_block_x - 1] : -1;
	rFrameU[1]    = block_available_up      ? img->fw_refFrArr[pic_block_y - y_up][pic_block_x]   : -1;
	rFrameUR[1]   = block_available_upright ? img->fw_refFrArr[pic_block_y - y_upright][pic_block_x + ( blockshape_x / MIN_BLOCK_SIZE ) ] :  -1;
	rFrameUL[1]   = block_available_upleft  ? img->fw_refFrArr[pic_block_y - y_upleft][pic_block_x - 1] : -1;
	rFrameL1[1] = block_available_left1 ? img->fw_refFrArr[pic_block_y - off_y + blockshape_y / MIN_BLOCK_SIZE - 1][pic_block_x - 1] : -1;
	rFrameU1[1] = block_available_up1 ? img->fw_refFrArr[pic_block_y - y_up][pic_block_x + blockshape_x / MIN_BLOCK_SIZE -1] : -1;
#endif 

	for(i=0; i<2; i++)
	{
		bRefFrame[i][0] = rFrameUL[i];
		bRefFrame[i][1] = rFrameU[i];
		bRefFrame[i][2] = rFrameL[i];
		bRefFrame[i][3] = rFrameUR[i];
		bRefFrame[i][4] = rFrameU1[i];
		bRefFrame[i][5] = rFrameL1[i];
	}
	for ( hv = 0; hv < 2; hv++ )
	{     
#if !M3476_BUF_FIX
		mva[0][hv]  = block_available_left    ? img->bw_mv[pic_block_y - off_y][pic_block_x - 1][hv]              : 0;
		mva1[0][hv]  = block_available_left1 ? img->bw_mv[pic_block_y - off_y + blockshape_y / 8 - 1][pic_block_x - 1][hv] : 0;
		mvb[0][hv] = block_available_up      ? img->bw_mv[pic_block_y - y_up][pic_block_x][hv]                : 0;
		mvb1[0][hv]  = block_available_up1 ? img->bw_mv[pic_block_y - y_up][pic_block_x + blockshape_x / 8 -1][hv] : 0;
		mve[0][hv] = block_available_upleft  ? img->bw_mv[pic_block_y - y_upleft][pic_block_x - 1][hv]              : 0;
		mvc[0][hv]  = block_available_upright ? img->bw_mv[pic_block_y - y_upright][pic_block_x + ( blockshape_x / 8 ) ][hv] : 0;

		mva[1][hv]  = block_available_left    ? img->fw_mv[pic_block_y - off_y][pic_block_x - 1][hv]              : 0;
		mva1[1][hv]  = block_available_left1 ? img->fw_mv[pic_block_y - off_y + blockshape_y / 8 - 1][pic_block_x - 1][hv] : 0;
		mvb[1][hv] = block_available_up      ? img->fw_mv[pic_block_y - y_up][pic_block_x][hv]                : 0;
		mvb1[1][hv]  = block_available_up1 ? img->fw_mv[pic_block_y - y_up][pic_block_x + blockshape_x / 8 -1][hv] : 0;
		mve[1][hv] = block_available_upleft  ? img->fw_mv[pic_block_y - y_upleft][pic_block_x - 1][hv]              : 0;
		mvc[1][hv]  = block_available_upright ? img->fw_mv[pic_block_y - y_upright][pic_block_x + ( blockshape_x / 8 ) ][hv] : 0;
#else 
		mva[0][hv]  = block_available_left    ? img->bw_mv[pic_block_y - off_y][pic_block_x - 1][hv]              : 0;
		mva1[0][hv]  = block_available_left1 ? img->bw_mv[pic_block_y - off_y + blockshape_y / MIN_BLOCK_SIZE - 1][pic_block_x - 1][hv] : 0;
		mvb[0][hv] = block_available_up      ? img->bw_mv[pic_block_y - y_up][pic_block_x][hv]                : 0;
		mvb1[0][hv]  = block_available_up1 ? img->bw_mv[pic_block_y - y_up][pic_block_x + blockshape_x / MIN_BLOCK_SIZE -1][hv] : 0;
		mve[0][hv] = block_available_upleft  ? img->bw_mv[pic_block_y - y_upleft][pic_block_x - 1][hv]              : 0;
		mvc[0][hv]  = block_available_upright ? img->bw_mv[pic_block_y - y_upright][pic_block_x + ( blockshape_x / MIN_BLOCK_SIZE ) ][hv] : 0;

		mva[1][hv]  = block_available_left    ? img->fw_mv[pic_block_y - off_y][pic_block_x - 1][hv]              : 0;
		mva1[1][hv]  = block_available_left1 ? img->fw_mv[pic_block_y - off_y + blockshape_y / MIN_BLOCK_SIZE - 1][pic_block_x - 1][hv] : 0;
		mvb[1][hv] = block_available_up      ? img->fw_mv[pic_block_y - y_up][pic_block_x][hv]                : 0;
		mvb1[1][hv]  = block_available_up1 ? img->fw_mv[pic_block_y - y_up][pic_block_x + blockshape_x / MIN_BLOCK_SIZE -1][hv] : 0;
		mve[1][hv] = block_available_upleft  ? img->fw_mv[pic_block_y - y_upleft][pic_block_x - 1][hv]              : 0;
		mvc[1][hv]  = block_available_upright ? img->fw_mv[pic_block_y - y_upright][pic_block_x + ( blockshape_x / MIN_BLOCK_SIZE ) ][hv] : 0;
#endif 

		for(i = 0; i < 2; i++)
		{
			pmv[i][hv][0] = mve[i][hv];   //i-direction, hv-x or y
			pmv[i][hv][1] = mvb[i][hv];
			pmv[i][hv][2] = mva[i][hv];
			pmv[i][hv][3] = mvc[i][hv];
			pmv[i][hv][4] = mvb1[i][hv];
			pmv[i][hv][5] = mva1[i][hv];
		}
	}

	for(i = 0; i < 2; i++)
	{
		for(dir = 0; dir < 5; dir++)
		{
			tmp_bwBSkipMv[dir][i] = 0;
			tmp_fwBSkipMv[dir][i] = 0;
		}

	}


#if MOTION_BUGFIX
	// PixelPos block_L, block_U,block_UR, block_UL,block_L1, block_U1;

  getNeighbour ( -1, 0, 1, &block_L, uiPositionInPic, uiBitSize, &img->mb_data[uiPositionInPic] );
  getNeighbour ( -1, ( 1 << uiBitSize )-1, 1, &block_L1, uiPositionInPic, uiBitSize, &img->mb_data[uiPositionInPic] );	
  getNeighbour ( 0, -1, 1, &block_U, uiPositionInPic, uiBitSize, &img->mb_data[uiPositionInPic] );
  getNeighbour ( ( 1 << uiBitSize )-1, -1, 1, &block_U1, uiPositionInPic, uiBitSize, &img->mb_data[uiPositionInPic] );
  getNeighbour ( -1, -1, 1, &block_UL, uiPositionInPic, uiBitSize, &img->mb_data[uiPositionInPic] );
  getNeighbour ( ( 1 << uiBitSize ), -1, 1, &block_UR, uiPositionInPic, uiBitSize, &img->mb_data[uiPositionInPic] );

	/*
	bRefFrame[i][0] = rFrameUL[i];
	bRefFrame[i][1] = rFrameU[i];
	bRefFrame[i][2] = rFrameL[i];
	bRefFrame[i][3] = rFrameUR[i];
	bRefFrame[i][4] = rFrameU1[i];
	bRefFrame[i][5] = rFrameL1[i];
	*/
	if (block_UL.available&&block_available_upleft)
	{
       neighborMB = &img->mb_data[block_UL.mb_addr];
	   if (neighborMB->cuType==PHOR_UP)
	   {
		   sizeOfNeighborBlock_y= (1<<neighborMB->ui_MbBitSize)/4;
	   }
	   else if (neighborMB->cuType==PHOR_DOWN)
	   {
		   sizeOfNeighborBlock_y=(1<<neighborMB->ui_MbBitSize)/4*3;
	   }
	   else 
	   {
		   sizeOfNeighborBlock_y=(1<<neighborMB->ui_MbBitSize)/2;
	   }
	   
	   if (neighborMB->cuType==PVER_LEFT)
	   {
		   sizeOfNeighborBlock_x=(1<<neighborMB->ui_MbBitSize)/4;
	   }
	   else if (neighborMB->cuType==PVER_RIGHT)
	   {
		   sizeOfNeighborBlock_x=(1<<neighborMB->ui_MbBitSize)/4*3;
	   }
	   else 
	   {
		   sizeOfNeighborBlock_x=(1<<neighborMB->ui_MbBitSize)/2;
	   }

	   block_UL.x = (block_UL.x /sizeOfNeighborBlock_x)? 1:0;
	   block_UL.y = (block_UL.y / sizeOfNeighborBlock_y)? 1:0;
	   mode_info[0] =  img->mb_data[block_UL.mb_addr].b8pdir[block_UL.x+block_UL.y*2];
	   
	}
	else 
	{
		mode_info[0]= -1;
	}


	if (block_U.available&&block_available_up)
	{
		neighborMB = &img->mb_data[block_U.mb_addr];
		if (neighborMB->cuType==PHOR_UP)
		{
			sizeOfNeighborBlock_y= (1<<neighborMB->ui_MbBitSize)/4;
		}
		else if (neighborMB->cuType==PHOR_DOWN)
		{
			sizeOfNeighborBlock_y=(1<<neighborMB->ui_MbBitSize)/4*3;
		}
		else 
		{
			sizeOfNeighborBlock_y=(1<<neighborMB->ui_MbBitSize)/2;
		}

		if (neighborMB->cuType==PVER_LEFT)
		{
			sizeOfNeighborBlock_x=(1<<neighborMB->ui_MbBitSize)/4;
		}
		else if (neighborMB->cuType==PVER_RIGHT)
		{
			sizeOfNeighborBlock_x=(1<<neighborMB->ui_MbBitSize)/4*3;
		}
		else 
		{
			sizeOfNeighborBlock_x=(1<<neighborMB->ui_MbBitSize)/2;
		}
		
		block_U.x = (block_U.x / sizeOfNeighborBlock_x)? 1:0;
		block_U.y = (block_U.y / sizeOfNeighborBlock_y)? 1:0;
		mode_info[1] =  img->mb_data[block_U.mb_addr].b8pdir[block_U.x+block_U.y*2];
		
	}
	else 
	{
		mode_info[1]= -1;
	}

	if (block_L.available&&block_available_left)
	{
		neighborMB = &img->mb_data[block_L.mb_addr];
		if (neighborMB->cuType==PHOR_UP)
		{
			sizeOfNeighborBlock_y= (1<<neighborMB->ui_MbBitSize)/4;
		}
		else if (neighborMB->cuType==PHOR_DOWN)
		{
			sizeOfNeighborBlock_y=(1<<neighborMB->ui_MbBitSize)/4*3;
		}
		else 
		{
			sizeOfNeighborBlock_y=(1<<neighborMB->ui_MbBitSize)/2;
		}

		if (neighborMB->cuType==PVER_LEFT)
		{
			sizeOfNeighborBlock_x=(1<<neighborMB->ui_MbBitSize)/4;
		}
		else if (neighborMB->cuType==PVER_RIGHT)
		{
			sizeOfNeighborBlock_x=(1<<neighborMB->ui_MbBitSize)/4*3;
		}
		else 
		{
			sizeOfNeighborBlock_x=(1<<neighborMB->ui_MbBitSize)/2;
		}
	
		block_L.x = (block_L.x / sizeOfNeighborBlock_x)? 1:0;
		block_L.y = (block_L.y / sizeOfNeighborBlock_y)? 1:0;
		mode_info[2] = img->mb_data[block_L.mb_addr].b8pdir[block_L.x+block_L.y*2];
		
	}
	else 
	{
		mode_info[2]= -1;
	}

	if (block_UR.available&&block_available_upright)
	{
		neighborMB = &img->mb_data[block_UR.mb_addr];
		if (neighborMB->cuType==PHOR_UP)
		{
			sizeOfNeighborBlock_y= (1<<neighborMB->ui_MbBitSize)/4;
		}
		else if (neighborMB->cuType==PHOR_DOWN)
		{
			sizeOfNeighborBlock_y=(1<<neighborMB->ui_MbBitSize)/4*3;
		}
		else 
		{
			sizeOfNeighborBlock_y=(1<<neighborMB->ui_MbBitSize)/2;
		}

		if (neighborMB->cuType==PVER_LEFT)
		{
			sizeOfNeighborBlock_x=(1<<neighborMB->ui_MbBitSize)/4;
		}
		else if (neighborMB->cuType==PVER_RIGHT)
		{
			sizeOfNeighborBlock_x=(1<<neighborMB->ui_MbBitSize)/4*3;
		}
		else 
		{
			sizeOfNeighborBlock_x=(1<<neighborMB->ui_MbBitSize)/2;
		}
		
		block_UR.x = (block_UR.x / sizeOfNeighborBlock_x)? 1:0;
		block_UR.y = (block_UR.y / sizeOfNeighborBlock_y)? 1:0;
		mode_info[3] =  img->mb_data[block_UR.mb_addr].b8pdir[block_UR.x+block_UR.y*2];
	
	}
	else 
	{
		mode_info[3]= -1;
	}

	if (block_U1.available&&block_available_up1)
	{
		neighborMB = &img->mb_data[block_U1.mb_addr];
		if (neighborMB->cuType==PHOR_UP)
		{
			sizeOfNeighborBlock_y= (1<<neighborMB->ui_MbBitSize)/4;
		}
		else if (neighborMB->cuType==PHOR_DOWN)
		{
			sizeOfNeighborBlock_y=(1<<neighborMB->ui_MbBitSize)/4*3;
		}
		else 
		{
			sizeOfNeighborBlock_y=(1<<neighborMB->ui_MbBitSize)/2;
		}

		if (neighborMB->cuType==PVER_LEFT)
		{
			sizeOfNeighborBlock_x=(1<<neighborMB->ui_MbBitSize)/4;
		}
		else if (neighborMB->cuType==PVER_RIGHT)
		{
			sizeOfNeighborBlock_x=(1<<neighborMB->ui_MbBitSize)/4*3;
		}
		else 
		{
			sizeOfNeighborBlock_x=(1<<neighborMB->ui_MbBitSize)/2;
		}
		
		block_U1.x = (block_U1.x / sizeOfNeighborBlock_x)? 1:0;
		block_U1.y = (block_U1.y / sizeOfNeighborBlock_y)? 1:0;
		mode_info[4] =  img->mb_data[block_U1.mb_addr].b8pdir[block_U1.x+block_U1.y*2];
		
	}
	else 
	{
		mode_info[4]= -1;
	}

	if (block_L1.available&&block_available_left1)
	{
		neighborMB = &img->mb_data[block_L1.mb_addr];
		if (neighborMB->cuType==PHOR_UP)
		{
			sizeOfNeighborBlock_y= (1<<neighborMB->ui_MbBitSize)/4;
		}
		else if (neighborMB->cuType==PHOR_DOWN)
		{
			sizeOfNeighborBlock_y=(1<<neighborMB->ui_MbBitSize)/4*3;
		}
		else 
		{
			sizeOfNeighborBlock_y=(1<<neighborMB->ui_MbBitSize)/2;
		}

		if (neighborMB->cuType==PVER_LEFT)
		{
			sizeOfNeighborBlock_x=(1<<neighborMB->ui_MbBitSize)/4;
		}
		else if (neighborMB->cuType==PVER_RIGHT)
		{
			sizeOfNeighborBlock_x=(1<<neighborMB->ui_MbBitSize)/4*3;
		}
		else 
		{
			sizeOfNeighborBlock_x=(1<<neighborMB->ui_MbBitSize)/2;
		}
		
		block_L1.x = (block_L1.x / sizeOfNeighborBlock_x)? 1:0;
		block_L1.y = (block_L1.y / sizeOfNeighborBlock_y)? 1:0;
		mode_info[5] =  img->mb_data[block_L1.mb_addr].b8pdir[block_L1.x+block_L1.y*2];
		
	}
	else 
	{
		mode_info[5]= -1;
	}

#endif

	for(j = 0; j < 6; j++)
	{

#if !MOTION_BUGFIX
		if( bRefFrame[0][j] != -1 && bRefFrame[1][j] != -1 && ( pmv[0][0][j] != -pmv[1][0][j] || pmv[0][1][j] != -pmv[1][1][j] ) )
#else
		if (mode_info[j]==BID)
#endif 
		{
			tmp_bwBSkipMv[DS_BID][0] = pmv[0][0][j];
			tmp_bwBSkipMv[DS_BID][1] = pmv[0][1][j];
			tmp_fwBSkipMv[DS_BID][0] = pmv[1][0][j];
			tmp_fwBSkipMv[DS_BID][1] = pmv[1][1][j];
			bid_flag++;
			if(bid_flag == 1){
				bid2 = j;
			}
		}
#if !MOTION_BUGFIX
		else if(bRefFrame[0][j] != -1 && bRefFrame[1][j] != -1)
#else 
		else if (mode_info[j]==SYM)
#endif 
		{
			tmp_bwBSkipMv[DS_SYM][0] = pmv[0][0][j];
			tmp_bwBSkipMv[DS_SYM][1] = pmv[0][1][j];
			tmp_fwBSkipMv[DS_SYM][0] = pmv[1][0][j];
			tmp_fwBSkipMv[DS_SYM][1] = pmv[1][1][j];
			sym_flag++;
		}
#if !MOTION_BUGFIX
		else if(bRefFrame[0][j] != -1 && bRefFrame[1][j] == -1)
#else 
		else if(mode_info[j]==BACKWARD)
#endif 
		{
			tmp_bwBSkipMv[DS_BACKWARD][0] = pmv[0][0][j];
			tmp_bwBSkipMv[DS_BACKWARD][1] = pmv[0][1][j];
			bw_flag++;
		}
#if !MOTION_BUGFIX
		else if(bRefFrame[0][j] == -1 && bRefFrame[1][j] != -1)
#else 
		else if (mode_info[j]==FORWARD)
#endif 
		{
			tmp_fwBSkipMv[DS_FORWARD][0] = pmv[1][0][j];
			tmp_fwBSkipMv[DS_FORWARD][1] = pmv[1][1][j];
			fw_flag++;
		}
	}

	if(bid_flag == 0 && fw_flag != 0 && bw_flag != 0)
	{
		tmp_bwBSkipMv[DS_BID][0] = tmp_bwBSkipMv[DS_BACKWARD][0];
		tmp_bwBSkipMv[DS_BID][1] = tmp_bwBSkipMv[DS_BACKWARD][1];
		tmp_fwBSkipMv[DS_BID][0] = tmp_fwBSkipMv[DS_FORWARD][0];
        tmp_fwBSkipMv[DS_BID][1] = tmp_fwBSkipMv[DS_FORWARD][1];
	}

	if(sym_flag == 0 && bid_flag > 1)
	{
		tmp_bwBSkipMv[DS_SYM][0] = pmv[0][0][bid2];
		tmp_bwBSkipMv[DS_SYM][1] = pmv[0][1][bid2];
		tmp_fwBSkipMv[DS_SYM][0] = pmv[1][0][bid2];
		tmp_fwBSkipMv[DS_SYM][1] = pmv[1][1][bid2];
	}
	else if(sym_flag == 0 && bw_flag != 0)
	{
		tmp_bwBSkipMv[DS_SYM][0] = tmp_bwBSkipMv[DS_BACKWARD][0];
		tmp_bwBSkipMv[DS_SYM][1] = tmp_bwBSkipMv[DS_BACKWARD][1];
		tmp_fwBSkipMv[DS_SYM][0] = -tmp_bwBSkipMv[DS_BACKWARD][0];
		tmp_fwBSkipMv[DS_SYM][1] = -tmp_bwBSkipMv[DS_BACKWARD][1];
	}
#if !D20141230_BUG_FIX
	else if(sym_flag == 0 && fw_flag != 0)
	{
		tmp_bwBSkipMv[DS_SYM][0] = -tmp_bwBSkipMv[DS_BACKWARD][0];
		tmp_bwBSkipMv[DS_SYM][1] = -tmp_bwBSkipMv[DS_BACKWARD][1];
		tmp_fwBSkipMv[DS_SYM][0] = tmp_fwBSkipMv[DS_BACKWARD][0];
		tmp_fwBSkipMv[DS_SYM][1] = tmp_fwBSkipMv[DS_BACKWARD][1];
	}
#else 
	else if(sym_flag == 0 && fw_flag != 0)
	{
#if !PMV_BUGFIX
		tmp_bwBSkipMv[DS_SYM][0] = -tmp_bwBSkipMv[DS_FORWARD][0];
		tmp_bwBSkipMv[DS_SYM][1] = -tmp_bwBSkipMv[DS_FORWARD][1];
#else 
		tmp_bwBSkipMv[DS_SYM][0] = -tmp_fwBSkipMv[DS_FORWARD][0];
		tmp_bwBSkipMv[DS_SYM][1] = -tmp_fwBSkipMv[DS_FORWARD][1];
#endif 
		tmp_fwBSkipMv[DS_SYM][0] = tmp_fwBSkipMv[DS_FORWARD][0];
		tmp_fwBSkipMv[DS_SYM][1] = tmp_fwBSkipMv[DS_FORWARD][1];
	}
#endif 

	if(bw_flag == 0 && bid_flag > 1)
	{
		tmp_bwBSkipMv[DS_BACKWARD][0] = pmv[0][0][bid2];
		tmp_bwBSkipMv[DS_BACKWARD][1] = pmv[0][1][bid2];
	}
	else if(bw_flag == 0 && bid_flag !=0)
	{
		tmp_bwBSkipMv[DS_BACKWARD][0] = tmp_bwBSkipMv[DS_BID][0];
		tmp_bwBSkipMv[DS_BACKWARD][1] = tmp_bwBSkipMv[DS_BID][1];
	}

	if(fw_flag == 0 && bid_flag > 1)
	{
		tmp_fwBSkipMv[DS_FORWARD][0] = pmv[1][0][bid2];
		tmp_fwBSkipMv[DS_FORWARD][1] = pmv[1][1][bid2];
	}
	else if(fw_flag == 0 && bid_flag !=0)
	{
		tmp_fwBSkipMv[DS_FORWARD][0] = tmp_fwBSkipMv[DS_BID][0];
		tmp_fwBSkipMv[DS_FORWARD][1] = tmp_fwBSkipMv[DS_BID][1];
	}

	fw_pmv[0] = tmp_fwBSkipMv[num_skip_dir][0];
	fw_pmv[1] = tmp_fwBSkipMv[num_skip_dir][1];
	bw_pmv[0] = tmp_bwBSkipMv[num_skip_dir][0];
	bw_pmv[1] = tmp_bwBSkipMv[num_skip_dir][1];

}

void start_codingUnit ( unsigned int uiBitSize )
{
  int i, j; //,k,l;

  assert ( img->current_mb_nr >= 0 && img->current_mb_nr < img->max_mb_nr );

  // Update coordinates of the current codingUnit
  img->mb_x = ( img->current_mb_nr ) % ( img->PicWidthInMbs );
  img->mb_y = ( img->current_mb_nr ) / ( img->PicWidthInMbs );

  // Define vertical positions
  img->block8_y = img->mb_y * BLOCK_MULTIPLE;    // luma block position /
  //  img->block8_y = img->mb_y * BLOCK_SIZE/2;
  img->pix_y   = img->mb_y * MIN_CU_SIZE;   // luma codingUnit position /

  if ( input->chroma_format == 1 )
  {
    img->pix_c_y = img->mb_y * MIN_CU_SIZE / 2;  // chroma codingUnit position /
  }

  // Define horizontal positions /
  img->block8_x = img->mb_x * BLOCK_MULTIPLE;    // luma block position /
  // img->block8_x = img->mb_x * BLOCK_SIZE/2;
  img->pix_x   = img->mb_x * MIN_CU_SIZE;   // luma pixel position /
  img->pix_c_x = img->mb_x * MIN_CU_SIZE / 2; // chroma pixel position /


  // initialize img->resiY for ABT//Lou

  for ( j = 0; j < ( 1 << (uiBitSize+1) ); j++ )
  {
    for ( i = 0; i < ( 1 << (uiBitSize+1) ); i++ )
    {
      img->resiY[j][i] = 0;
    }
  }

  for ( j = 0; j < ( 1 << uiBitSize ); j++ )
  {
    for ( i = 0; i < ( 1 << uiBitSize ); i++ )
    {
      img->resiUV[0][j][i] = 0;
      img->resiUV[1][j][i] = 0;
    }
  }
}

void Init_SubMB ( unsigned int uiBitSize, unsigned int uiPositionInPic )
{
  int i, j, k, l, r, c;
  codingUnit *currMB = &img->mb_data[uiPositionInPic];  // intialization code deleted, see below, StW

  int N8_SizeScale = 1 << ( uiBitSize - MIN_CU_SIZE_IN_BIT );
  assert ( uiPositionInPic >= 0 && uiPositionInPic < img->max_mb_nr );

  // Reset syntax element entries in MB struct
  currMB->ui_MbBitSize = uiBitSize;
  currMB->qp          = img->qp ;

#if MB_DQP
  currMB->previouse_qp     = img->qp ;
  currMB->left_cu_qp       = img->qp ;
#endif

  currMB->cuType     = PSKIPDIRECT;
  currMB->delta_quant = 0;
  currMB->cbp         = 0;
  currMB->cbp_blk     = 0;
  currMB->c_ipred_mode = DC_PRED_C; //GB

  for ( l = 0; l < 2; l++ )
  {
    for ( j = 0; j < BLOCK_MULTIPLE; j++ )
    {
      for ( i = 0; i < BLOCK_MULTIPLE; i++ )
      {
        for ( k = 0; k < 2; k++ )
        {
          currMB->mvd[l][j][i][k] = 0;
        }
      }
    }
  }

  currMB->cbp_bits   = 0;

  for ( r = 0; r < N8_SizeScale; r++ )
  {
    int pos = uiPositionInPic + r * img->PicWidthInMbs;

    for ( c = 0; c < N8_SizeScale; c++, pos++ )
    {
      if ( r + c == 0 )
      {
        continue;
      }

      memcpy ( &img->mb_data[pos], &img->mb_data[uiPositionInPic], sizeof ( codingUnit ) );
    }
  }
}
/*
*************************************************************************
* Function:Interpret the mb mode for P-Frames
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
void mvd_prediction(int* mvd_U, int* mvd_L, int* mvd_C,int available_U, int available_L, int available_C, int *pmvd)
{
	int abs_U, abs_L, abs_C;
	abs_U = abs(mvd_U[0]) + abs(mvd_U[1]);
	abs_L = abs(mvd_L[0]) + abs(mvd_L[1]);
	abs_C = abs(mvd_C[0]) + abs(mvd_C[1]);

	if (!abs_L||!abs_U)
	{
		pmvd[0] = pmvd[1] = 0;
	} 
	else
	{
		pmvd[0] = MEDIAN( mvd_U[0], mvd_L[0], mvd_C[0]);
		pmvd[1] = MEDIAN( mvd_U[1], mvd_L[1], mvd_C[1]);
	}

}

void PskipMV_COL(codingUnit *currMB, unsigned int uiPositionInPic, int block8_in_row, int block8_in_col, int blockshape_pix_x, int blockshape_pix_y )
{
	//	int bx, by;
	int mb_nr = uiPositionInPic;
	int mb_width = img->width / MIN_CU_SIZE;
	int mb_x = mb_nr % mb_width;
	int mb_y = mb_nr / mb_width;
	int pic_block8_x = mb_x << (MIN_CU_SIZE_IN_BIT -MIN_BLOCK_SIZE_IN_BIT); //qyu 0830
	int pic_block8_y = mb_y << (MIN_CU_SIZE_IN_BIT -MIN_BLOCK_SIZE_IN_BIT);
	int pic_pix_x = mb_x << ( MIN_CU_SIZE_IN_BIT );
	int pic_pix_y = mb_y << ( MIN_CU_SIZE_IN_BIT );
	int blockshape_block_x = blockshape_pix_x >> MIN_BLOCK_SIZE_IN_BIT;
	int blockshape_block_y = blockshape_pix_y >> MIN_BLOCK_SIZE_IN_BIT;
	int blockshape_mb_x = blockshape_pix_x >> MIN_CU_SIZE_IN_BIT;
	int blockshape_mb_y = blockshape_pix_y >> MIN_CU_SIZE_IN_BIT;
	int **cur_ref = refFrArr;
	int **col_ref = refbuf[0];
	int ***cur_mv = img->tmp_mv;
	int ***col_mv = mvbuf[0];
	int *col_pic_dist = ref_poc[0];
	int refframe;
	int curT, colT, neiT;
	int cur_mv_scaled[2], col_mv_scaled[2];
	int i, j, l, m;
#if HALF_PIXEL_PSKIP
  int delta, delta2;
  delta = delta2 = 0;
#endif

	for ( i = 0; i < 2; i++)
	{
		for ( j = 0; j < 2; j++)
		{
			int block_x = pic_block8_x + blockshape_block_x / 2 * i ;
			int block_y = pic_block8_y + blockshape_block_y / 2 * j ;
			refframe =col_ref[block_y ][block_x];
			if (refframe >= 0)
			{
				curT = ( 2 * ( picture_distance - img->imgtr_fwRefDistance[0] ) + 512 ) % 512;
				colT = ( 2 * ( img->imgtr_fwRefDistance[0] - col_pic_dist[refframe]) + 512 ) % 512;
#if AVS2_SCENE_MV
				if (0 == curr_RPS.num_of_ref - 1 && background_reference_enable){
					curT = 1;
					colT = 1;
				}
				if (refframe == curr_RPS.num_of_ref - 1 && background_reference_enable){
					colT = 1;
				}
#endif
#if HALF_PIXEL_PSKIP
        if (img->is_field_sequence)
        {
          int oriPOC, oriRefPOC, scaledPOC, scaledRefPOC;
          oriPOC = 2*img->imgtr_fwRefDistance[0];
          oriRefPOC = oriPOC - colT;
          scaledPOC = 2*picture_distance;
          scaledRefPOC = 2*img->imgtr_fwRefDistance[0];
          getDeltas( &delta, &delta2, oriPOC, oriRefPOC, scaledPOC, scaledRefPOC );
        }
        scalingMV(&cur_mv[block_y ][block_x ][0],&cur_mv[block_y ][block_x][1],curT, col_mv[block_y][block_x][0],col_mv[block_y][block_x][1]+delta,colT, 1 );
        cur_mv[block_y ][block_x][1] -= delta2;
#else
				scalingMV(&cur_mv[block_y ][block_x ][0],&cur_mv[block_y ][block_x][1],curT, col_mv[block_y][block_x][0],col_mv[block_y][block_x][1],colT, 1 );
#endif
#if m3469
        cur_mv[block_y ][block_x ][0] = Clip3(-32768, 32767, cur_mv[block_y ][block_x ][0]);
        cur_mv[block_y ][block_x ][1] = Clip3(-32768, 32767, cur_mv[block_y ][block_x ][1]);
#endif
			}
			else
			{			
				cur_mv[block_y ][block_x ][0] =0;
				cur_mv[block_y ][block_x ][1] =0;
			}

			for ( l = 0; l < blockshape_block_x / 2; l++)
			{
				for ( m = 0; m < blockshape_block_y / 2; m++)
				{
					cur_mv[block_y + m][block_x + l][0] = cur_mv[block_y ][block_x ][0] ;
					cur_mv[block_y + m][block_x + l][1] = cur_mv[block_y ][block_x ][1] ;
				}
			}

		}
	}
#if OFF_4X4PU
	if ( currMB->ui_MbBitSize==MIN_CU_SIZE_IN_BIT)
	{

		for ( i = 0; i < 2; i++)
		{
			for ( j = 0; j < 2; j++)
			{

				int block_x = pic_block8_x + blockshape_block_x / 2 * i ;
				int block_y = pic_block8_y + blockshape_block_y / 2 * j ;

				cur_mv[block_y ][block_x ][0] =cur_mv[pic_block8_y ][pic_block8_x ][0] ;
				cur_mv[block_y ][block_x ][1] =cur_mv[pic_block8_y ][pic_block8_x ][1] ;
			

			 for ( l = 0; l < blockshape_block_x / 2; l++)
			 {
				for ( m = 0; m < blockshape_block_y / 2; m++)
				{
					cur_mv[block_y + m][block_x + l][0] = cur_mv[block_y ][block_x ][0] ;
					cur_mv[block_y + m][block_x + l][1] = cur_mv[block_y ][block_x ][1] ;
				}
			 }
			}
		}
	}
#endif 
}


void setPSkipMotionVector( unsigned int uiBitSize, unsigned int uiPositionInPic, int  ref_frame, int  mb_pix_x, int  mb_pix_y, int  blockshape_x, int  blockshape_y, int  direct_mv )
{
	int mb_nr                = uiPositionInPic;
	int mb_width             = img->width / MIN_CU_SIZE;
	int mb_x = mb_nr % mb_width;
	int mb_y = mb_nr / mb_width;
#if !M3476_BUF_FIX
	int mb_pix_x_temp = mb_pix_x % 8 == 0 ? mb_pix_x : 8;
	int mb_pix_y_temp = mb_pix_y % 8 == 0 ? mb_pix_y : 8;
#else 
	int mb_pix_x_temp = mb_pix_x % MIN_BLOCK_SIZE == 0 ? mb_pix_x : MIN_BLOCK_SIZE;
	int mb_pix_y_temp = mb_pix_y % MIN_BLOCK_SIZE == 0 ? mb_pix_y : MIN_BLOCK_SIZE;
#endif 
#if !M3476_BUF_FIX
	int pic_block_x          = ( mb_x << 1 ) + ( mb_pix_x_temp >> 3 );
	int pic_block_y          = ( mb_y << 1 ) + ( mb_pix_y_temp >> 3 );
#else 
	int pic_block_x          = ( mb_x << 1 ) + ( mb_pix_x_temp >> MIN_BLOCK_SIZE_IN_BIT );
	int pic_block_y          = ( mb_y << 1 ) + ( mb_pix_y_temp >> MIN_BLOCK_SIZE_IN_BIT );
#endif 

	int mb_available_up;
	int mb_available_left;
	int mb_available_upleft;
	int mb_available_upright;
	int block_available_left1, block_available_up1;
	int block_available_up, block_available_left, block_available_upright, block_available_upleft;
	int rFrameL[2], rFrameU[2], rFrameUR[2],rFrameUL[2],rFrameL1[2], rFrameU1[2];
	int hv;
	int mva[2][2] , mvb[2][2], mvc[2][2];
	int mva1[2][2], mvb1[2][2], mve[2][2];
	int y_up = 1, y_upright = 1, y_upleft = 1, off_y = 0;
	int upright;
	int mb_nb_L,mb_nb_L1, mb_nb_U,mb_nb_U1,mb_nb_UR,mb_nb_UL;
	int pRefFrame[2][6];
	int pmv[2][2][6];
	int i,j,dir;
	int bid_flag=0, bw_flag=0, fw_flag=0, sym_flag=0,bid2=0, fw2 = 0;


#if !M3476_BUF_FIX
	blockshape_x = blockshape_x % 8 == 0 ? blockshape_x : 8;
	blockshape_y = blockshape_y % 8 == 0 ? blockshape_y : 8;
#else 
	blockshape_x = blockshape_x % MIN_BLOCK_SIZE == 0 ? blockshape_x : MIN_BLOCK_SIZE;
	blockshape_y = blockshape_y % MIN_BLOCK_SIZE == 0 ? blockshape_y : MIN_BLOCK_SIZE;
#endif 

	if ( input->slice_set_enable ) 
	{
		mb_available_up      = ( mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr - mb_width  ].slice_set_index );
		mb_available_left    = ( mb_x == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr - 1         ].slice_set_index );
		mb_available_upleft  = ( mb_x == 0 || mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr - mb_width - 1].slice_set_index );
		mb_available_upright = ( mb_x >= mb_width - ( 1 << ( uiBitSize - 4 ) ) || mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr - mb_width + ( 1 << ( uiBitSize - 4 ) ) ].slice_set_index );
	}
	else
	{
		mb_available_up      = ( mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr - mb_width  ].slice_nr );      
		mb_available_left    = ( mb_x == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr - 1         ].slice_nr );      
		mb_available_upleft  = ( mb_x == 0 || mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr - mb_width - 1].slice_nr );

#if !M3476_BUF_FIX
		if ( ( pic_block_y == 0 ) || ( ( pic_block_x << 3 ) + blockshape_x == img->width ) )
#else 
		if ( ( pic_block_y == 0 ) || ( ( pic_block_x << MIN_BLOCK_SIZE_IN_BIT ) + blockshape_x == img->width ) )
#endif 
		{
			mb_available_upright = 1;
		}
		else if ( mb_pix_y > 0 )
		{
			mb_available_upright = 0;
		}
		else
		{
			mb_available_upright = ( img->mb_data[uiPositionInPic].slice_nr != img->mb_data[uiPositionInPic - img->PicWidthInMbs + ( mb_pix_x_temp + blockshape_x ) / MIN_CU_SIZE].slice_nr );
		}
	}

	block_available_up   = mb_available_up   || ( mb_pix_y > 0 );
	block_available_left = mb_available_left || ( mb_pix_x > 0 );
#if !M3476_BUF_FIX
	block_available_left1 = block_available_left && (blockshape_y > 8);
	block_available_up1 = block_available_up && (blockshape_x > 8);
#else
	block_available_left1 = block_available_left && (blockshape_y > MIN_BLOCK_SIZE);
	block_available_up1 = block_available_up && (blockshape_x > MIN_BLOCK_SIZE);
#endif 

	if ( input->g_uiMaxSizeInBit == B64X64_IN_BIT )
	{
		upright = g_Up_Right_Avail_Matrix64[pic_block_y - img->block8_y][pic_block_x - img->block8_x + blockshape_x / MIN_BLOCK_SIZE - 1];
	}
	else if ( input->g_uiMaxSizeInBit == B32X32_IN_BIT )
	{
		upright = g_Up_Right_Avail_Matrix32[pic_block_y - img->block8_y][pic_block_x - img->block8_x + blockshape_x / MIN_BLOCK_SIZE - 1];
	}
	else if ( input->g_uiMaxSizeInBit == B16X16_IN_BIT )
	{
		upright = g_Up_Right_Avail_Matrix16[pic_block_y - img->block8_y][pic_block_x - img->block8_x + blockshape_x / MIN_BLOCK_SIZE - 1];
	}

	if ( ( pic_block_x << MIN_BLOCK_SIZE_IN_BIT ) + blockshape_x >= img->width )
	{
		upright = 0;
	}

	block_available_upright = upright && ( !mb_available_upright );

	if ( mb_pix_x > 0 )
	{
		block_available_upleft = ( mb_pix_y > 0 ? 1 : block_available_up );
	}
	else if ( mb_pix_y > 0 )
	{
		block_available_upleft = block_available_left;
	}
	else
	{
		block_available_upleft = mb_available_upleft;
	}
#if !M3476_BUF_FIX
	mb_nb_L1 = mb_width * ((pic_block_y - off_y + blockshape_y / 8 - 1) >> 1) + ((pic_block_x - 1)>>1);
	mb_nb_L = mb_width * ((pic_block_y  - off_y) >> 1) + ((pic_block_x - 1)>>1);
	mb_nb_U = mb_width * ((pic_block_y - y_up) >> 1) + ((pic_block_x)>>1);
	mb_nb_U1 = mb_width * ((pic_block_y - y_up) >> 1) + ((pic_block_x + blockshape_x / 8 -1)>>1);
	mb_nb_UR = mb_width * ((pic_block_y - y_upright)>>1) + ((pic_block_x + blockshape_x / 8 )>>1);
	mb_nb_UL = mb_width * ((pic_block_y - y_upleft)>>1) + ((pic_block_x - 1)>>1);

	rFrameL[0]    = block_available_left    ? refFrArr[pic_block_y  - off_y]  [pic_block_x - 1] : -1;
	rFrameU[0]    = block_available_up      ? refFrArr[pic_block_y - y_up][pic_block_x]   : -1;
	rFrameUR[0]   = block_available_upright ? refFrArr[pic_block_y - y_upright][pic_block_x + ( blockshape_x / 8 ) ] :  -1;
	rFrameUL[0]   = block_available_upleft  ? refFrArr[pic_block_y - y_upleft][pic_block_x - 1] : -1;
	rFrameL1[0] = block_available_left1 ? refFrArr[pic_block_y - off_y + blockshape_y / 8 - 1][pic_block_x - 1] : -1;  
	rFrameU1[0] = block_available_up1 ? refFrArr[pic_block_y - y_up][pic_block_x + blockshape_x / 8 -1] : -1;

	rFrameL[1]    = block_available_left    ? p_snd_refFrArr[pic_block_y  - off_y]  [pic_block_x - 1] : -1;
	rFrameU[1]    = block_available_up      ? p_snd_refFrArr[pic_block_y - y_up][pic_block_x]   : -1;
	rFrameUR[1]   = block_available_upright ? p_snd_refFrArr[pic_block_y - y_upright][pic_block_x + ( blockshape_x / 8 ) ] :  -1;
	rFrameUL[1]   = block_available_upleft  ? p_snd_refFrArr[pic_block_y - y_upleft][pic_block_x - 1] : -1;
	rFrameL1[1] = block_available_left1 ? p_snd_refFrArr[pic_block_y - off_y + blockshape_y / 8 - 1][pic_block_x - 1] : -1;
	rFrameU1[1] = block_available_up1 ? p_snd_refFrArr[pic_block_y - y_up][pic_block_x + blockshape_x / 8 -1] : -1;
#else 
	mb_nb_L1 = mb_width * ((pic_block_y - off_y + blockshape_y / MIN_BLOCK_SIZE - 1) >> 1) + ((pic_block_x - 1)>>1);
	mb_nb_L = mb_width * ((pic_block_y  - off_y) >> 1) + ((pic_block_x - 1)>>1);
	mb_nb_U = mb_width * ((pic_block_y - y_up) >> 1) + ((pic_block_x)>>1);
	mb_nb_U1 = mb_width * ((pic_block_y - y_up) >> 1) + ((pic_block_x + blockshape_x / MIN_BLOCK_SIZE -1)>>1);
	mb_nb_UR = mb_width * ((pic_block_y - y_upright)>>1) + ((pic_block_x + blockshape_x / MIN_BLOCK_SIZE )>>1);
	mb_nb_UL = mb_width * ((pic_block_y - y_upleft)>>1) + ((pic_block_x - 1)>>1);

	rFrameL[0]    = block_available_left    ? refFrArr[pic_block_y  - off_y]  [pic_block_x - 1] : -1;
	rFrameU[0]    = block_available_up      ? refFrArr[pic_block_y - y_up][pic_block_x]   : -1;
	rFrameUR[0]   = block_available_upright ? refFrArr[pic_block_y - y_upright][pic_block_x + ( blockshape_x / MIN_BLOCK_SIZE ) ] :  -1;
	rFrameUL[0]   = block_available_upleft  ? refFrArr[pic_block_y - y_upleft][pic_block_x - 1] : -1;
	rFrameL1[0] = block_available_left1 ? refFrArr[pic_block_y - off_y + blockshape_y / MIN_BLOCK_SIZE - 1][pic_block_x - 1] : -1;  
	rFrameU1[0] = block_available_up1 ? refFrArr[pic_block_y - y_up][pic_block_x + blockshape_x /MIN_BLOCK_SIZE -1] : -1;

	rFrameL[1]    = block_available_left    ? p_snd_refFrArr[pic_block_y  - off_y]  [pic_block_x - 1] : -1;
	rFrameU[1]    = block_available_up      ? p_snd_refFrArr[pic_block_y - y_up][pic_block_x]   : -1;
	rFrameUR[1]   = block_available_upright ? p_snd_refFrArr[pic_block_y - y_upright][pic_block_x + ( blockshape_x / MIN_BLOCK_SIZE ) ] :  -1;
	rFrameUL[1]   = block_available_upleft  ? p_snd_refFrArr[pic_block_y - y_upleft][pic_block_x - 1] : -1;
	rFrameL1[1] = block_available_left1 ? p_snd_refFrArr[pic_block_y - off_y + blockshape_y / MIN_BLOCK_SIZE - 1][pic_block_x - 1] : -1;
	rFrameU1[1] = block_available_up1 ? p_snd_refFrArr[pic_block_y - y_up][pic_block_x + blockshape_x / MIN_BLOCK_SIZE -1] : -1;
#endif 

	for(i=0; i<2; i++)
	{
		pRefFrame[i][0] = rFrameUL[i];
		pRefFrame[i][1] = rFrameU[i];
		pRefFrame[i][2] = rFrameL[i];
		pRefFrame[i][3] = rFrameUR[i];
		pRefFrame[i][4] = rFrameU1[i];
		pRefFrame[i][5] = rFrameL1[i];
	}
	for ( hv = 0; hv < 2; hv++ )
	{     
#if !M3476_BUF_FIX
		mva[0][hv]  = block_available_left    ? img->tmp_mv[pic_block_y - off_y][pic_block_x - 1][hv]              : 0;
		mva1[0][hv]  = block_available_left1 ? img->tmp_mv[pic_block_y - off_y + blockshape_y / 8 - 1][pic_block_x - 1][hv] : 0;
		mvb[0][hv] = block_available_up      ? img->tmp_mv[pic_block_y - y_up][pic_block_x][hv]                : 0;
		mvb1[0][hv]  = block_available_up1 ? img->tmp_mv[pic_block_y - y_up][pic_block_x + blockshape_x / 8 -1][hv] : 0;
		mve[0][hv] = block_available_upleft  ? img->tmp_mv[pic_block_y - y_upleft][pic_block_x - 1][hv]              : 0;
		mvc[0][hv]  = block_available_upright ? img->tmp_mv[pic_block_y - y_upright][pic_block_x + ( blockshape_x / 8 ) ][hv] : 0;

		mva[1][hv]  = block_available_left    ? img->p_snd_tmp_mv[pic_block_y - off_y][pic_block_x - 1][hv]              : 0;
		mva1[1][hv]  = block_available_left1 ? img->p_snd_tmp_mv[pic_block_y - off_y + blockshape_y / 8 - 1][pic_block_x - 1][hv] : 0;
		mvb[1][hv] = block_available_up      ? img->p_snd_tmp_mv[pic_block_y - y_up][pic_block_x][hv]                : 0;
		mvb1[1][hv]  = block_available_up1 ? img->p_snd_tmp_mv[pic_block_y - y_up][pic_block_x + blockshape_x / 8 -1][hv] : 0;
		mve[1][hv] = block_available_upleft  ? img->p_snd_tmp_mv[pic_block_y - y_upleft][pic_block_x - 1][hv]             : 0;
		mvc[1][hv]  = block_available_upright ? img->p_snd_tmp_mv[pic_block_y - y_upright][pic_block_x + ( blockshape_x / 8 ) ][hv] : 0;
#else 
		mva[0][hv]  = block_available_left    ? img->tmp_mv[pic_block_y - off_y][pic_block_x - 1][hv]              : 0;
		mva1[0][hv]  = block_available_left1 ? img->tmp_mv[pic_block_y - off_y + blockshape_y / MIN_BLOCK_SIZE - 1][pic_block_x - 1][hv] : 0;
		mvb[0][hv] = block_available_up      ? img->tmp_mv[pic_block_y - y_up][pic_block_x][hv]                : 0;
		mvb1[0][hv]  = block_available_up1 ? img->tmp_mv[pic_block_y - y_up][pic_block_x + blockshape_x / MIN_BLOCK_SIZE -1][hv] : 0;
		mve[0][hv] = block_available_upleft  ? img->tmp_mv[pic_block_y - y_upleft][pic_block_x - 1][hv]              : 0;
		mvc[0][hv]  = block_available_upright ? img->tmp_mv[pic_block_y - y_upright][pic_block_x + ( blockshape_x / MIN_BLOCK_SIZE ) ][hv] : 0;

		mva[1][hv]  = block_available_left    ? img->p_snd_tmp_mv[pic_block_y - off_y][pic_block_x - 1][hv]              : 0;
		mva1[1][hv]  = block_available_left1 ? img->p_snd_tmp_mv[pic_block_y - off_y + blockshape_y / MIN_BLOCK_SIZE - 1][pic_block_x - 1][hv] : 0;
		mvb[1][hv] = block_available_up      ? img->p_snd_tmp_mv[pic_block_y - y_up][pic_block_x][hv]                : 0;
		mvb1[1][hv]  = block_available_up1 ? img->p_snd_tmp_mv[pic_block_y - y_up][pic_block_x + blockshape_x / MIN_BLOCK_SIZE -1][hv] : 0;
		mve[1][hv] = block_available_upleft  ? img->p_snd_tmp_mv[pic_block_y - y_upleft][pic_block_x - 1][hv]             : 0;
		mvc[1][hv]  = block_available_upright ? img->p_snd_tmp_mv[pic_block_y - y_upright][pic_block_x + ( blockshape_x / MIN_BLOCK_SIZE ) ][hv] : 0;
#endif 

		for(i = 0; i < 2; i++)
		{
			pmv[i][hv][0] = mve[i][hv];   //i:0-first;1-second, hv-x or y
			pmv[i][hv][1] = mvb[i][hv];
			pmv[i][hv][2] = mva[i][hv];
			pmv[i][hv][3] = mvc[i][hv];
			pmv[i][hv][4] = mvb1[i][hv];
			pmv[i][hv][5] = mva1[i][hv];
		}
	}

	for(dir = 0; dir < MH_PSKIP_NUM+NUM_OFFSET+1; dir++)
	{
		for(i = 0; i < 3; i++)
		{
			img->tmp_fstPSkipMv[dir][i] = 0;
			img->tmp_sndPSkipMv[dir][i] = 0;
		}
		img->tmp_pref_fst[dir] = 0;
		img->tmp_pref_snd[dir] = 0;
	}

	for(j = 0; j < 6; j++)
	{
		// bid
		if( pRefFrame[0][j] != -1 && pRefFrame[1][j] != -1 )
		{
			img->tmp_pref_fst[BID_P_FST] = pRefFrame[0][j];
			img->tmp_pref_snd[BID_P_FST] = pRefFrame[1][j];
			img->tmp_fstPSkipMv[BID_P_FST][0] = pmv[0][0][j];
			img->tmp_fstPSkipMv[BID_P_FST][1] = pmv[0][1][j];
			img->tmp_sndPSkipMv[BID_P_FST][0] = pmv[1][0][j];
			img->tmp_sndPSkipMv[BID_P_FST][1] = pmv[1][1][j];
			bid_flag++;
			if(bid_flag == 1){
				bid2 = j;
			}
		}
		// fw
		else if(pRefFrame[0][j] != -1 && pRefFrame[1][j] == -1)
		{
			img->tmp_pref_fst[FW_P_FST] = pRefFrame[0][j];
			img->tmp_fstPSkipMv[FW_P_FST][0] = pmv[0][0][j];
			img->tmp_fstPSkipMv[FW_P_FST][1] = pmv[0][1][j];
			fw_flag++;
			if(fw_flag == 1){
				fw2 = j;
			}
		}
	}

	//first bid
	if(bid_flag == 0 && fw_flag > 1)
	{
		img->tmp_pref_fst[BID_P_FST] = img->tmp_pref_fst[FW_P_FST];
		img->tmp_pref_snd[BID_P_FST] = pRefFrame[0][fw2];
		img->tmp_fstPSkipMv[BID_P_FST][0] = img->tmp_fstPSkipMv[FW_P_FST][0];
        img->tmp_fstPSkipMv[BID_P_FST][1] = img->tmp_fstPSkipMv[FW_P_FST][1];
		img->tmp_sndPSkipMv[BID_P_FST][0] = pmv[0][0][fw2];
		img->tmp_sndPSkipMv[BID_P_FST][1] = pmv[0][1][fw2];
	}

	//second bid
	if( bid_flag > 1)
	{
		img->tmp_pref_fst[BID_P_SND] = pRefFrame[0][bid2];
		img->tmp_pref_snd[BID_P_SND] = pRefFrame[1][bid2];
		img->tmp_fstPSkipMv[BID_P_SND][0] = pmv[0][0][bid2];
		img->tmp_fstPSkipMv[BID_P_SND][1] = pmv[0][1][bid2];
		img->tmp_sndPSkipMv[BID_P_SND][0] = pmv[1][0][bid2];
		img->tmp_sndPSkipMv[BID_P_SND][1] = pmv[1][1][bid2];
	}
	else if(bid_flag == 1 && fw_flag > 1)
	{
		img->tmp_pref_fst[BID_P_SND] = img->tmp_pref_fst[FW_P_FST];
		img->tmp_pref_snd[BID_P_SND] = pRefFrame[0][fw2];
		img->tmp_fstPSkipMv[BID_P_SND][0] = img->tmp_fstPSkipMv[FW_P_FST][0];
		img->tmp_fstPSkipMv[BID_P_SND][1] = img->tmp_fstPSkipMv[FW_P_FST][1];
		img->tmp_sndPSkipMv[BID_P_SND][0] = pmv[0][0][fw2];
		img->tmp_sndPSkipMv[BID_P_SND][1] = pmv[0][1][fw2];
	}

	//first fw
	if(fw_flag == 0 && bid_flag > 1)
	{
		img->tmp_pref_fst[FW_P_FST] = pRefFrame[0][bid2];
		img->tmp_fstPSkipMv[FW_P_FST][0] = pmv[0][0][bid2];
		img->tmp_fstPSkipMv[FW_P_FST][1] = pmv[0][1][bid2];
	}
	else if(fw_flag == 0 && bid_flag == 1)
	{
		img->tmp_pref_fst[FW_P_FST] = img->tmp_pref_fst[BID_P_FST];
		img->tmp_fstPSkipMv[FW_P_FST][0] = img->tmp_fstPSkipMv[BID_P_FST][0];
		img->tmp_fstPSkipMv[FW_P_FST][1] = img->tmp_fstPSkipMv[BID_P_FST][1];
	}

	// second fw
	if(fw_flag > 1)
	{
		img->tmp_pref_fst[FW_P_SND] = pRefFrame[0][fw2];
		img->tmp_fstPSkipMv[FW_P_SND][0] = pmv[0][0][fw2];
		img->tmp_fstPSkipMv[FW_P_SND][1] = pmv[0][1][fw2];
	}
	else if( bid_flag > 1)
	{
		img->tmp_pref_fst[FW_P_SND] = pRefFrame[1][bid2];
		img->tmp_fstPSkipMv[FW_P_SND][0] = pmv[1][0][bid2];
		img->tmp_fstPSkipMv[FW_P_SND][1] = pmv[1][1][bid2];
	}
	else if(bid_flag == 1)
	{
		img->tmp_pref_fst[FW_P_SND] = img->tmp_pref_snd[BID_P_FST];
		img->tmp_fstPSkipMv[FW_P_SND][0] = img->tmp_sndPSkipMv[BID_P_FST][0];
		img->tmp_fstPSkipMv[FW_P_SND][1] = img->tmp_sndPSkipMv[BID_P_FST][1];
	}
}

/*
*************************************************************************
* Function:Interpret the mb mode for P-Frames
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
void interpret_mb_mode_PF ( unsigned int uiPositionInPic, int pdir )
{
	int pdir0[4] = {FORWARD, FORWARD, DUAL, DUAL};
	int pdir1[4] = {FORWARD, DUAL, FORWARD, DUAL};
  int i, j, k;

  codingUnit *currMB = &img->mb_data[uiPositionInPic];//GB current_mb_nr];
  int         mbmode = currMB->cuType;
  int N8_SizeScale = 1 << ( currMB->ui_MbBitSize - MIN_CU_SIZE_IN_BIT );

  int num_of_orgMB_in_row = N8_SizeScale;//4:1  5:2  6:4
  int num_of_orgMB_in_col = N8_SizeScale;
  int size = 1 << currMB->ui_MbBitSize;
  int pix_x = ( uiPositionInPic % img->PicWidthInMbs ) << MIN_CU_SIZE_IN_BIT;
  int pix_y = ( uiPositionInPic / img->PicWidthInMbs ) << MIN_CU_SIZE_IN_BIT;

  if ( pix_x + size >= img->width )
  {
    num_of_orgMB_in_col = min ( N8_SizeScale, ( img->width - pix_x ) >> MIN_CU_SIZE_IN_BIT );
  }

  if ( pix_y + size >= img->height )
  {
    num_of_orgMB_in_row = min ( N8_SizeScale, ( img->height - pix_y ) >> MIN_CU_SIZE_IN_BIT );
  }
  if ( mbmode == 0 )   
  {
	  for ( i = 0; i < 4; i++ )
	  {
		  currMB->b8mode[i] = mbmode;
		  currMB->b8pdir[i] = FORWARD;
	  }
  }
  else if ( mbmode == 1 ) // 16x16
  {
	  for ( i = 0; i < 4; i++ )
	  {
		  currMB->b8mode[i] = mbmode;
		  currMB->b8pdir[i] = pdir == 0 ? FORWARD : DUAL;
	  }
  }
  else if ( mbmode == 2 || mbmode == 4 || mbmode == 5 ) // 16x8, 16x4, 16x12
  {
	  for ( i = 0; i < 4; i++ )
	  {
		  currMB->b8mode[i] = mbmode;
	  }
	  currMB->b8pdir[0] = currMB->b8pdir[1] = pdir0 [pdir];
	  currMB->b8pdir[2] = currMB->b8pdir[3] = pdir1 [pdir];
  }
  else if ( mbmode == 3 || mbmode == 6 || mbmode == 7 )
  {
	  for ( i = 0; i < 4; i++ )
	  {
		  currMB->b8mode[i] = mbmode;
	  }
	  currMB->b8pdir[0] = currMB->b8pdir[2] = pdir0[pdir];
	  currMB->b8pdir[1] = currMB->b8pdir[3] = pdir1[pdir];
  }
  else if ( mbmode >= 9 ) //modefy by xfwang 2004.7.29
  {
    currMB->cbp = NCBP[currMB->cuType - 9][0]; // qhg  //modefy by xfwang 2004.7.29
#if !CU_TYPE_BINARIZATION
	currMB->cuType = I8MB;
#endif

    for ( i = 0; i < 4; i++ )
    {
      currMB->b8mode[i] = IBLOCK;
      currMB->b8pdir[i] = INTRA;
    }
  }

  for ( i = 0; i < num_of_orgMB_in_row; i++ )
  {
    int pos = uiPositionInPic + i * img->PicWidthInMbs;

    for ( j = 0; j < num_of_orgMB_in_col; j++, pos++ )
    {
      codingUnit* tmpMB = &img->mb_data[pos];
      tmpMB->cuType = currMB->cuType;
      tmpMB->cbp = currMB->cbp;

      for ( k = 0; k < 4; k++ )
      {
        tmpMB->b8mode[k] = currMB->b8mode[k];
        tmpMB->b8pdir[k] = currMB->b8pdir[k];
      }
    }
  }
}

/*
*************************************************************************
* Function:Interpret the mb mode for I-Frames
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void interpret_mb_mode_I ( unsigned int uiPositionInPic )
{
  int i, j, k;
  codingUnit *currMB   = &img->mb_data[uiPositionInPic];
  int         num      = 4;
  int N8_SizeScale = 1 << ( currMB->ui_MbBitSize - MIN_CU_SIZE_IN_BIT );
  int num_of_orgMB_in_row = N8_SizeScale;//4:1  5:2  6:4
  int num_of_orgMB_in_col = N8_SizeScale;
  int size = 1 << currMB->ui_MbBitSize;
  int pix_x = ( uiPositionInPic % img->PicWidthInMbs ) << MIN_CU_SIZE_IN_BIT;
  int pix_y = ( uiPositionInPic / img->PicWidthInMbs ) << MIN_CU_SIZE_IN_BIT;

  if ( pix_x + size >= img->width )
  {
    num_of_orgMB_in_col = min ( N8_SizeScale, ( img->width - pix_x ) >> MIN_CU_SIZE_IN_BIT );
  }

  if ( pix_y + size >= img->height )
  {
    num_of_orgMB_in_row = min ( N8_SizeScale, ( img->height - pix_y ) >> MIN_CU_SIZE_IN_BIT );
  }
#if !CU_TYPE_BINARIZATION
  currMB->cuType = I8MB;
#endif


  for ( i = 0; i < 4; i++ )
  {
    currMB->b8mode[i] = IBLOCK;
    currMB->b8pdir[i] = INTRA;
  }

  for ( i = num; i < 4; i++ )
  {
    currMB->b8mode[i] = 0/*currMB->cuType_2==PNXN? 4 : currMB->cuType_2*/;
    currMB->b8pdir[i] = FORWARD;
  }

  for ( i = 0; i < num_of_orgMB_in_row; i++ )
  {
    int pos = uiPositionInPic + i * img->PicWidthInMbs;

    for ( j = 0; j < num_of_orgMB_in_col; j++, pos++ )
    {
      codingUnit* tmpMB = &img->mb_data[pos];
      tmpMB->cuType = currMB->cuType;

      for ( k = 0; k < 4; k++ )
      {
        tmpMB->b8mode[k] = currMB->b8mode[k];
        tmpMB->b8pdir[k] = currMB->b8pdir[k];
      }
    }
  }
}

/*
*************************************************************************
* Function:Interpret the mb mode for B-Frames
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void interpret_mb_mode_B ( unsigned int uiPositionInPic, int pdir )
{

  int pdir0[16] = {FORWARD, BACKWARD, FORWARD, BACKWARD, FORWARD, BACKWARD, SYM, SYM, SYM, FORWARD, BACKWARD, SYM, BID, BID, BID, BID};
  int pdir1[16] = {FORWARD, BACKWARD, BACKWARD, FORWARD, SYM, SYM, FORWARD, BACKWARD, SYM, BID, BID, BID, FORWARD, BACKWARD, SYM, BID};


  codingUnit *currMB = &img->mb_data[uiPositionInPic];

  int i, mbmode, j, k;
  int mbtype  = currMB->cuType;
  int *b8mode = currMB->b8mode;
  int *b8pdir = currMB->b8pdir;
  int N8_SizeScale = 1 << ( currMB->ui_MbBitSize - MIN_CU_SIZE_IN_BIT );

  int num_of_orgMB_in_row = N8_SizeScale;//4:1  5:2  6:4
  int num_of_orgMB_in_col = N8_SizeScale;
  int size = 1 << currMB->ui_MbBitSize;
  int pix_x = ( uiPositionInPic % img->PicWidthInMbs ) << MIN_CU_SIZE_IN_BIT;
  int pix_y = ( uiPositionInPic / img->PicWidthInMbs ) << MIN_CU_SIZE_IN_BIT;

  if ( pix_x + size >= img->width )
  {
    num_of_orgMB_in_col = min ( N8_SizeScale, ( img->width - pix_x ) >> MIN_CU_SIZE_IN_BIT );
  }

  if ( pix_y + size >= img->height )
  {
    num_of_orgMB_in_row = min ( N8_SizeScale, ( img->height - pix_y ) >> MIN_CU_SIZE_IN_BIT );
  }

  //--- set mbtype, b8type, and b8pdir ---
  if ( mbtype == 0 )   // direct
  {
    mbmode = 0;
	switch(currMB->md_directskip_mode)
	{
	case 0:
	case DS_SYM:
		for ( i = 0; i < 4; i++ )
		{
			b8mode[i] = 0;
			b8pdir[i] = SYM;
		}
		break;
	case DS_BID:
		for ( i = 0; i < 4; i++ )
		{
			b8mode[i] = 0;
			b8pdir[i] = BID;
		}
		break;
	case DS_BACKWARD:
		for ( i = 0; i < 4; i++ )
		{
			b8mode[i] = 0;
			b8pdir[i] = BACKWARD;
		}
		break;
	case DS_FORWARD:
		for ( i = 0; i < 4; i++ )
		{
			b8mode[i] = 0;
			b8pdir[i] = FORWARD;
		}
		break;
	}
  }
  else if ( mbtype == 8 ) // intra8x8
  {
    mbmode = PNXN;     // b8mode and pdir is transmitted in additional codewords
  }
#if CU_TYPE_BINARIZATION
  else if ( mbtype == 9 || mbtype==10 ) // 8x8(+split)
#else
  else if ( mbtype == 9 ) // 8x8(+split)
#endif
  {
    currMB->cbp = NCBP[mbtype - 23][0]; // qhg
#if CU_TYPE_BINARIZATION
	mbmode = currMB->cuType;
#else
	mbmode = I8MB;
#endif

    for ( i = 0; i < 4; i++ )
    {
      b8mode[i] = IBLOCK;
      b8pdir[i] = INTRA;
    }
  }
  else if ( mbtype == 1 ) // 16x16
  {
    mbmode = 1;

    for ( i = 0; i < 4; i++ )
    {
      b8mode[i] = 1;
      b8pdir[i] = pdir;
    }
  }
  else if ( mbtype == 2 || mbtype == 4 || mbtype == 5 ) // 16x8, 16x4, 16x12
  {
    //mbmode = 2;
    mbmode = mbtype;

    for ( i = 0; i < 4; i++ )
    {
      b8mode[i] = mbmode;//2;
    }
    b8pdir[0] = b8pdir[1] = pdir0 [pdir];
    b8pdir[2] = b8pdir[3] = pdir1 [pdir];
  }
  else if ( mbtype == 3 || mbtype == 6 || mbtype == 7 )
  {
    //mbmode = 3;
    mbmode = mbtype;

    for ( i = 0; i < 4; i++ )
    {
      b8mode[i] = mbmode;//3;
    }
    b8pdir[0] = b8pdir[2] = pdir0[pdir];
    b8pdir[1] = b8pdir[3] = pdir1[pdir];
  }

  currMB->cuType = mbmode;

  for ( i = 0; i < num_of_orgMB_in_row; i++ )
  {
    int pos = uiPositionInPic + i * img->PicWidthInMbs;

    for ( j = 0; j < num_of_orgMB_in_col; j++, pos++ )
    {
      codingUnit* tmpMB = &img->mb_data[pos];
      tmpMB->cuType = currMB->cuType;

      for ( k = 0; k < 4; k++ )
      {
        tmpMB->b8mode[k] = currMB->b8mode[k];
        tmpMB->b8pdir[k] = currMB->b8pdir[k];
      }
    }
  }
}

/*
*************************************************************************
* Function:init codingUnit I and P frames
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
void init_codingUnit ( unsigned int uiPositionInPic )
{
  int i, j;

  int r, c;
  int row, col;
  int width, height;
  codingUnit *currMB = &img->mb_data[uiPositionInPic];//GB current_mb_nr];
  int N8_SizeScale = 1 << ( currMB->ui_MbBitSize - MIN_CU_SIZE_IN_BIT );
  int block8_y = ( uiPositionInPic / img->PicWidthInMbs ) << 1;
  int block8_x = ( uiPositionInPic % img->PicWidthInMbs ) << 1;
  int num_of_orgMB_in_row = N8_SizeScale;//4:1  5:2  6:4
  int num_of_orgMB_in_col = N8_SizeScale;
  int size = 1 << currMB->ui_MbBitSize;
  int pix_x = ( uiPositionInPic % img->PicWidthInMbs ) << MIN_CU_SIZE_IN_BIT;
  int pix_y = ( uiPositionInPic / img->PicWidthInMbs ) << MIN_CU_SIZE_IN_BIT;

  if ( pix_x + size >= img->width )
  {
    num_of_orgMB_in_col = min ( N8_SizeScale, ( img->width - pix_x ) >> MIN_CU_SIZE_IN_BIT );
  }

  if ( pix_y + size >= img->height )
  {
    num_of_orgMB_in_row = min ( N8_SizeScale, ( img->height - pix_y ) >> MIN_CU_SIZE_IN_BIT );
  }

  for ( i = 0; i < 2 * num_of_orgMB_in_col; i++ )
  {
    for ( j = 0; j < 2 * num_of_orgMB_in_row; j++ )
    {
      img->tmp_mv[block8_y + j][block8_x + i][0] = 0;
      img->tmp_mv[block8_y + j][block8_x + i][1] = 0;
      img->tmp_mv[block8_y + j][block8_x + i][3] = 0;
      img->p_snd_tmp_mv[block8_y + j][block8_x + i][0] = 0;
      img->p_snd_tmp_mv[block8_y + j][block8_x + i][1] = 0;
      img->p_snd_tmp_mv[block8_y + j][block8_x + i][3] = 0;
    }
  }

  for ( i = 0; i < 2 * num_of_orgMB_in_col; i++ )
  {
    for ( j = 0; j < 2 * num_of_orgMB_in_row; j++ )
    {
      img->ipredmode[block8_y + j + 1][block8_x + i + 1] = -1;    //by oliver 0512
    }
  }

  // Set the reference frame information for motion vector prediction
  if ( IS_INTRA ( currMB ) )
  {
    for ( i = 0; i < 2 * num_of_orgMB_in_col; i++ )
    {
      for ( j = 0; j < 2 * num_of_orgMB_in_row; j++ )
      {
        refFrArr[block8_y + j][block8_x + i] = -1;
        p_snd_refFrArr[block8_y + j][block8_x + i] = -1;
      }
    }
  }
  else if ( !IS_P8x8 ( currMB ) )
  {
    for ( j = 0; j < 2; j++ )
    {
      for ( i = 0; i < 2; i++ )
      {
        get_b8_offset( currMB->cuType, currMB->ui_MbBitSize, i , j , &col , &row, &width, &height );
        for ( r = 0; r < height; r++ )
        {
          for ( c = 0; c < width; c++ )
          {
            refFrArr[block8_y + row + r][block8_x + col + c] = 0;
            p_snd_refFrArr[block8_y + row + r][block8_x + col + c] = -1;
          }
        }
      }
    }
  }
  else
  {
    for ( j = 0; j < 2; j++ )
    {
      for ( i = 0; i < 2; i++ )
      {
        get_b8_offset( currMB->cuType, currMB->ui_MbBitSize, i , j , &col , &row, &width, &height );
        for ( r = 0; r < height; r++ )
        {
          for ( c = 0; c < width; c++ )
          {
            refFrArr[block8_y + row + r][block8_x + col + c] = ( currMB->b8mode[ ( i / N8_SizeScale ) + 2 * ( j / N8_SizeScale ) ] == IBLOCK ? -1 : 0 );
            p_snd_refFrArr[block8_y + row + r][block8_x + col + c] = -1;
          }
        }
      }
    }
  }
}

/*
*************************************************************************
* Function:Sets mode for 8x8 block
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void SetB8Mode ( codingUnit* currMB, int value, int i )
{
  static const int b_v2b8 [5] = {0, 8, 8, 8, 8};
  static const int b_v2pd [5] = {2, 0, 1, 2, 3};

	if( value >= 6)
	{
		value = (value >> 1) + (value & 0x0001);
	}


  if ( img->type == B_IMG )
  {
    currMB->b8mode[i]   = b_v2b8[value];
    currMB->b8pdir[i]   = b_v2pd[value];
  }
  else
  {
    currMB->b8mode[i]   = 8;
		currMB->b8pdir[i]   = value == 0 ? FORWARD : DUAL;
  }

}

/*
*************************************************************************
* Function:Get the syntax elements from the NAL
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

#if CU_TYPE_BINARIZATION
void read_MBHeader( codingUnit *currMB, unsigned int uiPositionInPic, int size, int num_of_orgMB_in_row, int num_of_orgMB_in_col, int *pdir, int *real_cuType )
{
	SyntaxElement currSE;
#if WEIGHTED_SKIP
	int weighted_skipmode1;
	int weighted_skipmode2;
#endif
	Slice *currSlice = img->currentSlice;
	DataPartition *dP;
	int i, j;


	currSE.type = SE_MBTYPE;
	currSE.mapping = linfo_ue;
	CheckAvailabilityOfNeighbors ( currMB, currMB->ui_MbBitSize, uiPositionInPic ); //need fix

#if INTRA_2N
	dP = & ( currSlice->partArr[0] );
	currSE.reading = readcuTypeInfo;
	dP->readSyntaxElement ( &currSE, dP, currMB, uiPositionInPic ); //check: first_mb_nr - LCU position, need current CU's position

	*real_cuType = currSE.value1;
	if ( currSE.value1 < 0 )
	{
		currSE.value1 = 0;
	}
#else
	if ( img->type == I_IMG )
	{
		currMB->cuType = 0;
	}
#endif
#if INTRA_2N
	for ( i = 0; i < num_of_orgMB_in_row; i++ )
	{
		int pos = uiPositionInPic + i * img->PicWidthInMbs;
		for ( j = 0; j < num_of_orgMB_in_col; j++, pos++ )
		{
			codingUnit *tmpMB = &img->mb_data[pos];
			tmpMB->cuType = currSE.value1;
			tmpMB->trans_size = currSE.value1==I8MB? 1:0;
		}
	}
#endif

	if ( img->type == B_IMG && ( currMB->cuType >= 1 && currMB->cuType <= 7 ) )
	{
		dP = & ( currSlice->partArr[0] );
		currSE.reading = readPdir;
		dP->readSyntaxElement ( &currSE, dP, currMB, uiPositionInPic ); //check: first_mb_nr - LCU position, need current CU's position

		*pdir = currSE.value1;
	}
#if WEIGHTED_SKIP
	if ( IS_P_SKIP ( currMB ) && img->num_of_references > 2 )
	{
		dP = & ( currSlice->partArr[0] );
#if WPM_ACE_OPT
        currSE.type = SE_WPM1;
#endif
		currSE.reading = readWPM;
		dP->readSyntaxElement ( &currSE, dP, currMB, uiPositionInPic ); //check: first_mb_nr - LCU position, need current CU's position

#if TRACE
		fprintf ( p_trace, "weighted_skipmode1 = %3d \n", currSE.value1 );
#endif
		weighted_skipmode1 = currSE.value1;
	}

	if ( IS_P_SKIP ( currMB ) && img->num_of_references > 3 )
	{
		dP = & ( currSlice->partArr[0] );
#if WPM_ACE_OPT
        currSE.type = SE_WPM2;
#endif
		currSE.reading = readWPM;
		dP->readSyntaxElement ( &currSE, dP, currMB, uiPositionInPic ); //check: first_mb_nr - LCU position, need current CU's position

#if TRACE
		fprintf ( p_trace, "weighted_skipmode2 = %3d \n", currSE.value1 );
#endif
		weighted_skipmode2 = currSE.value1;

	}


	if ( IS_P_SKIP ( currMB ) && img->num_of_references > 2 )
	{
		for ( i = 0; i < num_of_orgMB_in_row; i++ )
		{
			int pos = uiPositionInPic + i * img->PicWidthInMbs;

			for ( j = 0; j < num_of_orgMB_in_row; j++, pos++ )
			{
				codingUnit *tmpMB = &img->mb_data[pos];
				tmpMB->weighted_skipmode = weighted_skipmode1;
				if ( IS_P_SKIP ( currMB ) && img->num_of_references > 3 )
				{
					tmpMB->weighted_skipmode += 2 * weighted_skipmode2;
				}
			}
		}
	}

#endif
	if ( ( img->type == P_IMG ) || ( img->type == F_IMG ) ) 
	{
		interpret_mb_mode_P ( uiPositionInPic );
	}
	else if ( img->type == I_IMG )                              // intra frame
	{
		interpret_mb_mode_I ( uiPositionInPic );
	}
	else if ( ( img->type == B_IMG ) ) // B frame
	{
		interpret_mb_mode_B ( uiPositionInPic, *pdir );
	}

	//====== READ 8x8 SUB-PARTITION MODES (modes of 8x8 blocks) and Intra VBST block modes ======
	if ( IS_P8x8 ( currMB ) )
	{
		currSE.type    = SE_MBTYPE;
		if ( img->type == B_IMG )
		{
			for ( i = 0; i < 4; i++ )
			{
				currSE.mapping = linfo_ue;

#if TRACE
				strncpy ( currSE.tracestring, "8x8 mode", TRACESTRING_SIZE );
#endif

				//mb_part_type is fix length coding(fix length equal 2)!!   jlzheng    7.22
				assert ( currStream->streamBuffer != NULL );
				currSE.len = 2;
				dP = & ( currSlice->partArr[0] );
				currSE.reading = readB8TypeInfo;
				dP->readSyntaxElement ( &currSE, dP, currMB, uiPositionInPic ); //check: first_mb_nr - LCU position, need current CU's position

				SetB8Mode ( currMB, currSE.value1, i );



			}
		}
		else
		{
			currSE.value1 = 0;

			for ( i = 0; i < 4; i++ )
			{
				SetB8Mode ( currMB, currSE.value1, i );
			}
		}

		for ( i = 0; i < num_of_orgMB_in_row; i++ )
		{
			int pos = uiPositionInPic + i * img->PicWidthInMbs;

			for ( j = 0; j < num_of_orgMB_in_col; j++, pos++ )
			{
				codingUnit *tmpMB = &img->mb_data[pos];
				int k;

				for ( k = 0; k < 4; k++ )
				{
					tmpMB->b8mode[k] = currMB->b8mode[k];
					tmpMB->b8pdir[k] = currMB->b8pdir[k];
				}
			}
		}
	}
#if !CBP_MODIFICATION
#if INTRA_2N
	//read currMB->trans_size
	if ( !( IS_P_SKIP( currMB ) ) && ( *real_cuType != -1 ) )
	{
		currSE.value2 = ( currMB->cuType == I8MB );
		dP = & ( currSlice->partArr[0] );
		currSE.reading = readTrSize;
		dP->readSyntaxElement ( &currSE, dP, currMB, uiPositionInPic );
#if TRACE
		fprintf ( p_trace, "Transform_Size = %3d \n", currSE.value1 );
#endif
		currMB->trans_size = currSE.value1;
	}
	else if ( IS_P_SKIP( currMB ) )
	{
		currMB->trans_size = 0;
	}

	for ( i = 0; i < num_of_orgMB_in_row; i++ )
	{
		int pos = uiPositionInPic + i * img->PicWidthInMbs;

		for ( j = 0; j < num_of_orgMB_in_col; j++, pos++ )
		{
			codingUnit *tmpMB = &img->mb_data[pos];
			int tmp_intra_blksize = currMB->cuType;

			tmpMB->trans_size = currMB->trans_size;
			tmpMB->cuType = ( ( currMB->trans_size == 0 ) && ( tmp_intra_blksize == I8MB ) ) ? I16MB : currMB->cuType;
		}
	}
#endif
#endif
	//--- init codingUnit data ---
	if ( img->type == B_IMG )
	{
		init_codingUnit_Bframe ( uiPositionInPic );
	}
	else
	{
		init_codingUnit ( uiPositionInPic );
	}

	if ( IS_INTRA ( currMB ) )
	{
#if INTRA_2N
		if ( currMB->cuType == I16MB )
		{
			read_ipred_block_modes ( 0, uiPositionInPic );
			read_ipred_block_modes ( 4, uiPositionInPic );
		}
		else
		{
			for ( i = 0; i < ( input->chroma_format == 1 ? 5 : 6 ); i++ )
			{
				read_ipred_block_modes ( i, uiPositionInPic );
			}
		}
#else
		for ( i = 0; i < ( input->chroma_format == 1 ? 5 : 6 ); i++ )
		{
			read_ipred_block_modes ( i, uiPositionInPic );
		}
#endif
	}

}
#else
void read_MBHeader( codingUnit *currMB, unsigned int uiPositionInPic, int size, int num_of_orgMB_in_row, int num_of_orgMB_in_col, int *pdir, int *real_cuType )
{
	SyntaxElement currSE;

#if WEIGHTED_SKIP
#if WPM_ACE_FIX
    int weighted_skipmode_fix = 0;
#else
	int weighted_skipmode1=0;//没用
	int weighted_skipmode2=0;
#endif
#endif

	Slice *currSlice = img->currentSlice;
	DataPartition *dP;
	int i, j;
	int md_directskip_mode = 0;
	currMB->md_directskip_mode = 0;

	currSE.type = SE_MBTYPE;
	currSE.mapping = linfo_ue;
	
#if INTRA_2N
	CheckAvailabilityOfNeighbors ( currMB, currMB->ui_MbBitSize, uiPositionInPic ); //need fix
	if ( img->type == I_IMG )
	{
		dP = & ( currSlice->partArr[0] );
		currSE.reading = readcuTypeInfo;
		dP->readSyntaxElement ( &currSE, dP, currMB, uiPositionInPic ); //check: first_mb_nr - LCU position, need current CU's position
		
		*real_cuType = currSE.value1;
		if ( currSE.value1 < 0 )
		{
			currSE.value1 = 0;
		}
	}
#else
	if ( img->type == I_IMG )
	{
		currMB->cuType = 0;
	}
#endif

#if S_FRAME_OPT
	else if (img->type == P_IMG && img->typeb==BP_IMG)
	{
		dP = & ( currSlice->partArr[0] );
		currSE.reading = readcuTypeInfo_SFRAME;
		dP->readSyntaxElement ( &currSE, dP, currMB, uiPositionInPic ); //check: first_mb_nr - LCU position, need current CU's position
		*real_cuType = currSE.value1;
		if ( currSE.value1 < 0 )
		{
			currSE.value1 = 0;
		}
	}
#endif 
	//读取P F B 帧的cu 类型以及pu 2划分cu_type_index, shape_of_partion_index
	else //qyu 0822 delete skip_mode_flag
	{
		dP = & ( currSlice->partArr[0] );
		currSE.reading = readcuTypeInfo;
		dP->readSyntaxElement ( &currSE, dP, currMB, uiPositionInPic ); //check: first_mb_nr - LCU position, need current CU's position
		if (( img->type == F_IMG )||( img->type == P_IMG ))
		{
			currSE.value1++;
		}

		currSE.value1--;
		*real_cuType = currSE.value1;

		if ( currSE.value1 < 0 )
		{
			currSE.value1 = 0;
		}
	}
	
#if EXTRACT_lcu_info_BAC
  fprintf(p_lcu,"cu_type_index RD  %d\n",currSE.value1);//cu_type_index  
#endif	

#if INTRA_2N
	for ( i = 0; i < num_of_orgMB_in_row; i++ )
	{
		int pos = uiPositionInPic + i * img->PicWidthInMbs;

		for ( j = 0; j < num_of_orgMB_in_col; j++, pos++ )
		{
			codingUnit *tmpMB = &img->mb_data[pos];
			tmpMB->cuType = currSE.value1;
		}
	}
#endif
	//读取B 的b_pu_type_index\b_pu_type_min_index
	if ( img->type == B_IMG && ( currMB->cuType >= P2NX2N && currMB->cuType <= PVER_RIGHT ) )
	{
		dP = & ( currSlice->partArr[0] );
		currSE.reading = readPdir;
		dP->readSyntaxElement ( &currSE, dP, currMB, uiPositionInPic ); //check: first_mb_nr - LCU position, need current CU's position

		*pdir = currSE.value1;
#if EXTRACT_lcu_info_BAC_inter
	    fprintf(p_lcu,"b_pu_type_index  %d\n",currSE.value1);//b_pu_type_index  	    
#endif			
	}

	//读取F  f_pu_type_index
#if DHP_OPT
	else if ( img->type == F_IMG  && img->num_of_references > 1 && dhp_enabled && ( currMB->cuType >= P2NX2N && currMB->cuType <= PVER_RIGHT ) )
#else
	else if ( img->type == F_IMG  && dhp_enabled && ( currMB->cuType >= P2NX2N && currMB->cuType <= PVER_RIGHT ) )
#endif
	{

#if SIMP_MV
	  //cu=8x8 则默认为0
      if (currMB->ui_MbBitSize == B8X8_IN_BIT && currMB->cuType >= P2NXN && currMB->cuType <= PVER_RIGHT)
        *pdir = 0;
      else
      {
        dP = & ( currSlice->partArr[0] );
        currSE.reading = readPdir_dhp;
        dP->readSyntaxElement ( &currSE, dP, currMB, uiPositionInPic ); //check: first_mb_nr - LCU position, need current CU's position
        *pdir = currSE.value1;
#if EXTRACT_lcu_info_BAC_inter
	      fprintf(p_lcu,"f_pu_type_index  %d\n",currSE.value1);//f_pu_type_index  
#endif			
      }
#else
		dP = & ( currSlice->partArr[0] );
		currSE.reading = readPdir_dhp;
		dP->readSyntaxElement ( &currSE, dP, currMB, uiPositionInPic ); //check: first_mb_nr - LCU position, need current CU's position
		*pdir = currSE.value1;
#endif
	}
	else//读取F  f_pu_type_index，不存在默认为0
	{
		*pdir = 0;
#if EXTRACT
		EcuInfoInterSyntax[3] = *pdir; //f_pu_type_index Table73	默认0	
#if EXTRACT_lcu_info_BAC_inter		
	    fprintf(p_lcu,"f_pu_type_index  %d  F_2Nx2N default:0\n",*pdir);//f_pu_type_index  
#endif
#endif				
	}

	//读取F  weighted_skip_mode
#if WEIGHTED_SKIP
	if ( IS_P_SKIP ( currMB ) && img->type== F_IMG && wsm_enabled && img->num_of_references > 1 )
	{
		dP = & ( currSlice->partArr[0] );
#if WPM_ACE_OPT
    currSE.type = SE_WPM1;
#endif
		currSE.reading = readWPM;
		dP->readSyntaxElement ( &currSE, dP, currMB, uiPositionInPic ); //check: first_mb_nr - LCU position, need current CU's position
#if EXTRACT
		EcuInfoInterSyntax[4] = currSE.value1;

#if EXTRACT_lcu_info_debug
		fprintf(p_lcu_debug,"F weighted_skip_mode %d readWPM\n",currSE.value1);//weighted_skip_mode 
#endif
#endif	
#if EXTRACT_lcu_info_BAC_inter
		fprintf(p_lcu,"F weighted_skip_mode %d\n",currSE.value1);//weighted_skip_mode 
#endif	

#if TRACE
		fprintf ( p_trace, "weighted_skipmode1 = %3d \n", currSE.value1 );
#endif
#if WPM_ACE_FIX
        weighted_skipmode_fix = currSE.value1;
#else
		weighted_skipmode1 = currSE.value1;
#endif
	}

	//被宏关闭
#if !WPM_ACE_FIX
	if ( IS_P_SKIP ( currMB ) && img->type== F_IMG && wsm_enabled && img->num_of_references > 2 )
	{
		dP = & ( currSlice->partArr[0] );
#if WPM_ACE_OPT
        currSE.type = SE_WPM2;
#endif
		currSE.reading = readWPM;
		dP->readSyntaxElement ( &currSE, dP, currMB, uiPositionInPic ); //check: first_mb_nr - LCU position, need current CU's position
#if EXTRACT
		EcuInfoInterSyntax[4] = currSE.value1;
		//fprintf(p_lcu,"F weighted_skip_mode %d\n",currSE.value1);//weighted_skip_mode 
#endif	

#if TRACE
		fprintf ( p_trace, "weighted_skipmode2 = %3d \n", currSE.value1 );
#endif
		weighted_skipmode2 = currSE.value1;

	}
#endif

	if ( IS_P_SKIP ( currMB ) )
	{
		for ( i = 0; i < num_of_orgMB_in_row; i++ )
		{
			int pos = uiPositionInPic + i * img->PicWidthInMbs;

			for ( j = 0; j < num_of_orgMB_in_row; j++, pos++ )
			{
				codingUnit *tmpMB = &img->mb_data[pos];
#if WPM_ACE_FIX
                tmpMB->weighted_skipmode = weighted_skipmode_fix;
#else
				tmpMB->weighted_skipmode = weighted_skipmode1;
				if ( IS_P_SKIP ( currMB ) && img->num_of_references > 2 )
				{
					tmpMB->weighted_skipmode += 2 * weighted_skipmode2;
				}
#endif
			}
		}
	}

#endif

	//读取F  cu_subtype_index
#if MH_PSKIP_NEW
#if WPM_ACE_FIX
    if(IS_P_SKIP(currMB) && (weighted_skipmode_fix==0) && b_mhpskip_enabled && img->type == F_IMG )
#else
	if(IS_P_SKIP(currMB) && (weighted_skipmode1==0) && (weighted_skipmode2==0) && b_mhpskip_enabled && img->type == F_IMG )
#endif
#else
#if WPM_ACE_FIX
    if(IS_P_SKIP(currMB) && (weighted_skipmode_fix==0))
#else
    if(IS_P_SKIP(currMB) && (weighted_skipmode1==0) && (weighted_skipmode2==0))
#endif
#endif
	{
		dP = & ( currSlice->partArr[0] );
		currSE.reading = read_p_skip_mode;
		dP->readSyntaxElement ( &currSE, dP, currMB, uiPositionInPic ); //check: first_mb_nr - LCU position, need current CU's position
#if EXTRACT
		EcuInfoInterSyntax[5] = currSE.value1;
	    //fprintf(p_lcu,"cu_subtype_index %d\n",currSE.value1);//read_p_skip_mode 
#endif	
#if EXTRACT_lcu_info_BAC_inter
		fprintf(p_lcu,"F cu_subtype_index %d\n",currSE.value1);//read_p_skip_mode 
#endif	

		

		md_directskip_mode = currSE.value1;

#if TRACE
		fprintf ( p_trace, "p_directskip_mode = %3d \n", currSE.value1 );
#endif

		for ( i = 0; i < num_of_orgMB_in_row; i++ )
		{
			int pos = uiPositionInPic + i * img->PicWidthInMbs;

			for ( j = 0; j < num_of_orgMB_in_col; j++, pos++ )
			{
				codingUnit *tmpMB = &img->mb_data[pos];
				tmpMB->md_directskip_mode = md_directskip_mode;
			}
		}
	}


#if MH_PSKIP_NEW
	if(!(b_mhpskip_enabled && img->type == F_IMG ) )
	{
		for ( i = 0; i < num_of_orgMB_in_row; i++ )
		{
			int pos = uiPositionInPic + i * img->PicWidthInMbs;

			for ( j = 0; j < num_of_orgMB_in_col; j++, pos++ )
			{
				codingUnit *tmpMB = &img->mb_data[pos];
				tmpMB->md_directskip_mode = 0;
			}
		}
	}
#endif



	//读取B  cu_subtype_index
	if(IS_DIRECT(currMB))//B 图像
	{
		dP = & ( currSlice->partArr[0] );
		currSE.reading = read_b_dir_skip;
		dP->readSyntaxElement ( &currSE, dP, currMB, uiPositionInPic ); //check: first_mb_nr - LCU position, need current CU's position

		md_directskip_mode = currSE.value1;
#if EXTRACT
		EcuInfoInterSyntax[6] = currSE.value1;
	    
#endif	
#if EXTRACT_lcu_info_BAC_inter
		fprintf(p_lcu,"cu_subtype_index %d\n",currSE.value1);//read_p_skip_mode 
#endif	


#if TRACE
		fprintf ( p_trace, "md_directskip_mode = %3d \n", currSE.value1 );
#endif

		for ( i = 0; i < num_of_orgMB_in_row; i++ )
		{
			int pos = uiPositionInPic + i * img->PicWidthInMbs;

			for ( j = 0; j < num_of_orgMB_in_col; j++, pos++ )
			{
				codingUnit *tmpMB = &img->mb_data[pos];
				tmpMB->md_directskip_mode = md_directskip_mode;
			}
		}
	}

	
    if ( ( img->type == F_IMG ) || ( img->type == P_IMG ))
	{
		interpret_mb_mode_PF ( uiPositionInPic, *pdir );
	}
	else if ( img->type == I_IMG )                              // intra frame
	{
		interpret_mb_mode_I ( uiPositionInPic );
	}
	else if ( ( img->type == B_IMG ) ) // B frame
	{
		interpret_mb_mode_B ( uiPositionInPic, *pdir );
	}

	//读取B  b_pu_type_index2

	//====== READ 8x8 SUB-PARTITION MODES (modes of 8x8 blocks) and Intra VBST block modes ======
	if ( IS_P8x8 ( currMB ) )
	{
		currSE.type    = SE_MBTYPE;
    if ( img->type == B_IMG )
		{
			for ( i = 0; i < 4; i++ )
			{
				currSE.mapping = linfo_ue;
#if TRACE
				strncpy ( currSE.tracestring, "8x8 mode", TRACESTRING_SIZE );
#endif
				//mb_part_type is fix length coding(fix length equal 2)!!   jlzheng    7.22
				assert ( currStream->streamBuffer != NULL );
				currSE.len = 2;
				dP = & ( currSlice->partArr[0] );
				currSE.reading = readB8TypeInfo;
				dP->readSyntaxElement ( &currSE, dP, currMB, uiPositionInPic ); //check: first_mb_nr - LCU position, need current CU's position
#if EXTRACT
				if( currSE.value1 >= 6)
				{
					int value=currSE.value1;
					value = (value >> 1) + (value & 0x0001);
					EcuInfoInterSyntax[7+i]= value;//B b_pu_type_index2 	NxN 划分时4个PU的预测方向
				}
				else
				{			
					EcuInfoInterSyntax[7+i]= currSE.value1;//B b_pu_type_index2 	NxN 划分时4个PU的预测方向
				}
#if EXTRACT_lcu_info_debug
				fprintf(p_lcu_debug,"b_pu_type_index2  %d B\n",currSE.value1);//B b_pu_type_index2
#endif

#endif	

				SetB8Mode ( currMB, currSE.value1, i );
			}
		}
	//读取B  b_pu_type_index2		
#if DHP_OPT
		else if(img->type == F_IMG && dhp_enabled && img->num_of_references > 1 )
#else
		else if(img->type == F_IMG && dhp_enabled )
#endif
		{
			for ( i = 0; i < 4; i++ )
			{
				currSE.mapping = linfo_ue;
#if TRACE
				strncpy ( currSE.tracestring, "8x8 mode", TRACESTRING_SIZE );
#endif
				//mb_part_type is fix length coding(fix length equal 2)!!   jlzheng    7.22
				assert ( currStream->streamBuffer != NULL );
				currSE.len = 2;
				dP = & ( currSlice->partArr[0] );
				currSE.reading = readB8TypeInfo_dhp;
				dP->readSyntaxElement ( &currSE, dP, currMB, uiPositionInPic ); //check: first_mb_nr - LCU position, need current CU's position
#if EXTRACT
				EcuInfoInterSyntax[11+i]= currSE.value1;//F f_pu_type_index2	NxN 划分时4个PU的预测方向
#if EXTRACT_MV_debug
				fprintf(p_lcu,"b_pu_type_index2  %d F\n",currSE.value1);//F f_pu_type_index2
#endif				
#endif	

				SetB8Mode ( currMB, currSE.value1, i );
			}
		}
		else 
		{
			currSE.value1 = 0;
			for ( i = 0; i < 4; i++ )
			{
				SetB8Mode ( currMB, currSE.value1, i );
			}
		}


		for ( i = 0; i < num_of_orgMB_in_row; i++ )
		{
			int pos = uiPositionInPic + i * img->PicWidthInMbs;

			for ( j = 0; j < num_of_orgMB_in_col; j++, pos++ )
			{
				codingUnit *tmpMB = &img->mb_data[pos];
				int k;

				for ( k = 0; k < 4; k++ )
				{
					tmpMB->b8mode[k] = currMB->b8mode[k];
					tmpMB->b8pdir[k] = currMB->b8pdir[k];
				}
			}
		}
	}
	//读取intra CU  transform_split_flag
#if CBP_MODIFICATION
#if INTRA_2N
	//read currMB->trans_size
	if ( IS_INTRA(currMB) )
	{
		currSE.value2 = ( currMB->cuType == I8MB );
		dP = & ( currSlice->partArr[0] );
		currSE.reading = readTrSize;
		dP->readSyntaxElement ( &currSE, dP, currMB, uiPositionInPic );
#if TRACE
		fprintf ( p_trace, "Transform_Size = %3d \n", currSE.value1 );
#endif
		currMB->trans_size = currSE.value1;
#if EXTRACT
		cuInfoIntra[5]= currSE.value1;

#if EXTRACT_lcu_info_debug
		fprintf(p_lcu_debug,"intra Transform_Size %d cuInfoIntra[5]\n",currMB->trans_size);//intra Transform_Size
#endif
		fflush(p_lcu);
#endif

	}
	
	//读取intra CU  intra_pu_type_index
	if(input->useSDIP&&IS_INTRA(currMB))
	{
		dP = & ( currSlice->partArr[0] );
		currSE.reading = readcuTypeInfo_SDIP;
		dP->readSyntaxElement ( &currSE, dP, currMB, uiPositionInPic );
		currMB->cuType= currSE.value1;
#if EXTRACT
        //CU 大小为16或32，且进行TU划分
	    //fprintf(p_lcu,"intra cuType %d\n",currMB->cuType);//intra cuType 非正方形帧内预测开启
#endif		
#if EXTRACT_lcu_info_cutype_aec_debug2 
      fprintf(p_lcu_debug, "intra cuType %d\n", currMB->cuType);
      if (currMB->cuType == InNxNMB)
      {
        fprintf(p_lcu_debug, "intra cuType %d InNxNMB \n", currMB->cuType);
      }
      else if (currMB->cuType == INxnNMB)
      {
        fprintf(p_lcu_debug, "intra cuType %d INxnNMB \n", currMB->cuType);
      }
      else if (currMB->cuType == I8MB)
      {
        fprintf(p_lcu_debug, "intra cuType %d I8MB 4 cross \n", currMB->cuType);
      }
#endif	
	}
	
	
	for ( i = 0; i < num_of_orgMB_in_row; i++ )
	{
		int pos = uiPositionInPic + i * img->PicWidthInMbs;

		for ( j = 0; j < num_of_orgMB_in_col; j++, pos++ )
		{
			codingUnit *tmpMB = &img->mb_data[pos];
			int tmp_intra_blksize = currMB->cuType;

			tmpMB->trans_size = currMB->trans_size;
			tmpMB->cuType = ( ( currMB->trans_size == 0 ) && ( tmp_intra_blksize == I8MB ) ) ? I16MB : currMB->cuType;
		}
	}
#endif
#else
#if INTRA_2N
	//read currMB->trans_size
	if ( !( IS_P_SKIP( currMB ) ) && ( *real_cuType != -1 ) )
	{
		currSE.value2 = ( currMB->cuType == I8MB );
		dP = & ( currSlice->partArr[0] );
		currSE.reading = readTrSize;
		dP->readSyntaxElement ( &currSE, dP, currMB, uiPositionInPic );
#if TRACE
		fprintf ( p_trace, "Transform_Size = %3d \n", currSE.value1 );
#endif
		currMB->trans_size = currSE.value1;
	}
	else if ( IS_P_SKIP( currMB ) )
	{
		currMB->trans_size = 0;
	}

	for ( i = 0; i < num_of_orgMB_in_row; i++ )
	{
		int pos = uiPositionInPic + i * img->PicWidthInMbs;

		for ( j = 0; j < num_of_orgMB_in_col; j++, pos++ )
		{
			codingUnit *tmpMB = &img->mb_data[pos];
			int tmp_intra_blksize = currMB->cuType;

			tmpMB->trans_size = currMB->trans_size;
			tmpMB->cuType = ( ( currMB->trans_size == 0 ) && ( tmp_intra_blksize == I8MB ) ) ? I16MB : currMB->cuType;
		}
	}
#endif
#endif
	//--- init codingUnit data ---
	if ( img->type == B_IMG )
	{
		init_codingUnit_Bframe ( uiPositionInPic );
	}
	else
	{
		init_codingUnit ( uiPositionInPic );
	}

	if ( IS_INTRA ( currMB ) )
	{
#if INTRA_2N
		if ( currMB->cuType == I16MB )
		{
			read_ipred_block_modes ( 0, uiPositionInPic );
			read_ipred_block_modes ( 4, uiPositionInPic );
		}
		else
		{
			for ( i = 0; i < ( input->chroma_format == 1 ? 5 : 6 ); i++ )
			{
				read_ipred_block_modes ( i, uiPositionInPic );
			}
		}
#else
		for ( i = 0; i < ( input->chroma_format == 1 ? 5 : 6 ); i++ )
		{
			read_ipred_block_modes ( i, uiPositionInPic );
		}
#endif
	}

}
#endif

#if CBP_MODIFICATION
void read_CBP ( codingUnit *currMB, unsigned int uiPositionInPic, int num_of_orgMB_in_row, int num_of_orgMB_in_col )
{
	SyntaxElement currSE;
	Slice *currSlice = img->currentSlice;
	DataPartition *dP;
	int fixqp = ( fixed_picture_qp || fixed_slice_qp );
	int i, j;

#if TRACE
	snprintf ( currSE.tracestring, TRACESTRING_SIZE, "CBP" );
#endif

	dP = & ( currSlice->partArr[0] );
	currSE.reading = readCBP;
	dP->readSyntaxElement ( &currSE, dP, currMB, uiPositionInPic ); //Check: first_mb_nr
	currMB->cbp  = currSE.value1;


#if MB_DQP 

	// Delta quant only if nonzero coeffs
	if (input->useDQP)
	{
		if (currMB->cbp)
		{
#if TRACE
			snprintf ( currSE.tracestring, TRACESTRING_SIZE, "Delta quant " );
#endif
			dP = & ( currSlice->partArr[0] );
			currSE.reading = readDquant;
			dP->readSyntaxElement ( &currSE,  dP, currMB, uiPositionInPic ); //check: first_mb_nr
#if EXTRACT
      EcuInfoInterSyntax[38]= currSE.value1;//cu delta_qp
      //fprintf(p_lcu,"delta_qp  %d \n",currSE.value1);//delta_qp
#if EXTRACT_lcu_info_debug
      fprintf(p_lcu_debug, "currMB delta_qp=%d currMB->left_cu_qp=%d img->qp=%d\n", currSE.value1, currMB->left_cu_qp, img->qp);//qp
#endif
#endif

			currMB->delta_quant = currSE.value1;
		}
#if LEFT_PREDICTION
		currMB->qp = currMB->delta_quant + currMB->left_cu_qp;
#else
		currMB->qp = currMB->delta_quant + lastQP;
#endif

		//currMB->previouse_qp = lastQP;
		lastQP = currMB->qp;
	}
	else
	{
		currMB->delta_quant = 0;
#if QUANT_FIX
    img->qp = img->qp + currMB->delta_quant;
    assert(img->qp>=0);
    assert(img->qp<=MAX_QP+(input->sample_bit_depth - 8)*8);
#else
		img->qp = ( img->qp - MIN_QP + currMB->delta_quant + ( MAX_QP - MIN_QP + 1 ) ) % ( MAX_QP - MIN_QP + 1 ) + MIN_QP;
#endif
		currMB->qp = img->qp;
	}
	
#endif

#if EXTRACT
  EcuInfoInterSyntax[39]= currMB->qp;//cu qp
#if EXTRACT_lcu_info_debug
  fprintf(p_lcu_debug,"currMB QP  currMB->qp=%d img->qp=%d\n",currMB->qp, img->qp);//qp
#endif
#endif	


	for ( i = 0; i < num_of_orgMB_in_row; i++ )
	{
		int pos = uiPositionInPic + i * img->PicWidthInMbs;

		for ( j = 0; j < num_of_orgMB_in_col; j++, pos++ )
		{
			codingUnit* tmpMB = &img->mb_data[pos];
			tmpMB->cbp = currMB->cbp;
#if MB_DQP

			tmpMB->delta_quant = currMB->delta_quant;
			tmpMB->qp = currMB->qp;
			tmpMB->left_cu_qp = currMB->left_cu_qp;
			tmpMB->previouse_qp = currMB->previouse_qp;

#else
			tmpMB->delta_quant = 0;
#endif
			tmpMB->trans_size = currMB->trans_size;
		}
	}
}
#else
void read_CBP ( codingUnit *currMB, unsigned int uiPositionInPic, int num_of_orgMB_in_row, int num_of_orgMB_in_col )
{
	SyntaxElement currSE;
	Slice *currSlice = img->currentSlice;
	DataPartition *dP;
	int tempcbp;
	int fixqp = ( fixed_picture_qp || fixed_slice_qp );
	int i, j;
	if ( IS_INTRA ( currMB ) )
	{
		currSE.type = SE_CBP_INTRA;  //qyu 0927 delete OLD || currMB->cuType == SI4MB
	}
	else
	{
		currSE.type = SE_CBP_INTER;
	}

	if ( IS_INTRA ( currMB ) ) //qyu 0927 delete OLD || currMB->cuType == SI4MB
	{
		currSE.mapping = linfo_cbp_intra;
	}
	else
	{
		currSE.mapping = linfo_cbp_inter;
	}


#if TRACE
	snprintf ( currSE.tracestring, TRACESTRING_SIZE, "CBP" );
#endif

	dP = & ( currSlice->partArr[0] );
	currSE.reading = readCBP;
	dP->readSyntaxElement ( &currSE, dP, currMB, uiPositionInPic ); //Check: first_mb_nr
	currMB->cbp  = currSE.value1;

	if ( input->chroma_format == 1 )
	{
		if ( currMB->cbp != 0 )
		{
			tempcbp = 1;
		}
		else
		{
			tempcbp = 0;
		}
	}

#if MB_DQP 

	// Delta quant only if nonzero coeffs
		if (input->useDQP)
	{
		if (currMB->cbp)
		{
#if TRACE
			snprintf ( currSE.tracestring, TRACESTRING_SIZE, "Delta quant " );
#endif
			dP = & ( currSlice->partArr[0] );
			currSE.reading = readDquant;
			dP->readSyntaxElement ( &currSE,  dP, currMB, uiPositionInPic ); //check: first_mb_nr

			currMB->delta_quant = currSE.value1;
		}
#if LEFT_PREDICTION
		currMB->qp = currMB->delta_quant + currMB->left_cu_qp;
#else
		currMB->qp = currMB->delta_quant + lastQP;
#endif

		//currMB->previouse_qp = lastQP;
		lastQP = currMB->qp;
	}
	else
	{
		currMB->delta_quant = 0;
		img->qp = ( img->qp - MIN_QP + currMB->delta_quant + ( MAX_QP - MIN_QP + 1 ) ) % ( MAX_QP - MIN_QP + 1 ) + MIN_QP;
		currMB->qp = img->qp;
	}
#else

	// Delta quant only if nonzero coeffs
	if ( !fixqp && ( tempcbp ) )
	{
		if ( IS_INTER ( currMB ) )
		{
			currSE.type = SE_DELTA_QUANT_INTER;
		}
		else
		{
			currSE.type = SE_DELTA_QUANT_INTRA;
		}

#if TRACE
		snprintf ( currSE.tracestring, TRACESTRING_SIZE, "Delta quant " );
#endif
		dP = & ( currSlice->partArr[0] );
		currSE.reading = readDquant;
		dP->readSyntaxElement ( &currSE, dP, currMB, uiPositionInPic ); //check: first_mb_nr

		currMB->delta_quant = currSE.value1;
		//CHECKDELTAQP  //rm52k
		img->qp = ( img->qp - MIN_QP + currMB->delta_quant + ( MAX_QP - MIN_QP + 1 ) ) % ( MAX_QP - MIN_QP + 1 ) + MIN_QP;
	}

	if ( fixqp )
	{
		currMB->delta_quant = 0;
		img->qp = ( img->qp - MIN_QP + currMB->delta_quant + ( MAX_QP - MIN_QP + 1 ) ) % ( MAX_QP - MIN_QP + 1 ) + MIN_QP;
	}

#endif

	for ( i = 0; i < num_of_orgMB_in_row; i++ )
	{
		int pos = uiPositionInPic + i * img->PicWidthInMbs;

		for ( j = 0; j < num_of_orgMB_in_col; j++, pos++ )
		{
			codingUnit* tmpMB = &img->mb_data[pos];
			tmpMB->cbp = currMB->cbp;
#if MB_DQP
			tmpMB->delta_quant = currMB->delta_quant;
			tmpMB->qp = currMB->qp;
			tmpMB->left_cu_qp = currMB->left_cu_qp;
			tmpMB->previouse_qp = currMB->previouse_qp;
#else
			tmpMB->delta_quant = 0;
#endif
		}
	}
}
#endif

void fill_B_Skip_block( codingUnit *currMB, unsigned int uiPositionInPic, int num_of_orgMB_in_row, int num_of_orgMB_in_col, int N8_SizeScale )
{
  int i, j;

  for ( i = 0; i < num_of_orgMB_in_row; i++ )
  {
    int pos = uiPositionInPic + i * img->PicWidthInMbs;

    for ( j = 0; j < num_of_orgMB_in_col; j++, pos++ )
    {
      codingUnit* tmpMB = &img->mb_data[pos];
      tmpMB->cbp = 0;
    }
  }

  for ( i = 0; i < 4; i++ )
  {
    if ( ( currMB->b8mode[i] != IBLOCK ) && ( currMB->b8mode[i] == 0 ) )
    {
      int i8 = ( ( uiPositionInPic % img->PicWidthInMbs ) << 1 ) + ( i % 2 ) * N8_SizeScale;
      int j8 = ( ( uiPositionInPic / img->PicWidthInMbs ) << 1 ) + ( i / 2 ) * N8_SizeScale;

      int r, c, hv;


#if SIMP_MV
      // store 2D index of block 0
      int i8_1st;
      int j8_1st;

      if (i == 0)
      {
        i8_1st = i8;
        j8_1st = j8;
      }
#endif


#if EXTRACT_lcu_info_debug
      fprintf(p_lcu_debug, "B Skip currMB->md_directskip_mode =%4d \n", currMB->md_directskip_mode);
#endif
#if EXTRACT_lcu_info_debug
      fprintf(p_lcu_debug, "B Skip refbuf[0][j8][i8] =%4d \n", refbuf[0][j8][i8]);

      fprintf(p_lcu_debug, "B Skip img->is_field_sequence =%4d \n", img->is_field_sequence);
      
#endif
      //===== DIRECT PREDICTION =====
      if(currMB->md_directskip_mode == 0)
      {
        if ( refbuf[0][j8][i8] == -1 )
        {
#if EXTRACT_lcu_info_debug
          fprintf(p_lcu_debug, "B Skip refbuf[0][j8][i8] =%4d \n", refbuf[0][j8][i8]);
#endif
          for ( r = 0; r < num_of_orgMB_in_row; r++ )
          {
            for ( c = 0; c < num_of_orgMB_in_col; c++ )
            {
              for ( hv = 0; hv < 2; hv++ )
              {
                img->fw_mv[j8 + r][i8 + c][hv] = img->bw_mv[j8 + r][i8 + c][hv] = 0;
              }
            }
          }

          for ( r = 0; r < num_of_orgMB_in_row; r++ )
          {
            for ( c = 0; c < num_of_orgMB_in_col; c++ )
            {
              img->fw_refFrArr[j8 + r][i8 + c] = 0;
              img->bw_refFrArr[j8 + r][i8 + c] = 0;
            }
          }


#if SIMP_MV
          // only MV of block 0 is calculated, block 1/2/3 will copy the MV from block 0
          if (i==0 || currMB->ui_MbBitSize != MIN_CU_SIZE_IN_BIT)
          {
#endif

#if M3198_CU8
            SetMotionVectorPredictor ( currMB->ui_MbBitSize, uiPositionInPic, & ( img->fw_mv[j8][i8][0] )
              , img->fw_refFrArr, img->fw_mv, 0, 0, 0, MIN_CU_SIZE * N8_SizeScale, MIN_CU_SIZE * N8_SizeScale, 0, 1 );
            SetMotionVectorPredictor ( currMB->ui_MbBitSize, uiPositionInPic, & ( img->bw_mv[j8][i8][0] )
              , img->bw_refFrArr, img->bw_mv, 0, 0, 0, MIN_CU_SIZE * N8_SizeScale, MIN_CU_SIZE * N8_SizeScale, -1, 1 );
#else
            SetMotionVectorPredictor ( currMB->ui_MbBitSize, uiPositionInPic, & ( img->fw_mv[j8][i8][0] )
              , img->fw_refFrArr, img->fw_mv, 0, 0, 0, 16 * N8_SizeScale, 16 * N8_SizeScale, 0, 1 );
            SetMotionVectorPredictor ( currMB->ui_MbBitSize, uiPositionInPic, & ( img->bw_mv[j8][i8][0] )
              , img->bw_refFrArr, img->bw_mv, 0, 0, 0, 16 * N8_SizeScale, 16 * N8_SizeScale, -1, 1 );
#endif

#if SIMP_MV
          }
          else
          {
            img->fw_mv[j8][i8][0] = img->fw_mv[j8_1st][i8_1st][0];
            img->fw_mv[j8][i8][1] = img->fw_mv[j8_1st][i8_1st][1];

            img->bw_mv[j8][i8][0] = img->bw_mv[j8_1st][i8_1st][0];
            img->bw_mv[j8][i8][1] = img->bw_mv[j8_1st][i8_1st][1];
          }
#endif


          for ( r = 0; r < num_of_orgMB_in_row; r++ )
          {
            for ( c = 0; c < num_of_orgMB_in_col; c++ )
            {
              for ( hv = 0; hv < 2; hv++ )
              {
                img->fw_mv[j8 + r][i8 + c][hv] = img->fw_mv[j8][i8][hv]; // need to fix
                img->bw_mv[j8 + r][i8 + c][hv] = img->bw_mv[j8][i8][hv];
              }
            }
          }
        }
        else // next P is skip or inter mode
        {
          int iTRp, iTRb, iTRp1, iTRd, scale_refframe;
          int refframe = refbuf[0][j8][i8];
          int frame_no_next_P = 2 * img->imgtr_next_P;

          int frame_no_B = 2 * img->pic_distance;
          int delta_P[4];
          for ( j = 0; j < 4; j++ )
          {
            delta_P[j] = 2 * (img->imgtr_next_P - ref_poc[0][j]);
            delta_P[j] = ( delta_P[j] + 512 ) % 512;
          }
          iTRp = ( refframe + 1 ) * delta_P[0];
          iTRb = iTRp - ( frame_no_next_P - frame_no_B );

          scale_refframe = 0;

          iTRp = delta_P[refframe];
          iTRp1 = 2 * (img->imgtr_next_P - img->imgtr_fwRefDistance[0]);


          iTRd = frame_no_next_P - frame_no_B;
          iTRb = iTRp1 - ( frame_no_next_P - frame_no_B );

          iTRp  = ( iTRp  + 512 ) % 512;
          iTRp1 = ( iTRp1 + 512 ) % 512;
          iTRd  = ( iTRd  + 512 ) % 512;
          iTRb  = ( iTRb  + 512 ) % 512;


#if SIMP_MV
          // only MV of block 0 is calculated, block 1/2/3 will copy the MV from block 0
          if (i == 0 || currMB->ui_MbBitSize != MIN_CU_SIZE_IN_BIT)
          {
#endif

#if EXTRACT_lcu_info_debug
            
            fprintf(p_lcu_debug, "fill_B_Skip_block()\n");
            fprintf(p_lcu_debug, "B Skip ColMV fw_mv  %4d x\n", mvbuf[0][j8][i8][0]);
            fprintf(p_lcu_debug, "B Skip ColMV fw_mv  %4d y\n", mvbuf[0][j8][i8][1]);
            fprintf(p_lcu_debug, "B Skip ColMV bw_mv  %4d x\n", mvbuf[0][j8][i8][0]);
            fprintf(p_lcu_debug, "B Skip ColMV bw_mv  %4d y\n", mvbuf[0][j8][i8][1]);
#endif
#if MV_SCALING_UNIFY
            if ( mvbuf[0][j8][i8][0] < 0 )
            {
              img->fw_mv[j8][i8][0] = - ( ( ( MULTI / iTRp ) * ( 1 - iTRb * mvbuf[0][j8][i8][0] ) - 1 ) >> OFFSET );
              img->bw_mv[j8][i8][0] = ( ( MULTI / iTRp ) * ( 1 - iTRd * mvbuf[0][j8][i8][0] ) - 1 ) >> OFFSET;
            }
            else
            {
              img->fw_mv[j8][i8][0] = ( ( MULTI / iTRp ) * ( 1 + iTRb * mvbuf[0][j8][i8][0] ) - 1 ) >> OFFSET;
              img->bw_mv[j8][i8][0] = - ( ( ( MULTI / iTRp ) * ( 1 + iTRd * mvbuf[0][j8][i8][0] ) - 1 ) >> OFFSET );
            }

            if ( mvbuf[0][j8][i8][1] < 0 )
            {
              img->fw_mv[j8][i8][1] = - ( ( ( MULTI / iTRp ) * ( 1 - iTRb * mvbuf[0][j8][i8][1] ) - 1 ) >> OFFSET );
              img->bw_mv[j8][i8][1] = ( ( MULTI / iTRp ) * ( 1 - iTRd * mvbuf[0][j8][i8][1] ) - 1 ) >> OFFSET;
            }
            else
            {
              img->fw_mv[j8][i8][1] = ( ( MULTI / iTRp ) * ( 1 + iTRb * mvbuf[0][j8][i8][1] ) - 1 ) >> OFFSET;
              img->bw_mv[j8][i8][1] = - ( ( ( MULTI / iTRp ) * ( 1 + iTRd * mvbuf[0][j8][i8][1] ) - 1 ) >> OFFSET );
            }
#if EXTRACT_lcu_info_debug
            fprintf(p_lcu_debug, "B Skip PU[%4d][%4d]\n", j8,i8);
            fprintf(p_lcu_debug, "B Skip MV fw_mv  %4d x\n", img->fw_mv[j8][i8][0]);
            fprintf(p_lcu_debug, "B Skip MV fw_mv  %4d y\n", img->fw_mv[j8][i8][1]);
            fprintf(p_lcu_debug, "B Skip MV bw_mv  %4d x\n", img->bw_mv[j8][i8][0]);
            fprintf(p_lcu_debug, "B Skip MV bw_mv  %4d y\n", img->bw_mv[j8][i8][1]);
#endif
#else
            if ( mvbuf[0][j8][i8][0] < 0 )
            {
              img->fw_mv[j8][i8][0] = - ( ( ( 16384 / iTRp ) * ( 1 - iTRb * mvbuf[0][j8][i8][0] ) - 1 ) >> 14 );
              img->bw_mv[j8][i8][0] = ( ( 16384 / iTRp ) * ( 1 - iTRd * mvbuf[0][j8][i8][0] ) - 1 ) >> 14;
            }
            else
            {
              img->fw_mv[j8][i8][0] = ( ( 16384 / iTRp ) * ( 1 + iTRb * mvbuf[0][j8][i8][0] ) - 1 ) >> 14;
              img->bw_mv[j8][i8][0] = - ( ( ( 16384 / iTRp ) * ( 1 + iTRd * mvbuf[0][j8][i8][0] ) - 1 ) >> 14 );
            }

            if ( mvbuf[0][j8][i8][1] < 0 )
            {
              img->fw_mv[j8][i8][1] = - ( ( ( 16384 / iTRp ) * ( 1 - iTRb * mvbuf[0][j8][i8][1] ) - 1 ) >> 14 );
              img->bw_mv[j8][i8][1] = ( ( 16384 / iTRp ) * ( 1 - iTRd * mvbuf[0][j8][i8][1] ) - 1 ) >> 14;
            }
            else
            {
              img->fw_mv[j8][i8][1] = ( ( 16384 / iTRp ) * ( 1 + iTRb * mvbuf[0][j8][i8][1] ) - 1 ) >> 14;
              img->bw_mv[j8][i8][1] = - ( ( ( 16384 / iTRp ) * ( 1 + iTRd * mvbuf[0][j8][i8][1] ) - 1 ) >> 14 );
            }
#endif
#if HALF_PIXEL_COMPENSATION_DIRECT
            if (img->is_field_sequence)
            {
              int delta, delta2, delta_d, delta2_d;
              int oriPOC = frame_no_next_P;
              int oriRefPOC = frame_no_next_P - iTRp;
              int scaledPOC = frame_no_B;
              int scaledRefPOC = frame_no_B - iTRb;
              getDeltas( &delta, &delta2, oriPOC, oriRefPOC, scaledPOC, scaledRefPOC );
              scaledRefPOC = frame_no_B - iTRd;
              getDeltas( &delta_d, &delta2_d, oriPOC, oriRefPOC, scaledPOC, scaledRefPOC );
              assert(delta == delta_d );

              if ( mvbuf[0][j8][i8][1]+delta < 0 )
              {
                img->fw_mv[j8][i8][1] = - ( ( ( MULTI / iTRp ) * ( 1 - iTRb * (mvbuf[0][j8][i8][1]+delta) ) - 1 ) >> OFFSET ) -delta2;
                img->bw_mv[j8][i8][1] = ( ( ( MULTI / iTRp ) * ( 1 - iTRd * (mvbuf[0][j8][i8][1]+delta_d) ) - 1 ) >> OFFSET ) -delta2_d;
              }
              else
              {
                img->fw_mv[j8][i8][1] = ( ( ( MULTI / iTRp ) * ( 1 + iTRb * (mvbuf[0][j8][i8][1]+delta) ) - 1 ) >> OFFSET)-delta2;
                img->bw_mv[j8][i8][1] = - ( ( ( MULTI / iTRp ) * ( 1 + iTRd * (mvbuf[0][j8][i8][1]+delta_d) ) - 1 ) >> OFFSET ) -delta2_d;
              }

#if EXTRACT_lcu_info_debug
              fprintf(p_lcu_debug, "B Skip PU[%4d][%4d] img->is_field_sequence=%d\n", j8, i8, img->is_field_sequence);
              fprintf(p_lcu_debug, "B Skip MV fw_mv  %4d x\n", img->fw_mv[j8][i8][0]);
              fprintf(p_lcu_debug, "B Skip MV fw_mv  %4d y\n", img->fw_mv[j8][i8][1]);
              fprintf(p_lcu_debug, "B Skip MV bw_mv  %4d x\n", img->bw_mv[j8][i8][0]);
              fprintf(p_lcu_debug, "B Skip MV bw_mv  %4d y\n", img->bw_mv[j8][i8][1]);
#endif
            }
#endif
#if SIMP_MV
          }
          else
          {
            img->fw_mv[j8][i8][0] = img->fw_mv[j8_1st][i8_1st][0];
            img->bw_mv[j8][i8][0] = img->bw_mv[j8_1st][i8_1st][0];

            img->fw_mv[j8][i8][1] = img->fw_mv[j8_1st][i8_1st][1];
            img->bw_mv[j8][i8][1] = img->bw_mv[j8_1st][i8_1st][1];
          }
#endif

#if m3469
          img->fw_mv[j8][i8][0] = Clip3(-32768, 32767, img->fw_mv[j8][i8][0]);
          img->bw_mv[j8][i8][0] = Clip3(-32768, 32767, img->bw_mv[j8][i8][0]);
          img->fw_mv[j8][i8][1] = Clip3(-32768, 32767, img->fw_mv[j8][i8][1]);
          img->bw_mv[j8][i8][1] = Clip3(-32768, 32767, img->bw_mv[j8][i8][1]);
#endif

          for ( r = 0; r < num_of_orgMB_in_row; r++ )
          {
            for ( c = 0; c < num_of_orgMB_in_col; c++ )
            {
              if ( r + c == 0 )
              {
                continue;
              }

              img->fw_mv[j8 + r][i8 + c][0] = img->fw_mv[j8][i8][0];
              img->bw_mv[j8 + r][i8 + c][0] = img->bw_mv[j8][i8][0];
              img->fw_mv[j8 + r][i8 + c][1] = img->fw_mv[j8][i8][1];
              img->bw_mv[j8 + r][i8 + c][1] = img->bw_mv[j8][i8][1];
            }
          }

#if EXTRACT_lcu_info_mvp_debug
          if (1)
          {
            int j8, i8;
            for (j8 = 0; j8<16; j8++)
              for (i8 = 0; i8 < 16; i8++)
              {
                fprintf(p_lcu_debug, "img->fw_mv[%4d][%4d][0] =%4d\n", j8, i8, img->fw_mv[j8][i8][0]);
                fprintf(p_lcu_debug, "img->fw_mv[%4d][%4d][1] =%4d\n", j8, i8, img->fw_mv[j8][i8][1]);

                fprintf(p_lcu_debug, "img->bw_mv[%4d][%4d][0] =%4d\n", j8, i8, img->fw_mv[j8][i8][0]);
                fprintf(p_lcu_debug, "img->bw_mv[%4d][%4d][1] =%4d\n", j8, i8, img->fw_mv[j8][i8][1]);
              }
          }
#endif	 
          img->fw_refFrArr[j8][i8] = 0;
          img->bw_refFrArr[j8][i8] = 0;

          for ( r = 0; r < num_of_orgMB_in_row; r++ )
          {
            for ( c = 0; c < num_of_orgMB_in_col; c++ )
            {
              img->bw_refFrArr[j8 + r][i8 + c] = 0; //img->bw_refFrArr[j8][i8];
              img->fw_refFrArr[j8 + r][i8 + c] = 0; //img->fw_refFrArr[j8][i8];
            }
          }
        }
      }//end of md_directskip_mode == 0;
      else if(currMB->md_directskip_mode != 0)
      {
        for ( r = 0; r < num_of_orgMB_in_row; r++ )
        {
          for ( c = 0; c < num_of_orgMB_in_col; c++ )
          {
            for ( hv = 0; hv < 2; hv++ )
            {
              img->fw_mv[j8 + r][i8 + c][hv] = img->bw_mv[j8 + r][i8 + c][hv] = 0;
            }
          }
        }

        for ( r = 0; r < num_of_orgMB_in_row; r++ )
        {
          for ( c = 0; c < num_of_orgMB_in_col; c++ )
          {
            img->fw_refFrArr[j8 + r][i8 + c] = 0;
            img->bw_refFrArr[j8 + r][i8 + c] = 0;
          }
        }

        for ( r = 0; r < num_of_orgMB_in_row; r++ )
        {
          for ( c = 0; c < num_of_orgMB_in_col; c++ )
          {
            switch(currMB->md_directskip_mode)
            {
            case 0:
            case DS_SYM:
            case DS_BID:
              img->fw_refFrArr[j8 + r][i8 + c] = 0;
              img->bw_refFrArr[j8 + r][i8 + c] = 0;
              break;
            case DS_BACKWARD:
              img->fw_refFrArr[j8 + r][i8 + c] = -1;
              img->bw_refFrArr[j8 + r][i8 + c] = 0;
              break;
            case DS_FORWARD:
              img->fw_refFrArr[j8 + r][i8 + c] = 0;
              img->bw_refFrArr[j8 + r][i8 + c] = -1;
              break;

            }
          }
        }

        SetSkipMotionVectorPredictor( currMB->ui_MbBitSize, uiPositionInPic, & ( img->fw_mv[j8][i8][0] ), & ( img->bw_mv[j8][i8][0] ), 0, 0, 0, MIN_CU_SIZE * N8_SizeScale, MIN_CU_SIZE * N8_SizeScale, currMB->md_directskip_mode );

        for ( r = 0; r < num_of_orgMB_in_row; r++ )
        {
          for ( c = 0; c < num_of_orgMB_in_col; c++ )
          {
            for ( hv = 0; hv < 2; hv++ )
            {
              img->fw_mv[j8 + r][i8 + c][hv] = img->fw_mv[j8][i8][hv]; // need to fix
              img->bw_mv[j8 + r][i8 + c][hv] = img->bw_mv[j8][i8][hv];
            }
          }
        }
      }
    }
  }

#if EXTRACT_lcu_info_mvp_debug
  if (1)
  {
    int j8, i8;
    for (j8 = 0; j8<16; j8++)
      for (i8 = 0; i8 < 16; i8++)
      {
        fprintf(p_lcu_debug, "img->fw_mv[%4d][%4d][0] =%4d\n", j8, i8, img->fw_mv[j8][i8][0]);
        fprintf(p_lcu_debug, "img->fw_mv[%4d][%4d][1] =%4d\n", j8, i8, img->fw_mv[j8][i8][1]);

        fprintf(p_lcu_debug, "img->bw_mv[%4d][%4d][0] =%4d\n", j8, i8, img->fw_mv[j8][i8][0]);
        fprintf(p_lcu_debug, "img->bw_mv[%4d][%4d][1] =%4d\n", j8, i8, img->fw_mv[j8][i8][1]);
      }
  }
#endif	  
}

void fill_P_Skip_for_MH( codingUnit *currMB, unsigned int uiPositionInPic, int num_of_orgMB_in_row, int num_of_orgMB_in_col )
{
  int ***tmp_mv         = img->tmp_mv;
  int mb_y = uiPositionInPic / img->PicWidthInMbs;
  int mb_x = uiPositionInPic % img->PicWidthInMbs;
  int block8_y = mb_y << 1;
  int block8_x = mb_x << 1;
  int i, j;
  int delta[4];

  int N8_SizeScale = 1 << ( currMB->ui_MbBitSize - MIN_CU_SIZE_IN_BIT );
  if(currMB->md_directskip_mode == 0)
  {
    PskipMV_COL(currMB,  uiPositionInPic,num_of_orgMB_in_row,  num_of_orgMB_in_col, num_of_orgMB_in_row << MIN_CU_SIZE_IN_BIT, num_of_orgMB_in_col << MIN_CU_SIZE_IN_BIT);

    for ( i = 0; i < 2 * num_of_orgMB_in_row; i++ )
    {
      for ( j = 0; j < 2 * num_of_orgMB_in_col; j++ )
      {
        if(currMB->weighted_skipmode != 0)
        {
          delta[0] = ( 2 * ( picture_distance - img->imgtr_fwRefDistance[0] ) + 512 ) % 512;
          delta[1] = ( 2 * ( picture_distance - img->imgtr_fwRefDistance[1] ) + 512 ) % 512;
          delta[2] = ( 2 * ( picture_distance - img->imgtr_fwRefDistance[2] ) + 512 ) % 512;
          delta[3] = ( 2 * ( picture_distance - img->imgtr_fwRefDistance[3] ) + 512 ) % 512;
#if AVS2_SCENE_MV
          if (0 == curr_RPS.num_of_ref - 1 && background_reference_enable){
            delta[0] = 1;
          }
          if (1 == curr_RPS.num_of_ref - 1 && background_reference_enable) {
            delta[1] = 1;
          }
          if (2 == curr_RPS.num_of_ref - 1 && background_reference_enable) {
            delta[2] = 1;
          }
          if (3 == curr_RPS.num_of_ref - 1 && background_reference_enable) {
            delta[3] = 1;
          }
#endif
          p_snd_refFrArr[block8_y + j][block8_x + i] = currMB->weighted_skipmode;
#if MV_SCALING_UNIFY
          img->p_snd_tmp_mv[block8_y + j][block8_x + i][0] = ( int ) ( delta[currMB->weighted_skipmode ] * img->tmp_mv[block8_y + j][block8_x + i][0] * (MULTI / delta[0]) + HALF_MULTI >> OFFSET );
          img->p_snd_tmp_mv[block8_y + j][block8_x + i][1] = ( int ) ( delta[currMB->weighted_skipmode ] * img->tmp_mv[block8_y + j][block8_x + i][1] * (MULTI / delta[0]) + HALF_MULTI >> OFFSET );
#else
          img->p_snd_tmp_mv[block8_y + j][block8_x + i][0] = ( int ) ( delta[currMB->weighted_skipmode ] * img->tmp_mv[block8_y + j][block8_x + i][0] * (16384 / delta[0]) + 8192 >> 14 );
          img->p_snd_tmp_mv[block8_y + j][block8_x + i][1] = ( int ) ( delta[currMB->weighted_skipmode ] * img->tmp_mv[block8_y + j][block8_x + i][1] * (16384 / delta[0]) + 8192 >> 14 );
#endif
#if HALF_PIXEL_COMPENSATION_M1
          assert(is_field_sequence == img->is_field_sequence);
          if (img->is_field_sequence)
          {
            int deltaT, delta2;
            int oriPOC = 2*picture_distance;
            int oriRefPOC = oriPOC - delta[0];
            int scaledPOC = 2*picture_distance;
            int scaledRefPOC = scaledPOC - delta[currMB->weighted_skipmode ];
            getDeltas( &deltaT, &delta2, oriPOC, oriRefPOC, scaledPOC, scaledRefPOC);
            img->p_snd_tmp_mv[block8_y + j][block8_x + i][1] = ( int ) ( ( delta[currMB->weighted_skipmode ] * (img->tmp_mv[block8_y + j][block8_x + i][1]+deltaT) * (16384 / delta[0]) + 8192 >> 14 ) -delta2 );
          }
#endif
#if EXTRACT_F_MV_debug
      fprintf(p_lcu,"fill_P_Skip_for_MH block8_y,x %d %d mv(x,y)%d %d\n",block8_y,block8_x,img->p_snd_tmp_mv[block8_y + j][block8_x + i][0],img->p_snd_tmp_mv[block8_y + j][block8_x + i][1]);
#endif

#if m3469
          img->p_snd_tmp_mv[block8_y + j][block8_x + i][0] = Clip3(-32768, 32767, img->p_snd_tmp_mv[block8_y + j][block8_x + i][0]);
          img->p_snd_tmp_mv[block8_y + j][block8_x + i][1] = Clip3(-32768, 32767, img->p_snd_tmp_mv[block8_y + j][block8_x + i][1]);
#endif
          img->p_snd_tmp_mv[block8_y + j][block8_x + i][3] = 0;
        }
        else
        {
          p_snd_refFrArr[block8_y + j][block8_x + i] = -1;
          img->p_snd_tmp_mv[block8_y + j][block8_x + i][0] = 0;
          img->p_snd_tmp_mv[block8_y + j][block8_x + i][1] = 0;
          img->p_snd_tmp_mv[block8_y + j][block8_x + i][3] = 0;
        }
      }
    }
  }
  else
  {
    setPSkipMotionVector(currMB->ui_MbBitSize, uiPositionInPic, 0, 0, 0, ( 1 << currMB->ui_MbBitSize ), ( 1 << currMB->ui_MbBitSize ), 1);
    if(currMB->md_directskip_mode == BID_P_FST || currMB->md_directskip_mode == BID_P_SND)
    {
      for ( i = 0; i < 2 * num_of_orgMB_in_row; i++ )
      {
        for ( j = 0; j < 2 * num_of_orgMB_in_col; j++ )
        {
          refFrArr[block8_y + j][block8_x + i] = img->tmp_pref_fst[currMB->md_directskip_mode];
          p_snd_refFrArr[block8_y + j][block8_x + i] = img->tmp_pref_snd[currMB->md_directskip_mode];
          img->tmp_mv[block8_y + j][block8_x + i][0] = img->tmp_fstPSkipMv[currMB->md_directskip_mode][0];
          img->tmp_mv[block8_y + j][block8_x + i][1] = img->tmp_fstPSkipMv[currMB->md_directskip_mode][1];
          img->tmp_mv[block8_y + j][block8_x + i][3] = 0;
          img->p_snd_tmp_mv[block8_y + j][block8_x + i][0] = img->tmp_sndPSkipMv[currMB->md_directskip_mode][0];
          img->p_snd_tmp_mv[block8_y + j][block8_x + i][1] = img->tmp_sndPSkipMv[currMB->md_directskip_mode][1];
          img->p_snd_tmp_mv[block8_y + j][block8_x + i][3] = 0;
        }
      }

    }
    else if(currMB->md_directskip_mode == FW_P_FST || currMB->md_directskip_mode == FW_P_SND)
    {
      for ( i = 0; i < 2 * num_of_orgMB_in_row; i++ )
      {
        for ( j = 0; j < 2 * num_of_orgMB_in_col; j++ )
        {
          refFrArr[block8_y + j][block8_x + i] = img->tmp_pref_fst[currMB->md_directskip_mode];
          p_snd_refFrArr[block8_y + j][block8_x + i] = -1;
          img->tmp_mv[block8_y + j][block8_x + i][0] = img->tmp_fstPSkipMv[currMB->md_directskip_mode][0];
          img->tmp_mv[block8_y + j][block8_x + i][1] = img->tmp_fstPSkipMv[currMB->md_directskip_mode][1];
          img->tmp_mv[block8_y + j][block8_x + i][3] = 0;
          img->p_snd_tmp_mv[block8_y + j][block8_x + i][0] = img->tmp_sndPSkipMv[currMB->md_directskip_mode][0];
          img->p_snd_tmp_mv[block8_y + j][block8_x + i][1] = img->tmp_sndPSkipMv[currMB->md_directskip_mode][1];
          img->p_snd_tmp_mv[block8_y + j][block8_x + i][3] = 0;
        }
      }
    }
  }
}

void fill_blockMV( codingUnit *currMB, unsigned int uiPositionInPic, int num_of_orgMB_in_row, int num_of_orgMB_in_col, int N8_SizeScale )
{
  int i, j;

  int mb_y = uiPositionInPic / img->PicWidthInMbs;
  int mb_x = uiPositionInPic % img->PicWidthInMbs;
  int block8_y = mb_y << 1;
  int block8_x = mb_x << 1;
  int delta[4];
#if EXTRACT_lcu_info_mvp_debug
  fprintf(p_lcu_debug, "fill_blockMV()  mb_y,x = %4d %4d\n", mb_y, mb_x);
#endif
  if ( ((img->type == F_IMG)||(img->type == P_IMG)) && currMB->b8pdir[0] ==FORWARD && currMB->b8mode[0] == 0 )
  {
    if(currMB->md_directskip_mode == 0)
    {
      PskipMV_COL(currMB,  uiPositionInPic,num_of_orgMB_in_row,  num_of_orgMB_in_col, num_of_orgMB_in_row << MIN_CU_SIZE_IN_BIT, num_of_orgMB_in_col << MIN_CU_SIZE_IN_BIT);
      for ( i = 0; i < 2 * num_of_orgMB_in_row; i++ )
      {
        for ( j = 0; j < 2 * num_of_orgMB_in_col; j++ )
        {
          if(currMB->weighted_skipmode != 0)
          {
            delta[0] = ( 2 * ( picture_distance - img->imgtr_fwRefDistance[0] ) + 512 ) % 512;
            delta[1] = ( 2 * ( picture_distance - img->imgtr_fwRefDistance[1] ) + 512 ) % 512;
            delta[2] = ( 2 * ( picture_distance - img->imgtr_fwRefDistance[2] ) + 512 ) % 512;
            delta[3] = ( 2 * ( picture_distance - img->imgtr_fwRefDistance[3] ) + 512 ) % 512;
#if AVS2_SCENE_MV
            if (0 == curr_RPS.num_of_ref - 1 && background_reference_enable){
              delta[0] = 1;
            }
            if (1 == curr_RPS.num_of_ref - 1 && background_reference_enable) {
              delta[1] = 1;
            }
            if (2 == curr_RPS.num_of_ref - 1 && background_reference_enable) {
              delta[2] = 1;
            }
            if (3 == curr_RPS.num_of_ref - 1 && background_reference_enable) {
              delta[3] = 1;
            }
#endif
            p_snd_refFrArr[block8_y + j][block8_x + i] = currMB->weighted_skipmode;
#if MV_SCALING_UNIFY
            img->p_snd_tmp_mv[block8_y + j][block8_x + i][0] = ( int ) ( delta[currMB->weighted_skipmode ] * img->tmp_mv[block8_y + j][block8_x + i][0] * (MULTI / delta[0]) + HALF_MULTI >> OFFSET );
            img->p_snd_tmp_mv[block8_y + j][block8_x + i][1] = ( int ) ( delta[currMB->weighted_skipmode ] * img->tmp_mv[block8_y + j][block8_x + i][1] * (MULTI / delta[0]) + HALF_MULTI >> OFFSET );
#else
            img->p_snd_tmp_mv[block8_y + j][block8_x + i][0] = ( int ) ( delta[currMB->weighted_skipmode ] * img->tmp_mv[block8_y + j][block8_x + i][0] * (16384 / delta[0]) + 8192 >> 14 );
            img->p_snd_tmp_mv[block8_y + j][block8_x + i][1] = ( int ) ( delta[currMB->weighted_skipmode ] * img->tmp_mv[block8_y + j][block8_x + i][1] * (16384 / delta[0]) + 8192 >> 14 );
#endif
#if HALF_PIXEL_COMPENSATION_M1
            assert(is_field_sequence == img->is_field_sequence);
            if (img->is_field_sequence)
            {
              int deltaT, delta2;
              int oriPOC = 2*picture_distance;
              int oriRefPOC = oriPOC - delta[0];
              int scaledPOC = 2*picture_distance;
              int scaledRefPOC = scaledPOC - delta[currMB->weighted_skipmode ];
              getDeltas( &deltaT, &delta2, oriPOC, oriRefPOC, scaledPOC, scaledRefPOC);
              img->p_snd_tmp_mv[block8_y + j][block8_x + i][1] = ( int ) ( delta[currMB->weighted_skipmode ] * (img->tmp_mv[block8_y + j][block8_x + i][1]+deltaT) * (16384 / delta[0]) + 8192 >> 14 ) -delta2;
            }
#endif
#if m3469
            img->p_snd_tmp_mv[block8_y + j][block8_x + i][0] = Clip3(-32768, 32767, img->p_snd_tmp_mv[block8_y + j][block8_x + i][0]);
            img->p_snd_tmp_mv[block8_y + j][block8_x + i][1] = Clip3(-32768, 32767, img->p_snd_tmp_mv[block8_y + j][block8_x + i][1]);
#endif
            img->p_snd_tmp_mv[block8_y + j][block8_x + i][3] = 0;
          }
          else
          {
            p_snd_refFrArr[block8_y + j][block8_x + i] = -1;
            img->p_snd_tmp_mv[block8_y + j][block8_x + i][0] = 0;
            img->p_snd_tmp_mv[block8_y + j][block8_x + i][1] = 0;
            img->p_snd_tmp_mv[block8_y + j][block8_x + i][3] = 0;
          }
        }
      }
    }
    else
    {
      setPSkipMotionVector(currMB->ui_MbBitSize, uiPositionInPic, 0, 0, 0, ( 1 << currMB->ui_MbBitSize ), ( 1 << currMB->ui_MbBitSize ), 1);
      if(currMB->md_directskip_mode == BID_P_FST || currMB->md_directskip_mode == BID_P_SND)
      {
        for ( i = 0; i < 2 * num_of_orgMB_in_row; i++ )
        {
          for ( j = 0; j < 2 * num_of_orgMB_in_col; j++ )
          {
            refFrArr[block8_y + j][block8_x + i] = img->tmp_pref_fst[currMB->md_directskip_mode];
            p_snd_refFrArr[block8_y + j][block8_x + i] = img->tmp_pref_snd[currMB->md_directskip_mode];
            img->tmp_mv[block8_y + j][block8_x + i][0] = img->tmp_fstPSkipMv[currMB->md_directskip_mode][0];
            img->tmp_mv[block8_y + j][block8_x + i][1] = img->tmp_fstPSkipMv[currMB->md_directskip_mode][1];
            img->tmp_mv[block8_y + j][block8_x + i][3] = 0;
            img->p_snd_tmp_mv[block8_y + j][block8_x + i][0] = img->tmp_sndPSkipMv[currMB->md_directskip_mode][0];
            img->p_snd_tmp_mv[block8_y + j][block8_x + i][1] = img->tmp_sndPSkipMv[currMB->md_directskip_mode][1];
            img->p_snd_tmp_mv[block8_y + j][block8_x + i][3] = 0;
          }
        }

      }
      else if(currMB->md_directskip_mode == FW_P_FST || currMB->md_directskip_mode == FW_P_SND)
      {
        for ( i = 0; i < 2 * num_of_orgMB_in_row; i++ )
        {
          for ( j = 0; j < 2 * num_of_orgMB_in_col; j++ )
          {
            refFrArr[block8_y + j][block8_x + i] = img->tmp_pref_fst[currMB->md_directskip_mode];
            p_snd_refFrArr[block8_y + j][block8_x + i] = -1;
            img->tmp_mv[block8_y + j][block8_x + i][0] = img->tmp_fstPSkipMv[currMB->md_directskip_mode][0];
            img->tmp_mv[block8_y + j][block8_x + i][1] = img->tmp_fstPSkipMv[currMB->md_directskip_mode][1];
            img->tmp_mv[block8_y + j][block8_x + i][3] = 0;
            img->p_snd_tmp_mv[block8_y + j][block8_x + i][0] = img->tmp_sndPSkipMv[currMB->md_directskip_mode][0];
            img->p_snd_tmp_mv[block8_y + j][block8_x + i][1] = img->tmp_sndPSkipMv[currMB->md_directskip_mode][1];
            img->p_snd_tmp_mv[block8_y + j][block8_x + i][3] = 0;
          }
        }
      }
    }
  }

#if BUGFIX20150607
  if (img->type == B_IMG)
  {
#endif
    for ( i = 0; i < 4; i++ )
    {
#if EXTRACT_lcu_info_mvp_debug
      fprintf(p_lcu_debug, "fill_blockMV()  B Skip block=%4d currMB->md_directskip_mode=%4d\n", i, currMB->md_directskip_mode);
#endif
      if ( ( currMB->b8mode[i] != IBLOCK ) && ( currMB->b8mode[i] == 0 ) )
      {
        int i8 = ( ( uiPositionInPic % img->PicWidthInMbs ) << 1 ) + ( i % 2 ) * N8_SizeScale;
        int j8 = ( ( uiPositionInPic / img->PicWidthInMbs ) << 1 ) + ( i / 2 ) * N8_SizeScale;
        int r, c, hv;


#if SIMP_MV
        int i8_1st;
        int j8_1st;

        if (i == 0)
        {
          i8_1st = i8;
          j8_1st = j8;
        }
#endif


#if EXTRACT_lcu_info_mvp_debug
        fprintf(p_lcu_debug, "fill_blockMV()  B Skip PU[%4d][%4d] currMB->md_directskip_mode=%4d\n", j8, i8, currMB->md_directskip_mode);
#endif
        //===== DIRECT PREDICTION =====
        if(currMB->md_directskip_mode == 0)
        {
          if ( refbuf[0][j8][i8] == -1 )
          {
            for ( r = 0; r < num_of_orgMB_in_row; r++ )
            {
              for ( c = 0; c < num_of_orgMB_in_col; c++ )
              {
                for ( hv = 0; hv < 2; hv++ )
                {
                  img->fw_mv[j8 + r][i8 + c][hv] = img->bw_mv[j8 + r][i8 + c][hv] = 0;
                }
              }
            }

            for ( r = 0; r < num_of_orgMB_in_row; r++ )
            {
              for ( c = 0; c < num_of_orgMB_in_col; c++ )
              {
                img->fw_refFrArr[j8 + r][i8 + c] = 0;
                img->bw_refFrArr[j8 + r][i8 + c] = 0;
              }
            }


#if SIMP_MV
            if (i == 0 || img->type != B_IMG || currMB->ui_MbBitSize != MIN_CU_SIZE_IN_BIT)
            {
#endif

              SetMotionVectorPredictor ( currMB->ui_MbBitSize, uiPositionInPic, & ( img->fw_mv[j8][i8][0] )
                , img->fw_refFrArr, img->fw_mv, 0, 0, 0, MIN_CU_SIZE * N8_SizeScale, MIN_CU_SIZE * N8_SizeScale, 0, 1 );
              SetMotionVectorPredictor ( currMB->ui_MbBitSize, uiPositionInPic, & ( img->bw_mv[j8][i8][0] )
                , img->bw_refFrArr, img->bw_mv, 0, 0, 0, MIN_CU_SIZE * N8_SizeScale, MIN_CU_SIZE * N8_SizeScale, -1, 1 );


#if SIMP_MV
            }
            else
            {
              img->fw_mv[j8][i8][0] = img->fw_mv[j8_1st][i8_1st][0];
              img->fw_mv[j8][i8][1] = img->fw_mv[j8_1st][i8_1st][1];

              img->bw_mv[j8][i8][0] = img->bw_mv[j8_1st][i8_1st][0];
              img->bw_mv[j8][i8][1] = img->bw_mv[j8_1st][i8_1st][1];
            }
#endif



            for ( r = 0; r < num_of_orgMB_in_row; r++ )
            {
              for ( c = 0; c < num_of_orgMB_in_col; c++ )
              {
                for ( hv = 0; hv < 2; hv++ )
                {
                  img->fw_mv[j8 + r][i8 + c][hv] = img->fw_mv[j8][i8][hv]; // need to fix
                  img->bw_mv[j8 + r][i8 + c][hv] = img->bw_mv[j8][i8][hv];
                }
              }
            }
          }
          else // next P is skip or inter mode
          {
            int iTRp, iTRb, iTRp1, iTRd, scale_refframe;
            int refframe = refbuf[0][j8][i8];
            int frame_no_next_P = 2 * img->imgtr_next_P;

            int frame_no_B = 2 * img->pic_distance;
            int delta_P[4];
            for ( j = 0; j < 4; j++ )
            {
              delta_P[j] = 2 * (img->imgtr_next_P - ref_poc[0][j]);
              delta_P[j] = ( delta_P[j] + 512 ) % 512;
            }
            iTRp = ( refframe + 1 ) * delta_P[0];
            iTRb = iTRp - ( frame_no_next_P - frame_no_B );

            scale_refframe = 0;

            iTRp = delta_P[refframe];
            iTRp1 = 2 * (img->imgtr_next_P - img->imgtr_fwRefDistance[0]);

            iTRd = frame_no_next_P - frame_no_B;
            iTRb = iTRp1 - ( frame_no_next_P - frame_no_B );

            iTRp  = ( iTRp  + 512 ) % 512;
            iTRp1 = ( iTRp1 + 512 ) % 512;
            iTRd  = ( iTRd  + 512 ) % 512;
            iTRb  = ( iTRb  + 512 ) % 512;


#if SIMP_MV
            if (i==0 || img->type != B_IMG || currMB->ui_MbBitSize != MIN_CU_SIZE_IN_BIT)
            {
#endif

#if MV_SCALING_UNIFY
              if ( mvbuf[0][j8][i8][0] < 0 )
              {
                img->fw_mv[j8][i8][0] = - ( ( ( MULTI / iTRp ) * ( 1 - iTRb * mvbuf[0][j8][i8][0] ) - 1 ) >> OFFSET );
                img->bw_mv[j8][i8][0] = ( ( MULTI / iTRp ) * ( 1 - iTRd * mvbuf[0][j8][i8][0] ) - 1 ) >> OFFSET;
              }
              else
              {
                img->fw_mv[j8][i8][0] = ( ( MULTI / iTRp ) * ( 1 + iTRb * mvbuf[0][j8][i8][0] ) - 1 ) >> OFFSET;
                img->bw_mv[j8][i8][0] = - ( ( ( MULTI / iTRp ) * ( 1 + iTRd * mvbuf[0][j8][i8][0] ) - 1 ) >> OFFSET );
              }

              if ( mvbuf[0][j8][i8][1] < 0 )
              {
                img->fw_mv[j8][i8][1] = - ( ( ( MULTI / iTRp ) * ( 1 - iTRb * mvbuf[0][j8][i8][1] ) - 1 ) >> OFFSET );
                img->bw_mv[j8][i8][1] = ( ( MULTI / iTRp ) * ( 1 - iTRd * mvbuf[0][j8][i8][1] ) - 1 ) >> OFFSET;
              }
              else
              {
                img->fw_mv[j8][i8][1] = ( ( MULTI / iTRp ) * ( 1 + iTRb * mvbuf[0][j8][i8][1] ) - 1 ) >> OFFSET;
                img->bw_mv[j8][i8][1] = - ( ( ( MULTI / iTRp ) * ( 1 + iTRd * mvbuf[0][j8][i8][1] ) - 1 ) >> OFFSET );
              }

#if EXTRACT_lcu_info_mvp_debug
              fprintf(p_lcu_debug, "fill_blockMV B Skip PU[%4d][%4d]\n", j8, i8);
              fprintf(p_lcu_debug, "B Skip MV fw_mv  %4d x\n", img->fw_mv[j8][i8][0]);
              fprintf(p_lcu_debug, "B Skip MV fw_mv  %4d y\n", img->fw_mv[j8][i8][1]);
              fprintf(p_lcu_debug, "B Skip MV bw_mv  %4d x\n", img->bw_mv[j8][i8][0]);
              fprintf(p_lcu_debug, "B Skip MV bw_mv  %4d y\n", img->bw_mv[j8][i8][1]);
#endif
#else
              if ( mvbuf[0][j8][i8][0] < 0 )
              {
                img->fw_mv[j8][i8][0] = - ( ( ( 16384 / iTRp ) * ( 1 - iTRb * mvbuf[0][j8][i8][0] ) - 1 ) >> 14 );
                img->bw_mv[j8][i8][0] = ( ( 16384 / iTRp ) * ( 1 - iTRd * mvbuf[0][j8][i8][0] ) - 1 ) >> 14;
              }
              else
              {
                img->fw_mv[j8][i8][0] = ( ( 16384 / iTRp ) * ( 1 + iTRb * mvbuf[0][j8][i8][0] ) - 1 ) >> 14;
                img->bw_mv[j8][i8][0] = - ( ( ( 16384 / iTRp ) * ( 1 + iTRd * mvbuf[0][j8][i8][0] ) - 1 ) >> 14 );
              }

              if ( mvbuf[0][j8][i8][1] < 0 )
              {
                img->fw_mv[j8][i8][1] = - ( ( ( 16384 / iTRp ) * ( 1 - iTRb * mvbuf[0][j8][i8][1] ) - 1 ) >> 14 );
                img->bw_mv[j8][i8][1] = ( ( 16384 / iTRp ) * ( 1 - iTRd * mvbuf[0][j8][i8][1] ) - 1 ) >> 14;
              }
              else
              {
                img->fw_mv[j8][i8][1] = ( ( 16384 / iTRp ) * ( 1 + iTRb * mvbuf[0][j8][i8][1] ) - 1 ) >> 14;
                img->bw_mv[j8][i8][1] = - ( ( ( 16384 / iTRp ) * ( 1 + iTRd * mvbuf[0][j8][i8][1] ) - 1 ) >> 14 );
              }
#endif
#if HALF_PIXEL_COMPENSATION_DIRECT
              //assert( img->is_field_sequence == 1 );
              //assert(is_field_sequence == 1);
              if ( img->is_field_sequence )
              {
                int delta, delta2, delta_d, delta2_d;
                int oriPOC = frame_no_next_P;
                int oriRefPOC = frame_no_next_P - iTRp;
                int scaledPOC = frame_no_B;
                int scaledRefPOC = frame_no_B - iTRb;
                getDeltas( &delta, &delta2, oriPOC, oriRefPOC, scaledPOC, scaledRefPOC );
                scaledRefPOC = frame_no_B - iTRd;
                getDeltas( &delta_d, &delta2_d, oriPOC, oriRefPOC, scaledPOC, scaledRefPOC);
                assert(delta == delta_d );

                if ( mvbuf[0][j8][i8][1]+delta < 0 )
                {
                  img->fw_mv[j8][i8][1] = - ( ( ( MULTI / iTRp ) * ( 1 - iTRb * (mvbuf[0][j8][i8][1]+delta) ) - 1 ) >> OFFSET ) -delta2;
                  img->bw_mv[j8][i8][1] =( ( ( MULTI / iTRp ) * ( 1 - iTRd * (mvbuf[0][j8][i8][1]+delta_d) ) - 1 ) >> OFFSET ) -delta2_d;
                }
                else
                {
                  img->fw_mv[j8][i8][1] =( ( ( MULTI / iTRp ) * ( 1 + iTRb * (mvbuf[0][j8][i8][1]+delta) ) - 1 ) >> OFFSET ) -delta2;
                  img->bw_mv[j8][i8][1] = - ( ( ( MULTI / iTRp ) * ( 1 + iTRd * (mvbuf[0][j8][i8][1]+delta_d) ) - 1 ) >> OFFSET ) -delta2_d;
                }
              }
#endif

#if SIMP_MV
            }
            else
            {
              img->fw_mv[j8][i8][0] = img->fw_mv[j8_1st][i8_1st][0];
              img->bw_mv[j8][i8][0] = img->bw_mv[j8_1st][i8_1st][0];

              img->fw_mv[j8][i8][1] = img->fw_mv[j8_1st][i8_1st][1];
              img->bw_mv[j8][i8][1] = img->bw_mv[j8_1st][i8_1st][1];
            }
#endif
#if m3469
            img->fw_mv[j8][i8][0] = Clip3(-32768, 32767, img->fw_mv[j8][i8][0]);
            img->bw_mv[j8][i8][0] = Clip3(-32768, 32767, img->bw_mv[j8][i8][0]);
            img->fw_mv[j8][i8][1] = Clip3(-32768, 32767, img->fw_mv[j8][i8][1]);
            img->bw_mv[j8][i8][1] = Clip3(-32768, 32767, img->bw_mv[j8][i8][1]);

#endif


            for ( r = 0; r < num_of_orgMB_in_row; r++ )
            {
              for ( c = 0; c < num_of_orgMB_in_col; c++ )
              {
                if ( r + c == 0 )
                {
                  continue;
                }

                img->fw_mv[j8 + r][i8 + c][0] = img->fw_mv[j8][i8][0];
                img->bw_mv[j8 + r][i8 + c][0] = img->bw_mv[j8][i8][0];
                img->fw_mv[j8 + r][i8 + c][1] = img->fw_mv[j8][i8][1];
                img->bw_mv[j8 + r][i8 + c][1] = img->bw_mv[j8][i8][1];
              }
            }

            img->fw_refFrArr[j8][i8] = 0;
            img->bw_refFrArr[j8][i8] = 0;

            for ( r = 0; r < num_of_orgMB_in_row; r++ )
            {
              for ( c = 0; c < num_of_orgMB_in_col; c++ )
              {
                img->bw_refFrArr[j8 + r][i8 + c] = 0; //img->bw_refFrArr[j8][i8];
                img->fw_refFrArr[j8 + r][i8 + c] = 0; //img->fw_refFrArr[j8][i8];
              }
            }
          }
        }
        else if(currMB->md_directskip_mode != 0)
        {
          for ( r = 0; r < num_of_orgMB_in_row; r++ )
          {
            for ( c = 0; c < num_of_orgMB_in_col; c++ )
            {
              for ( hv = 0; hv < 2; hv++ )
              {
                img->fw_mv[j8 + c][i8 + r][hv] = img->bw_mv[j8 + c][i8 + r][hv] = 0;
              }
            }
          }

          for ( r = 0; r < num_of_orgMB_in_row; r++ )
          {
            for ( c = 0; c < num_of_orgMB_in_col; c++ )
            { 
              switch(currMB->md_directskip_mode)
              {
              case 0:
              case DS_SYM:
              case DS_BID:
                img->fw_refFrArr[j8 + r][i8 + c] = 0;
                img->bw_refFrArr[j8 + r][i8 + c] = 0;
                break;
              case DS_BACKWARD:
                img->fw_refFrArr[j8 + r][i8 + c] = -1;
                img->bw_refFrArr[j8 + r][i8 + c] = 0;
                break;
              case DS_FORWARD:
                img->fw_refFrArr[j8 + r][i8 + c] = 0;
                img->bw_refFrArr[j8 + r][i8 + c] = -1;
                break;
              }
            }
          }

          SetSkipMotionVectorPredictor( currMB->ui_MbBitSize, uiPositionInPic, & ( img->fw_mv[j8][i8][0] ), & ( img->bw_mv[j8][i8][0] ), 0, 0, 0, MIN_CU_SIZE * N8_SizeScale, MIN_CU_SIZE * N8_SizeScale, currMB->md_directskip_mode );

          for ( r = 0; r < num_of_orgMB_in_row; r++ )
          {
            for ( c = 0; c < num_of_orgMB_in_col; c++ )
            {
              for ( hv = 0; hv < 2; hv++ )
              {
                img->fw_mv[j8 + c][i8 + r][hv] = img->fw_mv[j8][i8][hv]; // need to fix
                img->bw_mv[j8 + c][i8 + r][hv] = img->bw_mv[j8][i8][hv];
              }
            }
          }
        }
      }
    }
#if BUGFIX20150607
  }
#endif 
}

int read_SubMB (  unsigned int uiBitSize, unsigned int uiPositionInPic )
{
  codingUnit *currMB = &img->mb_data[uiPositionInPic];//GB current_mb_nr];
  int N8_SizeScale = 1 << ( uiBitSize - MIN_CU_SIZE_IN_BIT );
  int mb_y = ( uiPositionInPic / img->PicWidthInMbs );
  int mb_x = uiPositionInPic % img->PicWidthInMbs;

  int num_of_orgMB_in_row = N8_SizeScale;//4:1  5:2  6:4
  int num_of_orgMB_in_col = N8_SizeScale;
  int size = 1 << uiBitSize;
  int pix_x = ( img->current_mb_nr % img->PicWidthInMbs ) << MIN_CU_SIZE_IN_BIT;
  int pix_y = ( img->current_mb_nr / img->PicWidthInMbs ) << MIN_CU_SIZE_IN_BIT;
  int pdir=0;
  int real_cuType;

#if MB_DQP

  int i,j;
  int PositionInPic = uiPositionInPic;
  int mb_width;
  int pix_a;
  int remove_prediction;

  int    pix_x_InSMB = (mb_x % 4) << MIN_CU_SIZE_IN_BIT;
  int    pix_y_InSMB = (mb_y % 4) << MIN_CU_SIZE_IN_BIT;
  int    currMB_pix_height = 1 << uiBitSize;

#endif

#if INTRA_2N
  pix_x = ( img->current_mb_nr % img->PicWidthInMbs ) << MIN_CU_SIZE_IN_BIT;
  pix_y = ( img->current_mb_nr / img->PicWidthInMbs ) << MIN_CU_SIZE_IN_BIT;

  if ( pix_x + size >= img->width )
  {
    num_of_orgMB_in_col = min ( N8_SizeScale, ( img->width - pix_x ) >> MIN_CU_SIZE_IN_BIT );
  }

  if ( pix_y + size >= img->height )
  {
    num_of_orgMB_in_row = min ( N8_SizeScale, ( img->height - pix_y ) >> MIN_CU_SIZE_IN_BIT );
  }
#endif

  Init_SubMB ( uiBitSize, uiPositionInPic );

  //  currMB->cuType_2= 0;
  read_MBHeader( currMB, uiPositionInPic, size, num_of_orgMB_in_row, num_of_orgMB_in_col, &pdir, &real_cuType );

  //qyu 0822 delete skip_mode_flag

#if MB_DQP

  mb_width = img->width / MIN_CU_SIZE;
  pix_a   = ( PositionInPic % mb_width ) * MIN_CU_SIZE;
  if ( pix_a >= MIN_CU_SIZE )
  {
	  //currMB->qp = currMB->delta_quant + img->qp;
	  remove_prediction = (currMB->slice_nr != img->mb_data[PositionInPic - 1].slice_nr);
	  if (!remove_prediction)
	  {
		  currMB->left_cu_qp = currMB->mb_available_left->qp;  //mb_data[PositionInPic - 1].qp;
	  }
	  else
	  {
		  currMB->left_cu_qp = img->qp;
	  }
  }
  else
  {
	  currMB->left_cu_qp = img->qp;
  } 

#if !LAST_DQP_BUGFIX
  if(currMB->cbp == 0 )
#endif 
  {

#if LEFT_PREDICTION
	  currMB->delta_quant = 0;
	  currMB->qp = currMB->left_cu_qp;
#else
	  currMB->delta_quant = 0;
	  currMB->qp = lastQP;
#endif
	  for ( i = 0; i < num_of_orgMB_in_row; i++ )
	  {
		  int pos = uiPositionInPic + i * img->PicWidthInMbs;

		  for ( j = 0; j < num_of_orgMB_in_col; j++, pos++ )
		  {
			  codingUnit* tmpMB = &img->mb_data[pos];
			  tmpMB->delta_quant = currMB->delta_quant;
			  tmpMB->qp = currMB->qp;
			  tmpMB->previouse_qp = currMB->previouse_qp;
			  tmpMB->left_cu_qp = currMB->left_cu_qp;
		  }
	  }

  }

#endif


#if EXTRACT
  EcuInfoInterSyntax[39] = currMB->qp;//cu qp
#if EXTRACT_lcu_info_debug
  fprintf(p_lcu_debug, "currMB QP  currMB->qp=%d img->qp=%d\n", currMB->qp, img->qp);//qp
#endif
#endif	


  if ( img->type == B_IMG && real_cuType < 0 )
  {
    fill_B_Skip_block( currMB, uiPositionInPic, num_of_orgMB_in_row, num_of_orgMB_in_col, N8_SizeScale );
#if LAST_DQP_BUGFIX
	last_dquant=0;
#endif 

#if EXTRACT_lcu_info_mvp_debug
  if (1)
  {
    int j8, i8;
    for (j8 = 0; j8<16; j8++)
      for (i8 = 0; i8 < 16; i8++)
      {
        fprintf(p_lcu_debug, "img->fw_mv[%4d][%4d][0] =%4d\n", j8, i8, img->fw_mv[j8][i8][0]);
        fprintf(p_lcu_debug, "img->fw_mv[%4d][%4d][1] =%4d\n", j8, i8, img->fw_mv[j8][i8][1]);

        fprintf(p_lcu_debug, "img->bw_mv[%4d][%4d][0] =%4d\n", j8, i8, img->fw_mv[j8][i8][0]);
        fprintf(p_lcu_debug, "img->bw_mv[%4d][%4d][1] =%4d\n", j8, i8, img->fw_mv[j8][i8][1]);
      }
  }
#endif	  
    return DECODE_MB;
  }

  readReferenceIndex ( currMB->ui_MbBitSize, uiPositionInPic );
  readMotionVector ( currMB->ui_MbBitSize, uiPositionInPic );

  if ( ((img->type == F_IMG)||(img->type == P_IMG)) && real_cuType < 0)
  {
      fill_P_Skip_for_MH( currMB, uiPositionInPic, num_of_orgMB_in_row, num_of_orgMB_in_col );
#if LAST_DQP_BUGFIX
	  last_dquant=0;
#endif 
	  return DECODE_MB;
  }

#if EXTRACT_lcu_info_debug
  fprintf(p_lcu_debug, "fill_blockMV() uiPositionInPic= %4d\n", uiPositionInPic);
#endif
  fill_blockMV( currMB, uiPositionInPic, num_of_orgMB_in_row, num_of_orgMB_in_col, N8_SizeScale );

  // read CBP if not new intra mode
  read_CBP( currMB, uiPositionInPic, num_of_orgMB_in_row, num_of_orgMB_in_col );
#if EXTRACT
  fflush(p_lcu);
#endif
  // read CBP and Coeffs  ***************************************************************
  readBlockCoeffs ( uiPositionInPic ); //check
  return DECODE_MB;
}

int decode_SMB ( unsigned int uiBitSize, unsigned int uiPositionInPic )
{
  int i, split_flag = 1;
  int N8_SizeScale = 1 << ( uiBitSize - MIN_CU_SIZE_IN_BIT );

  int pix_x_InPic_start = ( uiPositionInPic % img->PicWidthInMbs ) * MIN_CU_SIZE;
  int pix_y_InPic_start = ( uiPositionInPic / img->PicWidthInMbs ) * MIN_CU_SIZE;
  int pix_x_InPic_end = ( uiPositionInPic % img->PicWidthInMbs ) * MIN_CU_SIZE + ( 1 << uiBitSize ); //Liwr 0915
  int pix_y_InPic_end = ( uiPositionInPic / img->PicWidthInMbs ) * MIN_CU_SIZE + ( 1 << uiBitSize ); //Liwr 0915
  int iBoundary_start = ( pix_x_InPic_start >= img->width ) || ( pix_y_InPic_start >= img->height );
  int iBoundary_end = ( pix_x_InPic_end > img->width ) || ( pix_y_InPic_end > img->height );
  int pos_x, pos_y, pos_InPic;

  int tmp_pos;
#if EXTRACT_08X_test      
  fprintf(p_lcu,"start pix_y,x=%d %d \n",pix_y_InPic_start,pix_x_InPic_start);
  fprintf(p_lcu,"end   pix_y,x=%d %d \n",pix_y_InPic_end,pix_x_InPic_end);
  fprintf(p_lcu,"iBoundary_start= %d \n",iBoundary_start);
  fprintf(p_lcu,"iBoundary_end= %d \n",iBoundary_end);
#endif


  if ( uiBitSize == MIN_CU_SIZE_IN_BIT ) //Liwr 0915
  {
    split_flag = 0;
  }

  if ( uiBitSize > MIN_CU_SIZE_IN_BIT && !iBoundary_end )
  {
    split_flag = readSplitFlag ( uiBitSize );
#if EXTRACT_lcu_info_debug
    fprintf(p_lcu_debug, "split_cu_flag  %d\n", split_flag);
#endif
  }

  if ( split_flag )
  {
    for ( i = 0; i < 4; i++ )
    {
      int uiBitSize_Next = uiBitSize - 1;
      int pos = uiPositionInPic + ( i / 2 ) * img->PicWidthInMbs * ( N8_SizeScale / 2 ) + ( i % 2 ) * ( N8_SizeScale / 2 );
      //Liwr 0915
#if  M3198_CU8
      pos_x = pix_x_InPic_start + ( ( ( i % 2 ) * N8_SizeScale / 2 ) << MIN_CU_SIZE_IN_BIT );
      pos_y = pix_y_InPic_start + ( ( ( i / 2 ) * N8_SizeScale / 2 ) << MIN_CU_SIZE_IN_BIT );
#else
      pos_x = pix_x_InPic_start + ( ( ( i % 2 ) * N8_SizeScale / 2 ) << 4 );
      pos_y = pix_y_InPic_start + ( ( ( i / 2 ) * N8_SizeScale / 2 ) << 4 );
#endif
      pos_InPic = ( pos_x >= img->width || pos_y >= img->height );

      if ( pos_InPic )
      {
        continue;
      }

      decode_SMB ( uiBitSize_Next, pos );
    }

    return DECODE_MB;
  }
  else
  {
    if ( !iBoundary_start )
    {
      tmp_pos = img->current_mb_nr;
      img->mb_x = ( img->current_mb_nr ) % ( img->width / MIN_CU_SIZE );
      img->mb_y = ( img->current_mb_nr ) / ( img->width / MIN_CU_SIZE );
      /* Define vertical positions */
      img->block8_y = img->mb_y * BLOCK_MULTIPLE;    /* luma block position */
      /* Define horizontal positions */
      img->block8_x = img->mb_x * BLOCK_MULTIPLE;    /* luma block position */

#if EXTRACT
			initCUInfoExtract();//初始化CU 的信息存储
			//printf("%d %d %d\n",pix_y_InPic_start,pix_x_InPic_start,uiPositionInPic);	
			//cms CU 开始
#if EXTRACT_08X      
      fprintf(p_lcu,"%08X\n",uiBitSize);
#endif

#if EXTRACT_D      
      fprintf(p_lcu,"CU uiBitSize %d\n",uiBitSize);
#endif
			//fprintf(p_lcu,"%d %d %d\n",pix_y_InPic_start,pix_x_InPic_start,uiPositionInPic);  
			//fprintf(p_lcu,"%08X\n",((pix_y_InPic_start>>3)<<16)+(pix_x_InPic_start>>3));  
			fflush(p_lcu);
#if EXTRACT_08X	  
			//fprintf(p_lcu_coeff,"%08X\n",((pix_y_InPic_start>>3)<<16)+(pix_x_InPic_start>>3));  
			//fflush(p_lcu_coeff);
      cuPosInLCUPixX = pix_x_InPic_start;
      cuPosInLCUPixY = pix_y_InPic_start;//CU 在LCU中的像素坐标
#endif
	 
#if EXTRACT_D	  
			fprintf(p_lcu_coeff,"CU uiBitSize %d\n",uiBitSize);
			//fprintf(p_lcu_coeff,"%d %d %d\n",pix_y_InPic_start,pix_x_InPic_start,uiPositionInPic);  
			fprintf(p_lcu_coeff,"%08X\n",((pix_y_InPic_start>>3)<<16)+(pix_x_InPic_start>>3));  
			fflush(p_lcu_coeff);
#endif	  

			int cu_rd_x = (pix_x_InPic_start) + (1<<uiBitSize);
			int cu_rd_y = (pix_y_InPic_start) + (1<<uiBitSize);
			int LCUW=(1<<input->g_uiMaxSizeInBit);//
			int lcu_rd_x = ((pix_x_InPic_start>>input->g_uiMaxSizeInBit)<<input->g_uiMaxSizeInBit) + LCUW;
			int lcu_rd_y = ((pix_y_InPic_start>>input->g_uiMaxSizeInBit)<<input->g_uiMaxSizeInBit) + LCUW;
			int is_lsat_cu = 0;
			if( (cu_rd_x == lcu_rd_x) &&  (cu_rd_y == lcu_rd_y) )
			{
				is_lsat_cu =1;
			}
			else if( (cu_rd_x == lcu_rd_x) &&  (cu_rd_y >= img->height) )
			{
				is_lsat_cu =1;
			}
			else if( (cu_rd_y == lcu_rd_y) &&  (cu_rd_x >= img->width) )
			{
				is_lsat_cu =1;
			}
			else if( (cu_rd_y >= img->height) && (cu_rd_x >= img->width) )//cu 超出水平边界
			{
				is_lsat_cu =1;
			}
			EcuInfoInterMv[1]=is_lsat_cu;//last_cu 
#if EXTRACT_08X
			EcuInfoInterMv[0]=((lcuPosPixY>>4)<<16)+((lcuPosPixX>>4));////{LCUaddr_y>>4, LCUaddr_x>>4}，各占16bits。 
			//LCU 在图像中的像素坐标	
			fprintf(p_lcu_mv,"%08X\n",EcuInfoInterMv[0]);
			//fprintf(p_lcu_mv,"%08X\n",EcuInfoInterMv[0]);  
			fprintf(p_lcu_mv,"%08X\n",is_lsat_cu);
#endif

#if EXTRACT_D
			fprintf(p_lcu_mv,"is last CU of LCU  %d\n",is_lsat_cu);
#endif
			
#if EXTRACT_lcu_info_debug
			
			fprintf(p_lcu_debug,"input->g_uiMaxSizeInBit %d\n",input->g_uiMaxSizeInBit);
			fprintf(p_lcu_debug,"cu_rd_x=%6d cu_rd_y=%6d\n",cu_rd_x,cu_rd_y);
			fprintf(p_lcu_debug,"lcu_rd_x=%6d lcu_rd_y=%6d\n",lcu_rd_x,lcu_rd_y);
			fprintf(p_lcu_debug,"is last CU of LCU  %d\n",is_lsat_cu);
			fprintf(p_lcu_debug,"LCUW  %d\n",LCUW);
#endif

			int cu_inlcu_x = (pix_x_InPic_start -  ((pix_x_InPic_start>>input->g_uiMaxSizeInBit)<<input->g_uiMaxSizeInBit));
			int cu_inlcu_y = (pix_y_InPic_start -  ((pix_y_InPic_start>>input->g_uiMaxSizeInBit)<<input->g_uiMaxSizeInBit));

			EcuInfoInterMv[2]=((cu_inlcu_y>>3)<<16)+(cu_inlcu_x>>3);//CU position  y  CU position  x	,{CUaddr_y>>3, CUaddr_x>>3},

			EcuInfoInterMv[3]=uiBitSize;//log2CbSize  CU 的对数大小 4==> 16
      
			//fprintf(p_lcu_mv,"CU posInPic:%d %d %d\n",pix_y_InPic_start,pix_x_InPic_start,uiPositionInPic);  
			//fprintf(p_lcu_mv,"LCUW=%d %d %d %d %d\n",LCUW,cu_rd_y,cu_rd_x,lcu_rd_y,lcu_rd_x);    
#if EXTRACT_08X
			fprintf(p_lcu_mv,"%08X\n",EcuInfoInterMv[2]);  
			fprintf(p_lcu_mv,"%08X\n",uiBitSize);
#endif
	  
#if EXTRACT_D//EXTRACT_08X //
			fprintf(p_lcu_mv,"cu_inlcu_y,x:%d %d \n",cu_inlcu_y,cu_inlcu_x);  
			fprintf(p_lcu_mv,"CU uiBitSize %d\n",uiBitSize);
#endif
 
#if EXTRACT_lcu_info_debug
			fprintf(p_lcu_debug,"\ncu_inlcu_y,x:%d %d \n",cu_inlcu_y,cu_inlcu_x);  
			fprintf(p_lcu_debug,"CU uiBitSize %d\n",uiBitSize);
#endif
#if EXTRACT_08X	  
      cuPosInLCUPixX = cu_inlcu_x;
      cuPosInLCUPixY = cu_inlcu_y;//CU 在LCU中的像素坐标
#endif

#if EXTRACT_08X
			fprintf(p_lcu,"%08X\n",EcuInfoInterMv[2]); 
#endif

			//fprintf(p_lcu_mv,"cu_inlcu:%08X\n",EcuInfoInterMv[2]);  
			//fprintf(p_lcu_mv,"log2CbSize:%d\n",EcuInfoInterMv[3]); 
			fflush(p_lcu_mv);
#endif

      read_SubMB ( uiBitSize, uiPositionInPic );
      img->current_mb_nr = uiPositionInPic;
      img->mb_x = ( img->current_mb_nr ) % ( img->width / MIN_CU_SIZE );
      img->mb_y = ( img->current_mb_nr ) / ( img->width / MIN_CU_SIZE );


      /* Define vertical positions */
      img->block8_y = img->mb_y * BLOCK_MULTIPLE;    /* luma block position */
      img->pix_y   = img->mb_y * MIN_CU_SIZE;   /* luma codingUnit position */
      img->pix_c_y = img->mb_y * MIN_CU_SIZE / 2; /* chroma codingUnit position */

      /* Define horizontal positions */
      img->block8_x = img->mb_x * BLOCK_MULTIPLE;    /* luma block position */
      img->pix_x   = img->mb_x * MIN_CU_SIZE;   /* luma pixel position */
      img->pix_c_x = img->mb_x * MIN_CU_SIZE / 2; /* chroma pixel position */


#if EXTRACT_lcu_info_mvp_debug
      if (1)
      {
        int j8, i8;
        for(j8=0;j8<16;j8++)
          for (i8 = 0; i8 < 16; i8++)
          {
            fprintf(p_lcu_debug, "img->fw_mv[%4d][%4d][0] =%4d\n", j8, i8, img->fw_mv[j8][i8][0]);
            fprintf(p_lcu_debug, "img->fw_mv[%4d][%4d][1] =%4d\n", j8, i8, img->fw_mv[j8][i8][1]);

            fprintf(p_lcu_debug, "img->bw_mv[%4d][%4d][0] =%4d\n", j8, i8, img->fw_mv[j8][i8][0]);
            fprintf(p_lcu_debug, "img->bw_mv[%4d][%4d][1] =%4d\n", j8, i8, img->fw_mv[j8][i8][1]);
          }
      }
#endif	  
      decode_SubMB();

#if EXTRACT
      //获取当前CU
      codingUnit *currMB   = &img->mb_data[img->current_mb_nr];//GB current_mb_nr];
			if( IS_INTRA(currMB))
			{
				EcuInfoInterMv[4]=1;//CU pred_mode_flag  0-inter, 1-intra 
				//cms CU 开始
				//X fprintf(p_lcu,"currMB uiBitSize %d %d\n",currMB->uiBitSize,currMB->ui_MbBitSize);
				//fprintf(p_lcu,"%08X\n",currMB->ui_MbBitSize);

				if( cuInfoIntra[5] ==0 )//CU  2Nx2N
				{
					EcuInfoInterMv[5] = 10;//CU  2Nx2N
          EcuInfoInterMv[43] = 1;//CU:	pu 个数
				}
				else if( cuInfoIntra[5] ==1 )//CU  
				{
          EcuInfoInterMv[43] = 4;//CU:	pu 个数
					switch(cuInfoIntra[6])
					{
	        case 2:
						EcuInfoInterMv[5] = 11;//CU  NxN
						break;
	        case 0:
						EcuInfoInterMv[5] = 12;//CU  I_nNxN Hor
						break;
	        case 1:
						EcuInfoInterMv[5] = 13;//CU  I_NxnN Ver
						break;
					default:
						fprintf(p_lcu_mv,"CU Intra error type\n");
						break;
					}
				}
				//X
				//fprintf(p_lcu,"cuType=%d\n",EcuInfoInterMv[5]);
				//fprintf(p_lcu,"intra_pred_modes %d\n",currMB->intra_pred_modes[0]);
				//fprintf(p_lcu,"intra_pred_modes %d\n",currMB->intra_pred_modes[1]);
				//fprintf(p_lcu,"intra_pred_modes %d\n",currMB->intra_pred_modes[2]);
				//fprintf(p_lcu,"intra_pred_modes %d\n",currMB->intra_pred_modes[3]);
				//fprintf(p_lcu,"c_ipred_mode=%d \n",currMB->c_ipred_mode);
				
				fprintf(p_lcu,"%08X\n",EcuInfoInterMv[5]);
        fprintf(p_lcu,"%08X\n", currMB->c_ipred_mode);
        for (i = 0; i < EcuInfoInterMv[43]; i++)//PU 
        {
          fprintf(p_lcu, "%08X\n", currMB->intra_pred_modes[i]);
        }
        for (i = EcuInfoInterMv[43]; i < 4; i++)//PU 
        {
          fprintf(p_lcu, "%08X\n", 0);
        }
				//4个pu，10个信息
				for ( i = 0; i < 40; i++ )
				{
					fprintf(p_lcu,"%08X\n",0);
				}
				//fprintf(p_lcu,"transform split %d\n",cuInfoIntra[5]);	
				//fprintf(p_lcu,"currMB ctp  %d \n",EcuInfoInterSyntax[36]);//ctp
				//fprintf(p_lcu,"currMB QP  %d \n",EcuInfoInterSyntax[39]);//qp

				fprintf(p_lcu,"%08X\n",cuInfoIntra[5]);	
				fprintf(p_lcu,"%08X\n",EcuInfoInterSyntax[36]);//ctp
				fprintf(p_lcu,"%08X\n", EcuInfoInterSyntax[39]);//qp

				//fprintf(p_lcu_mv,"CU Intra %d\n",1);
				//fprintf(p_lcu_mv,"part_mode  %d\n",EcuInfoInterMv[5]);fprintf(p_lcu,"cuType=%d\n",EcuInfoInterMv[5]);
				fprintf(p_lcu_mv,"%08X\n",1);
				fprintf(p_lcu_mv,"%08X\n",EcuInfoInterMv[5]);
#if EXTRACT_lcu_info_debug  //控制lcu debug信息是否打印
				fprintf(p_lcu_debug,"part mode %4d\n",EcuInfoInterMv[5]);
#endif				
				//4个pu，10个信息
				for ( i = 0; i < 32; i++ )
				{
					fprintf(p_lcu_mv,"%08X\n",0);
				}
			
				//fprintf(p_lcu_mv,"%08X\n",EcuInfoInterMv[5]);
        for (i = 0; i < EcuInfoInterMv[43]; i++)//PU 
        {
          fprintf(p_lcu_mv, "%08X\n", currMB->intra_pred_modes[i]);
        }
        for (i = EcuInfoInterMv[43]; i < 4; i++)//PU 
        {
          fprintf(p_lcu_mv, "%08X\n", 0);
        }
				fprintf(p_lcu_mv,"%08X\n",currMB->c_ipred_mode);
#if EXTRACT_lcu_info_debug  //控制lcu debug信息是否打印
        fprintf(p_lcu_debug, "EcuInfoInter[0]cu type index %4d\n", EcuInfoInter[0]);
        fprintf(p_lcu_debug, "EcuInfoInter[1]shape_of_partion_index %4d\n", EcuInfoInter[1]);
        fprintf(p_lcu_debug, "EcuInfoInterMv[43] pu number %4d\n", EcuInfoInterMv[43]);

        //EcuInfoInterMv[43] = 4;//CU:	pu 个数
        fprintf(p_lcu_debug, "intra_pred_modes[0] %4d\n", currMB->intra_pred_modes[0]);
        fprintf(p_lcu_debug, "intra_pred_modes[1] %4d\n", currMB->intra_pred_modes[1]);
        fprintf(p_lcu_debug, "intra_pred_modes[2] %4d\n", currMB->intra_pred_modes[2]);
        fprintf(p_lcu_debug, "intra_pred_modes[3] %4d\n", currMB->intra_pred_modes[3]);

        for (i = 0; i < EcuInfoInterMv[43]; i++)//PU 
        {
          fprintf(p_lcu_debug, "PU %d :intra_pred_modes[%d] %08X\n",i,i, currMB->intra_pred_modes[i]);
        }
        fprintf(p_lcu_debug, "c_ipred_mode %4d\n", currMB->c_ipred_mode);
#endif		      

				//EcuInfoInter[0]= act_sym;//cu_type_index 文档对应
				//EcuInfoInter[1]= -1;//shape_of_partion_index 文档对应			
			}			
			else//下面是帧间cu
			{
				//X fprintf(p_lcu,"currMB uiBitSize %d %d\n",currMB->uiBitSize,currMB->ui_MbBitSize);
				//fprintf(p_lcu,"%08X\n",currMB->ui_MbBitSize);
				EcuInfoInterMv[4]=0;//CU pred_mode_flag  0-inter, 1-intra 
				//EcuInfoInterMv[5];//CU part_mode CU的类型主要是划分		  
				//fprintf(p_lcu,"b8mode=%d %d %d %d\n",currMB->b8mode[0],currMB->b8mode[1],currMB->b8mode[2],currMB->b8mode[3]);
				//fprintf(p_lcu,"b8pdir=%d %d %d %d\n",currMB->b8pdir[0],currMB->b8pdir[1],currMB->b8pdir[2],currMB->b8pdir[3]);

				//fprintf(p_lcu,"b8mode=%s %s %s %s\n",aInterPredModeStr[currMB->b8mode[0]],aInterPredModeStr[currMB->b8mode[1]],aInterPredModeStr[currMB->b8mode[2]],aInterPredModeStr[currMB->b8mode[3]]);
				//fprintf(p_lcu,"b8pdir=%s %s %s %s\n",aInterPredDirStr[currMB->b8pdir[0]],aInterPredDirStr[currMB->b8pdir[1]],aInterPredDirStr[currMB->b8pdir[2]],aInterPredDirStr[currMB->b8pdir[3]]);
			
	      //fprintf(p_lcu_mv,"CU Intra %d\n",0);
#if EXTRACT_lcu_info_debug  //控制lcu debug信息是否打印
				fprintf(p_lcu_debug,"EcuInfoInter[0]cu type index %4d\n",EcuInfoInter[0]);
				fprintf(p_lcu_debug,"EcuInfoInter[1]shape_of_partion_index %4d\n",EcuInfoInter[1]);
        fprintf(p_lcu_debug,"EcuInfoInter[6]CUSubType %4d\n", EcuInfoInter[6]);
#endif		      
				if( img->type == B_IMG)//B 图像
				{
					//EcuInfoInterSyntax[6] CUSubType
					switch(EcuInfoInter[0])//cu_type_index 文档对应
					{
					case 0:
						EcuInfoInterMv[5] = 0;//CU	skip
						if(EcuInfoInterSyntax[6]== 0 && currMB->ui_MbBitSize > MIN_CU_SIZE_IN_BIT)
						{
							EcuInfoInterMv[43]=4;//CU:	pu 个数
							puInfoInter[0][0]=0;//对称
							puInfoInter[1][0]=0;
							puInfoInter[2][0]=0;
							puInfoInter[3][0]=0;
							puInfoInter[0][1]=0;//CUSubType
							puInfoInter[1][1]=0;
							puInfoInter[2][1]=0;
							puInfoInter[3][1]=0;
							EcuInfoInterMv[14]=puInfoInter[0][10];//pu0 refIdxL0
							EcuInfoInterMv[15]=puInfoInter[0][11];//pu0 refIdxL1
							EcuInfoInterMv[16]=puInfoInter[1][10];//pu1 refIdxL0
							EcuInfoInterMv[17]=puInfoInter[1][11];//pu1 refIdxL1
							EcuInfoInterMv[18]=puInfoInter[2][10];//pu2 refIdxL0
							EcuInfoInterMv[19]=puInfoInter[2][11];//pu2 refIdxL1
							EcuInfoInterMv[20]=puInfoInter[3][10];//pu3 refIdxL0
							EcuInfoInterMv[21]=puInfoInter[3][11];//pu4 refIdxL1
							
							EcuInfoInterMv[22]=puInfoInter[0][12];//pu0 MV L0 x
							EcuInfoInterMv[23]=puInfoInter[0][13];//pu0 MV L0 y
							EcuInfoInterMv[24]=puInfoInter[0][14];//pu0 MV L1 x
							EcuInfoInterMv[25]=puInfoInter[0][15];//pu0 MV L1 y
							
							EcuInfoInterMv[26]=puInfoInter[1][12];//pu1 MV L0 x
							EcuInfoInterMv[27]=puInfoInter[1][13];//pu1 MV L0 y
							EcuInfoInterMv[28]=puInfoInter[1][14];//pu1 MV L1 x
							EcuInfoInterMv[29]=puInfoInter[1][15];//pu1 MV L1 y
							
							EcuInfoInterMv[30]=puInfoInter[2][12];//pu2 MV L0 x
							EcuInfoInterMv[31]=puInfoInter[2][13];//pu2 MV L0 y
							EcuInfoInterMv[32]=puInfoInter[2][14];//pu2 MV L1 x
							EcuInfoInterMv[33]=puInfoInter[2][15];//pu2 MV L1 y
							
							EcuInfoInterMv[34]=puInfoInter[3][12];//pu3 MV L0 x
							EcuInfoInterMv[35]=puInfoInter[3][13];//pu3 MV L0 y
							EcuInfoInterMv[36]=puInfoInter[3][14];//pu3 MV L1 x
							EcuInfoInterMv[37]=puInfoInter[3][15];//pu3 MV L1 y
						}
						else		
						{
							EcuInfoInterMv[43]=1;//CU:	pu 个数
							puInfoInter[0][0]=B_skip_dir[EcuInfoInterSyntax[6]];//根据CUSubType 查表得到pu的预测方向
							puInfoInter[0][1]=EcuInfoInterSyntax[6];//CUSubType

							EcuInfoInterMv[14]=puInfoInter[0][10];//pu0 refIdxL0
							EcuInfoInterMv[15]=puInfoInter[0][11];//pu0 refIdxL1
							
							EcuInfoInterMv[22]=puInfoInter[0][12];//pu0 MV L0 x
							EcuInfoInterMv[23]=puInfoInter[0][13];//pu0 MV L0 y
							EcuInfoInterMv[24]=puInfoInter[0][14];//pu0 MV L1 x
							EcuInfoInterMv[25]=puInfoInter[0][15];//pu0 MV L1 y
						}
						break;
					case 1:
						EcuInfoInterMv[5] = 1;//CU	direct 
						if(EcuInfoInterSyntax[6]== 0 && currMB->ui_MbBitSize > MIN_CU_SIZE_IN_BIT)
						{
							EcuInfoInterMv[43]=4;//CU:	pu 个数
							puInfoInter[0][0]=0;//对称
							puInfoInter[1][0]=0;
							puInfoInter[2][0]=0;
							puInfoInter[3][0]=0;
							puInfoInter[0][1]=0+5;//CUSubType  由于提取格式文档中把skip、direct融合在一起
							puInfoInter[1][1]=0+5;
							puInfoInter[2][1]=0+5;
							puInfoInter[3][1]=0+5;
							
							EcuInfoInterMv[14]=puInfoInter[0][10];//pu0 refIdxL0
							EcuInfoInterMv[15]=puInfoInter[0][11];//pu0 refIdxL1
							EcuInfoInterMv[16]=puInfoInter[1][10];//pu1 refIdxL0
							EcuInfoInterMv[17]=puInfoInter[1][11];//pu1 refIdxL1
							EcuInfoInterMv[18]=puInfoInter[2][10];//pu2 refIdxL0
							EcuInfoInterMv[19]=puInfoInter[2][11];//pu2 refIdxL1
							EcuInfoInterMv[20]=puInfoInter[3][10];//pu3 refIdxL0
							EcuInfoInterMv[21]=puInfoInter[3][11];//pu4 refIdxL1

							EcuInfoInterMv[22]=puInfoInter[0][12];//pu0 MV L0 x
							EcuInfoInterMv[23]=puInfoInter[0][13];//pu0 MV L0 y
							EcuInfoInterMv[24]=puInfoInter[0][14];//pu0 MV L1 x
							EcuInfoInterMv[25]=puInfoInter[0][15];//pu0 MV L1 y

							EcuInfoInterMv[26]=puInfoInter[1][12];//pu1 MV L0 x
							EcuInfoInterMv[27]=puInfoInter[1][13];//pu1 MV L0 y
							EcuInfoInterMv[28]=puInfoInter[1][14];//pu1 MV L1 x
							EcuInfoInterMv[29]=puInfoInter[1][15];//pu1 MV L1 y

							EcuInfoInterMv[30]=puInfoInter[2][12];//pu2 MV L0 x
							EcuInfoInterMv[31]=puInfoInter[2][13];//pu2 MV L0 y
							EcuInfoInterMv[32]=puInfoInter[2][14];//pu2 MV L1 x
							EcuInfoInterMv[33]=puInfoInter[2][15];//pu2 MV L1 y

							EcuInfoInterMv[34]=puInfoInter[3][12];//pu3 MV L0 x
							EcuInfoInterMv[35]=puInfoInter[3][13];//pu3 MV L0 y
							EcuInfoInterMv[36]=puInfoInter[3][14];//pu3 MV L1 x
							EcuInfoInterMv[37]=puInfoInter[3][15];//pu3 MV L1 y
						}
						else		
						{
							EcuInfoInterMv[43]=1;//CU:	pu 个数
							puInfoInter[0][0]=B_skip_dir[EcuInfoInterSyntax[6]];//根据CUSubType 查表得到pu的预测方向
							puInfoInter[0][1]=EcuInfoInterSyntax[6]+5;//CUSubType

							EcuInfoInterMv[14]=puInfoInter[0][10];//pu0 refIdxL0
							EcuInfoInterMv[15]=puInfoInter[0][11];//pu0 refIdxL1

							EcuInfoInterMv[22]=puInfoInter[0][12];//pu0 MV L0 x
							EcuInfoInterMv[23]=puInfoInter[0][13];//pu0 MV L0 y
							EcuInfoInterMv[24]=puInfoInter[0][14];//pu0 MV L1 x
							EcuInfoInterMv[25]=puInfoInter[0][15];//pu0 MV L1 y
						}
						break;
					case 2:
						EcuInfoInterMv[5] = 2;//CU	2Nx2N
						EcuInfoInterMv[43]=1;//CU:	pu 个数
						//EcuInfoInterSyntax[2] 存放句法b_pu_type_index		
						puInfoInter[0][0]=B_2N_dir[EcuInfoInterSyntax[2]];//根据b_pu_type_index 查表得到pu的预测方向
						puInfoInter[0][1]=0;//CUSubType

						EcuInfoInterMv[14]=puInfoInter[0][10];//pu0 refIdxL0
						EcuInfoInterMv[15]=puInfoInter[0][11];//pu0 refIdxL1

						EcuInfoInterMv[22]=puInfoInter[0][12];//pu0 MV L0 x
						EcuInfoInterMv[23]=puInfoInter[0][13];//pu0 MV L0 y
						EcuInfoInterMv[24]=puInfoInter[0][14];//pu0 MV L1 x
						EcuInfoInterMv[25]=puInfoInter[0][15];//pu0 MV L1 y
						break;
					case 3:
						EcuInfoInterMv[43]=2;//CU:	pu 个数
						switch(EcuInfoInter[1])//shape_of_partion_index 文档对应
						{
						case 0:
							EcuInfoInterMv[5] = 3;//CU	2NxH
							break;
						case 1:
							EcuInfoInterMv[5] = 5;//CU	2NxHU
							break;
						case 2: 				
							EcuInfoInterMv[5] = 6;//CU	2NxHD
							break;
						}
						//
						
						if(currMB->ui_MbBitSize > MIN_CU_SIZE_IN_BIT)
						{
							//EcuInfoInterSyntax[2] 存放句法b_pu_type_index		
							puInfoInter[0][0]=B_2PU_dir_CUNo8x8[EcuInfoInterSyntax[2]][0];//根据b_pu_type_index 查表得到pu的预测方向
							puInfoInter[0][1]=0;//CUSubType
							puInfoInter[1][0]=B_2PU_dir_CUNo8x8[EcuInfoInterSyntax[2]][1];//根据b_pu_type_index 查表得到pu的预测方向
							puInfoInter[1][1]=0;//CUSubType
						}
						else
						{
							//EcuInfoInterSyntax[2] 存放句法b_pu_type_index		
							puInfoInter[0][0]=B_2PU_dir_CU8x8[EcuInfoInterSyntax[2]][0];//根据b_pu_type_index 查表得到pu的预测方向
							puInfoInter[0][1]=0;//CUSubType
							puInfoInter[1][0]=B_2PU_dir_CU8x8[EcuInfoInterSyntax[2]][1];//根据b_pu_type_index 查表得到pu的预测方向
							puInfoInter[1][1]=0;//CUSubType
						}
						
						EcuInfoInterMv[14]=puInfoInter[0][10];//pu0 refIdxL0
						EcuInfoInterMv[15]=puInfoInter[0][11];//pu0 refIdxL1
						EcuInfoInterMv[16]=puInfoInter[2][10];//pu1 refIdxL0
						EcuInfoInterMv[17]=puInfoInter[2][11];//pu1 refIdxL1

						EcuInfoInterMv[22]=puInfoInter[0][12];//pu0 MV L0 x
						EcuInfoInterMv[23]=puInfoInter[0][13];//pu0 MV L0 y
						EcuInfoInterMv[24]=puInfoInter[0][14];//pu0 MV L1 x
						EcuInfoInterMv[25]=puInfoInter[0][15];//pu0 MV L1 y

						EcuInfoInterMv[26]=puInfoInter[2][12];//pu1 MV L0 x
						EcuInfoInterMv[27]=puInfoInter[2][13];//pu1 MV L0 y
						EcuInfoInterMv[28]=puInfoInter[2][14];//pu1 MV L1 x
						EcuInfoInterMv[29]=puInfoInter[2][15];//pu1 MV L1 y
						break;
					case 4:
						EcuInfoInterMv[43]=2;//CU:	pu 个数
						switch(EcuInfoInter[1])//shape_of_partion_index 文档对应
						{
						case 0:
							EcuInfoInterMv[5] = 4;//CU	2NxV
							break;
						case 1:
							EcuInfoInterMv[5] = 7;//CU	2NxVL
							break;
						case 2: 				
							EcuInfoInterMv[5] = 8;//CU	2NxVR
							break;
						}

						if(currMB->ui_MbBitSize > MIN_CU_SIZE_IN_BIT)
						{
	            //fprintf(p_lcu,"EcuInfoInterSyntax[2]:%d pir=%d %d\n",	EcuInfoInterSyntax[2],B_2PU_dir_CUNo8x8[EcuInfoInterSyntax[2]][0],B_2PU_dir_CUNo8x8[EcuInfoInterSyntax[2]][1]);
							//EcuInfoInterSyntax[2] 存放句法b_pu_type_index		
							puInfoInter[0][0]=B_2PU_dir_CUNo8x8[EcuInfoInterSyntax[2]][0];//根据b_pu_type_index 查表得到pu的预测方向
							puInfoInter[0][1]=0;//CUSubType
							puInfoInter[1][0]=B_2PU_dir_CUNo8x8[EcuInfoInterSyntax[2]][1];//根据b_pu_type_index 查表得到pu的预测方向
							puInfoInter[1][1]=0;//CUSubType
						}
						else
						{
							//EcuInfoInterSyntax[2] 存放句法b_pu_type_index		
							puInfoInter[0][0]=B_2PU_dir_CU8x8[EcuInfoInterSyntax[2]][0];//根据b_pu_type_index 查表得到pu的预测方向
							puInfoInter[0][1]=0;//CUSubType
							puInfoInter[1][0]=B_2PU_dir_CU8x8[EcuInfoInterSyntax[2]][1];//根据b_pu_type_index 查表得到pu的预测方向
							puInfoInter[1][1]=0;//CUSubType
						}

						
						EcuInfoInterMv[14]=puInfoInter[0][10];//pu0 refIdxL0
						EcuInfoInterMv[15]=puInfoInter[0][11];//pu0 refIdxL1
						EcuInfoInterMv[16]=puInfoInter[1][10];//pu1 refIdxL0
						EcuInfoInterMv[17]=puInfoInter[1][11];//pu1 refIdxL1

						EcuInfoInterMv[22]=puInfoInter[0][12];//pu0 MV L0 x
						EcuInfoInterMv[23]=puInfoInter[0][13];//pu0 MV L0 y
						EcuInfoInterMv[24]=puInfoInter[0][14];//pu0 MV L1 x
						EcuInfoInterMv[25]=puInfoInter[0][15];//pu0 MV L1 y

						EcuInfoInterMv[26]=puInfoInter[1][12];//pu1 MV L0 x
						EcuInfoInterMv[27]=puInfoInter[1][13];//pu1 MV L0 y
						EcuInfoInterMv[28]=puInfoInter[1][14];//pu1 MV L1 x
						EcuInfoInterMv[29]=puInfoInter[1][15];//pu1 MV L1 y
						break;
					case 5:
						EcuInfoInterMv[5] = 9;//CU	NxN
						EcuInfoInterMv[43]= 4;//CU:  pu 个数
						//EcuInfoInterSyntax[  7-10] 存放句法B b_pu_type_index2 
						puInfoInter[0][0]= B_NxN_dir[EcuInfoInterSyntax[7] ];
						puInfoInter[1][0]= B_NxN_dir[EcuInfoInterSyntax[8] ];
						puInfoInter[2][0]= B_NxN_dir[EcuInfoInterSyntax[9] ];
						puInfoInter[3][0]= B_NxN_dir[EcuInfoInterSyntax[10] ];
						puInfoInter[0][2]=EcuInfoInterSyntax[7];//BNxNType
						puInfoInter[1][2]=EcuInfoInterSyntax[8];//BNxNType	
						puInfoInter[2][2]=EcuInfoInterSyntax[9];//BNxNType
						puInfoInter[3][2]=EcuInfoInterSyntax[10];//BNxNType				

						
						EcuInfoInterMv[14]=puInfoInter[0][10];//pu0 refIdxL0
						EcuInfoInterMv[15]=puInfoInter[0][11];//pu0 refIdxL1
						EcuInfoInterMv[16]=puInfoInter[1][10];//pu1 refIdxL0
						EcuInfoInterMv[17]=puInfoInter[1][11];//pu1 refIdxL1
						EcuInfoInterMv[18]=puInfoInter[2][10];//pu2 refIdxL0
						EcuInfoInterMv[19]=puInfoInter[2][11];//pu2 refIdxL1
						EcuInfoInterMv[20]=puInfoInter[3][10];//pu3 refIdxL0
						EcuInfoInterMv[21]=puInfoInter[3][11];//pu4 refIdxL1

						EcuInfoInterMv[22]=puInfoInter[0][12];//pu0 MV L0 x
						EcuInfoInterMv[23]=puInfoInter[0][13];//pu0 MV L0 y
						EcuInfoInterMv[24]=puInfoInter[0][14];//pu0 MV L1 x
						EcuInfoInterMv[25]=puInfoInter[0][15];//pu0 MV L1 y

						EcuInfoInterMv[26]=puInfoInter[1][12];//pu1 MV L0 x
						EcuInfoInterMv[27]=puInfoInter[1][13];//pu1 MV L0 y
						EcuInfoInterMv[28]=puInfoInter[1][14];//pu1 MV L1 x
						EcuInfoInterMv[29]=puInfoInter[1][15];//pu1 MV L1 y

						EcuInfoInterMv[30]=puInfoInter[2][12];//pu2 MV L0 x
						EcuInfoInterMv[31]=puInfoInter[2][13];//pu2 MV L0 y
						EcuInfoInterMv[32]=puInfoInter[2][14];//pu2 MV L1 x
						EcuInfoInterMv[33]=puInfoInter[2][15];//pu2 MV L1 y

						EcuInfoInterMv[34]=puInfoInter[3][12];//pu3 MV L0 x
						EcuInfoInterMv[35]=puInfoInter[3][13];//pu3 MV L0 y
						EcuInfoInterMv[36]=puInfoInter[3][14];//pu3 MV L1 x
						EcuInfoInterMv[37]=puInfoInter[3][15];//pu3 MV L1 y
						break;
					case 6://intra
						if( cuInfoIntra[5] ==0 )//CU  2Nx2N
						{
							EcuInfoInterMv[5] = 10;//CU  2Nx2N
							EcuInfoInterMv[43]= 1;//CU:  pu 个数
						}
						else if( cuInfoIntra[5] ==1 )//CU  2Nx2N
						{
							switch(cuInfoIntra[6])
							{
							case 2:
								EcuInfoInterMv[5] = 11;//CU  NxN
								EcuInfoInterMv[43]= 4;//CU:  pu 个数
								break;
							case 0:
								EcuInfoInterMv[5] = 12;//CU  I_nNxN Hor
								EcuInfoInterMv[43]= 4;//CU:  pu 个数
								break;
							case 1:
								EcuInfoInterMv[5] = 13;//CU  I_NxnN Ver
								EcuInfoInterMv[43]= 4;//CU:  pu 个数
								break;
								
							default:
								fprintf(p_lcu_mv,"CU Intra error type\n",1);
								break;
							}
						}
						break;
					default:
						fprintf(p_lcu_mv,"CU error type\n");
						break;
					}
				}
				else if( img->type == F_IMG)//F 图像
				{
					//fprintf(p_lcu,"CU  type act %d  shape %d\n",EcuInfoInter[0],EcuInfoInter[1]);

#if EXTRACT_lcu_info_debug
					fprintf(p_lcu_debug,"wighted_skip_mode [4] %d \n",EcuInfoInterSyntax[4]);
#endif
					//fprintf(p_lcu,"CUSubType [5] %d \n",EcuInfoInterSyntax[5]);
					//注意F 图像只有两种预测方向:单前向、双前向
					//EcuInfoInterSyntax[6] CUSubType
					switch(EcuInfoInter[0])//cu_type_index 文档对应
					{
					case 0:
						EcuInfoInterMv[5] = 0;//CU	skip
						//EcuInfoInterSyntax[4] 存放句法wighted_skip_mode
						if( EcuInfoInterSyntax[4] !=0  && currMB->ui_MbBitSize > MIN_CU_SIZE_IN_BIT)//wighted_skip_mode
						{
							EcuInfoInterMv[43]=4;//CU:	pu 个数
							puInfoInter[0][0]=2;//双前向
							puInfoInter[1][0]=2;
							puInfoInter[2][0]=2;
							puInfoInter[3][0]=2;
							puInfoInter[0][1]=0;//CUSubType
							puInfoInter[1][1]=0;
							puInfoInter[2][1]=0;
							puInfoInter[3][1]=0;
							puInfoInter[0][5]= EcuInfoInterSyntax[4];//wighted_skip_mode
							puInfoInter[1][5]= EcuInfoInterSyntax[4];//wighted_skip_mode
							puInfoInter[2][5]= EcuInfoInterSyntax[4];//wighted_skip_mode
							puInfoInter[3][5]= EcuInfoInterSyntax[4];//wighted_skip_mode
							
						}
						else if( EcuInfoInterSyntax[4] !=0  && currMB->ui_MbBitSize == MIN_CU_SIZE_IN_BIT)//wighted_skip_mode
						{
							EcuInfoInterMv[43]=1;//CU:	pu 个数
							puInfoInter[0][0]=2;//双前向
							puInfoInter[0][1]=0;//CUSubType
							puInfoInter[0][5]= EcuInfoInterSyntax[4];//wighted_skip_mode
						} 
						else//EcuInfoInterSyntax[4] ==0 wighted_skip_mode=0
						{
							if(EcuInfoInterSyntax[5]== 0 && currMB->ui_MbBitSize > MIN_CU_SIZE_IN_BIT)
							{
								EcuInfoInterMv[43]=4;//CU:	pu 个数
								puInfoInter[0][0]=4;//单前向
								puInfoInter[1][0]=4;
								puInfoInter[2][0]=4;
								puInfoInter[3][0]=4;
								puInfoInter[0][1]=EcuInfoInterSyntax[5]+1;//CUSubType
								puInfoInter[1][1]=EcuInfoInterSyntax[5]+1;
								puInfoInter[2][1]=EcuInfoInterSyntax[5]+1;
								puInfoInter[3][1]=EcuInfoInterSyntax[5]+1;
								puInfoInter[0][5]= EcuInfoInterSyntax[4];//wighted_skip_mode
								puInfoInter[1][5]= EcuInfoInterSyntax[4];//wighted_skip_mode
								puInfoInter[2][5]= EcuInfoInterSyntax[4];//wighted_skip_mode
								puInfoInter[3][5]= EcuInfoInterSyntax[4];//wighted_skip_mode
							}
							else		
							{
								EcuInfoInterMv[43]=1;//CU:	pu 个数
								puInfoInter[0][0]=F_skip_dir[EcuInfoInterSyntax[5]];//根据CUSubType 查表得到pu的预测方向
								puInfoInter[0][1]=EcuInfoInterSyntax[5]+1;//CUSubType
					            //fprintf(p_lcu,"puInfoInter[0][1]=EcuInfoInterSyntax[5]+1 CUSubType [5] %d \n",puInfoInter[0][1]);							
								puInfoInter[0][5]= EcuInfoInterSyntax[4];//wighted_skip_mode
							}
						}
						
						break;
					case 1:
						EcuInfoInterMv[5] = 1;//CU	direct 
						//EcuInfoInterSyntax[4] 存放句法wighted_skip_mode
						if( EcuInfoInterSyntax[4] !=0  && currMB->ui_MbBitSize > MIN_CU_SIZE_IN_BIT)//wighted_skip_mode
						{
							EcuInfoInterMv[43]=4;//CU:	pu 个数
							puInfoInter[0][0]=2;//双前向
							puInfoInter[1][0]=2;
							puInfoInter[2][0]=2;
							puInfoInter[3][0]=2;
							puInfoInter[0][1]=0+6;//CUSubType
							puInfoInter[1][1]=0+6;
							puInfoInter[2][1]=0+6;
							puInfoInter[3][1]=0+6;
							puInfoInter[0][5]= EcuInfoInterSyntax[4];//wighted_skip_mode
							puInfoInter[1][5]= EcuInfoInterSyntax[4];//wighted_skip_mode
							puInfoInter[2][5]= EcuInfoInterSyntax[4];//wighted_skip_mode
							puInfoInter[3][5]= EcuInfoInterSyntax[4];//wighted_skip_mode
						}
						else if( EcuInfoInterSyntax[4] !=0  && currMB->ui_MbBitSize == MIN_CU_SIZE_IN_BIT)//wighted_skip_mode
						{
							EcuInfoInterMv[43]=1;//CU:	pu 个数
							puInfoInter[0][0]=2;//双前向
							puInfoInter[0][1]=0+6;//CUSubType
							puInfoInter[0][5]= EcuInfoInterSyntax[4];//wighted_skip_mode
						} 
						else//EcuInfoInterSyntax[4] ==0 wighted_skip_mode=0
						{
							if(EcuInfoInterSyntax[5]== 0 && currMB->ui_MbBitSize > MIN_CU_SIZE_IN_BIT)
							{
								EcuInfoInterMv[43]=4;//CU:	pu 个数
								puInfoInter[0][0]=4;//单前向
								puInfoInter[1][0]=4;
								puInfoInter[2][0]=4;
								puInfoInter[3][0]=4;
								puInfoInter[0][1]=0+7;//CUSubType
								puInfoInter[1][1]=0+7;
								puInfoInter[2][1]=0+7;
								puInfoInter[3][1]=0+7;
								puInfoInter[0][5]= EcuInfoInterSyntax[4];//wighted_skip_mode
								puInfoInter[1][5]= EcuInfoInterSyntax[4];//wighted_skip_mode
								puInfoInter[2][5]= EcuInfoInterSyntax[4];//wighted_skip_mode
								puInfoInter[3][5]= EcuInfoInterSyntax[4];//wighted_skip_mode
							}
							else		
							{
								EcuInfoInterMv[43]=1;//CU:	pu 个数
								puInfoInter[0][0]=F_direct_dir[EcuInfoInterSyntax[5]];//根据CUSubType 查表得到pu的预测方向
								puInfoInter[0][1]=EcuInfoInterSyntax[5]+7;//CUSubType
								puInfoInter[0][5]= EcuInfoInterSyntax[4];//wighted_skip_mode
							}
						}
						
						break;
					case 2:
						EcuInfoInterMv[5] = 2;//CU	2Nx2N
						EcuInfoInterMv[43]=1;//CU:	pu 个数
						//EcuInfoInterSyntax[3] 存放句法f_pu_type_index Table74	
						if( EcuInfoInterSyntax[3] == 0 )//f_pu_type_index
						{
							//EcuInfoInterSyntax[19] 存放句法F dir_multi_hypothesis
							if( EcuInfoInterSyntax[19]  != 0)
								puInfoInter[0][0]= 2;//双前向
							else if( EcuInfoInterSyntax[19]  == 0)
								puInfoInter[0][0]= 4;//单前向
							puInfoInter[0][3]= EcuInfoInterSyntax[19];//dir_multi_hypothesis
						}
						else if( EcuInfoInterSyntax[3] == 1 )//f_pu_type_index
						{
							puInfoInter[0][0]= 2;//双前向
							puInfoInter[0][3]= EcuInfoInterSyntax[19];//dir_multi_hypothesis
							puInfoInter[0][5]= EcuInfoInterSyntax[4];//wighted_skip_mode
						}
						break;
					case 3:
						EcuInfoInterMv[43]=2;//CU:	pu 个数
						switch(EcuInfoInter[1])//shape_of_partion_index 文档对应
						{
						case 0:
							EcuInfoInterMv[5] = 3;//CU	2NxH
							break;
						case 1:
							EcuInfoInterMv[5] = 5;//CU	2NxHU
							break;
						case 2: 				
							EcuInfoInterMv[5] = 6;//CU	2NxHD
							break;
						default:
							printf("EcuInfoInter[1] != 0,1,2 =%d",EcuInfoInter[1]);
							exit(-1);
							break;
						}
						if(currMB->ui_MbBitSize == MIN_CU_SIZE_IN_BIT)
							EcuInfoInterMv[5] = 3;//CU	2NxH
						//
						if(currMB->ui_MbBitSize == MIN_CU_SIZE_IN_BIT)
						{
							//EcuInfoInterSyntax[3] 存放句法f_pu_type_index Table74	
							puInfoInter[0][0]=4;//单前向
							puInfoInter[0][1]=0;//CUSubType
							puInfoInter[1][0]=4;//单前向
							puInfoInter[1][1]=0;//CUSubType
						}
						else if(EcuInfoInterSyntax[3] == 0 && currMB->ui_MbBitSize > MIN_CU_SIZE_IN_BIT)
						{
							if(EcuInfoInterSyntax[19] != 0)
							{
								//EcuInfoInterSyntax[3] 存放句法f_pu_type_index Table74	
								puInfoInter[0][0]=2;//双前向
								puInfoInter[0][1]=0;//CUSubType
								puInfoInter[1][0]=2;//双前向
								puInfoInter[1][1]=0;//CUSubType
							}
							else if(EcuInfoInterSyntax[19] == 0)
							{
								//EcuInfoInterSyntax[3] 存放句法f_pu_type_index Table74	
								puInfoInter[0][0]=4;//单前向
								puInfoInter[0][1]=0;//CUSubType
								puInfoInter[1][0]=4;//单前向
								puInfoInter[1][1]=0;//CUSubType
							}
						}
						else
						{
							puInfoInter[0][0]=F_2PU_dir_CUNo8x8[EcuInfoInterSyntax[3]][0];//根据b_pu_type_index 查表得到pu的预测方向
							puInfoInter[0][1]=0;//CUSubType
							puInfoInter[1][0]=F_2PU_dir_CUNo8x8[EcuInfoInterSyntax[3]][1];//根据b_pu_type_index 查表得到pu的预测方向
							puInfoInter[1][1]=0;//CUSubType
						}
						
						break;
					case 4:
						EcuInfoInterMv[43]=2;//CU:	pu 个数
						switch(EcuInfoInter[1])//shape_of_partion_index 文档对应
						{
						case 0:
							EcuInfoInterMv[5] = 4;//CU	2NxV
							break;
						case 1:
							EcuInfoInterMv[5] = 7;//CU	2NxVL
							break;
						case 2: 				
							EcuInfoInterMv[5] = 8;//CU	2NxVR	
							break;					break;
						default:
							fprintf(p_lcu_mv,"CU shape_of_partion_index error 0,1,2");
							break;
						}
						if(currMB->ui_MbBitSize == MIN_CU_SIZE_IN_BIT)
							EcuInfoInterMv[5] = 4;//CU	2NxV
							
						//
						if(currMB->ui_MbBitSize == MIN_CU_SIZE_IN_BIT)
						{
							//EcuInfoInterSyntax[3] 存放句法f_pu_type_index Table74 
							puInfoInter[0][0]=4;//单前向
							puInfoInter[0][1]=0;//CUSubType
							puInfoInter[1][0]=4;//单前向
							puInfoInter[1][1]=0;//CUSubType
						}
						else if(EcuInfoInterSyntax[3] == 0 && currMB->ui_MbBitSize > MIN_CU_SIZE_IN_BIT)
						{
							if(EcuInfoInterSyntax[19] != 0)
							{
								//EcuInfoInterSyntax[3] 存放句法f_pu_type_index Table74 
								puInfoInter[0][0]=2;//双前向
								puInfoInter[0][1]=0;//CUSubType
								puInfoInter[1][0]=2;//双前向
								puInfoInter[1][1]=0;//CUSubType
							}
							else if(EcuInfoInterSyntax[19] == 0)
							{
								//EcuInfoInterSyntax[3] 存放句法f_pu_type_index Table74 
								puInfoInter[0][0]=4;//单前向
								puInfoInter[0][1]=0;//CUSubType
								puInfoInter[1][0]=4;//单前向
								puInfoInter[1][1]=0;//CUSubType
							}
						}
						else
						{
							puInfoInter[0][0]=F_2PU_dir_CUNo8x8[EcuInfoInterSyntax[3]][0];//根据b_pu_type_index 查表得到pu的预测方向
							puInfoInter[0][1]=0;//CUSubType
							puInfoInter[1][0]=F_2PU_dir_CUNo8x8[EcuInfoInterSyntax[3]][1];//根据b_pu_type_index 查表得到pu的预测方向
							puInfoInter[1][1]=0;//CUSubType
						}

						break;
					case 5:
						EcuInfoInterMv[5] = 9;//CU	NxN
						EcuInfoInterMv[43]= 4;//CU:  pu 个数
						//EcuInfoInterSyntax[11-14] 存放句法F f_pu_type_index2 
						if( EcuInfoInterSyntax[11] == 1)
							puInfoInter[0][0]= 2;//双前向
						else if(EcuInfoInterSyntax[19] != 0)
							puInfoInter[0][0]= 2;//双前向
						else if(EcuInfoInterSyntax[19] == 0)
							puInfoInter[0][0]= 4;//单前向

						if( EcuInfoInterSyntax[12] == 1)
							puInfoInter[1][0]= 2;//双前向
						else if(EcuInfoInterSyntax[19] != 0)
							puInfoInter[1][0]= 2;//双前向
						else if(EcuInfoInterSyntax[19] == 0)
							puInfoInter[1][0]= 4;//单前向

						if( EcuInfoInterSyntax[13] == 1)
							puInfoInter[2][0]= 2;//双前向
						else if(EcuInfoInterSyntax[19] != 0)
							puInfoInter[2][0]= 2;//双前向
						else if(EcuInfoInterSyntax[19] == 0)
							puInfoInter[2][0]= 4;//单前向

						if( EcuInfoInterSyntax[14] == 1)
							puInfoInter[3][0]= 2;//双前向
						else if(EcuInfoInterSyntax[19] != 0)
							puInfoInter[3][0]= 2;//双前向
						else if(EcuInfoInterSyntax[19] == 0)
							puInfoInter[3][0]= 4;//单前向
						
					case 6://intra
						break;
						if( cuInfoIntra[5] ==0 )//CU  2Nx2N
						{
							EcuInfoInterMv[5] = 10;//CU  2Nx2N
							EcuInfoInterMv[43]= 1;//CU:  pu 个数
						}
						else if( cuInfoIntra[5] ==1 )//CU  2Nx2N
						{
							switch(cuInfoIntra[6])
							{
							case 2:
								EcuInfoInterMv[5] = 11;//CU  NxN
								EcuInfoInterMv[43]= 4;//CU:  pu 个数
								break;
							case 0:
								EcuInfoInterMv[5] = 12;//CU  I_nNxN Hor
								EcuInfoInterMv[43]= 4;//CU:  pu 个数
								break;
							case 1:
								EcuInfoInterMv[5] = 13;//CU  I_NxnN Ver
								EcuInfoInterMv[43]= 4;//CU:  pu 个数
								break;
								
							default:
								fprintf(p_lcu_mv,"CU Intra error type\n",1);
								break;
							}
						}
						break;
					default:
						fprintf(p_lcu_mv,"CU error type\n");
						break;
					}
					           
				}
				else if( img->type == P_IMG)//P 图像
				{
					//注意P 图像只有1 种预测方向:单前向
					//EcuInfoInterSyntax[6] CUSubType

#if EXTRACT_lcu_info_debug  //控制lcu debug信息是否打印
          fprintf(p_lcu_debug, "P_IMG cu_type_index %4d\n", EcuInfoInter[0]);
#endif		
					switch(EcuInfoInter[0])//cu_type_index 文档对应
					{
					case 0:
						EcuInfoInterMv[5] = 0;//CU	skip
						if( currMB->ui_MbBitSize > MIN_CU_SIZE_IN_BIT)
						{
							EcuInfoInterMv[43]=4;//CU:	pu 个数
							puInfoInter[0][0]=4;//单前向
							puInfoInter[1][0]=4;
							puInfoInter[2][0]=4;
							puInfoInter[3][0]=4;
						}
						else if( currMB->ui_MbBitSize == MIN_CU_SIZE_IN_BIT)
						{
							EcuInfoInterMv[43]=1;//CU:	pu 个数
							puInfoInter[0][0]=4;//单前向
						} 
						
						break;
					case 1:
						EcuInfoInterMv[5] = 1;//CU	Direct
						if( currMB->ui_MbBitSize > MIN_CU_SIZE_IN_BIT)
						{
							EcuInfoInterMv[43]=4;//CU:	pu 个数
							puInfoInter[0][0]=4;//单前向
							puInfoInter[1][0]=4;
							puInfoInter[2][0]=4;
							puInfoInter[3][0]=4;
						}
						else if( currMB->ui_MbBitSize == MIN_CU_SIZE_IN_BIT)//wighted_skip_mode
						{
							EcuInfoInterMv[43]=1;//CU:	pu 个数
							puInfoInter[0][0]=4;//单前向
						} 
						break;
					case 2:
						EcuInfoInterMv[5] = 2;//CU	2Nx2N
						EcuInfoInterMv[43]=1;//CU:	pu 个数
						puInfoInter[0][0]=4;//单前向
						
						break;
					case 3:
						EcuInfoInterMv[43]=2;//CU:	pu 个数
						switch(EcuInfoInter[1])//shape_of_partion_index 文档对应
						{
						case 0:
							EcuInfoInterMv[5] = 3;//CU	2NxH
							break;
						case 1:
							EcuInfoInterMv[5] = 5;//CU	2NxHU
							break;
						case 2: 				
							EcuInfoInterMv[5] = 6;//CU	2NxHD
							break;
						}
						
						puInfoInter[0][0]=4;//单前向
						puInfoInter[0][1]=0;//CUSubType
						puInfoInter[1][0]=4;//单前向
						puInfoInter[1][1]=0;//CUSubType
						
						
						break;
					case 4:
						EcuInfoInterMv[43]=2;//CU:	pu 个数
						switch(EcuInfoInter[1])//shape_of_partion_index 文档对应
						{
						case 0:
							EcuInfoInterMv[5] = 4;//CU	2NxV
							break;
						case 1:
							EcuInfoInterMv[5] = 7;//CU	2NxVL
							break;
						case 2: 				
							EcuInfoInterMv[5] = 8;//CU	2NxVR
							break;
						}
						
						puInfoInter[0][0]=4;//单前向
						puInfoInter[0][1]=0;//CUSubType
						puInfoInter[1][0]=4;//单前向
						puInfoInter[1][1]=0;//CUSubType
						break;
					case 5:
						EcuInfoInterMv[5] = 9;//CU	NxN
						EcuInfoInterMv[43]= 4;//CU:  pu 个数
						
						puInfoInter[0][0]=4;//单前向
						puInfoInter[1][0]=4;
						puInfoInter[2][0]=4;
						puInfoInter[3][0]=4;
						break;
					case 6://intra
						if( cuInfoIntra[5] ==0 )//CU  2Nx2N
						{
							EcuInfoInterMv[5] = 10;//CU  2Nx2N
							EcuInfoInterMv[43]= 1;//CU:  pu 个数
						}
						else if( cuInfoIntra[5] ==1 )//CU  2Nx2N
						{
							switch(cuInfoIntra[6])
							{
							case 2:
								EcuInfoInterMv[5] = 11;//CU  NxN
								EcuInfoInterMv[43]= 4;//CU:  pu 个数
								break;
							case 0:
								EcuInfoInterMv[5] = 12;//CU  I_nNxN Hor
								EcuInfoInterMv[43]= 4;//CU:  pu 个数
								break;
							case 1:
								EcuInfoInterMv[5] = 13;//CU  I_NxnN Ver
								EcuInfoInterMv[43]= 4;//CU:  pu 个数
								break;
								
							default:
								fprintf(p_lcu_mv,"CU Intra error type\n",1);
								break;
							}
						}
						break;
					default:
						fprintf(p_lcu_mv,"CU error type\n");
						break;
					}
				}

				//pu 个数
				if(EcuInfoInterMv[43] == 1)//1 
				{
					EcuInfoInterMv[14]=puInfoInter[0][10];//pu0 refIdxL0
					EcuInfoInterMv[15]=puInfoInter[0][11];//pu0 refIdxL1

					EcuInfoInterMv[22]=puInfoInter[0][12];//pu0 MV L0 x
					EcuInfoInterMv[23]=puInfoInter[0][13];//pu0 MV L0 y
					EcuInfoInterMv[24]=puInfoInter[0][14];//pu0 MV L1 x
					EcuInfoInterMv[25]=puInfoInter[0][15];//pu0 MV L1 y
				}
				else if(EcuInfoInterMv[43] == 2)
				{
					if(EcuInfoInter[0] == 3 )//水平划分时
					{					
						EcuInfoInterMv[14]=puInfoInter[0][10];//pu0 refIdxL0
						EcuInfoInterMv[15]=puInfoInter[0][11];//pu0 refIdxL1
						EcuInfoInterMv[16]=puInfoInter[2][10];//pu1 refIdxL0
						EcuInfoInterMv[17]=puInfoInter[2][11];//pu1 refIdxL1

						EcuInfoInterMv[22]=puInfoInter[0][12];//pu0 MV L0 x
						EcuInfoInterMv[23]=puInfoInter[0][13];//pu0 MV L0 y
						EcuInfoInterMv[24]=puInfoInter[0][14];//pu0 MV L1 x
						EcuInfoInterMv[25]=puInfoInter[0][15];//pu0 MV L1 y

						EcuInfoInterMv[26]=puInfoInter[2][12];//pu1 MV L0 x
						EcuInfoInterMv[27]=puInfoInter[2][13];//pu1 MV L0 y
						EcuInfoInterMv[28]=puInfoInter[2][14];//pu1 MV L1 x
						EcuInfoInterMv[29]=puInfoInter[2][15];//pu1 MV L1 y
					}
					else
					{
						EcuInfoInterMv[14]=puInfoInter[0][10];//pu0 refIdxL0
						EcuInfoInterMv[15]=puInfoInter[0][11];//pu0 refIdxL1
						EcuInfoInterMv[16]=puInfoInter[1][10];//pu1 refIdxL0
						EcuInfoInterMv[17]=puInfoInter[1][11];//pu1 refIdxL1

						EcuInfoInterMv[22]=puInfoInter[0][12];//pu0 MV L0 x
						EcuInfoInterMv[23]=puInfoInter[0][13];//pu0 MV L0 y
						EcuInfoInterMv[24]=puInfoInter[0][14];//pu0 MV L1 x
						EcuInfoInterMv[25]=puInfoInter[0][15];//pu0 MV L1 y

						EcuInfoInterMv[26]=puInfoInter[1][12];//pu1 MV L0 x
						EcuInfoInterMv[27]=puInfoInter[1][13];//pu1 MV L0 y
						EcuInfoInterMv[28]=puInfoInter[1][14];//pu1 MV L1 x
						EcuInfoInterMv[29]=puInfoInter[1][15];//pu1 MV L1 y
					}
				}
				else if(EcuInfoInterMv[43] == 4)
				{				
					EcuInfoInterMv[14]=puInfoInter[0][10];//pu0 refIdxL0
					EcuInfoInterMv[15]=puInfoInter[0][11];//pu0 refIdxL1
					EcuInfoInterMv[16]=puInfoInter[1][10];//pu1 refIdxL0
					EcuInfoInterMv[17]=puInfoInter[1][11];//pu1 refIdxL1
					EcuInfoInterMv[18]=puInfoInter[2][10];//pu2 refIdxL0
					EcuInfoInterMv[19]=puInfoInter[2][11];//pu2 refIdxL1
					EcuInfoInterMv[20]=puInfoInter[3][10];//pu3 refIdxL0
					EcuInfoInterMv[21]=puInfoInter[3][11];//pu4 refIdxL1

					EcuInfoInterMv[22]=puInfoInter[0][12];//pu0 MV L0 x
					EcuInfoInterMv[23]=puInfoInter[0][13];//pu0 MV L0 y
					EcuInfoInterMv[24]=puInfoInter[0][14];//pu0 MV L1 x
					EcuInfoInterMv[25]=puInfoInter[0][15];//pu0 MV L1 y

					EcuInfoInterMv[26]=puInfoInter[1][12];//pu1 MV L0 x
					EcuInfoInterMv[27]=puInfoInter[1][13];//pu1 MV L0 y
					EcuInfoInterMv[28]=puInfoInter[1][14];//pu1 MV L1 x
					EcuInfoInterMv[29]=puInfoInter[1][15];//pu1 MV L1 y

					EcuInfoInterMv[30]=puInfoInter[2][12];//pu2 MV L0 x
					EcuInfoInterMv[31]=puInfoInter[2][13];//pu2 MV L0 y
					EcuInfoInterMv[32]=puInfoInter[2][14];//pu2 MV L1 x
					EcuInfoInterMv[33]=puInfoInter[2][15];//pu2 MV L1 y

					EcuInfoInterMv[34]=puInfoInter[3][12];//pu3 MV L0 x
					EcuInfoInterMv[35]=puInfoInter[3][13];//pu3 MV L0 y
					EcuInfoInterMv[36]=puInfoInter[3][14];//pu3 MV L1 x
					EcuInfoInterMv[37]=puInfoInter[3][15];//pu3 MV L1 y
				}

				if( img->type == B_IMG)//B 图像
				{
					for ( i = 0; i < EcuInfoInterMv[43]; i++ )
					{
						if( puInfoInter[i][0] == 4)//single_fwd 单前向
						{
							EcuInfoInterMv[14+i*2]  =1;
							EcuInfoInterMv[14+i*2+1]=-1;
						}
					  else if( puInfoInter[i][0] == 3)//bck 后向
						{
							EcuInfoInterMv[14+i*2]  =-1;
							EcuInfoInterMv[14+i*2+1]=0;
						}
						//对称、双向的前向索引为1，后向索引为0
						else
						{
							EcuInfoInterMv[14+i*2]  =1;
							EcuInfoInterMv[14+i*2+1]=0;
						}
					}				 
				}
				  
        //fprintf(p_lcu,"CU CuType %d\n", EcuInfoInterMv[5]);
        //fprintf(p_lcu,"PU num %d\n", EcuInfoInterMv[43]);
        fprintf(p_lcu,"%08X\n", EcuInfoInterMv[5]);
        //fprintf(p_lcu,"%08X\n", EcuInfoInterMv[43]);
#if EXTRACT_lcu_info_debug  //控制lcu debug信息是否打印
				fprintf(p_lcu_debug,"part mode %4d\n",EcuInfoInterMv[5]);
        fprintf(p_lcu_debug, "PU number %4d\n", EcuInfoInterMv[43]);
#endif		

				//5个帧内信息 ，
				for ( i = 0; i < 5; i++ )
				{
					fprintf(p_lcu,"%08X\n",0);
				}
				// EcuInfoInterMv[43]个pu 信息 ，
				for ( i = 0; i < EcuInfoInterMv[43]; i++ )
	      {
#if EXTRACT_08X

					fprintf(p_lcu,"%08X\n", puInfoInter[i][0]);
					fprintf(p_lcu,"%08X\n", puInfoInter[i][1]);
					fprintf(p_lcu,"%08X\n", puInfoInter[i][2]);
					fprintf(p_lcu,"%08X\n", EcuInfoInterSyntax[19] );//EcuInfoInterSyntax[19]   F  DMH

					fprintf(p_lcu,"%08X\n",	EcuInfoInterSyntax[15+i]);    //puInfoInter[i][4]);
					fprintf(p_lcu,"%08X\n", EcuInfoInterSyntax[4]);           //EcuInfoInterSyntax[4] 存放句法wighted_skip_mode		默认0		
					fprintf(p_lcu,"%08X\n",	EcuInfoInterSyntax[20+i*2]);  //puInfoInter[i][6]);
					fprintf(p_lcu,"%08X\n",	EcuInfoInterSyntax[20+i*2+1]);//puInfoInter[i][7]);
					fprintf(p_lcu,"%08X\n",	EcuInfoInterSyntax[28+i*2]);  //puInfoInter[i][8]);
					fprintf(p_lcu,"%08X\n",	EcuInfoInterSyntax[28+i*2+1]);//puInfoInter[i][9]);
#endif
#if EXTRACT_D

					fprintf(p_lcu,"PU %d dir PredMode:%d\n",	i,puInfoInter[i][0]);
					fprintf(p_lcu,"PU %d CuSubType:%d\n",		i,puInfoInter[i][1]);
					fprintf(p_lcu,"PU %d BNxNType:%d\n",		i,puInfoInter[i][2]);
					fprintf(p_lcu,"PU %d MHS_mode:%d\n",		i,EcuInfoInterSyntax[19] );


					fprintf(p_lcu,"PU %d ref_idx:%d\n",		i,EcuInfoInterSyntax[15+i]);//puInfoInter[i][4]);
					fprintf(p_lcu,"PU %d WeightedSkipMode:%d\n",	i,puInfoInter[i][5]);
					fprintf(p_lcu,"PU %d mvd_l0_x:%d\n",		i,EcuInfoInterSyntax[20+i*2]);//puInfoInter[i][6]);
					fprintf(p_lcu,"PU %d mvd_l0_y:%d\n",		i,EcuInfoInterSyntax[20+i*2+1]);//puInfoInter[i][7]);
					fprintf(p_lcu,"PU %d mvd_l1_x:%d\n",		i,EcuInfoInterSyntax[28+i*2]);//puInfoInter[i][8]);
					fprintf(p_lcu,"PU %d mvd_l1_y:%d\n",		i,EcuInfoInterSyntax[28+i*2+1]);//puInfoInter[i][9]);
#endif
#if EXTRACT_lcu_info_debug
          //EcuInfoInterSyntax[3] 存放句法f_pu_type_index Table74	默认0
          fprintf(p_lcu_debug,"f_pu_type_index %d\n", EcuInfoInterSyntax[3]);
					fprintf(p_lcu_debug,"PU %d dir PredMode:%d\n",	i,puInfoInter[i][0]);
					fprintf(p_lcu_debug,"PU %d CuSubType:%d\n", 	i,puInfoInter[i][1]);
					fprintf(p_lcu_debug,"PU %d BNxNType:%d\n",		i,puInfoInter[i][2]);
					fprintf(p_lcu_debug,"PU %d MHS_mode:%d\n",		i,EcuInfoInterSyntax[19] );


					fprintf(p_lcu_debug,"PU %d ref_idx:%d\n", 	i,EcuInfoInterSyntax[15+i]);//puInfoInter[i][4]);
					fprintf(p_lcu_debug,"PU %d WeightedSkipMode:%d\n",	i,puInfoInter[i][5]);
					fprintf(p_lcu_debug,"PU %d mvd_l0_x:%d\n",		i,EcuInfoInterSyntax[20+i*2]);//puInfoInter[i][6]);
					fprintf(p_lcu_debug,"PU %d mvd_l0_y:%d\n",		i,EcuInfoInterSyntax[20+i*2+1]);//puInfoInter[i][7]);
					fprintf(p_lcu_debug,"PU %d mvd_l1_x:%d\n",		i,EcuInfoInterSyntax[28+i*2]);//puInfoInter[i][8]);
					fprintf(p_lcu_debug,"PU %d mvd_l1_y:%d\n",		i,EcuInfoInterSyntax[28+i*2+1]);//puInfoInter[i][9]);
#endif

				}
				//若不到4个pu，剩余的pu 全部打印0
				for ( i = EcuInfoInterMv[43]; i < 4; i++ )
				{
					int j;
					for ( j = 0; j < 10; j++ )
					{
#if EXTRACT_08X
						fprintf(p_lcu,"%08X\n",0);
#endif
#if EXTRACT_D
						fprintf(p_lcu,"%d\n",0);
#endif
					}
				}

				//76	transform_split_flag	32	1
#if EXTRACT_08X
				fprintf(p_lcu,"%08X\n",EcuInfoInterSyntax[37]);

#endif
#if EXTRACT_D
				fprintf(p_lcu,"Cu transform_split_flag %d\n",EcuInfoInterSyntax[37]);
#endif			
#if EXTRACT_lcu_info_debug
        fprintf(p_lcu_debug, "Cu transform_split_flag %d\n", EcuInfoInterSyntax[37]);
#endif			
				
				//77	CuCTP	32	1	
#if EXTRACT_08X
				fprintf(p_lcu,"%08X\n",EcuInfoInterSyntax[36]);
#endif
#if EXTRACT_D
				fprintf(p_lcu,"CuCTP %d\n",EcuInfoInterSyntax[36]);
#endif			
#if EXTRACT_lcu_info_debug
        fprintf(p_lcu_debug, "CuCTP %d\n", EcuInfoInterSyntax[36]);
#endif			
				//78	CurrentQP	32	1	当前Lcu的QP
#if EXTRACT_08X
				fprintf(p_lcu,"%08X\n",EcuInfoInterSyntax[39]);
#endif
#if EXTRACT_D
				fprintf(p_lcu,"Cu QP %d\n",EcuInfoInterSyntax[39]);
#endif		
#if EXTRACT_lcu_info_debug
        fprintf(p_lcu_debug, "Cu QP %d\n", EcuInfoInterSyntax[39]);
#endif								

#if EXTRACT_08X
				fprintf(p_lcu_mv,"%08X\n", 0);//pred_mode_flag 0:inter 1:intra
				fprintf(p_lcu_mv,"%08X\n", EcuInfoInterMv[5]);
				//fprintf(p_lcu_mv,"%08X\n", EcuInfoInterMv[43]);
#endif

#if EXTRACT_D
				fprintf(p_lcu_mv,"CU CuType %d\n", EcuInfoInterMv[5]);
				fprintf(p_lcu_mv,"PU num %d\n", EcuInfoInterMv[43]);
#endif			

				//8个predflag全为0
				for ( i = 0; i < 8; i++ )
				{
					fprintf(p_lcu_mv,"%08X\n",0);
				}
				for ( i = 0; i < EcuInfoInterMv[43]; i++ )
				{
#if EXTRACT_08X              
					fprintf(p_lcu_mv,"%08X\n",	EcuInfoInterMv[14+i*2]);
					fprintf(p_lcu_mv,"%08X\n",	EcuInfoInterMv[14+i*2+1]);
#endif

#if EXTRACT_D// EXTRACT_08X //
					fprintf(p_lcu_mv,"PU %d ref_idxL0:%d\n",  i,EcuInfoInterMv[14+i*2]);
					fprintf(p_lcu_mv,"PU %d ref_idxL1:%d\n",  i,EcuInfoInterMv[14+i*2+1]);
#endif      
#if EXTRACT_lcu_info_debug// EXTRACT_08X //
					fprintf(p_lcu_debug,"PU %d ref_idxL0:%d\n",	i,EcuInfoInterMv[14+i*2]);
					fprintf(p_lcu_debug,"PU %d ref_idxL1:%d\n",	i,EcuInfoInterMv[14+i*2+1]);
#endif 

				}
				//若不到4个pu，剩余的pu 全部打印0
				for ( i = EcuInfoInterMv[43]; i < 4; i++ )
				{
#if EXTRACT_08X            
					fprintf(p_lcu_mv,"%08X\n",	0);
					fprintf(p_lcu_mv,"%08X\n",	0);
#endif

#if EXTRACT_D            
					fprintf(p_lcu_mv,"PU %d ref_idxL0:%d\n",	i,0);
					fprintf(p_lcu_mv,"PU %d ref_idxL1:%d\n",	i,0);
#endif
				}
				for ( i = 0; i < EcuInfoInterMv[43]; i++ )
				{
#if EXTRACT_08X            
					fprintf(p_lcu_mv,"%08X\n",	EcuInfoInterMv[22+i*4]);
					fprintf(p_lcu_mv,"%08X\n",	EcuInfoInterMv[22+i*4+1]);
					fprintf(p_lcu_mv,"%08X\n",	EcuInfoInterMv[22+i*4+2]);
					fprintf(p_lcu_mv,"%08X\n",	EcuInfoInterMv[22+i*4+3]);
#endif

#if EXTRACT_D            
					fprintf(p_lcu_mv,"PU %d MV L0 x:%d\n",	i,EcuInfoInterMv[22+i*4]);
					fprintf(p_lcu_mv,"PU %d MV L0 y:%d\n",	i,EcuInfoInterMv[22+i*4+1]);
					fprintf(p_lcu_mv,"PU %d MV L1 x:%d\n",	i,EcuInfoInterMv[22+i*4+2]);
					fprintf(p_lcu_mv,"PU %d MV L1 y:%d\n",	i,EcuInfoInterMv[22+i*4+3]);
#endif
#if EXTRACT_lcu_info_debug            
					fprintf(p_lcu_debug,"PU %d MV L0 x:%d\n",	i,EcuInfoInterMv[22+i*4]);
					fprintf(p_lcu_debug,"PU %d MV L0 y:%d\n",	i,EcuInfoInterMv[22+i*4+1]);
					fprintf(p_lcu_debug,"PU %d MV L1 x:%d\n",	i,EcuInfoInterMv[22+i*4+2]);
					fprintf(p_lcu_debug,"PU %d MV L1 y:%d\n",	i,EcuInfoInterMv[22+i*4+3]);
#endif
				}
				//若不到4个pu，剩余的pu 全部打印0
				for ( i = EcuInfoInterMv[43]; i < 4; i++ )
				{
#if EXTRACT_08X            
					fprintf(p_lcu_mv,"%08X\n",	0);
					fprintf(p_lcu_mv,"%08X\n",	0);
					fprintf(p_lcu_mv,"%08X\n",	0);
					fprintf(p_lcu_mv,"%08X\n",	0);
#endif

#if EXTRACT_D            
					fprintf(p_lcu_mv,"PU %d MV L0 x:%d\n",	i,0);
					fprintf(p_lcu_mv,"PU %d MV L0 y:%d\n",	i,0);
					fprintf(p_lcu_mv,"PU %d MV L1 x:%d\n",	i,0);
					fprintf(p_lcu_mv,"PU %d MV L1 y:%d\n",	i,0);
#endif
				}
				//5个帧内信息 ，
				for ( i = 0; i < 5; i++ )
				{
					fprintf(p_lcu_mv,"%08X\n",0);
				}
			}		
//fprintf(p_lcu,"b8mode=%d %d %d %d\n",currMB->b8pdir[0],currMB->b8pdir[1],currMB->b8pdir[2],currMB->b8pdir[3]);

#endif	  
      img->current_mb_nr = tmp_pos;
    }
  }

  return DECODE_MB;
}


void read_ipred_block_modes ( int b8, unsigned int uiPositionInPic )
{
  int bi, bj, dec, i, j;
  SyntaxElement currSE;
  codingUnit *currMB = img->mb_data + uiPositionInPic; //current_mb_nr
  int mostProbableIntraPredMode[2];
  int upIntraPredMode;
  int leftIntraPredMode;
  Slice *currSlice = img->currentSlice;
  DataPartition *dP;
  int N8_SizeScale = 1 << ( currMB->ui_MbBitSize - MIN_CU_SIZE_IN_BIT );
  int block8_y = ( uiPositionInPic / img->PicWidthInMbs ) << 1;
  int block8_x = ( uiPositionInPic % img->PicWidthInMbs ) << 1;

  currSE.type = SE_INTRAPREDMODE;
#if TRACE
  strncpy ( currSE.tracestring, "Ipred Mode", TRACESTRING_SIZE );
#endif

  if ( b8 < 4 )
  {
    if ( currMB->b8mode[b8] == IBLOCK )
    {
      dP = & ( currSlice->partArr[0] );
      currSE.reading = readIntraPredMode;
	    dP->readSyntaxElement ( &currSE, dP, currMB, uiPositionInPic );
	    if(currMB->cuType==InNxNMB)
	    {
		    bi = block8_x ;
		    bj = block8_y + b8*(1<<(currMB->ui_MbBitSize-4)); ;
	    }
	    else if(currMB->cuType==INxnNMB )
	    {
		    bi = block8_x+b8*(1<<(currMB->ui_MbBitSize-4)); ;
		    bj = block8_y  ;
	    }
      //get from array and decode
	    else
	    {
		    //get from array and decode
		    bi = block8_x + ( b8 & 1 ) * N8_SizeScale;
		    bj = block8_y + ( b8 / 2 ) * N8_SizeScale;
	    }

      upIntraPredMode            = img->ipredmode[bj][bi + 1];
      leftIntraPredMode          = img->ipredmode[bj + 1][bi];

      upIntraPredMode  = ( upIntraPredMode < 0 ) ? DC_PRED : upIntraPredMode ;
      leftIntraPredMode  = ( leftIntraPredMode < 0 ) ? DC_PRED : leftIntraPredMode ;
      mostProbableIntraPredMode[0]=min(upIntraPredMode, leftIntraPredMode);
      mostProbableIntraPredMode[1]=max(upIntraPredMode, leftIntraPredMode);
      if(mostProbableIntraPredMode[0]==mostProbableIntraPredMode[1])
      {
#if INTRA_2MPM_FIX
        mostProbableIntraPredMode[0]=DC_PRED;
        mostProbableIntraPredMode[1]=(mostProbableIntraPredMode[1]==DC_PRED) ? BI_PRED : mostProbableIntraPredMode[1];
#else
        mostProbableIntraPredMode[1]=(mostProbableIntraPredMode[0]!=DC_PRED) ? DC_PRED : BI_PRED;
#endif
      }
#if INTRA_2MPM_FIX
      dec = ( currSE.value1 < 0 ) ? mostProbableIntraPredMode[currSE.value1+2] : currSE.value1 + ( currSE.value1 >= mostProbableIntraPredMode[0] ) + ( currSE.value1+1 >= mostProbableIntraPredMode[1] );
#else
      dec = ( currSE.value1 < 0 ) ? mostProbableIntraPredMode[currSE.value1+2] : currSE.value1 + ( currSE.value1 >= mostProbableIntraPredMode[0] );
#endif
#if EXTRACT	  
	    cuInfoIntra[b8]= dec;//intra pu  pred mode
#if EXTRACT_lcu_info_debug
	    fprintf(p_lcu_debug,"b8=%d intra_luma_pred_mode %d\n", b8,dec);//intra_luma_pred_mode
#endif
#endif

      //set
#if INTRA_2N
      if ( currMB->trans_size == 0 )
      {
        int k = 0;
        for ( k = 0; k < 4; k++ )
        {
          bi = block8_x + ( k & 1 ) * N8_SizeScale;
          bj = block8_y + ( k / 2 ) * N8_SizeScale;
          for ( i = 0; i < N8_SizeScale; i++ )
          {
            for ( j = 0; j < N8_SizeScale; j++ )
            {
              img->ipredmode[1 + bj + j][1 + bi + i] = dec;
            }
          }
        }
      }
      else
      {
		    if(currMB->cuType==InNxNMB )
		    {
			    i = b8*(1<<(currMB->ui_MbBitSize-4));
			    for ( j = 0; j < ( 1 << ( currMB->ui_MbBitSize+1 - MIN_CU_SIZE_IN_BIT ) ); j++ )
			    {
				    if(currMB->ui_MbBitSize==4)
				    {
              img->ipredmode[1 + block8_y + i][1 + block8_x + j] = dec;
				    }
				    else
				    {
					    img->ipredmode[1 + block8_y + i][1 + block8_x + j] = dec;
					    img->ipredmode[1 + block8_y + i+1][1 + block8_x + j] = dec;
				    }
			    }
		    }
		    else if(currMB->cuType==INxnNMB)
		    {
			    i = b8*(1<<(currMB->ui_MbBitSize-4));
			    for ( j = 0; j < ( 1 << ( currMB->ui_MbBitSize+1 - MIN_CU_SIZE_IN_BIT ) ); j++ )
			    {
				    if(currMB->ui_MbBitSize==4)
				    {
					    img->ipredmode[1 + block8_y + j][1 + block8_x + i] = dec;
				    }
				    else
				    {
					    img->ipredmode[1 + block8_y + j][1 + block8_x + i] =dec;
					    img->ipredmode[1 + block8_y + j][1 + block8_x + i+1] = dec;
				    }
			    }		   
		    }
		    else 
		    {
          for ( i = 0; i < N8_SizeScale; i++ )
          {
            for ( j = 0; j < N8_SizeScale; j++ )
            {
              img->ipredmode[1 + bj + j][1 + bi + i] = dec;
            }
          }
		    }
      }
      currMB->intra_pred_modes[b8] = dec;
#else
      for ( i = 0; i < N8_SizeScale; i++ )
      {
        for ( j = 0; j < N8_SizeScale; j++ )
        {
          img->ipredmode[1 + bj + j][1 + bi + i] = dec;
        }
      }
#endif
    }
  }
  else if ( b8 == 4 && currMB->b8mode[b8 - 3] == IBLOCK )
  {

    currSE.type = SE_INTRAPREDMODE;
#if TRACE
    strncpy ( currSE.tracestring, "Chroma intra pred mode", TRACESTRING_SIZE );
#endif

    dP = & ( currSlice->partArr[0] );
    currSE.reading = readCIPredMode;
    dP->readSyntaxElement ( &currSE, dP, currMB, uiPositionInPic );
#if EXTRACT
	  cuInfoIntra[b8]= dec;//intra pu  chroma pred mode
    //fprintf(p_lcu,"intra_chroma_pred_mode %d\n",currSE.value1);//intra_chroma_pred_mode
#endif

    for ( i = 0; i < N8_SizeScale; i++ )
    {
      int pos = uiPositionInPic + i * img->PicWidthInMbs;

      for ( j = 0; j < N8_SizeScale; j++, pos++ )
      {
        codingUnit* tmpMB = &img->mb_data[pos];
        tmpMB->c_ipred_mode = currSE.value1;
      }
    }

    if ( currSE.value1 < 0 || currSE.value1 >= NUM_INTRA_PMODE_CHROMA )
    {
      printf ( "%d\n", uiPositionInPic );
      error ( "illegal chroma intra pred mode!\n", 600 );
    }
  }
}

/*
*************************************************************************
* Function:Set context for reference frames
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

int BType2CtxRef ( int btype )
{
  if ( btype < 4 )
  {
    return 0;
  }
  else
  {
    return 1;
  }
}

/*
*************************************************************************
* Function:
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
void readReferenceIndex ( unsigned int uiBitSize, unsigned int uiPositionInPic )
{
  int k;
  codingUnit *currMB  = &img->mb_data[uiPositionInPic];
  SyntaxElement currSE;
  int bframe          = ( img->type == B_IMG );
  int partmode        = currMB->cuType;
  int step_h0         = BLOCK_STEP [partmode][0];
  int step_v0         = BLOCK_STEP [partmode][1];

  int i0, j0, refframe;

  Slice *currSlice    = img->currentSlice;
  DataPartition *dP;

  int r, c;
  int N8_SizeScale = 1 << ( uiBitSize - MIN_CU_SIZE_IN_BIT );
  int block8_y = ( uiPositionInPic / img->PicWidthInMbs ) << 1;
  int block8_x = ( uiPositionInPic % img->PicWidthInMbs ) << 1;

  //  If multiple ref. frames, read reference frame for the MB *********************************

#ifdef AVS2_S2_S
#if AVS2_SCENE_CD
  if(img->typeb == BP_IMG)
#else
  if(input->profile_id == 0x50 && img->typeb == BP_IMG)
#endif
	  return;
#endif

  currSE.type = SE_REFFRAME;
  currSE.mapping = linfo_ue;
#if EXTRACT //_lcu_info_BAC
	int puNumInCU=0;
#endif	

  for ( j0 = 0; j0 < 2; )
  {
#if INTRA_2N
    if ( ( currMB->cuType == I8MB || currMB->cuType == I16MB ||currMB->cuType==InNxNMB ||currMB->cuType==INxnNMB) && j0 == 0 )
#else
    if ( ( currMB->cuType == I8MB && j0 == 0 ) )
#endif
    {
      j0 += 1;
      continue;
    }

    for ( i0 = 0; i0 < 2; )
    {
      int b8_x, b8_y;

      k = 2 * j0 + i0;
	    if ( ( currMB->b8pdir[k] == FORWARD || currMB->b8pdir[k] == SYM || currMB->b8pdir[k] == BID || currMB->b8pdir[k] == DUAL ) && currMB->b8mode[k] != 0 ) //non skip (direct)
      {
        int start_x, start_y, tmp_step_h, tmp_step_v;
        get_b8_offset( currMB->cuType, uiBitSize, i0, j0, &start_x, &start_y, &tmp_step_h, &tmp_step_v );
        img->subblock_x = start_x;
        img->subblock_y = start_y;

#if !M3595_REMOVE_REFERENCE_FLAG
        if ( !picture_reference_flag && ((img->type == F_IMG)||(img->type == P_IMG)) && img->types != I_IMG )
#else 
        if ( img->num_of_references > 1 && ((img->type == F_IMG)||(img->type == P_IMG)) && img->types != I_IMG )
#endif 

        {
#if TRACE
          strncpy ( currSE.tracestring,  "Fwd ref frame no ", TRACESTRING_SIZE );
#endif
          currSE.context = BType2CtxRef ( currMB->b8mode[k] );

          currSE.len = 1;

          currSE.type = SE_REFFRAME;
          dP = & ( currSlice->partArr[0] );
          currSE.reading = readRefFrame;
          currSE.value2 = 0;
          dP->readSyntaxElement ( &currSE, dP, currMB, uiPositionInPic );
#if TRACE
          fprintf ( p_trace, "Fwd Ref frame no  = %3d \n", currSE.value1 );
#endif
#if EXTRACT //_lcu_info_BAC

#if EXTRACT_lcu_info_debug
          fprintf(p_lcu_debug, "READ FORWARD pu_reference_index  =%d puNumInCU=%d\n", currSE.value1, puNumInCU);
#endif
		      //EcuInfoInterSyntax[15-18] 存放句法前向PF pu_reference_index   4个PU的参考帧索引
		      EcuInfoInterSyntax[15+puNumInCU]=currSE.value1;//保存
		      puNumInCU++;

#if EXTRACT_lcu_info_BAC 		  
		      fprintf(p_lcu,"Fwd Ref frame idx = %d\n",currSE.value1);//pu_reference_index   
#endif

#endif		
          refframe = currSE.value1;
        }
        else
        {
          refframe = 0;
        }

        if ( !bframe )
        {
          b8_x = block8_x + start_x;
          b8_y = block8_y + start_y;
          for ( r = 0; r < step_v0 * tmp_step_v; r++ )
          {
            for ( c = 0; c < step_h0 * tmp_step_h; c++ )
            {
              refFrArr[b8_y + r][b8_x + c] = refframe;
              p_snd_refFrArr[b8_y + r][b8_x + c] = -1;
            }
          }
        }
        else // !! for B frame shenyanfei
        {
          b8_x = block8_x + start_x;
          b8_y = block8_y + start_y;
          for ( r = 0; r < step_v0 * tmp_step_v; r++ )
          {
            for ( c = 0; c < step_h0 * tmp_step_h; c++ )
            {
              img->fw_refFrArr[b8_y + r][b8_x + c] = refframe;
            }
          }
        }
      }

      i0 += max ( 1, step_h0 );
    }

    j0 += max ( 1, step_v0 );
  }

  for ( j0 = 0; j0 < 2; )
  {
#if INTRA_2N
    if ( ( currMB->cuType == I8MB || currMB->cuType == I16MB ||currMB->cuType==InNxNMB ||currMB->cuType==INxnNMB) && j0 == 0 )
#else
    if ( ( currMB->cuType == I8MB && j0 == 0 ) )
#endif
    {
      j0 += 1;
      continue;
    }

    for ( i0 = 0; i0 < 2; )
    {
      int b8_x, b8_y;
      k = 2 * j0 + i0;

	    if ( currMB->b8pdir[k] == DUAL && currMB->b8mode[k] != 0 ) //non skip (direct)
	    {
		    int start_x, start_y, tmp_step_h, tmp_step_v;
		    get_b8_offset( currMB->cuType, uiBitSize, i0, j0, &start_x, &start_y, &tmp_step_h, &tmp_step_v );
		    img->subblock_x = start_x;
		    img->subblock_y = start_y;

#if !M3595_REMOVE_REFERENCE_FLAG
		    if ( !picture_reference_flag && currMB->b8pdir[k] != DUAL && dhp_enabled && img->type == F_IMG && img->types != I_IMG )
#else 
		    if ( img->num_of_references > 1 && currMB->b8pdir[k] != DUAL && dhp_enabled && img->type == F_IMG && img->types != I_IMG )
#endif 
		    {
  #if TRACE
			    strncpy ( currSE.tracestring,  "Fwd snd ref frame no ", TRACESTRING_SIZE );
  #endif
			    currSE.context = BType2CtxRef ( currMB->b8mode[k] );

          currSE.len = 1;

			    currSE.type = SE_REFFRAME;
			    dP = & ( currSlice->partArr[0] );
			    currSE.reading = readRefFrame;
			    currSE.value2 = 1;
			    dP->readSyntaxElement ( &currSE, dP, currMB, uiPositionInPic );
  #if TRACE
			    fprintf ( p_trace, "Fwd Snd Ref frame no  = %3d \n", currSE.value1 );
  #endif
  #if EXTRACT //_lcu_info_BAC
			    fprintf(p_lcu,"Fwd Snd Ref frame idx = %d\n",currSE.value1);//pu_reference_index   
  #endif	

			    refframe = currSE.value1;
		    }
		    else
		    {
			    refframe = 0;
		    }
			  b8_x = block8_x + start_x;
			  b8_y = block8_y + start_y;
			  for ( r = 0; r < step_v0 * tmp_step_v; r++ )
			  {
				  for ( c = 0; c < step_h0 * tmp_step_h; c++ )
				  {
#if DHP_OPT
					  p_snd_refFrArr[b8_y + r][b8_x + c] = refFrArr[b8_y + r][b8_x + c] == 0 ? 1 : 0;
#else
					  p_snd_refFrArr[b8_y + r][b8_x + c] = refframe;
#endif
				  }
			  }
	  }

      if ( ( currMB->b8pdir[k] == BACKWARD || currMB->b8pdir[k] == BID ) && currMB->b8mode[k] != 0 ) // non skip(direct) - backward
      {
        int start_x, start_y, tmp_step_h, tmp_step_v;
        get_b8_offset( currMB->cuType, uiBitSize, i0, j0, &start_x, &start_y, &tmp_step_h, &tmp_step_v );
        img->subblock_x = start_x;
        img->subblock_y = start_y;

        refframe = 0;
        b8_x = block8_x + start_x;
        b8_y = block8_y + start_y;
        for ( r = 0; r < step_v0 * tmp_step_v; r++ )
        {
          for ( c = 0; c < step_h0 * tmp_step_h; c++ )
          {
            img->bw_refFrArr[b8_y + r][b8_x + c] = refframe;
          }
        }
      }

      i0 += max ( 1, step_h0 );
    }

    j0 += max ( 1, step_v0 );
  }
}

static __inline int pmvr_sign(int val)
{
	if (val > 0)
		return 1;
	else if (val < 0)
		return -1;
	else return 0;
}
void pmvr_mv_derivation(int mv[2], int mvd[2], int mvp[2])
{
	int ctrd[2];
	ctrd[0] = ((mvp[0] >> 1) << 1) - mvp[0];
	ctrd[1] = ((mvp[1] >> 1) << 1) - mvp[1];
	if (abs(mvd[0] - ctrd[0]) > TH)
	{
		mv[0] = mvp[0] + (mvd[0]<<1) - ctrd[0] - pmvr_sign(mvd[0] - ctrd[0])*TH;
		mv[1] = mvp[1] + (mvd[1]<<1) + ctrd[1];
	}
	else if (abs(mvd[1] - ctrd[1]) > TH)
	{
		mv[0] = mvp[0] + (mvd[0]<<1) + ctrd[0];
		mv[1] = mvp[1] + (mvd[1]<<1) - ctrd[1] - pmvr_sign(mvd[1] - ctrd[1])*TH;
	}
	else
	{
		mv[0] = mvd[0] + mvp[0];
		mv[1] = mvd[1] + mvp[1];
	}
#if m3469
	mv[0] = Clip3(-32768, 32767, mv[0]);
	mv[1] = Clip3(-32768, 32767, mv[1]);
#endif
}

/*
*************************************************************************
* Function:
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
void readMotionVector ( unsigned int uiBitSize, unsigned int uiPositionInPic )
{
  int i, j, k, l, m, n;
  int curr_mvd;
  int dmh_mode = 0;
  codingUnit *currMB  = &img->mb_data[uiPositionInPic];
  SyntaxElement currSE;
  int bframe          = ( img->type == B_IMG );
  int partmode = currMB->cuType;
  int step_h0         = BLOCK_STEP [partmode][0];
  int step_v0         = BLOCK_STEP [partmode][1];
  Slice *currSlice    = img->currentSlice;
  DataPartition *dP;

  int i0, j0, refframe;
  int pmv[2];
  int j8, i8, ii, jj;
  int vec;

  int iTRb, iTRp, iTRd;
  int frame_no_next_P, frame_no_B;
  int  delta_P[4];
  int ref;
  int fw_refframe;

  int scale_refframe, iTRp1;
  int block8_y = ( uiPositionInPic / img->PicWidthInMbs ) << 1;
  int block8_x = ( uiPositionInPic % img->PicWidthInMbs ) << 1;
  int N8_SizeScale = 1 << ( uiBitSize - MIN_CU_SIZE_IN_BIT );

  int num_of_orgMB_in_row = N8_SizeScale;//4:1  5:2  6:4
  int num_of_orgMB_in_col = N8_SizeScale;
  int size = 1 << currMB->ui_MbBitSize;
  int pix_x = ( uiPositionInPic % img->PicWidthInMbs ) << MIN_CU_SIZE_IN_BIT;
  int pix_y = ( uiPositionInPic / img->PicWidthInMbs ) << MIN_CU_SIZE_IN_BIT;
  int start_x, start_y, step_h, step_v;
  int pmvr_mvd[2];
  int pmvr_mv[2];

  if ( pix_x + size >= img->width )
  {
    num_of_orgMB_in_col = min ( N8_SizeScale, ( img->width - pix_x ) >> MIN_CU_SIZE_IN_BIT );
  }

  if ( pix_y + size >= img->height )
  {
    num_of_orgMB_in_row = min ( N8_SizeScale, ( img->height - pix_y ) >> MIN_CU_SIZE_IN_BIT );
  }

  if ( bframe && IS_P8x8 ( currMB ) )
  {
    for ( i = 0; i < 4; i++ )
    {
      if ( currMB->b8mode[i] == 0 )
      {
        int pos_x, pos_y;

        pos_x = ( i % 2 ) * N8_SizeScale;
        pos_y = ( i / 2 ) * N8_SizeScale;
        for ( j = 0; j < num_of_orgMB_in_row; j++ )
        {
          for ( k = 0; k < num_of_orgMB_in_col; k++ )
          {
            img->fw_refFrArr[block8_y + pos_y + j][block8_x + pos_x + k] = 0;
            img->bw_refFrArr[block8_y + pos_y + j][block8_x + pos_x + k] = 0;
          }
        }
      }
    }
  }
#if EXTRACT //_lcu_info_BAC
	  int puNumInCU=0;
#endif	

  //=====  READ FORWARD MOTION VECTORS =====
  currSE.type = SE_MVD;
#if EXTRACT_lcu_info_debug
		fprintf(p_lcu_debug,"READ FORWARD MOTION VECTORS  puNumInCU=%d \n",puNumInCU);
#endif

  currSE.mapping = linfo_se;

  for ( j0 = 0; j0 < 2; )
  {
#if INTRA_2N
    if ( ( currMB->cuType == I8MB || currMB->cuType == I16MB ||currMB->cuType==InNxNMB ||currMB->cuType==INxnNMB) && j0 == 0 ) // for  by jhzheng [2004/08/02]
#else
    if ( ( currMB->cuType == I8MB && j0 == 0 ) ) // for  by jhzheng [2004/08/02]
#endif
    {
      j0 += 1;
      continue;
    }
	
    for ( i0 = 0; i0 < 2; )
    {
      k = 2 * j0 + i0;
	  	if ( ( currMB->b8pdir[k] == FORWARD || currMB->b8pdir[k] == SYM || currMB->b8pdir[k] == BID || currMB->b8pdir[k] == DUAL ) && ( currMB->b8mode[k] != 0 ) ) //has forward vector
      {
        get_b8_offset( currMB->cuType, currMB->ui_MbBitSize, i0, j0, &start_x, &start_y, &step_h, &step_v );

        j8 = block8_y + start_y ;
        i8 = block8_x + start_x ;
        if ( !bframe )
        {
          refframe = refFrArr        [ j8 ][ i8 ];
        }
        else
        {
          refframe = img->fw_refFrArr[ j8 ][ i8 ];
        }

        // first make mv-prediction
        if ( !bframe )
        {
          int pix_start_x, pix_start_y, pix_step_h, pix_step_v;
          get_pix_offset( currMB->cuType, currMB->ui_MbBitSize, i0, j0, &pix_start_x, &pix_start_y, &pix_step_h, &pix_step_v );
          pix_step_h = ( currMB->cuType == 0 || currMB->cuType == 1 || currMB->cuType == 2 || currMB->cuType == 4 || currMB->cuType == 5 ) ? pix_step_h * 2 : pix_step_h;
          pix_step_v = ( currMB->cuType == 0 || currMB->cuType == 1 || currMB->cuType == 3 || currMB->cuType == 6 || currMB->cuType == 7 ) ? pix_step_v * 2 : pix_step_v;

          SetMotionVectorPredictor ( uiBitSize, uiPositionInPic, pmv, refFrArr , img->tmp_mv,  refframe, pix_start_x, pix_start_y, pix_step_h , pix_step_v, 0, 0 ); //Lou 1016
         
        }
        else
        {
          int pix_start_x, pix_start_y, pix_step_h, pix_step_v;
          get_pix_offset( currMB->cuType, currMB->ui_MbBitSize, i0, j0, &pix_start_x, &pix_start_y, &pix_step_h, &pix_step_v );
          pix_step_h = ( currMB->cuType == 0 || currMB->cuType == 1 || currMB->cuType == 2 || currMB->cuType == 4 || currMB->cuType == 5 ) ? pix_step_h * 2 : pix_step_h;
          pix_step_v = ( currMB->cuType == 0 || currMB->cuType == 1 || currMB->cuType == 3 || currMB->cuType == 6 || currMB->cuType == 7 ) ? pix_step_v * 2 : pix_step_v;

          SetMotionVectorPredictor ( uiBitSize, uiPositionInPic, pmv, img->fw_refFrArr, img->fw_mv, refframe, pix_start_x, pix_start_y, pix_step_h , pix_step_v, 0, 0 ); //Lou 1016
        }

#ifdef AVS2_S2_S
				if (  img->type == F_IMG && b_dmh_enabled && currMB->b8pdir[0] == FORWARD && currMB->b8pdir[1] == FORWARD && currMB->b8pdir[2] == FORWARD && currMB->b8pdir[3] == FORWARD && img->typeb !=BP_IMG)
#else
				if ( img->type == F_IMG  && b_dmh_enabled && currMB->b8pdir[0] == FORWARD && currMB->b8pdir[1] == FORWARD && currMB->b8pdir[2] == FORWARD && currMB->b8pdir[3] == FORWARD)
#endif
        {
#if SIMP_MV
          if (k == 0 && (!(uiBitSize == B8X8_IN_BIT && currMB->cuType >= P2NXN && currMB->cuType <= PVER_RIGHT)))
#else
          if ( k == 0 )
#endif
          {
            dP = & ( currSlice->partArr[0] );
            currSE.type = SE_DMH;
            currSE.reading = readDmhMode;
            currSE.value2 = currMB->ui_MbBitSize; // only used for context determination
            dP->readSyntaxElement ( &currSE, dP, currMB, uiPositionInPic );
            dmh_mode = currSE.value1;
#if EXTRACT
				    EcuInfoInterSyntax[19] = currSE.value1;
				    //fprintf(p_lcu,"F dmh_mode %d\n",currSE.value1);//dmh_mode 
#endif	
#if EXTRACT_lcu_info_BAC_inter
		    		fprintf(p_lcu,"F dmh_mode %d\n",currSE.value1);//dmh_mode 
#endif	

#if TRACE
						fprintf ( p_trace, "dmh_mode = %3d\n", dmh_mode );
#endif
          }
          for ( ii = 0; ii < step_h0 * step_h; ii++ )
          {
            for ( jj = 0; jj < step_v0 * step_v; jj++ )
            {
              if ( !bframe )
              {
                img->tmp_mv[j8 + jj][i8 + ii][3] = dmh_mode;
              }
              else
              {
                img->fw_mv[j8 + jj][i8 + ii][3] = dmh_mode;
              }
            }
          }
        }
#ifdef AVS2_S2_S
        else
        {
            dmh_mode = 0;
        }
#endif 

#ifdef AVS2_S2_S
#if AVS2_SCENE_CD
		if(!(img->typeb == BP_IMG ))	//no mvd for S frame, just set it to 0
#else
		if(!(img->typeb == BP_IMG && input->profile_id == 0x50))	//no mvd for S frame, just set it to 0
#endif
		{
#endif
			for ( n = 0; n < 2; n++ )
			{
#if TRACE
			  snprintf ( currSE.tracestring, TRACESTRING_SIZE, "FMVD (pred %3d)", pmv[n] );
#endif
			  currSE.value2 = ( !bframe ? n : 2 * n ); // identifies the component; only used for context determination

			  img->subblock_x = start_x;
			  img->subblock_y = start_y;

			  dP = & ( currSlice->partArr[0] );
			  currSE.reading = readMVD;
			  currSE.value2 = ( n << 1 ) + 0; // identifies the component; only used for context determination
			  dP->readSyntaxElement ( &currSE, dP, currMB, uiPositionInPic );

#if EXTRACT_lcu_info_debug
			  fprintf(p_lcu_debug,"MVD  %d PU:%d Fwd n=%d puNumInCU=%d\n",currSE.value1,(2 * j0 + i0),n,puNumInCU);
#endif					
#if EXTRACT
			  EcuInfoInterSyntax[20+2*puNumInCU+n]= currSE.value1;//readMVD
			  if(n==1)//读取完一个PU 后计数加1
			  	puNumInCU++;			  
#endif	
#if EXTRACT_lcu_info_BAC_inter
			  fprintf(p_lcu,"MVD  %d PU:%d Fwd n=%d puNumInCU=%d\n",currSE.value1,(2 * j0 + i0),n,puNumInCU);//F pu_reference_index
			  fprintf(p_lcu,"MVD	%d PU:%d Fwd n=%d\n",currSE.value1,(2 * j0 + i0),n);//F pu_reference_index
#endif	
			  pmvr_mvd[n] = currSE.value1;
			}
#ifdef AVS2_S2_S
		}
#if AVS2_SCENE_CD
		if(img->typeb == BP_IMG )
#else
		if(img->typeb == BP_IMG && input->profile_id == 0x50)
#endif
		{
			pmvr_mvd[0]=0;
			pmvr_mvd[1]=0;
		}
#endif

		if (b_pmvr_enabled)
			pmvr_mv_derivation(pmvr_mv, pmvr_mvd, pmv);
		else
		{
			pmvr_mv[0] = pmvr_mvd[0] + pmv[0];
			pmvr_mv[1] = pmvr_mvd[1] + pmv[1];
#if m3469
			pmvr_mv[0] = Clip3(-32768, 32767, pmvr_mv[0]);
			pmvr_mv[1] = Clip3(-32768, 32767, pmvr_mv[1]);
#endif
		}
		for (n = 0; n < 2; n++)
			{
				vec = pmvr_mv[n];
				curr_mvd = pmvr_mvd[n];

	      // need B support
	      if ( !bframe )
	      {
	        for ( ii = 0; ii < step_h0 * step_h; ii++ ) //qyu 0903
	        {
	          for ( jj = 0; jj < step_v0 * step_v; jj++ )
	          {
	            img->tmp_mv[j8 + jj][i8 + ii][n] = vec;
	            img->p_snd_tmp_mv[j8 + jj][i8 + ii][n] = 0;
	          }
	        }
	      }
	      else      // B frame
	      {
	        for ( ii = 0; ii < step_h0 * step_h; ii++ ) //qyu 0903
	        {
	          for ( jj = 0; jj < step_v0 * step_v; jj++ )
	          {
	            img->fw_mv[j8 + jj][i8 + ii][n] = vec;
	          }
	        }
	      }

	      /* store (oversampled) mvd */
	      for ( jj = 0; jj < num_of_orgMB_in_row; jj++ )
	      {
	        int pos = uiPositionInPic + jj * img->PicWidthInMbs;

	        for ( ii = 0; ii < num_of_orgMB_in_col; ii++, pos++ )
	        {
	          codingUnit *tmpMB = &img->mb_data[pos];

	          for ( l = 0; l < step_v0; l++ )
	          {
	            for ( m = 0; m < step_h0; m++ )
	            {
	              tmpMB->mvd[0][j0 + l][i0 + m][n] =  curr_mvd;
	            }
	          }
	        }
	      }
	      }
      }
      else if ( currMB->b8mode[k = 2 * j0 + i0] == 0 )
      {
        // direct mode
        for ( j = j0; j < j0 + step_v0; j++ )
        {
          for ( i = i0; i < i0 + step_h0; i++ )
          {
            j8 = block8_y + j * N8_SizeScale;
            i8 = block8_x + i * N8_SizeScale; //qyu 0904
            ref = refbuf[0][j8][i8];
            if ( ref == -1 )
            {
              for ( l = 0; l < num_of_orgMB_in_row; l++ )
              {
                for ( m = 0; m < num_of_orgMB_in_col; m++ )
                {
                  img->fw_refFrArr[block8_y + j * N8_SizeScale + l][block8_x + i * N8_SizeScale + m] = 0;
                  img->bw_refFrArr[block8_y + j * N8_SizeScale + l][block8_x + i * N8_SizeScale + m] = 0;
                }
              }

              j8 = block8_y + j * N8_SizeScale;
              i8 = block8_x + i * N8_SizeScale;

              for ( ii = 0; ii < 2; ii++ )
              {
                img->fw_mv[j8][i8][ii] = 0;
                img->bw_mv[j8][i8][ii] = 0;
              }

              SetMotionVectorPredictor ( currMB->ui_MbBitSize, uiPositionInPic, & ( img->fw_mv[j8][i8][0] ),
                                         img->fw_refFrArr, img->fw_mv, 0, 0, 0, MIN_CU_SIZE * N8_SizeScale, MIN_CU_SIZE * N8_SizeScale, 0, 1 );
              SetMotionVectorPredictor ( currMB->ui_MbBitSize, uiPositionInPic, & ( img->bw_mv[j8][i8][0] ),
                                         img->bw_refFrArr, img->bw_mv, 0, 0, 0, MIN_CU_SIZE * N8_SizeScale, MIN_CU_SIZE * N8_SizeScale, -1, 1 );

              for ( l = 0; l < num_of_orgMB_in_row; l++ )
              {
                for ( m = 0; m < num_of_orgMB_in_col; m++ )
                {
                  for ( ii = 0; ii < 2; ii++ )
                  {
                    img->fw_mv[j8 + l][i8 + m][ii] = img->fw_mv[j8][i8][ii];
                    img->bw_mv[j8 + l][i8 + m][ii] = img->bw_mv[j8][i8][ii];
                  }
                }
              }
            }
            else
            {
              frame_no_next_P = 2 * img->imgtr_next_P;
              frame_no_B = 2 * img->pic_distance;

              for ( n = 0; n < 4; n++ )
              {
								delta_P[n] = 2 * (img->imgtr_next_P - ref_poc[0][n]);
                delta_P[n] = ( delta_P[n] + 512 ) % 512;
              }

              scale_refframe = 0;

              iTRp = delta_P[ref];
              iTRp1 = 2 * (img->imgtr_next_P - img->imgtr_fwRefDistance[0]);

              iTRd = frame_no_next_P - frame_no_B;
              iTRb = iTRp1 - iTRd;

              iTRp  = ( iTRp  + 512 ) % 512;
              iTRp1 = ( iTRp1 + 512 ) % 512;
              iTRd  = ( iTRd  + 512 ) % 512;
              iTRb  = ( iTRb  + 512 ) % 512;
#if AVS2_SCENE_MV
						  if (ref == curr_RPS.num_of_ref -1  && background_reference_enable){
							  iTRp = 1;
						  }
						  if (0 == curr_RPS.num_of_ref - 1 && background_reference_enable){
							  iTRp1 = 1;
							  iTRb = 1;
						  }
#endif
              for ( l = 0; l < num_of_orgMB_in_row; l++ )
              {
                for ( m = 0; m < num_of_orgMB_in_col; m++ )
                {
                  img->fw_refFrArr[block8_y + j * N8_SizeScale + l][block8_x + i * N8_SizeScale + m] = 0;
                  img->bw_refFrArr[block8_y + j * N8_SizeScale + l][block8_x + i * N8_SizeScale + m] = 0;
                }
              }

              j8 = block8_y + j * N8_SizeScale;
              i8 = block8_x + i * N8_SizeScale;

              for ( ii = 0; ii < 2; ii++ )
              {
                for ( l = 0; l < num_of_orgMB_in_row; l++ )
                {
                  for ( m = 0; m < num_of_orgMB_in_col; m++ )
                  {
#if MV_SCALING_UNIFY
									  if ( mvbuf[0][block8_y + j * N8_SizeScale][block8_x + i * N8_SizeScale][ii] < 0 )
									  {
										  img->fw_mv[j8 + l][i8 + m][ii] =  - ( ( ( MULTI / iTRp ) * ( 1 - iTRb * mvbuf[0][j8][i8][ii] ) - 1 ) >> OFFSET );
										  img->bw_mv[j8 + l][i8 + m][ii] = ( ( MULTI / iTRp ) * ( 1 - iTRd * mvbuf[0][j8][i8][ii] ) - 1 ) >> OFFSET;
									  }
									  else
									  {
										  img->fw_mv[j8 + l][i8 + m][ii] = ( ( MULTI / iTRp ) * ( 1 + iTRb * mvbuf[0][j8][i8][ii] ) - 1 ) >> OFFSET;
										  img->bw_mv[j8 + l][i8 + m][ii] = - ( ( ( MULTI / iTRp ) * ( 1 + iTRd * mvbuf[0][j8][i8][ii] ) - 1 ) >> OFFSET );
									  }

#else
                    if ( mvbuf[0][block8_y + j * N8_SizeScale][block8_x + i * N8_SizeScale][ii] < 0 )
                    {
                      img->fw_mv[j8 + l][i8 + m][ii] =  - ( ( ( 16384 / iTRp ) * ( 1 - iTRb * mvbuf[0][j8][i8][ii] ) - 1 ) >> 14 );
                      img->bw_mv[j8 + l][i8 + m][ii] = ( ( 16384 / iTRp ) * ( 1 - iTRd * mvbuf[0][j8][i8][ii] ) - 1 ) >> 14;
                    }
                    else
                    {
                      img->fw_mv[j8 + l][i8 + m][ii] = ( ( 16384 / iTRp ) * ( 1 + iTRb * mvbuf[0][j8][i8][ii] ) - 1 ) >> 14;
                      img->bw_mv[j8 + l][i8 + m][ii] = - ( ( ( 16384 / iTRp ) * ( 1 + iTRd * mvbuf[0][j8][i8][ii] ) - 1 ) >> 14 );
                    }
#endif
#if HALF_PIXEL_COMPENSATION_DIRECT
                    if ( img->is_field_sequence && ii == 1 )
                    {
                        int delta, delta2, delta_d, delta2_d;
                        int oriPOC = frame_no_next_P;
                        int oriRefPOC = frame_no_next_P - iTRp;
                        int scaledPOC = frame_no_B;
                        int scaledRefPOC = frame_no_B - iTRb;
                        getDeltas( &delta, &delta2, oriPOC, oriRefPOC, scaledPOC, scaledRefPOC );
                        scaledRefPOC = frame_no_B - iTRd;
                        getDeltas( &delta_d, &delta2_d, oriPOC, oriRefPOC, scaledPOC, scaledRefPOC);
                        assert(delta == delta_d );

                        if ( mvbuf[0][block8_y + j * N8_SizeScale][block8_x + i * N8_SizeScale][ii]+delta < 0 )
                        {
                            img->fw_mv[j8 + l][i8 + m][ii] =  - ( ( ( MULTI / iTRp ) * ( 1 - iTRb * (mvbuf[0][j8][i8][ii]+delta) ) - 1 ) >> OFFSET ) -delta2;
                            img->bw_mv[j8 + l][i8 + m][ii] =( ( ( MULTI / iTRp ) * ( 1 - iTRd * (mvbuf[0][j8][i8][ii]+delta_d) ) - 1 ) >> OFFSET ) -delta2_d;
                        }
                        else
                        {
                            img->fw_mv[j8 + l][i8 + m][ii] =( ( ( MULTI / iTRp ) * ( 1 + iTRb * (mvbuf[0][j8][i8][ii]+delta) ) - 1 ) >> OFFSET ) -delta2;
                            img->bw_mv[j8 + l][i8 + m][ii] = - ( ( ( MULTI / iTRp ) * ( 1 + iTRd * (mvbuf[0][j8][i8][ii]+delta_d) ) - 1 ) >> OFFSET ) -delta2_d;
                        }
                    }
#endif
#if m3469

#if EXTRACT_lcu_info_debug
                    fprintf(p_lcu_debug, "MVP img->fw_mv[%4d][%4d] = %4d ii=%d\n", j8 + l, i8 + m,img->fw_mv[j8 + l][i8 + m][ii],ii);
                    fprintf(p_lcu_debug, "MVP img->bw_mv[%4d][%4d] = %4d ii=%d\n", j8 + l, i8 + m, img->bw_mv[j8 + l][i8 + m][ii],ii);
#endif
                    img->fw_mv[j8 + l][i8 + m][ii] = Clip3(-32768, 32767, img->fw_mv[j8 + l][i8 + m][ii]);
                    img->bw_mv[j8 + l][i8 + m][ii] = Clip3(-32768, 32767, img->bw_mv[j8 + l][i8 + m][ii]);
#endif
                  }
                }
              }
            }
          }
        }
#if EXTRACT
				//读取完一个PU 后计数加1
				puNumInCU++;			
#endif
      }
#if EXTRACT
			//读取完一个PU 后计数加1
			else
			{
				puNumInCU++;	
			}			
#endif

      i0 += max ( 1, step_h0 );
    }

    j0 += max ( 1, step_v0 );
  }


  //=====  READ BACKWARD MOTION VECTORS =====
  currSE.type = SE_MVD;

  currSE.mapping = linfo_se;
#if EXTRACT //_lcu_info_BAC
  puNumInCU=0;

#if EXTRACT_lcu_info_debug
	fprintf(p_lcu_debug,"READ BACKWARD MOTION VECTORS  puNumInCU=%d \n",puNumInCU);
#endif

#endif	


  for ( j0 = 0; j0 < 2; )
  {
#if INTRA_2N
    if ( ( currMB->cuType == I8MB || currMB->cuType == I16MB ||currMB->cuType==InNxNMB ||currMB->cuType==INxnNMB) && j0 == 0 ) // for  by jhzheng [2004/08/02]
#else
    if ( ( currMB->cuType == I8MB && j0 == 0 ) ) // for  by jhzheng [2004/08/02]
#endif
    {
      j0 += 1;
      continue;
    }


    for ( i0 = 0; i0 < 2; )
    {
      k = 2 * j0 + i0;
	  	if ( ( currMB->b8pdir[k] == BACKWARD || currMB->b8pdir[k] == SYM || currMB->b8pdir[k] == BID || currMB->b8pdir[k] == DUAL ) && ( currMB->b8mode[k] != 0 ) ) //has backward vector
      {
        get_b8_offset( currMB->cuType, currMB->ui_MbBitSize, i0, j0, &start_x, &start_y, &step_h, &step_v );

        j8 = block8_y + start_y ;
        i8 = block8_x + start_x ;
				refframe = ( bframe ? img->bw_refFrArr : p_snd_refFrArr )[j8][i8];
				if ( currMB->b8pdir[k] == SYM )
				{
					fw_refframe = img->fw_refFrArr[ j8 ][ i8 ];
				}
				if ( !bframe )
				{
					int pix_start_x, pix_start_y, pix_step_h, pix_step_v;
					get_pix_offset( currMB->cuType, currMB->ui_MbBitSize, i0, j0, &pix_start_x, &pix_start_y, &pix_step_h, &pix_step_v );
					pix_step_h = ( currMB->cuType == 0 || currMB->cuType == 1 || currMB->cuType == 2 || currMB->cuType == 4 || currMB->cuType == 5 ) ? pix_step_h * 2 : pix_step_h;
					pix_step_v = ( currMB->cuType == 0 || currMB->cuType == 1 || currMB->cuType == 3 || currMB->cuType == 6 || currMB->cuType == 7 ) ? pix_step_v * 2 : pix_step_v;

					SetMotionVectorPredictor ( currMB->ui_MbBitSize, uiPositionInPic, pmv, refFrArr , img->tmp_mv,  refframe, pix_start_x, pix_start_y, pix_step_h , pix_step_v, 0, 0 ); //?????
				}
				else
	      {
	        int pix_start_x, pix_start_y, pix_step_h, pix_step_v;
	        get_pix_offset( currMB->cuType, currMB->ui_MbBitSize, i0, j0, &pix_start_x, &pix_start_y, &pix_step_h, &pix_step_v );
	        pix_step_h = ( currMB->cuType == 0 || currMB->cuType == 1 || currMB->cuType == 2 || currMB->cuType == 4 || currMB->cuType == 5 ) ? pix_step_h * 2 : pix_step_h;
	        pix_step_v = ( currMB->cuType == 0 || currMB->cuType == 1 || currMB->cuType == 3 || currMB->cuType == 6 || currMB->cuType == 7 ) ? pix_step_v * 2 : pix_step_v;

	        SetMotionVectorPredictor ( currMB->ui_MbBitSize, uiPositionInPic, pmv,  img->bw_refFrArr, img->bw_mv, refframe, pix_start_x, pix_start_y, pix_step_h, pix_step_v, -1, 0 ); //Lou 1016

	      }

       	for ( k = 0; k < 2; k++ )
				{
#if TRACE
	        snprintf ( currSE.tracestring, TRACESTRING_SIZE, "BMVD (pred %3d)", pmv[k] );
#endif

	        currSE.value2   = 2 * k + 1; // identifies the component; only used for context determination

	        if ( currMB->b8pdir[2 * j0 + i0] == SYM )
	        {
	          int delta_P, iTRp, DistanceIndexFw, DistanceIndexBw, refframe, delta_PB;
	          refframe = fw_refframe;
	          delta_P = 2 * ( img->imgtr_next_P - img->imgtr_fwRefDistance[0] );
	          delta_P = ( delta_P + 512 ) % 512;

	          iTRp = ( refframe + 1 ) * delta_P;  //the lates backward reference

	          delta_PB = 2 * ( img->pic_distance - img->imgtr_fwRefDistance[0] );
	          delta_PB = ( delta_PB + 512 ) % 512;
	          iTRp     = ( iTRp + 512 ) % 512;

	          DistanceIndexFw = delta_PB;

	          DistanceIndexBw = ( iTRp - DistanceIndexFw + 512 ) % 512;
#if MV_SCALING_UNIFY
						curr_mvd = - ( ( img->fw_mv[j8][i8][k] * DistanceIndexBw * ( MULTI / DistanceIndexFw ) + HALF_MULTI ) >> OFFSET );
#else
	          curr_mvd = - ( ( img->fw_mv[j8][i8][k] * DistanceIndexBw * ( 512 / DistanceIndexFw ) + 256 ) >> 9 );
#endif
#if HALF_PIXEL_COMPENSATION_MVD
	          if (img->is_field_sequence && k==1)
	          {
	              int delta, delta2, oriPOC, oriRefPOC, scaledPOC, scaledRefPOC;
	              oriPOC = 2*picture_distance; 
	              oriRefPOC = oriPOC - DistanceIndexFw;
	              scaledPOC = 2*picture_distance;
	              scaledRefPOC = scaledPOC - DistanceIndexBw;
	              getDeltas( &delta, &delta2, oriPOC, oriRefPOC, scaledPOC, scaledRefPOC);
	              curr_mvd = - ( ( (img->fw_mv[j8][i8][k]+delta) * DistanceIndexBw * ( MULTI / DistanceIndexFw ) + HALF_MULTI ) >> OFFSET );
	              curr_mvd -= delta2;
	          }
#endif

						pmvr_mv[k] = curr_mvd;
#if m3469
						pmvr_mv[k] = Clip3(-32768, 32767, pmvr_mv[k]);
#endif			
						pmvr_mvd[k] = curr_mvd - pmv[k];
#if m3469
						pmvr_mvd[k] = Clip3(-32768, 32767, pmvr_mvd[k]);
#endif				
#if EXTRACT
						//pu 是SYM模式时，没有候后向mvd，pu计数加1
						if(k==1)
							puNumInCU++;
#if EXTRACT_lcu_info_debug
						fprintf(p_lcu_debug,"pu=SYM  puNumInCU=%d \n",puNumInCU);
#endif

#endif	
					}
	  			else if ( currMB->b8pdir[2 * j0 + i0] == DUAL )
			  	{
					  int DistanceIndexFw, DistanceIndexBw;
					  fw_refframe = refFrArr[ j8 ][ i8 ];

					  DistanceIndexFw = 2 * ( img->pic_distance - img->imgtr_fwRefDistance[fw_refframe]);
					  DistanceIndexFw = ( DistanceIndexFw + 512 ) % 512;
					  DistanceIndexBw = 2 * ( img->pic_distance - img->imgtr_fwRefDistance[refframe]);
					  DistanceIndexBw = ( DistanceIndexBw + 512 ) % 512;
#if AVS2_SCENE_MV
					  if (fw_refframe == curr_RPS.num_of_ref - 1 && background_reference_enable) {
						  DistanceIndexFw = 1;
					  }
					  if (refframe == curr_RPS.num_of_ref - 1 && background_reference_enable){
						  DistanceIndexBw = 1;
					  }
#endif

#if MV_SCALING_UNIFY
					  curr_mvd =  ( img->tmp_mv[j8][i8][k] * DistanceIndexBw * ( MULTI / DistanceIndexFw ) + HALF_MULTI ) >> OFFSET ;
#else
					  curr_mvd =  ( img->tmp_mv[j8][i8][k] * DistanceIndexBw * ( 512 / DistanceIndexFw ) + 256 ) >> 9 ;
#endif
#if HALF_PIXEL_COMPENSATION_MVD
            if (img->is_field_sequence && k==1)
            {
                int delta, delta2, oriPOC, oriRefPOC, scaledPOC, scaledRefPOC;
                oriPOC = 2*picture_distance; 
                oriRefPOC = oriPOC - DistanceIndexFw;
                scaledPOC = 2*picture_distance;
                scaledRefPOC = scaledPOC - DistanceIndexBw;
                getDeltas( &delta, &delta2, oriPOC, oriRefPOC, scaledPOC, scaledRefPOC);
                curr_mvd =  ( (img->tmp_mv[j8][i8][k]+delta) * DistanceIndexBw * ( MULTI / DistanceIndexFw ) + HALF_MULTI ) >> OFFSET ;
                curr_mvd -= delta2;
            }
#endif
					  pmvr_mv[k] = curr_mvd;
#if m3469
					  pmvr_mv[k] = Clip3(-32768, 32767, pmvr_mv[k]);
#endif				  
					  pmvr_mvd[k] = curr_mvd - pmv[k];
#if m3469
			 			pmvr_mvd[k] = Clip3(-32768, 32767, pmvr_mvd[k]);
#endif				  
								
#if EXTRACT
						//pu 是DUAL模式时，没有候后向mvd，pu计数加1
						//puNumInCU++;
#endif

				  }
	       	else
	        {
	          img->subblock_x = start_x;
	          img->subblock_y = start_y;
	          dP = & ( currSlice->partArr[0] );
	          currSE.reading = readMVD;

						currSE.value2   = ( k << 1 ) + 1; // identifies the component; only used for context determination

						dP->readSyntaxElement ( &currSE, dP, currMB, uiPositionInPic );
#if EXTRACT_lcu_info_debug
						fprintf(p_lcu_debug,"MVD  %d PU:%d puNumInCU=%d Bck\n",currSE.value1,(2 * j0 + i0),puNumInCU);
#endif

#if EXTRACT
						EcuInfoInterSyntax[28+2*(puNumInCU)+k]= currSE.value1;//PF pu_reference_index 
						if(k==1)
							puNumInCU++;
						//fprintf(p_lcu,"MVD  %d PU:%d Bck\n",currSE.value1,(2 * j0 + i0));//
#endif	
#if EXTRACT_lcu_info_BAC_inter
						fprintf(p_lcu,"MVD  %d PU:%d Bck\n",currSE.value1,(2 * j0 + i0));//
#endif

						pmvr_mvd[k] = currSE.value1;
						pmvr_mv[k] = pmvr_mvd[k] + pmv[k];
#if m3469
						pmvr_mvd[k] = Clip3(-32768, 32767, pmvr_mvd[k]);
#endif			
					}

				}		
				if ( b_pmvr_enabled && currMB->b8pdir[2 * j0 + i0] != SYM && currMB->b8pdir[2 * j0 + i0] != DUAL)
				{
					pmvr_mv_derivation(pmvr_mv, pmvr_mvd, pmv);
				}
				for ( k = 0; k < 2; k++ )	//jcma
				{
					vec = pmvr_mv[k];
					curr_mvd = pmvr_mvd[k];
					for ( ii = 0; ii < step_h0 * step_h; ii++ ) //qyu 0903
					{
						for ( jj = 0; jj < step_v0 * step_v; jj++ )
						{
							( bframe ? img->bw_mv : img->p_snd_tmp_mv ) [j8 + jj][i8 + ii][k] = vec;
						}
					}

          // set all org MB in the current MB
          for ( jj = 0; jj < num_of_orgMB_in_row; jj++ )
          {
            int pos = uiPositionInPic + jj * img->PicWidthInMbs;

            for ( ii = 0; ii < num_of_orgMB_in_col; ii++, pos++ )
            {
              codingUnit *tmpMB = &img->mb_data[pos];

              for ( l = 0; l < step_v0; l++ )
              {
                for ( m = 0; m < step_h0; m++ )
                {
                  tmpMB->mvd[1][j0 + l][i0 + m][k] =  curr_mvd;
                }
              }
            }
          }//set all org MB 
					
		    }
      }
#if EXTRACT_lcu_info_debug
			else
			{				
				puNumInCU++;//下一个pu
				fprintf(p_lcu_debug,"next pu puNumInCU=%d \n",puNumInCU);
			}
#endif

      i0 += max ( 1, step_h0 );
    }

    j0 += max ( 1, step_v0 );

  }
	////////////////////////////////////////////////////////////////////////////////////////////////////
	///read PMV Index
	/////////////////////////////////////////////////////////////////////////////////////////////////////
}


/*
*************************************************************************
* Function:Get coded block pattern and coefficients (run/level)
from the bitstream
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void readBlockCoeffs ( unsigned int uiPosition )
{
  int i, j;
  codingUnit *currMB = &img->mb_data[uiPosition];
  int b8;

  int offset_x = ( ( uiPosition % img->PicWidthInMbs ) - ( img->current_mb_nr % img->PicWidthInMbs ) ) * MIN_CU_SIZE;
  int offset_y = ( ( uiPosition / img->PicWidthInMbs ) - ( img->current_mb_nr / img->PicWidthInMbs ) ) * MIN_CU_SIZE;
  int bit_size = currMB->ui_MbBitSize;

  int block_x, block_y;
  Slice *currSlice = img->currentSlice;
  int     level, run, coef_ctr, k , i0, j0, uv, val, qp;
  int     shift, QPI, sum;
  int     start_scan;
  SyntaxElement currSE;          //lzhang
  DataPartition *dP;

  int **SCAN;
  int CurrentChromaQP;

  // Adaptive frequency weighting quantization
#if FREQUENCY_WEIGHTING_QUANTIZATION
  int wqm_coef;
  int WQMSizeId,WQMSize,iStride;
  int wqm_shift;
#endif
  extern int DCT_CGFlag[CG_SIZE * CG_SIZE];
  extern int DCT_PairsInCG[CG_SIZE * CG_SIZE];
  extern int DCT_CGNum;
  int iCG = 0;
  int pairs = 0;
  extern int g_intraModeClassified[NUM_INTRA_PMODE];

  int iVer=0, iHor=0;
  int iStartX,iStartY;
  //CU大于8x8， 帧间预测， 进行TU划分，PU是水平划分
  //TU 进行水平2划分
  if (input->useNSQT && currMB->ui_MbBitSize > B8X8_IN_BIT &&(currMB->cuType == P2NXN || currMB->cuType == PHOR_UP || currMB->cuType == PHOR_DOWN) && currMB->trans_size == 1)
  {
    iHor = 1;
  }
  //CU大于8x8， 帧间预测， 进行TU划分，PU垂直划分
  //TU 进行垂直2划分  
  else if (input->useNSQT && currMB->ui_MbBitSize > B8X8_IN_BIT &&(currMB->cuType == PNX2N || currMB->cuType == PVER_LEFT || currMB->cuType == PVER_RIGHT) &&currMB->trans_size == 1)
  {
    iVer = 1;
  }
  else 
    //CU大于8x8，帧内预测， 进行TU划分，PU水平划分
    //TU 进行水平4划分  
    if (input->useSDIP && (currMB->trans_size == 1) && (currMB->cuType == InNxNMB))//add yuqh 20130825
    {
      iHor = 1;
    }
  //CU大于8x8， 帧内预测， 进行TU划分，PU垂直划分
  //TU 进行垂直4划分
    else if (input->useSDIP && (currMB->trans_size == 1) && (currMB->cuType == INxnNMB))//add yuqh 20130825
    {
      iVer = 1;
    }
    
  DCT_Pairs = -1;

#if EXTRACT
  //fprintf(p_lcu,"iVer=%d, iHor=%d\n",iVer, iHor);//
#endif

#if MB_DQP
#else
  currMB->qp = img->qp;
#endif

  // Adaptive frequency weighting quantization  
#if FREQUENCY_WEIGHTING_QUANTIZATION
  if(WeightQuantEnable)
  {
#if !WQ_MATRIX_FCD
	  wqm_shift=(pic_weight_quant_data_index==1)?3:0;
#else
	  wqm_shift=(pic_weight_quant_data_index==1)?2:0;
#endif

  }
#endif

  qp  = currMB->qp;
#if INTRA_2N
  // luma coefficients

  //如果当前CU 是帧内，或者是不划分CU  
  if ( currMB->cuType == I16MB || currMB->trans_size == 0 )
  {
#if FREQUENCY_WEIGHTING_QUANTIZATION
#if M3198_CU8
	  if ( currMB->ui_MbBitSize == B8X8_IN_BIT)
	  {
		  SCAN = AVS_CG_SCAN8x8;
		  WQMSizeId=1;
	  }
	  else
#endif
		  if ( currMB->ui_MbBitSize == B16X16_IN_BIT )
		  {
			  SCAN = AVS_CG_SCAN16x16;
			  WQMSizeId=2;
		  }
		  else if ( currMB->ui_MbBitSize == B32X32_IN_BIT )
		  {
			  SCAN = AVS_CG_SCAN32x32;
			  WQMSizeId=3;
		  }
		  else if ( currMB->ui_MbBitSize == B64X64_IN_BIT )
		  {
			  SCAN = AVS_CG_SCAN32x32;
			  WQMSizeId=3;
		  }
#else
#if M3198_CU8
    if ( currMB->ui_MbBitSize == B8X8_IN_BIT)
    {
      SCAN = AVS_CG_SCAN8x8;
    }
    else
#endif
    if ( currMB->ui_MbBitSize == B16X16_IN_BIT )
    {
      SCAN = AVS_CG_SCAN16x16;
    }
    else if ( currMB->ui_MbBitSize == B32X32_IN_BIT )
    {
      SCAN = AVS_CG_SCAN32x32;
    }
    else if ( currMB->ui_MbBitSize == B64X64_IN_BIT )
    {
      SCAN = AVS_CG_SCAN32x32;
    }
#endif //FREQUENCY_WEIGHTING_QUANTIZATION

	  //存在亮度非零系数
    if ( currMB->cbp & 0xF )
    {
      // === set offset in current codingUnit ===
      int coded_bitsize ;
	    //如果是64x64的CU  ,则变换系数的大小减为32x32
      if ( currMB->ui_MbBitSize == B64X64_IN_BIT )
      {
        coded_bitsize = currMB->ui_MbBitSize - 1;
      }
      else
      {
        coded_bitsize = currMB->ui_MbBitSize;
      }


      start_scan = 0; // take all coeffs
      coef_ctr = start_scan - 1;
      level    = 1;
      iCG = 0;
      pairs = 0;
	  
#if EXTRACT
      
#if EXTRACT_08X
      //fprintf(p_lcu_coeff, "%08X\n", ((cuPosInLCUPixY >> 3) << 16) + (cuPosInLCUPixX >> 3));
      //fprintf(p_lcu_coeff,"%08X\n",0);
      //fprintf(p_lcu,"coded_bitsize=%d \n",coded_bitsize);//
#endif
			 
#if EXTRACT_D
      fprintf(p_lcu_coeff,"block=%d \n",0);
      //fprintf(p_lcu,"coded_bitsize=%d \n",coded_bitsize);//
#endif
#endif	  
      //开始读取亮度块的正方形系数块
      for ( k = start_scan; ( k < ( 1 << coded_bitsize  ) * ( 1 << coded_bitsize ) + 1 ) && ( level != 0 ); k++ )
      {
        //============ read =============
        currSE.context      = LUMA_8x8;
        currSE.type         = ( IS_INTRA ( currMB ) ) ? SE_LUM_AC_INTRA : SE_LUM_AC_INTER;
#if TRACE
        fprintf ( p_trace, "  Luma8x8 sng" );
#endif
				// 读取整个TB 在readRunLevelRef 中完成
        currMB->l_ipred_mode = currMB->intra_pred_modes[0];
        dP = & ( currSlice->partArr[0] );
        currSE.reading = readRunLevelRef;
        dP->readSyntaxElement ( &currSE, dP, currMB, uiPosition );
        level = currSE.value1;
        run   = currSE.value2;
#if TRACE
        fprintf ( p_trace, "(%2d) level =%3d run =%2d\n", k, level, run );
        fflush ( p_trace );
#endif
        //============ decode =============
        if ( level != 0 )  /* leave if len=1 */
        {
          while ( DCT_CGFlag[ DCT_CGNum - iCG - 1 ] == 0 )
          {
            coef_ctr += 16;
            iCG ++;
          }
#if EXTRACT
          if(pairs==0){
#if EXTRACT_08X 
            fprintf(p_lcu_coeff, "%08X\n", ((cuPosInLCUPixY >> 3) << 16) + (cuPosInLCUPixX >> 3));
            fprintf(p_lcu_coeff, "%08X\n", 0);
				    fprintf(p_lcu_coeff,"%08X\n",iCG);
#endif			 
#if EXTRACT_lcu_info_coeff_debug 
            fprintf(p_lcu_debug, "cuPosInLCUPixY,X=%4d %4d\n", cuPosInLCUPixY,cuPosInLCUPixX);
            fprintf(p_lcu_debug, "block %08X\n", 0);
            fprintf(p_lcu_debug, "iCG   %08X\n", iCG);//从低频往高频的序号
#endif    
#if EXTRACT_D 
				    fprintf(p_lcu_coeff,"DCT_CGNum=%d DCT_Pairs=%d\n",DCT_CGNum ,DCT_Pairs);
				    fprintf(p_lcu_coeff,"iCG=%d coef_ctr=%d\n",iCG,coef_ctr);
#endif 
          }
#endif	 

          pairs ++;
          coef_ctr += run + 1;
#if EXTRACT
#if EXTRACT_08X
          int tmp_run=0;
          while( tmp_run <run){
					  fprintf(p_lcu_coeff,"%08X\n",0);
					  tmp_run++;
          }
		  		fprintf(p_lcu_coeff,"%08X\n",level);		  
#endif
#if EXTRACT_lcu_info_coeff_debug 
          fprintf(p_lcu_debug, "level   %4d   run   %4d coef_ctr=%d\n", level, run, coef_ctr);
#endif	
			   
#if EXTRACT_D
		 		 	//fprintf(p_lcu,"level=%d coef_ctr=%d\n",level,coef_ctr);
          int tmp_run2=0;
          while( tmp_run2 <run){
					  fprintf(p_lcu_coeff,"level=%d\n",0);
					  tmp_run2++;
          }
		 		 	fprintf(p_lcu_coeff,"level=%d coef_ctr=%d\n",level,coef_ctr);		  
#endif

#endif

          i = SCAN[coef_ctr][0];
          j = SCAN[coef_ctr][1];

          if( currSE.type == SE_LUM_AC_INTRA && g_intraModeClassified[currMB->l_ipred_mode] == INTRA_PRED_HOR )
          {
            SWAP(i, j);
          }

#if TRANS_16_BITS
#if EXTEND_BD
					shift = IQ_SHIFT[qp] + (input->sample_bit_depth + 1) + currMB->ui_MbBitSize - LIMIT_BIT + 1;
#else
					shift = IQ_SHIFT[qp] + RESID_BIT + currMB->ui_MbBitSize - LIMIT_BIT + 1;
#endif
#else
					shift = IQ_SHIFT[qp];
#endif
#if M3175_SHIFT
					shift ++;
#endif
#if SHIFT_DQ_INV
          shift --;
#endif

//Adaptive frequency weighting quantization
#if FREQUENCY_WEIGHTING_QUANTIZATION
					if (WeightQuantEnable)
					{
						WQMSize=1<<(WQMSizeId+2);
						if((WQMSizeId==0)||(WQMSizeId==1))
						{
							iStride=WQMSize;
							wqm_coef=cur_wq_matrix[WQMSizeId][(j&(iStride-1))*iStride+(i&(iStride-1))];
						}
#if AWQ_LARGE_BLOCK_EXT_MAPPING
						else if (WQMSizeId==2)
						{
							iStride=WQMSize>>1;
							wqm_coef=cur_wq_matrix[WQMSizeId][((j>>1)&(iStride-1))*iStride+((i>>1)&(iStride-1))];
						}
						else if (WQMSizeId==3)
						{
							iStride=WQMSize>>2;
							wqm_coef=cur_wq_matrix[WQMSizeId][((j>>2)&(iStride-1))*iStride+((i>>2)&(iStride-1))];
						}
#endif
					}

					QPI   = IQ_TAB[qp];
					val   = level;
					if(WeightQuantEnable)
					{
#if AWQ_WEIGHTING
						sum = ((((((long long int)val*wqm_coef)>>wqm_shift)*QPI)>>4) + (1<<(shift-2)) )>>(shift-1);	// M2239, N1466
#else
						sum = ( val * QPI + ( 1 << ( shift - 2 ) ) ) >> ( shift - 1 );
#endif
					}
					else
					{
						sum = ( val * QPI + ( 1 << ( shift - 2 ) ) ) >> ( shift - 1 );
					}
#if QuantClip
          sum = Clip3(-32768, 32767, sum);
#endif
#else
          QPI   = IQ_TAB[qp];
          val =   level;
          sum = ( val * QPI + ( 1 << ( shift - 2 ) ) ) >> ( shift - 1 );
#endif

#if CLIP_FIX
          sum = Clip3 (  0 - ( 1 << 15 ), ( 1 << 15 ) - 1,  sum);
#endif
#if EXTRACT
		  
		  		cuCoeffY[j][i]=level;
#endif

          img->resiY[offset_y + j][offset_x + i] = sum;
        }
#if EXTRACT
				else
				{
#if EXTRACT_08X
				  //fprintf(p_lcu_coeff,"%08X else \n",0);
#endif
			 
#if EXTRACT_D
				  fprintf(p_lcu_coeff,"level=%d\n",0);
#endif			
				}
#endif
    		if( pairs == DCT_PairsInCG[DCT_CGNum - iCG - 1] )
		    {
#if EXTRACT
          //打印一个CG中最后剩余的0系数
				  int leftrun=coef_ctr&0xf;
				  int tmp_run=0;
				  leftrun=16-leftrun-1;
#if EXTRACT_08X		  
				  while( tmp_run <leftrun)
					{
						fprintf(p_lcu_coeff,"%08X\n",0);

            //fprintf(p_lcu_coeff, "%08X %d tmp_run=%d\n", 0, leftrun, tmp_run);
						tmp_run++;
					} 
#endif
			 
#if EXTRACT_D	
				  leftrun=coef_ctr&0xf;
				  tmp_run=0;
				  leftrun=16-leftrun-1;	  
				  while( tmp_run <leftrun)
					{
						fprintf(p_lcu_coeff,"level=%d\n",0);
						tmp_run++;
				  } 
#endif

#endif        
		      coef_ctr |= 0xf;
		      pairs = 0;
		      iCG ++;        	  
		    }
      }
    }
  }
  // 是帧间CU  或者进行划分
  else
  {
    //开启 NSQT, 帧间CU , 且CU 大于8x8，进行1:4 变换
    if (input->useNSQT && currMB->ui_MbBitSize > B8X8_IN_BIT &&( currMB->cuType < 9 && (iVer || iHor)))
    {
      if (iHor)
      {
        if ( currMB->ui_MbBitSize == B16X16_IN_BIT )
        {
          SCAN = AVS_SCAN4x16;
        }
        else if ( currMB->ui_MbBitSize == B32X32_IN_BIT )
        {
          SCAN = AVS_SCAN8x32;
        }
        else if ( currMB->ui_MbBitSize == B64X64_IN_BIT )
        {
          SCAN = AVS_SCAN8x32;
          bit_size -= 1;
        }
      }
      if (iVer)
      {
        if ( currMB->ui_MbBitSize == B16X16_IN_BIT )
        {
          SCAN = AVS_SCAN16x4;
        }
        else if ( currMB->ui_MbBitSize == B32X32_IN_BIT )
        {
          SCAN = AVS_SCAN32x8;
        }
        else if ( currMB->ui_MbBitSize == B64X64_IN_BIT )
        {
          SCAN = AVS_SCAN32x8;
          bit_size -= 1;
        }
      }
#if FREQUENCY_WEIGHTING_QUANTIZATION
	  if ( currMB->ui_MbBitSize == B16X16_IN_BIT )
		  WQMSizeId=2;
	  else if ( (currMB->ui_MbBitSize== B32X32_IN_BIT)|| (currMB->ui_MbBitSize == B64X64_IN_BIT))
		  WQMSizeId=3;
#endif
    }
	//不开启 NSQT, 帧内CU ,或者CU 等于8x8，进行1:4 变换
	else 
		if (input->useSDIP && ( currMB->cuType >11 && (iVer || iHor)) )  //inter MB
		{
			if (iHor)
			{
				if ( currMB->ui_MbBitSize  == B16X16_IN_BIT )
				{
					SCAN = AVS_SCAN4x16;
				}
				else if ( currMB->ui_MbBitSize == B32X32_IN_BIT )
				{
					SCAN = AVS_SCAN8x32;
				}
				else if ( currMB->ui_MbBitSize == B64X64_IN_BIT )
				{
					SCAN = AVS_SCAN8x32;
					bit_size -= 1;
				}
			}
			if (iVer)
			{
				if ( currMB->ui_MbBitSize == B16X16_IN_BIT )
				{
					SCAN = AVS_SCAN16x4;
				}
				else if ( currMB->ui_MbBitSize == B32X32_IN_BIT )
				{
					SCAN = AVS_SCAN32x8;
				}
				else if ( currMB->ui_MbBitSize == B64X64_IN_BIT )
				{
					SCAN = AVS_SCAN32x8;
					bit_size -= 1;
				}
			}
#if FREQUENCY_WEIGHTING_QUANTIZATION
			if ( currMB->ui_MbBitSize == B16X16_IN_BIT )
				WQMSizeId=2;
			else if ( (currMB->ui_MbBitSize== B32X32_IN_BIT)|| (currMB->ui_MbBitSize == B64X64_IN_BIT))
				WQMSizeId=3;
#endif
		}
		else
    {
//Adaptive frequency weighting quantization
#if FREQUENCY_WEIGHTING_QUANTIZATION
#if M3198_CU8
		if( currMB->ui_MbBitSize == B8X8_IN_BIT )
		{
			SCAN = AVS_SCAN4x4;
			WQMSizeId=0;
		}
		else
#endif
			if ( currMB->ui_MbBitSize == B16X16_IN_BIT )
			{
				SCAN = AVS_CG_SCAN8x8;
				WQMSizeId=1;
			}
			else if ( currMB->ui_MbBitSize == B32X32_IN_BIT )
			{
				SCAN = AVS_CG_SCAN16x16;
				WQMSizeId=2;
			}
			else if ( currMB->ui_MbBitSize == B64X64_IN_BIT )
			{
				SCAN = AVS_CG_SCAN32x32;
				WQMSizeId=3;
			}
#else //FREQUENCY_WEIGHTING_QUANTIZATION
#if M3198_CU8
      if( currMB->ui_MbBitSize == B8X8_IN_BIT )
      {
        SCAN = AVS_SCAN4x4;
      }
      else
#endif
      if ( currMB->ui_MbBitSize == B16X16_IN_BIT )
      {
      SCAN = AVS_CG_SCAN8x8;
      }
      else if ( currMB->ui_MbBitSize == B32X32_IN_BIT )
      {
        SCAN = AVS_CG_SCAN16x16;
      }
      else if ( currMB->ui_MbBitSize == B64X64_IN_BIT )
      {
        SCAN = AVS_CG_SCAN32x32;
      }
#endif //FREQUENCY_WEIGHTING_QUANTIZATION
    }

    for ( block_y = 0; block_y < 4; block_y += 2 ) /* all modes */
    {
      for ( block_x = 0; block_x < 4; block_x += 2 )
      {
        b8 = 2 * ( block_y / 2 ) + block_x / 2;
#if EXTRACT
		//fprintf(p_lcu_coeff,"block=%d bit_size=%d\n",b8,bit_size);//
#endif			
        if (iHor == 1)
        {
          iStartX = 0;
          iStartY = b8 * ( 1 << ( bit_size - 2 ) );
        }
        else if (iVer == 1)
        {
          iStartX = b8 * ( 1 << ( bit_size - 2 ) );
          iStartY = 0;
        }
        if ( currMB->cbp & ( 1 << b8 ) )
        {
          start_scan = 0; // take all coeffs
          coef_ctr = start_scan - 1;
          level    = 1;
          iCG = 0;
          pairs = 0;
#if EXTRACT
#if EXTRACT_08X
		  //fprintf(p_lcu_coeff,"%08X\n",b8);//打印block的序号 0-3
#endif
			   
#if EXTRACT_D
		  fprintf(p_lcu_coeff,"block=%d \n",b8);
#endif
#endif	

          for ( k = start_scan; ( k < ( 1 << ( bit_size - 1 ) ) * ( 1 << ( bit_size - 1 ) ) + 1 ) && ( level != 0 ); k++ )
          {
            //============ read =============
            currSE.context      = LUMA_8x8;
            currSE.type         = ( IS_INTRA ( currMB ) ) ? SE_LUM_AC_INTRA : SE_LUM_AC_INTER;
#if TRACE
            fprintf ( p_trace, "  Luma8x8 sng" );
#endif
            currMB->l_ipred_mode = currMB->intra_pred_modes[b8];
            dP = & ( currSlice->partArr[0] );
            currSE.reading = readRunLevelRef;
            dP->readSyntaxElement ( &currSE, dP, currMB, uiPosition );
            level = currSE.value1;
            run   = currSE.value2;
#if TRACE
            fprintf ( p_trace, "(%2d) level =%3d run =%2d\n", k, level, run );
            fflush ( p_trace );
#endif

            //============ decode =============
            if ( level != 0 )  /* leave if len=1 */
            {
              while ( DCT_CGFlag[ DCT_CGNum - iCG - 1 ] == 0 )
              {
                coef_ctr += 16;
                iCG ++;
              }
#if EXTRACT
              // pairs==0 表示是第一次读取完系数
              if(pairs==0){
#if EXTRACT_08X						
              fprintf(p_lcu_coeff, "%08X\n", ((cuPosInLCUPixY >> 3) << 16) + (cuPosInLCUPixX >> 3));
              fprintf(p_lcu_coeff, "%08X\n", b8);
              fprintf(p_lcu_coeff, "%08X\n",iCG);
#endif

#if EXTRACT_lcu_info_coeff_debug 
              fprintf(p_lcu_debug, "cuPosInLCUPixY,X=%4d %4d\n", cuPosInLCUPixY, cuPosInLCUPixX);
              fprintf(p_lcu_debug, "block %08X\n", b8);
              fprintf(p_lcu_debug, "iCG   %08X\n", iCG);
#endif	
#if EXTRACT_D					
              //fprintf(p_lcu,"iCG=%d coef_ctr=%d\n",iCG,coef_ctr);
              fprintf(p_lcu_coeff,"DCT_Pairs=%d DCT_CGNum=%d\n",DCT_Pairs,DCT_CGNum);
              fprintf(p_lcu_coeff,"iCG=%d coef_ctr=%d\n",iCG,coef_ctr);
#endif
				}
#endif	 

              pairs ++;
              coef_ctr += run + 1;
#if EXTRACT
			  //fprintf(p_lcu,"level=%d coef_ctr=%d\n",level,coef_ctr);//
#endif	

              i = SCAN[coef_ctr][0];
              j = SCAN[coef_ctr][1];

#if EXTRACT
              //fprintf(p_lcu,"level=%d coef_ctr=%d\n",level,coef_ctr);
              int tmp_run=0;
#if EXTRACT_08X
              while( tmp_run <run){
                fprintf(p_lcu_coeff,"%08X\n",0);
                tmp_run++;
              }
              fprintf(p_lcu_coeff,"%08X\n",level);
#endif
#if EXTRACT_lcu_info_coeff_debug 
              fprintf(p_lcu_debug, "level   %4d   run   %4d coef_ctr=%d\n", level, run, coef_ctr);
#endif	
				 
#if EXTRACT_D
              tmp_run=0;
              while( tmp_run <run){
                fprintf(p_lcu_coeff,"level=%d\n",0);
                tmp_run++;
              }
              fprintf(p_lcu_coeff,"level=%d coef_ctr=%d\n",level,coef_ctr);
#endif
						
#endif

              if( currSE.type == SE_LUM_AC_INTRA && g_intraModeClassified[currMB->l_ipred_mode] == INTRA_PRED_HOR&&currMB->cuType != InNxNMB&&currMB->cuType != INxnNMB )
              {
                SWAP(i, j);
              }
#if TRANS_16_BITS
              if (input->useNSQT && currMB->ui_MbBitSize > B8X8_IN_BIT && (iHor || iVer) && currMB->ui_MbBitSize == 6)
              {
#if EXTEND_BD
                shift = IQ_SHIFT[qp] + (input->sample_bit_depth + 1) + ( bit_size ) - LIMIT_BIT + 1;
#else
                shift = IQ_SHIFT[qp] + RESID_BIT + ( bit_size ) - LIMIT_BIT + 1;
#endif
              }
              else
              {
#if EXTEND_BD
                shift = IQ_SHIFT[qp] + (input->sample_bit_depth + 1) + ( bit_size - 1 ) - LIMIT_BIT + 1;
#else
                shift = IQ_SHIFT[qp] + RESID_BIT + ( bit_size - 1 ) - LIMIT_BIT + 1;
#endif
              }
#else
              shift = IQ_SHIFT[qp];
#endif
#if M3175_SHIFT
              shift ++;
#endif
#if SHIFT_DQ_INV
              shift --;
#endif
			  
#if EXTRACT

              cuCoeffY[j][i]=level;
#endif
              // Adaptive frequency weighting quantization
#if FREQUENCY_WEIGHTING_QUANTIZATION
              if (WeightQuantEnable)
              {
                wqm_coef=cur_wq_matrix[1][(j&7)*8+(i&7)];
                WQMSize=1<<(WQMSizeId+2);
                if((WQMSizeId==0)||(WQMSizeId==1))
                {
                  iStride=WQMSize;
                  wqm_coef=cur_wq_matrix[WQMSizeId][(j&(iStride-1))*iStride+(i&(iStride-1))];
                }
#if AWQ_LARGE_BLOCK_EXT_MAPPING
                else if (WQMSizeId==2)
                {
                  iStride=WQMSize>>1;
                  wqm_coef=cur_wq_matrix[WQMSizeId][((j>>1)&(iStride-1))*iStride+((i>>1)&(iStride-1))];
                }
                else if (WQMSizeId==3)
                {
                  iStride=WQMSize>>2;
                  wqm_coef=cur_wq_matrix[WQMSizeId][((j>>2)&(iStride-1))*iStride+((i>>2)&(iStride-1))];
                }
#endif
							}
							QPI   = IQ_TAB[qp];
							val   = level;
							if(WeightQuantEnable)
							{
#if AWQ_WEIGHTING
								sum = ((((((long long int)val*wqm_coef)>>wqm_shift)*QPI)>>4) + (1<<(shift-2)) )>>(shift-1);	// M2239, N1466
#else
								sum = ( val * QPI + ( 1 << ( shift - 2 ) ) ) >> ( shift - 1 );
#endif
							}
							else
							{
								sum = ( val * QPI + ( 1 << ( shift - 2 ) ) ) >> ( shift - 1 );
							}
#if QuantClip
              sum = Clip3(-32768, 32767, sum);
#endif
#else  // Adaptive frequency weighting quantization
							QPI   = IQ_TAB[qp];
							val =   level;
							sum = ( val * QPI + ( 1 << ( shift - 2 ) ) ) >> ( shift - 1 );
#endif  // Adaptive frequency weighting quantization


#if CLIP_FIX
              sum = Clip3 (  0 - ( 1 << 15 ), ( 1 << 15 ) - 1,  sum);
#endif
              if ((input->useNSQT && currMB->ui_MbBitSize > B8X8_IN_BIT &&(iVer || iHor)))
              {
                img->resiY[offset_y + iStartY + j][offset_x + iStartX + i] = sum;
              }
              else 
                if(input->useSDIP &&(iVer || iHor))
                {
                   img->resiY[offset_y + iStartY + j][offset_x + iStartX + i] = sum;
                }
                else
                {
                  img->resiY[offset_y + ( ( b8 / 2 ) << ( bit_size - 1 ) ) + j][offset_x + ( ( b8 % 2 ) << ( bit_size - 1 ) ) + i] = sum;
                }
            }
            if( pairs == DCT_PairsInCG[DCT_CGNum - iCG - 1] )
            {
#if EXTRACT
              int leftrun=coef_ctr&0xf;
              int tmp_run=0;
              leftrun=16-leftrun-1;
#if EXTRACT_08X			  
              while( tmp_run <leftrun){
              fprintf(p_lcu_coeff,"%08X\n",0);

              //fprintf(p_lcu_coeff, "%08X %d tmp_run=%d\n", 0, leftrun, tmp_run);
              tmp_run++;
              } 
#endif
	 
#if EXTRACT_D		
              leftrun=16-leftrun-1;	  
              while( tmp_run <leftrun){
              fprintf(p_lcu_coeff,"level=%d\n",0);
              tmp_run++;
              } 
#endif
#endif             
              coef_ctr |= 0xf;
              pairs = 0;
              iCG ++;
            }
          }
        }
      }
    }
  }
#else
  // luma coefficients
  for ( block_y = 0; block_y < 4; block_y += 2 ) /* all modes */
  {
    for ( block_x = 0; block_x < 4; block_x += 2 )
    {
      b8 = 2 * ( block_y / 2 ) + block_x / 2;

      if ( currMB->cbp & ( 1 << b8 ) )
      {
        {
          // === set offset in current codingUnit ===
          boff_x = ( b8 % 2 ) << 3;
          boff_y = ( b8 / 2 ) << 3;

          img->subblock_x = boff_x >> 2; // position for coeff_count ctx
          img->subblock_y = boff_y >> 2; // position for coeff_count ctx

          start_scan = 0; // take all coeffs
          coef_ctr = start_scan - 1;
          level    = 1;

          for ( k = start_scan; ( k < ( 1 << ( bit_size - 1 ) ) * ( 1 << ( bit_size - 1 ) ) + 1 ) && ( level != 0 ); k++ )
          {
            //============ read =============
            currSE.context      = LUMA_8x8;
            currSE.type         = ( IS_INTRA ( currMB ) ) ? SE_LUM_AC_INTRA : SE_LUM_AC_INTER;
#if TRACE
            fprintf ( p_trace, "  Luma8x8 sng" );
#endif
            dP = & ( currSlice->partArr[0] );
            currSE.reading = readRunLevelRef;
            dP->readSyntaxElement ( &currSE, dP, currMB, uiPosition );
            level = currSE.value1;
            run   = currSE.value2;
            len   = currSE.len;
#if TRACE
            fprintf ( p_trace, "(%2d) level =%3d run =%2d\n", k, level, run );
            fflush ( p_trace );
#endif

            //============ decode =============
            if ( level != 0 )  /* leave if len=1 */
            {
              coef_ctr += run + 1;

              i = SCAN[coef_ctr][0];
              j = SCAN[coef_ctr][1];

              shift = IQ_SHIFT[qp];
              QPI   = IQ_TAB[qp];
              val =   level;
              sum = ( val * QPI + ( 1 << ( shift - 2 ) ) ) >> ( shift - 1 );

#if EXTRACT
						
			cuCoeffY[j][i]=level;
#endif

              img->resiY[offset_y + ( ( b8 / 2 ) << ( bit_size - 1 ) ) + j][offset_x + ( ( b8 % 2 ) << ( bit_size - 1 ) ) + i] = sum;
            }
          }
        }
      }
    }
  }
#endif

  // Adaptive frequency weighting quantization
#if FREQUENCY_WEIGHTING_QUANTIZATION
#if INTRA_2N
#if M3198_CU8
  if ( currMB->ui_MbBitSize == B8X8_IN_BIT)
  {
	  SCAN = AVS_SCAN4x4;
	  WQMSizeId=0;
  }
  else
#endif
	  if ( currMB->ui_MbBitSize == B16X16_IN_BIT )
	  {
		  SCAN = AVS_CG_SCAN8x8;
		  WQMSizeId=1;
	  }
	  else if ( currMB->ui_MbBitSize == B32X32_IN_BIT )
	  {
		  SCAN = AVS_CG_SCAN16x16;
		  WQMSizeId=2;
	  }
	  else if ( currMB->ui_MbBitSize == B64X64_IN_BIT )
	  {
		  SCAN = AVS_CG_SCAN32x32;
		  WQMSizeId=3;
	  }
#endif
#else // Adaptive frequency weighting quantization
#if INTRA_2N
#if M3198_CU8
  if ( currMB->ui_MbBitSize == B8X8_IN_BIT)
  {
    SCAN = AVS_SCAN4x4;
  }
  else
#endif
  if ( currMB->ui_MbBitSize == B16X16_IN_BIT )
  {
    SCAN = AVS_CG_SCAN8x8;
  }
  else if ( currMB->ui_MbBitSize == B32X32_IN_BIT )
  {
    SCAN = AVS_CG_SCAN16x16;
  }
  else if ( currMB->ui_MbBitSize == B64X64_IN_BIT )
  {
    SCAN = AVS_CG_SCAN32x32;
  }
#endif
#endif  // Adaptive frequency weighting quantization


  uv = -1;
  block_y = 4;

  if (input->useNSQT && currMB->ui_MbBitSize > B8X8_IN_BIT &&((iHor || iVer)&& (currMB->ui_MbBitSize == B64X64_IN_BIT)))
  {
    bit_size++;
  }

  //读取两个色度的系数block
  for ( block_x = 0; block_x < 4; block_x += 2 )
  {
    uv++;

#if EXTRACT_08X
    //fprintf(p_lcu_coeff, "%08X\n", uv);//打印block的序号 0-1
#endif
#if EXTRACT_lcu_info_BAC
    fprintf(p_lcu,"UV block %d\n",uv);
    fprintf(p_lcu_coeff,"UV block %d block %d\n",uv,4+uv);
#endif
#if EXTEND_BD
    if (input->sample_bit_depth > 8)
    {
#if CHROMA_DELTA_QP_SYNTEX_OPT
      qp = currMB->qp - 8 * (input->sample_bit_depth - 8) + (uv==0?chroma_quant_param_delta_u:chroma_quant_param_delta_v);
      qp = qp < 0 ? qp : QP_SCALE_CR[qp];
      qp = Clip3(0, 63 + 8 * (input->sample_bit_depth - 8), qp + 8 * (input->sample_bit_depth - 8));
#else
      qp = QP_SCALE_CR[Clip3(0, 63, (currMB->qp - (8 * (input->sample_bit_depth - 8))))] + (8 * (input->sample_bit_depth - 8));
      //qp = Clip3(0, 51, (QP_SCALE_CR[Clip3(0, 63, (currMB->qp - 16))] + 16));
#if ( FREQUENCY_WEIGHTING_QUANTIZATION && CHROMA_DELTA_QP )
      if((pic_weight_quant_enable_flag)&&(!chroma_quant_param_disable))
      {
             qp = QP_SCALE_CR[Clip3(0, 63, (currMB->qp+(uv==0?chroma_quant_param_delta_u:chroma_quant_param_delta_v) - (8 * (input->sample_bit_depth - 8))))] + (8 * (input->sample_bit_depth - 8));
      }
#endif
#endif
    }
    else
    {
#endif
#if CHROMA_DELTA_QP_SYNTEX_OPT
      qp = currMB->qp - 8 * (input->sample_bit_depth - 8) + (uv==0?chroma_quant_param_delta_u:chroma_quant_param_delta_v);
      qp = qp < 0 ? qp : QP_SCALE_CR[qp];
      qp = Clip3(0, 63 + 8 * (input->sample_bit_depth - 8), qp + 8 * (input->sample_bit_depth - 8));
#else
      = QP_SCALE_CR[currMB->qp];
#if ( FREQUENCY_WEIGHTING_QUANTIZATION && CHROMA_DELTA_QP )
      if((pic_weight_quant_enable_flag)&&(!chroma_quant_param_disable))
      {
#if !CLIP
        qp = QP_SCALE_CR[currMB->qp+(uv==0?chroma_quant_param_delta_u:chroma_quant_param_delta_v)];
#else 
        qp = QP_SCALE_CR[Clip3(0, 63,currMB->qp+(uv==0?chroma_quant_param_delta_u:chroma_quant_param_delta_v))];
#endif
      }
#endif
#endif
#if EXTEND_BD
    }
#endif

    if ( ( currMB->cbp >> ( uv + 4 ) ) & 0x1 )
    {
      iCG = 0;
      pairs = 0;
      coef_ctr = -1;
      level = 1;

      currSE.context     = CHROMA;
	    currSE.type        = ( IS_INTRA ( currMB ) ? SE_CHR_AC_INTRA : SE_CHR_AC_INTER ); // element is of type DC
      dP = & ( currSlice->partArr[0] );
      currSE.reading = readRunLevelRef;

      for ( k = 0; ( k < ( ( 1 << ( bit_size - 1 ) ) * ( 1 << ( bit_size - 1 ) ) + 1 ) ) && ( level != 0 ); k++ )
      {
#if TRACE
        fprintf ( p_trace, "  AC chroma 8X8 " );
#endif
        dP->readSyntaxElement ( &currSE, dP, currMB, uiPosition );
        level = currSE.value1;
        run   = currSE.value2;
#if TRACE
        fprintf ( p_trace, "%2d: level =%3d run =%2d\n", k, level, run );
        fflush ( p_trace );
#endif

        if ( level != 0 )                   // leave if len=1
        {
          while ( DCT_CGFlag[ DCT_CGNum - iCG - 1 ] == 0 )
          {
            coef_ctr += 16;
            iCG ++;
          }    

#if EXTRACT
          if(pairs==0){
#if EXTRACT_08X 
				    //fprintf(p_lcu_coeff,"DCT_CGNum=%d DCT_Pairs=%d\n",DCT_CGNum ,DCT_Pairs);
				    //fprintf(p_lcu_coeff,"iCG %08X %d\n",iCG,iCG);

            fprintf(p_lcu_coeff, "%08X\n", ((cuPosInLCUPixY >> 3) << 16) + (cuPosInLCUPixX >> 3));
            fprintf(p_lcu_coeff, "%08X\n", uv);
				    fprintf(p_lcu_coeff, "%08X\n",iCG);
#endif			 

#if EXTRACT_lcu_info_coeff_debug 
            fprintf(p_lcu_debug, "cuPosInLCUPixY,X=%4d %4d\n", cuPosInLCUPixY, cuPosInLCUPixX);
            fprintf(p_lcu_debug, "block uv %08X\n", uv);
            fprintf(p_lcu_debug, "iCG   %08X\n", iCG);
#endif	
#if EXTRACT_D 
				    fprintf(p_lcu_coeff,"DCT_CGNum=%d DCT_Pairs=%d\n",DCT_CGNum ,DCT_Pairs);
				    fprintf(p_lcu_coeff,"iCG=%d coef_ctr=%d\n",iCG,coef_ctr);
#endif  	
		   
          }
#endif	           
          pairs ++;
          coef_ctr = coef_ctr + run + 1;
#if EXTRACT
#if EXTRACT_08X
          int tmp_run=0;
          while( tmp_run <run){
					  fprintf(p_lcu_coeff,"%08X\n",0);
					  tmp_run++;
          }
		  		fprintf(p_lcu_coeff,"%08X\n",level);		  
#endif
#if EXTRACT_lcu_info_coeff_debug 
          fprintf(p_lcu_debug, "level   %4d   run   %4d coef_ctr=%d\n", level, run, coef_ctr);
#endif	
#endif

          i0 = SCAN[coef_ctr][0];
          j0 = SCAN[coef_ctr][1];
#if TRANS_16_BITS
#if EXTEND_BD
          shift = IQ_SHIFT[qp] + (input->sample_bit_depth + 1) + ( bit_size - 1 ) - LIMIT_BIT + 1;
#else
          shift = IQ_SHIFT[qp] + RESID_BIT + ( bit_size - 1 ) - LIMIT_BIT + 1;
#endif
#else
          shift = IQ_SHIFT[qp];
#endif
#if M3175_SHIFT
          shift ++;
#endif
#if SHIFT_DQ_INV
          shift --;
#endif

	// Adaptive frequency weighting quantization  
#if FREQUENCY_WEIGHTING_QUANTIZATION
          if (WeightQuantEnable)
          {
            WQMSize=1<<(WQMSizeId+2);
            if((WQMSizeId==0)||(WQMSizeId==1))
            {
              iStride=WQMSize;
              wqm_coef=cur_wq_matrix[WQMSizeId][(j0&(iStride-1))*iStride+(i0&(iStride-1))];
            }

#if AWQ_LARGE_BLOCK_EXT_MAPPING
            else if (WQMSizeId==2)
            {
              iStride=WQMSize>>1;
              wqm_coef=cur_wq_matrix[WQMSizeId][((j0>>1)&(iStride-1))*iStride+((i0>>1)&(iStride-1))];
            }
            else if (WQMSizeId==3)
            {
              iStride=WQMSize>>2;
              wqm_coef=cur_wq_matrix[WQMSizeId][((j0>>2)&(iStride-1))*iStride+((i0>>2)&(iStride-1))];
            }
#endif
          }
          QPI   = IQ_TAB[qp];
          val   = level;
          if(WeightQuantEnable)
          {
#if AWQ_WEIGHTING
			  //sum = ((((((int)val*wqm_coef)>>3)*QPI)>>4) + (1<<(shift-2)) )>>(shift-1);	// M2239, N1466
            sum = ((((((long long int)val*wqm_coef)>>wqm_shift)*QPI)>>4) + (1<<(shift-2)) )>>(shift-1);	// M2239, N1466
#else
            sum = ( val * QPI + ( 1 << ( shift - 2 ) ) ) >> ( shift - 1 );
#endif
          }
          else
          {
            sum = ( val * QPI + ( 1 << ( shift - 2 ) ) ) >> ( shift - 1 );
          }
#if QuantClip
          sum = Clip3(-32768, 32767, sum);
#endif
#else // Adaptive frequency weighting quantization
          QPI   = IQ_TAB[qp];
          val   = level;
          sum = ( val * QPI + ( 1 << ( shift - 2 ) ) ) >> ( shift - 1 );
#endif // Adaptive frequency weighting quantization 


#if CLIP_FIX
          sum = Clip3 (  0 - ( 1 << 15 ), ( 1 << 15 ) - 1,  sum);
#endif
#if EXTRACT
          if(uv==0){
            cuCoeffCb[j0][i0]=level;
          }					
          else
            cuCoeffCr[j0][i0]=level;
          //fprintf(p_lcu,"level=%d coef_ctr=%d j0=%d i0=%d\n",level,coef_ctr,j0,i0);
#endif

          img->resiUV[uv][offset_y / 2 + j0][offset_x / 2 + i0] = sum;
        } //level!=0
#if EXTRACT
				else
				{
#if EXTRACT_08X
				  //fprintf(p_lcu_coeff,"%08X\n",0);
#endif
			 
#if EXTRACT_D
				  fprintf(p_lcu_coeff,"level=%d\n",0);
#endif			
				}
#endif
        if( pairs == DCT_PairsInCG[DCT_CGNum - iCG - 1] )
        {

#if EXTRACT
				  int leftrun=coef_ctr&0xf;
				  int tmp_run=0;
				  leftrun=16-leftrun-1;
#if EXTRACT_08X		  
				  while( tmp_run <leftrun)
					{
						//fprintf(p_lcu_coeff,"%08X %d tmp_run=%d\n",0, leftrun, tmp_run);
            fprintf(p_lcu_coeff, "%08X\n", 0);
						tmp_run++;
					} 
#endif
			 
#if EXTRACT_D	
				  leftrun=coef_ctr&0xf;
				  tmp_run=0;
				  leftrun=16-leftrun-1;	  
				  while( tmp_run <leftrun)
					{
						fprintf(p_lcu_coeff,"level=%d\n",0);
						tmp_run++;
				  } 
#endif

#endif              
          coef_ctr |= 0xf;
          pairs = 0;
          iCG ++;
        }
      } //end k=0~64
#if EXTRACT_Coeff

#if EXTRACT_D
      fprintf(p_lcu_coeff,"DCT_CGNum=%d\n",DCT_CGNum);
#endif

      int coeffInUV=0,coeffInBlcok=0,cgInBlock=0,cuW=0;
      if(uv==0){
        iCG=0;
        coef_ctr=0;
        while( iCG < DCT_CGNum)
        {
          if(DCT_CGFlag[ DCT_CGNum - iCG - 1 ] == 1){
            fprintf(p_lcu_coeff,"iCG=%d\n",iCG);
            for(coeffInUV=0;coeffInUV<16;coeffInUV++)
            {
              coeffInBlcok = coef_ctr+coeffInUV;
              i0 = SCAN[coeffInBlcok][0];
                j0 = SCAN[coeffInBlcok][1];
              fprintf(p_lcu_coeff,"%d\n",cuCoeffCb[j0][i0]);
            }
          }
        
          coef_ctr += 16;
          iCG ++;
        } 
		 
      }
      else
	  	{
	      iCG=0;
        coef_ctr=0;
        while( iCG< DCT_CGNum)
        {
          if(DCT_CGFlag[ DCT_CGNum - iCG - 1 ] == 1){
            fprintf(p_lcu_coeff,"iCG=%d\n",iCG);
            for(coeffInUV=0;coeffInUV<16;coeffInUV++)
            {
              coeffInBlcok = coef_ctr+coeffInUV;
            i0 = SCAN[coeffInBlcok][0];
                j0 = SCAN[coeffInBlcok][1];
              fprintf(p_lcu_coeff,"%d\n",cuCoeffCr[j0][i0]);
            }
          }
        
          coef_ctr += 16;
          iCG ++;
        }
	  	}
#endif
    }
  }
#if EXTRACT
	  fflush(p_lcu);


	int coefii,coefjj,cuW;
  if ( currMB->ui_MbBitSize == B64X64_IN_BIT )
  {
    cuW = currMB->ui_MbBitSize - 1;
  }
  else
  {
    cuW = currMB->ui_MbBitSize;
  }
  cuW=(1<<cuW);
  /*
  //cuW=cuW;
  	fprintf(p_lcu,"coeff Y cuW=%d\n",cuW);// 
	  fflush(p_lcu);
  for(coefjj=0;coefjj<cuW;coefjj++){
  	for(coefii=0;coefii<cuW;coefii++)
  		fprintf(p_lcu,"%5d\t",cuCoeffY[coefjj][coefii]);//  
  	fprintf(p_lcu,"\n");// 
  }
	  fflush(p_lcu);
  	fprintf(p_lcu,"coeff Cb\n");// 
  for(coefjj=0;coefjj<cuW/2;coefjj++){
  	for(coefii=0;coefii<cuW/2;coefii++)
  		fprintf(p_lcu,"%5d\t",cuCoeffCb[coefjj][coefii]);//  
  	fprintf(p_lcu,"\n");// 
  }
	  fflush(p_lcu);
  	fprintf(p_lcu,"coeff Cr\n");// 
  for(coefjj=0;coefjj<cuW/2;coefjj++){
  	for(coefii=0;coefii<cuW/2;coefii++)
  		fprintf(p_lcu,"%5d\t",cuCoeffCr[coefjj][coefii]);//  
  	fprintf(p_lcu,"\n");// 
  }
	  fflush(p_lcu);
*/


#endif  
}

/*
*************************************************************************
* Function:decode one codingUnit
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
void decode_one_InterLumaBlock( int block8Nx8N )
{
  codingUnit *currMB   = &img->mb_data[img->current_mb_nr];//GB current_mb_nr];

  int tmp_block[MAX_CU_SIZE][MAX_CU_SIZE];
  int tmp_blockbw[MAX_CU_SIZE][MAX_CU_SIZE];
#if WEIGHTED_SKIP
  int tmp_block_wgt[MAX_CU_SIZE][MAX_CU_SIZE];
#endif
  int i = 0, j = 0, ii = 0, jj = 0, j8 = 0, i8 = 0;

  int vec1_x = 0, vec1_y = 0, vec2_x = 0, vec2_y = 0;
  int ioff, joff;

  int mv_mul = 4;

  int refframe, fw_refframe, bw_refframe, mv_mode, pred_dir; // = currMB->ref_frame;
  int bw_ref_idx;
  int*** mv_array, ***fw_mv_array, ***bw_mv_array;
  int bframe = ( img->type == B_IMG );
  int frame_no_B, delta_P, delta_P_scale;

  int mb_nr             = img->current_mb_nr;//GBimg->current_mb_nr;

  int mb_BitSize = img->mb_data[mb_nr].ui_MbBitSize;
  int start_x, start_y, step_h, step_v;

  int scale_refframe;
  // !! shenyanfei

#if WEIGHTED_SKIP
  int vec_wgt_x, vec_wgt_y;
  int delta[4];
#endif

  int first_x, first_y, second_x, second_y;
  int dmh_mode = 0;

#if WEIGHTED_SKIP
  delta[0] = ( 2 * ( picture_distance - img->imgtr_fwRefDistance[0] ) + 512 ) % 512;
  delta[1] = ( 2 * ( picture_distance - img->imgtr_fwRefDistance[1] ) + 512 ) % 512;
  delta[2] = ( 2 * ( picture_distance - img->imgtr_fwRefDistance[2] ) + 512 ) % 512;
  delta[3] = ( 2 * ( picture_distance - img->imgtr_fwRefDistance[3] ) + 512 ) % 512;
#if AVS2_SCENE_MV
  if (0 == curr_RPS.num_of_ref - 1 && background_reference_enable){
	  delta[0] = 1;
  }
  if (1 == curr_RPS.num_of_ref - 1 && background_reference_enable) {
	  delta[1] = 1;
  }
  if (2 == curr_RPS.num_of_ref - 1 && background_reference_enable) {
	  delta[2] = 1;
  }
  if (3 == curr_RPS.num_of_ref - 1 && background_reference_enable) {
	  delta[3] = 1;
  }
#endif
#endif
	//当前块的索引 0-3
  i = block8Nx8N % 2;
  j = block8Nx8N / 2;
#if EXTRACT //_lcu_info_BAC
  int puNumInCU=0;
#endif	

  {
    get_b8_offset( currMB->cuType, mb_BitSize, i, j, &start_x, &start_y, &step_h, &step_v );
    i8 = img->block8_x + start_x;
    j8 = img->block8_y + start_y;
    get_pix_offset( currMB->cuType, mb_BitSize, i, j, &start_x, &start_y, &step_h, &step_v );
    ioff = start_x;
    joff = start_y;
  }


  mv_mode  = currMB->b8mode[block8Nx8N];
  pred_dir = currMB->b8pdir[block8Nx8N];
#if EXTRACT
  char * ttPicTypeStr[6]={"I_IMG","P_IMG","B_IMG","BACKGROUND_IMG","F_IMG","BP_IMG"};
  char * ttPredDirStr[6]={"INTRA","FORWARD","BACKWARD","SYM","BID","DUAL"};//pred_dir+1来读取字符串，因为-1对应索引0

#endif
#if EXTRACT_lcu_info_mvp_debug
    fprintf(p_lcu_debug, "decode_one_InterLumaBlock block8Nx8N=%4d j8=%4d i8=%4d\n", block8Nx8N, j8,i8);
#endif
#if EXTRACT_MV_debug
	fprintf(p_lcu_mv,"%s mv_mode=%s pred_dir=%s\n",ttPicTypeStr[img->type],aInterPredModeStr[mv_mode],ttPredDirStr[pred_dir+1]);//
	fprintf(p_lcu_mv,"joff=%d ioff=%d \n",joff,ioff);//
	fprintf(p_lcu_mv,"j8=%d i8=%d \n",j8,i8);//
#endif
  //#define DUAL                         4
  //#define FORWARD                      0
  //#define BACKWARD                     1
  if ( pred_dir != SYM && pred_dir != BID )
  {
    //===== FORWARD/BACKWARD PREDICTION =====
    if ( !bframe ) // !! P MB shenyanfei    F DUAL  F FORWARD
    {
      refframe = refFrArr[j8][i8];
      mv_array = img->tmp_mv;
    }
    else if ( !pred_dir ) // !! B forward shenyanfei
    {
      refframe = img->fw_refFrArr[j8][i8];// fwd_ref_idx_to_refframe(img->fw_refFrArr[j8][i8]);
      mv_array = img->fw_mv;
    }
    else // !! B backward shenyanfei
    {
      refframe = img->bw_refFrArr[j8][i8];// bwd_ref_idx_to_refframe(img->bw_refFrArr[j8][i8]);
      bw_ref_idx = img->bw_refFrArr[j8][i8];
      mv_array = img->bw_mv;
    }
#if INTERLACE_CODING
    if ( bframe )
#else
    if ( ( progressive_sequence || progressive_frame ) && bframe && img->picture_structure )
#endif
    {
      refframe = 0;
    }

    vec1_x = ( img->pix_x + ioff ) * mv_mul + mv_array[j8][i8][0];
    vec1_y = ( img->pix_y + joff ) * mv_mul + mv_array[j8][i8][1];
#if EXTRACT_1
    //EcuInfoInterSyntax[20+k+n]= currSE.value1;//PF pu_reference_index 
    //fprintf(p_lcu_mv,"MV  block%d MV:%d %d  refframe=%d vec1\n",block8Nx8N,mv_array[j8][i8][0],mv_array[j8][i8][1],refframe);//F pu_reference_index
    if ( pred_dir == FORWARD)//单前向
		{
			puInfoInter[block8Nx8N][10]=refframe;//block8Nx8N 是4个子块的序号
			puInfoInter[block8Nx8N][12]=mv_array[j8][i8][0];
			puInfoInter[block8Nx8N][13]=mv_array[j8][i8][1];
			puInfoInter[block8Nx8N][11]=-1;
			puInfoInter[block8Nx8N][14]=0;
			puInfoInter[block8Nx8N][15]=0;
		}
		else if( pred_dir==BACKWARD) //单后向
		{
			puInfoInter[block8Nx8N][10]=-1;//block8Nx8N 是4个子块的序号
			puInfoInter[block8Nx8N][12]=0;
			puInfoInter[block8Nx8N][13]=0;
			puInfoInter[block8Nx8N][11]=refframe;
			puInfoInter[block8Nx8N][14]=mv_array[j8][i8][0];
			puInfoInter[block8Nx8N][15]=mv_array[j8][i8][1];
		}
		else if ( pred_dir==DUAL) //双前向
		{
			puInfoInter[block8Nx8N][10]=refframe;//block8Nx8N 是4个子块的序号
			puInfoInter[block8Nx8N][12]=mv_array[j8][i8][0];
			puInfoInter[block8Nx8N][13]=mv_array[j8][i8][1];
		}
#endif

//F mv_mode=0 是skip 、direct
#if WEIGHTED_SKIP    
    if ( img->num_of_references > 1 && ( !mv_mode ) &&  img->type == F_IMG && currMB->weighted_skipmode )
    {
#if MV_SCALING_UNIFY
      vec_wgt_x = ( img->pix_x + ioff ) * mv_mul + ( int ) ( delta[currMB->weighted_skipmode ] * mv_array[j8][i8][0] * (MULTI / delta[0]) + HALF_MULTI >> OFFSET );
      vec_wgt_y = ( img->pix_y + joff ) * mv_mul + ( int ) ( delta[currMB->weighted_skipmode ] * mv_array[j8][i8][1] * (MULTI / delta[0]) + HALF_MULTI >> OFFSET );
#else
      vec_wgt_x = ( img->pix_x + ioff ) * mv_mul + ( int ) ( delta[currMB->weighted_skipmode ] * mv_array[j8][i8][0] * (16384 / delta[0]) + 8192 >> 14 );
      vec_wgt_y = ( img->pix_y + joff ) * mv_mul + ( int ) ( delta[currMB->weighted_skipmode ] * mv_array[j8][i8][1] * (16384 / delta[0]) + 8192 >> 14 );
#endif
#if HALF_PIXEL_COMPENSATION_M1
      assert(is_field_sequence == img->is_field_sequence);
      if (img->is_field_sequence)
      {
        int deltaT, delta2;
        int oriPOC = 2*picture_distance;
        int oriRefPOC = oriPOC - delta[0];
        int scaledPOC = 2*picture_distance;
        int scaledRefPOC = scaledPOC - delta[currMB->weighted_skipmode ];
        getDeltas( &deltaT, &delta2, oriPOC, oriRefPOC, scaledPOC, scaledRefPOC);
        vec_wgt_y = ( img->pix_y + joff ) * mv_mul + ( int ) ( delta[currMB->weighted_skipmode ] * (mv_array[j8][i8][1]+deltaT) * (16384 / delta[0]) + 8192 >> 14 ) -delta2;
      }
#endif
#if m3469
      vec_wgt_x = Clip3(-32768, 32767, vec_wgt_x);
      vec_wgt_y = Clip3(-32768, 32767, vec_wgt_y);
#endif
		}  
#endif

#ifdef AVS2_S2_S
	if (  img->type == F_IMG && b_dmh_enabled && img->typeb != BP_IMG )
#else
    if (  img->type == F_IMG && b_dmh_enabled )
#endif
     {
       if ( mv_mode == 0 )
       {
         dmh_mode = 0;
       }
       else
       {
         dmh_mode = mv_array[j8][i8][3];

         first_x  = dmh_pos[dmh_mode][0][0]; first_y  = dmh_pos[dmh_mode][0][1];
         second_x = dmh_pos[dmh_mode][1][0]; second_y = dmh_pos[dmh_mode][1][1];
       }
    }

    if ( !bframe )//P F 帧
    {
      //CHECKMOTIONRANGE
#ifdef AVS2_S2_SBD

#ifdef AVS2_S2_S 
		// dmh_mode=0: Skip\Direct 

		// SPS b_dmh_enabled=0 
		// img->type != F_IMG   P
		// img->typeb == BP_IMG   S
		if( dmh_mode == 0 || ! b_dmh_enabled || img->type != F_IMG || img->typeb == BP_IMG)
#else 
		if( dmh_mode == 0 || ! b_dmh_enabled || img->type != F_IMG)
#endif

#else

#ifdef AVS2_S2_S
		if ( dmh_mode == 0 || !b_dmh_enabled || img->type != F_IMG|| img->typeb == BP_IMG)
#else 
		if ( dmh_mode == 0 || !b_dmh_enabled || img->type != F_IMG )
#endif

#endif
		{
#if	  EXTRACT_F_MV_debug
			fprintf(p_lcu_mv,"MV	block%d MV:%d %d  ref=%d BP_IMG dmh_mode == 0)\n",block8Nx8N,vec1_x-( img->pix_x + ioff )*mv_mul,vec1_y-( img->pix_y + joff )*mv_mul,refframe);//F 
#endif	      
#if EXTRACT				
		  puInfoInter[block8Nx8N][10]=refframe;//block8Nx8N 是4个子块的序号
		  puInfoInter[block8Nx8N][12]=vec1_x-( img->pix_x + ioff )*mv_mul;
		  puInfoInter[block8Nx8N][13]=vec1_y-( img->pix_y + joff )*mv_mul;
#endif

			get_block ( refframe, vec1_x, vec1_y, step_h, step_v, tmp_block, integerRefY[refframe] 
#ifdef AVS2_S2_SBD
								, ioff, joff
#endif		
								); // need fix
      }
      else 
      {
        first_x  = dmh_pos[dmh_mode][0][0]; first_y  = dmh_pos[dmh_mode][0][1];
        second_x = dmh_pos[dmh_mode][1][0]; second_y = dmh_pos[dmh_mode][1][1];

#if	  EXTRACT_F_MV_debug
				fprintf(p_lcu_mv,"MV	block%d MV:%d %d  DMH)\n",block8Nx8N,mv_array[j8][i8][0] + first_x,mv_array[j8][i8][1] + first_y,refframe);//F 
				fprintf(p_lcu_mv,"MV	block%d MV:%d %d  DMH)\n",block8Nx8N,mv_array[j8][i8][0] + second_x,mv_array[j8][i8][1] + second_y,refframe);//F 
#endif	 


				get_block ( refframe, vec1_x + first_x, vec1_y + first_y, step_h, step_v, tmp_block, integerRefY[refframe] 
#ifdef AVS2_S2_SBD
									, ioff, joff
#endif		
									); // need fix
				get_block ( refframe, vec1_x + second_x, vec1_y + second_y, step_h, step_v, tmp_blockbw, integerRefY[refframe] 
#ifdef AVS2_S2_SBD
									, ioff, joff
#endif		
									); // need fix // re-use of tmp_blockbw for memory saving
#if EXTRACT
	    //EcuInfoInterSyntax[20+k+n]= currSE.value1;//PF pu_reference_index 
	    //fprintf(p_lcu_mv,"MV  DMH=%d block%d MV:%d %d  refframe=%d First\n",dmh_mode,block8Nx8N,mv_array[j8][i8][0] + first_x,mv_array[j8][i8][1] + first_y,refframe);//F 
	    //fprintf(p_lcu_mv,"MV  DMH=%d block%d MV:%d %d  refframe=%d First\n",dmh_mode,block8Nx8N,mv_array[j8][i8][0] + second_x,mv_array[j8][i8][1] + second_y,refframe);//F 
			puInfoInter[block8Nx8N][10]=refframe;//block8Nx8N 是4个子块的序号
			puInfoInter[block8Nx8N][12]=mv_array[j8][i8][0] + first_x;
			puInfoInter[block8Nx8N][13]=mv_array[j8][i8][1] + first_y;
			puInfoInter[block8Nx8N][11]=refframe;//block8Nx8N 是4个子块的序号
			puInfoInter[block8Nx8N][14]=mv_array[j8][i8][0] + second_x;
			puInfoInter[block8Nx8N][15]=mv_array[j8][i8][1] + second_y;	
#endif		
        for ( i = 0; i < MAX_CU_SIZE; i++ )
        {
          for ( j = 0; j < MAX_CU_SIZE; j++ )
          {
            tmp_block[j][i] = ( tmp_block[j][i] + tmp_blockbw[j][i] + 1 ) / 2;
          }
        }
      }

#if WEIGHTED_SKIP    
	    if ( img->num_of_references > 1 && img->type == F_IMG && currMB->weighted_skipmode  && ( mv_mode == 0 ) )
	    {
#if	  EXTRACT_F_MV_debug
		  fprintf(p_lcu_mv,"MV	block%d MV:%d %d get_block ( currMB->weighted_skipmode\n",block8Nx8N,img->p_snd_tmp_mv[j8][i8][0],img->p_snd_tmp_mv[j8][i8][1],p_snd_refFrArr[j8][i8]);//F 
#endif	  
#if EXTRACT				
		  puInfoInter[block8Nx8N][11]=currMB->weighted_skipmode;//block8Nx8N 是4个子块的序号
		  puInfoInter[block8Nx8N][14]=vec_wgt_x-( img->pix_x + ioff )*mv_mul;
		  puInfoInter[block8Nx8N][15]=vec_wgt_y-( img->pix_y + joff )*mv_mul;
#endif

      	  get_block ( currMB->weighted_skipmode, vec_wgt_x, vec_wgt_y, step_h, step_v, tmp_block_wgt, integerRefY[currMB->weighted_skipmode] 
#ifdef AVS2_S2_SBD
          , ioff, joff
#endif		  
          );

	      for ( i = 0; i < MAX_CU_SIZE; i++ )
	      {
	        for ( j = 0; j < MAX_CU_SIZE; j++ )
	        {
	          tmp_block[j][i] = ( tmp_block[j][i] + tmp_block_wgt[j][i] + 1 ) / 2;
	        }
	      }
    	}
#endif

	  	if(  img->type == F_IMG && pred_dir == DUAL )
		  {
#if	  EXTRACT_F_MV_debug
		    fprintf(p_lcu_mv,"MV	block%d MV:%d %d F DUAL\n",block8Nx8N,img->p_snd_tmp_mv[j8][i8][0],img->p_snd_tmp_mv[j8][i8][1],p_snd_refFrArr[j8][i8]);//F 
#endif	  
			  vec_wgt_x = ( img->pix_x + ioff ) * mv_mul + img->p_snd_tmp_mv[j8][i8][0];
			  vec_wgt_y = ( img->pix_y + joff ) * mv_mul + img->p_snd_tmp_mv[j8][i8][1];
#if EXTRACT
			  //EcuInfoInterSyntax[20+k+n]= currSE.value1;//PF pu_reference_index 
			  //fprintf(p_lcu_mv,"MV	block%d MV:%d %d refframe=%d Snd F_IMG DUAL\n",block8Nx8N,img->p_snd_tmp_mv[j8][i8][0],img->p_snd_tmp_mv[j8][i8][1],p_snd_refFrArr[j8][i8]);//F pu_reference_index
			  puInfoInter[block8Nx8N][11]=p_snd_refFrArr[j8][i8];//block8Nx8N 是4个子块的序号
			  puInfoInter[block8Nx8N][14]=img->p_snd_tmp_mv[j8][i8][0];
			  puInfoInter[block8Nx8N][15]=img->p_snd_tmp_mv[j8][i8][1];
#endif

			  get_block ( p_snd_refFrArr[j8][i8], vec_wgt_x, vec_wgt_y, step_h, step_v, tmp_block_wgt, integerRefY[p_snd_refFrArr[j8][i8]] 
#ifdef AVS2_S2_SBD
			, ioff, joff
#endif		  
			  );

			  for ( i = 0; i < MAX_CU_SIZE; i++ )
			  {
				  for ( j = 0; j < MAX_CU_SIZE; j++ )
				  {
					  tmp_block[j][i] = ( tmp_block[j][i] + tmp_block_wgt[j][i] + 1 ) / 2;
				  }
			  }
		  }


	    if(  img->type == F_IMG && (currMB->md_directskip_mode == BID_P_FST || currMB->md_directskip_mode == BID_P_SND))
			{
#if	  EXTRACT_F_MV_debug
          fprintf(p_lcu_mv,"MV	block%d MV:%d %d Snd F_IMG BID_P_FST|BID_P_SND\n",block8Nx8N,img->p_snd_tmp_mv[j8][i8][0],img->p_snd_tmp_mv[j8][i8][1],p_snd_refFrArr[j8][i8]);//F 
#endif
          vec_wgt_x = ( img->pix_x + ioff ) * mv_mul + img->p_snd_tmp_mv[j8][i8][0];
          vec_wgt_y = ( img->pix_y + joff ) * mv_mul + img->p_snd_tmp_mv[j8][i8][1];
#if EXTRACT
			    //EcuInfoInterSyntax[20+k+n]= currSE.value1;//PF pu_reference_index 
			    //fprintf(p_lcu_mv,"MV	block%d MV:%d %d Snd F_IMG BID_P_FST|BID_P_SND\n",block8Nx8N,img->p_snd_tmp_mv[j8][i8][0],img->p_snd_tmp_mv[j8][i8][1],p_snd_refFrArr[j8][i8]);//F pu_reference_index
			    puInfoInter[block8Nx8N][11]=p_snd_refFrArr[j8][i8];//block8Nx8N 是4个子块的序号
			    puInfoInter[block8Nx8N][14]=img->p_snd_tmp_mv[j8][i8][0];
					puInfoInter[block8Nx8N][15]=img->p_snd_tmp_mv[j8][i8][1];
					
#endif

				get_block ( p_snd_refFrArr[j8][i8], vec_wgt_x, vec_wgt_y, step_h, step_v, tmp_block_wgt, integerRefY[p_snd_refFrArr[j8][i8]] 
#ifdef AVS2_S2_SBD
									, ioff, joff
#endif		
									);

				for ( i = 0; i < MAX_CU_SIZE; i++ )
				{
					for ( j = 0; j < MAX_CU_SIZE; j++ )
					{
						tmp_block[j][i] = ( tmp_block[j][i] + tmp_block_wgt[j][i] + 1 ) / 2;
					}
				}
			}

    }
    else // !! for B MB one direction  shenyanfei cjw
    {
      // pred_dir=0  单前向
      if ( !pred_dir )   // !! forward shenyanfei
      {
        //CHECKMOTIONRANGE
        //CHECKMOTIONDIRECTION
        get_block ( refframe, vec1_x, vec1_y, step_h, step_v, tmp_block, integerRefY_fref[refframe] 
#ifdef AVS2_S2_SBD
									, ioff, joff
#endif		
									);
#if EXTRACT
			  puInfoInter[block8Nx8N][10]=refframe;//block8Nx8N 是4个子块的序号
			  puInfoInter[block8Nx8N][12]=mv_array[j8][i8][0];
			  puInfoInter[block8Nx8N][13]=mv_array[j8][i8][1];
			  puInfoInter[block8Nx8N][11]=-1;
			  puInfoInter[block8Nx8N][14]=0;
			  puInfoInter[block8Nx8N][15]=0;
#endif			  
      }
      else   // !! backward shenyanfei
      {
        //CHECKMOTIONRANGE
        get_block ( refframe, vec1_x, vec1_y, step_h, step_v, tmp_block, integerRefY_bref[refframe] 
#ifdef AVS2_S2_SBD
					, ioff, joff
#endif		
					);
	  
#if EXTRACT
					puInfoInter[block8Nx8N][11]=refframe;//block8Nx8N 是4个子块的序号
					puInfoInter[block8Nx8N][14]=mv_array[j8][i8][0];
					puInfoInter[block8Nx8N][15]=mv_array[j8][i8][1];
					puInfoInter[block8Nx8N][10]=-1;
					puInfoInter[block8Nx8N][12]=0;
					puInfoInter[block8Nx8N][13]=0;
#endif		
      }
    }
	
	  //将预测值从tmp_block 拷贝到img->predBlock
    for ( ii = 0; ii < step_h; ii++ )
    {
      for ( jj = 0; jj < step_v; jj++ )
      {
        img->predBlock[jj + joff][ii + ioff] = tmp_block[jj][ii];
#if EXTRACT_PIC_YUV_PRED   //2016-5-23 提取整帧图像的经过预测后的数据
        pPicYPred[(img->pix_y + joff + jj)*img->width + img->pix_x + ioff+ii] = tmp_block[jj][ii];
#endif
      }
    }

#if EXTRACT_lcu_info_debug
    if (1)
    {
      int x, y;
      fprintf(p_lcu_debug, "InterPred Y CU posx,y=%d %d  blk x,y=%d %d\n", img->pix_x + ioff, img->pix_y + joff, step_v, step_h);

      for (ii = 0; ii < step_h; ii++)
      {
        for (jj = 0; jj < step_v; jj++)
        {
          img->predBlock[jj + joff][ii + ioff] = tmp_block[jj][ii];
          //img->predBlock[y + y_off][x + x_off] = piPredBuf[y][x];
          fprintf(p_lcu_debug, "%4d", img->predBlock[jj + joff][ii + ioff]);
        }
        fprintf(p_lcu_debug, "\n");
      }
    }
#endif
  }
  else  // !! pred_dir == 2
  {

#if EXTRACT_lcu_info_mvp_debug
    if (1)
    {
      fprintf(p_lcu_debug, "img->fw_mv[%4d][%4d][0] =%4d\n", j8, i8,img->fw_mv[j8][i8][0]);
      fprintf(p_lcu_debug, "img->fw_mv[%4d][%4d][1] =%4d\n", j8, i8,img->fw_mv[j8][i8][1]);

      fprintf(p_lcu_debug, "img->bw_mv[%4d][%4d][0] =%4d\n", j8, i8,img->fw_mv[j8][i8][0]);
      fprintf(p_lcu_debug, "img->bw_mv[%4d][%4d][1] =%4d\n", j8, i8,img->fw_mv[j8][i8][1]);
    }
#endif
    if ( mv_mode != 0 )
    {
      //===== BI-DIRECTIONAL PREDICTION =====

      fw_mv_array = img->fw_mv;
      bw_mv_array = img->bw_mv;

      fw_refframe = img->fw_refFrArr[j8][i8];// fwd_ref_idx_to_refframe(img->fw_refFrArr[j8][i8]);
      bw_refframe = img->bw_refFrArr[j8][i8];// bwd_ref_idx_to_refframe(img->bw_refFrArr[j8][i8]);
      bw_ref_idx = img->bw_refFrArr[j8][i8];

    }
    else
    {
      //===== DIRECT PREDICTION =====
      fw_mv_array = img->fw_mv;
      bw_mv_array = img->bw_mv;
      bw_refframe = 0;


      if ( refFrArr[j8][i8] == -1 ) // next P is intra mode
      {
        fw_refframe = 0;
      }
      else // next P is skip or inter mode
      {
        refframe = refFrArr[j8][i8];

        frame_no_B = 2 * img->pic_distance;
        delta_P = 2 * ( img->imgtr_next_P - img->imgtr_fwRefDistance[0] );
        delta_P = ( delta_P + 512 ) % 512;
        delta_P_scale = 2 * ( img->imgtr_next_P - img->imgtr_fwRefDistance[1] ); // direct, 20071224
        delta_P_scale = ( delta_P_scale + 512 ) % 512;          // direct, 20071224

        scale_refframe = 0;

        fw_refframe = 0;  // DIRECT
        img->fw_refFrArr[j8][i8] = 0;
        img->bw_refFrArr[j8][i8] = 0;
      }
    }

    vec1_x = ( img->pix_x + start_x ) * mv_mul + fw_mv_array[j8][i8][0];
    vec1_y = ( img->pix_y + start_y ) * mv_mul + fw_mv_array[j8][i8][1];
#if EXTRACT
    //EcuInfoInterSyntax[20+k+n]= currSE.value1;//PF pu_reference_index 
    //fprintf(p_lcu_mv,"MV  block%d MV[%4d][%4d]:%d %d ref=%d Bi-Fwd\n",block8Nx8N,j8,i8,fw_mv_array[j8][i8][0],fw_mv_array[j8][i8][1],fw_refframe);//
    puInfoInter[block8Nx8N][10]=fw_refframe;

    puInfoInter[block8Nx8N][12]=fw_mv_array[j8][i8][0];
    puInfoInter[block8Nx8N][13]=fw_mv_array[j8][i8][1];
#endif

    //CHECKMOTIONRANGE
    //CHECKMOTIONDIRECTION
    //rm52k

    vec2_x = ( img->pix_x + start_x ) * mv_mul + bw_mv_array[j8][i8][0];
    vec2_y = ( img->pix_y + start_y ) * mv_mul + bw_mv_array[j8][i8][1];
#if EXTRACT
    //EcuInfoInterSyntax[20+k+n]= currSE.value1;//PF pu_reference_index 
    //fprintf(p_lcu_mv,"MV  block%d MV:%d %d ref=%d Bi-Bck\n",block8Nx8N,bw_mv_array[j8][i8][0],bw_mv_array[j8][i8][1],bw_refframe);//
    puInfoInter[block8Nx8N][11]=bw_refframe;

    puInfoInter[block8Nx8N][14]=bw_mv_array[j8][i8][0];
    puInfoInter[block8Nx8N][15]=bw_mv_array[j8][i8][1];
#endif

    //CHECKMOTIONRANGE

    // !! symirection prediction shenyanfei

    get_block ( fw_refframe, vec1_x, vec1_y, step_h, step_v, tmp_block, integerRefY_fref[fw_refframe] 
#ifdef AVS2_S2_SBD
		, ioff, joff
#endif	
	);
    get_block ( bw_refframe, vec2_x, vec2_y, step_h, step_v, tmp_blockbw, integerRefY_bref[bw_refframe] 
#ifdef AVS2_S2_SBD
		, ioff, joff
#endif	
	);
    for ( ii = 0; ii < step_h; ii++ )
    {
      for ( jj = 0; jj < step_v; jj++ )
      {
        img->predBlock[jj + joff][ii + ioff] = ( tmp_block[jj][ii] + tmp_blockbw[jj][ii] + 1 ) / 2;
#if EXTRACT_PIC_YUV_PRED   //2016-5-23 提取整帧图像的经过预测后的数据
        pPicYPred[(img->pix_y + joff + jj)*img->width + img->pix_x + ioff + ii] = tmp_block[jj][ii];
#endif
      }
    }

#if EXTRACT_lcu_info_debug_yuv
    if (1)
    {
      int x, y;
      fprintf(p_lcu_debug, "Inter Y CU posx,y=%d %d  blk x,y=%d %d\n", img->pix_x + ioff, img->pix_y + joff, step_v, step_h);

      for (ii = 0; ii < step_h; ii++)
      {
        for (jj = 0; jj < step_v; jj++)
        {
          img->predBlock[jj + joff][ii + ioff] = tmp_block[jj][ii];
          //img->predBlock[y + y_off][x + x_off] = piPredBuf[y][x];
          fprintf(p_lcu_debug, "%4d", img->predBlock[jj + joff][ii + ioff]);
        }
        fprintf(p_lcu_debug, "\n");
      }
    }
#endif
  }
}
void decode_one_IntraChromaBlock ( int uv )
{
  codingUnit *currMB   = &img->mb_data[img->current_mb_nr];//GB current_mb_nr];
  //short * edgepixels = ( short * ) malloc ( ( ( 1 << currMB->ui_MbBitSize ) * 4 + 1 ) * sizeof ( short ) );
  short edgepixels[4*MAX_CU_SIZE+1];
#define EP ( edgepixels + ( ( 1 << currMB->ui_MbBitSize ) * 2 ) )
  int x, y;
  int bs_x;
  int bs_y;
  int i = 0, j = 0;
  int **piPredBuf;
#if M3198_CU8
  int img_x= ( img->mb_x << MIN_CU_SIZE_IN_BIT );
  int img_y= ( img->mb_y << MIN_CU_SIZE_IN_BIT );
#else
  int img_x= ( img->mb_x << 4 );
  int img_y= ( img->mb_y << 4 );
#endif
  int predLmode = img->ipredmode[img_y / MIN_BLOCK_SIZE + 1][img_x / MIN_BLOCK_SIZE + 1];
  int mb_available_left_down;
  int p_avai[5];
  int mb_nr             = img->current_mb_nr;//GBimg->current_mb_nr;
  int mb_available_up;
  int mb_available_left;
  int mb_available_up_left;
  int mb_available_up_right;
  int mb_BitSize = img->mb_data[mb_nr].ui_MbBitSize;
  int N8_SizeScale;
  int mb_x, mb_y;
  get_mb_pos ( img->current_mb_nr, &mb_x, &mb_y, input->g_uiMaxSizeInBit );
  N8_SizeScale = ( 1 << mb_BitSize ) >> MIN_CU_SIZE_IN_BIT;

  bs_x = MIN_BLOCK_SIZE * N8_SizeScale;
  bs_y = MIN_BLOCK_SIZE * N8_SizeScale;

  getIntraNeighborAvailabilities(currMB, input->g_uiMaxSizeInBit, img_x, img_y, (1 << mb_BitSize), (1 << mb_BitSize), p_avai);

  mb_available_left_down = p_avai[NEIGHBOR_INTRA_LEFT_DOWN];
  mb_available_left      = p_avai[NEIGHBOR_INTRA_LEFT];
  mb_available_up_left   = p_avai[NEIGHBOR_INTRA_UP_LEFT];
  mb_available_up        = p_avai[NEIGHBOR_INTRA_UP];
  mb_available_up_right  = p_avai[NEIGHBOR_INTRA_UP_RIGHT];


  for ( i = -2 * bs_y; i <= 2 * bs_x; i++ )
  {
#if EXTEND_BD
    EP[i] = 1 << (input->sample_bit_depth - 1);
#else
    EP[i] = 128;
#endif
  }

  if ( mb_available_up )
  {
    for ( x = 0; x < bs_x; x++ )
    {
      EP[x + 1] = imgUV[uv][img->pix_c_y - 1][img->pix_c_x + x];
    }

#if !BUGFIX_INTRA_REF_SAMPLES
    if ( mb_available_up_right )
    {
      for ( x = 0; x < bs_x; x++ )
      {
        if ( img->pix_c_x + bs_x + x >= img->width_cr )
        {
          EP[1 + x + bs_x] = imgUV[uv][img->pix_c_y - 1][img->width_cr - 1];
        }
        else
        {
          EP[1 + x + bs_x] = imgUV[uv][img->pix_c_y - 1][img->pix_c_x + bs_x + x];
        }
      }
    }
    else
    {
      for ( x = 0; x < bs_x; x++ )
      {
        EP[1 + x + bs_x] = EP[bs_x]; 
      }
    }

    if ( mb_available_up_left )
    {
      EP[0] = imgUV[uv][img->pix_c_y - 1][img->pix_c_x-1];
    }
    else
    {
      EP[0] = imgUV[uv][img->pix_c_y - 1][img->pix_c_x];
    }
#endif
  }

#if BUGFIX_AVAILABILITY_INTRA
  if (mb_available_up_right) 
  {
      for (x = 0; x < bs_x; x++) 
      {
          if (img->pix_c_x + bs_x + x >= img->width_cr) 
          {
              EP[1 + x + bs_x] = imgUV[uv][img->pix_c_y - 1][img->width_cr - 1];
          } 
          else 
          {
              EP[1 + x + bs_x] = imgUV[uv][img->pix_c_y - 1][img->pix_c_x + bs_x + x];
          }
      }
  }
  else 
  {
      for (x = 0; x < bs_x; x++) 
      {
          EP[1 + x + bs_x] = EP[bs_x];
      }
  }
#endif

  if ( mb_available_left )
  {
    for ( y = 0; y < bs_y; y++ )
    {
      EP[-1 - y] = imgUV[uv][img->pix_c_y + y][img->pix_c_x - 1];
    }

#if !BUGFIX_INTRA_REF_SAMPLES
    if ( mb_available_left_down )
    {
      for ( y = 0; y < bs_y; y++ )
      {
        if ( img->pix_c_y + bs_y + y >= img->height_cr )
        {
          EP[-1 - y - bs_y] = imgUV[uv][img->height_cr - 1][img->pix_c_x - 1];
        }
        else
        {
          EP[-1 - y - bs_y]  = imgUV[uv][img->pix_c_y + bs_y + y][img->pix_c_x - 1];
        }
      }
    }
    else
    {
      for ( y = 0; y < bs_y; y++ )
      {
        EP[-1 - y - bs_y] = EP[-bs_y];
      }
    }

    if ( !mb_available_up )
    {
      EP[0] = imgUV[uv][img->pix_c_y][img->pix_c_x - 1];
    }
#endif
  }

#if BUGFIX_INTRA_REF_SAMPLES
  if (mb_available_left_down) 
  {
      for (y = 0; y < bs_y; y++) 
      {
          if (img->pix_c_y + bs_y + y >= img->height_cr) 
          {
              EP[-1 - y - bs_y] = imgUV[uv][img->height_cr - 1][img->pix_c_x - 1];
          }
          else 
          {
              EP[-1 - y - bs_y] = imgUV[uv][img->pix_c_y + bs_y + y][img->pix_c_x - 1];
          }
      }
  }
  else 
  {
      for (y = 0; y < bs_y; y++) 
      {
          EP[-1 - y - bs_y] = EP[-bs_y];
      }
  }
#endif

#if !BUGFIX_INTRA_REF_SAMPLES
  if ( !input->slice_set_enable )
  {
    if ( mb_available_up && mb_available_left )
    {
      EP[0] = imgUV[uv][img->pix_c_y - 1][img->pix_c_x - 1];
    }
  }
  else 
#endif
  {
    if ( mb_available_up_left )
    {
      EP[0] = imgUV[uv][img->pix_c_y - 1][img->pix_c_x - 1];
    }
    else if ( mb_available_up )
    {
      EP[0] = imgUV[uv][img->pix_c_y - 1][img->pix_c_x];
    }
    else if ( mb_available_left )
    {
      EP[0] = imgUV[uv][img->pix_c_y][img->pix_c_x - 1];
    }
  }

#if EXTRACT_lcu_info_debug_IntraPred_UV
  if (1)
  {
    fprintf(p_lcu_debug, "IntraY-XX0 left\n ");
    for (y = 0; y >= (-2 * bs_y); y--)
    {
      fprintf(p_lcu_debug, "%4d,", EP[y]);
    }
    fprintf(p_lcu_debug, "\n ");
    fprintf(p_lcu_debug, "up\n ");
    for (y = 0; y <= (2 * bs_x); y++)
    {
      fprintf(p_lcu_debug, "%4d,", EP[y]);
    }
  }
#endif
  get_mem2Dint ( &piPredBuf, bs_y, bs_x );

  for ( y = 0; y < bs_y; y++ )
  {
    for ( x = 0; x < bs_x; x++ )
    {
      piPredBuf[y][x] = 0;
    }
  }
#if EXTEND_BD
	predIntraChromaAdi( EP, piPredBuf, currMB->c_ipred_mode, mb_BitSize-1, mb_available_up, mb_available_left, predLmode, input->sample_bit_depth );
#else
	predIntraChromaAdi( EP, piPredBuf, currMB->c_ipred_mode, mb_BitSize-1, mb_available_up, mb_available_left, predLmode );
#endif
  for ( y = 0; y < bs_y; y++ )
  {
    for ( x = 0; x < bs_x; x++ )
    {
      img->predBlock[y][x] = piPredBuf[y][x];
#if EXTRACT_PIC_YUV_PRED   //2016-5-23 提取整帧图像的经过预测后的数据
      pPicUVPred[uv][(img->pix_c_y+y)*img->width_cr + img->pix_c_x + x] = img->predBlock[y][x];
#endif
    }
  }
#if EXTRACT_lcu_info_debug_IntraPred_UV
  if (1)
  {
    fprintf(p_lcu_debug, "Intra UV=%d CU posx,y=%d %d  blk x,y=%d %d\n", uv, img_x, img_y, bs_x, bs_y);
    for (y = 0; y < bs_y; y++)
    {
      for (x = 0; x < bs_x; x++)
      {
        //img->predBlock[y + y_off][x + x_off] = piPredBuf[y][x];
        fprintf(p_lcu_debug, "%4d", piPredBuf[y][x]);
      }
      fprintf(p_lcu_debug, "\n");
    }
  }
#endif
  free_mem2Dint( piPredBuf);
  //free(edgepixels);
}

#define  IClip( Min, Max, Val) (((Val)<(Min))? (Min):(((Val)>(Max))? (Max):(Val)))

int get_chroma_subpix_blk(int ** dst, int xoffs, int yoffs, int width, int height, int curx, int cury, int posx, int posy, byte** refpic)
{
    int i, j;
    int x0, x1, x2, x3;
    int y0, y1, y2, y3;
    int val;

    int shift1 = input->sample_bit_depth - 8;
    int add1 = (1 << (shift1)) >> 1;
    int shift2 = 20 - input->sample_bit_depth;
    int add2 = 1 << (19 - input->sample_bit_depth);

#if EXTEND_BD
    int max_pel_value = (1 << input->sample_bit_depth) - 1;
#else
    int max_pel_value = 255;
#endif
    static const int COEF[8][4] = {
        { 0, 64, 0, 0 },
        { -4, 62, 6, 0 },
        { -6, 56, 15, -1 },
        { -5, 47, 25, -3 },
        { -4, 36, 36, -4 },
        { -3, 25, 47, -5 },
        { -1, 15, 56, -6 },
        { 0, 6, 62, -4 }
    };

    
    if (posy == 0) {
        for (i = 0; i < height; i++) {
            for (j = 0; j < width; j++) {
                int base_x = curx - 1 + j;
                x0 = max(0, min(img->width_cr - 1, base_x + 0));
                x1 = max(0, min(img->width_cr - 1, base_x + 1));
                x2 = max(0, min(img->width_cr - 1, base_x + 2));
                x3 = max(0, min(img->width_cr - 1, base_x + 3));
                y0 = y1 = y2 = y3 = max(0, min(img->height_cr - 1, cury + i));
                val = refpic[y0][x0] * COEF[posx][0] + refpic[y1][x1] * COEF[posx][1] + refpic[y2][x2] * COEF[posx][2] + refpic[y3][x3] * COEF[posx][3];
                dst[yoffs + i][xoffs + j] = IClip(0, max_pel_value, (val + 32) >> 6);
            }
        }
    } else if (posx == 0) {
        for (i = 0; i < height; i++) {
            for (j = 0; j < width; j++) {
                int base_y = cury - 1 + i;
                y0 = max(0, min(img->height_cr - 1, base_y + 0));
                y1 = max(0, min(img->height_cr - 1, base_y + 1));
                y2 = max(0, min(img->height_cr - 1, base_y + 2));
                y3 = max(0, min(img->height_cr - 1, base_y + 3));
                x0 = x1 = x2 = x3 = max(0, min(img->width_cr - 1, curx + j));
                val = refpic[y0][x0] * COEF[posy][0] + refpic[y1][x1] * COEF[posy][1] + refpic[y2][x2] * COEF[posy][2] + refpic[y3][x3] * COEF[posy][3];
                dst[yoffs + i][xoffs + j] = IClip(0, max_pel_value, (val + 32) >> 6);
            }
        }
    } else {
        int tmpbuf[36][32];

        for (i = -1; i < height + 3; i++) {
            for (j = 0; j < width; j++) {
                int base_x = curx - 1 + j;
                x0 = max(0, min(img->width_cr - 1, base_x + 0));
                x1 = max(0, min(img->width_cr - 1, base_x + 1));
                x2 = max(0, min(img->width_cr - 1, base_x + 2));
                x3 = max(0, min(img->width_cr - 1, base_x + 3));
                y0 = y1 = y2 = y3 = max(0, min(img->height_cr - 1, cury + i));
                tmpbuf[i + 1][j] = refpic[y0][x0] * COEF[posx][0] + refpic[y1][x1] * COEF[posx][1] + refpic[y2][x2] * COEF[posx][2] + refpic[y3][x3] * COEF[posx][3];
                tmpbuf[i + 1][j] = (tmpbuf[i + 1][j] + add1) >> shift1;
            }
        }

        for (i = 0; i < height; i++) {
            for (j = 0; j < width; j++) {
                val = tmpbuf[i    ][j] * COEF[posy][0] + tmpbuf[i + 1][j] * COEF[posy][1] + 
                      tmpbuf[i + 2][j] * COEF[posy][2] + tmpbuf[i + 3][j] * COEF[posy][3];

                dst[yoffs + i][xoffs + j] = IClip(0, max_pel_value, (val + add2) >> shift2);
            }
        }
    }

    return val;
}


void decode_one_InterChromaBlock(int uv)
{
    codingUnit *currMB = &img->mb_data[img->current_mb_nr];//GB current_mb_nr];
    int i = 0, j = 0, ii = 0, jj = 0, i1 = 0, j1 = 0;
    int jf = 0;

    int ioff, joff, i8, j8;

    int bw_pred, fw_pred, ifx;
    int f1, f2, f3, f4;
    int if1;
    int refframe, fw_refframe, bw_refframe, mv_mode, pred_dir; // = currMB->ref_frame;
    int bw_ref_idx;
    int*** mv_array, ***fw_mv_array, ***bw_mv_array;
    int bframe = (img->type == B_IMG);

    int mb_nr = img->current_mb_nr;//GBimg->current_mb_nr;

    int mb_BitSize = img->mb_data[mb_nr].ui_MbBitSize;
    int N8_SizeScale;
    int start_x, start_y, step_h, step_v;
    int start_b8x, start_b8y, step_b8h, step_b8v;
    int k;
    int mpr_tmp[2];
    int dmh_mode = 0;
    int center_x, center_y;
    int delta = 0;
    int delta2 = 0;
    byte **p_ref;
    byte **p_ref1;
    byte **p_ref2;

    int center_x1, center_y1;
    int psndrefframe;

#if ChromaWSM
    int distanceDelta[4];
    distanceDelta[0] = ( 2 * ( picture_distance - img->imgtr_fwRefDistance[0] ) + 512 ) % 512;
    distanceDelta[1] = ( 2 * ( picture_distance - img->imgtr_fwRefDistance[1] ) + 512 ) % 512;
    distanceDelta[2] = ( 2 * ( picture_distance - img->imgtr_fwRefDistance[2] ) + 512 ) % 512;
    distanceDelta[3] = ( 2 * ( picture_distance - img->imgtr_fwRefDistance[3] ) + 512 ) % 512;
#if AVS2_SCENE_MV
	if (0 == curr_RPS.num_of_ref - 1 && background_reference_enable){
		distanceDelta[0] = 1;
	}
	if (1 == curr_RPS.num_of_ref - 1 && background_reference_enable) {
		distanceDelta[1] = 1;
	}
	if (2 == curr_RPS.num_of_ref - 1 && background_reference_enable) {
		distanceDelta[2] = 1;
	}
	if (3 == curr_RPS.num_of_ref - 1 && background_reference_enable) {
		distanceDelta[3] = 1;
	}
#endif
#endif

    N8_SizeScale = (1 << mb_BitSize) >> MIN_CU_SIZE_IN_BIT;

    ifx = if1 = 0;

    f1 = 8;
    f2 = 7;

    f3 = f1 * f1;
    f4 = f3 / 2;

    for (j = 4; j < 6; j++)
    {
        joff = (j - 4) * MIN_BLOCK_SIZE / 2 * N8_SizeScale;

        for (i = 0; i < 2; i++)
        {
            ioff = i * MIN_BLOCK_SIZE / 2 * N8_SizeScale;
            get_pix_offset(currMB->cuType, mb_BitSize, i, j - 4, &start_x, &start_y, &step_h, &step_v);
            ioff = start_x = start_x >> 1;
            joff = start_y = start_y >> 1;
            step_h = step_h >> 1;
            step_v = step_v >> 1;
            i8 = img->pix_c_x + start_x;
            j8 = img->pix_c_y + start_y;

            mv_mode = currMB->b8mode[2 * (j - 4) + i];
            pred_dir = currMB->b8pdir[2 * (j - 4) + i];
            if (pred_dir != SYM && pred_dir != BID)
            {
                //--- FORWARD/BACKWARD PREDICTION ---
                if (!bframe) {
                    mv_array = img->tmp_mv;
                } else if (!pred_dir) {
                    mv_array = img->fw_mv;
                } else {
                    mv_array = img->bw_mv;
                }

                get_b8_offset(currMB->cuType, mb_BitSize, i, j - 4, &start_b8x, &start_b8y, &step_b8h, &step_b8v);

                jf = img->block8_y + start_b8y;
                if1 = img->block8_x + start_b8x;

                if (!bframe) {
                    refframe = refFrArr[jf][if1];
                } else if (!pred_dir) {
                    refframe = img->fw_refFrArr[jf][if1];// fwd_ref_idx_to_refframe(img->fw_refFrArr[jf][if1]);
                } else {
                    refframe = img->bw_refFrArr[jf][if1];// bwd_ref_idx_to_refframe(img->bw_refFrArr[jf][if1]);
                    bw_ref_idx = img->bw_refFrArr[jf][if1];
                }

#ifdef AVS2_S2_S
                if (img->type == F_IMG && b_dmh_enabled && img->typeb != BP_IMG)
#else
                if ((img->type == P_IMG || img->type == F_IMG) && b_dmh_enabled && img->typeb != BP_IMG)
#endif

                {
                    dmh_mode = mv_array[jf][if1][3];
                } else {
                    dmh_mode = 0;
                }

#if HALF_PIXEL_CHROMA
                if (img->is_field_sequence && input->chroma_format == 1) {
                    int fw_bw = bframe && pred_dir ? -1 : 0;
                    int distance = calculate_distance(refframe, fw_bw);
                    delta = (distance % 4) == 0 ? 0 : img->is_top_field ? 2 : -2;
                }
#endif
                psndrefframe = p_snd_refFrArr[jf][if1];

                if (img->type == F_IMG && pred_dir == DUAL)
                {
#if HALF_PIXEL_CHROMA
                    if (img->is_field_sequence && input->chroma_format == 1)
                    {
                        int fw_bw = 0; //fw
                        int distance = calculate_distance(psndrefframe, fw_bw);
                        delta2 = (distance % 4) == 0 ? 0 : img->is_top_field ? 2 : -2;
                    }
#endif
                }

                if (img->type == F_IMG && (currMB->md_directskip_mode == BID_P_FST || currMB->md_directskip_mode == BID_P_SND))
                {
#if HALF_PIXEL_CHROMA
                    if (img->is_field_sequence && input->chroma_format == 1)
                    {
                        int fw_bw = 0; //fw
                        int distance = calculate_distance(psndrefframe, fw_bw);
                        delta2 = (distance % 4) == 0 ? 0 : img->is_top_field ? 2 : -2;
                    }
#endif
                }

                center_y = (img->pix_c_y + joff) * f1 + mv_array[jf][if1][1] - delta;
                center_x = (img->pix_c_x + ioff) * f1 + mv_array[jf][if1][0];

                center_y1 = (img->pix_c_y + joff) * f1 + img->p_snd_tmp_mv[jf][if1][1] - delta2;
                center_x1 = (img->pix_c_x + ioff) * f1 + img->p_snd_tmp_mv[jf][if1][0];

#if ChromaWSM
                if ( img->num_of_references > 1 && ( !mv_mode ) &&  img->type == F_IMG && currMB->weighted_skipmode )
                {
#if HALF_PIXEL_CHROMA
                  if (img->is_field_sequence && input->chroma_format == 1)
                  {
                    int fw_bw = 0; //fw
                    int distance = calculate_distance(currMB->weighted_skipmode, fw_bw);
                    delta2 = (distance % 4) == 0 ? 0 : img->is_top_field ? 2 : -2;

                    distance = calculate_distance(refframe, fw_bw);
                    delta = (distance % 4) == 0 ? 0 : img->is_top_field ? 2 : -2;
                  }
#endif
#if ChromaWSMFix
                  //center_y1 = (img->pix_c_y + joff) * f1 + img->p_snd_tmp_mv[jf][if1][1] - delta2;
                  center_y1 = (img->pix_c_y + joff) * f1 + ( int ) ( distanceDelta[currMB->weighted_skipmode ] * (mv_array[jf][if1][1]+delta) * (MULTI / distanceDelta[0]) + HALF_MULTI >> OFFSET ) - delta2;
                  center_y1 -= delta2;
                  center_x1 = (img->pix_c_x + ioff) * f1 + img->p_snd_tmp_mv[jf][if1][0];
#else
                  center_y1 = (img->pix_c_y + joff) * f1 + ( int ) ( distanceDelta[currMB->weighted_skipmode ] * (mv_array[jf][if1][1]) * (MULTI / distanceDelta[0]) + HALF_MULTI >> OFFSET ) - delta2;
                  center_x1 = (img->pix_c_x + ioff) * f1 + ( int ) ( distanceDelta[currMB->weighted_skipmode ] * mv_array[jf][if1][0] * (MULTI / distanceDelta[0]) + HALF_MULTI >> OFFSET );
#endif
                }
#endif

                if (!bframe)
                {
#ifdef AVS2_S2_BGLONGTERM
#if AVS2_SCENE_CD
                  if (background_reference_enable && img->num_of_references >= 2 && refframe == img->num_of_references - 1 && (img->type == P_IMG || img->type == F_IMG) && img->typeb != BP_IMG)
#else
                  if (input->profile_id == 0x50 && background_reference_enable && img->num_of_references >= 2 && refframe == img->num_of_references - 1 && img->type == P_IMG && img->typeb != BP_IMG)
#endif
                  {
                    p_ref = background_frame[uv + 1];
                  }
#ifdef AVS2_S2_S
#if AVS2_SCENE_CD
                  else if (img->typeb == BP_IMG)
#else
                  else if (input->profile_id == 0x50 && img->typeb == BP_IMG)
#endif
                  {
                    p_ref = background_frame[uv + 1];
                  }
#endif
                  else 
                  {
                    p_ref = integerRefUV[refframe][uv];
                  }
#endif

                } 
                else 
                {
                  refframe = 0;

                  if (!pred_dir) {
                    p_ref = integerRefUV_fref[refframe][uv];
                  } else {
                    p_ref = integerRefUV_bref[refframe][uv];
                  }
                }

#ifdef AVS2_S2_BGLONGTERM
#if AVS2_SCENE_CD
                if (background_reference_enable && img->num_of_references >= 2 && psndrefframe == img->num_of_references - 1 && (img->type == P_IMG || img->type == F_IMG)/* && img->typeb != BP_IMG*/)
#else
                if (input->profile_id == 0x50 && background_reference_enable && img->num_of_references >= 2 && psndrefframe == img->num_of_references - 1 && img->type == P_IMG/* && img->typeb != BP_IMG*/)
#endif
                {
                    p_ref1 = background_frame[uv + 1];
                }
#ifdef AVS2_S2_S
#if AVS2_SCENE_CD
                else if (img->typeb == BP_IMG)
#else
                else if (input->profile_id == 0x50 && img->typeb == BP_IMG)
#endif
                {
                    p_ref1 = background_frame[uv + 1];
                }
#endif
#if ChromaWSM
                else if ( img->num_of_references > 1 && ( !mv_mode ) &&  img->type == F_IMG && currMB->weighted_skipmode )
                {
                  p_ref1 = integerRefUV[currMB->weighted_skipmode][uv];
                }
#endif
                else
                {
                  p_ref1 = integerRefUV[psndrefframe][uv];
                }
#endif

#ifdef AVS2_S2_BGLONGTERM
#if AVS2_SCENE_CD
                if (background_reference_enable && img->num_of_references >= 2 && psndrefframe == img->num_of_references - 1 && (img->type == P_IMG || img->type == F_IMG)/* && img->typeb != BP_IMG*/)
#else
                if (input->profile_id == 0x50 && background_reference_enable && img->num_of_references >= 2 && psndrefframe == img->num_of_references - 1 && img->type == P_IMG/* && img->typeb != BP_IMG*/)
#endif
                {
                    p_ref2 = background_frame[uv + 1];
                }
#ifdef AVS2_S2_S
#if AVS2_SCENE_CD
                else if (img->typeb == BP_IMG)
#else
                else if (input->profile_id == 0x50 && img->typeb == BP_IMG)
#endif
                {
                    p_ref2 = background_frame[uv + 1];
                }
#endif
#if ChromaWSM
                else if ( img->num_of_references > 1 && ( !mv_mode ) &&  img->type == F_IMG && currMB->weighted_skipmode )
                {
                  p_ref1 = integerRefUV[currMB->weighted_skipmode][uv];
                }
#endif
                else
                {
                  p_ref2 = integerRefUV[psndrefframe][uv];
                }
#endif

                if (dmh_mode) {
                    i1 = center_x + dmh_pos[dmh_mode][0][0];
                    j1 = center_y + dmh_pos[dmh_mode][0][1];
                    get_chroma_subpix_blk(img->predBlock, ioff, joff, step_h, step_v, i1 >> 3, j1 >> 3, i1 & 7, j1 & 7, p_ref);

                    i1 = center_x + dmh_pos[dmh_mode][1][0];
                    j1 = center_y + dmh_pos[dmh_mode][1][1];
                    get_chroma_subpix_blk(img->predBlockTmp, ioff, joff, step_h, step_v, i1 >> 3, j1 >> 3, i1 & 7, j1 & 7, p_ref);

                    for (jj = 0; jj < step_v; jj++) {
                        for (ii = 0; ii < step_h; ii++) {
                            img->predBlock[jj + joff][ii + ioff] = 
                                (img->predBlock[jj + joff][ii + ioff] + img->predBlockTmp[jj + joff][ii + ioff] + 1) >> 1;
                        }
                    }

#if EXTRACT_lcu_info_debug_yuv
                    if (1)
                    {
                      fprintf(p_lcu_debug, "Inter UV=%d CU posx,y=%d %d  blk x,y=%d %d dmh_mode=%d\n",uv, img->pix_x+ioff, img->pix_y+joff, step_v, step_h, dmh_mode);
                      for (jj = 0; jj < step_v; jj++) {
                        for (ii = 0; ii < step_h; ii++) {
                          //img->predBlock[y + y_off][x + x_off] = piPredBuf[y][x];
                          fprintf(p_lcu_debug, "%4d", img->predBlock[jj + joff][ii + ioff]);
                        }
                        fprintf(p_lcu_debug, "\n");
                      }
                    }
#endif
                } else {
                    i1 = center_x;
                    j1 = center_y;
                    get_chroma_subpix_blk(img->predBlock, ioff, joff, step_h, step_v, i1 >> 3, j1 >> 3, i1 & 7, j1 & 7, p_ref);

#if EXTRACT_lcu_info_debug_yuv
                    if (1)
                    {
                      fprintf(p_lcu_debug, "Inter UV=%d CU posx,y=%d %d  blk x,y=%d %d dmh_mode=%d\n", uv, img->pix_x + ioff, img->pix_y + joff, step_v, step_h, dmh_mode);
                      for (jj = 0; jj < step_v; jj++) {
                        for (ii = 0; ii < step_h; ii++) {
                          //img->predBlock[y + y_off][x + x_off] = piPredBuf[y][x];
                          fprintf(p_lcu_debug, "%4d", img->predBlock[jj + joff][ii + ioff]);
                        }
                        fprintf(p_lcu_debug, "\n");
                      }
                    }
#endif
#if ChromaWSM
                    if ( img->num_of_references > 1 && ( !mv_mode ) &&  img->type == F_IMG && currMB->weighted_skipmode )
                    {
                      i1 = center_x1;
                      j1 = center_y1;
                      get_chroma_subpix_blk(img->predBlockTmp, ioff, joff, step_h, step_v, i1 >> 3, j1 >> 3, i1 & 7, j1 & 7, p_ref1);
                      for (jj = 0; jj < step_v; jj++) {
                        for (ii = 0; ii < step_h; ii++) {
                          img->predBlock[jj + joff][ii + ioff] =
                            (img->predBlock[jj + joff][ii + ioff] + img->predBlockTmp[jj + joff][ii + ioff] + 1) >> 1;
                        }
                      }
#if EXTRACT_lcu_info_debug_yuv
                      if (1)
                      {
                        fprintf(p_lcu_debug, "Inter UV=%d CU posx,y=%d %d  blk x,y=%d %d ChromaWSM\n", uv, img->pix_x + ioff, img->pix_y + joff, step_v, step_h);
                        for (jj = 0; jj < step_v; jj++) {
                          for (ii = 0; ii < step_h; ii++) {
                            //img->predBlock[y + y_off][x + x_off] = piPredBuf[y][x];
                            fprintf(p_lcu_debug, "%4d", img->predBlock[jj + joff][ii + ioff]);
                          }
                          fprintf(p_lcu_debug, "\n");
                        }
                      }
#endif
                    }
#endif
                    if (img->type == F_IMG && pred_dir == DUAL)
                    {
                        center_x1 = (img->pix_c_x + ioff) * f1 + img->p_snd_tmp_mv[jf][if1][0];
                        i1 = center_x1;
                        j1 = center_y1;
                        get_chroma_subpix_blk(img->predBlockTmp, ioff, joff, step_h, step_v, i1 >> 3, j1 >> 3, i1 & 7, j1 & 7, p_ref1);
                        for (jj = 0; jj < step_v; jj++) {
                            for (ii = 0; ii < step_h; ii++) {
                                img->predBlock[jj + joff][ii + ioff] =
                                    (img->predBlock[jj + joff][ii + ioff] + img->predBlockTmp[jj + joff][ii + ioff] + 1) >> 1;
                            }
                        }
#if EXTRACT_lcu_info_debug_yuv
                        if (1)
                        {
                          fprintf(p_lcu_debug, "Inter UV=%d CU posx,y=%d %d  blk x,y=%d %d F-DUAL\n", uv, img->pix_x + ioff, img->pix_y + joff, step_v, step_h);
                          for (jj = 0; jj < step_v; jj++) {
                            for (ii = 0; ii < step_h; ii++) {
                              //img->predBlock[y + y_off][x + x_off] = piPredBuf[y][x];
                              fprintf(p_lcu_debug, "%4d", img->predBlock[jj + joff][ii + ioff]);
                            }
                            fprintf(p_lcu_debug, "\n");
                          }
                        }
#endif
                    }


                    if (img->type == F_IMG && (currMB->md_directskip_mode == BID_P_FST || currMB->md_directskip_mode == BID_P_SND))
                    {
                        center_x1 = (img->pix_c_x + ioff) * f1 + img->p_snd_tmp_mv[jf][if1][0];
                        i1 = center_x1;
                        j1 = center_y1;

                        get_chroma_subpix_blk(img->predBlockTmp, ioff, joff, step_h, step_v, i1 >> 3, j1 >> 3, i1 & 7, j1 & 7, p_ref2);
                        for (jj = 0; jj < step_v; jj++) {
                            for (ii = 0; ii < step_h; ii++) {
                                img->predBlock[jj + joff][ii + ioff] =
                                    (img->predBlock[jj + joff][ii + ioff] + img->predBlockTmp[jj + joff][ii + ioff] + 1) >> 1;
                            }
                        }

#if EXTRACT_lcu_info_debug_yuv
                        if (1)
                        {
                          fprintf(p_lcu_debug, "Inter UV=%d CU posx,y=%d %d  blk x,y=%d %d F-BID_P_FST or BID_P_SND\n", uv, img->pix_x + ioff, img->pix_y + joff, step_v, step_h);
                          for (jj = 0; jj < step_v; jj++) {
                            for (ii = 0; ii < step_h; ii++) {
                              //img->predBlock[y + y_off][x + x_off] = piPredBuf[y][x];
                              fprintf(p_lcu_debug, "%4d", img->predBlock[jj + joff][ii + ioff]);
                            }
                            fprintf(p_lcu_debug, "\n");
                          }
                        }
#endif
                    }
                }
#if EXTRACT_PIC_YUV_PRED   //2016-5-23 提取整帧图像的经过预测后的数据
                for (jj = 0; jj < step_v; jj++) {
                  for (ii = 0; ii < step_h; ii++) {
                    pPicUVPred[uv][(img->pix_c_y + joff + jj)*img->width_cr + img->pix_c_x + ioff + ii] = img->predBlock[jj + joff][ii + ioff];
                  }
                }                
#endif
            } //if (pred_dir != SYM && pred_dir != BID)
            else {
                if (mv_mode != 0) {
                    //===== BI-DIRECTIONAL PREDICTION =====
                    fw_mv_array = img->fw_mv;
                    bw_mv_array = img->bw_mv;
                } else {
                    //===== DIRECT PREDICTION =====
                    fw_mv_array = img->fw_mv;
                    bw_mv_array = img->bw_mv;
                }

                get_b8_offset(currMB->cuType, mb_BitSize, i, j - 4, &start_b8x, &start_b8y, &step_b8h, &step_b8v);
                jf = img->block8_y + start_b8y;
                ifx = img->block8_x + start_b8x;

                if (mv_mode != 0) {
                    fw_refframe = img->fw_refFrArr[jf][ifx];
                    bw_refframe = img->bw_refFrArr[jf][ifx];
                    bw_ref_idx = img->bw_refFrArr[jf][ifx];
                    bw_ref_idx = bw_ref_idx;
                } else {
                    fw_refframe = 0;
                    bw_refframe = 0;
                }

#if HALF_PIXEL_CHROMA
                if (img->is_field_sequence && input->chroma_format == 1) {
                    int fw_bw = 0;  //fw
                    int distance = calculate_distance(fw_refframe, fw_bw);
                    delta = (distance % 4) == 0 ? 0 : img->is_top_field ? 2 : -2;
                }
#endif
#if HALF_PIXEL_CHROMA
                if (img->is_field_sequence && input->chroma_format == 1) {
                    int fw_bw = -1;  //bw
                    int distance = calculate_distance(bw_refframe, fw_bw);
                    delta2 = (distance % 4) == 0 ? 0 : img->is_top_field ? 2 : -2;
                }
#endif
                i1 = (img->pix_c_x + ioff) * f1 + fw_mv_array[jf][ifx][0];
                j1 = (img->pix_c_y + joff) * f1 + fw_mv_array[jf][ifx][1] - delta;
                get_chroma_subpix_blk(img->predBlock, ioff, joff, step_h, step_v, i1 >> 3, j1 >> 3, i1 & 7, j1 & 7, integerRefUV_fref[fw_refframe][uv]);

                i1 = (img->pix_c_x + ioff) * f1 + bw_mv_array[jf][ifx][0];
                j1 = (img->pix_c_y + joff) * f1 + bw_mv_array[jf][ifx][1] - delta2;
                get_chroma_subpix_blk(img->predBlockTmp, ioff, joff, step_h, step_v, i1 >> 3, j1 >> 3, i1 & 7, j1 & 7, integerRefUV_bref[fw_refframe][uv]);

                for (jj = 0; jj < step_v; jj++) {
                    for (ii = 0; ii < step_h; ii++) {
                        img->predBlock[jj + joff][ii + ioff] = 
                            (img->predBlock[jj + joff][ii + ioff] + img->predBlockTmp[jj + joff][ii + ioff] + 1) / 2; 
                    }
                }
#if EXTRACT_lcu_info_debug_yuv
                if (1)
                {
                  fprintf(p_lcu_debug, "Inter UV=%d CU posx,y=%d %d  blk x,y=%d %d F-BID_P_FST or BID_P_SND\n", uv, img->pix_x + ioff, img->pix_y + joff, step_v, step_h);
                  for (jj = 0; jj < step_v; jj++) {
                    for (ii = 0; ii < step_h; ii++) {
                      //img->predBlock[y + y_off][x + x_off] = piPredBuf[y][x];
                      fprintf(p_lcu_debug, "%4d", img->predBlock[jj + joff][ii + ioff]);
                    }
                    fprintf(p_lcu_debug, "\n");
                  }
                }
#endif
#if EXTRACT_PIC_YUV_PRED   //2016-5-23 提取整帧图像的经过预测后的数据
                for (jj = 0; jj < step_v; jj++) {
                  for (ii = 0; ii < step_h; ii++) {
                    pPicUVPred[uv][(img->pix_c_y + joff + jj)*img->width_cr + img->pix_c_x + ioff + ii] = img->predBlock[jj + joff][ii + ioff];
                  }
                }                
#endif
            }
        }
    }
}

int decode_SubMB ()
{
  codingUnit *currMB   = &img->mb_data[img->current_mb_nr];//GB current_mb_nr];

  int uv;//, hv;
  int tmp;
  int **curr_blk;
  //  int curr_blk[MAX_CU_SIZE][MAX_CU_SIZE];  //SW for AVS   //liweiran
  int block8Nx8N;

  int mb_BitSize = img->mb_data[img->current_mb_nr].ui_MbBitSize;
  int N8_SizeScale = 1 << ( mb_BitSize - MIN_CU_SIZE_IN_BIT );

  get_mem2Dint ( &curr_blk, 1 << mb_BitSize, 1 << mb_BitSize );

  //block8Nx8N：0-3 表示亮度，block8Nx8N：4-5 表示色度
#if INTRA_2N
  if ( currMB->trans_size == 0 )
  {
    if ( currMB->cuType != I16MB )
    {
      for ( block8Nx8N = 0; block8Nx8N < 4; block8Nx8N++ )
      {
        if ( currMB->b8mode[block8Nx8N] != IBLOCK )
        {
          decode_one_InterLumaBlock( block8Nx8N );
        }
      }
      //cms 从LCU级的变换系数数组中取数据
      get_curr_blk ( 0, curr_blk, mb_BitSize );

      //cms 进行反变换
      idct_dequant_B8 ( 0, currMB->qp - MIN_QP, curr_blk, mb_BitSize );
    }
    else//cms TU=CU PU=CU  I16MB
    {
      get_curr_blk ( 0, curr_blk, mb_BitSize );
      tmp = intrapred ( ( img->mb_x << MIN_CU_SIZE_IN_BIT ), ( img->mb_y << MIN_CU_SIZE_IN_BIT ), mb_BitSize );

      if ( tmp == SEARCH_SYNC ) /* make 4x4 prediction block mpr from given prediction img->mb_mode */
      {
        return SEARCH_SYNC;  /* bit error */
      }

      idct_dequant_B8 ( 0, currMB->qp - MIN_QP, curr_blk, mb_BitSize );
    }
  }
  else
  {
    // luma decoding **************************************************
    for ( block8Nx8N = 0; block8Nx8N < 4; block8Nx8N++ )
    {
      if ( currMB->b8mode[block8Nx8N] != IBLOCK )
      {
        decode_one_InterLumaBlock( block8Nx8N );
      }
      else
      {
        get_curr_blk ( block8Nx8N, curr_blk, mb_BitSize - 1 );
        if(currMB->cuType==InNxNMB)
        {
          tmp = intrapred (  ( img->mb_x << MIN_CU_SIZE_IN_BIT ), ( img->mb_y << MIN_CU_SIZE_IN_BIT ) +  block8Nx8N *(1<<(mb_BitSize-2)), mb_BitSize  );
          if ( tmp == SEARCH_SYNC ) /* make 4x4 prediction block mpr from given prediction img->mb_mode */
          {
            return SEARCH_SYNC;  /* bit error */
          }
          idct_dequant_B8_NSQT( block8Nx8N, curr_blk, mb_BitSize - 1 );
        }
        else if(currMB->cuType==INxnNMB)
        {
          tmp = intrapred (  ( img->mb_x << MIN_CU_SIZE_IN_BIT ) + block8Nx8N *(1<<(mb_BitSize-2)), ( img->mb_y << MIN_CU_SIZE_IN_BIT ) , mb_BitSize );
          if ( tmp == SEARCH_SYNC ) /* make 4x4 prediction block mpr from given prediction img->mb_mode */
          {
            return SEARCH_SYNC;  /* bit error */
          }
          idct_dequant_B8_NSQT( block8Nx8N, curr_blk, mb_BitSize - 1 );
        }
        else
        {

#if M3198_CU8
          ///////////////////
          tmp = intrapred (  ( img->mb_x << MIN_CU_SIZE_IN_BIT ) + ( ( block8Nx8N & 1 ) << MIN_BLOCK_SIZE_IN_BIT ) * N8_SizeScale, ( img->mb_y << MIN_CU_SIZE_IN_BIT ) + ( ( block8Nx8N & 2 ) << (MIN_BLOCK_SIZE_IN_BIT - 1) ) * N8_SizeScale, mb_BitSize - 1 );
          if ( tmp == SEARCH_SYNC ) /* make 4x4 prediction block mpr from given prediction img->mb_mode */
          {
            return SEARCH_SYNC;  /* bit error */
          }

          idct_dequant_B8 ( block8Nx8N, currMB->qp - MIN_QP, curr_blk, mb_BitSize - 1 );
          ///////////////////
#else
          tmp = intrapred (  ( img->mb_x << MIN_CU_SIZE_IN_BIT ) + ( ( block8Nx8N & 1 ) << MIN_BLOCK_SIZE_IN_BIT ) * N8_SizeScale, ( img->mb_y << MIN_CU_SIZE_IN_BIT ) + ( ( block8Nx8N & 2 ) << 2 ) * N8_SizeScale, mb_BitSize - 1 );
          if ( tmp == SEARCH_SYNC ) /* make 4x4 prediction block mpr from given prediction img->mb_mode */
          {
            return SEARCH_SYNC;  /* bit error */
          }

          idct_dequant_B8 ( block8Nx8N, currMB->qp - MIN_QP, curr_blk, mb_BitSize - 1 );
#endif
        }

      }
    }
    for ( block8Nx8N = 0; block8Nx8N < 4; block8Nx8N++ )
    {
      if ( currMB->b8mode[block8Nx8N] != IBLOCK )
      {
        get_curr_blk ( block8Nx8N, curr_blk, mb_BitSize - 1 );
        if (input->useNSQT && currMB->ui_MbBitSize > B8X8_IN_BIT &&(currMB->cuType == P2NXN || currMB->cuType == PHOR_UP || currMB->cuType == PHOR_DOWN || currMB->cuType == PNX2N || currMB->cuType == PVER_LEFT || currMB->cuType == PVER_RIGHT))
        {
          idct_dequant_B8_NSQT( block8Nx8N, curr_blk, mb_BitSize - 1 );
        }
        else
        {
          idct_dequant_B8 ( block8Nx8N, currMB->qp - MIN_QP, curr_blk, mb_BitSize - 1 );
        }
      }
    }
  }
#else
  // luma decoding **************************************************
  for ( block8Nx8N = 0; block8Nx8N < 4; block8Nx8N++ )
  {

    if ( currMB->b8mode[block8Nx8N] != IBLOCK )
    {
      decode_one_InterLumaBlock( block8Nx8N );
      get_curr_blk ( block8Nx8N, curr_blk, mb_BitSize - 1 );
      idct_dequant_B8 ( block8Nx8N, currMB->qp - MIN_QP, curr_blk, mb_BitSize - 1 );
    }
    else
    {
      get_curr_blk ( block8Nx8N, curr_blk, mb_BitSize - 1 );
      tmp = intrapred ( ( img->mb_x << 4 ) + ( ( block8Nx8N & 1 ) << 3 ) * N8_SizeScale, ( img->mb_y << 4 ) + ( ( block8Nx8N & 2 ) << 2 ) * N8_SizeScale, mb_BitSize - 1 );

      if ( tmp == SEARCH_SYNC ) /* make 4x4 prediction block mpr from given prediction img->mb_mode */
      {
        return SEARCH_SYNC;  /* bit error */
      }

      idct_dequant_B8 ( block8Nx8N, currMB->qp - MIN_QP, curr_blk, mb_BitSize - 1 );
    }
  }
#endif


  if ( input->chroma_format == 1 )
  {
    for ( uv = 0; uv < 2; uv++ )
    {
      if ( IS_INTRA( currMB ) )
      {
        decode_one_IntraChromaBlock( uv );
      }
      else
      {
        decode_one_InterChromaBlock( uv );
      }
      get_curr_blk ( 4 + uv, curr_blk, mb_BitSize - 1 );
#if EXTEND_BD
      if (input->sample_bit_depth > 8)
      {
        idct_dequant_B8 ( 4 + uv, QP_SCALE_CR[Clip3(0, 63, (currMB->qp - (8 * (input->sample_bit_depth - 8)) - MIN_QP))] + (8 * (input->sample_bit_depth - 8)), curr_blk, mb_BitSize - 1 );				
      }
      else
      {
#endif
        idct_dequant_B8 ( 4 + uv, QP_SCALE_CR[currMB->qp - MIN_QP], curr_blk, mb_BitSize - 1 );
#if EXTEND_BD
      }
#endif

    }
  }

  free_mem2Dint ( curr_blk );
  return 0;
}







//!EDIT end <added by lzhang AEC

/*
*************************************************************************
* Function:decode the current supercodingUnit
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

