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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <memory.h>

#include "../../lcommon/inc/defines.h"
#include "../../lcommon/inc/commonVariables.h"
#include "../../lcommon/inc/memalloc.h"
#include "../../lcommon/inc/transform.h"
#include "../../lcommon/inc/intra-prediction.h"
#include "global.h"
#include "rdopt_coding_state.h"
#include "vlc.h"
#include "block.h"
#include "codingUnit.h"
#include "rdoq.h"

#if FREQUENCY_WEIGHTING_QUANTIZATION
#include "wquant.h"
#endif


#define max(a, b)                   ((a) > (b) ? (a) : (b))  //!< Macro returning max value 
#define min(a, b)                   ((a) < (b) ? (a) : (b))  //!< Macro returning min value 

#if EXTEND_BD
void quant_B8 ( int qp, int mode, int **curr_blk, int iSizeX, int iSizeY, int uiBitSize, int sample_bit_depth )
#else
void quant_B8 ( int qp, int mode, int **curr_blk, int iSizeX, int iSizeY, int uiBitSize )
#endif
{
	int xx, yy;
	int val, temp;
	int qp_const;
	int shift3;
	int WQMSize;

	// Adaptive frequency weighting quantization
#if FREQUENCY_WEIGHTING_QUANTIZATION
	int **levelscale;
	int intra=0;
	int levelscale_coef;

	intra=(mode>3)?1:0;
	if (WeightQuantEnable)
	{
		if(uiBitSize==B4X4_IN_BIT)
		{
			levelscale=LevelScale4x4[intra];
			WQMSize=1<<2;
		}
		else if(uiBitSize==B8X8_IN_BIT)
		{
			levelscale=LevelScale8x8[intra];
			WQMSize=1<<3;
		}
		else if(uiBitSize==B16X16_IN_BIT)
		{
			levelscale=LevelScale16x16[intra];
			WQMSize=1<<4;
		}
		else if(uiBitSize==B32X32_IN_BIT)
		{
			levelscale=LevelScale32x32[intra];
			WQMSize=1<<5;
		}
	}
#endif


#if EXTEND_BD
	shift3 = 15 + LIMIT_BIT - (sample_bit_depth + 1) - uiBitSize;
#else
	shift3 = 15 + LIMIT_BIT - RESID_BIT - uiBitSize;
#endif
	qp_const = (mode > 3) ? ( 1 << shift3 ) * 10 / 31 : ( 1 << shift3 ) * 10 / 62;

	for ( yy = 0; yy < iSizeY; yy++ )
	{
		for ( xx = 0; xx < iSizeX; xx++ )
		{
			val = curr_blk[yy][xx];
			temp = absm ( val );

// Adaptive frequency weighting quantization
#if FREQUENCY_WEIGHTING_QUANTIZATION
			if(WeightQuantEnable)
			{
#if AWQ_WEIGHTING
				//levelscale_coef=levelscale[(yy&(WQMSize-1))*WQMSize+(xx&(WQMSize-1))];
				levelscale_coef=levelscale[yy&(WQMSize-1)][xx&(WQMSize-1)];
				curr_blk[yy][xx] = sign ( ( ( ( ( temp * levelscale_coef + ( 1 << 18 ) ) >> 19 ) * Q_TAB[qp] + qp_const ) >> shift3 ), val );
#else
				curr_blk[yy][xx] = sign ( ( ( temp * Q_TAB[qp] + qp_const ) >> shift3 ), val );
#endif
			}
			else
			{
				curr_blk[yy][xx] = sign ( ( ( temp * Q_TAB[qp] + qp_const ) >> shift3 ), val );
			}
#if QuantClip
      curr_blk[yy][xx] = Clip3(-32768, 32767, curr_blk[yy][xx]);
#endif
#else
			curr_blk[yy][xx] = sign ( ( ( temp * Q_TAB[qp] + qp_const ) >> shift3 ), val );
#endif

		}
	}
}


void quantization (int qp, int mode, int b8, int **curr_blk,   unsigned int uiBitSize, unsigned int uiPositionInPic, codingUnit * currMB, int isChroma, int intraPredMode)
{
	int iSizeX, iSizeY;
	int iBlockType = currMB->cuType;

	if (input->useNSQT && currMB->ui_MbBitSize > B8X8_IN_BIT  && !isChroma && (( iBlockType == P2NXN || iBlockType == PHOR_UP || iBlockType == PHOR_DOWN) &&  b8 < 4 && currMB->trans_size ==1))
	{
		iSizeX       = (uiBitSize == B32X32_IN_BIT) ? ( 1 << (uiBitSize) ) : ( 1 << (uiBitSize + 1 ) );
		iSizeY       = (uiBitSize == B32X32_IN_BIT) ? ( 1 << (uiBitSize - 2 ) ) : ( 1 << (uiBitSize - 1 ) );
	}
	else if (input->useNSQT  && currMB->ui_MbBitSize > B8X8_IN_BIT && !isChroma && ( (iBlockType == PNX2N || iBlockType ==PVER_LEFT || iBlockType ==PVER_RIGHT) && b8 < 4 && currMB->trans_size ==1))
	{
		iSizeX       = (uiBitSize == B32X32_IN_BIT) ? ( 1 << (uiBitSize - 2 ) ) : ( 1 << (uiBitSize - 1 ) );
		iSizeY       = (uiBitSize == B32X32_IN_BIT) ? ( 1 << (uiBitSize) ) : ( 1 << (uiBitSize + 1 ) );
	}
	else 
		if (input->useSDIP  && !isChroma  && iBlockType == InNxNMB && currMB->trans_size ==1 &&  b8 < 4) 
		{
			iSizeX       = (uiBitSize == B32X32_IN_BIT) ? ( 1 << (uiBitSize) ) : ( 1 << (uiBitSize + 1 ) );
			iSizeY       = (uiBitSize == B32X32_IN_BIT) ? ( 1 << (uiBitSize - 2 ) ) : ( 1 << (uiBitSize - 1 ) );
		}
		else if (input->useSDIP   && !isChroma && iBlockType == INxnNMB&& currMB->trans_size ==1 &&  b8 < 4 )
		{
			iSizeX       = (uiBitSize == B32X32_IN_BIT) ? ( 1 << (uiBitSize - 2 ) ) : ( 1 << (uiBitSize - 1 ) );
			iSizeY       = (uiBitSize == B32X32_IN_BIT) ? ( 1 << (uiBitSize) ) : ( 1 << (uiBitSize + 1 ) );
		}
		else
		{
			iSizeX       = ( 1 << uiBitSize );
			iSizeY       = ( 1 << uiBitSize );

		}

		if ( input->use_rdoq)
		{
#if TRANS_16_BITS
			rdoq_block( qp, mode, curr_blk, uiBitSize, isChroma, uiPositionInPic, intraPredMode, currMB );
#else
			if ( uiBitSize == B64X64_IN_BIT )
			{
				rdoq_block( qp, mode, curr_blk, uiBitSize - 1, isChroma, uiPositionInPic );
			}
			else
			{
				rdoq_block( qp, mode, curr_blk, uiBitSize, isChroma, uiPositionInPic, intraPredMode );
			}
#endif
		}
		else
		{
#if EXTEND_BD
			quant_B8( qp, mode, curr_blk, iSizeX, iSizeY, uiBitSize, input->sample_bit_depth);
#else
			quant_B8( qp, mode, curr_blk, iSizeX, iSizeY, uiBitSize);
#endif
		}
}

int  inverse_quantization (int qp, int mode, int b8, int **curr_blk, int scrFlag, int *cbp,  unsigned int uiBitSize, unsigned int uiPositionInPic, codingUnit * currMB, int isChroma)
{   
	int  run;
	int  xx, yy;
	int  icoef, ipos;
	int  b8_y          = ( b8 / 2 ) << uiBitSize;
	int  b8_x          = ( b8 % 2 ) << uiBitSize;
	int  coeff_cost    = 0;
	int coeff_num = ( 1 << uiBitSize ) * ( 1 << uiBitSize ) + 1;
	//int *ACLevel = ( int * ) malloc ( coeff_num * sizeof ( int ) );
	//int *ACRun = ( int * ) malloc ( coeff_num * sizeof ( int ) );
  int ACLevel[MAX_CU_SIZE*MAX_CU_SIZE+1];
  int ACRun[MAX_CU_SIZE*MAX_CU_SIZE+1];
	int **AVS_SCAN;
	int iSizeX, iSizeY;
	int iVer=0, iHor=0;
	int iBlockType = currMB->cuType;

// Adaptive frequency weighting quantization
#if FREQUENCY_WEIGHTING_QUANTIZATION
	int **levelscale;
	int intra;
	int wqm_coef;
	int WQMSizeId,WQMSize;
	int iStride;
	int wqm_shift;
#endif

	int  curr_val;
	int clip1 = 0 - ( 1 << 15 );
	int clip2 = ( 1 << 15 ) - 1;


	if (input->useNSQT && currMB->ui_MbBitSize > B8X8_IN_BIT  && !isChroma && (( iBlockType == P2NXN || iBlockType == PHOR_UP || iBlockType == PHOR_DOWN) &&  b8 < 4 && currMB->trans_size ==1))
	{
		b8_y = (uiBitSize == B32X32_IN_BIT) ? (b8 * ( 1 << (uiBitSize - 2) )) : (b8 * ( 1 << (uiBitSize - 1) ));
		b8_x = 0;
		iSizeX       = (uiBitSize == B32X32_IN_BIT) ? ( 1 << (uiBitSize) ) : ( 1 << (uiBitSize + 1 ) );
		iSizeY       = (uiBitSize == B32X32_IN_BIT) ? ( 1 << (uiBitSize - 2 ) ) : ( 1 << (uiBitSize - 1 ) );
		iHor         = 1;
	}
	else if (input->useNSQT  && currMB->ui_MbBitSize > B8X8_IN_BIT && !isChroma && ( (iBlockType == PNX2N || iBlockType ==PVER_LEFT || iBlockType ==PVER_RIGHT) && b8 < 4 && currMB->trans_size ==1))
	{
		b8_x = (uiBitSize == B32X32_IN_BIT) ? (b8 * ( 1 << (uiBitSize - 2) )) : (b8 * ( 1 << (uiBitSize - 1) ));
		b8_y = 0;
		iSizeX       = (uiBitSize == B32X32_IN_BIT) ? ( 1 << (uiBitSize - 2 ) ) : ( 1 << (uiBitSize - 1 ) );
		iSizeY       = (uiBitSize == B32X32_IN_BIT) ? ( 1 << (uiBitSize) ) : ( 1 << (uiBitSize + 1 ) );
		iVer         = 1;
	}
	else 
		if (input->useSDIP  && !isChroma  && iBlockType == InNxNMB && currMB->trans_size ==1 &&  b8 < 4) 
		{
			b8_y = (uiBitSize == B32X32_IN_BIT) ? (b8 * ( 1 << (uiBitSize - 2) )) : (b8 * ( 1 << (uiBitSize - 1) ));
			b8_x = 0;
			iSizeX       = (uiBitSize == B32X32_IN_BIT) ? ( 1 << (uiBitSize) ) : ( 1 << (uiBitSize + 1 ) );
			iSizeY       = (uiBitSize == B32X32_IN_BIT) ? ( 1 << (uiBitSize - 2 ) ) : ( 1 << (uiBitSize - 1 ) );
			iHor         = 1;
		}
		else if (input->useSDIP   && !isChroma && iBlockType == INxnNMB&& currMB->trans_size ==1 &&  b8 < 4 )
		{
			b8_x = (uiBitSize == B32X32_IN_BIT) ? (b8 * ( 1 << (uiBitSize - 2) )) : (b8 * ( 1 << (uiBitSize - 1) ));
			b8_y = 0;
			iSizeX       = (uiBitSize == B32X32_IN_BIT) ? ( 1 << (uiBitSize - 2 ) ) : ( 1 << (uiBitSize - 1 ) );
			iSizeY       = (uiBitSize == B32X32_IN_BIT) ? ( 1 << (uiBitSize) ) : ( 1 << (uiBitSize + 1 ) );
			iVer         = 1;
		}
		else
		{
			b8_y          = ( b8 / 2 ) << uiBitSize;
			b8_x          = ( b8 % 2 ) << uiBitSize;
			iSizeX       = ( 1 << uiBitSize );
			iSizeY       = ( 1 << uiBitSize );

		}

// Adaptive frequency weighting quantization
#if FREQUENCY_WEIGHTING_QUANTIZATION
		intra=(mode>3)?1:0;
		if(WeightQuantEnable)
		{
#if !WQ_MATRIX_FCD
			wqm_shift=(input->PicWQDataIndex==1)?3:0;
#else
			wqm_shift=(input->PicWQDataIndex==1)?2:0;
#endif
		}
#endif
		// Scan
		mode %= 4; // mode+=4 is used to signal intra coding to quant_B8().

		for ( yy = 0; yy < iSizeY; yy++ )
		{
			for ( xx = 0; xx < iSizeX; xx++ )
			{
				img->Coeff_all[yy + b8_y][xx + b8_x] = curr_blk[yy][xx];
			}
		}

		// General block information
		for ( xx = 0; xx < coeff_num; xx++ )
		{
			ACRun[xx] = ACLevel[xx] = 0;
		}

		run  = -1;
		ipos = 0;
		if ((input->useNSQT  && currMB->ui_MbBitSize > B8X8_IN_BIT &&  iHor ))
		{
			if ( uiBitSize == B8X8_IN_BIT )
			{
				AVS_SCAN = AVS_SCAN4x16;
			}
			else if ( uiBitSize == B16X16_IN_BIT )
			{
				AVS_SCAN = AVS_SCAN8x32;
			}
			else if ( uiBitSize == B32X32_IN_BIT )
			{
				coeff_num = ( 1 << ( uiBitSize - 1 ) ) * ( 1 << ( uiBitSize - 1 ) ) + 1;
				AVS_SCAN = AVS_SCAN8x32;
			}
#if FREQUENCY_WEIGHTING_QUANTIZATION
			if ( uiBitSize == B8X8_IN_BIT )
				WQMSizeId=2;
			else if ( (uiBitSize == B16X16_IN_BIT)|| (uiBitSize == B32X32_IN_BIT))
				WQMSizeId=3;
#endif

		}
		else if (input->useNSQT  && currMB->ui_MbBitSize > B8X8_IN_BIT && ( iVer ))
		{
			if ( uiBitSize == B8X8_IN_BIT )
			{
				AVS_SCAN = AVS_SCAN16x4;
			}
			else if ( uiBitSize == B16X16_IN_BIT )
			{
				AVS_SCAN = AVS_SCAN32x8;
			}
			else if ( uiBitSize == B32X32_IN_BIT )
			{
				coeff_num = ( 1 << ( uiBitSize - 1 ) ) * ( 1 << ( uiBitSize - 1 ) ) + 1;
				AVS_SCAN = AVS_SCAN32x8;
			}
#if FREQUENCY_WEIGHTING_QUANTIZATION
			if ( uiBitSize == B8X8_IN_BIT )
				WQMSizeId=2;
			else if ( (uiBitSize == B16X16_IN_BIT)|| (uiBitSize == B32X32_IN_BIT))
				WQMSizeId=3;
#endif
		}
		else
			if (input->useSDIP &&  iHor )
			{
				if ( uiBitSize == B8X8_IN_BIT )
				{
					AVS_SCAN = AVS_SCAN4x16;
				}
				else if ( uiBitSize == B16X16_IN_BIT )
				{
					AVS_SCAN = AVS_SCAN8x32;
				}
				else if ( uiBitSize == B32X32_IN_BIT )
				{
					coeff_num = ( 1 << ( uiBitSize - 1 ) ) * ( 1 << ( uiBitSize - 1 ) ) + 1;
					AVS_SCAN = AVS_SCAN8x32;
				}
#if FREQUENCY_WEIGHTING_QUANTIZATION
				if ( uiBitSize == B8X8_IN_BIT )
					WQMSizeId=2;
				else if ( (uiBitSize == B16X16_IN_BIT)|| (uiBitSize == B32X32_IN_BIT))
					WQMSizeId=3;
#endif
			}
			else if (input->useSDIP && iVer )
			{
				if ( uiBitSize == B8X8_IN_BIT )
				{
					AVS_SCAN = AVS_SCAN16x4;
				}
				else if ( uiBitSize == B16X16_IN_BIT )
				{
					AVS_SCAN = AVS_SCAN32x8;
				}
				else if ( uiBitSize == B32X32_IN_BIT )
				{
					coeff_num = ( 1 << ( uiBitSize - 1 ) ) * ( 1 << ( uiBitSize - 1 ) ) + 1;
					AVS_SCAN = AVS_SCAN32x8;
				}
#if FREQUENCY_WEIGHTING_QUANTIZATION
				if ( uiBitSize == B8X8_IN_BIT )
					WQMSizeId=2;
				else if ( (uiBitSize == B16X16_IN_BIT)|| (uiBitSize == B32X32_IN_BIT))
					WQMSizeId=3;
#endif
			}
			else
			{
#if FREQUENCY_WEIGHTING_QUANTIZATION

#if M3198_CU8
				if( uiBitSize == B4X4_IN_BIT )
				{
					AVS_SCAN = AVS_SCAN4x4;
					WQMSizeId=0;
				}
#endif
				if ( uiBitSize == B8X8_IN_BIT )
				{
					AVS_SCAN = AVS_CG_SCAN8x8;
					WQMSizeId=1;
				}
				else if ( uiBitSize == B16X16_IN_BIT )
				{
					AVS_SCAN = AVS_CG_SCAN16x16;
					WQMSizeId=2;
				}
				else if ( uiBitSize == B32X32_IN_BIT )
				{
					AVS_SCAN = AVS_CG_SCAN32x32;
					WQMSizeId=3;
				}
#if INTRA_2N
				else if ( uiBitSize == B64X64_IN_BIT )
				{
					AVS_SCAN = AVS_CG_SCAN32x32;
					coeff_num = ( 1 << ( uiBitSize - 1 ) ) * ( 1 << ( uiBitSize - 1 ) ) + 1;
					WQMSizeId=3;
				}
#endif

#else //FREQUENCY_WEIGHTING_QUANTIZATION
#if M3198_CU8
				if( uiBitSize == B4X4_IN_BIT )
				{
					AVS_SCAN = AVS_SCAN4x4;
				}
#endif
				if ( uiBitSize == B8X8_IN_BIT )
				{
					AVS_SCAN = AVS_CG_SCAN8x8;
				}
				else if ( uiBitSize == B16X16_IN_BIT )
				{
					AVS_SCAN = AVS_CG_SCAN16x16;
				}
				else if ( uiBitSize == B32X32_IN_BIT )
				{
					AVS_SCAN = AVS_CG_SCAN32x32;
				}
#if INTRA_2N
				else if ( uiBitSize == B64X64_IN_BIT )
				{
					AVS_SCAN = AVS_CG_SCAN32x32;
					coeff_num = ( 1 << ( uiBitSize - 1 ) ) * ( 1 << ( uiBitSize - 1 ) ) + 1;
				}
#endif
#endif //FREQUENCY_WEIGHTING_QUANTIZATION
			}

// Adaptive frequency weighting quantization
#if FREQUENCY_WEIGHTING_QUANTIZATION
		if (WeightQuantEnable)
		{
			WQMSize=1<<(WQMSizeId+2);

			if(WQMSizeId==0)
			{
				levelscale=LevelScale4x4[intra];
			}
			else if(WQMSizeId==1)
			{
				levelscale=LevelScale8x8[intra];
			}
			else if(WQMSizeId==2)
			{
				levelscale=LevelScale16x16[intra];
			}
			else if(WQMSizeId==3)
			{
				levelscale=LevelScale32x32[intra];
			}
		}
#endif

			for ( icoef = 0; icoef < ( coeff_num - 1 ); icoef++ )
			{
				run++;
				xx = AVS_SCAN[icoef][0];
				yy = AVS_SCAN[icoef][1];
// Adaptive frequency weighting quantization
#if FREQUENCY_WEIGHTING_QUANTIZATION
		if (WeightQuantEnable)
		{
			if((WQMSizeId==0)||(WQMSizeId==1))
			{
				iStride=WQMSize;
				wqm_coef=cur_wq_matrix[WQMSizeId][(yy&(iStride-1))*iStride+(xx&(iStride-1))];
			}
#if AWQ_LARGE_BLOCK_EXT_MAPPING
			else if (WQMSizeId==2)
			{
				iStride=WQMSize>>1;
				wqm_coef=cur_wq_matrix[WQMSizeId][((yy>>1)&(iStride-1))*iStride+((xx>>1)&(iStride-1))];
			}
			else if (WQMSizeId==3)
			{
				iStride=WQMSize>>2;
				wqm_coef=cur_wq_matrix[WQMSizeId][((yy>>2)&(iStride-1))*iStride+((xx>>2)&(iStride-1))];
			}
#endif

		}
#endif

				curr_val = curr_blk[yy][xx];

				if ( curr_val != 0 )
				{
					ACLevel[ipos] = curr_val;
					ACRun[ipos]   = run;

					if ( scrFlag && absm ( ACLevel[ipos] ) == 1 )
					{
						coeff_cost += 1;
					}
					else
					{
						coeff_cost += MAX_VALUE;  // block has to be saved
					}

					{
						int val, temp, shift, QPI;
						shift = IQ_SHIFT[qp];
						QPI   = IQ_TAB[qp];
						val = curr_blk[yy][xx];
// Adaptive frequency weighting quantization
#if FREQUENCY_WEIGHTING_QUANTIZATION
#if TRANS_16_BITS
#if EXTEND_BD
						shift = IQ_SHIFT[qp] + (input->sample_bit_depth + 1) + uiBitSize - LIMIT_BIT;
#else
						shift = IQ_SHIFT[qp] + RESID_BIT + uiBitSize - LIMIT_BIT;
#endif
#if M3175_SHIFT
						shift ++;
#endif
#if SHIFT_DQ_INV
						shift --;
#endif
						if(WeightQuantEnable)
						{
#if AWQ_WEIGHTING
							temp = ((((((long long int)val*wqm_coef)>>wqm_shift)*QPI)>>4) + (1<<(shift-1)) )>>shift; // dequantization, M2239, N1466
#else
							temp = ( val * QPI + ( 1 << ( shift - 1 ) ) ) >> shift; // dequantization
#endif
						}
						else
						{
							temp = ( val * QPI + ( 1 << ( shift - 1 ) ) ) >> shift; // dequantization
						}
#if QuantClip
            temp = Clip3(-32768, 32767, temp);
#endif
#else
						if(WeightQuantEnable)
						{
#if AWQ_WEIGHTING
							temp = ((((((int)val*wqm_coef)>>wqm_shift)*QPI)>>4) + (1<<(shift-1)) )>>shift; // dequantization, M2239, N1466
#else
							temp = ( val * QPI + ( 1 << ( shift - 2 ) ) ) >> ( shift - 1 ); // dequantization
#endif
						}
						else
						{
							temp = ( val * QPI + ( 1 << ( shift - 2 ) ) ) >> ( shift - 1 ); // dequantization
						}
#endif
#else //FREQUENCY_WEIGHTING_QUANTIZATION
#if TRANS_16_BITS
#if EXTEND_BD
						shift = IQ_SHIFT[qp] + (input->sample_bit_depth + 1) + uiBitSize - LIMIT_BIT;
#else
						shift = IQ_SHIFT[qp] + RESID_BIT + uiBitSize - LIMIT_BIT;
#endif
#if M3175_SHIFT
						shift ++;
#endif
#if SHIFT_DQ_INV
						shift --;
#endif

						temp = ( val * QPI + ( 1 << ( shift - 1 ) ) ) >> shift; // dequantization

#else
						temp = ( val * QPI + ( 1 << ( shift - 2 ) ) ) >> ( shift - 1 ); // dequantization
#endif
#endif //FREQUENCY_WEIGHTING_QUANTIZATION
						curr_blk[yy][xx] = temp;// dequantization & descale
						curr_blk[yy][xx] = Clip3 ( clip1, clip2, curr_blk[yy][xx] ); //rm52k
					}

					run = -1;
					ipos++;
				}
			}

			if ( ipos > 0 ) // there are coefficients
			{
				if ( b8 <= 3 )
				{
					( *cbp )     |= ( 1 << b8 );
				}
				else
				{
					( *cbp )     |= ( 1 << ( b8 ) );  /*lgp*dct*/
				}
			}

			//free ( ACLevel );
			//free ( ACRun );
			return coeff_cost;
}

void inverse_transform(  int b8, int **curr_blk, unsigned int uiBitSize, unsigned int uiPositionInPic, codingUnit * currMB, int isChroma)
{   int  b8_y          = ( b8 / 2 ) << uiBitSize;
int  b8_x          = ( b8 % 2 ) << uiBitSize;
int iSizeX, iSizeY;
int iVer=0, iHor=0;
int iBlockType = currMB->cuType;
int uiPixInPic_y;
int uiPixInPic_x;
int uiPixInPic_c_y = input->chroma_format == 1 ? ( ( ( uiPositionInPic / img->PicWidthInMbs ) * MIN_CU_SIZE ) >> 1 ) : ( ( uiPositionInPic / img->PicWidthInMbs ) * MIN_CU_SIZE );
int uiPixInPic_c_x = ( ( ( uiPositionInPic % img->PicWidthInMbs ) * MIN_CU_SIZE ) >> 1 );
int xx,yy;
int curr_val;
if (input->useNSQT && currMB->ui_MbBitSize > B8X8_IN_BIT  && !isChroma && (( iBlockType == P2NXN || iBlockType == PHOR_UP || iBlockType == PHOR_DOWN) &&  b8 < 4 && currMB->trans_size ==1))
{
	b8_y = (uiBitSize == B32X32_IN_BIT) ? (b8 * ( 1 << (uiBitSize - 2) )) : (b8 * ( 1 << (uiBitSize - 1) ));
	b8_x = 0;
	iSizeX       = (uiBitSize == B32X32_IN_BIT) ? ( 1 << (uiBitSize) ) : ( 1 << (uiBitSize + 1 ) );
	iSizeY       = (uiBitSize == B32X32_IN_BIT) ? ( 1 << (uiBitSize - 2 ) ) : ( 1 << (uiBitSize - 1 ) );
	iHor         = 1;
}
else if (input->useNSQT  && currMB->ui_MbBitSize > B8X8_IN_BIT && !isChroma && ( (iBlockType == PNX2N || iBlockType ==PVER_LEFT || iBlockType ==PVER_RIGHT) && b8 < 4 && currMB->trans_size ==1))
{
	b8_x = (uiBitSize == B32X32_IN_BIT) ? (b8 * ( 1 << (uiBitSize - 2) )) : (b8 * ( 1 << (uiBitSize - 1) ));
	b8_y = 0;
	iSizeX       = (uiBitSize == B32X32_IN_BIT) ? ( 1 << (uiBitSize - 2 ) ) : ( 1 << (uiBitSize - 1 ) );
	iSizeY       = (uiBitSize == B32X32_IN_BIT) ? ( 1 << (uiBitSize) ) : ( 1 << (uiBitSize + 1 ) );
	iVer         = 1;
}
else 
if (input->useSDIP  && !isChroma  && iBlockType == InNxNMB && currMB->trans_size ==1 &&  b8 < 4) 
{
	b8_y = (uiBitSize == B32X32_IN_BIT) ? (b8 * ( 1 << (uiBitSize - 2) )) : (b8 * ( 1 << (uiBitSize - 1) ));
	b8_x = 0;
	iSizeX       = (uiBitSize == B32X32_IN_BIT) ? ( 1 << (uiBitSize) ) : ( 1 << (uiBitSize + 1 ) );
	iSizeY       = (uiBitSize == B32X32_IN_BIT) ? ( 1 << (uiBitSize - 2 ) ) : ( 1 << (uiBitSize - 1 ) );
	iHor         = 1;
}
else if (input->useSDIP   && !isChroma && iBlockType == INxnNMB&& currMB->trans_size ==1 &&  b8 < 4 )
{
	b8_x = (uiBitSize == B32X32_IN_BIT) ? (b8 * ( 1 << (uiBitSize - 2) )) : (b8 * ( 1 << (uiBitSize - 1) ));
	b8_y = 0;
	iSizeX       = (uiBitSize == B32X32_IN_BIT) ? ( 1 << (uiBitSize - 2 ) ) : ( 1 << (uiBitSize - 1 ) );
	iSizeY       = (uiBitSize == B32X32_IN_BIT) ? ( 1 << (uiBitSize) ) : ( 1 << (uiBitSize + 1 ) );
	iVer         = 1;
}
else
{
	b8_y          = ( b8 / 2 ) << uiBitSize;
	b8_x          = ( b8 % 2 ) << uiBitSize;
	iSizeX       = ( 1 << uiBitSize );
	iSizeY       = ( 1 << uiBitSize );

}

if (input->useNSQT  && currMB->ui_MbBitSize > B8X8_IN_BIT && ( iVer || iHor))
{
#if EXTEND_BD
	inv_transform_NSQT(curr_blk, uiBitSize, currMB, isChroma, input->b_secT_enabled, input->sample_bit_depth);
#else
	inv_transform_NSQT(curr_blk, uiBitSize, currMB, isChroma, input->b_secT_enabled);
#endif
	if(iBlockType<12)//add yuqh20130915
	{
		if ( uiBitSize == B32X32_IN_BIT )
		{
			iSizeX = iSizeX << 1;
			iSizeY = iSizeY << 1;
			b8_x   = b8_x   << 1;
			b8_y   = b8_y   << 1;
		}
	}
}
else
if (input->useSDIP &&( iVer || iHor))
{
#if EXTEND_BD
	inv_transform_NSQT(curr_blk, uiBitSize, currMB, isChroma, input->b_secT_enabled, input->sample_bit_depth);
#else
	inv_transform_NSQT(curr_blk, uiBitSize, currMB, isChroma, input->b_secT_enabled);
#endif
	if(iBlockType<12)//add yuqh20130915
	{
		if ( uiBitSize == B32X32_IN_BIT )
		{
			iSizeX = iSizeX << 1;
			iSizeY = iSizeY << 1;
			b8_x   = b8_x   << 1;
			b8_y   = b8_y   << 1;
		}
	}

}
else
{
#if EXTEND_BD
	inv_transform_B8(curr_blk, uiBitSize, currMB, isChroma, input->b_secT_enabled, input->sample_bit_depth);
#else
	inv_transform_B8(curr_blk, uiBitSize, currMB, isChroma, input->b_secT_enabled);
#endif
}
uiPixInPic_y = ( uiPositionInPic / img->PicWidthInMbs ) * MIN_CU_SIZE + b8_y;
uiPixInPic_x = ( uiPositionInPic % img->PicWidthInMbs ) * MIN_CU_SIZE + b8_x ;
// reconstruct current 8x8 block and add to prediction block

for ( yy = 0; yy < iSizeY; yy++ )
{
	for ( xx = 0; xx < iSizeX; xx++ )
	{
		if ( b8 <= 3 )
		{
			curr_val = img->predBlock[b8_y + yy][b8_x + xx] + curr_blk[yy][xx];
		}
		else
		{
			curr_val = img->predBlock[yy][xx] + curr_blk[yy][xx];
		}
#if EXTEND_BD
		img->resiY[yy][xx] = curr_blk[yy][xx] = clamp ( curr_val, 0, (1 << input->sample_bit_depth) - 1 );
#else
		img->resiY[yy][xx] = curr_blk[yy][xx] = clamp ( curr_val, 0, 255 );
#endif

		if ( b8 <= 3 )
		{
#if EXTEND_BD
			imgY[uiPixInPic_y + yy][uiPixInPic_x + xx] = ( byte ) curr_blk[yy][xx];
#else
			imgY[uiPixInPic_y + yy][uiPixInPic_x + xx] = ( unsigned char ) curr_blk[yy][xx];
#endif
		}
		else
		{
#if EXTEND_BD
			imgUV[ ( b8 - 4 ) % 2][uiPixInPic_c_y + yy][uiPixInPic_c_x + xx] = ( byte ) curr_blk[yy][xx];
#else
			imgUV[ ( b8 - 4 ) % 2][uiPixInPic_c_y + yy][uiPixInPic_c_x + xx] = ( unsigned char ) curr_blk[yy][xx];
#endif
		}
	}
}
}



/*
*************************************************************************
* Function: Calculate SAD or SATD for a prediction error block of size
iSizeX x iSizeY.
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
int find_sad_8x8 ( int iMode, int iSizeX, int iSizeY, int iOffX, int iOffY, int resiY[MAX_CU_SIZE][MAX_CU_SIZE] )
{
  int i, j;
  int ishift = 0;
  int sad    = 0;
  int tempresiY[MIN_CU_SIZE][MIN_CU_SIZE];
  int m, n;

  // resiY[y,j,line][x,i,pixel]
  switch ( iMode )
  {
  case 0 : // ---------- SAD ----------

    for ( j = 0; j < iSizeY; j++ )
    {
      for ( i = 0; i < iSizeX; i++ )
      {
        sad += absm ( resiY[j][i] );
      }
    }
    break;
  case 1 : // --------- SATD ----------
    if ( iSizeX % 8 == 0 && iSizeY % 8 == 0 )
    {
      for ( i = 0; i < iSizeX; i += 8 )
      {
        for ( j = 0; j < iSizeY; j += 8 )
        {
          for ( m = 0; m < 8; m++ )
          {
            for ( n = 0; n < 8; n++ )
            {
              tempresiY[m][n] = resiY[j + m][i + n];
            }
          }

          sad += sad_hadamard ( 8, 8, 0, 0, tempresiY );
        }
      }

      ishift = 2;
      sad = ( sad + ( 1 << ( ishift - 1 ) ) ) >> ishift;

    }
    else
    {
      for ( i = 0; i < iSizeX; i += 4 )
      {
        for ( j = 0; j < iSizeY; j += 4 )
        {
          for ( m = 0; m < 4; m++ )
          {
            for ( n = 0; n < 4; n++ )
            {
              tempresiY[m][n] = resiY[j + m][i + n];
            }
          }

          sad += sad_hadamard4x4 (  tempresiY );
        }
      }
    }



    break;

  default :
    assert ( 0 == 1 );           // more switches may be added here later
  }

  return sad;
}


/*
*************************************************************************
* Function:
calculates the SAD of the Hadamard transformed block of
size iSizeX*iSizeY. Block may have an offset of (iOffX,iOffY).
If offset!=0 then iSizeX/Y has to be <=8.
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

int sad_hadamard4x4 (  int resiY[MIN_CU_SIZE][MIN_CU_SIZE] )
{
  int i, j, k, satd = 0, diff[16], m[16], d[16];

  for ( j = 0; j < 4; j++ )
  {
    for ( i = 0; i < 4; i++ )
    {
      diff[j * 4 + i] = resiY[j][i];
    }
  }

  /*===== hadamard transform =====*/
  m[ 0] = diff[ 0] + diff[12];
  m[ 1] = diff[ 1] + diff[13];
  m[ 2] = diff[ 2] + diff[14];
  m[ 3] = diff[ 3] + diff[15];
  m[ 4] = diff[ 4] + diff[ 8];
  m[ 5] = diff[ 5] + diff[ 9];
  m[ 6] = diff[ 6] + diff[10];
  m[ 7] = diff[ 7] + diff[11];
  m[ 8] = diff[ 4] - diff[ 8];
  m[ 9] = diff[ 5] - diff[ 9];
  m[10] = diff[ 6] - diff[10];
  m[11] = diff[ 7] - diff[11];
  m[12] = diff[ 0] - diff[12];
  m[13] = diff[ 1] - diff[13];
  m[14] = diff[ 2] - diff[14];
  m[15] = diff[ 3] - diff[15];

  d[ 0] = m[ 0] + m[ 4];
  d[ 1] = m[ 1] + m[ 5];
  d[ 2] = m[ 2] + m[ 6];
  d[ 3] = m[ 3] + m[ 7];
  d[ 4] = m[ 8] + m[12];
  d[ 5] = m[ 9] + m[13];
  d[ 6] = m[10] + m[14];
  d[ 7] = m[11] + m[15];
  d[ 8] = m[ 0] - m[ 4];
  d[ 9] = m[ 1] - m[ 5];
  d[10] = m[ 2] - m[ 6];
  d[11] = m[ 3] - m[ 7];
  d[12] = m[12] - m[ 8];
  d[13] = m[13] - m[ 9];
  d[14] = m[14] - m[10];
  d[15] = m[15] - m[11];

  m[ 0] = d[ 0] + d[ 3];
  m[ 1] = d[ 1] + d[ 2];
  m[ 2] = d[ 1] - d[ 2];
  m[ 3] = d[ 0] - d[ 3];
  m[ 4] = d[ 4] + d[ 7];
  m[ 5] = d[ 5] + d[ 6];
  m[ 6] = d[ 5] - d[ 6];
  m[ 7] = d[ 4] - d[ 7];
  m[ 8] = d[ 8] + d[11];
  m[ 9] = d[ 9] + d[10];
  m[10] = d[ 9] - d[10];
  m[11] = d[ 8] - d[11];
  m[12] = d[12] + d[15];
  m[13] = d[13] + d[14];
  m[14] = d[13] - d[14];
  m[15] = d[12] - d[15];

  d[ 0] = m[ 0] + m[ 1];
  d[ 1] = m[ 0] - m[ 1];
  d[ 2] = m[ 2] + m[ 3];
  d[ 3] = m[ 3] - m[ 2];
  d[ 4] = m[ 4] + m[ 5];
  d[ 5] = m[ 4] - m[ 5];
  d[ 6] = m[ 6] + m[ 7];
  d[ 7] = m[ 7] - m[ 6];
  d[ 8] = m[ 8] + m[ 9];
  d[ 9] = m[ 8] - m[ 9];
  d[10] = m[10] + m[11];
  d[11] = m[11] - m[10];
  d[12] = m[12] + m[13];
  d[13] = m[12] - m[13];
  d[14] = m[14] + m[15];
  d[15] = m[15] - m[14];

  for ( k = 0; k < 16; ++k )
  {
    satd += abs( d[k] );
  }
  satd = ( ( satd + 1 ) >> 1 );

  return satd;
}


int sad_hadamard ( int iSizeX, int iSizeY, int iOffX, int iOffY, int resiY[MIN_CU_SIZE][MIN_CU_SIZE] )
{
  int i, j, ii;
  int m1[MIN_CU_SIZE][MIN_CU_SIZE];
  int m2[MIN_CU_SIZE][MIN_CU_SIZE];
  int m3[MIN_CU_SIZE][MIN_CU_SIZE];
  int sad = 0;

#if M3198_CU8
  int iy[MIN_CU_SIZE] =
  {
    iOffY, iOffY, iOffY, iOffY,
    iOffY, iOffY, iOffY, iOffY
  };
#else
  int iy[MIN_CU_SIZE] =
  {
    iOffY, iOffY, iOffY, iOffY,
    iOffY, iOffY, iOffY, iOffY,
    iOffY, iOffY, iOffY, iOffY,
    iOffY, iOffY, iOffY, iOffY
  };
#endif

  // in this routine, cols are j,y and rows are i,x

  assert ( ( ( iOffX == 0 ) || ( iSizeX <= 8 ) ) && ( ( iOffY == 0 ) || ( iSizeY <= 8 ) ) );

  for ( j = 1; j < iSizeY; j++ )
  {
    iy[j] += j;
  }

  // vertical transform
  if ( iSizeY == 4 )
  {
    for ( i = 0; i < iSizeX; i++ )
    {
      ii = i + iOffX;

      m1[i][0] = resiY[ii][iy[0]] + resiY[ii][iy[3]];
      m1[i][1] = resiY[ii][iy[1]] + resiY[ii][iy[2]];
      m1[i][2] = resiY[ii][iy[1]] - resiY[ii][iy[2]];
      m1[i][3] = resiY[ii][iy[0]] - resiY[ii][iy[3]];

      m3[i][0] = m1[i][0] + m1[i][1];
      m3[i][1] = m1[i][0] - m1[i][1];
      m3[i][2] = m1[i][2] + m1[i][3];
      m3[i][3] = m1[i][3] - m1[i][2];
    }
  }
  else
  {
    for ( i = 0; i < iSizeX; i++ )
    {
      ii = i + iOffX;

      m1[i][0] = resiY[ii][iy[0]] + resiY[ii][iy[4]];
      m1[i][1] = resiY[ii][iy[1]] + resiY[ii][iy[5]];
      m1[i][2] = resiY[ii][iy[2]] + resiY[ii][iy[6]];
      m1[i][3] = resiY[ii][iy[3]] + resiY[ii][iy[7]];
      m1[i][4] = resiY[ii][iy[0]] - resiY[ii][iy[4]];
      m1[i][5] = resiY[ii][iy[1]] - resiY[ii][iy[5]];
      m1[i][6] = resiY[ii][iy[2]] - resiY[ii][iy[6]];
      m1[i][7] = resiY[ii][iy[3]] - resiY[ii][iy[7]];

      m2[i][0] = m1[i][0] + m1[i][2];
      m2[i][1] = m1[i][1] + m1[i][3];
      m2[i][2] = m1[i][0] - m1[i][2];
      m2[i][3] = m1[i][1] - m1[i][3];
      m2[i][4] = m1[i][4] + m1[i][6];
      m2[i][5] = m1[i][5] + m1[i][7];
      m2[i][6] = m1[i][4] - m1[i][6];
      m2[i][7] = m1[i][5] - m1[i][7];

      m3[i][0] = m2[i][0] + m2[i][1];
      m3[i][1] = m2[i][0] - m2[i][1];
      m3[i][2] = m2[i][2] + m2[i][3];
      m3[i][3] = m2[i][2] - m2[i][3];
      m3[i][4] = m2[i][4] + m2[i][5];
      m3[i][5] = m2[i][4] - m2[i][5];
      m3[i][6] = m2[i][6] + m2[i][7];
      m3[i][7] = m2[i][6] - m2[i][7];
    }
  }

  // horizontal transform
  if ( iSizeX == 4 )
  {
    for ( j = 0; j < iSizeY; j++ )
    {
      m1[0][j] = m3[0][j] + m3[3][j];
      m1[1][j] = m3[1][j] + m3[2][j];
      m1[2][j] = m3[1][j] - m3[2][j];
      m1[3][j] = m3[0][j] - m3[3][j];

      m2[0][j] = m1[0][j] + m1[1][j];
      m2[1][j] = m1[0][j] - m1[1][j];
      m2[2][j] = m1[2][j] + m1[3][j];
      m2[3][j] = m1[3][j] - m1[2][j];

      for ( i = 0; i < iSizeX; i++ )
      {
        sad += absm ( m2[i][j] );
      }
    }
  }
  else
  {
    for ( j = 0; j < iSizeY; j++ )
    {
      m2[0][j] = m3[0][j] + m3[4][j];
      m2[1][j] = m3[1][j] + m3[5][j];
      m2[2][j] = m3[2][j] + m3[6][j];
      m2[3][j] = m3[3][j] + m3[7][j];
      m2[4][j] = m3[0][j] - m3[4][j];
      m2[5][j] = m3[1][j] - m3[5][j];
      m2[6][j] = m3[2][j] - m3[6][j];
      m2[7][j] = m3[3][j] - m3[7][j];

      m1[0][j] = m2[0][j] + m2[2][j];
      m1[1][j] = m2[1][j] + m2[3][j];
      m1[2][j] = m2[0][j] - m2[2][j];
      m1[3][j] = m2[1][j] - m2[3][j];
      m1[4][j] = m2[4][j] + m2[6][j];
      m1[5][j] = m2[5][j] + m2[7][j];
      m1[6][j] = m2[4][j] - m2[6][j];
      m1[7][j] = m2[5][j] - m2[7][j];

      m2[0][j] = m1[0][j] + m1[1][j];
      m2[1][j] = m1[0][j] - m1[1][j];
      m2[2][j] = m1[2][j] + m1[3][j];
      m2[3][j] = m1[2][j] - m1[3][j];
      m2[4][j] = m1[4][j] + m1[5][j];
      m2[5][j] = m1[4][j] - m1[5][j];
      m2[6][j] = m1[6][j] + m1[7][j];
      m2[7][j] = m1[6][j] - m1[7][j];

      for ( i = 0; i < iSizeX; i++ )
      {
        sad += absm ( m2[i][j] );
      }
    }
  }

  return ( sad );
}

/*
*************************************************************************
* Function:
Make Intra prediction for all 9 modes for 8*8,
img_x and img_y are pixels offsets in the picture.
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
void intrapred_luma_AVS ( codingUnit* currMB, unsigned int uiBitSize, unsigned int uiPositionInPic, int img_x, int img_y )
{
	//short * edgepixels = ( short * ) malloc ( ( ( 1 << (uiBitSize+1) ) * 4 + 1 ) * sizeof ( short ) );
  short edgepixels[MAX_CU_SIZE*4*2+1];
#define EP ( edgepixels + ( ( 1 << (uiBitSize+1) ) * 2 ) )
  int x, y;
  int i;
#if !D20141230_BUG_FIX
  int block_available_up, block_available_up_right;
  int block_available_left, block_available_left_down;
#else 
  int block_available_up=0, block_available_up_right=0;
  int block_available_left=0, block_available_left_down=0;
#endif 
  int bs_x = 1 << uiBitSize;
  int bs_y = 1 << uiBitSize;
  int MBRowSize = img->width / MIN_CU_SIZE;/*lgp*/
  int block_available_up_left = 0;

  int mb_x = uiPositionInPic % MBRowSize;
  int mb_y = uiPositionInPic / MBRowSize;
  int NumMBWidthInBlk = 1 << ( uiBitSize - MIN_BLOCK_SIZE_IN_BIT ); //qyu 0823 uiBitSize 3 1; 4 2; 5 4
  int p_avai[5];

  int uiBitSize1= currMB->cuType==INxnNMB ? uiBitSize-1  :currMB->cuType==InNxNMB ? (uiBitSize+1) :uiBitSize;
  int uiBitSize2= currMB->cuType==InNxNMB ? uiBitSize-1  :currMB->cuType==INxnNMB ? (uiBitSize+1) :uiBitSize;

  int NumMBWidthInBlk1 = 1 << ( uiBitSize1 - MIN_BLOCK_SIZE_IN_BIT );
  int NumMBWidthInBlk2 = 1 << ( uiBitSize2 - MIN_BLOCK_SIZE_IN_BIT );

  bs_x = 1 << uiBitSize1;
  bs_y = 1 << uiBitSize2;
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
      EP[-1 - y] = imgY[img_y + 1 * y][img_x - 1];
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
      EP[0] = imgY[img_y - 1 ][img_x - 1];
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

  for ( i = 0; i < NUM_INTRA_PMODE; i++)
  {
    for ( y = 0; y < bs_y; y++ )
    {
      for ( x = 0; x < bs_x; x++ )
      {
        img->predBlockY[i][y][x] = 0;
      }
    }
#if EXTEND_BD
	predIntraLumaAdi( EP, img->predBlockY[i], i, uiBitSize, block_available_up, block_available_left,bs_y,bs_x, input->sample_bit_depth  );
#else
	predIntraLumaAdi( EP, img->predBlockY[i], i, uiBitSize, block_available_up, block_available_left,bs_y,bs_x );
#endif
  }
  //free ( edgepixels );
}

