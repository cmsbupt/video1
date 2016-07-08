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

//#include "../../lcommon/inc/commonVariables.h"
//#include "../../lcommon/inc/memalloc.h"
//#include "../../lcommon/inc/commonStructures.h"
//#include "../../lcommon/inc/ComAdaptiveLoopFilter.h"

#include "commonVariables.h"
#include "commonStructures.h"
#include "ComAdaptiveLoopFilter.h"

#define  IClip( Min, Max, Val) (((Val)<(Min))? (Min):(((Val)>(Max))? (Max):(Val)))
unsigned int g_MaxSizeInbit;
int weightsShape1Sym[ALF_MAX_NUM_COEF+1] = 
{
	         2,
	         2,
	      2, 2, 2,
	2, 2, 2, 1, 
	1
};

void reconstructCoefficients(ALFParam* alfParam, int** filterCoeff)
{
	int g,sum,i,coeffPred;
	for(g=0; g< alfParam->filters_per_group; g++)
	{
		sum = 0;
		for(i=0; i< alfParam->num_coeff-1; i++)
		{
			sum += (2 * alfParam->coeffmulti[g][i]);
			filterCoeff[g][i] = alfParam->coeffmulti[g][i];
		}
		coeffPred = (1<<ALF_NUM_BIT_SHIFT) - sum;
		filterCoeff[g][alfParam->num_coeff-1] = coeffPred + alfParam->coeffmulti[g][alfParam->num_coeff-1];
	}
}
void reconstructCoefInfo(int compIdx, ALFParam* alfParam, int** filterCoeff, int* varIndTab)
{
	int i;
	if(compIdx == ALF_Y)
	{
		memset(varIndTab, 0, NO_VAR_BINS * sizeof(int));
		if(alfParam->filters_per_group > 1)
		{
			for(i = 1; i < NO_VAR_BINS; ++i)
			{
				if(alfParam->filterPattern[i])
				{
					varIndTab[i] = varIndTab[i-1] + 1;
				}
				else
				{
					varIndTab[i] = varIndTab[i-1];
				}
			}
		}
	}
	reconstructCoefficients(alfParam, filterCoeff);
}
void ExtendPicBorder(byte* img,int iHeight,int iWidth,int iMarginY,int iMarginX,byte* imgExt)
{
	int   x, y, iStride;
	byte*  pin;
	byte*  pout;

	pin  = img;
	pout = imgExt + iMarginY * (iWidth + 2 * iMarginX) + iMarginX;
	for (y = 0; y < iHeight; y++)
	{
		memcpy(pout,pin,iWidth * sizeof(byte));
		pin = pin + iWidth;
		pout = pout + (iWidth + 2 * iMarginX);
	}

	pout = imgExt + iMarginY * (iWidth + 2 * iMarginX) + iMarginX;
	iStride = (iWidth + 2 * iMarginX);
	for ( y = 0; y < iHeight; y++)
	{
		for ( x = 0; x < iMarginX; x++ )
		{
			pout[ -iMarginX + x ] = pout[0];
			pout[    iWidth + x ] = pout[iWidth-1];
		}
		pout += iStride;
	}

	pout -= (iStride + iMarginX);
	for ( y = 0; y < iMarginY; y++ )
	{
		memcpy( pout + (y+1)*iStride, pout, sizeof(byte)*(iWidth + (iMarginX<<1)) );
	}

	pout -= ((iHeight-1) * iStride);
	for ( y = 0; y < iMarginY; y++ )
	{
		memcpy( pout - (y+1)*iStride, pout, sizeof(byte)*(iWidth + (iMarginX<<1)) );
	}

}

#if ALF_BOUNDARY_FIX
//(x,y): the pixel used for filtering
//(lcuPosX, lcuPosY): the LCU position within the picture
//startX: x start postion of the current filtering unit 
//startY: y start postion of the current filtering unit 
//endX: x end postion of the current filtering unit 
//endY: y end postion of the current filtering unit 
int check_filtering_unit_boundary_extension(int x, int y, int lcuPosX, int lcuPosY, int startX, int startY, int endX, int endY, int isAboveLeftAvail, int isLeftAvail, int isAboveRightAvail, int isRightAvail)
{
	int modifiedX;

	if(x<lcuPosX ) 
	{
		if(y<startY) 
		{
			modifiedX = lcuPosX;
		}
		else if(y<lcuPosY)
		{
			if(isAboveLeftAvail) 
			{
				modifiedX = x;
			}
			else 
			{
				modifiedX = lcuPosX;
			}
		}
		else if(y<=endY)
		{
			if(isLeftAvail) 
			{
				modifiedX = x;
			}
			else 
			{
				modifiedX = lcuPosX;
			}
		}
		else
		{
			modifiedX = lcuPosX;
		}
	}
	else if(x<=endX)
	{
		modifiedX = x;
	}
	else
	{ // x>endX
		if(y<startY) 
		{
			modifiedX = endX;
		}
		else if(y<lcuPosY)
		{
			if(isAboveRightAvail) 
			{
				modifiedX = x;
			}
			else 
			{
				modifiedX = endX;
			}
		}
		else if(y<=endY)
		{
			if(isRightAvail) 
			{
				modifiedX = x;
			}
			else 
			{
				modifiedX = endX;
			}
		}
		else
		{
			modifiedX = endX;
		}
	}

	return modifiedX;
}

#endif


#if EXTEND_BD
void filterOneCompRegion(byte *imgRes, byte *imgPad, int stride, Boolean isChroma, int yPos, int lcuHeight, int xPos, int lcuWidth, int** filterSet, int* mergeTable, byte** varImg,
  int sample_bit_depth, int isLeftAvail, int isRightAvail, int isAboveAvail, int isBelowAvail, int isAboveLeftAvail, int isAboveRightAvail)
#else
void filterOneCompRegion(byte *imgRes, byte *imgPad, int stride, Boolean isChroma, int yPos, int yPosEnd, int xPos, int xPosEnd, int** filterSet, int* mergeTable, byte** varImg,
						 int isLeftAvail, int isRightAvail, int isAboveAvail, int isBelowAvail)
#endif
{
	int offset       = (1<<( (int)ALF_NUM_BIT_SHIFT-1));
	byte *imgPad1,*imgPad2,*imgPad3, *imgPad4, *imgPad5, *imgPad6;

	byte *var = varImg[yPos>>LOG2_VAR_SIZE_H] + (xPos>>LOG2_VAR_SIZE_W);
	int i, j, pixelInt;
	int *coef = (isChroma) ? filterSet[0] : filterSet[mergeTable[*(var)]] ;

	//define ????????
	int startPos		= isAboveAvail ? (yPos-4) : yPos;
	int endPos			= isBelowAvail ? (yPos + lcuHeight - 4) : (yPos+lcuHeight);
	int xOffSetLeft		= isLeftAvail  ? -3 : 0;
	int xOffSetRight	= isRightAvail ?  3 : 0;

	int xPosEnd			= xPos + lcuWidth;

	int yUp, yBottom;
	int xLeft, xRight;

	imgPad += (startPos*stride);
	imgRes += (startPos*stride);

	for(i=startPos; i<endPos; i++)
	{
		yUp		= Clip3(startPos, endPos-1, i-1);
		yBottom = Clip3(startPos, endPos-1, i+1);
		imgPad1 = imgPad + (yBottom-i)*stride;
		imgPad2 = imgPad + (yUp - i)*stride;

		yUp		= Clip3(startPos, endPos-1, i-2);
		yBottom = Clip3(startPos, endPos-1, i+2);
		imgPad3 = imgPad + (yBottom-i)*stride;
		imgPad4 = imgPad + (yUp-i)*stride;

		yUp		= Clip3(startPos, endPos-1, i-3);
		yBottom = Clip3(startPos, endPos-1, i+3);
		imgPad5 = imgPad + (yBottom-i)*stride;
		imgPad6 = imgPad + (yUp-i)*stride;
		for(j= xPos; j< xPosEnd ; j++)
		{
			pixelInt  = coef[0]* (imgPad5[j  ] + imgPad6[j  ]);

			pixelInt += coef[1]* (imgPad3[j  ] + imgPad4[j  ]);

#if ALF_BOUNDARY_FIX
      pixelInt += coef[3] * (imgPad1[j] + imgPad2[j]);

      // upper left c2
      xLeft  = check_filtering_unit_boundary_extension(j-1, i-1, xPos, yPos, xPos, startPos, xPosEnd-1, endPos-1, isAboveLeftAvail, isLeftAvail, isAboveRightAvail, isRightAvail);
      pixelInt += coef[2] * imgPad2[xLeft];

      // upper right c4
      xRight = check_filtering_unit_boundary_extension(j+1, i-1, xPos, yPos, xPos, startPos, xPosEnd-1, endPos-1, isAboveLeftAvail, isLeftAvail, isAboveRightAvail, isRightAvail);
      pixelInt += coef[4] * imgPad2[xRight];

      // lower left c4
      xLeft  = check_filtering_unit_boundary_extension(j-1, i+1, xPos, yPos, xPos, startPos, xPosEnd-1, endPos-1, isAboveLeftAvail, isLeftAvail, isAboveRightAvail, isRightAvail);
      pixelInt += coef[4] * imgPad1[xLeft];

      // lower right c2
      xRight = check_filtering_unit_boundary_extension(j+1, i+1, xPos, yPos, xPos, startPos, xPosEnd-1, endPos-1, isAboveLeftAvail, isLeftAvail, isAboveRightAvail, isRightAvail);
      pixelInt += coef[2] * imgPad1[xRight];


      xLeft  = Clip3(xPos+xOffSetLeft, xPosEnd-1+xOffSetRight, j-1);
      xRight = Clip3(xPos+xOffSetLeft, xPosEnd-1+xOffSetRight, j+1);
      pixelInt += coef[7] * (imgPad[xRight] + imgPad[xLeft]);


      xLeft  = Clip3(xPos+xOffSetLeft, xPosEnd-1+xOffSetRight, j-2);
      xRight = Clip3(xPos+xOffSetLeft, xPosEnd-1+xOffSetRight, j+2);
      pixelInt += coef[6]* (imgPad[xRight] + imgPad[xLeft]);

      xLeft  = Clip3(xPos+xOffSetLeft, xPosEnd-1+xOffSetRight, j-3);
      xRight = Clip3(xPos+xOffSetLeft, xPosEnd-1+xOffSetRight, j+3);
      pixelInt += coef[5]* (imgPad[xRight] + imgPad[xLeft]);
#else
			// the left and right pixels used for filtering according to the current LCU's boundary. 
			// the ????????'s left and right pixel availability may be different.
			xLeft  = Clip3(xPos+xOffSetLeft, xPosEnd-1+xOffSetRight, j-1);
			xRight = Clip3(xPos+xOffSetLeft, xPosEnd-1+xOffSetRight, j+1);

			// the current LCU is at the lower right quadrant
			//                        slice 0 | slice 0/1
			//												--------|---------
			//                        slice 1 | slice 1
      // When the samples from above left region is used and the the above left region does not exist 
      if (!isAboveLeftAvail && isLeftAvail && j == xPos && i <= yPos)
      {
        xLeft = j;
      }
			// the current LCU is at the lower left quadrant
			//                        slice 0 | slice 0
			//												--------|---------
			//                        slice 1 | slice 1
      if (!isAboveRightAvail && isRightAvail && j == xPosEnd - 1 && i <= yPos)
      {
        xRight = j;
      }

      pixelInt += coef[2] * imgPad2[xLeft];
      pixelInt += coef[3] * (imgPad1[j] + imgPad2[j]);
      pixelInt += coef[4] * imgPad2[xRight];

      pixelInt += coef[7] * (imgPad[xRight] + imgPad[xLeft]);

      xLeft  = Clip3(xPos + xOffSetLeft, xPosEnd - 1 + xOffSetRight, j - 1);
      xRight = Clip3(xPos + xOffSetLeft, xPosEnd - 1 + xOffSetRight, j + 1);

      if (!isAboveLeftAvail && isLeftAvail && j == xPos && i < yPos)
      {
        xLeft = j;
      }

			// the current LCU is the lower left one
			//                        slice 0 | slice 0
			//												--------|---------
			//                        slice 1 | slice 1
      if (!isAboveRightAvail && isRightAvail && j == xPosEnd - 1 && i < yPos) // this condidtion can never be true
      {
        xRight = j;
      }

      pixelInt += coef[2] * imgPad1[xRight] + coef[4] * imgPad1[xLeft];

			xLeft  = Clip3(xPos+xOffSetLeft, xPosEnd-1+xOffSetRight, j-2);
			xRight = Clip3(xPos+xOffSetLeft, xPosEnd-1+xOffSetRight, j+2);
			pixelInt += coef[6]* (imgPad[xRight] + imgPad[xLeft]);

			xLeft  = Clip3(xPos+xOffSetLeft, xPosEnd-1+xOffSetRight, j-3);
			xRight = Clip3(xPos+xOffSetLeft, xPosEnd-1+xOffSetRight, j+3);
			pixelInt += coef[5]* (imgPad[xRight] + imgPad[xLeft]);
#endif
			pixelInt += coef[8]* (imgPad[j  ]);

			pixelInt=(int)((pixelInt+offset) >> ALF_NUM_BIT_SHIFT);
#if EXTEND_BD
			imgRes[j] = IClip( 0, ((1 << sample_bit_depth) - 1),pixelInt );
#else
			imgRes[j] = IClip( 0, 255,pixelInt );
#endif
		}
		imgPad += stride;
		imgRes += stride;
	}  
}

void checkFilterCoeffValue( int *filter, int filterLength, Boolean isChroma )
{
	int i;
	int maxValueNonCenter ;
	int minValueNonCenter ;
	int maxValueCenter    ;
	int minValueCenter    ; 
	maxValueNonCenter = 1 * (1 << ALF_NUM_BIT_SHIFT) - 1;
	minValueNonCenter = 0 - 1 * (1 << ALF_NUM_BIT_SHIFT);
	maxValueCenter    = 2 * (1 << ALF_NUM_BIT_SHIFT) - 1;
	minValueCenter    = 0 ; 

	for(i = 0; i < filterLength-1; i++)
	{
		filter[i] = Clip3(minValueNonCenter, maxValueNonCenter, filter[i]);
	}

	filter[filterLength-1] = Clip3(minValueCenter, maxValueCenter, filter[filterLength-1]);
}
void copyALFparam(ALFParam* dst,ALFParam* src)
{
	int j;
	dst->alf_flag = src->alf_flag;
	dst->componentID = src->componentID;
	dst->filters_per_group = src->filters_per_group;
	dst->num_coeff  = src->num_coeff;

	switch (src->componentID)
	{
	case ALF_Y:
		for (j=0;j<NO_VAR_BINS;j++)
		{
			memcpy(dst->coeffmulti[j],src->coeffmulti[j],ALF_MAX_NUM_COEF * sizeof(int));
		}
		memcpy(dst->filterPattern,src->filterPattern,NO_VAR_BINS * sizeof(int));
		break;
	case ALF_Cb:
	case ALF_Cr:
		for (j=0;j<1;j++)
		{
			memcpy(dst->coeffmulti[j],src->coeffmulti[j],ALF_MAX_NUM_COEF * sizeof(int));
		}
		break;
	default:
		{
			printf("Not a legal component ID\n");
			assert(0);
			exit(-1);
		}
	}
}

int getLCUCtrCtx_Idx(int ctu,int numLCUInPicWidth,int numLCUInPicHeight,int NumCUInFrame,int compIdx,Boolean** AlfLCUEnabled)
{
	int row,col,a,b;
	row = ctu / numLCUInPicWidth;
	col = ctu % numLCUInPicWidth;
	if (row == 0) 
	{
		b = 0;
	}
	else
	{
		b = (int) AlfLCUEnabled[ctu-numLCUInPicWidth][compIdx];
	}

	if (col == 0) 
	{
		a = 0;
	}
	else
	{
		a = (int) AlfLCUEnabled[ctu-1][compIdx];
	}
	return (a + 2*b);

}

void setFilterImage(byte* pDecY,byte** pDecUV,int stride,byte** imgY_out,byte*** imgUV_out,int img_width,int img_height)
{
	int j,i,k,width_cr,stride_cr,height_cr;
	byte *psrc;
	psrc = pDecY;

	for (j = 0; j < img_height; j++)
	{
		for (i=0;i<img_width;i++)
		{
			imgY_out[j][i] = psrc[i];
		}

		psrc = psrc + stride;
	}
	width_cr = img_width>>1;
	height_cr = img_height>>1;
	stride_cr = stride >> 1;
	for (k=0;k<2;k++)
	{
		psrc = pDecUV[k];

		for (j = 0; j < height_cr; j++)
		{
			for (i=0;i<width_cr;i++)
			{
				imgUV_out[k][j][i] = psrc[i];
			}

			psrc = psrc + stride_cr;

		}
	}
}
