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
* File name: block.c
* Function: Description
*
*************************************************************************************
*/



#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <memory.h>

#include "../../lcommon/inc/defines.h"
#include "../../lcommon/inc/commonVariables.h"
#include "../../lcommon/inc/transform.h"
#include "../../lcommon/inc/block_info.h"
#include "../../lcommon/inc/intra-prediction.h"
#include "../../lcommon/inc/memalloc.h"
#include "global.h"
#include "block.h"
#include "vlc.h"
#include "AEC.h" 

#ifdef WIN32
#define int16 __int16
#else
#define int16 int16_t
#endif
#if INTRA_2N
#define LOT_MAX_WLT_TAP             2           ///< number of wavelet transform tap, (5-3)
static int iEBuff[ 64 + LOT_MAX_WLT_TAP * 2 ];
#endif


int **tmp;


#if TRANS_16_BITS
void idct_dequant_B8 ( int block8x8,
					  int qp,                         // Quantization parameter
					  int **curr_blk,
					  int uiBitSize )
{
	int  xx, yy;
	int  b8_y       = ( block8x8 / 2 ) << uiBitSize;
	int  b8_x       = ( block8x8 % 2 ) << uiBitSize;
	int  curr_val;
	int Size = 1 << uiBitSize;
	int pixInSMB_x = 0 ;
	int pixInSMB_y = 0 ;
  codingUnit * currMB = &img->mb_data[img->current_mb_nr];
  int isChroma= block8x8 <=3 ? 0 :1;
  currMB->l_ipred_mode = currMB->intra_pred_modes[block8x8];
	//  int tmp[MAX_CU_SIZE][MAX_CU_SIZE];
	get_mb_pos ( img->current_mb_nr, &pixInSMB_x, &pixInSMB_y, input->g_uiMaxSizeInBit );
	pixInSMB_x = img->pix_x - pixInSMB_x;
	pixInSMB_y = img->pix_y - pixInSMB_y;
	get_mem2Dint ( &tmp, 1 << uiBitSize, 1 << uiBitSize );

	// inverse transform
#if EXTEND_BD
	inv_transform_B8(curr_blk, uiBitSize, currMB, isChroma, b_secT_enabled, input->sample_bit_depth);
#else
	inv_transform_B8(curr_blk, uiBitSize, currMB, isChroma, b_secT_enabled);
#endif
	// normalize
	for ( yy = 0; yy < Size; yy++ )
	{
		for ( xx = 0; xx < Size; xx++ )
		{
			if ( block8x8 <= 3 )
			{
				curr_val = img->predBlock[b8_y + yy][b8_x + xx] + curr_blk[yy][xx];
			}
			else
			{
				curr_val = img->predBlock[yy][xx] + curr_blk[yy][xx];
			}

#if EXTEND_BD
			img->resiY[pixInSMB_y + yy][pixInSMB_x + xx] = curr_blk[yy][xx] = clamp ( curr_val, 0, (1 << (input->sample_bit_depth)) - 1 );
#else
			img->resiY[pixInSMB_y + yy][pixInSMB_x + xx] = curr_blk[yy][xx] = clamp ( curr_val, 0, 255 );
#endif
			if ( block8x8 <= 3 )
			{
#if EXTEND_BD
				imgY[img->pix_y + b8_y + yy][img->pix_x + b8_x + xx] = ( byte ) curr_blk[yy][xx];
#else
				imgY[img->pix_y + b8_y + yy][img->pix_x + b8_x + xx] = ( unsigned char ) curr_blk[yy][xx];
#endif
			}
			else
			{
#if EXTEND_BD
				imgUV[ ( block8x8 - 4 ) % 2][img->pix_c_y + yy][img->pix_c_x + xx] = ( byte ) curr_blk[yy][xx];
#else
				imgUV[ ( block8x8 - 4 ) % 2][img->pix_c_y + yy][img->pix_c_x + xx] = ( unsigned char ) curr_blk[yy][xx];
#endif
			}

		}
	}


#if EXTRACT_lcu_info_debug
  if (1)
  {
    int x, y;
    fprintf(p_lcu_debug, "Resi  CU block8x8=%d \n", block8x8);
    if (block8x8 <= 3)
      fprintf(p_lcu_debug, "InterResi Y  CU block8x8=%d \n", block8x8);
    else
      fprintf(p_lcu_debug, "InterResi UV CU block8x8=%d \n", block8x8);
    for (yy = 0; yy < Size; yy++)
    {
      for (xx = 0; xx < Size; xx++)
      {
        curr_val = img->predBlock[b8_y + yy][b8_x + xx] + curr_blk[yy][xx];
        fprintf(p_lcu_debug, "%4d", curr_blk[yy][xx]);
      }
      fprintf(p_lcu_debug, "\n");
    }for (yy = 0; yy < Size; yy++)
    {
      for (xx = 0; xx < Size; xx++)
      {
        curr_val = img->predBlock[b8_y + yy][b8_x + xx] + curr_blk[yy][xx];
        fprintf(p_lcu_debug, "%4d + %4d = %4d\n", img->predBlock[b8_y + yy][b8_x + xx] , curr_blk[yy][xx], curr_val);
      }
      fprintf(p_lcu_debug, "\n");
    }
  }
#endif
	free_mem2Dint ( tmp );
}

#else
void idct_dequant_B8 ( int block8x8,
					  int qp,                         // Quantization parameter
					  int **curr_blk,
					  int uiBitSize )
{
	int  xx, yy;
	int  b8_y       = ( block8x8 / 2 ) << uiBitSize;
	int  b8_x       = ( block8x8 % 2 ) << uiBitSize;
	int  curr_val;
	int Size = 1 << uiBitSize;
	int pixInSMB_x = 0 ;
	int pixInSMB_y = 0 ;
	int i, j;
	int **tmp;
	//  int tmp[MAX_CU_SIZE][MAX_CU_SIZE];
	get_mb_pos ( img->current_mb_nr, &pixInSMB_x, &pixInSMB_y, input->g_uiMaxSizeInBit );
	pixInSMB_x = img->pix_x - pixInSMB_x;
	pixInSMB_y = img->pix_y - pixInSMB_y;
	get_mem2Dint ( &tmp, 1 << uiBitSize, 1 << uiBitSize );

	// inverse transform
#if INTRA_2N
	switch ( uiBitSize )
	{
	case B8X8_IN_BIT:
		partialButterflyInverse8( curr_blk, tmp );
		partialButterflyInverse8( tmp, curr_blk );
		for ( i = 0; i < 8; i++ )
		{
			for ( j = 0; j < 8; j++ )
			{
				curr_blk[i][j] = sign( ( int )( ( abs( curr_blk[i][j] ) + ( 1 << ( shift_out - 2 ) ) ) >> ( shift_out - 1 ) ), curr_blk[i][j] );
				curr_blk[i][j] = Clip3( -255, 255, curr_blk[i][j] );
			}
		}
		break;
	case B16X16_IN_BIT:
		partialButterflyInverse16( curr_blk, tmp );
		partialButterflyInverse16( tmp, curr_blk );
		for ( i = 0; i < 16; i++ )
		{
			for ( j = 0; j < 16; j++ )
			{
				curr_blk[i][j] = sign( ( int )( ( abs( curr_blk[i][j] ) + ( 1 << ( shift_out - 2 ) ) ) >> ( shift_out - 1 ) ), curr_blk[i][j] );
				curr_blk[i][j] = Clip3( -255, 255, curr_blk[i][j] );
			}
		}
		break;
	case B32X32_IN_BIT:
		partialButterflyInverse32( curr_blk, tmp );
		partialButterflyInverse32( tmp, curr_blk );
		for ( i = 0; i < 32; i++ )
		{
			for ( j = 0; j < 32; j++ )
			{
				curr_blk[i][j] = sign( ( int )( ( abs( curr_blk[i][j] ) + ( 1 << ( shift_out - 2 ) ) ) >> ( shift_out - 1 ) ), curr_blk[i][j] );
				curr_blk[i][j] = Clip3( -255, 255, curr_blk[i][j] );
			}
		}
		break;
	case B64X64_IN_BIT:
		partialButterflyInverse32( curr_blk, tmp );
		partialButterflyInverse32( tmp, curr_blk );
		for ( i = 0; i < 32; i++ )
		{
			for ( j = 0; j < 32; j++ )
			{
				curr_blk[i][j] = sign( ( int )( ( abs( curr_blk[i][j] ) + ( 1 << ( shift_out - 2 ) ) ) >> ( shift_out - 1 ) ), curr_blk[i][j] );
				curr_blk[i][j] = Clip3( -255, 255, curr_blk[i][j] );
			}
		}
		inv_wavelet_B64( curr_blk );

	}
#else
	switch ( uiBitSize )
	{
	case 4:
		partialButterflyInverse8( curr_blk, tmp );
		partialButterflyInverse8( tmp, curr_blk );
		for ( i = 0; i < 8; i++ )
		{
			for ( j = 0; j < 8; j++ )
			{
				curr_blk[i][j] = sign( ( int )( ( abs( curr_blk[i][j] ) + ( 1 << ( shift_out - 2 ) ) ) >> ( shift_out - 1 ) ), curr_blk[i][j] );
				curr_blk[i][j] = Clip3( -255, 255, curr_blk[i][j] );
			}
		}
		break;
	case 5:
		partialButterflyInverse16( curr_blk, tmp );
		partialButterflyInverse16( tmp, curr_blk );
		for ( i = 0; i < 16; i++ )
		{
			for ( j = 0; j < 16; j++ )
			{
				curr_blk[i][j] = sign( ( int )( ( abs( curr_blk[i][j] ) + ( 1 << ( shift_out - 2 ) ) ) >> ( shift_out - 1 ) ), curr_blk[i][j] );
				curr_blk[i][j] = Clip3( -255, 255, curr_blk[i][j] );
			}
		}
		break;
	case 6:
		partialButterflyInverse32( curr_blk, tmp );
		partialButterflyInverse32( tmp, curr_blk );
		for ( i = 0; i < 32; i++ )
		{
			for ( j = 0; j < 32; j++ )
			{
				curr_blk[i][j] = sign( ( int )( ( abs( curr_blk[i][j] ) + ( 1 << ( shift_out - 2 ) ) ) >> ( shift_out - 1 ) ), curr_blk[i][j] );
				curr_blk[i][j] = Clip3( -255, 255, curr_blk[i][j] );
			}
		}
	}
#endif

	// normalize
	for ( yy = 0; yy < Size; yy++ )
	{
		for ( xx = 0; xx < Size; xx++ )
		{
			if ( block8x8 <= 3 )
			{
				curr_val = img->predBlock[b8_y + yy][b8_x + xx] + curr_blk[yy][xx];
			}
			else
			{
				curr_val = img->predBlock[yy][xx] + curr_blk[yy][xx];
			}

			img->resiY[pixInSMB_y + yy][pixInSMB_x + xx] = curr_blk[yy][xx] = clamp ( curr_val, 0, 255 );

			if ( block8x8 <= 3 )
			{
				imgY[img->pix_y + b8_y + yy][img->pix_x + b8_x + xx] = ( unsigned char ) curr_blk[yy][xx];
			}
			else
			{
				imgUV[ ( block8x8 - 4 ) % 2][img->pix_c_y + yy][img->pix_c_x + xx] = ( unsigned char ) curr_blk[yy][xx];
			}

		}
	}
	free_mem2Dint ( tmp );
}

#endif

void idct_dequant_B8_NSQT ( int block8x8, int **curr_blk, int uiBitSize )
{
	int  xx, yy;
	int  iSizeX, iSizeY;
	int  iStartX,iStartY;
	int  iHor = 0, iVer = 0;
	int  curr_val;
	int pixInSMB_x = 0 ;
	int pixInSMB_y = 0 ;
	codingUnit * currMB = &img->mb_data[img->current_mb_nr];
  int isChroma = block8x8 <=3 ? 0 : 1;
  currMB->l_ipred_mode = currMB->intra_pred_modes[block8x8];

	get_mb_pos ( img->current_mb_nr, &pixInSMB_x, &pixInSMB_y, input->g_uiMaxSizeInBit );
	pixInSMB_x = img->pix_x - pixInSMB_x;
	pixInSMB_y = img->pix_y - pixInSMB_y;
	get_mem2Dint ( &tmp, MAX_CU_SIZE, MAX_CU_SIZE );

	if (currMB->cuType == P2NXN || currMB->cuType == PHOR_UP || currMB->cuType == PHOR_DOWN||currMB->cuType==InNxNMB)
	{
		iHor = 1;
	}
	else if (currMB->cuType == PNX2N || currMB->cuType == PVER_LEFT || currMB->cuType == PVER_RIGHT||currMB->cuType==INxnNMB)
	{
		iVer = 1;
	}

	if (iHor == 1)
	{
		iStartX = 0;
		iStartY = block8x8 * (1 << (uiBitSize - 1));
		iSizeX  = (uiBitSize == B32X32_IN_BIT) ? (1 << uiBitSize) : (1 << (uiBitSize + 1));
		iSizeY  = (uiBitSize == B32X32_IN_BIT) ? (1 << (uiBitSize -2 )) : (1 << (uiBitSize - 1));
	}
	else if (iVer == 1)
	{
		iStartX = block8x8 * (1 << (uiBitSize - 1));
		iStartY = 0;
		iSizeX  = (uiBitSize == B32X32_IN_BIT) ? (1 << (uiBitSize -2 )) : (1 << (uiBitSize - 1));
		iSizeY  = (uiBitSize == B32X32_IN_BIT) ? (1 << uiBitSize) : (1 << (uiBitSize + 1));
	}

	// inverse transform
#if EXTEND_BD
  inv_transform_NSQT(curr_blk, uiBitSize, currMB, isChroma, b_secT_enabled, input->sample_bit_depth);
#else
	inv_transform_NSQT(curr_blk, uiBitSize, currMB, isChroma, b_secT_enabled);
#endif

#if EXTRACT_lcu_info_debug
  if (1)
  {
    int x, y;
    fprintf(p_lcu_debug, "Resi  CU block8x8=%d \n", block8x8);
    if (block8x8 <= 3)
      fprintf(p_lcu_debug, "InterResi Y  CU block8x8=%d \n", block8x8);
    else
      fprintf(p_lcu_debug, "InterResi UV CU block8x8=%d \n", block8x8);

    for (yy = 0; yy < iSizeY; yy++)
    {
      for (xx = 0; xx < iSizeX; xx++)
      {

        fprintf(p_lcu_debug, "%4d", curr_blk[yy][xx]);
      }
      fprintf(p_lcu_debug, "\n");
    }
  }
#endif  

	if ( uiBitSize == 5)
	{
		iSizeX = iSizeX << 1;
		iSizeY = iSizeY << 1;
	}

	// normalize

	for ( yy = 0; yy < iSizeY; yy++ )
	{
		for ( xx = 0; xx < iSizeX; xx++ )
		{
			if ( block8x8 <= 3 )
			{
				curr_val = img->predBlock[iStartY + yy][iStartX + xx] + curr_blk[yy][xx];
			}
			else
			{
				curr_val = img->predBlock[yy][xx] + curr_blk[yy][xx];
			}
#if EXTEND_BD
			curr_blk[yy][xx] = clamp ( curr_val, 0, (1 << (input->sample_bit_depth)) - 1 );
#else
			curr_blk[yy][xx] = clamp ( curr_val, 0, 255 );
#endif

			if ( block8x8 <= 3 )
			{
#if EXTEND_BD
				imgY[img->pix_y + iStartY + yy][img->pix_x + iStartX + xx] = ( byte ) curr_blk[yy][xx];
#else
				imgY[img->pix_y + iStartY + yy][img->pix_x + iStartX + xx] = ( unsigned char ) curr_blk[yy][xx];
#endif
			}
			else
			{
#if EXTEND_BD
				imgUV[ ( block8x8 - 4 ) % 2][img->pix_c_y + yy][img->pix_c_x + xx] = ( byte ) curr_blk[yy][xx];
#else
				imgUV[ ( block8x8 - 4 ) % 2][img->pix_c_y + yy][img->pix_c_x + xx] = ( unsigned char ) curr_blk[yy][xx];
#endif
			}

		}
	}

#if EXTRACT_lcu_info_debug
  if (1)
  {
    int x, y;
    fprintf(p_lcu_debug, "Resi  CU block8x8=%d \n", block8x8);
    if (block8x8 <= 3)
      fprintf(p_lcu_debug, "InterResi Y  CU block8x8=%d \n", block8x8);
    else
      fprintf(p_lcu_debug, "InterResi UV CU block8x8=%d \n", block8x8);

    for (yy = 0; yy < iSizeY; yy++)
    {
      for (xx = 0; xx < iSizeX; xx++)
      {
        fprintf(p_lcu_debug, "%4d", curr_blk[yy][xx]);
      }
      fprintf(p_lcu_debug, "\n");
    }

    for (yy = 0; yy < iSizeY; yy++)
    {
      for (xx = 0; xx < iSizeX; xx++)
      {
        curr_val = img->predBlock[iStartY + yy][iStartX + xx] + curr_blk[yy][xx];
        fprintf(p_lcu_debug, "%4d + %4d = %4d\n", img->predBlock[iStartY + yy][iStartX + xx], curr_blk[yy][xx], curr_val);
      }
      fprintf(p_lcu_debug, "\n");
    }
  }
#endif
  free_mem2Dint ( tmp );
}
/*
*************************************************************************
* Function:Copy region of img->resiY corresponding to block8x8 to curr_blk[][].
* Input:
* Output:
* Return:
* Attention:img->resiY is [x][y] and curr_blk is [y][x].
*************************************************************************
*/

void get_curr_blk ( int block8Nx8N, int **curr_blk, int uiBitSize )
{
  int  xx, yy;
  int  mb_y       = ( block8Nx8N / 2 ) << ( uiBitSize  );
  int  mb_x       = ( block8Nx8N % 2 ) << ( uiBitSize );
  int  Size = ( 1 << uiBitSize );
  int pixInSMB_x = 0 ;
  int pixInSMB_y = 0 ;
  int iHor = 0, iVer = 0;
  int iSizeX, iSizeY;
  int iStartX,iStartY;
  codingUnit * currMB = &img->mb_data[img->current_mb_nr];
  if ((input->useNSQT && currMB->ui_MbBitSize > B8X8_IN_BIT && block8Nx8N < 4 && currMB->trans_size == 1 && IS_INTER(currMB) && (currMB->cuType == P2NXN || currMB->cuType == PHOR_UP || currMB->cuType == PHOR_DOWN)))
  {
    iHor = 1;
  }
  else if ((input->useNSQT && currMB->ui_MbBitSize > B8X8_IN_BIT && currMB->ui_MbBitSize > B8X8_IN_BIT && block8Nx8N < 4 && currMB->trans_size == 1 && (IS_INTER(currMB)) &&(currMB->cuType == PNX2N || currMB->cuType == PVER_LEFT || currMB->cuType == PVER_RIGHT)))
  {
    iVer = 1;
  }
  else 
	  if (input->useSDIP && block8Nx8N < 4 && currMB->trans_size == 1 && IS_INTRA(currMB) && (currMB->cuType == InNxNMB))
	  {
		  iHor = 1;
	  }
	  else if (input->useSDIP && block8Nx8N < 4 && currMB->trans_size == 1 && (IS_INTRA(currMB)) &&(currMB->cuType == INxnNMB))
	  {
		  iVer = 1;
	  }
  if (iHor == 1)
  {
    iStartX = 0;
    iStartY = (uiBitSize == B32X32_IN_BIT) ? (block8Nx8N * (1 << (uiBitSize - 2))) : (block8Nx8N * (1 << (uiBitSize - 1)));
    iSizeX  = (uiBitSize == B32X32_IN_BIT) ? (1 << uiBitSize) : (1 << (uiBitSize + 1));
    iSizeY  = (uiBitSize == B32X32_IN_BIT) ? (1 << (uiBitSize -2 )) : (1 << (uiBitSize - 1));
  }
  else if (iVer == 1)
  {
    iStartX = (uiBitSize == B32X32_IN_BIT) ? (block8Nx8N * (1 << (uiBitSize - 2))) : (block8Nx8N * (1 << (uiBitSize - 1)));
    iStartY = 0;
    iSizeX  = (uiBitSize == B32X32_IN_BIT) ? (1 << (uiBitSize -2 )) : (1 << (uiBitSize - 1));
    iSizeY  = (uiBitSize == B32X32_IN_BIT) ? (1 << uiBitSize) : (1 << (uiBitSize + 1));
  }
  get_mb_pos ( img->current_mb_nr, &pixInSMB_x, &pixInSMB_y, input->g_uiMaxSizeInBit );
  pixInSMB_x = img->pix_x - pixInSMB_x;
  pixInSMB_y = img->pix_y - pixInSMB_y;

  if ((input->useNSQT && currMB->ui_MbBitSize > B8X8_IN_BIT &&( iHor || iVer)))
  {
    for ( yy = 0; yy < iSizeY; yy++ )
    {
      for ( xx = 0; xx < iSizeX; xx++ )
      {
        if ( block8Nx8N <= 3 )
        {
          curr_blk[yy][xx] = img->resiY[pixInSMB_y + iStartY + yy][pixInSMB_x + iStartX + xx];
        }
        else
        {
          curr_blk[yy][xx] = img->resiUV[block8Nx8N - 4][pixInSMB_y / 2 + yy][pixInSMB_x / 2 + xx];
        }
      }
    }
  }
  else 
	  if(input->useSDIP &&( iHor || iVer))
	  {
		  for ( yy = 0; yy < iSizeY; yy++ )
		  {
			  for ( xx = 0; xx < iSizeX; xx++ )
			  {
				  if ( block8Nx8N <= 3 )
				  {
					  curr_blk[yy][xx] = img->resiY[pixInSMB_y + iStartY + yy][pixInSMB_x + iStartX + xx];
				  }
				  else
				  {
					  curr_blk[yy][xx] = img->resiUV[block8Nx8N - 4][pixInSMB_y / 2 + yy][pixInSMB_x / 2 + xx];
				  }
			  }
		  }
	  }
	  else
  {
    for ( yy = 0; yy < Size; yy++ )
    {
      for ( xx = 0; xx < Size; xx++ )
      {
        if ( block8Nx8N <= 3 )
        {
          curr_blk[yy][xx] = img->resiY[pixInSMB_y + mb_y + yy][pixInSMB_x + mb_x + xx];
        }
        else
        {
          curr_blk[yy][xx] = img->resiUV[block8Nx8N - 4][pixInSMB_y / 2 + yy][pixInSMB_x / 2 + xx];
        }
      }
    }
  }
}

/*
*************************************************************************
* Function:Make Intra prediction for all 5 modes for 8*8 blocks.
bs_x and bs_y may be only 4 and 8.
img_x and img_y are pixels offsets in the picture.
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
int intrapred ( int img_x, int img_y, int uiBitSize ) //need to fix !!!
{
  //short * edgepixels = ( short * ) malloc ( ( ( 1 << uiBitSize ) * 4 + 1 ) * sizeof ( short ) );
  short edgepixels[MAX_CU_SIZE*4+1];
#define EP ( edgepixels + ( ( 1 << uiBitSize ) * 2 ) )
  int x, y;
  unsigned int x_off, y_off;
  int i, predmode;
#if !D20141230_BUG_FIX
  int block_available_up, block_available_up_right;
  int block_available_left, block_available_left_down;
#else 
  int block_available_up=0, block_available_up_right=0;
  int block_available_left=0, block_available_left_down=0;
#endif 
  int bs_x, bs_y;
  codingUnit *currMB   = &img->mb_data[img->current_mb_nr];/*lgp*/
  int MBRowSize = img->width / MIN_CU_SIZE;/*lgp*/
  int p_avai[5];
  int block_available_up_left = 0;
  int **piPredBuf;
  int  uiBitSize1= currMB->cuType==INxnNMB ? uiBitSize-2 :uiBitSize;
  int  uiBitSize2= currMB->cuType==InNxNMB ? uiBitSize-2 :uiBitSize;
  int NumMBWidthInBlk1 = 1 << ( uiBitSize1 - MIN_BLOCK_SIZE_IN_BIT );
  int NumMBWidthInBlk2 = 1 << ( uiBitSize2 - MIN_BLOCK_SIZE_IN_BIT );

  bs_x = 1 << uiBitSize1;
  bs_y = 1 << uiBitSize2;
  x_off = img_x % (1 << (currMB->ui_MbBitSize));
  y_off = img_y % (1 << (currMB->ui_MbBitSize));
  predmode = img->ipredmode[img_y / (MIN_BLOCK_SIZE)+1][img_x / (MIN_BLOCK_SIZE)+1];
  getIntraNeighborAvailabilities(currMB, input->g_uiMaxSizeInBit, img_x, img_y, bs_x, bs_y, p_avai);

  block_available_left_down = p_avai[NEIGHBOR_INTRA_LEFT_DOWN];
  block_available_left      = p_avai[NEIGHBOR_INTRA_LEFT];
  block_available_up_left   = p_avai[NEIGHBOR_INTRA_UP_LEFT];
  block_available_up        = p_avai[NEIGHBOR_INTRA_UP];
  block_available_up_right  = p_avai[NEIGHBOR_INTRA_UP_RIGHT];

	currMB->block_available_up = block_available_up;
	currMB->block_available_left = block_available_left;
  for ( i = -2 * bs_y; i <= 2 * bs_x; i++ )
  {
#if EXTEND_BD
    EP[i] = 1 << (input->sample_bit_depth - 1);
#else
		EP[i] = 128;
#endif
  }
    //get prediciton pixels
    if ( block_available_up )
    {
      for ( x = 0; x < bs_x; x++ )
      {
        EP[x + 1] = imgY[img_y - 1][img_x + x];
      }

#if !BUGFIX_INTRA_REF_SAMPLES
      if ( block_available_up_right )
      {
        for ( x = 0; x < bs_x; x++ )
        {
          if ( img_x + bs_x + x >= img->width )
          {
            EP[1 + x + bs_x] = imgY[img_y - 1][img->width - 1];
          }
          else
          {
            EP[1 + x + bs_x] = imgY[img_y - 1][img_x + bs_x + x];
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

      if ( block_available_left )
      {
        EP[0] = imgY[img_y - 1][img_x - 1];
      }
      else
      {
        EP[0] = imgY[img_y - 1][img_x];
      }
#endif
    }

#if BUGFIX_INTRA_REF_SAMPLES
    if (block_available_up_right) 
    {
        for (x = 0; x < bs_x; x++) 
        {
            if (img_x + bs_x + x >= img->width) 
            {
                EP[1 + x + bs_x] = imgY[img_y - 1][img->width - 1];
            } 
            else 
            {
                EP[1 + x + bs_x] = imgY[img_y - 1][img_x + bs_x + x];
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

    if ( block_available_left )
    {
      for ( y = 0; y < bs_y; y++ )
      {
        EP[-1 - y] = imgY[img_y + y][img_x - 1];
      }

#if !BUGFIX_INTRA_REF_SAMPLES
      if ( block_available_left_down )
      {
        for ( y = 0; y < bs_y; y++ )
        {
          if ( img_y + bs_y + y >= img->height )
          {
            EP[-1 - y - bs_y] = imgY[img->height - 1][img_x - 1];
          }
          else
          {
            EP[-1 - y - bs_y] = imgY[img_y + bs_y + y][img_x - 1];
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
      if ( !block_available_up )
      {
        EP[0] = imgY[img_y][img_x - 1];
      }
#endif
    }

#if BUGFIX_INTRA_REF_SAMPLES
    if (block_available_left_down) 
    {
        for (y = 0; y < bs_y; y++) 
        {
            if (img_y + bs_y + y >= img->height) 
            {
                EP[-1 - y - bs_y] = imgY[img->height - 1][img_x - 1];
            }
            else 
            {
                EP[-1 - y - bs_y] = imgY[img_y + bs_y + y][img_x - 1];
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
      if ( block_available_up && block_available_left )
      {
        EP[0] = imgY[img_y - 1][img_x - 1];
      }
    }
    else
#endif
    {
      if ( block_available_up_left )
      {
        EP[0] = imgY[img_y - 1][img_x - 1];
      }
      else if ( block_available_up )
      {
        EP[0] = imgY[img_y - 1][img_x];
      }
      else if ( block_available_left )
      {
        EP[0] = imgY[img_y][img_x - 1];
      }
    }

    get_mem2Dint ( &piPredBuf, bs_y, bs_x );
    for ( y = 0; y < bs_y; y++ )
    {
      for ( x = 0; x < bs_x; x++ )
      {
        piPredBuf[y][x] = 0;
      }
    }
#if EXTEND_BD
	predIntraLumaAdi( EP, piPredBuf, predmode, uiBitSize, block_available_up, block_available_left,bs_y,bs_x, input->sample_bit_depth );
#else
	predIntraLumaAdi( EP, piPredBuf, predmode, uiBitSize, block_available_up, block_available_left,bs_y,bs_x );
#endif
    for ( y = 0; y < bs_y; y++ )
    {
      for ( x = 0; x < bs_x; x++ )
      {
        img->predBlock[y + y_off][x + x_off] = piPredBuf[y][x];
#if EXTRACT_PIC_YUV_PRED   //2016-5-23 提取整帧图像的经过预测后的数据
        pPicYPred[(img_y + y)*img->width + img_x + x] = piPredBuf[y][x];
#endif
      }
    }
#if EXTRACT_lcu_info_debug
    if (1)
    {
      /*
      fprintf(p_lcu_debug, "IntraY-XX0 left\nIntraY-XX0 ");
      for (y = 0; y >= (-2*bs_y); y--)
      {
        fprintf(p_lcu_debug, "%4d,", EP[y]);
      }
      fprintf(p_lcu_debug, "\nIntraY-XX0 ");
      fprintf(p_lcu_debug, "up\nIntraY-XX0 ");
      for (y = 0; y <= (2 * bs_x); y++)
      {
        fprintf(p_lcu_debug, "%4d,", EP[y]);
      }
      */
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
      fprintf(p_lcu_debug, "\nIntraY-XX0 ");
      fprintf(p_lcu_debug, "Intra Y CU posx,y=%d %d  blk x,y=%d %d\nIntraY-XX0 ", img_x, img_y, bs_x, bs_y);
      for (y = 0; y < bs_y; y++)
      {
        for (x = 0; x < bs_x; x++)
        {
          //img->predBlock[y + y_off][x + x_off] = piPredBuf[y][x];
          fprintf(p_lcu_debug, "%4d", piPredBuf[y][x]);
        }
        if(y== bs_y-1)
          fprintf(p_lcu_debug, "\n");
        else
        fprintf(p_lcu_debug, "\nIntraY-XX0 ");
      }
    }
#endif
    free_mem2Dint( piPredBuf );
    //free ( edgepixels );
    return DECODING_OK;
  }

/// ADI
