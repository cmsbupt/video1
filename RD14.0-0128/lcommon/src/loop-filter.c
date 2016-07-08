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
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "../../ldecod/inc/global.h"
#include "../../lcommon/inc/commonVariables.h"
#include "../../lcommon/inc/commonStructures.h"
#include "../../lcommon/inc/loop-filter.h"
#define  IClip( Min, Max, Val) (((Val)<(Min))? (Min):(((Val)>(Max))? (Max):(Val)))
#if SAO_SLICE_BOUND_PRO
#if SAO_CROSS_SLICE_FLAG
int sao_cross_slice;
#else
int sao_cross_slice = SAO_CROSS_SLICE;
#endif
int sao_para_cross_slice = SAO_PARA_CROSS_SLICE;
#endif


#define max(a, b)                   ((a) > (b) ? (a) : (b))  //!< Macro returning max value 
#define min(a, b)                   ((a) < (b) ? (a) : (b))  //!< Macro returning min value 

int saoclip[NUM_SAO_OFFSET][3] =  
{
	//EO
		{-1, 6, 7},//low bound, upper bound, threshold
		{0, 1, 1},
		{0, 0, 0},
		{-1, 0, 1},
		{-6, 1, 7},
		{-7, 7, 7} //BO
};

void off_sao(SAOBlkParam* saoblkparam)
{
	int i;
	for ( i = 0; i< NUM_SAO_COMPONENTS; i++)
	{
		saoblkparam[i].modeIdc = SAO_MODE_OFF;
		saoblkparam[i].typeIdc = -1;
		saoblkparam[i].startBand = -1;
    saoblkparam[i].startBand2 = -1;
		saoblkparam[i].deltaband = -1;
		memcpy(saoblkparam[i].offset, 0, sizeof(int)*MAX_NUM_SAO_CLASSES);
	}
}
void copySAOParam_for_blk(SAOBlkParam* saopara_dst,SAOBlkParam* saopara_src)
{
	int i, j;
	for ( i = 0; i < NUM_SAO_COMPONENTS; i++)
	{
		saopara_dst[i].modeIdc = saopara_src[i].modeIdc;
		saopara_dst[i].typeIdc = saopara_src[i].typeIdc;
		saopara_dst[i].startBand = saopara_src[i].startBand;
    saopara_dst[i].startBand2 = saopara_src[i].startBand2;
		saopara_dst[i].deltaband = saopara_src[i].deltaband;
		for ( j = 0; j < MAX_NUM_SAO_CLASSES; j++)
		{
			saopara_dst[i].offset[j] = saopara_src[i].offset[j];
		}
#if EXTRACT
	    //额外增加2个成员来保存 BO的起始区间序号
	    saopara_dst[i].startPos = saopara_src[i].startPos;
	    saopara_dst[i].startPosM2 = saopara_src[i].startPosM2;
	    //额外增加数组存放4个offset BO、EO共用
			for ( j = 0; j < 4; j++)
			{
				saopara_dst[i].sao_offset[j] = saopara_src[i].sao_offset[j];
			}
#endif		
	}
}
void copySAOParam_for_blk_onecomponent(SAOBlkParam* saopara_dst,SAOBlkParam* saopara_src)
{
	int  j;
	saopara_dst->modeIdc = saopara_src->modeIdc;
	saopara_dst->typeIdc = saopara_src->typeIdc;
	saopara_dst->startBand = saopara_src->startBand;
    saopara_dst->startBand2 = saopara_src->startBand2;
	saopara_dst->deltaband = saopara_src->deltaband;
	for ( j = 0; j < MAX_NUM_SAO_CLASSES; j++)
	{
		saopara_dst->offset[j] = saopara_src->offset[j];
	}
#if EXTRACT
    //额外增加2个成员来保存 BO的起始区间序号
    saopara_dst->startPos = saopara_src->startPos;
    saopara_dst->startPosM2 = saopara_src->startPosM2;
    //额外增加数组存放4个offset BO、EO共用
		for ( j = 0; j < 4; j++)
		{
			saopara_dst->sao_offset[j] = saopara_src->sao_offset[j];
		}
#endif	
}

void Copy_frame_for_file(FILE * p_pic_yuv,byte   **imgY, byte   ***imgUV)
{
  int i, j, k;
  char tmpChar;
  
  printf(" Copy_frame_for_file() in\n");
  //byte   **imgY;               //!< Encoded/decoded luma images
  //byte   ***imgUV;              //!< Encoded/decoded croma images
  for (j = 0; j < img->height; j++)
  {
    for (i = 0; i< img->width; i++)
    {
      tmpChar = imgY[j][i];
      fwrite(&tmpChar, 1, 1, p_pic_yuv);
    }
  }
  printf(" Copy_frame_for_file() Y end\n");
  printf("  img->height_cr = %d\n", img->height_cr);
  printf("  img->width_cr = %d\n", img->width_cr);
  for (k = 0; k < 2; k++)
  {
    for (j = 0; j < img->height_cr; j++)
    {
      for (i = 0; i< img->width_cr; i++)
      {
        tmpChar = imgUV[k][j][i];
        fwrite(&tmpChar, 1, 1, p_pic_yuv);
      }
    }
  }
  printf(" Copy_frame_for_file() end\n");

}

void Copy_frame_for_file_pred(FILE * p_pic_yuv, int   *imgY, int   *imgUV[])
{
  int i, j, k;
  byte tmpChar;

  printf(" Copy_frame_for_file_pred() in\n");
  //byte   *imgY;               //!< Encoded/decoded luma images
  //byte   *imgUV[];              //!< Encoded/decoded croma images
  for (j = 0; j < img->height; j++)
  {
    //printf("  img j = %d\n", j);
    for (i = 0; i< img->width; i++)
    {
      //printf("  img j*img->width+i = %d\n", j*img->width + i);
      //fprintf(p_pic_yuv, "%4d", imgY[(j*img->width) + i]);
      tmpChar = imgY[(j*img->width)+i];
      fwrite(&tmpChar, 1, 1, p_pic_yuv);
      //fprintf(p_pic_yuv,"%4u,", tmpChar);
    }
    //fprintf(p_pic_yuv, "\n");
  }
  printf(" Copy_frame_for_file_pred() Y end\n");
  printf("  img->height_cr = %d\n", img->height_cr);
  printf("  img->width_cr = %d\n", img->width_cr);
  for (k = 0; k < 2; k++)
  {
    for (j = 0; j < img->height_cr; j++)
    {
      for (i = 0; i< img->width_cr; i++)
      {
        tmpChar = imgUV[k][j*img->width_cr+i];
        fwrite(&tmpChar, 1, 1, p_pic_yuv);
        //fprintf(p_pic_yuv, "%4u", tmpChar);
      }
      //fprintf(p_pic_yuv, "\n");
    }
  }
  printf(" Copy_frame_for_file_pred() end\n");

}
void Copy_frame_for_SAO()
{
	int i, j, k;
	char tmpChar;
	//printf(" \n\nCopy_frame_for_SAO()\n");
	for (j = 0; j < img->height; j++)
	{
		for ( i = 0; i< img->width; i++)
		{
#if EXTRACT_PIC_YUV_SAO   //2016-5-23 提取整帧图像的经过sao处理后的数据
			tmpChar = imgY[j][i];

			fwrite(&tmpChar,1,1,p_pic_yuv_sao);
			//printf("%4d\t",imgY[j][i]);
#endif	
			imgY_sao[j][i] = imgY[j][i];
		}
	}
	for (k = 0; k < 2; k++)
	{
		for (j = 0; j < img->height_cr; j++)
		{
			for ( i = 0; i< img->width_cr; i++)
			{
#if EXTRACT_PIC_YUV_SAO   //2016-5-23 提取整帧图像的经过sao处理后的数据
				tmpChar = imgUV[k][j][i];
				fwrite(&tmpChar,1,1,p_pic_yuv_sao);
#endif	
				imgUV_sao[k][j][i] = imgUV[k][j][i];
			}
		}
	}

}
void getMergeNeighbor(int smb_index, int pix_y, int pix_x, int smb_pix_width, int smb_pix_height, int input_MaxsizeInBit, int input_slice_set_enable,SAOBlkParam** rec_saoBlkParam, int* MergeAvail, SAOBlkParam sao_merge_param[][NUM_SAO_COMPONENTS] )
{
	int mb_y = pix_y >> MIN_CU_SIZE_IN_BIT;
	int mb_x = pix_x >> MIN_CU_SIZE_IN_BIT;
	int pic_mb_width             = img->width / MIN_CU_SIZE;
	int mb_nr;
	int mergeup_avail, mergeleft_avail;
	int width_in_smb;
	SAOBlkParam* sao_left_param;
	SAOBlkParam* sao_up_param;
	mb_nr = mb_y * pic_mb_width + mb_x;
	width_in_smb = ( img->width % (1<<input_MaxsizeInBit) ) ? ( img->width / (1<<input_MaxsizeInBit) + 1) : ( img->width / (1<<input_MaxsizeInBit)  );
	if ( input_slice_set_enable ) //added by mz, 2008.04
	{
		mergeup_avail      = ( mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr - pic_mb_width].slice_set_index );
		mergeleft_avail    = ( mb_x == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr - 1].slice_set_index );	
	}
	else
	{
		mergeup_avail      = ( mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr - pic_mb_width].slice_nr );
		mergeleft_avail    = ( mb_x == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr - 1].slice_nr );
	}
	if (mergeleft_avail)
	{
		sao_left_param =  rec_saoBlkParam[smb_index -  1] ;
		copySAOParam_for_blk(sao_merge_param[SAO_MERGE_LEFT], sao_left_param);
	}
	if (mergeup_avail)
	{
		sao_up_param = rec_saoBlkParam[smb_index - width_in_smb];
		copySAOParam_for_blk(sao_merge_param[SAO_MERGE_ABOVE], sao_up_param);
	}
	MergeAvail[SAO_MERGE_LEFT] = mergeleft_avail;
	MergeAvail[SAO_MERGE_ABOVE] = mergeup_avail;
}
void checkBoundaryAvail(int mb_y, int mb_x, int smb_pix_height, int smb_pix_width, int input_slice_set_enable, int* smb_available_left, int* smb_available_right, int* smb_available_up, int* smb_available_down, int* smb_available_upleft, int* smb_available_upright, int* smb_available_leftdown, int* smb_available_rightdwon, int filter_on)
{

	int pic_mb_width             = img->width / MIN_CU_SIZE;
	int pic_mb_height            = img->height / MIN_CU_SIZE;
	int mb_nr                    = mb_y * pic_mb_width + mb_x;
	int smb_mb_width             = smb_pix_width >> MIN_CU_SIZE_IN_BIT;
	int smb_mb_height            = smb_pix_height >> MIN_CU_SIZE_IN_BIT;

	if ( input_slice_set_enable ) //added by mz, 2008.04
	{
		*smb_available_up      = ( mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr - pic_mb_width].slice_set_index );
		*smb_available_down    = ( mb_y >= pic_mb_height - smb_mb_height) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr + smb_mb_height *pic_mb_width].slice_set_index );
		*smb_available_left    = ( mb_x == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr - 1].slice_set_index );
		*smb_available_right   = ( mb_x >= pic_mb_width - smb_mb_width ) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr + smb_mb_width].slice_set_index );
		*smb_available_upleft  = ( mb_x == 0 || mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr - pic_mb_width - 1].slice_set_index );
		*smb_available_upright = ( mb_x >= pic_mb_width - smb_mb_height  || mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr - pic_mb_width + smb_mb_width].slice_set_index );
		*smb_available_leftdown = ( mb_x == 0 || mb_y >= pic_mb_height - smb_mb_height ) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr  + smb_mb_height *pic_mb_width - 1].slice_set_index );
		*smb_available_rightdwon = ( mb_x >= pic_mb_width - smb_mb_width || mb_y >= pic_mb_height - smb_mb_height ) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr  + smb_mb_height *pic_mb_width + smb_mb_width].slice_set_index );
	}
	else
	{
		*smb_available_up      = ( mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr - pic_mb_width].slice_nr );
		*smb_available_down    = ( mb_y >= pic_mb_height - smb_mb_height) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr + smb_mb_height *pic_mb_width].slice_nr );
		*smb_available_left    = ( mb_x == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr - 1].slice_nr );
		*smb_available_right   = ( mb_x >= pic_mb_width - smb_mb_width ) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr + smb_mb_width].slice_nr );
		*smb_available_upleft  = ( mb_x == 0 || mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr - pic_mb_width - 1].slice_nr );
		*smb_available_upright = ( mb_x >= pic_mb_width - smb_mb_width  || mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr - pic_mb_width + smb_mb_width].slice_nr );
		*smb_available_leftdown = ( mb_x == 0 || mb_y >= pic_mb_height - smb_mb_height ) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr  + smb_mb_height *pic_mb_width - 1].slice_nr );
		*smb_available_rightdwon = ( mb_x >= pic_mb_width - smb_mb_width || mb_y >= pic_mb_height - smb_mb_height ) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr  + smb_mb_height *pic_mb_width + smb_mb_width].slice_nr );

	}
	if (!filter_on)
	{
		*smb_available_down = 0;
		*smb_available_right = 0;
		*smb_available_leftdown = 0;
		*smb_available_rightdwon = 0;
	}
}
#if SAO_SLICE_BOUND_PRO
#if SAO_MULSLICE_FIX
void checkBoundaryProc(int pix_y, int pix_x, int smb_pix_height, int smb_pix_width, int comp, int input_slice_set_enable, int* smb_process_left, int* smb_process_right, int* smb_process_up, int* smb_process_down, int* smb_process_upleft, int* smb_process_upright, int* smb_process_leftdown, int* smb_process_rightdwon, int filter_on)
{
	int pic_width                 = comp ? img->width_cr : img->width;
	int pic_height               = comp ? img->height_cr : img->height;
	int pic_mb_width             = img->width / MIN_CU_SIZE;
	int pic_mb_height            = img->height / MIN_CU_SIZE;
	int mb_size_in_bit            = comp ? (MIN_CU_SIZE_IN_BIT - 1) : MIN_CU_SIZE_IN_BIT;
	int mb_nr_cur, mb_nr_up, mb_nr_down, mb_nr_left, mb_nr_right, mb_nr_upleft, mb_nr_upright, mb_nr_leftdown, mb_nr_rightdown;

	
	mb_nr_cur = (pix_y >> mb_size_in_bit) * pic_mb_width + (pix_x >> mb_size_in_bit);
	mb_nr_up = ((pix_y - 1) >> mb_size_in_bit) * pic_mb_width + (pix_x >> mb_size_in_bit);
	mb_nr_down = ((pix_y + smb_pix_height) >> mb_size_in_bit) * pic_mb_width + (pix_x >> mb_size_in_bit);
	mb_nr_left = (pix_y >> mb_size_in_bit) * pic_mb_width + ((pix_x - 1) >> mb_size_in_bit);
	mb_nr_right = (pix_y >> mb_size_in_bit) * pic_mb_width + ((pix_x + smb_pix_width) >> mb_size_in_bit);
	mb_nr_upleft = ((pix_y - 1) >> mb_size_in_bit) * pic_mb_width + ((pix_x - 1) >> mb_size_in_bit);
	mb_nr_upright = ((pix_y - 1) >> mb_size_in_bit) * pic_mb_width + ((pix_x + smb_pix_width) >> mb_size_in_bit);
	mb_nr_leftdown = ((pix_y + smb_pix_height) >> mb_size_in_bit) * pic_mb_width + ((pix_x - 1) >> mb_size_in_bit);
	mb_nr_rightdown = ((pix_y + smb_pix_height) >> mb_size_in_bit) * pic_mb_width + ((pix_x + smb_pix_width) >> mb_size_in_bit);



	if ( input_slice_set_enable ) //added by mz, 2008.04
	{
		*smb_process_up = (pix_y == 0) ? 0 : (img->mb_data[mb_nr_cur].slice_set_index == img->mb_data[mb_nr_up].slice_set_index) ? 1 : sao_cross_slice;
		*smb_process_down = (pix_y >= pic_height - smb_pix_height) ? 0 : (img->mb_data[mb_nr_cur].slice_set_index == img->mb_data[mb_nr_down].slice_set_index) ? 1 : sao_cross_slice;
		*smb_process_left = (pix_x == 0) ? 0 : (img->mb_data[mb_nr_cur].slice_set_index == img->mb_data[mb_nr_left].slice_set_index) ? 1 : sao_cross_slice;
		*smb_process_right = (pix_x >= pic_width - smb_pix_width) ? 0 : (img->mb_data[mb_nr_cur].slice_set_index == img->mb_data[mb_nr_right].slice_set_index) ? 1 : sao_cross_slice;
		*smb_process_upleft = (pix_x == 0 || pix_y == 0) ? 0 : (img->mb_data[mb_nr_cur].slice_set_index == img->mb_data[mb_nr_upleft].slice_set_index) ? 1 : sao_cross_slice;
		*smb_process_upright = (pix_x >= pic_width - smb_pix_width || pix_y == 0) ? 0 : (img->mb_data[mb_nr_cur].slice_set_index == img->mb_data[mb_nr_upright].slice_set_index) ? 1 : sao_cross_slice;
		*smb_process_leftdown = (pix_x == 0 || pix_y >= pic_height - smb_pix_height) ? 0 : (img->mb_data[mb_nr_cur].slice_set_index == img->mb_data[mb_nr_leftdown].slice_set_index) ? 1 : sao_cross_slice;
		*smb_process_rightdwon = (pix_x >= pic_width - smb_pix_width || pix_y >= pic_height - smb_pix_height) ? 0 : (img->mb_data[mb_nr_cur].slice_set_index == img->mb_data[mb_nr_rightdown].slice_set_index) ? 1 : sao_cross_slice;
	}
	else
	{
		*smb_process_up = (pix_y == 0) ? 0 : (img->mb_data[mb_nr_cur].slice_nr == img->mb_data[mb_nr_up].slice_nr) ? 1 : sao_cross_slice;
		*smb_process_down = (pix_y >= pic_height - smb_pix_height) ? 0 : (img->mb_data[mb_nr_cur].slice_nr == img->mb_data[mb_nr_down].slice_nr) ? 1 : sao_cross_slice;
		*smb_process_left = (pix_x == 0) ? 0 : (img->mb_data[mb_nr_cur].slice_nr == img->mb_data[mb_nr_left].slice_nr) ? 1 : sao_cross_slice;
		*smb_process_right = (pix_x >= pic_width - smb_pix_width) ? 0 : (img->mb_data[mb_nr_cur].slice_nr == img->mb_data[mb_nr_right].slice_nr) ? 1 : sao_cross_slice;
		*smb_process_upleft = (pix_x == 0 || pix_y == 0) ? 0 : (img->mb_data[mb_nr_cur].slice_nr == img->mb_data[mb_nr_upleft].slice_nr) ? 1 : sao_cross_slice;
		*smb_process_upright = (pix_x >= pic_width - smb_pix_width || pix_y == 0) ? 0 : (img->mb_data[mb_nr_cur].slice_nr == img->mb_data[mb_nr_upright].slice_nr) ? 1 : sao_cross_slice;
		*smb_process_leftdown = (pix_x == 0 || pix_y >= pic_height - smb_pix_height) ? 0 : (img->mb_data[mb_nr_cur].slice_nr == img->mb_data[mb_nr_leftdown].slice_nr) ? 1 : sao_cross_slice;
		*smb_process_rightdwon = (pix_x >= pic_width - smb_pix_width || pix_y >= pic_height - smb_pix_height) ? 0 : (img->mb_data[mb_nr_cur].slice_nr == img->mb_data[mb_nr_rightdown].slice_nr) ? 1 : sao_cross_slice;
	}
	if (!filter_on)
	{
		*smb_process_down = 0;
		*smb_process_right = 0;
		*smb_process_leftdown = 0;
		*smb_process_rightdwon = 0;
	}
}
#else
void checkBoundaryProc(int mb_y, int mb_x, int smb_pix_height, int smb_pix_width, int input_slice_set_enable, int* smb_process_left, int* smb_process_right, int* smb_process_up, int* smb_process_down, int* smb_process_upleft, int* smb_process_upright, int* smb_process_leftdown, int* smb_process_rightdwon, int filter_on)
{

	int pic_mb_width             = img->width / MIN_CU_SIZE;
	int pic_mb_height            = img->height / MIN_CU_SIZE;
	int mb_nr                    = mb_y * pic_mb_width + mb_x;
	int smb_mb_width             = smb_pix_width >> MIN_CU_SIZE_IN_BIT;
	int smb_mb_height            = smb_pix_height >> MIN_CU_SIZE_IN_BIT;

	
	if ( input_slice_set_enable ) //added by mz, 2008.04
	{
		*smb_process_up      = ( mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr - pic_mb_width].slice_set_index ) ? 1 : sao_cross_slice;
		*smb_process_down    = ( mb_y >= pic_mb_height - smb_mb_height) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr + smb_mb_height *pic_mb_width].slice_set_index ) ? 1 : sao_cross_slice;
		*smb_process_left    = ( mb_x == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr - 1].slice_set_index ) ? 1 : sao_cross_slice;
		*smb_process_right   = ( mb_x >= pic_mb_width - smb_mb_width ) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr + smb_mb_width].slice_set_index ) ? 1 : sao_cross_slice;
		*smb_process_upleft  = ( mb_x == 0 || mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr - pic_mb_width - 1].slice_set_index ) ? 1 : sao_cross_slice;
		*smb_process_upright = ( mb_x >= pic_mb_width - smb_mb_height  || mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr - pic_mb_width + smb_mb_width].slice_set_index ) ? 1 : sao_cross_slice;
		*smb_process_leftdown = ( mb_x == 0 || mb_y >= pic_mb_height - smb_mb_height ) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr  + smb_mb_height *pic_mb_width - 1].slice_set_index ) ? 1 : sao_cross_slice;
		*smb_process_rightdwon = ( mb_x >= pic_mb_width - smb_mb_width || mb_y >= pic_mb_height - smb_mb_height ) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr  + smb_mb_height *pic_mb_width + smb_mb_width].slice_set_index ) ? 1 : sao_cross_slice;
	}
	else
	{
		*smb_process_up      = ( mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr - pic_mb_width].slice_nr ) ? 1 : sao_cross_slice;
		*smb_process_down    = ( mb_y >= pic_mb_height - smb_mb_height) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr + smb_mb_height *pic_mb_width].slice_nr ) ? 1 : sao_cross_slice;
		*smb_process_left    = ( mb_x == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr - 1].slice_nr ) ? 1 : sao_cross_slice;
		*smb_process_right   = ( mb_x >= pic_mb_width - smb_mb_width ) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr + smb_mb_width].slice_nr ) ? 1 : sao_cross_slice;
		*smb_process_upleft  = ( mb_x == 0 || mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr - pic_mb_width - 1].slice_nr ) ? 1 : sao_cross_slice;
		*smb_process_upright = ( mb_x >= pic_mb_width - smb_mb_width  || mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr - pic_mb_width + smb_mb_width].slice_nr ) ? 1 : sao_cross_slice;
		*smb_process_leftdown = ( mb_x == 0 || mb_y >= pic_mb_height - smb_mb_height ) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr  + smb_mb_height *pic_mb_width - 1].slice_nr ) ? 1 : sao_cross_slice;
		*smb_process_rightdwon = ( mb_x >= pic_mb_width - smb_mb_width || mb_y >= pic_mb_height - smb_mb_height ) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr  + smb_mb_height *pic_mb_width + smb_mb_width].slice_nr ) ? 1 : sao_cross_slice;

	}
	if (!filter_on)
	{
		*smb_process_down = 0;
		*smb_process_right = 0;
		*smb_process_leftdown = 0;
		*smb_process_rightdwon = 0;
	}
}
#endif
void checkBoundaryPara(int mb_y, int mb_x, int smb_pix_height, int smb_pix_width, int input_slice_set_enable, int* smb_process_left, int* smb_process_right, int* smb_process_up, int* smb_process_down, int* smb_process_upleft, int* smb_process_upright, int* smb_process_leftdown, int* smb_process_rightdwon, int filter_on)
{

	int pic_mb_width             = img->width / MIN_CU_SIZE;
	int pic_mb_height            = img->height / MIN_CU_SIZE;
	int mb_nr                    = mb_y * pic_mb_width + mb_x;
	int smb_mb_width             = smb_pix_width >> MIN_CU_SIZE_IN_BIT;
	int smb_mb_height            = smb_pix_height >> MIN_CU_SIZE_IN_BIT;

	
	if ( input_slice_set_enable ) //added by mz, 2008.04
	{
		*smb_process_up      = ( mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr - pic_mb_width].slice_set_index ) ? 1 : sao_para_cross_slice;
		*smb_process_down    = ( mb_y >= pic_mb_height - smb_mb_height) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr + smb_mb_height *pic_mb_width].slice_set_index ) ? 1 : sao_para_cross_slice;
		*smb_process_left    = ( mb_x == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr - 1].slice_set_index ) ? 1 : sao_para_cross_slice;
		*smb_process_right   = ( mb_x >= pic_mb_width - smb_mb_width ) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr + smb_mb_width].slice_set_index ) ? 1 : sao_para_cross_slice;
		*smb_process_upleft  = ( mb_x == 0 || mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr - pic_mb_width - 1].slice_set_index ) ? 1 : sao_para_cross_slice;
		*smb_process_upright = ( mb_x >= pic_mb_width - smb_mb_height  || mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr - pic_mb_width + smb_mb_width].slice_set_index ) ? 1 : sao_para_cross_slice;
		*smb_process_leftdown = ( mb_x == 0 || mb_y >= pic_mb_height - smb_mb_height ) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr  + smb_mb_height *pic_mb_width - 1].slice_set_index ) ? 1 : sao_para_cross_slice;
		*smb_process_rightdwon = ( mb_x >= pic_mb_width - smb_mb_width || mb_y >= pic_mb_height - smb_mb_height ) ? 0 : ( img->mb_data[mb_nr].slice_set_index == img->mb_data[mb_nr  + smb_mb_height *pic_mb_width + smb_mb_width].slice_set_index ) ? 1 : sao_para_cross_slice;
	}
	else
	{
		*smb_process_up      = ( mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr - pic_mb_width].slice_nr ) ? 1 : sao_para_cross_slice;
		*smb_process_down    = ( mb_y >= pic_mb_height - smb_mb_height) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr + smb_mb_height *pic_mb_width].slice_nr ) ? 1 : sao_para_cross_slice;
		*smb_process_left    = ( mb_x == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr - 1].slice_nr ) ? 1 : sao_para_cross_slice;
		*smb_process_right   = ( mb_x >= pic_mb_width - smb_mb_width ) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr + smb_mb_width].slice_nr ) ? 1 : sao_para_cross_slice;
		*smb_process_upleft  = ( mb_x == 0 || mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr - pic_mb_width - 1].slice_nr ) ? 1 : sao_para_cross_slice;
		*smb_process_upright = ( mb_x >= pic_mb_width - smb_mb_width  || mb_y == 0 ) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr - pic_mb_width + smb_mb_width].slice_nr ) ? 1 : sao_para_cross_slice;
		*smb_process_leftdown = ( mb_x == 0 || mb_y >= pic_mb_height - smb_mb_height ) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr  + smb_mb_height *pic_mb_width - 1].slice_nr ) ? 1 : sao_para_cross_slice;
		*smb_process_rightdwon = ( mb_x >= pic_mb_width - smb_mb_width || mb_y >= pic_mb_height - smb_mb_height ) ? 0 : ( img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr  + smb_mb_height *pic_mb_width + smb_mb_width].slice_nr ) ? 1 : sao_para_cross_slice;

	}
	if (!filter_on)
	{
		*smb_process_down = 0;
		*smb_process_right = 0;
		*smb_process_leftdown = 0;
		*smb_process_rightdwon = 0;
	}
}
#endif
#if EXTEND_BD
void SAOFrame(int input_MaxSizeInBit, int input_slice_set_enable, SAOBlkParam** rec_saoBlkParam, int *slice_sao_on, int sample_bit_depth)
#else
void SAOFrame(int input_MaxSizeInBit, int input_slice_set_enable, SAOBlkParam** rec_saoBlkParam, int *slice_sao_on)
#endif
{

	int pix_y, pix_x,  mb_x, mb_y, smb_y, smb_x;
	int smb_pix_height, smb_pix_width;
	int smb_index = -1;

	for ( pix_y = 0, smb_y = 0 ; pix_y <  img->height ; pix_y += smb_pix_height, smb_y ++ )
	{

		smb_pix_height = min( 1 << ( input_MaxSizeInBit ), ( img->height - pix_y )  );
		mb_y = pix_y >> MIN_CU_SIZE_IN_BIT;
		for ( pix_x = 0, smb_x = 0 ; pix_x <  img->width  ; pix_x += smb_pix_width, smb_x++ )
		{
			smb_pix_width = min( 1 << ( input_MaxSizeInBit ), ( img->width - pix_x ) );
			mb_x = pix_x >> MIN_CU_SIZE_IN_BIT;
			smb_index++;
#if EXTEND_BD
			SAO_on_smb(smb_index,pix_y, pix_x, smb_pix_width, smb_pix_height,input_slice_set_enable,rec_saoBlkParam[smb_index], sample_bit_depth);
#else
			SAO_on_smb(smb_index,pix_y, pix_x, smb_pix_width, smb_pix_height,input_slice_set_enable,rec_saoBlkParam[smb_index]);
#endif
		}
	}
}
#if EXTEND_BD
void SAO_on_smb(int smb_index,int pix_y, int pix_x, int smb_pix_width, int smb_pix_height, int input_slice_set_enable, SAOBlkParam* saoBlkParam, int sample_bit_depth)
#else
void SAO_on_smb(int smb_index,int pix_y, int pix_x, int smb_pix_width, int smb_pix_height, int input_slice_set_enable, SAOBlkParam* saoBlkParam)
#endif
{  
	int compIdx;
	int mb_y = pix_y >> MIN_CU_SIZE_IN_BIT;
	int mb_x = pix_x >> MIN_CU_SIZE_IN_BIT;
	int  isLeftAvail, isRightAvail, isAboveAvail, isBelowAvail, isAboveLeftAvail, isAboveRightAvail, isBelowLeftAvail, isBelowRightAvail;
	int smb_pix_height_t, smb_pix_width_t, pix_x_t, pix_y_t;
#if SAO_SLICE_BOUND_PRO
	int  isLeftProc, isRightProc, isAboveProc, isBelowProc, isAboveLeftProc, isAboveRightProc, isBelowLeftProc, isBelowRightProc;
#endif

#if SAO_SLICE_BOUND_PRO
#if !SAO_MULSLICE_FIX
	 checkBoundaryProc(mb_y, mb_x, smb_pix_height, smb_pix_width, input_slice_set_enable, &isLeftProc, &isRightProc, &isAboveProc, &isBelowProc, &isAboveLeftProc, &isAboveRightProc, &isBelowLeftProc, &isBelowRightProc, 1);
#endif
	 checkBoundaryPara(mb_y, mb_x, smb_pix_height, smb_pix_width, input_slice_set_enable, &isLeftAvail, &isRightAvail, &isAboveAvail, &isBelowAvail, &isAboveLeftAvail, &isAboveRightAvail, &isBelowLeftAvail, &isBelowRightAvail, 1);
#else
	checkBoundaryAvail(mb_y, mb_x, smb_pix_height, smb_pix_width, input_slice_set_enable, &isLeftAvail, &isRightAvail, &isAboveAvail, &isBelowAvail, &isAboveLeftAvail, &isAboveRightAvail, &isBelowLeftAvail, &isBelowRightAvail, 1);
#endif
	if((saoBlkParam[SAO_Y ].modeIdc == SAO_MODE_OFF) && (saoBlkParam[SAO_Cb].modeIdc == SAO_MODE_OFF) && (saoBlkParam[SAO_Cr].modeIdc == SAO_MODE_OFF))
	{
		return;
	}
	for ( compIdx= 0; compIdx < NUM_SAO_COMPONENTS; compIdx++)
	{

		if (saoBlkParam[compIdx].modeIdc != SAO_MODE_OFF)
		{
			smb_pix_width_t =  compIdx ? ((smb_pix_width >> 1) - SAO_SHIFT_PIX_NUM ) : ( smb_pix_width - SAO_SHIFT_PIX_NUM );
			smb_pix_height_t =  compIdx ? ((smb_pix_height >> 1) - SAO_SHIFT_PIX_NUM ) : ( smb_pix_height - SAO_SHIFT_PIX_NUM );
			pix_x_t = compIdx ? (pix_x >> 1 ) : pix_x ;
			pix_y_t = compIdx ? (pix_y >> 1 ) : pix_y;
#if SAO_SLICE_BOUND_PRO
#if EXTEND_BD
#if SAO_MULSLICE_FIX
			checkBoundaryProc(pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, compIdx, input_slice_set_enable, &isLeftProc, &isRightProc, &isAboveProc, &isBelowProc, &isAboveLeftProc, &isAboveRightProc, &isBelowLeftProc, &isBelowRightProc, 1);
			assert(isRightProc == 1); assert(isBelowProc == 1); assert(isBelowRightProc == 1); assert(isAboveProc == isAboveRightProc); assert(isLeftProc == isBelowLeftProc);
			//it's supposed that chroma has the same result as luma!!!
			SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftProc /*Left*/, isRightProc/*Right*/, isAboveProc/*Above*/, isBelowProc/*Below*/, isAboveLeftProc/*AboveLeft*/, isAboveRightProc/*AboveRight*/, isBelowLeftProc/*BelowLeft*/, isBelowRightProc/*BelowRight*/, sample_bit_depth);	
#else
			SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftProc /*Left*/, 1/*Right*/, isAboveProc/*Above*/, 1/*Below*/,isAboveLeftProc/*AboveLeft*/, isAboveProc/*AboveRight*/, isLeftProc/*BelowLeft*/, 1/*BelowRight*/, sample_bit_depth);
#endif
#else
			SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftProc /*Left*/, 1/*Right*/, isAboveProc/*Above*/, 1/*Below*/,isAboveLeftProc/*AboveLeft*/, isAboveProc/*AboveRight*/, isLeftProc/*BelowLeft*/, 1/*BelowRight*/);
#endif
#else
#if EXTEND_BD
			SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftAvail /*Left*/, 1/*Right*/, isAboveAvail/*Above*/, 1/*Below*/,isAboveLeftAvail/*AboveLeft*/, isAboveAvail/*AboveRight*/, isLeftAvail/*BelowLeft*/, 1/*BelowRight*/, sample_bit_depth);
#else
			SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftAvail /*Left*/, 1/*Right*/, isAboveAvail/*Above*/, 1/*Below*/,isAboveLeftAvail/*AboveLeft*/, isAboveAvail/*AboveRight*/, isLeftAvail/*BelowLeft*/, 1/*BelowRight*/);
#endif
#endif
			if (isAboveLeftAvail)
			{
				smb_pix_width_t = SAO_SHIFT_PIX_NUM;
				smb_pix_height_t = SAO_SHIFT_PIX_NUM;
				pix_x_t = compIdx ? ((pix_x >> 1) - SAO_SHIFT_PIX_NUM ) : ( pix_x - SAO_SHIFT_PIX_NUM );
				pix_y_t = compIdx ? ((pix_y >> 1) - SAO_SHIFT_PIX_NUM ) : ( pix_y - SAO_SHIFT_PIX_NUM );
				assert(isAboveAvail && isLeftAvail);
#if SAO_SLICE_BOUND_PRO
#if EXTEND_BD
#if SAO_MULSLICE_FIX	
				checkBoundaryProc(pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, compIdx, input_slice_set_enable, &isLeftProc, &isRightProc, &isAboveProc, &isBelowProc, &isAboveLeftProc, &isAboveRightProc, &isBelowLeftProc, &isBelowRightProc, 1);
				assert(isLeftProc == 1); assert(isAboveProc == 1); assert(isAboveLeftProc == 1); assert(isRightProc == isAboveRightProc); assert(isBelowProc == isBelowLeftProc);
				//it's supposed that chroma has the same result as luma!!!
				SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftProc /*Left*/, isRightProc/*Right*/, isAboveProc/*Above*/, isBelowProc/*Below*/, isAboveLeftProc/*AboveLeft*/, isAboveRightProc/*AboveRight*/, isBelowLeftProc/*BelowLeft*/, isBelowRightProc/*BelowRight*/, sample_bit_depth);
#else
				SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isAboveLeftProc /*Left*/, isAboveProc/*Right*/, isAboveLeftProc/*Above*/, isLeftProc/*Below*/,isAboveLeftProc/*AboveLeft*/, isAboveProc/*AboveRight*/, isLeftProc/*BelowLeft*/, 1/*BelowRight*/, sample_bit_depth);
#endif
#else
				SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isAboveLeftProc /*Left*/, isAboveProc/*Right*/, isAboveLeftProc/*Above*/, isLeftProc/*Below*/,isAboveLeftProc/*AboveLeft*/, isAboveProc/*AboveRight*/, isLeftProc/*BelowLeft*/, 1/*BelowRight*/);
#endif
#else
#if EXTEND_BD
				SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isAboveLeftAvail /*Left*/, isAboveAvail/*Right*/, isAboveLeftAvail/*Above*/, isLeftAvail/*Below*/,isAboveLeftAvail/*AboveLeft*/, isAboveAvail/*AboveRight*/, isLeftAvail/*BelowLeft*/, 1/*BelowRight*/, sample_bit_depth);
#else
				SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isAboveLeftAvail /*Left*/, isAboveAvail/*Right*/, isAboveLeftAvail/*Above*/, isLeftAvail/*Below*/,isAboveLeftAvail/*AboveLeft*/, isAboveAvail/*AboveRight*/, isLeftAvail/*BelowLeft*/, 1/*BelowRight*/);
#endif
#endif
			}

			if (isLeftAvail)
			{
				smb_pix_width_t = SAO_SHIFT_PIX_NUM;
				smb_pix_height_t = compIdx ? ((smb_pix_height >> 1) - SAO_SHIFT_PIX_NUM ) : ( smb_pix_height - SAO_SHIFT_PIX_NUM );
				pix_x_t = compIdx ? ((pix_x >> 1) - SAO_SHIFT_PIX_NUM ) : ( pix_x - SAO_SHIFT_PIX_NUM );
				pix_y_t = compIdx ? (pix_y >> 1) : pix_y ;
#if SAO_SLICE_BOUND_PRO
#if EXTEND_BD
#if SAO_MULSLICE_FIX
				checkBoundaryProc(pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, compIdx, input_slice_set_enable, &isLeftProc, &isRightProc, &isAboveProc, &isBelowProc, &isAboveLeftProc, &isAboveRightProc, &isBelowLeftProc, &isBelowRightProc, 1);
				assert(isLeftProc == 1); assert(isBelowProc == 1); assert(isBelowLeftProc == 1); assert(isAboveProc == isAboveLeftProc); assert(isRightProc == isBelowRightProc);
				//it's supposed that chroma has the same result as luma!!!
				SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftProc /*Left*/, isRightProc/*Right*/, isAboveProc/*Above*/, isBelowProc/*Below*/, isAboveLeftProc/*AboveLeft*/, isAboveRightProc/*AboveRight*/, isBelowLeftProc/*BelowLeft*/, isBelowRightProc/*BelowRight*/, sample_bit_depth);
#else
				SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftProc /*Left*/, 1/*Right*/, isAboveLeftProc/*Above*/, isLeftProc/*Below*/,isAboveLeftProc/*AboveLeft*/, isAboveProc/*AboveRight*/, isLeftProc/*BelowLeft*/, 1/*BelowRight*/, sample_bit_depth);
#endif
#else
				SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftProc /*Left*/, 1/*Right*/, isAboveLeftProc/*Above*/, isLeftProc/*Below*/,isAboveLeftProc/*AboveLeft*/, isAboveProc/*AboveRight*/, isLeftProc/*BelowLeft*/, 1/*BelowRight*/);
#endif
#else
#if EXTEND_BD
				SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftAvail /*Left*/, 1/*Right*/, isAboveLeftAvail/*Above*/, isLeftAvail/*Below*/,isAboveLeftAvail/*AboveLeft*/, isAboveAvail/*AboveRight*/, isLeftAvail/*BelowLeft*/, 1/*BelowRight*/, sample_bit_depth);
#else
				SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftAvail /*Left*/, 1/*Right*/, isAboveLeftAvail/*Above*/, isLeftAvail/*Below*/,isAboveLeftAvail/*AboveLeft*/, isAboveAvail/*AboveRight*/, isLeftAvail/*BelowLeft*/, 1/*BelowRight*/);
#endif
#endif
			}
			if (isAboveAvail)
			{
				smb_pix_width_t =compIdx ? ((smb_pix_width >> 1) - SAO_SHIFT_PIX_NUM ) : ( smb_pix_width - SAO_SHIFT_PIX_NUM );
				smb_pix_height_t = SAO_SHIFT_PIX_NUM;
				pix_x_t = compIdx ? (pix_x >> 1) : pix_x;
				pix_y_t = compIdx ? ((pix_y >> 1) - SAO_SHIFT_PIX_NUM ) : ( pix_y - SAO_SHIFT_PIX_NUM );
#if SAO_SLICE_BOUND_PRO
#if EXTEND_BD
#if SAO_MULSLICE_FIX
				checkBoundaryProc(pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, compIdx, input_slice_set_enable, &isLeftProc, &isRightProc, &isAboveProc, &isBelowProc, &isAboveLeftProc, &isAboveRightProc, &isBelowLeftProc, &isBelowRightProc, 1);
				assert(isRightProc == 1); assert(isAboveProc == 1); assert(isAboveRightProc == 1); assert(isBelowProc == isBelowRightProc); assert(isLeftProc == isAboveLeftProc);
				//it's supposed that chroma has the same result as luma!!!
				SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftProc /*Left*/, isRightProc/*Right*/, isAboveProc/*Above*/, isBelowProc/*Below*/, isAboveLeftProc/*AboveLeft*/, isAboveRightProc/*AboveRight*/, isBelowLeftProc/*BelowLeft*/, isBelowRightProc/*BelowRight*/, sample_bit_depth);
#else
				SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isAboveLeftProc /*Left*/, isAboveProc/*Right*/, isAboveProc/*Above*/, 1/*Below*/,isAboveLeftProc/*AboveLeft*/, isAboveProc/*AboveRight*/, isLeftProc/*BelowLeft*/, 1/*BelowRight*/, sample_bit_depth);
#endif
#else
				SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isAboveLeftProc /*Left*/, isAboveProc/*Right*/, isAboveProc/*Above*/, 1/*Below*/,isAboveLeftProc/*AboveLeft*/, isAboveProc/*AboveRight*/, isLeftProc/*BelowLeft*/, 1/*BelowRight*/);
#endif
#else
#if EXTEND_BD
				SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isAboveLeftAvail /*Left*/, isAboveAvail/*Right*/, isAboveAvail/*Above*/, 1/*Below*/,isAboveLeftAvail/*AboveLeft*/, isAboveAvail/*AboveRight*/, isLeftAvail/*BelowLeft*/, 1/*BelowRight*/, sample_bit_depth);
#else
				SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isAboveLeftAvail /*Left*/, isAboveAvail/*Right*/, isAboveAvail/*Above*/, 1/*Below*/,isAboveLeftAvail/*AboveLeft*/, isAboveAvail/*AboveRight*/, isLeftAvail/*BelowLeft*/, 1/*BelowRight*/);
#endif
#endif
			}

			if (!isRightAvail)
			{
				if (isAboveAvail && !isAboveRightAvail)
				{
					smb_pix_width_t = SAO_SHIFT_PIX_NUM;
					smb_pix_height_t = SAO_SHIFT_PIX_NUM;
					pix_x_t = compIdx ? ((pix_x >> 1) + (smb_pix_width >> 1) - SAO_SHIFT_PIX_NUM  ) : ( pix_x + smb_pix_width - SAO_SHIFT_PIX_NUM );
					pix_y_t = compIdx ? ((pix_y >> 1) - SAO_SHIFT_PIX_NUM ) : ( pix_y - SAO_SHIFT_PIX_NUM );
#if SAO_SLICE_BOUND_PRO
#if EXTEND_BD
#if SAO_MULSLICE_FIX
					checkBoundaryProc(pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, compIdx, input_slice_set_enable, &isLeftProc, &isRightProc, &isAboveProc, &isBelowProc, &isAboveLeftProc, &isAboveRightProc, &isBelowLeftProc, &isBelowRightProc, 1);
					assert(isLeftProc == 1);  assert(isAboveProc == 1);  assert(isAboveLeftProc == 1);  assert(isBelowProc == isBelowLeftProc);  assert(isRightProc == 0); assert(isAboveRightProc == 0); assert(isBelowRightProc == 0);
					//it's supposed that chroma has the same result as luma!!!
					SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftProc /*Left*/, isRightProc/*Right*/, isAboveProc/*Above*/, isBelowProc/*Below*/, isAboveLeftProc/*AboveLeft*/, isAboveRightProc/*AboveRight*/, isBelowLeftProc/*BelowLeft*/, isBelowRightProc/*BelowRight*/, sample_bit_depth);	
#else
					SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isAboveProc /*Left*/, isAboveRightProc/*Right*/, isAboveProc/*Above*/, 1/*Below*/,isAboveProc/*AboveLeft*/, isAboveRightProc/*AboveRight*/, 1/*BelowLeft*/, isRightProc/*BelowRight*/, sample_bit_depth);
#endif
#else
					SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isAboveProc /*Left*/, isAboveRightProc/*Right*/, isAboveProc/*Above*/, 1/*Below*/,isAboveProc/*AboveLeft*/, isAboveRightProc/*AboveRight*/, 1/*BelowLeft*/, isRightProc/*BelowRight*/);
#endif
#else
#if EXTEND_BD
					SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isAboveAvail /*Left*/, isAboveRightAvail/*Right*/, isAboveAvail/*Above*/, 1/*Below*/,isAboveAvail/*AboveLeft*/, isAboveRightAvail/*AboveRight*/, 1/*BelowLeft*/, isRightAvail/*BelowRight*/, sample_bit_depth);
#else
					SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isAboveAvail /*Left*/, isAboveRightAvail/*Right*/, isAboveAvail/*Above*/, 1/*Below*/,isAboveAvail/*AboveLeft*/, isAboveRightAvail/*AboveRight*/, 1/*BelowLeft*/, isRightAvail/*BelowRight*/);
#endif
#endif
				}

				
				smb_pix_width_t = SAO_SHIFT_PIX_NUM;
				smb_pix_height_t = compIdx ? ((smb_pix_height >> 1) - SAO_SHIFT_PIX_NUM ) : ( smb_pix_height - SAO_SHIFT_PIX_NUM );
				pix_x_t = compIdx ? ((pix_x >> 1) + (smb_pix_width >> 1) - SAO_SHIFT_PIX_NUM  ) : ( pix_x + smb_pix_width - SAO_SHIFT_PIX_NUM );
				pix_y_t = compIdx ? (pix_y >> 1) : pix_y ;
#if SAO_SLICE_BOUND_PRO
#if EXTEND_BD
#if SAO_MULSLICE_FIX
				checkBoundaryProc(pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, compIdx, input_slice_set_enable, &isLeftProc, &isRightProc, &isAboveProc, &isBelowProc, &isAboveLeftProc, &isAboveRightProc, &isBelowLeftProc, &isBelowRightProc, 1);
				assert(isLeftProc == 1); assert(isBelowProc == 1); assert(isBelowLeftProc == 1); assert(isAboveProc == isAboveLeftProc); assert(isRightProc == 0); assert(isAboveRightProc == 0); assert(isBelowRightProc == 0);
				//it's supposed that chroma has the same result as luma!!!
				SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftProc /*Left*/, isRightProc/*Right*/, isAboveProc/*Above*/, isBelowProc/*Below*/, isAboveLeftProc/*AboveLeft*/, isAboveRightProc/*AboveRight*/, isBelowLeftProc/*BelowLeft*/, isBelowRightProc/*BelowRight*/, sample_bit_depth);	
#else
				SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, 1 /*Left*/, isRightProc/*Right*/, isAboveProc/*Above*/, 1/*Below*/,isAboveProc/*AboveLeft*/, isAboveRightProc/*AboveRight*/, 1/*BelowLeft*/,isRightProc/*BelowRight*/, sample_bit_depth);
#endif
#else
				SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, 1 /*Left*/, isRightProc/*Right*/, isAboveProc/*Above*/, 1/*Below*/,isAboveProc/*AboveLeft*/, isAboveRightProc/*AboveRight*/, 1/*BelowLeft*/,isRightProc/*BelowRight*/);
#endif
#else
#if EXTEND_BD
				SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, 1 /*Left*/, isRightAvail/*Right*/, isAboveAvail/*Above*/, 1/*Below*/,isAboveAvail/*AboveLeft*/, isAboveRightAvail/*AboveRight*/, 1/*BelowLeft*/,isRightAvail/*BelowRight*/, sample_bit_depth);
#else
				SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, 1 /*Left*/, isRightAvail/*Right*/, isAboveAvail/*Above*/, 1/*Below*/,isAboveAvail/*AboveLeft*/, isAboveRightAvail/*AboveRight*/, 1/*BelowLeft*/,isRightAvail/*BelowRight*/);
#endif
#endif
			}
			if (!isBelowAvail)
			{
				if (isLeftAvail)
				{		
					smb_pix_width_t = SAO_SHIFT_PIX_NUM;
					smb_pix_height_t = SAO_SHIFT_PIX_NUM;
					pix_x_t = compIdx ? ((pix_x >> 1) - SAO_SHIFT_PIX_NUM ) : ( pix_x - SAO_SHIFT_PIX_NUM );
					pix_y_t = compIdx ? ((pix_y >> 1) + (smb_pix_height >> 1) - SAO_SHIFT_PIX_NUM  ) : ( pix_y + smb_pix_height - SAO_SHIFT_PIX_NUM );
#if SAO_SLICE_BOUND_PRO
#if EXTEND_BD
#if SAO_MULSLICE_FIX
					assert(!isBelowLeftAvail);
					checkBoundaryProc(pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, compIdx, input_slice_set_enable, &isLeftProc, &isRightProc, &isAboveProc, &isBelowProc, &isAboveLeftProc, &isAboveRightProc, &isBelowLeftProc, &isBelowRightProc, 1);
					assert(isLeftProc == 1); assert(isAboveProc == 1); assert(isAboveLeftProc == 1); assert(isRightProc == isAboveRightProc); assert(isBelowProc == 0); assert(isBelowLeftProc == 0); assert(isBelowRightProc == 0);
					//it's supposed that chroma has the same result as luma!!!
					SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftProc /*Left*/, isRightProc/*Right*/, isAboveProc/*Above*/, isBelowProc/*Below*/, isAboveLeftProc/*AboveLeft*/, isAboveRightProc/*AboveRight*/, isBelowLeftProc/*BelowLeft*/, isBelowRightProc/*BelowRight*/, sample_bit_depth);	
#else
					SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftProc /*Left*/, 1/*Right*/, isLeftProc/*Above*/, isBelowLeftProc/*Below*/,isLeftProc/*AboveLeft*/, 1/*AboveRight*/, isBelowLeftProc/*BelowLeft*/, isBelowProc/*BelowRight*/, sample_bit_depth);
#endif
#else
					SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftProc /*Left*/, 1/*Right*/, isLeftProc/*Above*/, isBelowLeftProc/*Below*/,isLeftProc/*AboveLeft*/, 1/*AboveRight*/, isBelowLeftProc/*BelowLeft*/, isBelowProc/*BelowRight*/);
#endif
#else
#if EXTEND_BD
					SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftAvail /*Left*/, 1/*Right*/, isLeftAvail/*Above*/, isBelowLeftAvail/*Below*/,isLeftAvail/*AboveLeft*/, 1/*AboveRight*/, isBelowLeftAvail/*BelowLeft*/, isBelowAvail/*BelowRight*/, sample_bit_depth);
#else
					SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftAvail /*Left*/, 1/*Right*/, isLeftAvail/*Above*/, isBelowLeftAvail/*Below*/,isLeftAvail/*AboveLeft*/, 1/*AboveRight*/, isBelowLeftAvail/*BelowLeft*/, isBelowAvail/*BelowRight*/);
#endif
#endif
				}

				
				smb_pix_width_t =compIdx ? ((smb_pix_width >> 1) - SAO_SHIFT_PIX_NUM ) : ( smb_pix_width - SAO_SHIFT_PIX_NUM );
				smb_pix_height_t = SAO_SHIFT_PIX_NUM;
				pix_x_t = compIdx ? (pix_x >> 1) :  pix_x;
				pix_y_t = compIdx ? ((pix_y >> 1) + (smb_pix_height >> 1) - SAO_SHIFT_PIX_NUM  ) : ( pix_y + smb_pix_height - SAO_SHIFT_PIX_NUM );
#if SAO_SLICE_BOUND_PRO
#if EXTEND_BD
#if SAO_MULSLICE_FIX
				checkBoundaryProc(pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, compIdx, input_slice_set_enable, &isLeftProc, &isRightProc, &isAboveProc, &isBelowProc, &isAboveLeftProc, &isAboveRightProc, &isBelowLeftProc, &isBelowRightProc, 1);
				assert(isRightProc == 1); assert(isAboveProc == 1); assert(isAboveRightProc == 1); assert(isLeftProc == isAboveLeftProc); assert(isBelowProc == 0); assert(isBelowLeftProc == 0); assert(isBelowRightProc == 0);
				//it's supposed that chroma has the same result as luma!!!
				SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftProc /*Left*/, isRightProc/*Right*/, isAboveProc/*Above*/, isBelowProc/*Below*/, isAboveLeftProc/*AboveLeft*/, isAboveRightProc/*AboveRight*/, isBelowLeftProc/*BelowLeft*/, isBelowRightProc/*BelowRight*/, sample_bit_depth);	
#else
				SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftProc /*Left*/, 1/*Right*/, 1/*Above*/, isBelowProc/*Below*/,isLeftProc/*AboveLeft*/, 1/*AboveRight*/, isBelowLeftProc/*BelowLeft*/, isBelowProc/*BelowRight*/, sample_bit_depth);
#endif
#else
				SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftProc /*Left*/, 1/*Right*/, 1/*Above*/, isBelowProc/*Below*/,isLeftProc/*AboveLeft*/, 1/*AboveRight*/, isBelowLeftProc/*BelowLeft*/, isBelowProc/*BelowRight*/);
#endif
#else
#if EXTEND_BD
				SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftAvail /*Left*/, 1/*Right*/, 1/*Above*/, isBelowAvail/*Below*/,isLeftAvail/*AboveLeft*/, 1/*AboveRight*/, isBelowLeftAvail/*BelowLeft*/, isBelowAvail/*BelowRight*/, sample_bit_depth);
#else
				SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftAvail /*Left*/, 1/*Right*/, 1/*Above*/, isBelowAvail/*Below*/,isLeftAvail/*AboveLeft*/, 1/*AboveRight*/, isBelowLeftAvail/*BelowLeft*/, isBelowAvail/*BelowRight*/);
#endif
#endif
			}
			if (!isBelowRightAvail && !isRightAvail && !isBelowAvail)
			{	
				smb_pix_width_t = SAO_SHIFT_PIX_NUM;
				smb_pix_height_t = SAO_SHIFT_PIX_NUM;
				pix_x_t = compIdx ? ((pix_x >> 1) + (smb_pix_width >> 1) - SAO_SHIFT_PIX_NUM  ) : ( pix_x + smb_pix_width - SAO_SHIFT_PIX_NUM );
				pix_y_t = compIdx ? ((pix_y >> 1) + (smb_pix_height >> 1) - SAO_SHIFT_PIX_NUM  ) : ( pix_y + smb_pix_height - SAO_SHIFT_PIX_NUM );
#if SAO_SLICE_BOUND_PRO
#if EXTEND_BD
#if SAO_MULSLICE_FIX
				checkBoundaryProc(pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, compIdx, input_slice_set_enable, &isLeftProc, &isRightProc, &isAboveProc, &isBelowProc, &isAboveLeftProc, &isAboveRightProc, &isBelowLeftProc, &isBelowRightProc, 1);
				assert(isLeftProc == 1); assert(isAboveProc == 1); assert(isAboveLeftProc == 1); assert(isAboveRightProc == 0); assert(isRightProc == 0); assert(isBelowRightProc==0); assert(isBelowProc == 0); assert(isBelowLeftProc == 0);
				//it's supposed that chroma has the same result as luma!!!
				SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftProc /*Left*/, isRightProc/*Right*/, isAboveProc/*Above*/, isBelowProc/*Below*/, isAboveLeftProc/*AboveLeft*/, isAboveRightProc/*AboveRight*/, isBelowLeftProc/*BelowLeft*/, isBelowRightProc/*BelowRight*/, sample_bit_depth);	
#else
				SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, 1 /*Left*/, isRightProc/*Right*/, 1/*Above*/, isBelowProc/*Below*/,1/*AboveLeft*/, isRightProc/*AboveRight*/, isBelowProc/*BelowLeft*/, isBelowRightProc/*BelowRight*/, sample_bit_depth);
#endif
#else
				SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, 1 /*Left*/, isRightProc/*Right*/, 1/*Above*/, isBelowProc/*Below*/,1/*AboveLeft*/, isRightProc/*AboveRight*/, isBelowProc/*BelowLeft*/, isBelowRightProc/*BelowRight*/);
#endif
#else
#if EXTEND_BD
				SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, 1 /*Left*/, isRightAvail/*Right*/, 1/*Above*/, isBelowAvail/*Below*/,1/*AboveLeft*/, isRightAvail/*AboveRight*/, isBelowAvail/*BelowLeft*/, isBelowRightAvail/*BelowRight*/, sample_bit_depth);
#else
				SAO_on_block(&(saoBlkParam[compIdx]), compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, 1 /*Left*/, isRightAvail/*Right*/, 1/*Above*/, isBelowAvail/*Below*/,1/*AboveLeft*/, isRightAvail/*AboveRight*/, isBelowAvail/*BelowLeft*/, isBelowRightAvail/*BelowRight*/);
#endif
#endif
			}
		}
	}
}
#if EXTEND_BD
void SAO_on_block(SAOBlkParam* saoBlkParam, int compIdx, int smb_index, int pix_y, int pix_x, int smb_pix_height, int smb_pix_width, int smb_available_left, int smb_available_right, int smb_available_up, int smb_available_down, int smb_available_upleft, int smb_available_upright, int smb_available_leftdown, int smb_available_rightdwon, int sample_bit_depth)
#else
void SAO_on_block(SAOBlkParam* saoBlkParam, int compIdx, int smb_index, int pix_y, int pix_x, int smb_pix_height, int smb_pix_width, int smb_available_left, int smb_available_right, int smb_available_up, int smb_available_down, int smb_available_upleft, int smb_available_upright, int smb_available_leftdown, int smb_available_rightdwon)
#endif
{
	int type;
	int start_x, end_x, start_y, end_y;
	int start_x_r0, end_x_r0, start_x_r, end_x_r, start_x_rn, end_x_rn;
	int x, y;
	byte **Src, **Dst;
	char leftsign, rightsign, upsign, downsign;
	long diff;
	char *signupline, *signupline1;
	int reg=0;
	int edgetype, bandtype;
	if (compIdx == SAO_Y)
	{
		Src = imgY_sao;
		Dst = imgY;
	}
	else
	{

		Src = imgUV_sao[compIdx - 1];
		Dst = imgUV[compIdx - 1];
	}
	signupline = (char*) malloc((smb_pix_width+1) * sizeof(char));

	assert(saoBlkParam->modeIdc==SAO_MODE_NEW);
	type = saoBlkParam->typeIdc;

		switch (type)
		{
		case SAO_TYPE_EO_0:
			{
				start_y = 0;
				end_y = smb_pix_height;
				start_x = smb_available_left? 0 : 1;
				end_x = smb_available_right ? smb_pix_width : (smb_pix_width - 1); 
				for( y = start_y; y < end_y; y++ )
				{
					diff = Src[pix_y  + y][pix_x  + start_x] - Src[pix_y  + y][pix_x + start_x - 1];
					leftsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
					for ( x = start_x; x < end_x; x++)
					{
						diff = Src[pix_y  + y][pix_x + x] - Src[pix_y  + y][pix_x + x + 1];
						rightsign =  diff > 0 ? 1 : (diff < 0 ? -1 : 0);
						edgetype = leftsign + rightsign;
						leftsign = - rightsign;
#if EXTEND_BD
						Dst[pix_y  + y][pix_x + x] = Clip3(0, ((1 << sample_bit_depth) - 1), Src[pix_y + y][pix_x + x] + saoBlkParam->offset[edgetype + 2]);
#else
						Dst[pix_y  + y][pix_x + x] = Clip1(Src[pix_y + y][pix_x + x] + saoBlkParam->offset[edgetype + 2]);
#endif
					}
				}

			}
			break;
		case SAO_TYPE_EO_90:
			{
				start_x = 0;
				end_x = smb_pix_width;
				start_y = smb_available_up? 0 : 1;
				end_y = smb_available_down ? smb_pix_height : (smb_pix_height - 1); 
				for ( x = start_x; x < end_x; x++ )
				{
					diff = Src[pix_y  + start_y][pix_x  + x] - Src[pix_y  + start_y - 1][pix_x + x];
					upsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
					for ( y = start_y; y < end_y; y++)
					{
						diff = Src[pix_y + y][pix_x  + x] - Src[pix_y + y + 1][pix_x + x ];
						downsign =  diff > 0 ? 1 : (diff < 0 ? -1 : 0);
						edgetype = downsign + upsign;
						upsign = - downsign;
#if EXTEND_BD
						Dst[pix_y  + y][pix_x + x] = Clip3(0, ((1 << sample_bit_depth) - 1), Src[pix_y + y][pix_x + x] + saoBlkParam->offset[edgetype + 2]);
#else
						Dst[pix_y  + y][pix_x + x] = Clip1(Src[pix_y + y][pix_x + x] + saoBlkParam->offset[edgetype + 2]);
#endif
					}
				}
			}
			break;
		case SAO_TYPE_EO_135:
			{
				start_x_r0 = smb_available_upleft ? 0 : 1;
				end_x_r0 = smb_available_up ? (smb_available_right ? smb_pix_width : (smb_pix_width - 1)) : 1;
				start_x_r = smb_available_left ? 0 : 1;
				end_x_r = smb_available_right ? smb_pix_width : (smb_pix_width - 1);
				start_x_rn = smb_available_down ? (smb_available_left ? 0 : 1) : (smb_pix_width - 1);
				end_x_rn = smb_available_rightdwon ? smb_pix_width : (smb_pix_width - 1);

				//init the line buffer
				for ( x = start_x_r + 1; x < end_x_r + 1; x ++)
				{
					diff = Src[pix_y  + 1][pix_x  + x] - Src[pix_y ][pix_x  + x - 1];
					upsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
					signupline[x] = upsign;
				}
				//first row
				for ( x = start_x_r0; x < end_x_r0 ; x++ )
				{
					diff = Src[pix_y ][pix_x + x] - Src[pix_y  - 1][pix_x + x - 1]; 
					upsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
					edgetype = upsign - signupline[x + 1];
#if EXTEND_BD
					Dst[pix_y ][pix_x + x] = Clip3(0, ((1 << sample_bit_depth) - 1), Src[pix_y][pix_x + x] + saoBlkParam->offset[edgetype + 2]);
#else
					Dst[pix_y ][pix_x + x] = Clip1(Src[pix_y][pix_x + x] + saoBlkParam->offset[edgetype + 2]);
#endif
				}

				//middle rows
				for ( y = 1 ; y < smb_pix_height - 1; y++)
				{
					for ( x = start_x_r; x < end_x_r; x++ )
					{
						if ( x == start_x_r )
						{
							diff = Src[pix_y  + y][pix_x + x] - Src[pix_y  + y - 1][pix_x + x - 1];  
							upsign =  diff > 0 ? 1 : (diff < 0 ? -1 : 0);
							signupline[x] = upsign;
						}
						diff = Src[pix_y + y][pix_x + x] - Src[pix_y  + y + 1][pix_x + x + 1];  
						downsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
						edgetype = downsign + signupline[x];
#if EXTEND_BD
						Dst[pix_y  + y][pix_x + x] = Clip3(0, ((1 << sample_bit_depth) - 1), Src[pix_y  + y][pix_x + x] + saoBlkParam->offset[edgetype + 2]);
#else
						Dst[pix_y  + y][pix_x + x] = Clip1(Src[pix_y  + y][pix_x + x] + saoBlkParam->offset[edgetype + 2]);
#endif
						signupline[x] = reg;
						reg = -downsign;
					}
				}
				//last row
				for ( x = start_x_rn; x < end_x_rn; x++)
				{
					if ( x == start_x_r )
					{
						diff = Src[pix_y + smb_pix_height - 1][pix_x + x] - Src[pix_y + smb_pix_height - 2][pix_x + x - 1];  
						upsign =  diff > 0 ? 1 : (diff < 0 ? -1 : 0);
						signupline[x] = upsign;
					}
					diff = Src[pix_y  + smb_pix_height - 1][pix_x  + x] - Src[pix_y  + smb_pix_height ][pix_x  + x + 1];  
					downsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
					edgetype = downsign + signupline[x];
#if EXTEND_BD
					Dst[pix_y  + smb_pix_height - 1][pix_x + x] = Clip3(0, ((1 << sample_bit_depth) - 1), Src[pix_y  + smb_pix_height - 1][pix_x + x] + saoBlkParam->offset[edgetype + 2]);
#else
					Dst[pix_y  + smb_pix_height - 1][pix_x + x] = Clip1(Src[pix_y  + smb_pix_height - 1][pix_x + x] + saoBlkParam->offset[edgetype + 2]);
#endif
				}
			}
			break;
		case SAO_TYPE_EO_45:
			{
				start_x_r0 = smb_available_up ? (smb_available_left ? 0: 1) : (smb_pix_width - 1);
				end_x_r0 = smb_available_upright ?  smb_pix_width : (smb_pix_width - 1);
				start_x_r = smb_available_left ? 0 : 1;
				end_x_r = smb_available_right ? smb_pix_width : (smb_pix_width - 1);
				start_x_rn = smb_available_leftdown ? 0 : 1;
				end_x_rn = smb_available_down ? (smb_available_right ? smb_pix_width : (smb_pix_width - 1)) : 1;

				//init the line buffer
				signupline1 = signupline + 1;
				for ( x = start_x_r - 1; x < end_x_r - 1; x ++)
				{
					diff = Src[pix_y  + 1][pix_x + x] - Src[pix_y][pix_x + x + 1];
					upsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
					signupline1[x] = upsign;
				}
				//first row
				for ( x = start_x_r0; x < end_x_r0; x++ )
				{
					diff = Src[pix_y ][pix_x + x] - Src[pix_y  - 1][pix_x  + x + 1]; 
					upsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
					edgetype = upsign - signupline1[x - 1];
#if EXTEND_BD
					Dst[pix_y ][pix_x + x] = Clip3(0, ((1 << sample_bit_depth) - 1), Src[pix_y ][pix_x + x] + saoBlkParam->offset[edgetype + 2]);
#else
					Dst[pix_y ][pix_x + x] = Clip1(Src[pix_y ][pix_x + x] + saoBlkParam->offset[edgetype + 2]);
#endif
				}

				//middle rows
				for ( y = 1 ; y < smb_pix_height - 1; y++)
				{
					for ( x = start_x_r; x < end_x_r; x++ )
					{
						if ( x == end_x_r - 1 )
						{
							diff = Src[pix_y  + y][pix_x  + x] - Src[pix_y  + y - 1][pix_x + x + 1];  
							upsign =  diff > 0 ? 1 : (diff < 0 ? -1 : 0);
							signupline1[x] = upsign;
						}
						diff = Src[pix_y + y][pix_x  + x] - Src[pix_y + y + 1][pix_x + x - 1];  
						downsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
						edgetype = downsign + signupline1[x];
#if EXTEND_BD
						Dst[pix_y + y][pix_x + x] = Clip3(0, ((1 << sample_bit_depth) - 1), Src[pix_y  + y][pix_x + x] + saoBlkParam->offset[edgetype + 2]);
#else
						Dst[pix_y + y][pix_x + x] = Clip1(Src[pix_y  + y][pix_x + x] + saoBlkParam->offset[edgetype + 2]);
#endif
						signupline1[x-1]= -downsign;
					}
				}
				for ( x = start_x_rn; x < end_x_rn; x++)
				{
					if ( x == end_x_r - 1 )
					{
						diff = Src[pix_y + smb_pix_height - 1] [pix_x + x] - Src[pix_y + smb_pix_height - 2][pix_x + x + 1];  
						upsign =  diff > 0 ? 1 : (diff < 0 ? -1 : 0);
						signupline1[x] = upsign;
					}
					diff = Src[pix_y + smb_pix_height - 1][pix_x  + x] - Src[pix_y + smb_pix_height ][pix_x + x - 1];  
					downsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
					edgetype = downsign + signupline1[x];
#if EXTEND_BD
					Dst[pix_y + smb_pix_height - 1][pix_x + x] = Clip3(0, ((1 << sample_bit_depth) - 1), Src[pix_y + smb_pix_height - 1][pix_x + x] + saoBlkParam->offset[edgetype + 2]);
#else
					Dst[pix_y + smb_pix_height - 1][pix_x + x] = Clip1(Src[pix_y + smb_pix_height - 1][pix_x + x] + saoBlkParam->offset[edgetype + 2]);
#endif
				}
			}
			break;
		case SAO_TYPE_BO:
			{
				start_x = 0;
				end_x = smb_pix_width;
				start_y = 0; 
				end_y = smb_pix_height;
				for ( x = start_x; x < end_x; x++)
				{
					for ( y = start_y; y < end_y; y++)
					{
#if EXTEND_BD
						bandtype = Src[pix_y  + y][pix_x  + x] >> (sample_bit_depth - NUM_SAO_BO_CLASSES_IN_BIT);
						
						Dst[pix_y + y][pix_x + x] = Clip3(0, ((1 << sample_bit_depth) - 1), Src[pix_y + y][pix_x + x] + saoBlkParam->offset[bandtype]);
#else
						bandtype = Src[pix_y  + y][pix_x  + x] >> (8 - NUM_SAO_BO_CLASSES_IN_BIT);
						
						Dst[pix_y + y][pix_x + x] = Clip1(Src[pix_y + y][pix_x + x] + saoBlkParam->offset[bandtype]);
#endif
					}
				}

			}
			break;
		default:
			{
				printf("Not a supported SAO types\n");
				assert(0);
				exit(-1);
			}
		}

    free(signupline);
}

