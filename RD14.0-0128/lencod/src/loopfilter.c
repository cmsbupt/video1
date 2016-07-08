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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "global.h"
#include "../../lcommon/inc/commonVariables.h"
#include "AEC.h"
#include "../../lcommon/inc/loop-filter.h"
#include "rdopt_coding_state.h"
#define  IClip( Min, Max, Val) (((Val)<(Min))? (Min):(((Val)>(Max))? (Max):(Val)))

#define max(a, b)                   ((a) > (b) ? (a) : (b))  //!< Macro returning max value 
#define min(a, b)                   ((a) < (b) ? (a) : (b))  //!< Macro returning min value 

extern const byte QP_SCALE_CR[64] ;
CSptr cs_sao_init, cs_sao_cur_blk, cs_sao_next_blk;
CSptr cs_sao_cur_type, cs_sao_next_type;
CSptr cs_sao_cur_mergetype, cs_sao_next_mergetype;
byte ALPHA_TABLE[64]  =
{
  0, 0, 0, 0, 0, 0, 1, 1,
  1, 1, 1, 2, 2, 2, 3, 3,
  4, 4, 5, 5, 6, 7, 8, 9,
  10, 11, 12, 13, 15, 16, 18, 20,
  22, 24, 26, 28, 30, 33, 33, 35,
  35, 36, 37, 37, 39, 39, 42, 44,
  46, 48, 50, 52, 53, 54, 55, 56,
  57, 58, 59, 60, 61, 62, 63, 64
} ;
byte  BETA_TABLE[64]  =
{
  0, 0, 0, 0, 0, 0, 1, 1,
  1, 1, 1, 1, 1, 2, 2, 2,
  2, 2, 3, 3, 3, 3, 4, 4,
  4, 4, 5, 5, 5, 5, 6, 6,
  6, 7, 7, 7, 8, 8, 8, 9,
  9, 10, 10, 11, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20, 21, 22,
  23, 23, 24, 24, 25, 25, 26, 27
};
byte CLIP_TAB[64] =
{
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 2, 2,
  2, 2, 2, 2, 2, 2, 3, 3,
  3, 3, 3, 3, 3, 4, 4, 4,
  5, 5, 5, 6, 6, 6, 7, 7,
  7, 7, 8, 8, 8, 9, 9, 9
} ;
int EO_OFFSET_MAP[] = {3, 1, 0, 2, 4, 5, 6, 7};
int deltaband_cost[] = {-1,-1,2,2,4,4,4,4,8,8,8,8,8,8,8,8,16};
////////////////////////////ADD BY HJQ 2012-11-1//////////////////////////////////////////////
	typedef Boolean bool;
#if CHROMA_DEBLOCK_BUG_FIX
    int ** ppbEdgeFilter[2];
#else
	bool ** ppbEdgeFilter[2];  //[Ver / Hor][b8_x][b8_y] 
#endif

	#define LOOPFILTER_SIZE 3   //the minimum 8X8

void GetStrength ( byte Strength[2], codingUnit* MbP, codingUnit* MbQ, int dir, int edge, int block_y, int block_x );
void EdgeLoop ( byte* SrcPtr, byte Strength[2], int QP,  int dir, int width, int Chro );
#if DBLK_16x16_BASED 
void DeblockMb ( byte **imgY, byte ***imgUV, int blk_y, int blk_x, int EdgeDir ) ;
#else
void DeblockMb ( byte **imgY, byte ***imgUV, int blk_y, int blk_x ) ;
#endif
void EdgeLoopX ( byte* SrcPtr, int QP, int dir, int width, int Chro, codingUnit* MbP, codingUnit* MbQ, int block_y, int block_x );
int SkipFiltering ( codingUnit* MbP, codingUnit* MbQ, int dir, int edge, int block_y, int block_x );
void getStatblk(SAOStatData* saostatsData, int compIdx, int smb, int smb_y, int smb_x, int smb_pix_height, int smb_pix_width,int smb_available_left, int smb_available_right, int smb_available_up, int smb_available_down, int smb_available_upleft, int smb_available_upright, int smb_available_leftdown, int smb_available_rightdwon);
void getStatisticsSAO_one_SMB(int smb_index, int mb_y, int mb_x, int smb_mb_height, int smb_mb_width, SAOStatData** saostatData);
void getParaSAO_one_SMB(int smb_index,int mb_y, int mb_x, int smb_mb_height, int smb_mb_width, int *slice_sao_on, SAOStatData** saostatData, SAOBlkParam* saoBlkParam, SAOBlkParam** rec_saoBlkParam, int* num_off_sao, double sao_labmda);
double SAORDCost_for_mode_new(int smb_index, int pix_y, int pix_x, int smb_pix_width, int smb_pix_height, int MergeLeftAvail, int MergeUpAvail,int* slice_sao_on, double sao_lambda, SAOStatData** saostatData, SAOBlkParam* sao_cur_param, SAOBlkParam* rec_sao_cur_param );
double SAORDCost_for_mode_new_YUV_sep(int smb_index, int pix_y, int pix_x, int smb_pix_width, int smb_pix_height, int MergeLeftAvail, int MergeUpAvail,int* slice_sao_on, double sao_lambda, SAOStatData** saostatData, SAOBlkParam* sao_cur_param, SAOBlkParam* rec_sao_cur_param );
double SAORDCost_for_mode_merge(int smb_index, int pix_y, int pix_x, int smb_pix_width, int smb_pix_height, int* slice_sao_on, double sao_labmda, SAOStatData** saostatData, SAOBlkParam** rec_saoBlkParam, SAOBlkParam* sao_cur_param, SAOBlkParam* rec_sao_cur_param, int* MergeLeftAvail, int* MergeUpAvail);
void find_offset( int compIdx, int typeIdc, SAOStatData** saostatData, SAOBlkParam* saoBlkParam,double lambda, int offsetTh );
int offset_estimation(int typeIdx, int classIdx, double lambda, long int offset_ori, int count, long int diff, double* bestCost);
long int get_distortion(int compIdx, int type,SAOStatData** saostatData, SAOBlkParam* sao_cur_param);
long int  distortion_cal( long int count, int offset, long int diff);
void writeParaSAO_one_SMB(int smb_index,int mb_y, int mb_x, int smb_mb_height, int smb_mb_width, int *slice_sao_on, SAOBlkParam* saoBlkParam);
void write_sao_smb(int smb_index, int pix_y, int pix_x, int smb_pix_width, int smb_pix_height, int* slice_sao_on, SAOBlkParam* sao_cur_param);
#if CHROMA_DEBLOCK_BUG_FIX
void xSetEdgeFilterParam(unsigned int uiBitSize, unsigned int b8_x_start, unsigned int b8_y_start, int idir, int edgecondition)
#else
void xSetEdgeFilterParam(unsigned int uiBitSize, unsigned int b8_x_start, unsigned int b8_y_start, int idir)
#endif
{
	int i;

	if (idir==0)
	{
		if (b8_x_start == 0)
		{
			return;
		}
		for (i=0; i< (1<<(uiBitSize-LOOPFILTER_SIZE)) ; i++)
		{
			if ( (b8_y_start+i )< ( img->height >> LOOPFILTER_SIZE) )
			{
				if (ppbEdgeFilter[idir][b8_y_start+i][b8_x_start])
				{
					break;
				}
#if CHROMA_DEBLOCK_BUG_FIX
                ppbEdgeFilter[idir][b8_y_start + i][b8_x_start] = edgecondition;
#else
				ppbEdgeFilter[idir][b8_y_start+i][b8_x_start] = TRUE;
#endif
			}
 			else
 				break;
		}
	}
	else
	{
		if (b8_y_start == 0)
		{
			return;
		}
		for (i=0; i< (1<<(uiBitSize-LOOPFILTER_SIZE)) ; i++)
		{
			if ((b8_x_start+i) < (img->width)>>LOOPFILTER_SIZE)
			{
				if (ppbEdgeFilter[idir][b8_y_start][b8_x_start+i])
				{
					break;
				}
#if CHROMA_DEBLOCK_BUG_FIX
                ppbEdgeFilter[idir][b8_y_start][b8_x_start + i] = edgecondition;
#else
				ppbEdgeFilter[idir][b8_y_start][b8_x_start+i] = TRUE;
#endif
			}
 			else
 				break;
		}
	}

}


void xSetEdgeFilter_One_SMB( unsigned int uiBitSize, unsigned int uiPositionInPic)
{
	int i;
#if CHROMA_DEBLOCK_BUG_FIX
	int edge_type_onlyluma=1;
	int edge_type_all=2;   //EdgeCondition 0:not boundary 1:intra PU boundary,TU boundary 2:CU boundary,inter pu boundary

#endif

	int pix_x_InPic_start = ( uiPositionInPic % img->PicWidthInMbs ) * MIN_CU_SIZE;
	int pix_y_InPic_start = ( uiPositionInPic / img->PicWidthInMbs ) * MIN_CU_SIZE;
	int pix_x_InPic_end = ( uiPositionInPic % img->PicWidthInMbs ) * MIN_CU_SIZE + ( 1 << uiBitSize ); 
	int pix_y_InPic_end = ( uiPositionInPic / img->PicWidthInMbs ) * MIN_CU_SIZE + ( 1 << uiBitSize ); 
	//   int  iBoundary_start = (pix_x_InPic_start>=img->width)||(pix_y_InPic_start>=img->height);
	int  iBoundary_end = ( pix_x_InPic_end > img->width ) || ( pix_y_InPic_end > img->height );
	int pos_x, pos_y, pos_InPic;
#if M3198_CU8
    unsigned int b8_x_start = ( uiPositionInPic % img->PicWidthInMbs ) *(1<<(MIN_CU_SIZE_IN_BIT - LOOPFILTER_SIZE));
    unsigned int b8_y_start = ( uiPositionInPic / img->PicWidthInMbs ) *(1<<(MIN_CU_SIZE_IN_BIT - LOOPFILTER_SIZE)) ;
#else
	unsigned int b8_x_start = ( uiPositionInPic % img->PicWidthInMbs ) *2;
	unsigned int b8_y_start = ( uiPositionInPic / img->PicWidthInMbs ) *2 ;
#endif

	codingUnit *currMB = &img->mb_data[uiPositionInPic];
/*#if M3198_CU8
	if (currMB->ui_MbBitSize < uiBitSize && currMB->ui_MbBitSize > B8X8_IN_BIT)
#else*/
	if (currMB->ui_MbBitSize < uiBitSize)
//#endif
	{
		for (i=0; i<4; i++)
		{
#if M3198_CU8
			int mb_x = ( i % 2 ) << ( uiBitSize - MIN_CU_SIZE_IN_BIT - 1 ); //uiBitSize 5:1 ; 6:2
			int mb_y = ( i / 2 ) << ( uiBitSize - MIN_CU_SIZE_IN_BIT - 1 ); //uiBitSize 5:1 ; 6:2
#else
			int mb_x = ( i % 2 ) << ( uiBitSize - 5 ); //uiBitSize 5:1 ; 6:2
			int mb_y = ( i / 2 ) << ( uiBitSize - 5 ); //uiBitSize 5:1 ; 6:2
#endif
			int pos = uiPositionInPic + mb_y * img->PicWidthInMbs + mb_x;

#if M3198_CU8
			pos_x = pix_x_InPic_start + ( mb_x << MIN_CU_SIZE_IN_BIT );
			pos_y = pix_y_InPic_start + ( mb_y << MIN_CU_SIZE_IN_BIT );
#else
			pos_x = pix_x_InPic_start + ( mb_x << 4 );
			pos_y = pix_y_InPic_start + ( mb_y << 4 );
#endif
			pos_InPic = ( pos_x >= img->width || pos_y >= img->height );

			if ( pos_InPic )
				continue;

			xSetEdgeFilter_One_SMB(uiBitSize-1, pos);
		}
		return;

	}
	//////////////////////////////////////////////////////////////////////////
#if M3198_CU8
  //if (currMB->ui_MbBitSize > B8X8_IN_BIT) 
  {
#if CHROMA_DEBLOCK_BUG_FIX
      xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start, 0, edge_type_all);  /// LEFT
      xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start, 1, edge_type_all);  ///UP
#else
      xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start, 0);  /// LEFT
      xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start, 1);  ///UP
#endif
  }
#else
#if CHROMA_DEBLOCK_BUG_FIX
    xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start, 0,edge_type_all);  /// LEFT
    xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start, 1,edge_type_all);  ///UP
#else
    xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start, 0);  /// LEFT
    xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start, 1);  ///UP
#endif
#endif

	//xSetEdgeFilter_TU() 
	i= (uiBitSize- LOOPFILTER_SIZE-1) ; /// b8 


	//xSetEdgeFilter_PU()
#if M3198_CU8
  if (currMB->ui_MbBitSize > B8X8_IN_BIT) 
#endif
	switch(currMB->cuType)
  {
	case  2:  ///2NXN
#if CHROMA_DEBLOCK_BUG_FIX
        xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start + (1 << (i)), 1, edge_type_all);
#else
		xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start+ (1<<(i) ), 1);
#endif
		break;
	case 3: ///NX2N
#if CHROMA_DEBLOCK_BUG_FIX
        xSetEdgeFilterParam(uiBitSize, b8_x_start + (1 << (i)), b8_y_start, 0, edge_type_all);
#else
        xSetEdgeFilterParam(uiBitSize, b8_x_start + (1 << (i)), b8_y_start, 0);
#endif
		break;
	case 8:  ///NXN_inter
#if CHROMA_DEBLOCK_BUG_FIX
        xSetEdgeFilterParam(uiBitSize, b8_x_start + (1 << (i)), b8_y_start, 0, edge_type_all);
        xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start + (1 << (i)), 1, edge_type_all);
#else
        xSetEdgeFilterParam(uiBitSize, b8_x_start + (1 << (i)), b8_y_start, 0);
        xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start + (1 << (i)), 1);
#endif
	case 9: ///NXN_intra
#if CHROMA_DEBLOCK_BUG_FIX
        xSetEdgeFilterParam(uiBitSize, b8_x_start + (1 << (i)), b8_y_start, 0, edge_type_onlyluma);
        xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start + (1 << (i)), 1, edge_type_onlyluma);
#else
        xSetEdgeFilterParam(uiBitSize, b8_x_start + (1 << (i)), b8_y_start, 0);
        xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start + (1 << (i)), 1);
#endif
		break;
	case InNxNMB:
		if (i-1 >=0)
		{
#if CHROMA_DEBLOCK_BUG_FIX
            xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start + (1 << (i - 1)), 1, edge_type_onlyluma);   ///?¨²32???¦Ì¨ª3?? 1<<-1=0  ???¨²64???¦Ì¨ª3??2?¨º?
            xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start + (1 << (i - 1)) * 2, 1, edge_type_onlyluma);
            xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start + (1 << (i - 1)) * 3, 1, edge_type_onlyluma);
#else
            xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start + (1 << (i - 1)), 1);   ///?¨²32???¦Ì¨ª3?? 1<<-1=0  ???¨²64???¦Ì¨ª3??2?¨º?
            xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start + (1 << (i - 1)) * 2, 1);
            xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start + (1 << (i - 1)) * 3, 1);
#endif
		}
		else
	    {
#if CHROMA_DEBLOCK_BUG_FIX
            xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start + (1 << (i)), 1, edge_type_onlyluma);
#else
            xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start + (1 << (i)), 1);
#endif
        }
		break;
	case INxnNMB:
		if (i-1 >=0)
		{
#if CHROMA_DEBLOCK_BUG_FIX
            xSetEdgeFilterParam(uiBitSize, b8_x_start + (1 << (i - 1)), b8_y_start, 0, edge_type_onlyluma);
            xSetEdgeFilterParam(uiBitSize, b8_x_start + (1 << (i - 1)) * 2, b8_y_start, 0, edge_type_onlyluma);
            xSetEdgeFilterParam(uiBitSize, b8_x_start + (1 << (i - 1)) * 3, b8_y_start, 0, edge_type_onlyluma);
#else
            xSetEdgeFilterParam(uiBitSize, b8_x_start + (1 << (i - 1)), b8_y_start, 0);
            xSetEdgeFilterParam(uiBitSize, b8_x_start + (1 << (i - 1)) * 2, b8_y_start, 0);
            xSetEdgeFilterParam(uiBitSize, b8_x_start + (1 << (i - 1)) * 3, b8_y_start, 0);
#endif
		}
		else
		{
#if CHROMA_DEBLOCK_BUG_FIX
            xSetEdgeFilterParam(uiBitSize, b8_x_start + (1 << (i)), b8_y_start, 0, edge_type_onlyluma);
#else
            xSetEdgeFilterParam(uiBitSize, b8_x_start + (1 << (i)), b8_y_start, 0);
#endif
        }
		break;
	case 4:  ///2NxnU
		if (i-1 >=0)
		{
#if CHROMA_DEBLOCK_BUG_FIX
            xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start + (1 << (i - 1)), 1, edge_type_all);   ///?¨²32???¦Ì¨ª3?? 1<<-1=0  ???¨²64???¦Ì¨ª3??2?¨º?
#else
            xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start + (1 << (i - 1)), 1);   ///?¨²32???¦Ì¨ª3?? 1<<-1=0  ???¨²64???¦Ì¨ª3??2?¨º?
#endif
		}
		break;
	case 5:  ///2NxnD
		if (i-1 >=0)
		{
#if CHROMA_DEBLOCK_BUG_FIX
            xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start + (1 << (i - 1)) * 3, 1, edge_type_all);
#else
            xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start + (1 << (i - 1)) * 3, 1);
#endif
		}
		break;
	case 6:  ///nLx2N
		if (i-1 >=0)
		{
#if CHROMA_DEBLOCK_BUG_FIX
            xSetEdgeFilterParam(uiBitSize, b8_x_start + (1 << (i - 1)), b8_y_start, 0, edge_type_all);
#else
            xSetEdgeFilterParam(uiBitSize, b8_x_start + (1 << (i - 1)), b8_y_start, 0);
#endif
		}
		break;
	case 7:  ///nRx2N
		if (i-1>=0)
		{
#if CHROMA_DEBLOCK_BUG_FIX
            xSetEdgeFilterParam(uiBitSize, b8_x_start + (1 << (i - 1)) * 3, b8_y_start, 0, edge_type_all);
#else
            xSetEdgeFilterParam(uiBitSize, b8_x_start + (1 << (i - 1)) * 3, b8_y_start, 0);
#endif
		}
		break;

#if DEBLOCK_NXN
	case 0: ///direct/skip NXN //qyu modified
		if (currMB->md_directskip_mode==0&&!(img->typeb==BP_IMG))
		{
			xSetEdgeFilterParam(uiBitSize, b8_x_start+ (1<<(i) ), b8_y_start, 0);
			xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start+ (1<<(i )), 1);
		}
		break;
#endif

	default:
// 	case 0: ///direct/skip
// 	case 1: ///size_2Nx2N_inter
// 	case 10: ///2Nx2N_intra
		///currMB->trans_size = 0;
		break;
	}
	//////////////////////////////////////////////////////////////////////////
	
	////////////////////////////////////TU////////////////////////////////////////////////////
#if M3198_CU8
  if (currMB->ui_MbBitSize > B8X8_IN_BIT)
#endif
	if (currMB->cuType!=8 && currMB->cuType!=9 && currMB->trans_size ==1&&currMB->cbp!=0)
	{
		if (input->useNSQT && currMB->ui_MbBitSize > B8X8_IN_BIT&& (currMB->cuType==P2NXN || currMB->cuType==PHOR_UP || currMB->cuType==PHOR_DOWN||currMB->cuType==InNxNMB))
	{
      if (currMB->ui_MbBitSize== B16X16_IN_BIT)
      {
#if CHROMA_DEBLOCK_BUG_FIX
          xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start + (1 << (i)), 1, edge_type_onlyluma);
#else
          xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start + (1 << (i)), 1);
#endif
      }
      else
      {
#if CHROMA_DEBLOCK_BUG_FIX
          xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start + (1 << (i - 1)), 1, edge_type_onlyluma);
          xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start + (1 << (i)), 1, edge_type_onlyluma);
          xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start + (1 << (i)) + (1 << (i - 1)), 1, edge_type_onlyluma);
#else
          xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start + (1 << (i - 1)), 1);
          xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start + (1 << (i)), 1);
          xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start + (1 << (i)) + (1 << (i - 1)), 1);
#endif
      }
    }
	else if (input->useNSQT && currMB->ui_MbBitSize > B8X8_IN_BIT&& (currMB->cuType==PNX2N || currMB->cuType==PVER_LEFT || currMB->cuType==PVER_RIGHT||currMB->cuType==INxnNMB))
	{
      if (currMB->ui_MbBitSize==4)
      {
#if CHROMA_DEBLOCK_BUG_FIX
          xSetEdgeFilterParam(uiBitSize, b8_x_start + (1 << (i)), b8_y_start, 0, edge_type_onlyluma);
#else
          xSetEdgeFilterParam(uiBitSize, b8_x_start + (1 << (i)), b8_y_start, 0);
#endif
      }
      else
      {
#if CHROMA_DEBLOCK_BUG_FIX
          xSetEdgeFilterParam(uiBitSize, b8_x_start + (1 << (i - 1)), b8_y_start, 0, edge_type_onlyluma);
          xSetEdgeFilterParam(uiBitSize, b8_x_start + (1 << (i)), b8_y_start, 0, edge_type_onlyluma);
          xSetEdgeFilterParam(uiBitSize, b8_x_start + (1 << (i)) + (1 << (i - 1)), b8_y_start, 0, edge_type_onlyluma);
#else
          xSetEdgeFilterParam(uiBitSize, b8_x_start + (1 << (i - 1)), b8_y_start, 0);
          xSetEdgeFilterParam(uiBitSize, b8_x_start + (1 << (i)), b8_y_start, 0);
          xSetEdgeFilterParam(uiBitSize, b8_x_start + (1 << (i)) + (1 << (i - 1)), b8_y_start, 0);
#endif
      }
    }
#if M3198_CU8
    else if (currMB->ui_MbBitSize > B8X8_IN_BIT)
#else
    else
#endif
    {
#if CHROMA_DEBLOCK_BUG_FIX
        xSetEdgeFilterParam(uiBitSize, b8_x_start + (1 << (i)), b8_y_start, 0, edge_type_onlyluma);
        xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start + (1 << (i)), 1, edge_type_onlyluma);
#else
        xSetEdgeFilterParam(uiBitSize, b8_x_start + (1 << (i)), b8_y_start, 0);
        xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start + (1 << (i)), 1);
#endif
    }
	}
	//////////////////////////////////////////////////////////////////////////
}

void SetEdgeFilter()
{
	Boolean end_of_picture = FALSE;
	int CurrentMbNumber = 0;
	int MBRowSize = img->width / MIN_CU_SIZE;  

	int num_of_orgMB_in_col ;
	int num_of_orgMB_in_row ;
	int size = 1 << input->g_uiMaxSizeInBit;
	int pix_x;
	int pix_y;
	int num_of_orgMB;

	while ( end_of_picture == FALSE ) // loop over codingUnits
	{
#if M3198_CU8
    pix_x = ( CurrentMbNumber % img->PicWidthInMbs ) << MIN_CU_SIZE_IN_BIT;
    pix_y = ( CurrentMbNumber / img->PicWidthInMbs ) << MIN_CU_SIZE_IN_BIT;

    if ( pix_x + size >= img->width )
    {
      num_of_orgMB_in_row = ( img->width - pix_x ) >> MIN_CU_SIZE_IN_BIT;
    }
    else
    {
      num_of_orgMB_in_row = 1 << ( input->g_uiMaxSizeInBit - MIN_CU_SIZE_IN_BIT );
    }

    if ( pix_y + size >= img->height )
    {
      num_of_orgMB_in_col = ( img->height - pix_y ) >> MIN_CU_SIZE_IN_BIT;
    }
    else
    {
      num_of_orgMB_in_col = 1 << ( input->g_uiMaxSizeInBit - MIN_CU_SIZE_IN_BIT );
    }
    num_of_orgMB = num_of_orgMB_in_col * num_of_orgMB_in_row;

    xSetEdgeFilter_One_SMB(input->g_uiMaxSizeInBit, CurrentMbNumber);

    if ( ( CurrentMbNumber + num_of_orgMB_in_row ) % MBRowSize == 0 ) //end of the row
    {
      CurrentMbNumber += ( num_of_orgMB_in_row + MBRowSize * ( ( 1 << ( input->g_uiMaxSizeInBit - MIN_CU_SIZE_IN_BIT ) ) - 1 ) ); //eg. 64 +: (4-1)*num_mb_inWidth+4
    }
    else
    {
      CurrentMbNumber += num_of_orgMB_in_row;
    }

    if (CurrentMbNumber >= img->PicSizeInMbs)
    {
      end_of_picture = TRUE;
    }
#else
		pix_x = ( CurrentMbNumber % img->PicWidthInMbs ) << 4;
		pix_y = ( CurrentMbNumber / img->PicWidthInMbs ) << 4;

		if ( pix_x + size >= img->width )
		{
			num_of_orgMB_in_row = ( img->width - pix_x ) >> 4;
		}
		else
		{
			num_of_orgMB_in_row = 1 << ( input->g_uiMaxSizeInBit - 4 );
		}

		if ( pix_y + size >= img->height )
		{
			num_of_orgMB_in_col = ( img->height - pix_y ) >> 4;
		}
		else
		{
			num_of_orgMB_in_col = 1 << ( input->g_uiMaxSizeInBit - 4 );
		}
        num_of_orgMB = num_of_orgMB_in_col * num_of_orgMB_in_row;

		xSetEdgeFilter_One_SMB(input->g_uiMaxSizeInBit, CurrentMbNumber);

		if ( ( CurrentMbNumber + num_of_orgMB_in_row ) % MBRowSize == 0 ) //end of the row
		{
			CurrentMbNumber += ( num_of_orgMB_in_row + MBRowSize * ( ( 1 << ( input->g_uiMaxSizeInBit - 4 ) ) - 1 ) ); //eg. 64 +: (4-1)*num_mb_inWidth+4
		}
		else
		{
			CurrentMbNumber += num_of_orgMB_in_row;
		}

		if (CurrentMbNumber >= img->PicSizeInMbs)
		{
			end_of_picture = TRUE;
		}
#endif
	}

}

void CreateEdgeFilter()
{
	int b8_x, b8_y;
	int i, j;

	//////////////////////////////////////////////////////////////////////////initialize
	for (i=0; i<2; i++)
	{
#if CHROMA_DEBLOCK_BUG_FIX
		ppbEdgeFilter[i]= (int **)calloc(( img->height >> LOOPFILTER_SIZE), sizeof(int*));
#else
        ppbEdgeFilter[i] = (bool **)calloc((img->height >> LOOPFILTER_SIZE), sizeof(bool*));
#endif
		for (j=0; j< ( img->height >> LOOPFILTER_SIZE ) ; j++)
		{
#if CHROMA_DEBLOCK_BUG_FIX
			ppbEdgeFilter[i][j] = (int*)calloc(( img->width >> LOOPFILTER_SIZE), sizeof(int));
#else
            ppbEdgeFilter[i][j] = (bool*)calloc((img->width >> LOOPFILTER_SIZE), sizeof(bool));
#endif
		}
	}

	for (i=0; i<2; i++)
	{
		for ( b8_y = 0 ; b8_y < ( img->height >> LOOPFILTER_SIZE ) ; b8_y++ )
			for ( b8_x = 0 ; b8_x < ( img->width >> LOOPFILTER_SIZE ) ; b8_x++ )
			{
#if CHROMA_DEBLOCK_BUG_FIX
                ppbEdgeFilter[i][b8_y][b8_x] = 0;
#else
                ppbEdgeFilter[i][b8_y][b8_x] = FALSE;
#endif
			}
	}
	//////////////////////////////////////////////////////////////////////////
}

void DeleteEdgeFilter(int height)
{
	int i, j;

	for (i=0; i<2; i++)
	{
		for (j=0; j< ( height >> LOOPFILTER_SIZE ) ; j++)
		{
			free(ppbEdgeFilter[i][j]);
		}
		free(ppbEdgeFilter[i]);
	}

}

//////////////////////////////////////////////////////////////////////////


/*
*************************************************************************
* Function:
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/


void DeblockFrame ( byte **imgY, byte ***imgUV )
{
    int       mb_x, mb_y ;    
#if DBLK_16x16_BASED
#endif
#if !FIX_LEAK
	CreateEdgeFilter();
#endif
	SetEdgeFilter();

#if !DBLK_16x16_BASED
    img->current_mb_nr = -1;    // jlzheng  7.18

#if M3198_CU8
    for ( mb_y = 0 ; mb_y < ( img->height >> MIN_CU_SIZE_IN_BIT ) ; mb_y++ )
    {
      for ( mb_x = 0 ; mb_x < ( img->width >> MIN_CU_SIZE_IN_BIT ) ; mb_x++ )
#else
    for ( mb_y = 0 ; mb_y < ( img->height >> 4 ) ; mb_y++ )
    {
      for ( mb_x = 0 ; mb_x < ( img->width >> 4 ) ; mb_x++ )
#endif
    {
      img->current_mb_nr++;   // jlzheng 7.18

      if ( input->chroma_format == 1 )
      {
        DeblockMb ( imgY, imgUV, mb_y, mb_x ) ;
      }
    }
  }
#else
#if !DBLK_FRAME_BASED
  for ( blk16x16_y = 0 ; blk16x16_y < ( img->height+8) >> 4  ; blk16x16_y++ )
  {
    for ( blk16x16_x = 0 ; blk16x16_x < ( img->width+8) >> 4  ; blk16x16_x++ )
    {
      mb_y0 = blk16x16_y *2;
      mb_x0 = blk16x16_x *2;

      for ( mb_y = mb_y0 ; mb_y < min(2 + mb_y0 , img->height >> MIN_CU_SIZE_IN_BIT); mb_y++ )
      {
        for ( mb_x = mb_x0 ; mb_x < min(2 + mb_x0 , img->width >> MIN_CU_SIZE_IN_BIT); mb_x++ )
        {
          img->current_mb_nr = mb_y *( img->width >> MIN_CU_SIZE_IN_BIT ) + mb_x ;

          if ( input->chroma_format == 1 )
          {
            DeblockMb ( imgY, imgUV, mb_y, mb_x, 0 ) ;
          }
        }

      }
      for ( mb_y = mb_y0 ; mb_y < min(2 + mb_y0 , img->height >> MIN_CU_SIZE_IN_BIT); mb_y++ )
      {
        for ( mb_x = mb_x0 ; mb_x < min(2 + mb_x0 , img->width >> MIN_CU_SIZE_IN_BIT); mb_x++ )
        {
          img->current_mb_nr = mb_y *( img->width >> MIN_CU_SIZE_IN_BIT ) + mb_x ;

          if ( input->chroma_format == 1 )
          {
            DeblockMb ( imgY, imgUV, mb_y, mb_x, 1 ) ;
          }
        }

      }

    }
  }
#else
  img->current_mb_nr = -1;    // jlzheng  7.18
  for ( mb_y = 0 ; mb_y < ( img->height >> MIN_CU_SIZE_IN_BIT ) ; mb_y++ )
  {
    for ( mb_x = 0 ; mb_x < ( img->width >> MIN_CU_SIZE_IN_BIT ) ; mb_x++ )
    {
      img->current_mb_nr++;   // jlzheng 7.18
      if ( input->chroma_format == 1 )
      {
        DeblockMb ( imgY, imgUV, mb_y, mb_x, 0 ) ;
      }
    }
  }

  img->current_mb_nr = -1;    // jlzheng  7.18
  for ( mb_y = 0 ; mb_y < ( img->height >> MIN_CU_SIZE_IN_BIT ) ; mb_y++ )
  {
    for ( mb_x = 0 ; mb_x < ( img->width >> MIN_CU_SIZE_IN_BIT ) ; mb_x++ )
    {
      img->current_mb_nr++;   // jlzheng 7.18
      if ( input->chroma_format == 1 )
      {
        DeblockMb ( imgY, imgUV, mb_y, mb_x, 1 ) ;
      }
    }
  }
#endif
#endif
		DeleteEdgeFilter(img->height);
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

#if DBLK_16x16_BASED 
void DeblockMb (  byte **imgY, byte ***imgUV, int mb_y, int mb_x, int EdgeDir )
#else
void DeblockMb (  byte **imgY, byte ***imgUV, int mb_y, int mb_x )
#endif
{
  int           EdgeCondition;
  int           dir, QP ;
  unsigned int b8_x_start ;
  unsigned int b8_y_start ;
  byte          *SrcY, *SrcU = NULL, *SrcV = NULL;
  codingUnit    *MbP, *MbQ ;
  const int     mb_width = img->width / MIN_CU_SIZE;
  codingUnit    *currMB = &img->mb_data[img->current_mb_nr];
  int           iFlagSkipFiltering;

#if M3198_CU8
  int           flag=0;
  SrcY = imgY   [mb_y << MIN_CU_SIZE_IN_BIT] + ( mb_x << MIN_CU_SIZE_IN_BIT ) ;
#else
  SrcY = imgY    [mb_y << 4] + ( mb_x << 4 ) ;
#endif   

  if ( imgUV != NULL )
  {
#if M3198_CU8
    SrcU = imgUV[0][mb_y << (MIN_CU_SIZE_IN_BIT-1)] + ( mb_x << (MIN_CU_SIZE_IN_BIT-1) ) ;
    SrcV = imgUV[1][mb_y << (MIN_CU_SIZE_IN_BIT-1)] + ( mb_x << (MIN_CU_SIZE_IN_BIT-1) ) ;
#else
    SrcU = imgUV[0][mb_y << 3] + ( mb_x << 3 ) ;
    SrcV = imgUV[1][mb_y << 3] + ( mb_x << 3 ) ;
#endif
  }

#if M3198_CU8
  MbQ  = &img->mb_data[mb_y * ( img->width >> MIN_CU_SIZE_IN_BIT ) + mb_x] ;                                           // current Mb
#else
  MbQ  = &img->mb_data[mb_y * ( img->width >> 4 ) + mb_x] ;                                           // current Mb
#endif
#if DBLK_16x16_BASED 
  dir = EdgeDir;
#else
  for ( dir = 0 ; dir < 2 ; dir++ )                       // vertical edges, than horicontal edges
#endif
  {
    EdgeCondition = ( dir && mb_y ) || ( !dir && mb_x )  ; // can not filter beyond frame boundaries

    //added by mz, 2008.04
    if ( !dir && mb_x && input->slice_set_enable )
    {
      EdgeCondition = ( currMB->slice_nr == img->mb_data[img->current_mb_nr - 1].slice_nr ) ? EdgeCondition : 0; //  can not filter beyond slice boundaries   jlzheng 7.8
    }

    if ( !dir && mb_x && !input->crossSliceLoopFilter )
    {
      EdgeCondition = ( currMB->slice_nr == img->mb_data[img->current_mb_nr - 1].slice_nr ) ? EdgeCondition : 0; //  can not filter beyond slice boundaries   jlzheng 7.8
    }

    if ( dir && mb_y && !input->crossSliceLoopFilter )
    {
      EdgeCondition = ( currMB->slice_nr == img->mb_data[img->current_mb_nr - mb_width].slice_nr ) ? EdgeCondition : 0; //  can not filter beyond slice boundaries   jlzheng 7.8
    }

#if !M3198_CU8
    for ( edge = 0 ; edge < 2 ; edge++ )                                       // first 4 vertical strips of 16 pel
    {
#endif
      ///
#if M3198_CU8
      flag = 0;
      b8_x_start = mb_x;
      b8_y_start = mb_y;
#else
      if (dir==0)
      {
        b8_x_start = mb_x *2+edge;
        b8_y_start = mb_y *2 ;
      }
      else
      {
        b8_x_start = mb_x *2;
        b8_y_start = mb_y *2 +edge;
      }
#endif
      /*#if M3198_CU8
      if (flag == 0)
      {
      #endif*/
#if CHROMA_DEBLOCK_BUG_FIX
      EdgeCondition = (ppbEdgeFilter[dir][b8_y_start][b8_x_start] && EdgeCondition) ? ppbEdgeFilter[dir][b8_y_start][b8_x_start] : 0;
#else
      EdgeCondition = (ppbEdgeFilter[dir][b8_y_start][b8_x_start] ) ? EdgeCondition:0;
#endif
      // then  4 horicontal
      if ( EdgeCondition )
      {
#if M3198_CU8
        MbP = ( dir ) ? ( MbQ - ( img->width >> MIN_CU_SIZE_IN_BIT ) )  : ( MbQ - 1 ) ; // MbP = Mb of the remote 4x4 block   
#else
        MbP = ( edge ) ? MbQ : ( ( dir ) ? ( MbQ - ( img->width >> 4 ) )  : ( MbQ - 1 ) ) ; // MbP = Mb of the remote 4x4 block   
#endif 
        QP = ( MbP->qp + MbQ->qp + 1 ) >> 1 ;                             // Average QP of the two blocks

#if M3198_CU8//???
        iFlagSkipFiltering = 0;
#else
        iFlagSkipFiltering = SkipFiltering ( MbP, MbQ, dir, edge, mb_y << 2, mb_x << 2 );
#endif
        if ( !iFlagSkipFiltering )
        {
#if M3198_CU8//???
          EdgeLoopX ( SrcY, QP, dir, img->width, 0, MbP, MbQ, mb_y << 1, mb_x << 1 );

#if CHROMA_DEBLOCK_BUG_FIX
          if ((EdgeCondition==2) && ( imgUV != NULL )/* && ( !edge )*/ )
#else
          if ((imgUV != NULL)/* && ( !edge )*/)
#endif
          {
#if EXTEND_BD
            if (input->sample_bit_depth > 8)
            {
              if ((mb_y % 2 == 0 && dir) || (mb_x % 2 == 0) && (!dir))
              {

#if DBFIX_10bit && CHROMA_DELTA_QP
#if CHROMA_DELTA_QP_SYNTEX_OPT
                int cQPu, cQPv;
#if CHROMAQP_DB_FIX
                int cpQPu,cqQPu, cpQPv, cqQPv;

                cpQPu = MbP->qp - 8 * (input->sample_bit_depth - 8) + input->chroma_quant_param_delta_u;
                cpQPu = cpQPu < 0 ? cpQPu : QP_SCALE_CR[cpQPu];
                cpQPu = Clip3(0, 63 + 8 * (input->sample_bit_depth - 8), cpQPu + 8 * (input->sample_bit_depth - 8));
                cqQPu = MbQ->qp - 8 * (input->sample_bit_depth - 8) + input->chroma_quant_param_delta_u;
                cqQPu = cqQPu < 0 ? cqQPu : QP_SCALE_CR[cqQPu];
                cqQPu = Clip3(0, 63 + 8 * (input->sample_bit_depth - 8), cqQPu + 8 * (input->sample_bit_depth - 8));
                cQPu = (cpQPu + cqQPu + 1) >> 1;

                cpQPv = MbP->qp - 8 * (input->sample_bit_depth - 8) + input->chroma_quant_param_delta_v;
                cpQPv = cpQPv < 0 ? cpQPv : QP_SCALE_CR[cpQPv];
                cpQPv = Clip3(0, 63 + 8 * (input->sample_bit_depth - 8), cpQPv + 8 * (input->sample_bit_depth - 8));
                cqQPv = MbQ->qp - 8 * (input->sample_bit_depth - 8) + input->chroma_quant_param_delta_v;
                cqQPv = cqQPv < 0 ? cqQPv : QP_SCALE_CR[cqQPv];
                cqQPv = Clip3(0, 63 + 8 * (input->sample_bit_depth - 8), cqQPv + 8 * (input->sample_bit_depth - 8));
                cQPv = (cpQPv + cqQPv + 1) >> 1;
#else
                cQPu = QP - 8 * (input->sample_bit_depth - 8) + input->chroma_quant_param_delta_u;
                cQPu = cQPu < 0 ? cQPu : QP_SCALE_CR[cQPu];
                cQPu = Clip3(0, 63 + 8 * (input->sample_bit_depth - 8), cQPu + 8 * (input->sample_bit_depth - 8));
                cQPv = QP - 8 * (input->sample_bit_depth - 8) + input->chroma_quant_param_delta_v;
                cQPv = cQPv < 0 ? cQPv : QP_SCALE_CR[cQPv];
                cQPv = Clip3(0, 63 + 8 * (input->sample_bit_depth - 8), cQPv + 8 * (input->sample_bit_depth - 8));
#endif
                EdgeLoopX ( SrcU, cQPu, dir, img->width_cr, 1, MbP, MbQ, mb_y << 1, mb_x << 1 ) ;
                EdgeLoopX ( SrcV, cQPv, dir, img->width_cr, 1, MbP, MbQ, mb_y << 1, mb_x << 1 ) ;
#else
                if(input->PicWQEnable&&(!input->chroma_quant_param_disable))
                {
                  EdgeLoopX ( SrcU, QP_SCALE_CR[Clip3(0, 63, (QP + input->chroma_quant_param_delta_u - (8 * (input->sample_bit_depth - 8))))] + (8 * (input->sample_bit_depth - 8)), dir, img->width_cr, 1, MbP, MbQ, mb_y << 1, mb_x << 1 ) ;
                  EdgeLoopX ( SrcV, QP_SCALE_CR[Clip3(0, 63, (QP + input->chroma_quant_param_delta_v - (8 * (input->sample_bit_depth - 8))))] + (8 * (input->sample_bit_depth - 8)), dir, img->width_cr, 1, MbP, MbQ, mb_y << 1, mb_x << 1 ) ;
                }
                else
                {
                  EdgeLoopX ( SrcU, QP_SCALE_CR[Clip3(0, 63, (QP - (8 * (input->sample_bit_depth - 8))))] + (8 * (input->sample_bit_depth - 8)), dir, img->width_cr, 1, MbP, MbQ, mb_y << 1, mb_x << 1 ) ;
                  EdgeLoopX ( SrcV, QP_SCALE_CR[Clip3(0, 63, (QP - (8 * (input->sample_bit_depth - 8))))] + (8 * (input->sample_bit_depth - 8)), dir, img->width_cr, 1, MbP, MbQ, mb_y << 1, mb_x << 1 ) ;
                }
#endif
#else
                EdgeLoopX ( SrcU, QP_SCALE_CR[Clip3(0, 63, (QP - (8 * (input->sample_bit_depth - 8))))] + (8 * (input->sample_bit_depth - 8)), dir, img->width_cr, 1, MbP, MbQ, mb_y << 1, mb_x << 1 ) ;
                EdgeLoopX ( SrcV, QP_SCALE_CR[Clip3(0, 63, (QP - (8 * (input->sample_bit_depth - 8))))] + (8 * (input->sample_bit_depth - 8)), dir, img->width_cr, 1, MbP, MbQ, mb_y << 1, mb_x << 1 ) ;		
#endif
              }
            }
            else
            {
#endif
              if ((mb_y % 2 == 0 && dir) || (mb_x % 2 == 0) && (!dir))
              {

#if DBFIX_10bit && CHROMA_DELTA_QP
#if CHROMA_DELTA_QP_SYNTEX_OPT
                int cQPu, cQPv;
#if CHROMAQP_DB_FIX
                int cpQPu,cqQPu, cpQPv, cqQPv;

                cpQPu = MbP->qp - 8 * (input->sample_bit_depth - 8) + input->chroma_quant_param_delta_u;
                cpQPu = cpQPu < 0 ? cpQPu : QP_SCALE_CR[cpQPu];
                cpQPu = Clip3(0, 63 + 8 * (input->sample_bit_depth - 8), cpQPu + 8 * (input->sample_bit_depth - 8));
                cqQPu = MbQ->qp - 8 * (input->sample_bit_depth - 8) + input->chroma_quant_param_delta_u;
                cqQPu = cqQPu < 0 ? cqQPu : QP_SCALE_CR[cqQPu];
                cqQPu = Clip3(0, 63 + 8 * (input->sample_bit_depth - 8), cqQPu + 8 * (input->sample_bit_depth - 8));
                cQPu = (cpQPu + cqQPu + 1) >> 1;

                cpQPv = MbP->qp - 8 * (input->sample_bit_depth - 8) + input->chroma_quant_param_delta_v;
                cpQPv = cpQPv < 0 ? cpQPv : QP_SCALE_CR[cpQPv];
                cpQPv = Clip3(0, 63 + 8 * (input->sample_bit_depth - 8), cpQPv + 8 * (input->sample_bit_depth - 8));
                cqQPv = MbQ->qp - 8 * (input->sample_bit_depth - 8) + input->chroma_quant_param_delta_v;
                cqQPv = cqQPv < 0 ? cqQPv : QP_SCALE_CR[cqQPv];
                cqQPv = Clip3(0, 63 + 8 * (input->sample_bit_depth - 8), cqQPv + 8 * (input->sample_bit_depth - 8));
                cQPv = (cpQPv + cqQPv + 1) >> 1;
#else
                cQPu = QP - 8 * (input->sample_bit_depth - 8) + input->chroma_quant_param_delta_u;
                cQPu = cQPu < 0 ? cQPu : QP_SCALE_CR[cQPu];
                cQPu = Clip3(0, 63 + 8 * (input->sample_bit_depth - 8), cQPu + 8 * (input->sample_bit_depth - 8));
                cQPv = QP - 8 * (input->sample_bit_depth - 8) + input->chroma_quant_param_delta_v;
                cQPv = cQPv < 0 ? cQPv : QP_SCALE_CR[cQPv];
                cQPv = Clip3(0, 63 + 8 * (input->sample_bit_depth - 8), cQPv + 8 * (input->sample_bit_depth - 8));
#endif
                EdgeLoopX ( SrcU, cQPu, dir, img->width_cr, 1, MbP, MbQ, mb_y << 1, mb_x << 1 ) ;
                EdgeLoopX ( SrcV, cQPv, dir, img->width_cr, 1, MbP, MbQ, mb_y << 1, mb_x << 1 ) ;
#else
                if(input->PicWQEnable&&(!input->chroma_quant_param_disable))

                {
                  EdgeLoopX ( SrcU, QP_SCALE_CR[QP + input->chroma_quant_param_delta_u], dir, img->width_cr, 1, MbP, MbQ, mb_y << 1, mb_x << 1 ) ;
                  EdgeLoopX ( SrcV, QP_SCALE_CR[QP + input->chroma_quant_param_delta_v], dir, img->width_cr, 1, MbP, MbQ, mb_y << 1, mb_x << 1 ) ;
                }
                else
                {
                  EdgeLoopX ( SrcU, QP_SCALE_CR[QP], dir, img->width_cr, 1, MbP, MbQ, mb_y << 1, mb_x << 1 ) ;
                  EdgeLoopX ( SrcV, QP_SCALE_CR[QP], dir, img->width_cr, 1, MbP, MbQ, mb_y << 1, mb_x << 1 ) ;
                }
#endif
#else
                EdgeLoopX ( SrcU, QP_SCALE_CR[QP], dir, img->width_cr, 1 ) ;
                EdgeLoopX ( SrcV, QP_SCALE_CR[QP], dir, img->width_cr, 1 ) ;
#endif
              }
#if EXTEND_BD
            }
#endif
          }
#else
          EdgeLoopX ( SrcY + ( edge << 3 ) * ( ( dir ) ? img->width : 1 ), QP, dir, img->width, 0 );

          if ( ( imgUV != NULL ) && ( !edge ) )
          {
            EdgeLoopX ( SrcU + ( edge << 2 ) * ( ( dir ) ? img->width_cr : 1 ), QP_SCALE_CR[QP], dir, img->width_cr, 1 ) ;
            EdgeLoopX ( SrcV + ( edge << 2 ) * ( ( dir ) ? img->width_cr : 1 ), QP_SCALE_CR[QP], dir, img->width_cr, 1 ) ;
          }
#endif

        }

      }
#if M3198_CU8
      //}
#endif
#if !M3198_CU8
    }//end edge
#endif
  }//end loop dir
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


#if M3198_CU8//???
byte BLK_NUM[2][2][2]  = {{{0, 8}, {2, 10}}, {{0, 2}, {8, 10}}} ;
#else
byte BLK_NUM[2][2][2]  = {{{0, 8}, {2, 10}}, {{0, 2}, {8, 10}}} ;
#endif

void GetStrength ( byte Strength[2], codingUnit* MbP, codingUnit* MbQ, int dir, int edge, int block_y, int block_x )
{
  int    blkQ, idx ;
  int    blk_x2, blk_y2, blk_x, blk_y ;
  int    ***tmp_fwMV = img->fw_mv;
  int    ***tmp_mv   =  img->tmp_mv;
  int    ***tmp_bwMV = img->bw_mv;
  int    **fw_refFrArr = img->fw_refFrArr;
  int    **bw_refFrArr = img->bw_refFrArr;

  for ( idx = 0 ; idx < 2 ; idx++ )
  {
    blkQ = BLK_NUM[dir][edge][idx] ;
#if M3198_CU8//???
    blk_y = ( block_y + ( blkQ >> 2 ) ) >> 1;
    blk_x = ( ( block_x + ( blkQ & 3 ) ) >> 1 ) ;
#else
    blk_y = ( block_y + ( blkQ >> 2 ) ) >> 1;
    blk_x = ( ( block_x + ( blkQ & 3 ) ) >> 1 ) ;
#endif
    blk_y = block_y + ( blkQ >> 2 );
    blk_x = block_x + ( blkQ & 3 );
    blk_x >>= 1;
    blk_y >>= 1;
    blk_y2 = blk_y -  dir ;
    blk_x2 = blk_x - !dir ;

#if INTRA_2N
    if ( IS_INTRA( MbP ) || IS_INTRA( MbQ ) )
#else
    if ( ( MbP->cuType == I8MB ) || ( MbQ->cuType == I8MB ) )
#endif
    {
      Strength[idx] = 2;
    }
    else if (( img->type == F_IMG )||( img->type == P_IMG ))
    {
      Strength[idx] = ( abs ( tmp_mv[blk_y][blk_x][0] -   tmp_mv[blk_y2][blk_x2][0] ) >= 4 ) ||
                      ( abs ( tmp_mv[blk_y][blk_x][1] -   tmp_mv[blk_y2][blk_x2][1] ) >= 4 ) ||
                      ( refFrArr [blk_y][blk_x] !=   refFrArr[blk_y2][blk_x2] );
    }
    else
    {
      Strength[idx] = ( abs ( tmp_fwMV[blk_y][blk_x][0] - tmp_fwMV[blk_y2][blk_x2][0] ) >= 4 ) ||
                      ( abs ( tmp_fwMV[blk_y][blk_x][1] - tmp_fwMV[blk_y2][blk_x2][1] ) >= 4 ) ||
                      ( abs ( tmp_bwMV[blk_y][blk_x][0] - tmp_bwMV[blk_y2][blk_x2][0] ) >= 4 ) ||
                      ( abs ( tmp_bwMV[blk_y][blk_x][1] - tmp_bwMV[blk_y2][blk_x2][1] ) >= 4 ) ||
                      ( fw_refFrArr [blk_y][blk_x] !=   fw_refFrArr[blk_y2][blk_x2] ) ||
                      ( bw_refFrArr [blk_y][blk_x] !=   bw_refFrArr[blk_y2][blk_x2] );
    }
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
void EdgeLoop ( byte* SrcPtr, byte Strength[2], int QP, int dir, int width, int Chro )
{
  int      pel, PtrInc, Strng ;
  int      inc, inc2, inc3 ;
  int      C0,  Delta, dif, AbsDelta ;
  int      L2, L1, L0, R0, R1, R2, RL0 ;
  int      Alpha, Beta ;
  int      ap, aq, small_gap;

  PtrInc  = dir ?  1 : width ;
  inc     = dir ?  width : 1 ;
  inc2    = inc << 1 ;
  inc3    = inc + inc2 ;


  Alpha = ALPHA_TABLE[Clip3 ( 0, 63, QP + input->alpha_c_offset ) ]; // jlzheng 7.8

  Beta = BETA_TABLE[Clip3 ( 0, 63, QP + input->beta_offset ) ]; // jlzheng 7.8

#if M3198_CU8//???
  for ( pel = 0 ; pel < 16 ; pel++ )
  {
    if ( ( Strng = Strength[pel >> 3] ) )
#else
  for ( pel = 0 ; pel < 16 ; pel++ )
  {
    if ( ( Strng = Strength[pel >> 3] ) )
#endif
    {
      L2  = SrcPtr[-inc3] ;
      L1  = SrcPtr[-inc2] ;
      L0  = SrcPtr[-inc ] ;
      R0  = SrcPtr[    0] ;
      R1  = SrcPtr[ inc ] ;
      R2  = SrcPtr[ inc2] ;
      AbsDelta = abs ( Delta = R0 - L0 ) ;

      if ( AbsDelta < Alpha )
      {
        if ( ( abs ( R0 - R1 ) < Beta ) && ( abs ( L0 - L1 ) < Beta ) )
        {
          aq = ( abs ( R0 - R2 ) < Beta ) ;
          ap = ( abs ( L0 - L2 ) < Beta ) ;

          if ( Strng == 2 )
          {
            RL0 = L0 + R0 ;
            small_gap = ( AbsDelta < ( ( Alpha >> 2 ) + 2 ) );
            aq &= small_gap;
            ap &= small_gap;

            SrcPtr[   0 ] = aq ? ( R1 + RL0 +  R0 + 2 ) >> 2 : ( ( R1 << 1 ) + R0 + L0 + 2 ) >> 2 ;
            SrcPtr[-inc ] = ap ? ( L0 +  L1 + RL0 + 2 ) >> 2 : ( ( L1 << 1 ) + L0 + R0 + 2 ) >> 2 ;

            SrcPtr[ inc ] = ( aq && !Chro ) ? ( R0 + R1 + L0 + R1 + 2 ) >> 2 : SrcPtr[ inc ];
            SrcPtr[-inc2] = ( ap && !Chro ) ? ( L1 + L1 + L0 + R0 + 2 ) >> 2 : SrcPtr[-inc2];
          }
          else  //Strng == 2
          {
            C0  =  CLIP_TAB[Clip3 ( 0, 63, QP + input->alpha_c_offset ) ]; // jlzheng 7.12
            dif = IClip ( -C0, C0, ( ( R0 - L0 ) * 3 + ( L1 - R1 ) + 4 ) >> 3 ) ;
            SrcPtr[  -inc ] = IClip ( 0, 255, L0 + dif ) ;
            SrcPtr[     0 ] = IClip ( 0, 255, R0 - dif ) ;

            if ( !Chro )
            {
              L0 = SrcPtr[-inc] ;
              R0 = SrcPtr[   0] ;

              if ( ap )
              {
                dif  = IClip ( -C0, C0, ( ( L0 - L1 ) * 3 + ( L2 - R0 ) + 4 ) >> 3 ) ;
                SrcPtr[-inc2] = IClip ( 0, 255, L1 + dif ) ;
              }

              if ( aq )
              {
                dif  = IClip ( -C0, C0, ( ( R1 - R0 ) * 3 + ( L0 - R2 ) + 4 ) >> 3 ) ;
                SrcPtr[ inc ] = IClip ( 0, 255, R1 - dif ) ;
              }
            }
          }
        }
      }

      SrcPtr += PtrInc ;
      pel    += Chro ;
    }
    else                     
    {
      SrcPtr += PtrInc << ( 3 - Chro ) ;
      pel    += 7 ;
    }  ;
  }
}

/*
*************************************************************************
* Function: EdgeLoopX
* Input:  byte* SrcPtr,int QP, int dir,int width,int Chro
* Output: void
* Return: void
*************************************************************************
*/
void EdgeLoopX ( byte* SrcPtr, int QP, int dir, int width, int Chro, codingUnit* MbP, codingUnit* MbQ, int block_y, int block_x )
{

  int     pel , PtrInc;
  int     inc, inc2, inc3;
  int     AbsDelta ;
  int     L2, L1, L0, R0, R1, R2 ;
  int     fs; //fs stands for filtering strength.  The larger fs is, the stronger filter is applied.
  int     Alpha, Beta ;
  int     FlatnessL, FlatnessR;
#if EXTEND_BD
	int  shift1 = input->sample_bit_depth - 8;
#endif
  int skipFilteringFlag = 0;
  // FlatnessL and FlatnessR describe how flat the curve is of one codingUnit.

  PtrInc  = dir ?  1 : width ;
  inc     = dir ?  width : 1 ;
  inc2    = inc << 1 ;
  inc3    = inc + inc2 ;

#if EXTEND_BD
	if (input->sample_bit_depth > 8) // coded as 10/12 bit, QP is added by (8 * (input->sample_bit_depth - 8)) in config file
	{
		Alpha =  ALPHA_TABLE[Clip3 ( 0, 63, QP - (8 * (input->sample_bit_depth - 8)) + input->alpha_c_offset ) ]; // jlzheng 7.8
		Beta =  BETA_TABLE[Clip3 ( 0, 63, QP - (8 * (input->sample_bit_depth - 8)) + input->beta_offset ) ]; // jlzheng 7.8
	}
	else // coded as 8 bit
	{
#endif
		Alpha =  ALPHA_TABLE[Clip3 ( 0, 63, QP + input->alpha_c_offset ) ]; // jlzheng 7.8
		Beta =  BETA_TABLE[Clip3 ( 0, 63, QP + input->beta_offset ) ]; // jlzheng 7.8
#if EXTEND_BD
	}

	Alpha = Alpha << shift1;
	Beta = Beta << shift1;
#endif

#if M3198_CU8//???
  for ( pel = 0; pel < MIN_CU_SIZE ; pel++ )
#else
  for ( pel = 0; pel < 16 ; pel++ )
#endif
  {
    if ( pel %4 == 0 )
    {
      skipFilteringFlag = SkipFiltering ( MbP, MbQ, dir, 0, block_y+!dir*(pel>>2), block_x+dir*(pel>>2) );
    }
    L2  = SrcPtr[-inc3] ;
    L1  = SrcPtr[-inc2] ;
    L0  = SrcPtr[-inc ] ;
    R0  = SrcPtr[    0] ;
    R1  = SrcPtr[ inc ] ;
    R2  = SrcPtr[ inc2] ;


    AbsDelta = abs ( R0 - L0 ) ;
    if ( !skipFilteringFlag && ( AbsDelta < Alpha ) && ( AbsDelta > 1 ) )
    {
      if ( abs ( L1 - L0 ) < Beta )
      {
        FlatnessL = 2;
      }
      else
      {
        FlatnessL = 0;
      }

      if ( abs ( L2 - L0 ) < Beta )
      {
        FlatnessL += 1;
      }


      if ( abs ( R0 - R1 ) < Beta )
      {
        FlatnessR = 2;
      }
      else
      {
        FlatnessR = 0;
      }

      if ( abs ( R0 - R2 ) < Beta )
      {
        FlatnessR += 1;
      }

      switch ( FlatnessL + FlatnessR )
      {
      case 6:

        if ( ( R1 == R0 ) && ( ( L0 == L1 ) ) )
        {
          fs = 4;
        }
        else
        {
          fs = 3;
        }

        break;
      case 5:

        if ( ( R1 == R0 ) && ( ( L0 == L1 ) ) )
        {
          fs = 3;
        }
        else
        {
          fs = 2;
        }

        break;
      case 4:

        if ( FlatnessL == 2 )
        {
          fs = 2;
        }
        else
        {
          fs = 1;
        }

        break;

      case 3:

        if ( abs ( L1 - R1 ) < Beta )
        {
          fs = 1;
        }
        else
        {
          fs = 0;
        }

        break;
      default:
        fs = 0;
      }

#if M3198_CU8
      if (Chro && fs>0)
      {
        fs--;
      }
#else
      fs -= Chro;
#endif

      switch ( fs )
      {
      case 4:


        SrcPtr[-inc]  = ( L0 + ( ( L0 + L2 ) << 3 ) + L2 + ( R0 << 3 ) + ( R2 << 2 ) + ( R2 << 1 ) + 16 ) >> 5; //L0
        SrcPtr[-inc2] = ( ( L0 << 3 ) - L0  + ( L2 << 2 ) + ( L2 << 1 ) + R0 + ( R0 << 1 ) + 8 ) >> 4; //L1
        SrcPtr[-inc3] = ( ( L0 << 2 ) + L2 + ( L2 << 1 ) + R0 + 4 ) >> 3; //L2

        SrcPtr[0]     = ( R0 + ( ( R0 + R2 ) << 3 ) + R2 + ( L0 << 3 ) + ( L2 << 2 ) + ( L2 << 1 ) + 16 ) >> 5; //R0
        SrcPtr[inc]   = ( ( R0 << 3 ) - R0  + ( R2 << 2 ) + ( R2 << 1 ) + L0 + ( L0 << 1 ) + 8 ) >> 4; //R1
        SrcPtr[inc2]  = ( ( R0 << 2 ) + R2 + ( R2 << 1 ) + L0 + 4 ) >> 3; //R2

        break;
      case 3:


        SrcPtr[-inc] = ( L2 + ( L1 << 2 )  + ( L0 << 2 ) + ( L0 << 1 ) + ( R0 << 2 ) + R1 + 8 ) >> 4; //L0
        SrcPtr[0] = ( L1 + ( L0 << 2 ) + ( R0 << 2 ) + ( R0 << 1 ) + ( R1 << 2 ) + R2 + 8 ) >> 4; //R0


        SrcPtr[-inc2] = ( L2 * 3 + L1 * 8 + L0 * 4 + R0 + 8 ) >> 4;
        SrcPtr[inc] = ( R2 * 3 + R1 * 8 + R0 * 4 + L0 + 8 ) >> 4;

        break;
      case 2:

        SrcPtr[-inc] = ( ( L1 << 1 ) + L1 + ( L0 << 3 ) + ( L0 << 1 ) + ( R0 << 1 ) + R0 + 8 ) >> 4;
        SrcPtr[0] = ( ( L0 << 1 ) + L0 + ( R0 << 3 ) + ( R0 << 1 ) + ( R1 << 1 ) + R1  + 8 ) >> 4;

        break;
      case 1:

        SrcPtr[-inc] = ( L0 * 3 + R0 + 2 ) >> 2;
        SrcPtr[0] = ( R0 * 3 + L0 + 2 ) >> 2;

        break;
      default:
        break;
      }

      SrcPtr += PtrInc ;    // Next row or column
      pel    += Chro ;

    }
    else
    {
      SrcPtr += PtrInc ;
      pel    += Chro ;

    }
  }
}

/*
*************************************************************************
* Function:  SkipFiltering
* Input:  codingUnit* MbP,codingUnit* MbQ,int dir,int edge,int block_y,int block_x
* Output: integer flag
* Return: 1 if skip filtering is needed.
*************************************************************************
*/
int SkipFiltering ( codingUnit* MbP, codingUnit* MbQ, int dir, int edge, int block_y, int block_x )
{
  int    blk_x2, blk_y2, blk_x, blk_y ;
  int    ***tmp_mv   =  img->tmp_mv;
  int    iFlagSkipFiltering;

#if !M3198_CU8
  blkQ = BLK_NUM[dir][edge][0];
  blk_y = ( block_y + ( blkQ >> 2 ) ) >> 1;
  blk_x = ( ( block_x + ( blkQ & 3 ) ) >> 1 );
#endif
  blk_y=block_y;
  blk_x=block_x;
  blk_y2 = blk_y -  dir ;
  blk_x2 = blk_x - !dir ;

  switch ( img->type )
  {
  case INTER_IMG:
  case F_IMG:
#if M3198_CU8
    if ( ( MbP->cbp == 0 ) && ( MbQ->cbp == 0 ) &&
      ( abs ( tmp_mv[blk_y][blk_x][0] -   tmp_mv[blk_y2][blk_x2][0] ) < 4 ) &&
      ( abs ( tmp_mv[blk_y][blk_x][1] -   tmp_mv[blk_y2][blk_x2][1] ) < 4 ) &&
      ( refFrArr [blk_y][blk_x] ==   refFrArr[blk_y2][blk_x2] ) && refFrArr [blk_y][blk_x] != -1 )
#else
    if ( ( ( MbQ->cbp == 0 ) && ( edge ) )
         || ( ( MbP->cbp == 0 ) && ( MbQ->cbp == 0 ) && ( !edge ) &&
              ( abs ( tmp_mv[blk_y][blk_x][0] -   tmp_mv[blk_y2][blk_x2][0] ) < 4 ) &&
              ( abs ( tmp_mv[blk_y][blk_x][1] -   tmp_mv[blk_y2][blk_x2][1] ) < 4 ) &&
              ( refFrArr [blk_y][blk_x] ==   refFrArr[blk_y2][blk_x2] ) ) )
#endif


    {
      iFlagSkipFiltering = 1;
    }
    else
    {
      iFlagSkipFiltering = 0;
    }

    break;
  default:
    iFlagSkipFiltering = 0;
  }

  return iFlagSkipFiltering;

}
//////////////////////////////////////////////////////////////////////////////////////////
//SAO functions
void init_sao_entropy()
{
	cs_sao_init = create_coding_state ();
	cs_sao_cur_blk = create_coding_state ();
	cs_sao_next_blk = create_coding_state ();
	//   cs_sao_modenew = create_coding_state (); 
	cs_sao_cur_type = create_coding_state ();
	cs_sao_next_type = create_coding_state (); 
	cs_sao_cur_mergetype = create_coding_state (); 
	cs_sao_next_mergetype = create_coding_state (); 
}
void clear_sao_entropy()
{
	delete_coding_state(cs_sao_init );
	delete_coding_state(cs_sao_cur_blk );
	delete_coding_state(cs_sao_next_blk); 
	//  delete_coding_state( cs_sao_modenew );
	delete_coding_state(cs_sao_cur_type);
	delete_coding_state( cs_sao_next_type); 
	delete_coding_state(cs_sao_cur_mergetype);
	delete_coding_state( cs_sao_next_mergetype); 
}
void init_StateDate(SAOStatData* statsDate)
{
	int i;
	for( i = 0; i < MAX_NUM_SAO_CLASSES; i++)
	{
		statsDate->diff[i] = 0;
		statsDate->count[i] = 0;
	}
}

void getStatisticsSAO_one_SMB(int smb_index, int mb_y, int mb_x, int smb_mb_height, int smb_mb_width, SAOStatData** saostatData)
{

	int pix_x = mb_x << MIN_CU_SIZE_IN_BIT;
	int pix_y = mb_y << MIN_CU_SIZE_IN_BIT;
	int smb_pix_width = smb_mb_width << MIN_CU_SIZE_IN_BIT;
	int smb_pix_height = smb_mb_height << MIN_CU_SIZE_IN_BIT;
	int compIdx;
	int smb_pix_height_t, smb_pix_width_t, pix_x_t, pix_y_t;
	SAOStatData* statsData;
	int isLeftAvail,isRightAvail,isAboveAvail,isBelowAvail,isAboveLeftAvail,isAboveRightAvail,isBelowLeftAvail,isBelowRightAvail;
	int type;
#if SAO_SLICE_BOUND_PRO
	int  isLeftProc, isRightProc, isAboveProc, isBelowProc, isAboveLeftProc, isAboveRightProc, isBelowLeftProc, isBelowRightProc;
#endif

#if SAO_SLICE_BOUND_PRO
#if !SAO_MULSLICE_FIX	
	 checkBoundaryProc(mb_y, mb_x, smb_pix_height, smb_pix_width, input->slice_set_enable, &isLeftProc, &isRightProc, &isAboveProc, &isBelowProc, &isAboveLeftProc, &isAboveRightProc, &isBelowLeftProc, &isBelowRightProc, 0);
#endif
	 checkBoundaryPara(mb_y, mb_x, smb_pix_height, smb_pix_width, input->slice_set_enable, &isLeftAvail, &isRightAvail, &isAboveAvail, &isBelowAvail, &isAboveLeftAvail, &isAboveRightAvail, &isBelowLeftAvail, &isBelowRightAvail, 0);
#else
	checkBoundaryAvail(mb_y, mb_x, smb_pix_height, smb_pix_width, input->slice_set_enable, &isLeftAvail, &isRightAvail, &isAboveAvail, &isBelowAvail, &isAboveLeftAvail, &isAboveRightAvail, &isBelowLeftAvail, &isBelowRightAvail, 1);
#endif
	for ( compIdx = 0; compIdx < 3; compIdx ++)
	{
		for (type = 0; type < NUM_SAO_NEW_TYPES; type++)
		{
			statsData = &(saostatData[compIdx][type]);
			init_StateDate(statsData);
		}
		statsData = saostatData[compIdx];
		smb_pix_width_t =  compIdx ? ((smb_pix_width >> 1) - SAO_SHIFT_PIX_NUM ) : ( smb_pix_width - SAO_SHIFT_PIX_NUM );
		smb_pix_height_t =  compIdx ? ((smb_pix_height >> 1) - SAO_SHIFT_PIX_NUM ) : ( smb_pix_height - SAO_SHIFT_PIX_NUM );
		pix_x_t = compIdx ? (pix_x >> 1 ) : pix_x ;
		pix_y_t = compIdx ? (pix_y >> 1 ) : pix_y;
#if SAO_SLICE_BOUND_PRO
#if SAO_MULSLICE_FIX
		checkBoundaryProc(pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, compIdx, input->slice_set_enable, &isLeftProc, &isRightProc, &isAboveProc, &isBelowProc, &isAboveLeftProc, &isAboveRightProc, &isBelowLeftProc, &isBelowRightProc, 1);
		getStatblk(statsData, compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftProc /*Left*/, isRightProc/*Right*/, isAboveProc/*Above*/, isBelowProc/*Below*/, isAboveLeftProc/*AboveLeft*/, isAboveRightProc/*AboveRight*/, isBelowLeftProc/*BelowLeft*/, isBelowRightProc/*BelowRight*/);
#else
		getStatblk(statsData, compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftProc /*Left*/, 1/*Right*/, isAboveProc/*Above*/, 1/*Below*/,isAboveLeftProc/*AboveLeft*/, isAboveProc/*AboveRight*/, isLeftProc/*BelowLeft*/, 1/*BelowRight*/);
#endif
#else
		getStatblk(statsData, compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftAvail /*Left*/, 1/*Right*/, isAboveAvail/*Above*/, 1/*Below*/,isAboveLeftAvail/*AboveLeft*/, isAboveAvail/*AboveRight*/, isLeftAvail/*BelowLeft*/, 1/*BelowRight*/);
#endif
		if (isAboveLeftAvail)
		{
			smb_pix_width_t = SAO_SHIFT_PIX_NUM;
			smb_pix_height_t = SAO_SHIFT_PIX_NUM;
			pix_x_t = compIdx ? ((pix_x >> 1) - SAO_SHIFT_PIX_NUM ) : ( pix_x - SAO_SHIFT_PIX_NUM );
			pix_y_t = compIdx ? ((pix_y >> 1) - SAO_SHIFT_PIX_NUM ) : ( pix_y - SAO_SHIFT_PIX_NUM );
			assert(isAboveAvail && isLeftAvail);
#if SAO_SLICE_BOUND_PRO
#if SAO_MULSLICE_FIX	
			checkBoundaryProc(pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, compIdx, input->slice_set_enable, &isLeftProc, &isRightProc, &isAboveProc, &isBelowProc, &isAboveLeftProc, &isAboveRightProc, &isBelowLeftProc, &isBelowRightProc, 1);
			getStatblk(statsData, compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftProc /*Left*/, isRightProc/*Right*/, isAboveProc/*Above*/, isBelowProc/*Below*/, isAboveLeftProc/*AboveLeft*/, isAboveRightProc/*AboveRight*/, isBelowLeftProc/*BelowLeft*/, isBelowRightProc/*BelowRight*/);
#else
			getStatblk(statsData, compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isAboveLeftProc /*Left*/, isAboveProc/*Right*/, isAboveLeftProc/*Above*/, isLeftProc/*Below*/,isAboveLeftProc/*AboveLeft*/, isAboveProc/*AboveRight*/, isLeftProc/*BelowLeft*/, 1/*BelowRight*/);
#endif
#else
			getStatblk(statsData, compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isAboveLeftAvail /*Left*/, isAboveAvail/*Right*/, isAboveLeftAvail/*Above*/, isLeftAvail/*Below*/,isAboveLeftAvail/*AboveLeft*/, isAboveAvail/*AboveRight*/, isLeftAvail/*BelowLeft*/, 1/*BelowRight*/);
#endif
		}
		
		if (isLeftAvail)
		{
			smb_pix_width_t = SAO_SHIFT_PIX_NUM;
			smb_pix_height_t = compIdx ? ((smb_pix_height >> 1) - SAO_SHIFT_PIX_NUM ) : ( smb_pix_height - SAO_SHIFT_PIX_NUM );
			pix_x_t = compIdx ? ((pix_x >> 1) - SAO_SHIFT_PIX_NUM ) : ( pix_x - SAO_SHIFT_PIX_NUM );
			pix_y_t = compIdx ? (pix_y >> 1) : pix_y ;
#if SAO_SLICE_BOUND_PRO
#if SAO_MULSLICE_FIX
			checkBoundaryProc(pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, compIdx, input->slice_set_enable, &isLeftProc, &isRightProc, &isAboveProc, &isBelowProc, &isAboveLeftProc, &isAboveRightProc, &isBelowLeftProc, &isBelowRightProc, 1);
			getStatblk(statsData, compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftProc /*Left*/, isRightProc/*Right*/, isAboveProc/*Above*/, isBelowProc/*Below*/, isAboveLeftProc/*AboveLeft*/, isAboveRightProc/*AboveRight*/, isBelowLeftProc/*BelowLeft*/, isBelowRightProc/*BelowRight*/);
#else
			getStatblk(statsData, compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftProc /*Left*/, 1/*Right*/, isAboveLeftProc/*Above*/, isLeftProc/*Below*/,isAboveLeftProc/*AboveLeft*/, isAboveProc/*AboveRight*/, isLeftProc/*BelowLeft*/, 1/*BelowRight*/);
#endif
#else
			getStatblk(statsData, compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftAvail /*Left*/, 1/*Right*/, isAboveLeftAvail/*Above*/, isLeftAvail/*Below*/,isAboveLeftAvail/*AboveLeft*/, isAboveAvail/*AboveRight*/, isLeftAvail/*BelowLeft*/, 1/*BelowRight*/);
#endif
		}
		if (isAboveAvail)
		{
			smb_pix_width_t =compIdx ? ((smb_pix_width >> 1) - SAO_SHIFT_PIX_NUM ) : ( smb_pix_width - SAO_SHIFT_PIX_NUM );
			smb_pix_height_t = SAO_SHIFT_PIX_NUM;
			pix_x_t = compIdx ? (pix_x >> 1) : pix_x;
			pix_y_t = compIdx ? ((pix_y >> 1) - SAO_SHIFT_PIX_NUM ) : ( pix_y - SAO_SHIFT_PIX_NUM );
#if SAO_SLICE_BOUND_PRO
#if SAO_MULSLICE_FIX
			checkBoundaryProc(pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, compIdx, input->slice_set_enable, &isLeftProc, &isRightProc, &isAboveProc, &isBelowProc, &isAboveLeftProc, &isAboveRightProc, &isBelowLeftProc, &isBelowRightProc, 1);
			getStatblk(statsData, compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftProc /*Left*/, isRightProc/*Right*/, isAboveProc/*Above*/, isBelowProc/*Below*/, isAboveLeftProc/*AboveLeft*/, isAboveRightProc/*AboveRight*/, isBelowLeftProc/*BelowLeft*/, isBelowRightProc/*BelowRight*/);
#else
			getStatblk(statsData, compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isAboveLeftProc /*Left*/, isAboveProc/*Right*/, isAboveProc/*Above*/, 1/*Below*/,isAboveLeftProc/*AboveLeft*/, isAboveProc/*AboveRight*/, isLeftProc/*BelowLeft*/, 1/*BelowRight*/);
#endif
#else
			getStatblk(statsData, compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isAboveLeftAvail /*Left*/, isAboveAvail/*Right*/, isAboveAvail/*Above*/, 1/*Below*/,isAboveLeftAvail/*AboveLeft*/, isAboveAvail/*AboveRight*/, isLeftAvail/*BelowLeft*/, 1/*BelowRight*/);
#endif
		}
#if 1
		if (!isRightAvail)
		{
			
			if (isAboveAvail)
			{
				smb_pix_width_t = SAO_SHIFT_PIX_NUM;
				smb_pix_height_t = SAO_SHIFT_PIX_NUM;
				pix_x_t = compIdx ? ((pix_x >> 1) + (smb_pix_width >> 1) - SAO_SHIFT_PIX_NUM  ) : ( pix_x + smb_pix_width - SAO_SHIFT_PIX_NUM );
				pix_y_t = compIdx ? ((pix_y >> 1) - SAO_SHIFT_PIX_NUM ) : ( pix_y - SAO_SHIFT_PIX_NUM );
#if SAO_SLICE_BOUND_PRO
#if SAO_MULSLICE_FIX
				checkBoundaryProc(pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, compIdx, input->slice_set_enable, &isLeftProc, &isRightProc, &isAboveProc, &isBelowProc, &isAboveLeftProc, &isAboveRightProc, &isBelowLeftProc, &isBelowRightProc, 1);
				getStatblk(statsData, compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftProc /*Left*/, isRightProc/*Right*/, isAboveProc/*Above*/, isBelowProc/*Below*/, isAboveLeftProc/*AboveLeft*/, isAboveRightProc/*AboveRight*/, isBelowLeftProc/*BelowLeft*/, 0/*BelowRight is forced to 0*/);
#else
				getStatblk(statsData, compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isAboveProc /*Left*/, isAboveRightProc/*Right*/, isAboveProc/*Above*/, 1/*Below*/,isAboveProc/*AboveLeft*/, isAboveRightProc/*AboveRight*/, 1/*BelowLeft*/, isRightProc/*BelowRight*/);
#endif
#else
				getStatblk(statsData, compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isAboveAvail /*Left*/, isAboveRightAvail/*Right*/, isAboveAvail/*Above*/, 1/*Below*/,isAboveAvail/*AboveLeft*/, isAboveRightAvail/*AboveRight*/, 1/*BelowLeft*/, isRightAvail/*BelowRight*/);
#endif
			}
		
			smb_pix_width_t = SAO_SHIFT_PIX_NUM;
			smb_pix_height_t = compIdx ? ((smb_pix_height >> 1) - SAO_SHIFT_PIX_NUM ) : ( smb_pix_height - SAO_SHIFT_PIX_NUM );
			pix_x_t = compIdx ? ((pix_x >> 1) + (smb_pix_width >> 1) - SAO_SHIFT_PIX_NUM  ) : ( pix_x + smb_pix_width - SAO_SHIFT_PIX_NUM );
			pix_y_t = compIdx ? (pix_y >> 1) : pix_y ;
#if SAO_SLICE_BOUND_PRO
#if SAO_MULSLICE_FIX
			checkBoundaryProc(pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, compIdx, input->slice_set_enable, &isLeftProc, &isRightProc, &isAboveProc, &isBelowProc, &isAboveLeftProc, &isAboveRightProc, &isBelowLeftProc, &isBelowRightProc, 1);
			getStatblk(statsData, compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftProc /*Left*/, 0/*Right is forced to 0*/, isAboveProc/*Above*/, isBelowProc/*Below*/, isAboveLeftProc/*AboveLeft*/, isAboveRightProc/*AboveRight*/, isBelowLeftProc/*BelowLeft*/, 0/*BelowRight is forced to 0*/);
#else
			getStatblk(statsData, compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, 1 /*Left*/, isRightProc/*Right*/, isAboveProc/*Above*/, 1/*Below*/,isAboveProc/*AboveLeft*/, isAboveRightProc/*AboveRight*/, 1/*BelowLeft*/,isRightProc/*BelowRight*/);
#endif
#else
			getStatblk(statsData, compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, 1 /*Left*/, isRightAvail/*Right*/, isAboveAvail/*Above*/, 1/*Below*/,isAboveAvail/*AboveLeft*/, isAboveRightAvail/*AboveRight*/, 1/*BelowLeft*/,isRightAvail/*BelowRight*/);
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
#if SAO_MULSLICE_FIX
				checkBoundaryProc(pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, compIdx, input->slice_set_enable, &isLeftProc, &isRightProc, &isAboveProc, &isBelowProc, &isAboveLeftProc, &isAboveRightProc, &isBelowLeftProc, &isBelowRightProc, 1);
				getStatblk(statsData, compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftProc /*Left*/, isRightProc/*Right*/, isAboveProc/*Above*/, 0/*Below is forced to 0*/, isAboveLeftProc/*AboveLeft*/, isAboveRightProc/*AboveRight*/, 0/*BelowLeft is forced to 0*/, 0/*BelowRight is forced to 0*/);
#else
				getStatblk(statsData, compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftProc /*Left*/, 1/*Right*/, isLeftProc/*Above*/, isBelowLeftProc/*Below*/,isLeftProc/*AboveLeft*/, 1/*AboveRight*/, isBelowLeftProc/*BelowLeft*/, isBelowProc/*BelowRight*/);
#endif
#else
			getStatblk(statsData, compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftAvail /*Left*/, 1/*Right*/, isLeftAvail/*Above*/, isBelowLeftAvail/*Below*/,isLeftAvail/*AboveLeft*/, 1/*AboveRight*/, isBelowLeftAvail/*BelowLeft*/, isBelowAvail/*BelowRight*/);
#endif
			}

			
			smb_pix_width_t =compIdx ? ((smb_pix_width >> 1) - SAO_SHIFT_PIX_NUM ) : ( smb_pix_width - SAO_SHIFT_PIX_NUM );
			smb_pix_height_t = SAO_SHIFT_PIX_NUM;
			pix_x_t = compIdx ? (pix_x >> 1) :  pix_x;
			pix_y_t = compIdx ? ((pix_y >> 1) + (smb_pix_height >> 1) - SAO_SHIFT_PIX_NUM  ) : ( pix_y + smb_pix_height - SAO_SHIFT_PIX_NUM );
#if SAO_SLICE_BOUND_PRO
#if SAO_MULSLICE_FIX
			checkBoundaryProc(pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, compIdx, input->slice_set_enable, &isLeftProc, &isRightProc, &isAboveProc, &isBelowProc, &isAboveLeftProc, &isAboveRightProc, &isBelowLeftProc, &isBelowRightProc, 1);
			getStatblk(statsData, compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftProc /*Left*/, isRightProc/*Right*/, isAboveProc/*Above*/, 0/*Below is forced to 0*/, isAboveLeftProc/*AboveLeft*/, isAboveRightProc/*AboveRight*/, 0/*BelowLeft is forced to 0*/, 0/*BelowRight is forced to 0*/);
#else
			getStatblk(statsData, compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftProc /*Left*/, 1/*Right*/, 1/*Above*/, isBelowProc/*Below*/,isLeftProc/*AboveLeft*/, 1/*AboveRight*/, isBelowLeftProc/*BelowLeft*/, isBelowProc/*BelowRight*/);
#endif
#else
			getStatblk(statsData, compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftAvail /*Left*/, 1/*Right*/, 1/*Above*/, isBelowAvail/*Below*/,isLeftAvail/*AboveLeft*/, 1/*AboveRight*/, isBelowLeftAvail/*BelowLeft*/, isBelowAvail/*BelowRight*/);
#endif
		}
		if (!isBelowRightAvail && !isRightAvail && !isBelowAvail)
		{	
			smb_pix_width_t = SAO_SHIFT_PIX_NUM;
			smb_pix_height_t = SAO_SHIFT_PIX_NUM;
			pix_x_t = compIdx ? ((pix_x >> 1) + (smb_pix_width >> 1) - SAO_SHIFT_PIX_NUM  ) : ( pix_x + smb_pix_width - SAO_SHIFT_PIX_NUM );
			pix_y_t = compIdx ? ((pix_y >> 1) + (smb_pix_height >> 1) - SAO_SHIFT_PIX_NUM  ) : ( pix_y + smb_pix_height - SAO_SHIFT_PIX_NUM );
#if SAO_SLICE_BOUND_PRO
#if SAO_MULSLICE_FIX
			checkBoundaryProc(pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, compIdx, input->slice_set_enable, &isLeftProc, &isRightProc, &isAboveProc, &isBelowProc, &isAboveLeftProc, &isAboveRightProc, &isBelowLeftProc, &isBelowRightProc, 0);
			getStatblk(statsData, compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, isLeftProc /*Left*/, isRightProc/*Right*/, isAboveProc/*Above*/, isBelowProc/*Below*/, isAboveLeftProc/*AboveLeft*/, 0/*AboveRight is forced to 0*/, isBelowLeftProc/*BelowLeft*/, isBelowRightProc/*BelowRight*/);
#else
			getStatblk(statsData, compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, 1 /*Left*/, isRightProc/*Right*/, 1/*Above*/, isBelowProc/*Below*/,1/*AboveLeft*/, isRightProc/*AboveRight*/, isBelowProc/*BelowLeft*/, isBelowRightProc/*BelowRight*/);
#endif
#else
			getStatblk(statsData, compIdx, smb_index, pix_y_t, pix_x_t, smb_pix_height_t, smb_pix_width_t, 1 /*Left*/, isRightAvail/*Right*/, 1/*Above*/, isBelowAvail/*Below*/,1/*AboveLeft*/, isRightAvail/*AboveRight*/, isBelowAvail/*BelowLeft*/, isBelowRightAvail/*BelowRight*/);
#endif
		}
#endif
	}
}

void getStatblk(SAOStatData* saostatsData, int compIdx, int smb, int pix_y, int pix_x, int smb_pix_height, int smb_pix_width,int smb_available_left, int smb_available_right, int smb_available_up, int smb_available_down, int smb_available_upleft, int smb_available_upright, int smb_available_leftdown, int smb_available_rightdwon)
{
	int type;
	int start_x, end_x, start_y, end_y;
	int start_x_r0, end_x_r0, start_x_r, end_x_r,  start_x_rn, end_x_rn;
	int x, y;
	byte **Rec, **Org;
	char leftsign, rightsign, upsign, downsign;
	long diff;
	SAOStatData* statsDate;
	char *signupline,*signupline1;
	int reg=0;
	int edgetype, bandtype;
	long int cnt[5]={0};
	
	if (compIdx == SAO_Y)
	{
		Rec = imgY_sao;
		Org = imgY_org;
	}
	else
	{
		Rec = imgUV_sao[compIdx - 1];
		Org = imgUV_org[compIdx - 1];
	}
	signupline = (char*) malloc((smb_pix_width+1) * sizeof(char));

	for (type = 0; type < NUM_SAO_NEW_TYPES; type++)
	{
		statsDate = &(saostatsData[type]);
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
					diff = Rec[pix_y  + y][pix_x  + start_x] - Rec[pix_y  + y][pix_x + start_x - 1];
					leftsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
					for ( x = start_x; x < end_x; x++)
					{
						diff = Rec[pix_y  + y][pix_x + x] - Rec[pix_y  + y][pix_x + x + 1];
						rightsign =  diff > 0 ? 1 : (diff < 0 ? -1 : 0);
						edgetype = leftsign + rightsign;
						leftsign = - rightsign;
						statsDate->diff[edgetype + 2] += (Org[pix_y + y][pix_x + x] - Rec[pix_y + y][pix_x + x]);
						statsDate->count[edgetype + 2]++;
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
					diff = Rec[pix_y  + start_y][pix_x  + x] - Rec[pix_y  + start_y - 1][pix_x + x];
					upsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
					for ( y = start_y; y < end_y; y++)
					{
						diff = Rec[pix_y + y][pix_x  + x] - Rec[pix_y + y + 1][pix_x + x ];
						downsign =  diff > 0 ? 1 : (diff < 0 ? -1 : 0);
						edgetype = downsign + upsign;
						upsign = - downsign;
						statsDate->diff[edgetype + 2] += ( Org[pix_y + y][pix_x + x] - Rec[pix_y + y][pix_x + x] );
						statsDate->count[edgetype + 2]++;
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
					diff = Rec[pix_y  + 1][pix_x  + x] - Rec[pix_y ][pix_x  + x - 1];
					upsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
					signupline[x] = upsign;
				}
				//first row
				for ( x = start_x_r0; x < end_x_r0; x++ )
				{
					diff = Rec[pix_y ][pix_x + x] - Rec[pix_y  - 1][pix_x + x - 1]; 
					upsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
					edgetype = upsign - signupline[x + 1];
					statsDate->diff[edgetype + 2] += ( Org[pix_y][pix_x + x] - Rec[pix_y ][pix_x + x] );
					statsDate->count[edgetype + 2]++;
				}

				//middle rows
				for ( y = 1 ; y < smb_pix_height - 1; y++)
				{
					for ( x = start_x_r; x < end_x_r; x++ )
					{
						if ( x == start_x_r )
						{
							diff = Rec[pix_y  + y][pix_x + x] - Rec[pix_y  + y - 1][pix_x + x - 1];  
							upsign =  diff > 0 ? 1 : (diff < 0 ? -1 : 0);
							signupline[x] = upsign;
						}
						diff = Rec[pix_y + y][pix_x + x] - Rec[pix_y  + y + 1][pix_x + x + 1];  
						downsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
						edgetype = downsign + signupline[x];
						statsDate->diff[edgetype + 2] += ( Org[pix_y + y][pix_x + x] - Rec[pix_y  + y][pix_x + x] );
						statsDate->count[edgetype + 2]++;
						signupline[x] = reg;
						reg = -downsign;
					}
				}
				//last row
				for ( x = start_x_rn; x < end_x_rn; x++)
				{
					if ( x == start_x_r )
					{
						diff = Rec[pix_y + smb_pix_height - 1][pix_x + x] - Rec[pix_y + smb_pix_height - 2][pix_x + x - 1];  
						upsign =  diff > 0 ? 1 : (diff < 0 ? -1 : 0);
						signupline[x] = upsign;
					}
					diff = Rec[pix_y  + smb_pix_height - 1][pix_x  + x] - Rec[pix_y  + smb_pix_height ][pix_x  + x + 1];  
					downsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
					edgetype = downsign + signupline[x];
					statsDate->diff[edgetype + 2] += ( Org[pix_y + smb_pix_height - 1][pix_x + x] - Rec[pix_y  + smb_pix_height - 1][pix_x  + x] );
					statsDate->count[edgetype + 2]++;
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
				for ( x = start_x_r - 1; x < max(end_x_r - 1,end_x_r0 - 1)  ; x++)
					//for ( x = start_x_r - 1; x < end_x_r - 1  ; x++)
				{
					diff = Rec[pix_y  + 1][pix_x + x] - Rec[pix_y][pix_x + x + 1];
					upsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
					signupline1[x] = upsign;
				}
				//first row
				for ( x = start_x_r0; x < end_x_r0; x++ )
				{
					diff = Rec[pix_y ][pix_x + x] - Rec[pix_y  - 1][pix_x  + x + 1]; 
					upsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
					edgetype = upsign - signupline1[x - 1];
					statsDate->diff[edgetype + 2] += ( Org[pix_y][pix_x + x] - Rec[pix_y][pix_x  + x] );
					statsDate->count[edgetype + 2]++;
				}

				//middle rows
				for ( y = 1 ; y < smb_pix_height - 1; y++)
				{
					for ( x = start_x_r; x < end_x_r; x++ )
					{
						if ( x == end_x_r - 1 )
						{
							diff = Rec[pix_y  + y][pix_x  + x] - Rec[pix_y  + y - 1][pix_x + x + 1];  
							upsign =  diff > 0 ? 1 : (diff < 0 ? -1 : 0);
							signupline1[x] = upsign;
						}
						diff = Rec[pix_y + y][pix_x  + x] - Rec[pix_y + y + 1][pix_x + x - 1];  
						downsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
						edgetype = downsign + signupline1[x];
						statsDate->diff[edgetype + 2] += ( Org[pix_y  + y][pix_x  + x]  - Rec[pix_y  + y][pix_x  + x] );
						statsDate->count[edgetype + 2]++;
						signupline1[x - 1] = -downsign;
					}
				}
				for ( x = start_x_rn; x < end_x_rn; x++)
				{
					if ( x == end_x_r - 1 )
					{
						diff = Rec[pix_y + smb_pix_height - 1] [pix_x + x] - Rec[pix_y + smb_pix_height - 2][pix_x + x + 1];  
						upsign =  diff > 0 ? 1 : (diff < 0 ? -1 : 0);
						signupline1[x] = upsign;
					}
					diff = Rec[pix_y + smb_pix_height - 1][pix_x  + x] - Rec[pix_y + smb_pix_height ][pix_x + x - 1];  
					downsign = diff > 0 ? 1 : (diff < 0 ? -1 : 0);
					edgetype = downsign + signupline1[x];
					statsDate->diff[edgetype + 2] += ( Org[pix_y + smb_pix_height - 1][pix_x + x] - Rec[pix_y + smb_pix_height - 1][pix_x + x] );
					statsDate->count[edgetype + 2]++;
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
						bandtype = Rec[pix_y  + y][pix_x  + x] >> (input->sample_bit_depth - NUM_SAO_BO_CLASSES_IN_BIT);
#else
						bandtype = Rec[pix_y  + y][pix_x  + x] >> (8 - NUM_SAO_BO_CLASSES_IN_BIT);
#endif
						statsDate->diff[bandtype] += ( Org[pix_y + y][pix_x + x]  - Rec[pix_y + y][pix_x + x] );
						statsDate->count[bandtype]++;
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
	}
  free(signupline);
}



void slice_fastskipSAO(int *slice_sao_on, int* num_off_sao)
{
	int compIdx;
	for( compIdx = 0;  compIdx<NUM_SAO_COMPONENTS; compIdx++)
	{
		slice_sao_on[compIdx] = 1;
		if( saorate[0][compIdx] > ((compIdx==SAO_Y) ? SAO_RATE_THR : SAO_RATE_CHROMA_THR) )
		{
			slice_sao_on[compIdx] = 0;
		}
		num_off_sao[compIdx] = 0;
	}
}
void getParaSAO_one_SMB(int smb_index,int mb_y, int mb_x, int smb_mb_height, int smb_mb_width, int *slice_sao_on, SAOStatData** saostatData, SAOBlkParam* saoBlkParam, SAOBlkParam** rec_saoBlkParam, int* num_off_sao, double sao_labmda)
{
	int pix_x = mb_x << MIN_CU_SIZE_IN_BIT;
	int pix_y = mb_y << MIN_CU_SIZE_IN_BIT;
	int smb_pix_width = smb_mb_width << MIN_CU_SIZE_IN_BIT;
	int smb_pix_height = smb_mb_height << MIN_CU_SIZE_IN_BIT;
	int MergeLeftAvail, MergeUpAvail;
	double mcost,mincost;
	int mode,i;
	SAOBlkParam sao_cur_param[NUM_SAO_COMPONENTS];
	SAOBlkParam rec_sao_cur_param[NUM_SAO_COMPONENTS];

	if (!slice_sao_on[0] && !slice_sao_on[1] && !slice_sao_on[2])
	{
		off_sao(saoBlkParam);
		off_sao(rec_saoBlkParam[smb_index]);
		return;
	}

	store_coding_state(cs_sao_cur_blk);
	mincost = 1e30;
	for (mode = 0; mode < NUM_SAO_MODES; mode++)
	{
		switch( mode )
		{
		case SAO_MODE_OFF:
			{
				continue;
			}
			break;
		case SAO_MODE_MERGE:
			{
				mcost = SAORDCost_for_mode_merge(smb_index, pix_y, pix_x, smb_pix_width, smb_pix_height, slice_sao_on, sao_labmda,saostatData, rec_saoBlkParam,sao_cur_param, rec_sao_cur_param, &MergeLeftAvail, &MergeUpAvail);
			}
			break;
		case SAO_MODE_NEW:
			{
				mcost = SAORDCost_for_mode_new_YUV_sep(smb_index, pix_y, pix_x, smb_pix_width, smb_pix_height, MergeLeftAvail, MergeUpAvail, slice_sao_on, sao_labmda, saostatData, sao_cur_param,rec_sao_cur_param);
			}
			break;
		default:
			{
				printf("Not a supported SAO mode\n");
				assert(0);
				exit(-1);
			}  
		}
		if (mcost < mincost)
		{
			mincost = mcost;
			copySAOParam_for_blk(saoBlkParam,sao_cur_param);
			copySAOParam_for_blk(rec_saoBlkParam[smb_index],rec_sao_cur_param);
			store_coding_state(cs_sao_next_blk);
		}
		reset_coding_state(cs_sao_cur_blk);

	}
	reset_coding_state(cs_sao_cur_blk);
	for (i = 0; i < NUM_SAO_COMPONENTS; i++)
	{
		if (saoBlkParam[i].modeIdc == SAO_MODE_OFF)
		{
			num_off_sao[i]++;
		}
	}

}


double SAORDCost_for_mode_merge(int smb_index, int pix_y, int pix_x, int smb_pix_width, int smb_pix_height, int* slice_sao_on, double sao_labmda, SAOStatData** saostatData, SAOBlkParam** rec_saoBlkParam, SAOBlkParam* sao_cur_param, SAOBlkParam* rec_sao_cur_param, int* MergeLeftAvail, int* MergeUpAvail)
{
	SAOBlkParam merge_candidate[NUM_SAO_MERGE_TYPES][NUM_SAO_COMPONENTS];
	SAOBlkParam temp_sao_param[NUM_SAO_COMPONENTS];
	int mb_x = pix_x >> MIN_CU_SIZE_IN_BIT;
	int mb_y = pix_y >> MIN_CU_SIZE_IN_BIT;
	int mb = mb_y * img->PicWidthInMbs + mb_x;
	int merge_avail[NUM_SAO_MERGE_TYPES];
	int mergeIdx,compIdx;
	int type, mode;
	double curdist = 0;
	int currate;
	double curcost, mincost;
	mincost =1e30;
	getMergeNeighbor( smb_index, pix_y,  pix_x, smb_pix_width,smb_pix_height,input->g_uiMaxSizeInBit,input->slice_set_enable,rec_saoBlkParam, merge_avail, merge_candidate);

	*MergeLeftAvail = merge_avail[SAO_MERGE_LEFT];
	*MergeUpAvail = merge_avail[SAO_MERGE_ABOVE];

	store_coding_state(cs_sao_cur_mergetype);
	for (mergeIdx = 0; mergeIdx < NUM_SAO_MERGE_TYPES; mergeIdx++)
	{
		if (merge_avail[mergeIdx])
		{
			curdist = 0;  
			copySAOParam_for_blk(temp_sao_param, merge_candidate[mergeIdx]);
			for(compIdx=0; compIdx< NUM_SAO_COMPONENTS; compIdx++)
			{
				type = merge_candidate[mergeIdx][compIdx].typeIdc;
				mode = merge_candidate[mergeIdx][compIdx].modeIdc;
				temp_sao_param[compIdx].typeIdc = mergeIdx;
				temp_sao_param[compIdx].modeIdc = SAO_MODE_MERGE;

				if(mode!=SAO_MODE_OFF)
				{
#if SAO_ENC_FIX
					curdist += get_distortion(compIdx, type,saostatData, temp_sao_param)/sao_labmda;
#else
					curdist += (double)get_distortion(compIdx, type,saostatData, temp_sao_param)/sao_labmda;
#endif
				}
			}
			currate = write_sao_mergeflag( *MergeLeftAvail, *MergeUpAvail, temp_sao_param, mb);
			curcost = currate + curdist;
			if (curcost < mincost)
			{
				mincost = curcost ;
				copySAOParam_for_blk(sao_cur_param, temp_sao_param);
				copySAOParam_for_blk(rec_sao_cur_param, merge_candidate[mergeIdx]);
				store_coding_state(cs_sao_next_mergetype);
			}
		}
		reset_coding_state(cs_sao_cur_mergetype);
	}
	reset_coding_state(cs_sao_next_mergetype);
	return mincost;

}

double SAORDCost_for_mode_new(int smb_index, int pix_y, int pix_x, int smb_pix_width, int smb_pix_height, int MergeLeftAvail, int MergeUpAvail,int* slice_sao_on, double sao_lambda, SAOStatData** saostatData, SAOBlkParam* sao_cur_param, SAOBlkParam* rec_sao_cur_param )
{
	int compIdx;
	int type;
	long int mindist[NUM_SAO_COMPONENTS], curdist[NUM_SAO_COMPONENTS];
	int  minrate[NUM_SAO_COMPONENTS], currate[NUM_SAO_COMPONENTS];
	double curcost, mincost;
	double normmodecost;
	SAOBlkParam temp_sao_param[NUM_SAO_COMPONENTS];
	int mb_x = pix_x >> MIN_CU_SIZE_IN_BIT;
	int mb_y = pix_y >> MIN_CU_SIZE_IN_BIT;
	int mb = mb_y * img->PicWidthInMbs + mb_x;
	int offsetTh = 7;
	int mergeflag_rate;
#if SAO_ENC_FIX
	mergeflag_rate=0;
#endif
	sao_cur_param[SAO_Y].modeIdc = SAO_MODE_OFF;
	rec_sao_cur_param[SAO_Y].modeIdc = SAO_MODE_OFF;
	if (MergeLeftAvail + MergeUpAvail)
	{
		mergeflag_rate = write_sao_mergeflag( MergeLeftAvail, MergeUpAvail, &(sao_cur_param[SAO_Y]), mb);
	}
	store_coding_state(cs_sao_cur_type);

	compIdx = SAO_Y;
	/// for off mode
	sao_cur_param[compIdx].modeIdc = SAO_MODE_OFF;
	rec_sao_cur_param[SAO_Y].modeIdc = SAO_MODE_OFF;
	minrate[compIdx] = write_sao_mode(&(sao_cur_param[compIdx]),mb);
	mindist[compIdx] = 0;
	mincost = sao_lambda *((double)minrate[compIdx]);
	store_coding_state(cs_sao_next_type);
	reset_coding_state(cs_sao_cur_type);
	// for other normal mode
	if (slice_sao_on[compIdx])
	{
		for ( type = 0; type < NUM_SAO_NEW_TYPES; type++)
		{
			temp_sao_param[compIdx].modeIdc = SAO_MODE_NEW;
			temp_sao_param[compIdx].typeIdc = type;
			find_offset( compIdx, type, saostatData,  temp_sao_param, sao_lambda,offsetTh );
			curdist[compIdx] =get_distortion(compIdx, type, saostatData,temp_sao_param);
			// assert(curdist[compIdx]<=0);

			currate[compIdx] = write_sao_mode(&(temp_sao_param[compIdx]),mb);
			currate[compIdx] += write_sao_offset(&(temp_sao_param[compIdx]),mb,offsetTh);
			currate[compIdx] += write_sao_type(&(temp_sao_param[compIdx]),mb);

			curcost = (double)(curdist[compIdx]) + sao_lambda * ((double)currate[compIdx]);

			if (curcost < mincost)
			{
				mincost = curcost;
				minrate[compIdx] = currate[compIdx];
				mindist[compIdx] = curdist[compIdx];
				copySAOParam_for_blk_onecomponent(&sao_cur_param[compIdx], &temp_sao_param[compIdx]);
				copySAOParam_for_blk_onecomponent(&rec_sao_cur_param[compIdx], &temp_sao_param[compIdx]);
				store_coding_state(cs_sao_next_type);
			}
			reset_coding_state(cs_sao_cur_type);
		}
		reset_coding_state(cs_sao_next_type);
	}
	else
	{
		mindist[compIdx] = 0;
		minrate[compIdx] = 0;
	}

	store_coding_state(cs_sao_cur_type);
	compIdx = SAO_Cb;
	sao_cur_param[SAO_Cb].modeIdc = SAO_MODE_OFF;
	sao_cur_param[SAO_Cr].modeIdc = SAO_MODE_OFF;
	rec_sao_cur_param[SAO_Cb].modeIdc = SAO_MODE_OFF;
	rec_sao_cur_param[SAO_Cr].modeIdc = SAO_MODE_OFF;
	minrate[SAO_Cb] = write_sao_mode(&(sao_cur_param[compIdx]),mb);
	minrate[SAO_Cr] = 0;
	mindist[SAO_Cb] = mindist[SAO_Cr] = 0;
	mincost = sao_lambda *((double)(minrate[SAO_Cb] + minrate[SAO_Cr]));
	store_coding_state(cs_sao_next_type);
	reset_coding_state(cs_sao_cur_type);

	if (slice_sao_on[compIdx])
	{
		for ( type = 0;type < NUM_SAO_NEW_TYPES; type++ )
		{
			for (compIdx= SAO_Cb; compIdx< NUM_SAO_COMPONENTS; compIdx++ )
			{
				temp_sao_param[compIdx].modeIdc = SAO_MODE_NEW;
				temp_sao_param[compIdx].typeIdc = type;
				find_offset( compIdx, type, saostatData,  temp_sao_param, sao_lambda ,offsetTh);
				curdist[compIdx] = get_distortion(compIdx, type, saostatData,temp_sao_param);

			}
			assert(temp_sao_param[SAO_Cb].modeIdc == temp_sao_param[SAO_Cr].modeIdc);
			assert(temp_sao_param[SAO_Cb].typeIdc == temp_sao_param[SAO_Cr].typeIdc);
			currate[SAO_Cb] = write_sao_mode(&(temp_sao_param[SAO_Cb]),mb);
			currate[SAO_Cb] += write_sao_offset(&(temp_sao_param[SAO_Cb]),mb, offsetTh);
			currate[SAO_Cb] += write_sao_type(&(temp_sao_param[SAO_Cb]),mb);
			currate[SAO_Cr] = write_sao_offset(&(temp_sao_param[SAO_Cr]),mb, offsetTh);
			if (temp_sao_param[SAO_Cr].modeIdc == SAO_MODE_NEW && temp_sao_param[SAO_Cr].typeIdc == SAO_TYPE_BO)
			{
				currate[SAO_Cr] += write_sao_type(&(temp_sao_param[SAO_Cr]),mb);
			}


			curcost = (double)(curdist[SAO_Cb]+ curdist[SAO_Cr]) + sao_lambda*((double)(currate[SAO_Cb] + currate[SAO_Cr]));
			if (curcost < mincost)
			{
				mincost = curcost;
				for (compIdx= SAO_Cb; compIdx< NUM_SAO_COMPONENTS; compIdx++ )
				{
					mindist[compIdx] = curdist[compIdx];
					minrate[compIdx] = currate[compIdx];
					copySAOParam_for_blk_onecomponent(&(sao_cur_param[compIdx]), &(temp_sao_param[compIdx]));
					copySAOParam_for_blk_onecomponent(&(rec_sao_cur_param[compIdx]), &(temp_sao_param[compIdx]));
				}
				store_coding_state(cs_sao_next_type);
			}
			reset_coding_state(cs_sao_cur_type); 
		}
		reset_coding_state(cs_sao_next_type);
	}
	else
	{
		for (compIdx= SAO_Cb; compIdx< NUM_SAO_COMPONENTS; compIdx++ )
		{
			mindist[compIdx] = 0;
			minrate[compIdx] = 0;
		}

	}
	normmodecost = (double)(mindist[SAO_Y] + mindist[SAO_Cb] + mindist[SAO_Cr])/sao_lambda;
#if SAO_ENC_FIX
	normmodecost += minrate[SAO_Y] + minrate[SAO_Cb] + minrate[SAO_Cr] + mergeflag_rate;
#else
	normmodecost += minrate[SAO_Y] + minrate[SAO_Cb] + minrate[SAO_Cr];
#endif
	return normmodecost;

}
double SAORDCost_for_mode_new_YUV_sep(int smb_index, int pix_y, int pix_x, int smb_pix_width, int smb_pix_height, int MergeLeftAvail, int MergeUpAvail,int* slice_sao_on, double sao_lambda, SAOStatData** saostatData, SAOBlkParam* sao_cur_param, SAOBlkParam* rec_sao_cur_param )
{
	int compIdx;
	int type;
	long int mindist[NUM_SAO_COMPONENTS], curdist[NUM_SAO_COMPONENTS];
	int  minrate[NUM_SAO_COMPONENTS], currate[NUM_SAO_COMPONENTS];
	double curcost, mincost;
	double normmodecost;
	SAOBlkParam temp_sao_param[NUM_SAO_COMPONENTS];
	int mb_x = pix_x >> MIN_CU_SIZE_IN_BIT;
	int mb_y = pix_y >> MIN_CU_SIZE_IN_BIT;
	int mb = mb_y * img->PicWidthInMbs + mb_x;
	int offsetTh = 7;
	int mergeflag_rate;
#if SAO_ENC_FIX
	mergeflag_rate=0;
#endif
	sao_cur_param[SAO_Y].modeIdc = SAO_MODE_OFF;
	rec_sao_cur_param[SAO_Y].modeIdc = SAO_MODE_OFF;
	if (MergeLeftAvail + MergeUpAvail)
	{
		mergeflag_rate = write_sao_mergeflag( MergeLeftAvail, MergeUpAvail, &(sao_cur_param[SAO_Y]), mb);
	}


	for ( compIdx = 0; compIdx < NUM_SAO_COMPONENTS; compIdx++)
	{
		store_coding_state(cs_sao_cur_type);
		/// for off mode
		sao_cur_param[compIdx].modeIdc = SAO_MODE_OFF;
		rec_sao_cur_param[compIdx].modeIdc = SAO_MODE_OFF;
		minrate[compIdx] = write_sao_mode(&(sao_cur_param[compIdx]),mb);
		mindist[compIdx] = 0;
		mincost = sao_lambda *((double)minrate[compIdx]);
		store_coding_state(cs_sao_next_type);
		reset_coding_state(cs_sao_cur_type);
		// for other normal mode
		if (slice_sao_on[compIdx])
		{
			for ( type = 0; type < NUM_SAO_NEW_TYPES; type++)
			{
				temp_sao_param[compIdx].modeIdc = SAO_MODE_NEW;
				temp_sao_param[compIdx].typeIdc = type;
				find_offset( compIdx, type, saostatData,  temp_sao_param, sao_lambda,offsetTh );
				curdist[compIdx] =get_distortion(compIdx, type, saostatData,temp_sao_param);
				// assert(curdist[compIdx]<=0);

				currate[compIdx] = write_sao_mode(&(temp_sao_param[compIdx]),mb);
				currate[compIdx] += write_sao_offset(&(temp_sao_param[compIdx]),mb,offsetTh);
				currate[compIdx] += write_sao_type(&(temp_sao_param[compIdx]),mb);

				curcost = (double)(curdist[compIdx]) + sao_lambda * ((double)currate[compIdx]);

				if (curcost < mincost)
				{
					mincost = curcost;
					minrate[compIdx] = currate[compIdx];
					mindist[compIdx] = curdist[compIdx];
					copySAOParam_for_blk_onecomponent(&sao_cur_param[compIdx], &temp_sao_param[compIdx]);
					copySAOParam_for_blk_onecomponent(&rec_sao_cur_param[compIdx], &temp_sao_param[compIdx]);
					store_coding_state(cs_sao_next_type);
				}
				reset_coding_state(cs_sao_cur_type);
			}
			reset_coding_state(cs_sao_next_type);
		}
		else
		{
			mindist[compIdx] = 0;
			minrate[compIdx] = 0;
		}
	}

	normmodecost = (double)(mindist[SAO_Y] + mindist[SAO_Cb] + mindist[SAO_Cr])/sao_lambda;
#if SAO_ENC_FIX
	normmodecost += minrate[SAO_Y] + minrate[SAO_Cb] + minrate[SAO_Cr] + mergeflag_rate;
#else
	normmodecost += minrate[SAO_Y] + minrate[SAO_Cb] + minrate[SAO_Cr];
#endif
	return normmodecost;

}
void find_offset( int compIdx, int typeIdc, SAOStatData** saostatData, SAOBlkParam* saoBlkParam,double lambda, int offsetTh )
{
	int class_i;
	double classcost[MAX_NUM_SAO_CLASSES];
	double offth;
	int num_class = (typeIdc == SAO_TYPE_BO)?  NUM_SAO_BO_CLASSES: NUM_SAO_EO_CLASSES;
  double mincost_bandsum, cost_bandsum;
	int start_band1, start_band2,delta_band12;
	int sb_temp[2];
	int db_temp;
	for (class_i = 0; class_i < num_class; class_i++)
	{
		if( (typeIdc != SAO_TYPE_BO) && (class_i == SAO_CLASS_EO_PLAIN)  ) 
		{
			saoBlkParam[compIdx].offset[class_i] = 0;
			continue;
		}
		if(saostatData[compIdx][typeIdc].count[class_i] == 0)
		{
			saoBlkParam[compIdx].offset[class_i] = 0; //offset will be zero
			continue;
		}
		offth = saostatData[compIdx][typeIdc].diff[class_i] > 0 ? 0.5 : (saostatData[compIdx][typeIdc].diff[class_i] < 0 ? -0.5 : 0);
		saoBlkParam[compIdx].offset[class_i] = (int)((double)saostatData[compIdx][typeIdc].diff[class_i] / (double)saostatData[compIdx][typeIdc].count[class_i] + offth);
	}
	if (typeIdc ==SAO_TYPE_BO )
	{
		for (class_i = 0; class_i < num_class; class_i++)
		{
			saoBlkParam[compIdx].offset[class_i] = offset_estimation(typeIdc, class_i, lambda, saoBlkParam[compIdx].offset[class_i], saostatData[compIdx][typeIdc].count[class_i], saostatData[compIdx][typeIdc].diff[class_i], &(classcost[class_i]));
		}
         mincost_bandsum = MAX_DOUBLE;
		for(start_band1 = 0; start_band1 < ( NUM_SAO_BO_CLASSES - 1 ); start_band1++)
		{
			for(start_band2 = start_band1 + 2;  start_band2 < ( NUM_SAO_BO_CLASSES - 1 ); start_band2++)
			{
				cost_bandsum = classcost[start_band1] + classcost[start_band1 + 1] + classcost[start_band2] + classcost[start_band2 + 1];
				delta_band12 = (start_band2 - start_band1) > (NUM_SAO_BO_CLASSES>>1) ? (32 - start_band2 + start_band1) : (start_band2 - start_band1);
				assert(delta_band12 >=0 && delta_band12 <= (NUM_SAO_BO_CLASSES>>1));
				cost_bandsum += lambda * deltaband_cost[delta_band12];
				if(cost_bandsum < mincost_bandsum)
				{
					mincost_bandsum = cost_bandsum;
                    saoBlkParam[compIdx].startBand = start_band1;
					saoBlkParam[compIdx].startBand2 = start_band2;
				}
			}
		}
		for (class_i = 0; class_i < num_class; class_i++)
		{
			if ( (class_i >= saoBlkParam[compIdx].startBand && class_i <= saoBlkParam[compIdx].startBand + 1) || (class_i >= saoBlkParam[compIdx].startBand2 && class_i <= saoBlkParam[compIdx].startBand2 + 1)  )
			{
				continue;
			}
			saoBlkParam[compIdx].offset[class_i] = 0;
		}
		sb_temp[0] = min(saoBlkParam[compIdx].startBand,saoBlkParam[compIdx].startBand2);
		sb_temp[1] = max(saoBlkParam[compIdx].startBand,saoBlkParam[compIdx].startBand2);
		db_temp = (sb_temp[1] - sb_temp[0]);
		if(db_temp > (NUM_SAO_BO_CLASSES>>1))
		{
			saoBlkParam[compIdx].deltaband = 32 - db_temp;
			saoBlkParam[compIdx].startBand = sb_temp[1];
			saoBlkParam[compIdx].startBand2 = sb_temp[0];
		}     
		else
		{
			saoBlkParam[compIdx].deltaband = db_temp;
			saoBlkParam[compIdx].startBand = sb_temp[0];
			saoBlkParam[compIdx].startBand2 = sb_temp[1];
		}
	}
	else
	{
		assert(typeIdc>=SAO_TYPE_EO_0 && typeIdc<=SAO_TYPE_EO_45);
		for (class_i = 0; class_i < num_class; class_i++)
		{
			if (class_i==SAO_CLASS_EO_PLAIN)
			{
				saoBlkParam[compIdx].offset[class_i] = 0;
				classcost[class_i] = 0;
			}
			else
			{
				saoBlkParam[compIdx].offset[class_i] = offset_estimation(typeIdc, class_i, lambda, saoBlkParam[compIdx].offset[class_i],saostatData[compIdx][typeIdc].count[class_i], saostatData[compIdx][typeIdc].diff[class_i], &(classcost[class_i]));
			}
		}
		saoBlkParam[compIdx].startBand = 0; 
	}        
}
int offset_estimation(int typeIdx, int classIdx, double lambda, long int offset_ori, int count, long int diff, double* bestCost)
{
	int cur_offset = offset_ori;
	int offset_best=0;
	int lower_bd, upper_bd, Th;
	int temp_offset,start_offset, end_offset;
	int temprate;
	long int tempdist;
	double tempcost,mincost;
	int *eo_offset_bins = &(EO_OFFSET_MAP[1]);
	int offset_type;
	if(typeIdx == SAO_TYPE_BO )
	{
		offset_type = SAO_CLASS_BO;
	}
	else
	{
		offset_type = classIdx;
	}
	lower_bd = saoclip[offset_type][0];
	upper_bd = saoclip[offset_type][1];
	Th = saoclip[offset_type][2];
	cur_offset = Clip3(lower_bd, upper_bd, cur_offset);
	if (typeIdx == SAO_TYPE_BO)
	{
		start_offset = cur_offset >= 0 ? 0 : cur_offset ;
		end_offset = cur_offset >= 0 ? cur_offset : 0;
	}
	else
	{
		assert(typeIdx >= SAO_TYPE_EO_0 && typeIdx <= SAO_TYPE_EO_45);
		switch(classIdx)
		{
		case SAO_CLASS_EO_FULL_VALLEY:
			start_offset = -1;
			end_offset = max(cur_offset, 1);
			break;
		case SAO_CLASS_EO_HALF_VALLEY:
			start_offset = 0;
			end_offset = 1;
			break;
		case SAO_CLASS_EO_HALF_PEAK:
			start_offset = -1;
			end_offset = 0;
			break;
		case SAO_CLASS_EO_FULL_PEAK:
			start_offset = min(cur_offset, -1);
			end_offset = 1;
			break;
		default:				
			printf("Not a supported SAO mode\n");
			assert(0);
			exit(-1);
		}
	}
	mincost = 1e30;
	for ( temp_offset = start_offset; temp_offset <= end_offset; temp_offset++)
	{
		if (typeIdx == SAO_TYPE_BO)
		{
			assert(offset_type==SAO_CLASS_BO );
			temprate = abs(temp_offset);
			temprate = temprate ? (temprate + 1) : 0;
		}
		else if ( classIdx == SAO_CLASS_EO_HALF_VALLEY || classIdx == SAO_CLASS_EO_HALF_PEAK)
		{
			temprate = abs(temp_offset);
		}
		else
		{
			temprate = eo_offset_bins[classIdx == SAO_CLASS_EO_FULL_VALLEY ? temp_offset : -temp_offset];
		}
		temprate = (temprate == Th) ? temprate : (temprate + 1);

		tempdist = distortion_cal( count, temp_offset, diff);
		tempcost = (double)tempdist + lambda * (double) temprate;
		if(tempcost < mincost)
		{
			mincost = tempcost;
			offset_best = temp_offset;
			*bestCost = tempcost;
		}

	}
	return offset_best;
}

long int get_distortion(int compIdx, int type,SAOStatData** saostatData, SAOBlkParam* sao_cur_param)
{
	int classIdc, bandIdx;
	long int dist=0;
	switch(type)
	{
	case SAO_TYPE_EO_0:
	case SAO_TYPE_EO_90:
	case SAO_TYPE_EO_135:
	case SAO_TYPE_EO_45:
		{
			for (classIdc=0; classIdc<NUM_SAO_EO_CLASSES; classIdc++)
			{
				dist += distortion_cal( saostatData[compIdx][type].count[classIdc], sao_cur_param[compIdx].offset[classIdc], saostatData[compIdx][type].diff[classIdc]);
			}        
		}
		break;
	case SAO_TYPE_BO:
		{
			for ( classIdc = 0; classIdc < NUM_BO_OFFSET; classIdc++)
			{
				bandIdx = classIdc % NUM_SAO_BO_CLASSES ; 
				dist += distortion_cal( saostatData[compIdx][type].count[bandIdx], sao_cur_param[compIdx].offset[bandIdx], saostatData[compIdx][type].diff[bandIdx]);
			}
		}
		break;
	default:
		{
			printf("Not a supported type");
			assert(0);
			exit(-1);
		}
	}

	return dist;
}
long int  distortion_cal( long int count, int offset, long int diff)
{
	return ( count*offset*offset-diff*offset*2 ) ;

}


void writeParaSAO_one_SMB(int smb_index,int mb_y, int mb_x, int smb_mb_height, int smb_mb_width, int *slice_sao_on, SAOBlkParam* saoBlkParam)
{
	int pix_x = mb_x << MIN_CU_SIZE_IN_BIT;
	int pix_y = mb_y << MIN_CU_SIZE_IN_BIT;
	int smb_pix_width = smb_mb_width << MIN_CU_SIZE_IN_BIT;
	int smb_pix_height = smb_mb_height << MIN_CU_SIZE_IN_BIT;
	if (!slice_sao_on[0] && !slice_sao_on[1] && !slice_sao_on[2])
	{
		return;
	} 
	write_sao_smb(smb_index, pix_y, pix_x, smb_pix_width, smb_pix_height, slice_sao_on, saoBlkParam);
}
void write_sao_smb( int smb_index, int pix_y, int pix_x, int smb_pix_width, int smb_pix_height, int* slice_sao_on, SAOBlkParam* sao_cur_param)
{
	int mb_x = pix_x >> MIN_CU_SIZE_IN_BIT;
	int mb_y = pix_y >> MIN_CU_SIZE_IN_BIT;
	int mb = mb_y * img->PicWidthInMbs + mb_x;
	SAOBlkParam merge_candidate[NUM_SAO_MERGE_TYPES][NUM_SAO_COMPONENTS];
	int merge_avail[NUM_SAO_MERGE_TYPES];
	int MergeLeftAvail, MergeUpAvail;
	int offsetTh = 7;
	int compIdx;
	getMergeNeighbor( smb_index, pix_y,  pix_x, smb_pix_width,smb_pix_height, input->g_uiMaxSizeInBit,input->slice_set_enable, img->saoBlkParams, merge_avail, merge_candidate);
	MergeLeftAvail = merge_avail[0];
	MergeUpAvail = merge_avail[1];
	if (MergeLeftAvail + MergeUpAvail)
	{
		//assert(slice_sao_on[SAO_Y]);
		write_sao_mergeflag( MergeLeftAvail, MergeUpAvail, &(sao_cur_param[SAO_Y]), mb);	
	}
	if(sao_cur_param[SAO_Y].modeIdc != SAO_MODE_MERGE)
	{
		//luma
		if(slice_sao_on[SAO_Y]==1)
		{
			write_sao_mode(&(sao_cur_param[SAO_Y]),mb);
			if(sao_cur_param[SAO_Y].modeIdc == SAO_MODE_NEW)
			{
				write_sao_offset(&(sao_cur_param[SAO_Y]),mb,offsetTh);
				write_sao_type(&(sao_cur_param[SAO_Y]),mb);
			}
		}
		for (compIdx = SAO_Cb; compIdx < NUM_SAO_COMPONENTS; compIdx++)
		{
			if(slice_sao_on[compIdx]==1)
			{
				write_sao_mode(&(sao_cur_param[compIdx]),mb);
				if(sao_cur_param[compIdx].modeIdc == SAO_MODE_NEW)
				{
					write_sao_offset(&(sao_cur_param[compIdx]),mb, offsetTh);
					write_sao_type(&(sao_cur_param[compIdx]),mb);
				}
			}
		}
	}
}
void Copy_SMB_for_SAO(int smb_index, int mb_y, int mb_x, int smb_mb_height, int smb_mb_width)
{
	int i, j, k;
	int pix_y = mb_y << MIN_CU_SIZE_IN_BIT;
	int pix_x = mb_x << MIN_CU_SIZE_IN_BIT;
	int smb_pix_height = smb_mb_height << MIN_CU_SIZE_IN_BIT;
	int smb_pix_width = smb_mb_width << MIN_CU_SIZE_IN_BIT;
	for (j = pix_y; j < pix_y + smb_pix_height; j++)
	{
		for (i = pix_x; i < pix_x + smb_pix_width; i++)
		{
			imgY_sao[j][i] = imgY[j][i];
		}
	}
	for (k = 0; k < 2; k++)
	{
		for (j = ( pix_y >> 1 ); j < (( pix_y + smb_pix_height ) >> 1); j++)
		{
			for ( i = ( pix_x >> 1 ); i < (( pix_x + smb_pix_width ) >> 1); i++)
			{
				imgUV_sao[k][j][i] = imgUV[k][j][i];
			}
		}
	}

}
void Deblock_One_SMB(int smb_index, int mb_y, int mb_x, int smb_mb_height, int smb_mb_width)
 {
	 int mb_i, mb_j;
	 int temp_mb_nr = img->current_mb_nr;
	 assert(temp_mb_nr == mb_y*img->PicWidthInMbs + mb_x);
#if !DBLK_16x16_BASED
	 for ( mb_j = mb_y; mb_j < mb_y + smb_mb_height; mb_j++)
	 {
		 for (mb_i = mb_x; mb_i < mb_x + smb_mb_width; mb_i++)
		 {
			 img->current_mb_nr = mb_j * img->PicWidthInMbs + mb_i;
			 DeblockMb (  imgY_sao, imgUV_sao,  mb_j,  mb_i );
		 }
	 }
#else
	 for ( mb_j = mb_y; mb_j < mb_y + smb_mb_height; mb_j++)
	 {
		 for (mb_i = mb_x; mb_i < mb_x + smb_mb_width; mb_i++)
		 {
			 img->current_mb_nr = mb_j * img->PicWidthInMbs + mb_i;
			 DeblockMb (  imgY_sao, imgUV_sao,  mb_j,  mb_i, 0 );
		 }
	 }

	 	 for ( mb_j = mb_y; mb_j < mb_y + smb_mb_height; mb_j++)
	 {
		 for (mb_i = mb_x; mb_i < mb_x + smb_mb_width; mb_i++)
		 {
			 img->current_mb_nr = mb_j * img->PicWidthInMbs + mb_i;
			 DeblockMb (  imgY_sao, imgUV_sao,  mb_j,  mb_i, 1 );
		 }
	 }
#endif
	 img->current_mb_nr = temp_mb_nr;
 }
