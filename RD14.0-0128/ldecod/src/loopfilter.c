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
#include <math.h>
#include "global.h"
#include <assert.h>
#include "../../lcommon/inc/commonVariables.h"
#include "../../lcommon/inc/loop-filter.h"
#include "AEC.h"
#define  IClip( Min, Max, Val) (((Val)<(Min))? (Min):(((Val)>(Max))? (Max):(Val)))

extern const byte QP_SCALE_CR[64] ;

byte ALPHA_TABLE[64] =
{
  0, 0, 0, 0, 0, 0, 1, 1,
  1, 1, 1, 2, 2, 2, 3, 3,
  4, 4, 5, 5, 6, 7, 8, 9,
  10, 11, 12, 13, 15, 16, 18, 20,
  22, 24, 26, 28, 30, 33, 33, 35,
  35, 36, 37, 37, 39, 39, 42, 44,
  46, 48, 50, 52, 53, 54, 55, 56,
  57, 58, 59, 60, 61, 62, 63, 64
};
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
int EO_OFFSET_INV__MAP[] = {1, 0, 2, -1, 3, 4, 5, 6};
////////////////////////////ADD BY HJQ 2012-11-1//////////////////////////////////////////////
typedef Boolean bool;
#if CHROMA_DEBLOCK_BUG_FIX
int ** ppbEdgeFilter[2];
#else
bool ** ppbEdgeFilter[2];  //[Ver / Hor][b8_x][b8_y] 
#endif

#define LOOPFILTER_SIZE 3   //8X8

void GetStrength ( byte Strength[2], codingUnit* MbP, codingUnit* MbQ, int dir, int edge, int block_y, int block_x );
void EdgeLoop ( byte* SrcPtr, byte Strength[2], int QP,  int dir, int width, int Chro );
#if DBLK_16x16_BASED 
void DeblockMb ( byte **imgY, byte ***imgUV, int blk_y, int blk_x, int EdgeDir ) ;
#else
void DeblockMb ( byte **imgY, byte ***imgUV, int blk_y, int blk_x ) ;
#endif
void EdgeLoopX ( byte* SrcPtr, int QP, int dir, int width, int Chro, codingUnit* MbP, codingUnit* MbQ, int block_y, int block_x );
int SkipFiltering ( codingUnit* MbP, codingUnit* MbQ, int dir, int edge, int block_y, int block_x );
//void read_SAOParameter(int* slice_sao_on);
void read_sao_smb(int smb_index, int pix_y, int pix_x, int smb_pix_width, int smb_pix_height, int* slice_sao_on, SAOBlkParam* sao_cur_param, SAOBlkParam* rec_sao_cur_param);
#if CHROMA_DEBLOCK_BUG_FIX
void xSetEdgeFilterParam(unsigned int uiBitSize, unsigned int b8_x_start, unsigned int b8_y_start, int idir,int edgecondition)
#else
void xSetEdgeFilterParam( unsigned int uiBitSize, unsigned int b8_x_start, unsigned int b8_y_start, int idir)
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
                ppbEdgeFilter[idir][b8_y_start + i][b8_x_start] = TRUE;
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
        int edge_type_onlyluma = 1;    
        int edge_type_all = 2;    //EdgeCondition 0:not boundary 1:PU/TU boundary 2:CU boundary
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

  if (currMB->ui_MbBitSize < uiBitSize)
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
  xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start, 0, edge_type_all);  /// LEFT
  xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start, 1, edge_type_all);  ///UP
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
        break;
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
            xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start + (1 << (i - 1)), 1, edge_type_onlyluma);   ///?ú32???μí3?? 1<<-1=0  ???ú64???μí3??2?ê?
            xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start + (1 << (i - 1)) * 2, 1, edge_type_onlyluma);
            xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start + (1 << (i - 1)) * 3, 1, edge_type_onlyluma);
#else
            xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start + (1 << (i - 1)), 1);   ///?ú32???μí3?? 1<<-1=0  ???ú64???μí3??2?ê?
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
          xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start + (1 << (i - 1)), 1, edge_type_all);   ///?ú32???μí3?? 1<<-1=0  ???ú64???μí3??2?ê?
#else
          xSetEdgeFilterParam(uiBitSize, b8_x_start, b8_y_start+(1<<(i-1) ), 1);   ///?ú32???μí3?? 1<<-1=0  ???ú64???μí3??2?ê?
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
	int size = 1 << input->g_uiMaxSizeInBit;  //input->
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
      num_of_orgMB_in_row = 1 << ( input->g_uiMaxSizeInBit - MIN_CU_SIZE_IN_BIT );  //
    }

    if ( pix_y + size >= img->height )
    {
      num_of_orgMB_in_col = ( img->height - pix_y ) >> MIN_CU_SIZE_IN_BIT;
    }
    else
    {
      num_of_orgMB_in_col = 1 << ( input->g_uiMaxSizeInBit - MIN_CU_SIZE_IN_BIT ); //
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
			num_of_orgMB_in_row = 1 << ( input->g_uiMaxSizeInBit - 4 );  //input->
		}

		if ( pix_y + size >= img->height )
		{
			num_of_orgMB_in_col = ( img->height - pix_y ) >> 4;
		}
		else
		{
			num_of_orgMB_in_col = 1 << ( input->g_uiMaxSizeInBit - 4 ); //input->
		}
#endif
        num_of_orgMB = num_of_orgMB_in_col * num_of_orgMB_in_row;

		xSetEdgeFilter_One_SMB( input->g_uiMaxSizeInBit, CurrentMbNumber);

		if ( ( CurrentMbNumber + num_of_orgMB_in_row ) % MBRowSize == 0 ) //end of the row
		{
#if M3198_CU8
      CurrentMbNumber += ( num_of_orgMB_in_row + MBRowSize * ( ( 1 << ( input->g_uiMaxSizeInBit - MIN_CU_SIZE_IN_BIT ) ) - 1 ) ); //eg. 64 +: (4-1)*num_mb_inWidth+4
#else
      CurrentMbNumber += ( num_of_orgMB_in_row + MBRowSize * ( ( 1 << ( input->g_uiMaxSizeInBit - 4 ) ) - 1 ) ); //eg. 64 +: (4-1)*num_mb_inWidth+4
#endif	
		}
		else
		{
			CurrentMbNumber += num_of_orgMB_in_row;
		}

		if (CurrentMbNumber >=img->PicSizeInMbs)  //img->total_number_mb=( img->width * img->height)/(MIN_CU_SIZE*MIN_CU_SIZE)
		{
			end_of_picture = TRUE;
		}
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
        ppbEdgeFilter[i][b8_y][b8_x]=0;
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
* Function:The main MB-filtering function
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/


void DeblockFrame ( byte **imgY, byte ***imgUV )
{
  int       mb_x, mb_y ;
#if !DBLK_16x16_BASED
  img->current_mb_nr = -1;

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
      img->current_mb_nr++;

      if ( input->chroma_format == 1 )
      {
        DeblockMb ( imgY, imgUV, mb_y, mb_x ) ;
      }
    }
  }
#else
#if !DBLK_FRAME_BASED
  int blk16x16_x, blk16x16_y, mb_y0, mb_x0;
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
  //cms 按照8x8 块，循环
  img->current_mb_nr = -1;    // jlzheng  7.18
  for ( mb_y = 0 ; mb_y < ( img->height >> MIN_CU_SIZE_IN_BIT ) ; mb_y++ )
  {
    for ( mb_x = 0 ; mb_x < ( img->width >> MIN_CU_SIZE_IN_BIT ) ; mb_x++ )
    {
      img->current_mb_nr++;   // jlzheng 7.18
#if EXTRACT_PIC_DF_BS_debug   ////2016-6-1 提取整帧的去块滤波Bs参数
      fprintf(p_pic_df_bs, "DeblockFrame mb_y,x=%4d %4d \n", mb_y, mb_x);
#endif
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
* Function: Deblocks one codingUnit
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
      //flag = 0;
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
        MbP = ( dir ) ? ( MbQ - ( img->width >> MIN_CU_SIZE_IN_BIT ) )  : ( MbQ - 1 ); // MbP = Mb of the remote 4x4 block   
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
          img->current_df_yuv=0;              ////2016-6-1 提取整帧的去块滤波Bs参数,记录当前在滤波的色彩
          EdgeLoopX ( SrcY, QP, dir, img->width, 0, MbP, MbQ, mb_y << 1, mb_x << 1 );

#if CHROMA_DEBLOCK_BUG_FIX
          if ((EdgeCondition == 2) && (imgUV != NULL)/* && ( !edge )*/)
#else
          if ( ( imgUV != NULL )/* && ( !edge )*/ )
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

                cpQPu = MbP->qp - 8 * (input->sample_bit_depth - 8) + chroma_quant_param_delta_u;
                cpQPu = cpQPu < 0 ? cpQPu : QP_SCALE_CR[cpQPu];
                cpQPu = Clip3(0, 63 + 8 * (input->sample_bit_depth - 8), cpQPu + 8 * (input->sample_bit_depth - 8));
                cqQPu = MbQ->qp - 8 * (input->sample_bit_depth - 8) + chroma_quant_param_delta_u;
                cqQPu = cqQPu < 0 ? cqQPu : QP_SCALE_CR[cqQPu];
                cqQPu = Clip3(0, 63 + 8 * (input->sample_bit_depth - 8), cqQPu + 8 * (input->sample_bit_depth - 8));
                cQPu = (cpQPu + cqQPu + 1) >> 1;

                cpQPv = MbP->qp - 8 * (input->sample_bit_depth - 8) + chroma_quant_param_delta_v;
                cpQPv = cpQPv < 0 ? cpQPv : QP_SCALE_CR[cpQPv];
                cpQPv = Clip3(0, 63 + 8 * (input->sample_bit_depth - 8), cpQPv + 8 * (input->sample_bit_depth - 8));
                cqQPv = MbQ->qp - 8 * (input->sample_bit_depth - 8) + chroma_quant_param_delta_v;
                cqQPv = cqQPv < 0 ? cqQPv : QP_SCALE_CR[cqQPv];
                cqQPv = Clip3(0, 63 + 8 * (input->sample_bit_depth - 8), cqQPv + 8 * (input->sample_bit_depth - 8));
                cQPv = (cpQPv + cqQPv + 1) >> 1;
#else
                cQPu = QP - 8 * (input->sample_bit_depth - 8) + chroma_quant_param_delta_u;
                cQPu = cQPu < 0 ? cQPu : QP_SCALE_CR[cQPu];
                cQPu = Clip3(0, 63 + 8 * (input->sample_bit_depth - 8), cQPu + 8 * (input->sample_bit_depth - 8));
                cQPv = QP - 8 * (input->sample_bit_depth - 8) + chroma_quant_param_delta_v;
                cQPv = cQPv < 0 ? cQPv : QP_SCALE_CR[cQPv];
                cQPv = Clip3(0, 63 + 8 * (input->sample_bit_depth - 8), cQPv + 8 * (input->sample_bit_depth - 8));
#endif
                EdgeLoopX ( SrcU, cQPu, dir, img->width_cr, 1, MbP, MbQ, mb_y << 1, mb_x << 1 ) ;
                EdgeLoopX ( SrcV, cQPv, dir, img->width_cr, 1, MbP, MbQ, mb_y << 1, mb_x << 1 ) ;
#else
                if((pic_weight_quant_enable_flag)&&(!chroma_quant_param_disable))

                {
                  EdgeLoopX ( SrcU, QP_SCALE_CR[Clip3(0, 63, (QP + chroma_quant_param_delta_u - (8 * (input->sample_bit_depth - 8))))] + (8 * (input->sample_bit_depth - 8)), dir, img->width_cr, 1, MbP, MbQ, mb_y << 1, mb_x << 1 ) ;
                  EdgeLoopX ( SrcV, QP_SCALE_CR[Clip3(0, 63, (QP + chroma_quant_param_delta_v - (8 * (input->sample_bit_depth - 8))))] + (8 * (input->sample_bit_depth - 8)), dir, img->width_cr, 1, MbP, MbQ, mb_y << 1, mb_x << 1 ) ;
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
              //EdgeLoopX ( SrcU, Clip3(0, 51, (QP_SCALE_CR[Clip3(0, 63, (QP - 16))] + 16)), dir, img->width_cr, 1 ) ;
              //EdgeLoopX ( SrcV, Clip3(0, 51, (QP_SCALE_CR[Clip3(0, 63, (QP - 16))] + 16)), dir, img->width_cr, 1 ) ;
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

                cpQPu = MbP->qp - 8 * (input->sample_bit_depth - 8) + chroma_quant_param_delta_u;
                cpQPu = cpQPu < 0 ? cpQPu : QP_SCALE_CR[cpQPu];
                cpQPu = Clip3(0, 63 + 8 * (input->sample_bit_depth - 8), cpQPu + 8 * (input->sample_bit_depth - 8));
                cqQPu = MbQ->qp - 8 * (input->sample_bit_depth - 8) + chroma_quant_param_delta_u;
                cqQPu = cqQPu < 0 ? cqQPu : QP_SCALE_CR[cqQPu];
                cqQPu = Clip3(0, 63 + 8 * (input->sample_bit_depth - 8), cqQPu + 8 * (input->sample_bit_depth - 8));
                cQPu = (cpQPu + cqQPu + 1) >> 1;

                cpQPv = MbP->qp - 8 * (input->sample_bit_depth - 8) + chroma_quant_param_delta_v;
                cpQPv = cpQPv < 0 ? cpQPv : QP_SCALE_CR[cpQPv];
                cpQPv = Clip3(0, 63 + 8 * (input->sample_bit_depth - 8), cpQPv + 8 * (input->sample_bit_depth - 8));
                cqQPv = MbQ->qp - 8 * (input->sample_bit_depth - 8) + chroma_quant_param_delta_v;
                cqQPv = cqQPv < 0 ? cqQPv : QP_SCALE_CR[cqQPv];
                cqQPv = Clip3(0, 63 + 8 * (input->sample_bit_depth - 8), cqQPv + 8 * (input->sample_bit_depth - 8));
                cQPv = (cpQPv + cqQPv + 1) >> 1;
#else
                cQPu = QP - 8 * (input->sample_bit_depth - 8) + chroma_quant_param_delta_u;
                cQPu = cQPu < 0 ? cQPu : QP_SCALE_CR[cQPu];
                cQPu = Clip3(0, 63 + 8 * (input->sample_bit_depth - 8), cQPu + 8 * (input->sample_bit_depth - 8));
                cQPv = QP - 8 * (input->sample_bit_depth - 8) + chroma_quant_param_delta_v;
                cQPv = cQPv < 0 ? cQPv : QP_SCALE_CR[cQPv];
                cQPv = Clip3(0, 63 + 8 * (input->sample_bit_depth - 8), cQPv + 8 * (input->sample_bit_depth - 8));
#endif
                img->current_df_yuv = 1;              ////2016-6-1 提取整帧的去块滤波Bs参数,记录当前在滤波的色彩
                EdgeLoopX ( SrcU, cQPu, dir, img->width_cr, 1, MbP, MbQ, mb_y << 1, mb_x << 1 ) ;
                img->current_df_yuv = 2;              ////2016-6-1 提取整帧的去块滤波Bs参数,记录当前在滤波的色彩
                EdgeLoopX ( SrcV, cQPv, dir, img->width_cr, 1, MbP, MbQ, mb_y << 1, mb_x << 1 ) ;
#else
                if((pic_weight_quant_enable_flag)&&(!chroma_quant_param_disable))
                {
                  EdgeLoopX ( SrcU, QP_SCALE_CR[QP + chroma_quant_param_delta_u], dir, img->width_cr, 1, MbP, MbQ, mb_y << 1, mb_x << 1 ) ;
                  EdgeLoopX ( SrcV, QP_SCALE_CR[QP + chroma_quant_param_delta_v], dir, img->width_cr, 1, MbP, MbQ, mb_y << 1, mb_x << 1 ) ;
                }
                else
                {
                  EdgeLoopX ( SrcU, QP_SCALE_CR[QP], dir, img->width_cr, 1, MbP, MbQ, mb_y << 1, mb_x << 1 ) ;
                  EdgeLoopX ( SrcV, QP_SCALE_CR[QP], dir, img->width_cr, 1, MbP, MbQ, mb_y << 1, mb_x << 1 ) ;
                }
#endif
#else
                EdgeLoopX ( SrcU, QP_SCALE_CR[QP], dir, img->width_cr, 1, MbP, MbQ, mb_y << 1, mb_x << 1 ) ;
                EdgeLoopX ( SrcV, QP_SCALE_CR[QP], dir, img->width_cr, 1, MbP, MbQ, mb_y << 1, mb_x << 1 ) ;
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
byte BLK_NUM[2][2][2]  = {{{0, 8}, {2, 10}}, {{0, 2}, {8, 10}}} ;
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
	else if ( (img->type == F_IMG)||(img->type == P_IMG) )
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
  Alpha = ALPHA_TABLE[Clip3 ( 0, 63, QP + input->alpha_c_offset ) ]; // jlzheng  7.8

  Beta = BETA_TABLE[Clip3 ( 0, 63, QP + input->beta_offset ) ]; // jlzheng  7.8

  for ( pel = 0 ; pel < 16 ; pel++ )
  {
    if ( ( Strng = Strength[pel >> 3] ) )
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
          else  //Strng == 1
          {
            C0  = CLIP_TAB[Clip3 ( 0, 63, QP + input->alpha_c_offset ) ] ; // jlzheng  7.12
            dif             = IClip ( -C0, C0, ( ( R0 - L0 ) * 3 + ( L1 - R1 ) + 4 ) >> 3 ) ;
            SrcPtr[  -inc ] = IClip ( 0, 255, L0 + dif ) ;
            SrcPtr[     0 ] = IClip ( 0, 255, R0 - dif ) ;

            if ( !Chro )
            {
              L0 = SrcPtr[-inc] ;
              R0 = SrcPtr[   0] ;

              if ( ap )
              {
                dif           = IClip ( -C0, C0, ( ( L0 - L1 ) * 3 + ( L2 - R0 ) + 4 ) >> 3 ) ;
                SrcPtr[-inc2] = IClip ( 0, 255, L1 + dif ) ;
              }

              if ( aq )
              {
                dif           = IClip ( -C0, C0, ( ( R1 - R0 ) * 3 + ( L0 - R2 ) + 4 ) >> 3 ) ;
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
#if EXTRACT_PIC_DF_BS   ////2016-6-1 提取整帧的去块滤波Bs参数
  int     b88_y,b88_x;//8x8 块的坐标
#endif
#if EXTEND_BD
  int  shift1 = input->sample_bit_depth - 8;
  int skipFilteringFlag = 0;
#endif
  // FlatnessL and FlatnessR describe how flat the curve is of one codingUnit.



  PtrInc  = dir ?  1 : width ;
  inc     = dir ?  width : 1 ;
  inc2    = inc << 1 ;
  inc3    = inc + inc2 ;

#if EXTRACT_PIC_DF_BS   ////2016-6-1 提取整帧的去块滤波Bs参数
  b88_y = (img->current_mb_nr / (img->width / 8));
  b88_x = (img->current_mb_nr % (img->width / 8));
#if EXTRACT_PIC_DF_BS_debug   ////2016-6-1 提取整帧的去块滤波Bs参数
  fprintf(p_pic_df_bs, "\tEdgeLoopX block_y,x=%4d %4d dir=%d width=%d Chro=%d img->current_df_yuv = %d\n", block_y, block_x, dir, width, Chro, img->current_df_yuv );
  fprintf(p_pic_df_bs, "\tinc=%d inc2=%d inc3=%d shift1=%d\n", inc, inc2, inc3, shift1);
#endif
#endif

#if EXTEND_BD
	if (input->sample_bit_depth > 8) // coded as 10 bit, QP is added by 16 in config file
	{
		Alpha =  ALPHA_TABLE[Clip3 ( 0, 63, QP - (8 * (input->sample_bit_depth - 8)) + input->alpha_c_offset ) ]; // jlzheng 7.8
		Beta =  BETA_TABLE[Clip3 ( 0, 63, QP - (8 * (input->sample_bit_depth - 8)) + input->beta_offset ) ]; // jlzheng 7.8
	}
	else // coded as 8bit
	{
#endif
		Alpha =  ALPHA_TABLE[Clip3 ( 0, 63, QP + input->alpha_c_offset ) ]; // jlzheng 7.8
		Beta =  BETA_TABLE[Clip3 ( 0, 63, QP + input->beta_offset ) ]; // jlzheng 7.8
#if EXTEND_BD
	}

	Alpha = Alpha << shift1;
	Beta = Beta << shift1;
#endif

#if EXTRACT_PIC_DF_BS_debug   ////2016-6-1 提取整帧的去块滤波Bs参数
  fprintf(p_pic_df_bs, "\tAlpha=%d Beta=%d \n", Alpha, Beta);
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

#if EXTRACT_PIC_DF_BS   ////2016-6-1 提取整帧的去块滤波Bs参数
      fs = 0;
#endif
    }
#if EXTRACT_PIC_DF_BS   ////2016-6-1 提取整帧的去块滤波Bs参数
    if ( img->current_df_yuv == 0)//亮度
    {
      if (dir == 0)//垂直方向
      {
        picVerEdgeBsY[(b88_y<<3)+pel][b88_x] = fs;
#if EXTRACT_PIC_DF_BS_debug   ////2016-6-1 提取整帧的去块滤波Bs参数
        fprintf(p_pic_df_bs, "\tY Ver pel=%d fs=%d b88_y,x=%4d %4d [%3d][%3d]\n", pel, fs, b88_y, b88_x,(b88_y << 3) + pel,b88_x);
#endif
      }
      else //dir == 1)//水平方向
      {
        picHorEdgeBsY[b88_y][(b88_x<<3)+pel] = fs;
      }
    }
    else if (img->current_df_yuv == 1)//Cb
    {
      if (dir == 0)//垂直方向
      {
        picVerEdgeBsCb[(b88_y << 2) + (pel>> Chro)][b88_x] = fs;
#if EXTRACT_PIC_DF_BS_debug   ////2016-6-1 提取整帧的去块滤波Bs参数
        fprintf(p_pic_df_bs, "\tCb Ver pel=%d fs=%d b88_y,x=%4d %4d [%3d][%3d]\n", pel, fs, b88_y, b88_x, (b88_y << 2) + (pel >> Chro),b88_x);
#endif
      }
      else //dir == 1)//水平方向
      {
        picHorEdgeBsCb[b88_y][(b88_x << 2) + (pel >> Chro)] = fs;
#if EXTRACT_PIC_DF_BS_debug   ////2016-6-1 提取整帧的去块滤波Bs参数
        fprintf(p_pic_df_bs, "\tCb Hor pel=%d fs=%d b88_y,x=%4d %4d [%3d][%3d]\n", pel, fs, b88_y, b88_x, b88_y,(b88_x << 2) + (pel >> Chro));
#endif
      }
    }
    else if (img->current_df_yuv == 2)//Cr
    {
      if (dir == 0)//垂直方向
      {
        picVerEdgeBsCr[(b88_y << 2) + (pel >> Chro)][b88_x] = fs;
#if EXTRACT_PIC_DF_BS_debug   ////2016-6-1 提取整帧的去块滤波Bs参数
        fprintf(p_pic_df_bs, "\tCr Ver pel=%d fs=%d b88_y,x=%4d %4d [%3d][%3d]\n", pel, fs, b88_y, b88_x, (b88_y << 2) + (pel >> Chro), b88_x);
#endif
      }
      else //dir == 1)//水平方向
      {
        picHorEdgeBsCr[b88_y][(b88_x <<2) + (pel >> Chro)] = fs;
#if EXTRACT_PIC_DF_BS_debug   ////2016-6-1 提取整帧的去块滤波Bs参数
        fprintf(p_pic_df_bs, "\tCr Hor pel=%d fs=%d b88_y,x=%4d %4d [%3d][%3d]\n", pel, fs, b88_y, b88_x, b88_y,(b88_x << 2) + (pel >> Chro));
#endif
      }
    }
#endif
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
  blkQ = BLK_NUM[dir][edge][0] ;
  blk_y = block_y + ( blkQ >> 2 );
  blk_x = block_x + ( blkQ & 3 );
  blk_x >>= 1;
  blk_y >>= 1;
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

void readParaSAO_one_SMB(int smb_index,int mb_y, int mb_x, int smb_mb_height, int smb_mb_width, int *slice_sao_on, SAOBlkParam* saoBlkParam, SAOBlkParam* rec_saoBlkParam)
{
	int pix_x = mb_x << MIN_CU_SIZE_IN_BIT;
	int pix_y = mb_y << MIN_CU_SIZE_IN_BIT;
	int smb_pix_width = smb_mb_width << MIN_CU_SIZE_IN_BIT;
	int smb_pix_height = smb_mb_height << MIN_CU_SIZE_IN_BIT;
	if (!slice_sao_on[0] && !slice_sao_on[1] && !slice_sao_on[2])
	{
		off_sao(rec_saoBlkParam);
		off_sao(saoBlkParam);
	}
	else
	{
			read_sao_smb(smb_index, pix_y, pix_x, smb_pix_width, smb_pix_height, slice_sao_on, saoBlkParam,rec_saoBlkParam);
	}

}
void read_sao_smb(int smb_index, int pix_y, int pix_x, int smb_pix_width, int smb_pix_height, int* slice_sao_on, SAOBlkParam* sao_cur_param, SAOBlkParam* rec_sao_cur_param)
{
	int mb_x = pix_x >> MIN_CU_SIZE_IN_BIT;
	int mb_y = pix_y >> MIN_CU_SIZE_IN_BIT;
	int mb = mb_y * img->PicWidthInMbs + mb_x;
	SAOBlkParam merge_candidate[NUM_SAO_MERGE_TYPES][NUM_SAO_COMPONENTS];
	int merge_avail[NUM_SAO_MERGE_TYPES];
	int MergeLeftAvail, MergeUpAvail;
	int mergemode,saomode,saotype;
	int offset[32];
	int compIdx,i;
	int offsetTh = 7;
  int stBnd[2];
	int db_temp;
  getMergeNeighbor( smb_index, pix_y,  pix_x, smb_pix_width,smb_pix_height, input->g_uiMaxSizeInBit,input->slice_set_enable, img->rec_saoBlkParams, merge_avail, merge_candidate);
	MergeLeftAvail = merge_avail[0];
	MergeUpAvail = merge_avail[1];
	mergemode = 0;
	
			
#if EXTRACT
	lcuSAOInfo[0][8]=0;//mergemode 初始化为0  no-merge				
	lcuSAOInfo[1][8]=0;
	lcuSAOInfo[2][8]=0;
#endif	

	if (MergeLeftAvail + MergeUpAvail >0 )
	{
		mergemode = read_sao_mergeflag( MergeLeftAvail, MergeUpAvail, mb);	

#if EXTRACT_lcu_info_debug
		fprintf(p_lcu_debug,"mergemode %4d\n",mergemode);//8个 SAO
#endif		  
	}
	if (mergemode)
	{
    /*
    sao_cur_param[SAO_Y].modeIdc = sao_cur_param[SAO_Cb].modeIdc = sao_cur_param[SAO_Cr].modeIdc = SAO_MODE_MERGE;
		sao_cur_param[SAO_Y].typeIdc = sao_cur_param[SAO_Cb].typeIdc = sao_cur_param[SAO_Cr].typeIdc = mergemode==1 ? SAO_MERGE_LEFT : SAO_MERGE_ABOVE;
		*/
		if (mergemode == 2)
		{
			copySAOParam_for_blk(rec_sao_cur_param,merge_candidate[SAO_MERGE_LEFT]);
#if EXTRACT_lcu_info_debug
		  fprintf(p_lcu_debug,"mergemode %4d SAO_MERGE_LEFT\n",mergemode);//8个 SAO
#endif				
#if EXTRACT
			lcuSAOInfo[0][8]=1;
			lcuSAOInfo[1][8]=1;
			lcuSAOInfo[2][8]=1;
#endif			
		}
		else
		{
			assert(mergemode==1);
      copySAOParam_for_blk(rec_sao_cur_param,merge_candidate[SAO_MERGE_ABOVE]);
#if EXTRACT_lcu_info_debug
		  fprintf(p_lcu_debug,"mergemode %4d SAO_MERGE_ABOVE\n",mergemode);//8个 SAO
#endif						
#if EXTRACT
			lcuSAOInfo[0][8]=2;
			lcuSAOInfo[1][8]=2;
			lcuSAOInfo[2][8]=2;
#endif				
		}
      copySAOParam_for_blk(sao_cur_param,rec_sao_cur_param);
			sao_cur_param->modeIdc = SAO_MODE_MERGE;
			sao_cur_param->typeIdc = mergemode;
	}
	else
	{
#if EXTRACT
			lcuSAOInfo[0][8]=0;
			lcuSAOInfo[1][8]=0;
			lcuSAOInfo[2][8]=0;
#endif		
		for (compIdx = 0; compIdx < NUM_SAO_COMPONENTS; compIdx++)
		{
			if (!slice_sao_on[compIdx])
			{
				sao_cur_param[compIdx].modeIdc = SAO_MODE_OFF;
#if EXTRACT
				lcuSAOInfo[compIdx][0]=0;//off
#endif						
			}
			else
			{
				if (1)
				{
					saomode = read_sao_mode(mb);
#if EXTRACT_lcu_info_debug
		  		fprintf(p_lcu_debug,"compIdx=%d saomode %4d \n",compIdx,saomode);//8个 SAO
#endif							
					switch (saomode)
					{
						case 0: 
							sao_cur_param[compIdx].modeIdc = SAO_MODE_OFF; 
#if EXTRACT
							lcuSAOInfo[compIdx][0]=0;//off
#endif								
#if EXTRACT_lcu_info_debug
							fprintf(p_lcu_debug,"compIdx=%d saomode %4d off\n",compIdx,saomode);//8个 SAO
#endif		

							break;
						case 1:
							sao_cur_param[compIdx].modeIdc = SAO_MODE_NEW; 
							sao_cur_param[compIdx].typeIdc = SAO_TYPE_BO;
#if EXTRACT
							lcuSAOInfo[compIdx][0]=1;//BO
#endif								
							
#if EXTRACT_lcu_info_debug
							fprintf(p_lcu_debug,"compIdx=%d saomode %4d BO\n",compIdx,saomode);//8个 SAO
#endif		
							break;
						case 3:
							sao_cur_param[compIdx].modeIdc = SAO_MODE_NEW; 
							sao_cur_param[compIdx].typeIdc = SAO_TYPE_EO_0;
#if EXTRACT
							lcuSAOInfo[compIdx][0]=2;//EO
#endif												
#if EXTRACT_lcu_info_debug
							fprintf(p_lcu_debug,"compIdx=%d saomode %4d EO\n",compIdx,saomode);//8个 SAO
#endif			
							break;
						default: assert(1);
							break;
					}
				}
				else
				{
					sao_cur_param[compIdx].modeIdc = sao_cur_param[SAO_Cb].modeIdc; 
					if (sao_cur_param[compIdx].modeIdc != SAO_MODE_OFF)
					{
						sao_cur_param[compIdx].typeIdc = (sao_cur_param[SAO_Cb].typeIdc == SAO_TYPE_BO) ? SAO_TYPE_BO : SAO_TYPE_EO_0;
					}
				}
				
				if (sao_cur_param[compIdx].modeIdc == SAO_MODE_NEW)
				{
					read_sao_offset(&(sao_cur_param[compIdx]),mb, offsetTh,offset);
#if EXTRACT
					lcuSAOInfo[compIdx][1]=offset[0];//EO BO
					lcuSAOInfo[compIdx][2]=offset[1];//EO BO
					lcuSAOInfo[compIdx][3]=offset[2];//EO BO
					lcuSAOInfo[compIdx][4]=offset[3];//EO BO
					sao_cur_param[compIdx].sao_offset[0] = offset[0];
					sao_cur_param[compIdx].sao_offset[1] = offset[1];
					sao_cur_param[compIdx].sao_offset[2] = offset[2];
					sao_cur_param[compIdx].sao_offset[3] = offset[3];
#endif				

					if(1)
					{
						saotype = read_sao_type(&(sao_cur_param[compIdx]), mb);
					}	 
					else
					{
						assert(compIdx==SAO_Cr && sao_cur_param[SAO_Cr].typeIdc == SAO_TYPE_EO_0);
						saotype = sao_cur_param[SAO_Cb].typeIdc;
					}
					
					if (sao_cur_param[compIdx].typeIdc == SAO_TYPE_BO)
					{
						memset(sao_cur_param[compIdx].offset, 0, sizeof(int)*MAX_NUM_SAO_CLASSES);
						db_temp = saotype >> NUM_SAO_BO_CLASSES_LOG2;
						stBnd[0] = saotype - (db_temp << NUM_SAO_BO_CLASSES_LOG2);
						stBnd[1] = (stBnd[0] + db_temp) % 32;
#if EXTRACT
						lcuSAOInfo[compIdx][5]=stBnd[0];//BO start pos
						lcuSAOInfo[compIdx][6]=db_temp-2;//BO pos minus2

            //额外增加2个成员来保存 BO的起始区间序号
            sao_cur_param[compIdx].startPos = stBnd[0];
            sao_cur_param[compIdx].startPosM2 = db_temp - 2;
#endif							
						for ( i = 0; i < 2; i++)
						{
							sao_cur_param[compIdx].offset[stBnd[i]] = offset[i*2];
							sao_cur_param[compIdx].offset[(stBnd[i]+1)%32] = offset[i*2+1];
						}

					 }
					 else
					 {
						 assert(sao_cur_param[compIdx].typeIdc == SAO_TYPE_EO_0);
						 sao_cur_param[compIdx].typeIdc = saotype;
#if EXTRACT
						 lcuSAOInfo[compIdx][7]=saotype;//EO type
#endif							 
						 sao_cur_param[compIdx].offset[SAO_CLASS_EO_FULL_VALLEY] = offset[0];
						 sao_cur_param[compIdx].offset[SAO_CLASS_EO_HALF_VALLEY] = offset[1];
						 sao_cur_param[compIdx].offset[SAO_CLASS_EO_PLAIN      ] = 0;
						 sao_cur_param[compIdx].offset[SAO_CLASS_EO_HALF_PEAK  ] = offset[2];
						 sao_cur_param[compIdx].offset[SAO_CLASS_EO_FULL_PEAK  ] = offset[3];
#if EXTRACT
						sao_cur_param[compIdx].sao_offset[0] = offset[0];
						sao_cur_param[compIdx].sao_offset[1] = offset[1];
						sao_cur_param[compIdx].sao_offset[2] = offset[2];
						sao_cur_param[compIdx].sao_offset[3] = offset[3];
#endif								 
					 }
				}
			}
		}
		copySAOParam_for_blk(rec_sao_cur_param,sao_cur_param);

	}

}


