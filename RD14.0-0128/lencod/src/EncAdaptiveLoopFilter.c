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
#include <string.h>
#include <assert.h>
#include <math.h>
#include "global.h"
#include "../../lcommon/inc/commonVariables.h"
#include "../../lcommon/inc/memalloc.h"
#include "AEC.h"
#include "rdopt_coding_state.h"
#include "../../lencod/inc/EncAdaptiveLoopFilter.h"
#include "../../lencod/inc/AEC.h"
#include "../../lencod/inc/vlc.h"

#define ROUND(a)  (((a) < 0)? (int)((a) - 0.5) : (int)((a) + 0.5))
#define REG              0.0001
#define REG_SQR          0.0000001


#define max(a, b)                   ((a) > (b) ? (a) : (b))  //!< Macro returning max value 
#define min(a, b)                   ((a) < (b) ? (a) : (b))  //!< Macro returning min value 
EncALFVar *Enc_ALF;


#define Clip_post(high,val) ((val > high)? high: val)
/*
*************************************************************************
* Function: ALF encoding process top function
* Input:  
*             img  : The image parameter
*    imgY_alf_Org  : The Y component of the original image
*    imgUV_alf_Org : The UV component of the original image
*    imgY_alf_Rec  : The Y component of the SAO output image
*    imgUV_alf_Rec : The UV component of the SAO output image
*      lambda_mode : The lambda value in the ALF-RD decision
* Output: 
*    alfPictureParam: The ALF parameter
*              apsId: The ALF parameter index in the buffer
*       isNewApsSent£ºThe New flag index
* Return:
*************************************************************************
*/
void ALFProcess(ALFParam** alfPictureParam, ImageParameters* img, byte* imgY_alf_Org, byte** imgUV_alf_Org, byte* imgY_alf_Rec,byte** imgUV_alf_Rec, double lambda_mode)
{
#if !ALF_FAST
	byte* imgY_ExtRes;
	byte** imgUV_ExtRes;
	byte* imgY_ExtOrg;
	byte** imgUV_ExtOrg;
	byte* imgY_ExtDec;
	byte** imgUV_ExtDec;
#endif

	int LumaMarginX ;
	int LumaMarginY ;
	int ChromaMarginX;
	int ChromaMarginY;
#ifdef ALF_FAST
	int lumaStride; // Add to avoid micro trubles
#endif
	byte *pResY,*pDecY,*pOrgY;
	byte *pResUV[2],*pDecUV[2],*pOrgUV[2];
	LumaMarginX = (1<<g_MaxSizeInbit) + 16;
	LumaMarginY = (1<<g_MaxSizeInbit) + 16;
	ChromaMarginX = LumaMarginX>>1;
	ChromaMarginY = LumaMarginY>>1;

#if !ALF_FAST
	get_mem1D(&imgY_ExtOrg,(img->height + (LumaMarginY<<1)) * (img->width + (LumaMarginX<<1)));
	get_mem2D(&imgUV_ExtOrg,2,((img->height_cr + (ChromaMarginY<<1))*(img->width_cr + (ChromaMarginX<<1))));
 	get_mem1D(&imgY_ExtDec,(img->height + (LumaMarginY<<1)) * (img->width + (LumaMarginX<<1)));
	get_mem2D(&imgUV_ExtDec,2,((img->height_cr + (ChromaMarginY<<1))*(img->width_cr + (ChromaMarginX<<1))));
	get_mem1D(&imgY_ExtRes,(img->height + (LumaMarginY<<1)) * (img->width + (LumaMarginX<<1)));
	get_mem2D(&imgUV_ExtRes,2,((img->height_cr + (ChromaMarginY<<1))*(img->width_cr + (ChromaMarginX<<1))));
	ExtendPicBorder(imgY_alf_Rec,img->height,img->width,LumaMarginY,LumaMarginX,imgY_ExtDec);
	ExtendPicBorder(imgUV_alf_Rec[0],img->height_cr,img->width_cr,ChromaMarginY,ChromaMarginX,imgUV_ExtDec[0]);
	ExtendPicBorder(imgUV_alf_Rec[1],img->height_cr,img->width_cr,ChromaMarginY,ChromaMarginX,imgUV_ExtDec[1]);

	ExtendPicBorder(imgY_alf_Rec,img->height,img->width,LumaMarginY,LumaMarginX,imgY_ExtRes);
	ExtendPicBorder(imgUV_alf_Rec[0],img->height_cr,img->width_cr,ChromaMarginY,ChromaMarginX,imgUV_ExtRes[0]);
	ExtendPicBorder(imgUV_alf_Rec[1],img->height_cr,img->width_cr,ChromaMarginY,ChromaMarginX,imgUV_ExtRes[1]);

	ExtendPicBorder(imgY_alf_Org,img->height,img->width,LumaMarginY,LumaMarginX,imgY_ExtOrg);
	ExtendPicBorder(imgUV_alf_Org[0],img->height_cr,img->width_cr,ChromaMarginY,ChromaMarginX,imgUV_ExtOrg[0]);
	ExtendPicBorder(imgUV_alf_Org[1],img->height_cr,img->width_cr,ChromaMarginY,ChromaMarginX,imgUV_ExtOrg[1]);

	pResY     = imgY_ExtRes  + LumaMarginY * (img->width + (LumaMarginX<<1)) + LumaMarginX;
	pResUV[0] = imgUV_ExtRes[0] + (ChromaMarginY * (img->width_cr + (ChromaMarginX<<1)) + ChromaMarginX);
	pResUV[1] = imgUV_ExtRes[1] + (ChromaMarginY * (img->width_cr + (ChromaMarginX<<1)) + ChromaMarginX);


	pDecY     = imgY_ExtDec  + LumaMarginY * (img->width + (LumaMarginX<<1)) + LumaMarginX;
	pDecUV[0] = imgUV_ExtDec[0] + (ChromaMarginY * (img->width_cr + (ChromaMarginX<<1)) + ChromaMarginX);
	pDecUV[1] = imgUV_ExtDec[1] + (ChromaMarginY * (img->width_cr + (ChromaMarginX<<1)) + ChromaMarginX);

	pOrgY     = imgY_ExtOrg  + LumaMarginY * (img->width + (LumaMarginX<<1)) + LumaMarginX;
	pOrgUV[0] = imgUV_ExtOrg[0] + (ChromaMarginY * (img->width_cr + (ChromaMarginX<<1)) + ChromaMarginX);
	pOrgUV[1] = imgUV_ExtOrg[1] + (ChromaMarginY * (img->width_cr + (ChromaMarginX<<1)) + ChromaMarginX);
#else
	pResY = imgY[0];
	pResUV[0] = imgUV[0][0];
	pResUV[1] = imgUV[1][0];
	pDecY = imgY_alf_Rec;
	pDecUV[0] = imgUV_alf_Rec[0];
	pDecUV[1] = imgUV_alf_Rec[1];
	pOrgY = imgY_alf_Org;
	pOrgUV[0] = imgUV_alf_Org[0];
	pOrgUV[1] = imgUV_alf_Org[1];
#endif

#if ALF_ESTIMATE_AEC_RATE
  store_coding_state(Enc_ALF->m_cs_alf_initial);
#endif

#if ALF_FAST
  lumaStride = img->width;
#else
  lumaStride = (img->width + (LumaMarginX<<1));
#endif
  // !!! NOTED: From There on, We use lumaStride instead of (img->width + (LumaMarginX<<1)) to avoid micro trubles

  getStatistics(img,pOrgY,pOrgUV,pDecY,pDecUV,lumaStride,lambda_mode);
  setCurAlfParam(alfPictureParam, lambda_mode);
#if ALF_ENC_IMPORVE_RATE_ESTIMATE
  executePicLCUOnOffDecision(alfPictureParam, lambda_mode , FALSE, NULL, pOrgY,pOrgUV,pDecY,pDecUV,pResY,pResUV,lumaStride);
#else
  executePicLCUOnOffDecision(alfPictureParam, pOrgY,pOrgUV,pDecY,pDecUV,pResY,pResUV,lumaStride, lambda_mode);
#endif

#if !ALF_FAST
	setFilterImage(pResY,pResUV,lumaStride,imgY,imgUV,img->width,img->height);
#endif

	if(Enc_ALF->m_alfLowLatencyEncoding)
	{
		//derive time-delayed filter for next picture
		storeAlfTemporalLayerInfo(Enc_ALF->m_alfPictureParam[getTemporalLayerNo(img->tr, gop_size)], getTemporalLayerNo(img->tr, gop_size),lambda_mode);
	}
	
#if !ALF_FAST
	free_mem1D(imgY_ExtDec);
	free_mem2D(imgUV_ExtDec);
	free_mem1D(imgY_ExtRes);
	free_mem2D(imgUV_ExtRes);
	free_mem1D(imgY_ExtOrg);
	free_mem2D(imgUV_ExtOrg);
#endif
}

/*
*************************************************************************
* Function: Calculate the correlation matrix for image
* Input:  
*             img  : The image parameter
*        imgY_org  : The Y component of the original image
*        imgUV_org : The UV component of the original image
*        imgY_Dec  : The Y component of the SAO output image
*        imgUV_Dec : The UV component of the SAO output image
*           stride : The width of Y component of the SAO output image
*           lambda : The lambda value in the ALF-RD decision
* Output: 
* Return:
*************************************************************************
*/
void getStatistics(ImageParameters* img,byte* imgY_org,byte** imgUV_org,byte* imgY_Dec,byte** imgUV_Dec,int stride,double lambda)
{
	Boolean  isLeftAvail,isRightAvail,isAboveAvail,isBelowAvail;
	Boolean  isAboveLeftAvail, isAboveRightAvail;
	int lcuHeight,lcuWidth, img_height,img_width;
	int ctu,NumCUInFrame,numLCUInPicWidth,numLCUInPicHeight;
	int ctuYPos,ctuXPos, ctuHeight,ctuWidth; 
	int compIdx, formatShift;

	lcuHeight         = 1<<g_MaxSizeInbit;
	lcuWidth          = lcuHeight;
	img_height        = img->height;
	img_width         = img->width;
    numLCUInPicWidth  = img_width / lcuWidth ;
	numLCUInPicHeight = img_height / lcuHeight ;
	numLCUInPicWidth  += (img_width % lcuWidth)? 1 : 0;
	numLCUInPicHeight += (img_height % lcuHeight)? 1 : 0;
	NumCUInFrame = numLCUInPicHeight * numLCUInPicWidth;

	for ( ctu=0;ctu<NumCUInFrame; ctu++)
	{
		ctuYPos   = (ctu / numLCUInPicWidth )* lcuHeight;
		ctuXPos   = (ctu % numLCUInPicWidth )* lcuWidth;
		ctuHeight = (ctuYPos + lcuHeight > img_height)?( img_height- ctuYPos):lcuHeight;
		ctuWidth  = (ctuXPos + lcuWidth  > img_width )?( img_width - ctuXPos):lcuWidth;
    deriveBoundaryAvail(numLCUInPicWidth, numLCUInPicHeight, ctu, &isLeftAvail,&isRightAvail,&isAboveAvail, &isBelowAvail, &isAboveLeftAvail, &isAboveRightAvail);
		for (compIdx=0; compIdx < NUM_ALF_COMPONENT; compIdx++)
		{
			formatShift = (compIdx==ALF_Y)?0:1;
			reset_alfCorr(Enc_ALF->m_alfCorr[compIdx][ctu]);
			if (compIdx==ALF_Y)
			{
        getStatisticsOneLCU(Enc_ALF->m_alfLowLatencyEncoding, compIdx, ctu, ctuYPos, ctuXPos, ctuHeight, ctuWidth, isAboveAvail, isBelowAvail, isLeftAvail, isRightAvail
          , isAboveLeftAvail, isAboveRightAvail, Enc_ALF->m_alfCorr[compIdx][ctu], imgY_org, imgY_Dec, stride, formatShift, numLCUInPicWidth, NumCUInFrame);
        if (Enc_ALF->m_alfLowLatencyEncoding)
        {
          reset_alfCorr(Enc_ALF->m_alfNonSkippedCorr[compIdx][ctu]);
          getStatisticsOneLCU(FALSE, compIdx, ctu, ctuYPos, ctuXPos, ctuHeight, ctuWidth, isAboveAvail, isBelowAvail, isLeftAvail, isRightAvail
            , isAboveLeftAvail, isAboveRightAvail, Enc_ALF->m_alfNonSkippedCorr[compIdx][ctu], imgY_org, imgY_Dec, stride, formatShift, numLCUInPicWidth, NumCUInFrame);
        }
      }
      else
      {
        getStatisticsOneLCU(Enc_ALF->m_alfLowLatencyEncoding, compIdx, ctu, ctuYPos, ctuXPos, ctuHeight, ctuWidth, isAboveAvail, isBelowAvail, isLeftAvail, isRightAvail
          , isAboveLeftAvail, isAboveRightAvail, Enc_ALF->m_alfCorr[compIdx][ctu], imgUV_org[compIdx - ALF_Cb], imgUV_Dec[compIdx - ALF_Cb], (stride >> 1), formatShift, numLCUInPicWidth, NumCUInFrame);
        if (Enc_ALF->m_alfLowLatencyEncoding)
        {
          reset_alfCorr(Enc_ALF->m_alfNonSkippedCorr[compIdx][ctu]);
          getStatisticsOneLCU(FALSE, compIdx, ctu, ctuYPos, ctuXPos, ctuHeight, ctuWidth, isAboveAvail, isBelowAvail, isLeftAvail, isRightAvail
            , isAboveLeftAvail, isAboveRightAvail, Enc_ALF->m_alfNonSkippedCorr[compIdx][ctu], imgUV_org[compIdx - ALF_Cb], imgUV_Dec[compIdx - ALF_Cb], (stride >> 1), formatShift, numLCUInPicWidth, NumCUInFrame);
				}
			}
		}
	}
}

void deriveLoopFilterBoundaryAvailibility(ImageParameters* img,int numLCUInPicWidth, int numLCUInPicHeight, int ctu,Boolean* isLeftAvail,Boolean* isRightAvail,Boolean* isAboveAvail,Boolean* isBelowAvail,double lambda)
{
	int numLCUsInFrame = numLCUInPicHeight * numLCUInPicWidth;
	Boolean isAboveLeftAvail  ;
	Boolean isAboveRightAvail ;
	Boolean isBelowLeftAvail  ;
	Boolean isBelowRightAvail ;
	int  lcuHeight         = 1<<g_MaxSizeInbit;
	int  lcuWidth          = lcuHeight;
	int  img_height        = img->height;
	int  img_width         = img->width;

	int  NumCUInFrame;
	int  pic_x            ;
	int  pic_y            ;
	int  mb_x             ;
	int  mb_y             ;
	int  mb_nr            ;
	int  smb_mb_width     ;
	int  smb_mb_height    ;
	int  pic_mb_width     = img_width / MIN_CU_SIZE;
	int  pic_mb_height    = img_height / MIN_CU_SIZE;
	int  cuCurrNum        ;
	
	codingUnit* cuCurr          ;
	codingUnit* cuLeft          ;
	codingUnit* cuRight         ;
	codingUnit* cuAbove         ;
	codingUnit* cuBelow         ;
	codingUnit* cuAboveLeft     ;
	codingUnit* cuAboveRight    ;
	codingUnit* cuBelowLeft     ;
	codingUnit* cuBelowRight    ;

	int curSliceNr,neighorSliceNr,neighorSliceNr1,neighorSliceNr2;

	NumCUInFrame = numLCUInPicHeight * numLCUInPicWidth;

	pic_x        = (ctu % numLCUInPicWidth )* lcuWidth;
	pic_y        = (ctu / numLCUInPicWidth )* lcuHeight;

	pic_mb_width  += (img_width  % MIN_CU_SIZE)? 1 : 0;
	pic_mb_height += (img_height % MIN_CU_SIZE)? 1 : 0;

	mb_x               = pic_x / MIN_CU_SIZE;
	mb_y               = pic_y / MIN_CU_SIZE;
	mb_nr              = mb_y * pic_mb_width + mb_x;
	smb_mb_width       = lcuWidth >> MIN_CU_SIZE_IN_BIT;
	smb_mb_height      = lcuHeight >> MIN_CU_SIZE_IN_BIT;
	cuCurrNum          = mb_nr;
	


	*isLeftAvail  = (ctu % numLCUInPicWidth != 0);
	*isRightAvail = (ctu % numLCUInPicWidth != numLCUInPicWidth - 1);
	*isAboveAvail = (ctu  >= numLCUInPicWidth);
	*isBelowAvail = (ctu  < numLCUsInFrame - numLCUInPicWidth);

	isAboveLeftAvail  = (*isAboveAvail && *isLeftAvail);
	isAboveRightAvail = (*isAboveAvail && *isRightAvail);
	isBelowLeftAvail  = (*isBelowAvail && *isLeftAvail);
	isBelowRightAvail = (*isBelowAvail && *isRightAvail);
	
	cuCurr          = &(img->mb_data[cuCurrNum]);
	cuLeft          = *isLeftAvail      ? &(img->mb_data[cuCurrNum-1]):NULL;
	cuRight         = *isRightAvail     ? &(img->mb_data[cuCurrNum+1]):NULL;
	cuAbove         = *isAboveAvail     ? &(img->mb_data[cuCurrNum-pic_mb_width]):NULL;
	cuBelow         = *isBelowAvail     ? &(img->mb_data[cuCurrNum+pic_mb_width]):NULL;
	cuAboveLeft     = isAboveLeftAvail ? &(img->mb_data[cuCurrNum-pic_mb_width-1]):NULL;
	cuAboveRight    = isAboveRightAvail? &(img->mb_data[cuCurrNum-pic_mb_width+1]):NULL;
	cuBelowLeft     = isBelowLeftAvail ? &(img->mb_data[cuCurrNum+pic_mb_width-1]):NULL;
	cuBelowRight    = isBelowRightAvail? &(img->mb_data[cuCurrNum+pic_mb_width+1]):NULL;

	*isLeftAvail = *isRightAvail = *isAboveAvail = *isBelowAvail = FALSE;

	curSliceNr = cuCurr->slice_nr;
	if (cuLeft != NULL)
	{
		neighorSliceNr = cuLeft->slice_nr;
		if (curSliceNr == neighorSliceNr)
		{
			*isLeftAvail = TRUE;
		}
	}

	if (cuRight != NULL)
	{
		neighorSliceNr = cuRight->slice_nr;
		if (curSliceNr == neighorSliceNr)
		{
			*isRightAvail = TRUE;
		}
	}

	if (cuAbove != NULL && cuAboveLeft != NULL && cuAboveRight != NULL)
	{
		neighorSliceNr  = cuAbove->slice_nr;
		neighorSliceNr1 = cuAboveLeft->slice_nr;
		neighorSliceNr2 = cuAboveRight->slice_nr;
		if ((curSliceNr == neighorSliceNr) 
		  &&(neighorSliceNr == neighorSliceNr1)
		  &&(neighorSliceNr == neighorSliceNr2))
		{
			*isAboveAvail = TRUE;
		}
	}

	if (cuBelow != NULL && cuBelowLeft != NULL && cuBelowRight != NULL)
	{
		neighorSliceNr = cuBelow->slice_nr;
		neighorSliceNr1 = cuBelowLeft->slice_nr;
		neighorSliceNr2 = cuBelowRight->slice_nr;
		if ((curSliceNr == neighorSliceNr) 
			&&(neighorSliceNr == neighorSliceNr1)
			&&(neighorSliceNr == neighorSliceNr2))
		{
			*isBelowAvail = TRUE;
		}
	}


}
void createAlfGlobalBuffers(ImageParameters* img)
{
	int lcuHeight,lcuWidth, img_height,img_width;
	int NumCUInFrame,numLCUInPicWidth,numLCUInPicHeight;
	int compIdx, n, i,g;
    int numCoef = (int) ALF_MAX_NUM_COEF;
	int maxNumTemporalLayer;

	int regionTable[NO_VAR_BINS] = {0, 1, 4, 5, 15, 2, 3, 6, 14, 11, 10, 7, 13, 12,  9,  8}; 
	int xInterval  ;  
	int yInterval  ;
	int yIndex, xIndex;
	int yIndexOffset;

	g_MaxSizeInbit = input->g_uiMaxSizeInBit;
	gop_size = subGopNum==1? gop_size_all:input->successive_Bframe+1;

	lcuHeight         = 1<<g_MaxSizeInbit;
	lcuWidth          = lcuHeight;
	img_height        = img->height;
	img_width         = img->width;
	numLCUInPicWidth  = img_width / lcuWidth ;
	numLCUInPicHeight = img_height / lcuHeight ;
	numLCUInPicWidth  += (img_width % lcuWidth)? 1 : 0;
	numLCUInPicHeight += (img_height % lcuHeight)? 1 : 0;
	NumCUInFrame = numLCUInPicHeight * numLCUInPicWidth;

	xInterval = ((( (img_width +lcuWidth -1) / lcuWidth) +1) /4 *lcuWidth) ;  
	yInterval = ((((img_height +lcuHeight -1) / lcuHeight) +1) /4 *lcuHeight) ;

	maxNumTemporalLayer =   (int)(log10((float)(gop_size))/log10(2.0) + 1);

	Enc_ALF = (EncALFVar *)malloc(1*sizeof(EncALFVar));
	Enc_ALF->m_alfLowLatencyEncoding = (Boolean) input->alf_LowLatencyEncodingEnable;
	Enc_ALF->m_alfReDesignIteration  = 3;
	Enc_ALF->m_uiBitIncrement        = 0;
	memset(Enc_ALF->m_y_temp,0,ALF_MAX_NUM_COEF * sizeof(double));
	memset(Enc_ALF->m_pixAcc_merged,0,NO_VAR_BINS * sizeof(double));
	memset(Enc_ALF->m_varIndTab,0,NO_VAR_BINS * sizeof(int));

	for (compIdx=0;compIdx < NUM_ALF_COMPONENT; compIdx++)
	{
		Enc_ALF->m_alfCorr[compIdx]           = (AlfCorrData**)malloc(NumCUInFrame * sizeof(AlfCorrData*));
		Enc_ALF->m_alfNonSkippedCorr[compIdx] = (AlfCorrData**)malloc(NumCUInFrame * sizeof(AlfCorrData*));
				
		for ( n = 0; n < NumCUInFrame; n++)
		{
			AllocateAlfCorrData(&(Enc_ALF->m_alfCorr[compIdx][n]),compIdx);
			AllocateAlfCorrData(&(Enc_ALF->m_alfNonSkippedCorr[compIdx][n]),compIdx);
		}
		AllocateAlfCorrData(&(Enc_ALF->m_alfCorrMerged[compIdx]),compIdx);
	}

	Enc_ALF->m_alfPrevCorr = (AlfCorrData****)malloc(maxNumTemporalLayer * sizeof(AlfCorrData***));
	for (n = 0; n < maxNumTemporalLayer; n++)
	{
		Enc_ALF->m_alfPrevCorr[n] = (AlfCorrData***) malloc(NUM_ALF_COMPONENT * sizeof(AlfCorrData**));
		for (compIdx=0; compIdx<NUM_ALF_COMPONENT; compIdx++)
		{
			Enc_ALF->m_alfPrevCorr[n][compIdx] = (AlfCorrData**) malloc(NumCUInFrame * sizeof(AlfCorrData*));
			for (i=0; i<NumCUInFrame;i++)
			{
				AllocateAlfCorrData(&(Enc_ALF->m_alfPrevCorr[n][compIdx][i]),compIdx);
			}
		}
	}
	for (n = 0; n < NO_VAR_BINS; n++)
	{
		Enc_ALF->m_y_merged[n] = (double*) malloc(numCoef * sizeof(double));
		Enc_ALF->m_E_merged[n] = (double**) malloc(numCoef * sizeof(double*));
		for (i = 0;i<numCoef;i++)
		{
			Enc_ALF->m_E_merged[n][i] = (double*)malloc(numCoef * sizeof(double));
			memset(Enc_ALF->m_E_merged[n][i],0,numCoef*sizeof(double));
		}
	}

	Enc_ALF->m_E_temp = (double**) malloc(numCoef * sizeof(double*));
	for (n = 0; n < numCoef; n++)
	{
		Enc_ALF->m_E_temp[n] = (double*) malloc(numCoef * sizeof(double));
		memset(Enc_ALF->m_E_temp[n],0,numCoef*sizeof(double));
	}	

	Enc_ALF->m_alfPictureParam = (ALFParam***) malloc(maxNumTemporalLayer * sizeof(ALFParam**));
	for (n = 0; n < maxNumTemporalLayer; n++)
	{
		Enc_ALF->m_alfPictureParam[n] = (ALFParam**)malloc(NUM_ALF_COMPONENT * sizeof(ALFParam*));
		for (compIdx = 0; compIdx < NUM_ALF_COMPONENT; compIdx++)
		{
			AllocateAlfPar(&(Enc_ALF->m_alfPictureParam[n][compIdx]),compIdx);
		}
	}

	for (n = 0; n < NO_VAR_BINS; n++)
	{
		Enc_ALF->m_coeffNoFilter[n] = (int*) malloc(numCoef * sizeof(int));
		memset(&(Enc_ALF->m_coeffNoFilter[n][0]),0,sizeof(int)*numCoef);
		Enc_ALF->m_coeffNoFilter[n][numCoef-1] = (1<<ALF_NUM_BIT_SHIFT);
	}

	Enc_ALF->m_filterCoeffSym = (int**) malloc(NO_VAR_BINS * sizeof(int*));
	for(g=0 ; g< (int)NO_VAR_BINS; g++)
	{
		Enc_ALF->m_filterCoeffSym[g] = (int*)malloc(ALF_MAX_NUM_COEF * sizeof(int));
		memset(Enc_ALF->m_filterCoeffSym[g],0,ALF_MAX_NUM_COEF * sizeof(int));
	}

	Enc_ALF->m_numSlicesDataInOneLCU = (int*) malloc(NumCUInFrame * sizeof(int));
	memset(Enc_ALF->m_numSlicesDataInOneLCU,0,NumCUInFrame * sizeof(int));

	Enc_ALF->m_varImg = (byte**) malloc(img_height * sizeof(byte*));	
	for (n=0;n<img_height;n++)
	{
		Enc_ALF->m_varImg[n] = (byte*) malloc(img_width * sizeof(byte));
		memset(Enc_ALF->m_varImg[n],0,img_width * sizeof(byte));
	}

	Enc_ALF->m_AlfLCUEnabled = (Boolean**)malloc(NumCUInFrame * sizeof(Boolean*));
	for (n=0;n<NumCUInFrame;n++)
	{
		Enc_ALF->m_AlfLCUEnabled[n] = (Boolean*) malloc(NUM_ALF_COMPONENT * sizeof(Boolean));
		memset(Enc_ALF->m_AlfLCUEnabled[n],0,NUM_ALF_COMPONENT * sizeof(Boolean));
	}

	for(i = 0; i < img_height; i=i+4)
	{
		yIndex = (yInterval == 0)?(3):(Clip_post( 3, i / yInterval));
		yIndexOffset = yIndex * 4 ;
		for(g = 0; g < img_width; g=g+4)
		{
			xIndex = (xInterval==0)?(3):(Clip_post( 3, g / xInterval));
			Enc_ALF->m_varImg[i>>LOG2_VAR_SIZE_H][g>>LOG2_VAR_SIZE_W] = regionTable[yIndexOffset + xIndex];
		}
	}
	Enc_ALF->m_cs_alf_cu_ctr = create_coding_state ();
#if ALF_ESTIMATE_AEC_RATE
  Enc_ALF->m_cs_alf_initial = create_coding_state();
#endif
}
void destroyAlfGlobalBuffers(ImageParameters* img,unsigned int uiMaxSizeInbit)
{
	int lcuHeight,lcuWidth, img_height,img_width;
	int NumCUInFrame,numLCUInPicWidth,numLCUInPicHeight;
	int compIdx, n, i, g;
	int numCoef = (int) ALF_MAX_NUM_COEF;
//	int maxNumTemporalLayer =   (int)(logf((float)(gop_size))/logf(2.0) + 1);
	int maxNumTemporalLayer =   (int)(log10f((float)(gop_size))/log10f(2.0) + 1);//zhaohaiwu 20151011

	lcuHeight         = 1<<uiMaxSizeInbit;
	lcuWidth          = lcuHeight;
	img_height        = img->height;
	img_width         = img->width;
	numLCUInPicWidth  = img_width / lcuWidth ;
	numLCUInPicHeight = img_height / lcuHeight ;
	numLCUInPicWidth  += (img_width % lcuWidth)? 1 : 0;
	numLCUInPicHeight += (img_height % lcuHeight)? 1 : 0;
	NumCUInFrame = numLCUInPicHeight * numLCUInPicWidth;

	for (compIdx=0;compIdx<NUM_ALF_COMPONENT;compIdx++)
	{
		for (n=0;n<NumCUInFrame;n++)
		{
#if FIX_LEAK   
            freeAlfCorrData(Enc_ALF->m_alfCorr[compIdx][n]);
            freeAlfCorrData(Enc_ALF->m_alfNonSkippedCorr[compIdx][n]);
#else
			free(Enc_ALF->m_alfCorr[compIdx][n]);
			free(Enc_ALF->m_alfNonSkippedCorr[compIdx][n]);
#endif
		}
		free(Enc_ALF->m_alfCorr[compIdx]);
		Enc_ALF->m_alfCorr[compIdx] = NULL;
		free(Enc_ALF->m_alfNonSkippedCorr[compIdx]);
		Enc_ALF->m_alfNonSkippedCorr[compIdx] = NULL;
#if FIX_LEAK
		freeAlfCorrData(Enc_ALF->m_alfCorrMerged[compIdx]);
#endif
		free(Enc_ALF->m_alfCorrMerged[compIdx]);
	}

	for (n = 0; n < NO_VAR_BINS; n++)
	{
		free(Enc_ALF->m_y_merged[n]); Enc_ALF->m_y_merged[n] = NULL;
		for (i = 0;i<numCoef;i++)
		{
			free(Enc_ALF->m_E_merged[n][i]);
		}
		free(Enc_ALF->m_E_merged[n]); Enc_ALF->m_E_merged[n] = NULL;
	}

	for (i = 0;i<numCoef;i++)
	{
		free(Enc_ALF->m_E_temp[i]);
	}
	free(Enc_ALF->m_E_temp); Enc_ALF->m_E_temp = NULL;

	for (n = 0;n < maxNumTemporalLayer; n++)
	{
		for (compIdx=0;compIdx<NUM_ALF_COMPONENT;compIdx++)
		{
			for (i=0;i<NumCUInFrame;i++)
			{
#if FIX_LEAK
                freeAlfCorrData(Enc_ALF->m_alfPrevCorr[n][compIdx][i]);
#endif
				free(Enc_ALF->m_alfPrevCorr[n][compIdx][i]);
			}
			free(Enc_ALF->m_alfPrevCorr[n][compIdx]);
#if FIX_LEAK
            freeAlfPar(Enc_ALF->m_alfPictureParam[n][compIdx],compIdx);
#else
			free(Enc_ALF->m_alfPictureParam[n][compIdx]);
#endif
		}
		free(Enc_ALF->m_alfPrevCorr[n]);
		free(Enc_ALF->m_alfPictureParam[n]);
	}
	free(Enc_ALF->m_alfPrevCorr); Enc_ALF->m_alfPrevCorr = NULL ;
	free(Enc_ALF->m_alfPictureParam); Enc_ALF->m_alfPictureParam = NULL;

	for (n=0;n<NO_VAR_BINS;n++)
	{
		free(Enc_ALF->m_coeffNoFilter[n]);
	}
	free(Enc_ALF->m_numSlicesDataInOneLCU);
	
	for (n=0;n<img_height;n++)
	{
		free(Enc_ALF->m_varImg[n]);
	}
	free(Enc_ALF->m_varImg); Enc_ALF->m_varImg = NULL;

	
	for (n=0;n<NumCUInFrame;n++)
	{
		free(Enc_ALF->m_AlfLCUEnabled[n]);
	}
	free(Enc_ALF->m_AlfLCUEnabled); Enc_ALF->m_AlfLCUEnabled = NULL;

	for(g=0 ; g< (int)NO_VAR_BINS; g++)
	{
		free(Enc_ALF->m_filterCoeffSym[g]);
	}
	free(Enc_ALF->m_filterCoeffSym); Enc_ALF->m_filterCoeffSym = NULL;
	delete_coding_state(Enc_ALF->m_cs_alf_cu_ctr);
#if ALF_ESTIMATE_AEC_RATE
  delete_coding_state(Enc_ALF->m_cs_alf_initial);
#endif
	free(Enc_ALF); Enc_ALF = NULL;
}

void AllocateAlfCorrData(AlfCorrData **dst,int cIdx)
{
	const int numCoef = ALF_MAX_NUM_COEF;
	const int maxNumGroups = NO_VAR_BINS;

	int numGroups = (cIdx == ALF_Y)?(maxNumGroups):(1);
	int i,j,g;
     
	(*dst) = (AlfCorrData*)malloc(sizeof(AlfCorrData));
	(*dst)->componentID = cIdx;

	(*dst)->ECorr  = (double***) malloc(numGroups * sizeof(double**));
	(*dst)->yCorr  = (double**) malloc(numGroups * sizeof(double*));
	(*dst)->pixAcc = (double*) malloc(numGroups * sizeof(double));
	for(g= 0; g< numGroups; g++)
	{
		(*dst)->yCorr[g] = (double*) malloc(numCoef * sizeof(double));
		for(j=0; j< numCoef; j++)
		{
			(*dst)->yCorr[g][j] = 0;
		}

		(*dst)->ECorr[g] = (double**) malloc(numCoef * sizeof(double*));
		for(i=0; i< numCoef; i++)
		{
			(*dst)->ECorr[g][i] = (double*) malloc(numCoef * sizeof(double));
			for(j=0; j< numCoef; j++)
			{
				(*dst)->ECorr[g][i][j] = 0;
			}
		}
		(*dst)->pixAcc[g] = 0;  
	}
}
void freeAlfCorrData(AlfCorrData *dst)
{
	const int numCoef = ALF_MAX_NUM_COEF;
	const int maxNumGroups = NO_VAR_BINS;
	int numGroups,i,g;
	if(dst->componentID >=0)
	{
		numGroups = (dst->componentID == ALF_Y)?(maxNumGroups):(1);

		for(g= 0; g< numGroups; g++)
		{
			for(i=0; i< numCoef; i++)
			{
				free(dst->ECorr[g][i]);
			}
			free(dst->ECorr[g]);
			free(dst->yCorr[g]);
		}
		free(dst->ECorr);
		free(dst->yCorr);
		free(dst->pixAcc);
	}

}
void reset_alfCorr(AlfCorrData* alfCorr)
{
	if(alfCorr->componentID >=0)
	{
		int numCoef = ALF_MAX_NUM_COEF;
		int maxNumGroups = NO_VAR_BINS;
		int g,j,i;
		int numGroups = (alfCorr->componentID == ALF_Y)?(maxNumGroups):(1);
		for(g=0; g< numGroups; g++)
		{
			alfCorr->pixAcc[g] = 0;

			for(j=0; j< numCoef; j++)
			{
				alfCorr->yCorr[g][j] = 0;
				for(i=0; i< numCoef; i++)
				{
					alfCorr->ECorr[g][j][i] = 0;
				}
			}
		}
	}

}
/*
*************************************************************************
* Function: Calculate the correlation matrix for each LCU
* Input:  
*  skipCUBoundaries  : Boundary skip flag
*           compIdx  : Image component index
*            lcuAddr : The address of current LCU
* (ctuXPos,ctuYPos)  : The LCU position
*          isXXAvail : The Available of neighboring LCU
*            pPicOrg : The original image buffer
*            pPicSrc : The distortion image buffer
*           stride : The width of image buffer
* Output: 
* Return:
*************************************************************************
*/
void getStatisticsOneLCU(Boolean skipCUBoundaries, int compIdx, int lcuAddr
  , int ctuYPos, int ctuXPos, int ctuHeight, int ctuWidth
  , Boolean isAboveAvail, Boolean isBelowAvail, Boolean isLeftAvail, Boolean isRightAvail
  , Boolean isAboveLeftAvail, Boolean isAboveRightAvail, AlfCorrData* alfCorr, byte* pPicOrg, byte* pPicSrc
  , int stride, int formatShift, int numLCUPicWidth, int NumCUsInFrame)
{
	int ypos, xpos, height, width;

	switch(compIdx)
	{
	case ALF_Cb:
	case ALF_Cr:
		{
			ypos    = (ctuYPos   >> formatShift);
      xpos    = (ctuXPos   >> formatShift);
      height  = (ctuHeight >> formatShift);
      width   = (ctuWidth  >> formatShift);

      calcCorrOneCompRegionChma(pPicOrg, pPicSrc, stride, ypos, xpos, height, width, alfCorr->ECorr[0], alfCorr->yCorr[0], isLeftAvail, isRightAvail, isAboveAvail, isBelowAvail, isAboveLeftAvail, isAboveRightAvail);
    }
    break;
  case ALF_Y:
    {
      ypos    = (ctuYPos   >> formatShift);
      xpos    = (ctuXPos   >> formatShift);
      height  = (ctuHeight >> formatShift);
      width   = (ctuWidth  >> formatShift);
      calcCorrOneCompRegionLuma(pPicOrg, pPicSrc, stride, ypos, xpos, height, width, alfCorr->ECorr, alfCorr->yCorr, alfCorr->pixAcc, isLeftAvail, isRightAvail, isAboveAvail, isBelowAvail, isAboveLeftAvail, isAboveRightAvail);
		}
		break;
	default:
		{
			printf("Not a legal component index for ALF\n");
			assert(0);
			exit(-1);
		}
	}
}

/*
*************************************************************************
* Function: Calculate the correlation matrix for Luma
*************************************************************************
*/
void calcCorrOneCompRegionLuma(byte* imgOrg, byte* imgPad, int stride, int yPos, int xPos, int height, int width
  ,double ***eCorr, double **yCorr, double *pixAcc, int isLeftAvail, int isRightAvail, int isAboveAvail, int isBelowAvail, int isAboveLeftAvail, int isAboveRightAvail)
{
	int yPosEnd = yPos + height;
	int xPosEnd = xPos + width;
	int N = ALF_MAX_NUM_COEF; //m_sqrFiltLengthTab[0];

	int startPosLuma	  = isAboveAvail ? (yPos - 4) : yPos;
	int endPosLuma        = isBelowAvail ? (yPos + height - 4) : (yPos + height);
	int xOffSetLeft		  = isLeftAvail  ? -3 : 0;
	int xOffSetRight	  = isRightAvail ?  3 : 0;

	int yUp, yBottom;
	int xLeft, xRight;

	int ELocal[ALF_MAX_NUM_COEF];
	byte *imgPad1, *imgPad2, *imgPad3, *imgPad4, *imgPad5, *imgPad6;
	int i, j, k, l, yLocal, varInd;
	double **E;
	double *yy;

	imgPad += startPosLuma*stride;
	imgOrg += startPosLuma*stride;
	varInd = Enc_ALF->m_varImg[yPos>>LOG2_VAR_SIZE_H][xPos>>LOG2_VAR_SIZE_W];

	for (i=startPosLuma; i<endPosLuma; i++)
	{
		yUp		= Clip3(startPosLuma, endPosLuma-1, i-1);
		yBottom = Clip3(startPosLuma, endPosLuma-1, i+1);
		imgPad1 = imgPad + (yBottom-i)*stride;
		imgPad2 = imgPad + (yUp - i)*stride;

		yUp		= Clip3(startPosLuma, endPosLuma-1, i-2);
		yBottom = Clip3(startPosLuma, endPosLuma-1, i+2);
		imgPad3 = imgPad + (yBottom-i)*stride;
		imgPad4 = imgPad + (yUp-i)*stride;

		yUp		= Clip3(startPosLuma, endPosLuma-1, i-3);
		yBottom = Clip3(startPosLuma, endPosLuma-1, i+3);
		imgPad5 = imgPad + (yBottom-i)*stride;
		imgPad6 = imgPad + (yUp-i)*stride;

		for (j=xPos;j<xPosEnd;j++)
		{
			memset(ELocal, 0, N*sizeof(int));

			ELocal[0] = (imgPad5[j] + imgPad6[j]);
            ELocal[1] = (imgPad3[j] + imgPad4[j]);
#if ALF_BOUNDARY_FIX
			ELocal[3] = (imgPad1[j] + imgPad2[j]);

			// upper left c2
			xLeft = check_filtering_unit_boundary_extension(j - 1, i - 1, xPos, yPos, xPos, startPosLuma, xPosEnd - 1, endPosLuma - 1, isAboveLeftAvail, isLeftAvail, isAboveRightAvail, isRightAvail);
			ELocal[2] = imgPad2[xLeft];

			// upper right c4
			xRight = check_filtering_unit_boundary_extension(j + 1, i - 1, xPos, yPos, xPos, startPosLuma, xPosEnd - 1, endPosLuma - 1, isAboveLeftAvail, isLeftAvail, isAboveRightAvail, isRightAvail);
			ELocal[4] = imgPad2[xRight];

			// lower left c4
			xLeft = check_filtering_unit_boundary_extension(j - 1, i + 1, xPos, yPos, xPos, startPosLuma, xPosEnd - 1, endPosLuma - 1, isAboveLeftAvail, isLeftAvail, isAboveRightAvail, isRightAvail);
			ELocal[4] +=  imgPad1[xLeft];

			// lower right c2
			xRight = check_filtering_unit_boundary_extension(j + 1, i + 1, xPos, yPos, xPos, startPosLuma, xPosEnd - 1, endPosLuma - 1, isAboveLeftAvail, isLeftAvail, isAboveRightAvail, isRightAvail);
			ELocal[2] +=  imgPad1[xRight];


			xLeft  = Clip3(xPos+xOffSetLeft, xPosEnd-1+xOffSetRight, j-1);
			xRight = Clip3(xPos+xOffSetLeft, xPosEnd-1+xOffSetRight, j+1);
			ELocal[7] = (imgPad[xRight] + imgPad[xLeft]);


			xLeft  = Clip3(xPos+xOffSetLeft, xPosEnd-1+xOffSetRight, j-2);
			xRight = Clip3(xPos+xOffSetLeft, xPosEnd-1+xOffSetRight, j+2);
			ELocal[6] = (imgPad[xRight] + imgPad[xLeft]);

			xLeft  = Clip3(xPos+xOffSetLeft, xPosEnd-1+xOffSetRight, j-3);
			xRight = Clip3(xPos+xOffSetLeft, xPosEnd-1+xOffSetRight, j+3);
			ELocal[5] = (imgPad[xRight] + imgPad[xLeft]);

#else 
      xLeft  = Clip3(xPos+xOffSetLeft, xPosEnd-1+xOffSetRight, j-1);
      xRight = Clip3(xPos+xOffSetLeft, xPosEnd-1+xOffSetRight, j+1);
      // When the samples from above left region is used and the the above left region does not exist 
      if (!isAboveLeftAvail && isLeftAvail && j == xPos && i <= yPos)
      {
        xLeft = j;
      }
      if (!isAboveRightAvail && isRightAvail && j == xPosEnd - 1 && i <= yPos)
      {
        xRight = j;
      }
      ELocal[2] = imgPad2[xLeft];
      ELocal[3] = (imgPad1[j] + imgPad2[j]);
      ELocal[4] = imgPad2[xRight];
      ELocal[7] = (imgPad[xRight] + imgPad[xLeft]);

      xLeft = Clip3(xPos + xOffSetLeft, xPosEnd - 1 + xOffSetRight, j - 1);
      xRight = Clip3(xPos + xOffSetLeft, xPosEnd - 1 + xOffSetRight, j + 1);

      if (!isAboveLeftAvail && isLeftAvail && j == xPos && i < yPos)
      {
        xLeft = j;
      }
      if (!isAboveRightAvail && isRightAvail && j == xPosEnd - 1 && i < yPos)
      {
        xRight = j;
      }
      ELocal[2] += imgPad1[xRight];
      ELocal[4] += imgPad1[xLeft];

      xLeft  = Clip3(xPos+xOffSetLeft, xPosEnd-1+xOffSetRight, j-2);
      xRight = Clip3(xPos+xOffSetLeft, xPosEnd-1+xOffSetRight, j+2);
      if (!isAboveLeftAvail && isLeftAvail && j <= xPos + 1 && i < yPos)
      {
        xLeft = j;
      }
      if (!isAboveRightAvail && isRightAvail && j >= xPosEnd - 2 && i < yPos)
      {
        xRight = j;
      }
      ELocal[6] = (imgPad[xRight] + imgPad[xLeft]);

      xLeft  = Clip3(xPos+xOffSetLeft, xPosEnd-1+xOffSetRight, j-3);
      xRight = Clip3(xPos+xOffSetLeft, xPosEnd-1+xOffSetRight, j+3);
      if (!isAboveLeftAvail && isLeftAvail && j<=xPos+2 && i<yPos)
      {
        xLeft = j;
      }
      if (!isAboveRightAvail && isRightAvail && j>=xPosEnd-3 && i<yPos)
      {
        xRight = j;
      }
			ELocal[5] = (imgPad[xRight] + imgPad[xLeft]);
#endif  //end PEISONF_FIX
			ELocal[8] = (imgPad[j  ]);

			yLocal= imgOrg[j];
			pixAcc[varInd] += (yLocal*yLocal);
			E  = eCorr[varInd];
			yy = yCorr[varInd];

			for (k=0; k<N; k++)
			{
				for (l=k; l<N; l++)
				{
					E[k][l]+=(double)(ELocal[k]*ELocal[l]);
				}
				yy[k]+=(double)(ELocal[k]*yLocal);
			}
		}

		imgPad += stride;
		imgOrg += stride;
	}

	for (varInd=0; varInd<NO_VAR_BINS; varInd++)
	{
		E = eCorr[varInd];
		for (k=1; k<N; k++)
		{
			for (l=0; l<k; l++)
			{
				E[k][l] = E[l][k];
			}
		}
	}

}


/*
*************************************************************************
* Function: Calculate the correlation matrix for Chroma
*************************************************************************
*/
void calcCorrOneCompRegionChma(byte* imgOrg, byte* imgPad, int stride , int yPos, int xPos, int height, int width, double **eCorr, double *yCorr, int isLeftAvail, int isRightAvail, int isAboveAvail, int isBelowAvail, int isAboveLeftAvail, int isAboveRightAvail)
{
	int yPosEnd = yPos + height;
	int xPosEnd = xPos + width;
	int N = ALF_MAX_NUM_COEF; //m_sqrFiltLengthTab[0];

	int startPosChroma    = isAboveAvail ? (yPos - 4) : yPos;
	int endPosChroma      = isBelowAvail ? (yPos + height - 4) : (yPos + height);
	int xOffSetLeft		  = isLeftAvail  ? -3 : 0;
	int xOffSetRight	  = isRightAvail ?  3 : 0;

	int yUp, yBottom;
	int xLeft, xRight;

	int ELocal[ALF_MAX_NUM_COEF];
	byte *imgPad1, *imgPad2, *imgPad3, *imgPad4, *imgPad5, *imgPad6;
	int i, j, k, l, yLocal;

	imgPad += startPosChroma*stride;
	imgOrg += startPosChroma*stride;

	for (i=startPosChroma; i<endPosChroma; i++)
	{
		yUp		= Clip3(startPosChroma, endPosChroma-1, i-1);
		yBottom = Clip3(startPosChroma, endPosChroma-1, i+1);
		imgPad1 = imgPad + (yBottom-i)*stride;
		imgPad2 = imgPad + (yUp - i)*stride;

		yUp		= Clip3(startPosChroma, endPosChroma-1, i-2);
		yBottom = Clip3(startPosChroma, endPosChroma-1, i+2);
		imgPad3 = imgPad + (yBottom-i)*stride;
		imgPad4 = imgPad + (yUp-i)*stride;

		yUp		= Clip3(startPosChroma, endPosChroma-1, i-3);
		yBottom = Clip3(startPosChroma, endPosChroma-1, i+3);
		imgPad5 = imgPad + (yBottom-i)*stride;
		imgPad6 = imgPad + (yUp-i)*stride;
		for (j= xPos; j< xPosEnd; j++)
		{
			memset(ELocal, 0, N*sizeof(int));

			ELocal[0] = (imgPad5[j] + imgPad6[j]);
			ELocal[1] = (imgPad3[j] + imgPad4[j]);
#if ALF_BOUNDARY_FIX
			ELocal[3] = (imgPad1[j] + imgPad2[j]);

			// upper left c2
			xLeft = check_filtering_unit_boundary_extension(j - 1, i - 1, xPos, yPos, xPos, startPosChroma, xPosEnd - 1, endPosChroma - 1, isAboveLeftAvail, isLeftAvail, isAboveRightAvail, isRightAvail);
			ELocal[2] = imgPad2[xLeft];

			// upper right c4
			xRight = check_filtering_unit_boundary_extension(j + 1, i - 1, xPos, yPos, xPos, startPosChroma, xPosEnd - 1, endPosChroma - 1, isAboveLeftAvail, isLeftAvail, isAboveRightAvail, isRightAvail);
			ELocal[4] = imgPad2[xRight];

			// lower left c4
			xLeft = check_filtering_unit_boundary_extension(j - 1, i + 1, xPos, yPos, xPos, startPosChroma, xPosEnd - 1, endPosChroma - 1, isAboveLeftAvail, isLeftAvail, isAboveRightAvail, isRightAvail);
			ELocal[4] +=  imgPad1[xLeft];

			// lower right c2
			xRight = check_filtering_unit_boundary_extension(j + 1, i + 1, xPos, yPos, xPos, startPosChroma, xPosEnd - 1, endPosChroma - 1, isAboveLeftAvail, isLeftAvail, isAboveRightAvail, isRightAvail);
			ELocal[2] +=  imgPad1[xRight];


			xLeft  = Clip3(xPos+xOffSetLeft, xPosEnd-1+xOffSetRight, j-1);
			xRight = Clip3(xPos+xOffSetLeft, xPosEnd-1+xOffSetRight, j+1);
			ELocal[7] = (imgPad[xRight] + imgPad[xLeft]);


			xLeft  = Clip3(xPos+xOffSetLeft, xPosEnd-1+xOffSetRight, j-2);
			xRight = Clip3(xPos+xOffSetLeft, xPosEnd-1+xOffSetRight, j+2);
			ELocal[6] = (imgPad[xRight] + imgPad[xLeft]);

			xLeft  = Clip3(xPos+xOffSetLeft, xPosEnd-1+xOffSetRight, j-3);
			xRight = Clip3(xPos+xOffSetLeft, xPosEnd-1+xOffSetRight, j+3);
			ELocal[5] = (imgPad[xRight] + imgPad[xLeft]);
#else 
			xLeft  = Clip3(xPos+xOffSetLeft, xPosEnd-1+xOffSetRight, j-1);
			xRight = Clip3(xPos+xOffSetLeft, xPosEnd-1+xOffSetRight, j+1);
      // When the samples from above left region is used and the the above left region does not exist 
      if (!isAboveLeftAvail && isLeftAvail && j == xPos && i <= yPos)
      {
        xLeft = j;
      }
      if (!isAboveRightAvail && isRightAvail && j == xPosEnd - 1 && i <= yPos)
      {
        xRight = j;
      }
      ELocal[2] = imgPad2[xLeft];
      ELocal[3] = (imgPad1[j] + imgPad2[j]);
      ELocal[4] = imgPad2[xRight];
      ELocal[7] = (imgPad[xRight] + imgPad[xLeft]);

      xLeft = Clip3(xPos + xOffSetLeft, xPosEnd - 1 + xOffSetRight, j - 1);
      xRight = Clip3(xPos + xOffSetLeft, xPosEnd - 1 + xOffSetRight, j + 1);

      if (!isAboveLeftAvail && isLeftAvail && j == xPos && i < yPos)
      {
        xLeft = j;
      }
      if (!isAboveRightAvail && isRightAvail && j == xPosEnd - 1 && i < yPos)
      {
        xRight = j;
      }
      ELocal[2] += imgPad1[xRight];
      ELocal[4] += imgPad1[xLeft];

			xLeft  = Clip3(xPos+xOffSetLeft, xPosEnd-1+xOffSetRight, j-2);
			xRight = Clip3(xPos+xOffSetLeft, xPosEnd-1+xOffSetRight, j+2);
      if (!isAboveLeftAvail && isLeftAvail && j <= xPos + 1 && i < yPos)
      {
        xLeft = j;
      }
      if (!isAboveRightAvail && isRightAvail && j >= xPosEnd - 2 && i < yPos)
      {
        xRight = j;
      }
			ELocal[6] = (imgPad[xRight] + imgPad[xLeft]);

			xLeft  = Clip3(xPos+xOffSetLeft, xPosEnd-1+xOffSetRight, j-3);
			xRight = Clip3(xPos+xOffSetLeft, xPosEnd-1+xOffSetRight, j+3);
      if (!isAboveLeftAvail && isLeftAvail && j<=xPos+2 && i<yPos)
      {
        xLeft = j;
      }
      if (!isAboveRightAvail && isRightAvail && j>=xPosEnd-3 && i<yPos)
      {
        xRight = j;
      }
			ELocal[5] = (imgPad[xRight] + imgPad[xLeft]);
#endif
			ELocal[8] = (imgPad[j  ]);

			yLocal= (int)imgOrg[j];

			for(k=0; k<N; k++)
			{
				eCorr[k][k] += ELocal[k]*ELocal[k];
				for(l=k+1; l<N; l++)
				{
					eCorr[k][l] += ELocal[k]*ELocal[l];
				}

				yCorr[k] += yLocal*ELocal[k];
			}
		}

		imgPad+= stride;
		imgOrg+= stride;
	}

	for(j=0; j<N-1; j++)
	{
		for(i=j+1; i<N; i++)
		{
			eCorr[i][j] = eCorr[j][i];
		}
	}
}


/*
*************************************************************************
* Function: ALF parameter selection
* Input:  
*    alfPictureParam: The ALF parameter
*              apsId: The ALF parameter index in the buffer
*       isNewApsSent£ºThe New flag index
*       lambda      : The lambda value in the ALF-RD decision
* Return:
*************************************************************************
*/
void setCurAlfParam(ALFParam** alfPictureParam, double lambda)
{
	AlfCorrData*** alfLcuCorr = NULL;
	int temporalLayer = getTemporalLayerNo(img->tr,gop_size);
	int compIdx,i;
	AlfCorrData* alfPicCorr[NUM_ALF_COMPONENT];
	double costMin, cost;
	ALFParam* tempAlfParam[NUM_ALF_COMPONENT];
#if ALF_ENC_IMPORVE_RATE_ESTIMATE
  int picHeaderBitrate = 0;
  for(compIdx=0; compIdx < NUM_ALF_COMPONENT; compIdx++)
  {
    AllocateAlfPar(&(tempAlfParam[compIdx]),compIdx);
  }
#endif

	if (Enc_ALF->m_alfLowLatencyEncoding)
	{
		for (compIdx=0; compIdx< 3; compIdx++)
		{
			copyALFparam(alfPictureParam[compIdx],Enc_ALF->m_alfPictureParam[temporalLayer][compIdx]);
		}
		alfLcuCorr = Enc_ALF->m_alfPrevCorr[temporalLayer];
#if ALF_ENC_IMPORVE_RATE_ESTIMATE
		costMin= executePicLCUOnOffDecision(alfPictureParam, lambda , TRUE, alfLcuCorr, NULL, NULL, NULL, NULL, NULL, NULL,0);
	picHeaderBitrate = estimateALFBitrateInPicHeader(alfPictureParam);
	costMin += (double)picHeaderBitrate * lambda;
#endif
	}
	else
	{
    costMin = MAX_DOUBLE;

		for(compIdx=0; compIdx< NUM_ALF_COMPONENT; compIdx++)
		{
			AllocateAlfCorrData(&(alfPicCorr[compIdx]),compIdx);
		}

		accumulateLCUCorrelations(alfPicCorr, Enc_ALF->m_alfCorr, TRUE);
#if ALF_ENC_IMPORVE_RATE_ESTIMATE
    decideAlfPictureParam(tempAlfParam, alfPicCorr,lambda);
#else
		decideAlfPictureParam(alfPictureParam, alfPicCorr,lambda);
#endif

		for(i=0; i< Enc_ALF->m_alfReDesignIteration; i++)
		{
#if ALF_ENC_IMPORVE_RATE_ESTIMATE
      if(i != 0)
      {
        //redesign filter according to the last on off results
        accumulateLCUCorrelations(alfPicCorr, Enc_ALF->m_alfCorr, FALSE);
        decideAlfPictureParam(tempAlfParam, alfPicCorr,lambda);        
      }
      //estimate cost
	  cost= executePicLCUOnOffDecision(tempAlfParam, lambda , TRUE, Enc_ALF->m_alfCorr, NULL, NULL, NULL, NULL, NULL, NULL,0);
	  picHeaderBitrate = estimateALFBitrateInPicHeader(tempAlfParam);
      cost += (double)picHeaderBitrate * lambda;

      if(cost < costMin)
      {
        costMin = cost;
        for(compIdx=0; compIdx < NUM_ALF_COMPONENT; compIdx++)
        {
          copyALFparam(alfPictureParam[compIdx],tempAlfParam[compIdx]);
        }
      }

#else
			estimateLcuControl(alfPictureParam, Enc_ALF->m_alfCorr, TRUE,NULL);

			//redesign filter
			accumulateLCUCorrelations(alfPicCorr, Enc_ALF->m_alfCorr, FALSE);
			decideAlfPictureParam(alfPictureParam, alfPicCorr,lambda);
#endif
		}

		for(compIdx=0; compIdx< NUM_ALF_COMPONENT; compIdx++)
		{
			freeAlfCorrData(alfPicCorr[compIdx]);
#if FIX_LEAK
            free(alfPicCorr[compIdx]);
#endif
		}
		alfLcuCorr = Enc_ALF->m_alfCorr;
	}

#if !ALF_ENC_IMPORVE_RATE_ESTIMATE
	for(compIdx=0; compIdx < NUM_ALF_COMPONENT; compIdx++)
	{
		AllocateAlfPar(&(tempAlfParam[compIdx]),compIdx);
	}
	costMin = estimatePicCtrl(alfPictureParam, alfLcuCorr, TRUE, *apsId,lambda); 
#endif

	for(compIdx=0; compIdx < NUM_ALF_COMPONENT; compIdx++)
	{
#if FIX_LEAK
        freeAlfPar(tempAlfParam[compIdx], compIdx);
#else
		free(tempAlfParam[compIdx]);
#endif
	}
}

#if !ALF_ENC_IMPORVE_RATE_ESTIMATE
double estimatePicCtrl(ALFParam** alfPictureParam, AlfCorrData*** alfLcuCorr, Boolean considerApsOverhead, int apsId,double lambda)
{
    long int distReduction[NUM_ALF_COMPONENT];
	Boolean   compEnable[NUM_ALF_COMPONENT], bestCompEnable[NUM_ALF_COMPONENT];
	long  sumDist;
	int    sumRate,combinationIdx,compIdx,apsOverhead,sliceHeaderOverhead;
	double sumCost, minCost = MAX_DOUBLE;
	Boolean isAllCompsDisabled;
	/////
	int lcuHeight,lcuWidth, img_height,img_width;
	int ctu,NumCUsInFrame,numLCUInPicWidth,numLCUInPicHeight;

	lcuHeight         = 1<<g_MaxSizeInbit;
	lcuWidth          = lcuHeight;
	img_height        = img->height;
	img_width         = img->width;
	numLCUInPicWidth  = img_width / lcuWidth ;
	numLCUInPicHeight = img_height / lcuHeight ;
	numLCUInPicWidth  += (img_width % lcuWidth)? 1 : 0;
	numLCUInPicHeight += (img_height % lcuHeight)? 1 : 0;
	NumCUsInFrame = numLCUInPicHeight * numLCUInPicWidth;
	//double lambda = m_dLambdaLuma;
	//estimate distortion reduction for each component
	estimateLcuControl(alfPictureParam, alfLcuCorr, FALSE, distReduction);

	for(combinationIdx=0; combinationIdx< (int)(1<<NUM_ALF_COMPONENT); combinationIdx++)
	{
		sumDist = sumRate = 0;
		for(compIdx= 0;compIdx < NUM_ALF_COMPONENT; compIdx++)
		{
			compEnable[compIdx] = (combinationIdx >> compIdx) & 0x01 & alfPictureParam[compIdx]->alf_flag;
			if(compEnable[compIdx])
			{
				sumRate += NumCUsInFrame + (considerApsOverhead?(ALFParamBitrateEstimate(alfPictureParam[compIdx])):(0))-1; 
				sumDist += distReduction[compIdx];
			}
		}

		sumCost = (double)sumDist + lambda * ((double)sumRate);

		if(sumCost < minCost)
		{
			minCost = sumCost;
			for(compIdx=0;compIdx<NUM_ALF_COMPONENT;compIdx++)
			{
				bestCompEnable[compIdx] = compEnable[compIdx];
			}
		}
	}

	//assign picture-level on/off flag
	for(compIdx=0;compIdx< NUM_ALF_COMPONENT;compIdx++)
	{
		alfPictureParam[compIdx]->alf_flag = (bestCompEnable[compIdx]?1:0);
	}

	//estimate rd cost
	sumDist = 0;
	sumRate = 0;
	isAllCompsDisabled = TRUE;
	for(compIdx=0; compIdx< NUM_ALF_COMPONENT; compIdx++)
	{
		if(alfPictureParam[compIdx]->alf_flag == 1)
		{
			sumRate += NumCUsInFrame; //LCU: control flags
			sumDist += distReduction[compIdx];
			isAllCompsDisabled = FALSE;
		}
	}

	//------- estimate APS overhead ---------
	apsOverhead =0;
	if(considerApsOverhead && !isAllCompsDisabled)
	{
		//aps_id (APS)
		apsOverhead += uvlcBitrateEstimate(apsId);
		//filter rate
		for(compIdx= 0;compIdx < NUM_ALF_COMPONENT; compIdx++)
		{
			apsOverhead += ALFParamBitrateEstimate(alfPictureParam[compIdx]);
		}
		//extension flag + rbsp_trailing_bits
		apsOverhead = ((apsOverhead +1 +7)/8)*8;     
	}
	sumRate += apsOverhead;

	//------- estimate slice header overhead --------- (skip slice-level on/off flags since making no difference during RDO comparison)
	sliceHeaderOverhead = 0;
	if(!isAllCompsDisabled)
	{
		for(ctu=0; ctu< NumCUsInFrame; ctu++)
		{
			//if( ctu ==  (m_pcPic->getCU(ctu)->getSlice()->getSliceCurStartCUAddr()/m_pcPic->getNumPartInCU()) )
			if(ctu == 0)
			{
				sliceHeaderOverhead += uvlcBitrateEstimate(  apsId );  //aps_id (slice header)
			}
		}
	}
	sumRate += sliceHeaderOverhead;

	return ((double)sumDist + lambda*((double)sumRate));
}
#endif
unsigned int uvlcBitrateEstimate(int val)
{
	unsigned int length = 1;
	val++;
	assert ( val );
	
	while( 1 != val )
	{
		val >>= 1;
		length += 2;
	}
	return ((length >> 1) + ((length+1) >> 1));
}
unsigned int svlcBitrateEsitmate(int val) 
{  
	return uvlcBitrateEstimate (( val <= 0) ? (-val<<1) : ((val<<1)-1));
}

unsigned int filterCoeffBitrateEstimate(int compIdx, int* coeff)
{
	unsigned int  bitrate =0;
	int i;
	for(i=0; i< (int)ALF_MAX_NUM_COEF; i++)
	{
		bitrate += (svlcBitrateEsitmate(coeff[i]));
	}
	return bitrate;
}
unsigned int ALFParamBitrateEstimate(ALFParam* alfParam)
{
#if ALF_ENC_IMPORVE_RATE_ESTIMATE
  unsigned int  bitrate = 0; //alf enabled flag
#else
	unsigned int  bitrate = 1; //alf enabled flag
#endif
	int noFilters,g;
	if(alfParam->alf_flag == 1)
	{
		if(alfParam->componentID == ALF_Y)
		{
			noFilters = alfParam->filters_per_group-1;
			bitrate += uvlcBitrateEstimate(noFilters);
			bitrate += (4*noFilters);
		}
		for(g=0; g< alfParam->filters_per_group; g++)
		{
			bitrate += filterCoeffBitrateEstimate(alfParam->componentID, alfParam->coeffmulti[g]);
		}
	}
	return bitrate;
}

#if ALF_ENC_IMPORVE_RATE_ESTIMATE
unsigned int estimateALFBitrateInPicHeader(ALFParam** alfPicParam)
{
  //CXCTBD please help to check if the implementation is consistent with syntax coding
  int compIdx;
  unsigned int bitrate = 3; // pic_alf_enabled_flag[0,1,2]

  if(alfPicParam[0]->alf_flag ==1 || alfPicParam[1]->alf_flag ==1 || alfPicParam[2]->alf_flag ==1)
  {
      for(compIdx=0; compIdx < NUM_ALF_COMPONENT; compIdx++)
      {
        bitrate += ALFParamBitrateEstimate(alfPicParam[compIdx]);
      }
  }

  return bitrate;
}
#endif
#if !ALF_ENC_IMPORVE_RATE_ESTIMATE
void estimateLcuControl(ALFParam** alfPictureParam, AlfCorrData*** alfCorr, Boolean writeOnOff, long* encDist)
{
	int compIdx,addr;
	long offDist, dist;
	Boolean lcuEnabled;
	int lcuHeight,lcuWidth, img_height,img_width;
	int NumCUsInFrame,numLCUInPicWidth,numLCUInPicHeight;

	lcuHeight         = 1<<g_MaxSizeInbit;
	lcuWidth          = lcuHeight;
	img_height        = img->height;
	img_width         = img->width;
	numLCUInPicWidth  = img_width / lcuWidth ;
	numLCUInPicHeight = img_height / lcuHeight ;
	numLCUInPicWidth  += (img_width % lcuWidth)? 1 : 0;
	numLCUInPicHeight += (img_height % lcuHeight)? 1 : 0;
	NumCUsInFrame = numLCUInPicHeight * numLCUInPicWidth;

	for(compIdx = 0; compIdx < NUM_ALF_COMPONENT; compIdx++)
	{
		if(encDist != NULL)
		{
			encDist[compIdx] = 0;
		}

		if(writeOnOff)
		{
			for(addr = 0; addr < NumCUsInFrame; addr++)
			{
				Enc_ALF->m_AlfLCUEnabled[addr][compIdx] = FALSE;
			}
		}

		if(alfPictureParam[compIdx]->alf_flag == 1)
		{
			reconstructCoefInfo(compIdx, alfPictureParam[compIdx], Enc_ALF->m_filterCoeffSym, Enc_ALF->m_varIndTab);
			for(addr = 0; addr < NumCUsInFrame; addr++)
			{
				offDist    = estimateFilterDistortion(compIdx, alfCorr[compIdx][addr],NULL,1,NULL,FALSE);
				dist       = estimateFilterDistortion(compIdx, alfCorr[compIdx][addr], Enc_ALF->m_filterCoeffSym, alfPictureParam[compIdx]->filters_per_group, Enc_ALF->m_varIndTab, FALSE);
				lcuEnabled = (offDist <= dist)?FALSE:TRUE;
				if(writeOnOff)
				{
					Enc_ALF->m_AlfLCUEnabled[addr][compIdx] = lcuEnabled;
				}
				if(encDist != NULL)
				{
					encDist[compIdx] += (lcuEnabled?(dist - offDist):0);
				}
			}
		}
	}
}
#endif
long estimateFilterDistortion(int compIdx, AlfCorrData* alfCorr, int** coeffSet, int filterSetSize, int* mergeTable, Boolean doPixAccMerge)
{
	int numCoeff = (int)ALF_MAX_NUM_COEF;
	AlfCorrData* alfMerged = Enc_ALF->m_alfCorrMerged[compIdx];
	int f;

	int**     coeff = (coeffSet == NULL)?(Enc_ALF->m_coeffNoFilter):(coeffSet);
	long      iDist = 0;

	mergeFrom(alfMerged,alfCorr, mergeTable, doPixAccMerge);

	
	for(f=0; f< filterSetSize; f++)
	{
		iDist += xFastFiltDistEstimation(alfMerged->ECorr[f], alfMerged->yCorr[f], coeff[f], numCoeff);
	}
	return iDist;
}
long xFastFiltDistEstimation(double** ppdE, double* pdy, int* piCoeff, int iFiltLength)
{
	//static memory
	double pdcoeff[ALF_MAX_NUM_COEF];
	//variable
	int    i,j;
	long  iDist;
	double dDist, dsum;
	unsigned int uiShift;

	for(i=0; i< iFiltLength; i++)
	{
		pdcoeff[i]= (double)piCoeff[i] / (double)(1<< ((int)ALF_NUM_BIT_SHIFT) );
	}

	dDist =0;
	for(i=0; i< iFiltLength; i++)
	{
		dsum= ((double)ppdE[i][i]) * pdcoeff[i];
		for(j=i+1; j< iFiltLength; j++)
		{
			dsum += (double)(2*ppdE[i][j])* pdcoeff[j];
		}

		dDist += ((dsum - 2.0 * pdy[i])* pdcoeff[i] );
	}

	uiShift = Enc_ALF->m_uiBitIncrement<<1;
	if(dDist < 0)
	{
		iDist = -(((long)(-dDist + 0.5)) >> uiShift);
	}
	else //dDist >=0
	{
		iDist= ((long)(dDist+0.5)) >> uiShift;
	}

	return iDist;

}

/*
*************************************************************************
* Function: correlation matrix merge
* Input:  
*                src: input correlation matrix
*         mergeTable: merge table
* Output:
*                dst: output correlation matrix
* Return:
*************************************************************************
*/
void mergeFrom(AlfCorrData* dst, AlfCorrData* src, int* mergeTable, Boolean doPixAccMerge)
{
	int numCoef = ALF_MAX_NUM_COEF;
	double **srcE, **dstE;
	double *srcy, *dsty;
	int maxFilterSetSize,j,i,varInd,filtIdx;
	assert(dst->componentID == src->componentID);
	reset_alfCorr(dst);
	switch(dst->componentID)
	{
	case ALF_Cb:
	case ALF_Cr:
		{
			srcE = src->ECorr  [0];
			dstE = dst->ECorr[0];

			srcy  = src->yCorr[0];
			dsty  = dst->yCorr[0];

			for(j=0; j< numCoef; j++)
			{
				for(i=0; i< numCoef; i++)
				{
					dstE[j][i] += srcE[j][i];
				}

				dsty[j] += srcy[j];
			}
			if(doPixAccMerge)
			{
				dst->pixAcc[0] = src->pixAcc[0];
			}
		}
		break;
	case ALF_Y:
		{
			maxFilterSetSize = (int)NO_VAR_BINS;
			for (varInd=0; varInd< maxFilterSetSize; varInd++)
			{
				filtIdx = (mergeTable == NULL)?(0):(mergeTable[varInd]);
				srcE = src->ECorr  [varInd];
				dstE = dst->ECorr[ filtIdx ];
				srcy  = src->yCorr[varInd];
				dsty  = dst->yCorr[ filtIdx ];
				for(j=0; j< numCoef; j++)
				{
					for(i=0; i< numCoef; i++)
					{
						dstE[j][i] += srcE[j][i];
					}
					dsty[j] += srcy[j];
				}
				if(doPixAccMerge)
				{
					dst->pixAcc[filtIdx] += src->pixAcc[varInd];
				}
			}
		}
		break;
	default:
		{
			printf("not a legal component ID\n");
			assert(0);
			exit(-1);
		}
	}
}

int getTemporalLayerNo(int curPoc, int picDistance)
{
	int layer = 0;
	while(picDistance > 0)
	{
		if(curPoc % picDistance == 0)
		{
			break;
		}
		picDistance = (int)(picDistance/2);
		layer++;
	}
	return layer;
}


/*
*************************************************************************
* Function: ALF On/Off decision for LCU 
*************************************************************************
*/
#if ALF_ENC_IMPORVE_RATE_ESTIMATE
double executePicLCUOnOffDecision(ALFParam** alfPictureParam
                                  , double lambda
                                  , Boolean isRDOEstimate
                                  , AlfCorrData*** alfCorr
                                  ,byte* imgY_org,byte** imgUV_org,byte* imgY_Dec,byte** imgUV_Dec,byte* imgY_Res,byte** imgUV_Res
                                  ,int stride
                                  )
#else
void executePicLCUOnOffDecision(ALFParam** alfPictureParam,byte* imgY_org,byte** imgUV_org,byte* imgY_Dec,byte** imgUV_Dec,byte* imgY_Res,byte** imgUV_Res,int stride, double lambda)
#endif
{

	long  distEnc, distOff;
	double  rateEnc, rateOff, costEnc, costOff,costAlfOn,costAlfOff;
	Boolean isLeftAvail,isRightAvail,isAboveAvail,isBelowAvail;
	Boolean isAboveLeftAvail, isAboveRightAvail;
#if !ALF_ENC_IMPORVE_RATE_ESTIMATE
	long  distOffPic [NUM_ALF_COMPONENT];
#endif
	long  distBestPic[NUM_ALF_COMPONENT];
	double rateBestPic[NUM_ALF_COMPONENT];
	int compIdx,ctu,ctuYPos,ctuXPos,ctuHeight,ctuWidth,formatShift,n,stride_in,ctx_idx;
	byte* pOrg ;
	byte* pDec ;
	byte* pRest;
	double lambda_luma,lambda_chroma;
	Slice*  currSlice= img->currentSlice;
	DataPartition  *dataPart = &(currSlice->partArr[0]);

	EncodingEnvironmentPtr eep_dp = &(dataPart->ee_AEC);

	/////
	int lcuHeight,lcuWidth, img_height,img_width;
	int NumCUsInFrame,numLCUInPicWidth,numLCUInPicHeight;
#if ALF_ENC_IMPORVE_RATE_ESTIMATE
  double bestCost =0;
#endif
#if ALF_ESTIMATE_AEC_RATE
  reset_coding_state(Enc_ALF->m_cs_alf_initial);
  store_coding_state(Enc_ALF->m_cs_alf_cu_ctr);
#endif

	lcuHeight         = 1<<g_MaxSizeInbit;
	lcuWidth          = lcuHeight;
	img_height        = img->height;
	img_width         = img->width;
	numLCUInPicWidth  = img_width / lcuWidth ;
	numLCUInPicHeight = img_height / lcuHeight ;
	numLCUInPicWidth  += (img_width % lcuWidth)? 1 : 0;
	numLCUInPicHeight += (img_height % lcuHeight)? 1 : 0;
	NumCUsInFrame = numLCUInPicHeight * numLCUInPicWidth;

	lambda_luma = lambda; //VKTBD lambda is not correct
	lambda_chroma = LAMBDA_SCALE_CHROMA*lambda_luma;
	for(compIdx=0; compIdx< NUM_ALF_COMPONENT; compIdx++)
	{
#if !ALF_ENC_IMPORVE_RATE_ESTIMATE
		//these three variables are only for non-low-latency encoding mode
		distOffPic [ compIdx] = 0;
#endif
		distBestPic[ compIdx] = 0 ;
		rateBestPic[ compIdx] = 0;
	}

	for(ctu=0; ctu< NumCUsInFrame; ctu++)
	{
		//if the current CTU is the starting CTU at the slice, reset cabac
		/*if( ctu ==  (m_pcPic->getCU(ctu)->getSlice()->getSliceCurStartCUAddr()/m_pcPic->getNumPartInCU()) )
		{
			cabacCoder->resetEntropy();
		}*/

		//derive CTU width and height
		ctuYPos   = (ctu / numLCUInPicWidth )* lcuHeight;
		ctuXPos   = (ctu % numLCUInPicWidth )* lcuWidth;
		ctuHeight = (ctuYPos + lcuHeight > img_height)?( img_height- ctuYPos):lcuHeight;
		ctuWidth  = (ctuXPos + lcuWidth  > img_width )?( img_width - ctuXPos):lcuWidth;

		//derive CTU boundary availabilities
    deriveBoundaryAvail( numLCUInPicWidth,numLCUInPicHeight, ctu,&isLeftAvail,&isRightAvail,&isAboveAvail,&isBelowAvail, &isAboveLeftAvail, &isAboveRightAvail);
		for(compIdx=0; compIdx < NUM_ALF_COMPONENT; compIdx++)
		{
			//if slice-level enabled flag is 0, set CTB-level enabled flag 0
			if(alfPictureParam[compIdx]->alf_flag == 0)
			{				
				Enc_ALF->m_AlfLCUEnabled[ctu][compIdx] = FALSE;
				continue;
			}

#if ALF_ENC_IMPORVE_RATE_ESTIMATE
      if(!isRDOEstimate)
      {
#endif
			formatShift = (compIdx == ALF_Y)?0:1;
			pOrg = (compIdx == ALF_Y)?imgY_org:imgUV_org[compIdx-ALF_Cb];
			pDec = (compIdx == ALF_Y)?imgY_Dec:imgUV_Dec[compIdx-ALF_Cb];
			pRest= (compIdx == ALF_Y)?imgY_Res:imgUV_Res[compIdx-ALF_Cb];
			stride_in = (compIdx == ALF_Y)?(stride):(stride>>1);
#if ALF_ENC_IMPORVE_RATE_ESTIMATE
      }
#endif


			//ALF on
#if ALF_ENC_IMPORVE_RATE_ESTIMATE
      if(isRDOEstimate)
      {
        reconstructCoefInfo(compIdx, alfPictureParam[compIdx], Enc_ALF->m_filterCoeffSym, Enc_ALF->m_varIndTab);
        //distEnc is the estimated distortion reduction compared with filter-off case
        distEnc      = estimateFilterDistortion(compIdx, alfCorr[compIdx][ctu], Enc_ALF->m_filterCoeffSym, alfPictureParam[compIdx]->filters_per_group, Enc_ALF->m_varIndTab, FALSE)
                     - estimateFilterDistortion(compIdx, alfCorr[compIdx][ctu],NULL,1,NULL,FALSE);

      }
      else
      {
#endif
        filterOneCTB(pRest, pDec, stride_in, compIdx, alfPictureParam[compIdx]
        , ctuYPos, ctuHeight, ctuXPos, ctuWidth, isAboveAvail, isBelowAvail, isLeftAvail, isRightAvail, isAboveLeftAvail, isAboveRightAvail);
			distEnc = calcAlfLCUDist(Enc_ALF->m_alfLowLatencyEncoding, compIdx
				, ctu, ctuYPos, ctuXPos, ctuHeight, ctuWidth, isAboveAvail, isBelowAvail, isLeftAvail, isRightAvail
				, pOrg, pRest, stride_in, formatShift
				);
#if ALF_ENC_IMPORVE_RATE_ESTIMATE
			distEnc -= calcAlfLCUDist(Enc_ALF->m_alfLowLatencyEncoding, compIdx
				 , ctu, ctuYPos, ctuXPos, ctuHeight, ctuWidth, isAboveAvail, isBelowAvail, isLeftAvail, isRightAvail
				 , pOrg, pDec, stride_in, formatShift);

      }
#endif

#if ALF_ESTIMATE_AEC_RATE
			reset_coding_state(Enc_ALF->m_cs_alf_cu_ctr);
#else
			store_coding_state(Enc_ALF->m_cs_alf_cu_ctr);
#endif
#if !M3544_CONTEXT_OPT
			ctx_idx = getLCUCtrCtx_Idx(ctu,numLCUInPicWidth,numLCUInPicHeight,NumCUsInFrame,compIdx,Enc_ALF->m_AlfLCUEnabled);
#else
			ctx_idx = 0;
#endif 
			rateEnc = writeAlfLCUCtrl(1,dataPart,compIdx,ctx_idx);
#if !ALF_ESTIMATE_AEC_RATE
			reset_coding_state(Enc_ALF->m_cs_alf_cu_ctr);
#endif			
			costEnc = (double)distEnc + (compIdx==0?lambda_luma:lambda_chroma)*rateEnc;

			//ALF off
#if ALF_ENC_IMPORVE_RATE_ESTIMATE
			distOff =0;
#else
			distOff = calcAlfLCUDist(Enc_ALF->m_alfLowLatencyEncoding, compIdx
				, ctu, ctuYPos, ctuXPos, ctuHeight, ctuWidth, isAboveAvail, isBelowAvail, isLeftAvail, isRightAvail
				, pOrg, pDec, stride_in, formatShift);
#endif
			//rateOff = 1;
#if ALF_ESTIMATE_AEC_RATE
			reset_coding_state(Enc_ALF->m_cs_alf_cu_ctr);
#else
			store_coding_state(Enc_ALF->m_cs_alf_cu_ctr);
#endif
#if !M3544_CONTEXT_OPT
			ctx_idx = getLCUCtrCtx_Idx(ctu,numLCUInPicWidth,numLCUInPicHeight,NumCUsInFrame,compIdx,Enc_ALF->m_AlfLCUEnabled);
#else 
			ctx_idx = 0;
#endif 			
			rateOff = writeAlfLCUCtrl(0,dataPart,compIdx,ctx_idx);
#if !ALF_ESTIMATE_AEC_RATE
			reset_coding_state(Enc_ALF->m_cs_alf_cu_ctr);
#endif
			costOff = (double)distOff + (compIdx==0?lambda_luma:lambda_chroma)*rateOff;


			//set CTB-level on/off flag
			Enc_ALF->m_AlfLCUEnabled[ctu][compIdx] = (costEnc < costOff)?TRUE:FALSE;
#if ALF_ENC_IMPORVE_RATE_ESTIMATE
      if(!isRDOEstimate && !Enc_ALF->m_AlfLCUEnabled[ctu][compIdx])
#else
			if(!Enc_ALF->m_AlfLCUEnabled[ctu][compIdx])
#endif
			{
				copyOneAlfBlk(pRest, pDec, stride_in, ctuYPos>>formatShift, ctuXPos>>formatShift, ctuHeight>>formatShift, ctuWidth>>formatShift, isAboveAvail, isBelowAvail, (compIdx == 0) ? 0 : 1);
			}

			//update CABAC status
			//cabacCoder->updateAlfCtrlFlagState(m_pcPic->getCU(ctu)->getAlfLCUEnabled(compIdx)?1:0);
#if ALF_ESTIMATE_AEC_RATE
			reset_coding_state(Enc_ALF->m_cs_alf_cu_ctr);
#if !M3544_CONTEXT_OPT
			ctx_idx = getLCUCtrCtx_Idx(ctu, numLCUInPicWidth, numLCUInPicHeight, NumCUsInFrame, compIdx, Enc_ALF->m_AlfLCUEnabled);
#else 
			ctx_idx = 0;
#endif 
			rateOff = writeAlfLCUCtrl( (Enc_ALF->m_AlfLCUEnabled[ctu][compIdx]? 1 : 0), dataPart, compIdx, ctx_idx);
			store_coding_state(Enc_ALF->m_cs_alf_cu_ctr);
#endif
#if !ALF_ENC_IMPORVE_RATE_ESTIMATE
			if(!Enc_ALF->m_alfLowLatencyEncoding)
			{
#endif
				rateBestPic[compIdx] += ( Enc_ALF->m_AlfLCUEnabled[ctu][compIdx] ? rateEnc : rateOff);
				distBestPic[compIdx] += ( Enc_ALF->m_AlfLCUEnabled[ctu][compIdx] ? distEnc : distOff);
#if !ALF_ENC_IMPORVE_RATE_ESTIMATE
				distOffPic [compIdx] += ( distOff );
			}
#endif
		} //CTB
	} //CTU


#if ALF_ENC_IMPORVE_RATE_ESTIMATE
	if( isRDOEstimate || !Enc_ALF->m_alfLowLatencyEncoding)
#else
	if(!Enc_ALF->m_alfLowLatencyEncoding)
#endif
	{
		for(compIdx=0; compIdx < NUM_ALF_COMPONENT; compIdx++)
		{
			if(alfPictureParam[compIdx]->alf_flag == 1)
			{
#if ALF_ENC_IMPORVE_RATE_ESTIMATE
				costAlfOn = (double)distBestPic[compIdx] + (compIdx==0?lambda_luma:lambda_chroma)*(rateBestPic[compIdx] + (double)(  ALFParamBitrateEstimate(alfPictureParam[compIdx]) ));
				costAlfOff = 0;
#else
				costAlfOn = (double)distBestPic[compIdx] + (compIdx==0?lambda_luma:lambda_chroma)*(rateBestPic[compIdx] + (double)(  ALFParamBitrateEstimate(alfPictureParam[compIdx]) ));
				costAlfOff = (double)distOffPic [compIdx] + (compIdx==0?lambda_luma:lambda_chroma)*1;
#endif
				if( costAlfOn >= costAlfOff)
				{
					alfPictureParam[compIdx]->alf_flag = 0;
					for(n=0; n< NumCUsInFrame; n++)
					{
						Enc_ALF->m_AlfLCUEnabled[n][compIdx] = FALSE;
					}

#if ALF_ENC_IMPORVE_RATE_ESTIMATE
					if(!isRDOEstimate)
					{
#endif
						formatShift = (compIdx == ALF_Y)?0:1;
						pOrg = (compIdx == ALF_Y)?imgY_org:imgUV_org[compIdx-ALF_Cb];
						pDec = (compIdx == ALF_Y)?imgY_Dec:imgUV_Dec[compIdx-ALF_Cb];
						pRest= (compIdx == ALF_Y)?imgY_Res:imgUV_Res[compIdx-ALF_Cb];
						stride_in = (compIdx == ALF_Y)?(stride):(stride>>1);
						copyToImage(pRest,pDec,stride_in,img_height,img_width,formatShift);
#if ALF_ENC_IMPORVE_RATE_ESTIMATE
					}
#endif
				}
			}
		}
	}

#if ALF_ENC_IMPORVE_RATE_ESTIMATE
  bestCost =0;
  for(compIdx =0; compIdx< NUM_ALF_COMPONENT; compIdx++)
  {
    if(alfPictureParam[compIdx]->alf_flag == 1)
    {
      bestCost += (double)distBestPic[compIdx] + (compIdx==0?lambda_luma:lambda_chroma)*(rateBestPic[compIdx]);
    }
  }
  //return the block-level RD cost
  return bestCost;
#endif
}

/*
*************************************************************************
* Function: ALF filter on CTB 
*************************************************************************
*/
void filterOneCTB(byte* pRest, byte* pDec, int stride, int compIdx, ALFParam* alfParam, int ctuYPos, int ctuHeight, int ctuXPos, int ctuWidth
  , Boolean isAboveAvail, Boolean isBelowAvail, Boolean isLeftAvail, Boolean isRightAvail, Boolean isAboveLeftAvail, Boolean isAboveRightAvail)
{
	int skipSize = (ALF_FOOTPRINT_SIZE>>1); //half size of 7x7cross+ 3x3square
	int  formatShift  = (compIdx == ALF_Y)?0:1;
	int ypos,height,xpos,width;

	//reconstruct coefficients to m_filterCoeffSym and m_varIndTab
	reconstructCoefInfo(compIdx, alfParam, Enc_ALF->m_filterCoeffSym, Enc_ALF->m_varIndTab); //reconstruct ALF coefficients & related parameters 

	//derive CTB start positions, width, and height. If the boundary is not available, skip boundary samples.
	ypos         = (ctuYPos >> formatShift);
	height       = (ctuHeight >> formatShift);

	xpos         = (ctuXPos >> formatShift);
	width        = (ctuWidth >> formatShift );
#if EXTEND_BD
  filterOneCompRegion(pRest, pDec, stride, (compIdx != ALF_Y), ypos, height, xpos, width, Enc_ALF->m_filterCoeffSym, Enc_ALF->m_varIndTab, Enc_ALF->m_varImg,
    input->sample_bit_depth, isLeftAvail, isRightAvail, isAboveAvail, isBelowAvail, isAboveLeftAvail, isAboveRightAvail);
#else
	filterOneCompRegion(pRest, pDec, stride, (compIdx!=ALF_Y), ypos, ypos+height, xpos, xpos+width, Enc_ALF->m_filterCoeffSym, Enc_ALF->m_varIndTab,Enc_ALF->m_varImg, 
		isLeftAvail, isRightAvail, isAboveAvail, isBelowAvail);
#endif
}

long calcAlfLCUDist(Boolean isSkipLCUBoundary, int compIdx, int lcuAddr, int ctuYPos, int ctuXPos, int ctuHeight, int ctuWidth, Boolean isAboveAvail, Boolean isBelowAvail, Boolean isLeftAvail, Boolean isRightAvail
					, byte* picSrc, byte* picCmp, int stride, int formatShift)
{
	long dist = 0;  
	int  posOffset, ypos, xpos, height, width;
	byte* pelCmp;
	byte* pelSrc;

	Boolean notSkipLinesRightVB;
	Boolean notSkipLinesBelowVB = TRUE;
	int lcuHeight,lcuWidth, img_height,img_width;
	int NumCUsInFrame,numLCUInPicWidth,numLCUInPicHeight;

	lcuHeight         = 1<<g_MaxSizeInbit;
	lcuWidth          = lcuHeight;
	img_height        = img->height;
	img_width         = img->width;
	numLCUInPicWidth  = img_width / lcuWidth ;
	numLCUInPicHeight = img_height / lcuHeight ;
	numLCUInPicWidth  += (img_width % lcuWidth)? 1 : 0;
	numLCUInPicHeight += (img_height % lcuHeight)? 1 : 0;
	NumCUsInFrame = numLCUInPicHeight * numLCUInPicWidth;

	if(isSkipLCUBoundary)
	{
		if(lcuAddr + numLCUInPicWidth < NumCUsInFrame)
		{
			notSkipLinesBelowVB = FALSE;
		}
	}

	notSkipLinesRightVB = TRUE;
	if(isSkipLCUBoundary)
	{
		if((lcuAddr + 1) % numLCUInPicWidth != 0 )
		{
			notSkipLinesRightVB = FALSE;
		}
	}

	switch(compIdx)
	{
	case ALF_Cb:
	case ALF_Cr:
		{
			ypos    = (ctuYPos   >> formatShift);
			xpos    = (ctuXPos   >> formatShift);
			height  = (ctuHeight >> formatShift);
			width   = (ctuWidth  >> formatShift);

			if(!notSkipLinesBelowVB )
			{
				height = height - (int)(DF_CHANGED_SIZE/2) - (int)(ALF_FOOTPRINT_SIZE/2);
			}

			if( !notSkipLinesRightVB)
			{
				width = width - (int)(DF_CHANGED_SIZE/2) - (int)(ALF_FOOTPRINT_SIZE/2);
			}
			if (isAboveAvail)
				posOffset = ((ypos-4) * stride) + xpos;
			else
				posOffset = (ypos * stride) + xpos;
			pelCmp    = picCmp + posOffset;    
			pelSrc    = picSrc + posOffset;    

			dist  += xCalcSSD( pelSrc, pelCmp,  width, height, stride );
		}
		break;
	case ALF_Y:
		{
			ypos    = (ctuYPos   >> formatShift);
			xpos    = (ctuXPos   >> formatShift);
			height  = (ctuHeight >> formatShift);
			width   = (ctuWidth  >> formatShift);

			if(!notSkipLinesBelowVB)
			{
				height = height - (int)(DF_CHANGED_SIZE) - (int)(ALF_FOOTPRINT_SIZE/2);
			}

			if( !notSkipLinesRightVB)
			{
				width = width - (int)(DF_CHANGED_SIZE) - (int)(ALF_FOOTPRINT_SIZE/2);
			}

			posOffset = (ypos * stride) + xpos;
			pelCmp    = picCmp + posOffset;    
			pelSrc    = picSrc + posOffset;    

			dist  += xCalcSSD( pelSrc, pelCmp,  width, height, stride );
		}
		break;
	default:
		{
			printf("not a legal component ID for ALF \n");
			assert(0);
			exit(-1);
		}
	}

	return dist;
}

void copyOneAlfBlk(byte* picDst, byte* picSrc, int stride, int ypos, int xpos, int height, int width, int isAboveAvail, int isBelowAvail, int isChroma)
{
	int posOffset  = ( ypos * stride)+ xpos;
	byte* pelDst;
	byte* pelSrc;
	int j;
	int startPos   = isAboveAvail ? (ypos-4) : ypos;
	int endPos	   = isBelowAvail ? (ypos+height-4) : ypos+height;
	posOffset = (startPos*stride) + xpos;
	pelDst   = picDst  + posOffset;
	pelSrc   = picSrc  + posOffset;

	for (j=startPos; j<endPos; j++)
	{
		memcpy(pelDst, pelSrc, sizeof(byte)*width);
		pelDst += stride;
		pelSrc += stride;
	}
}
void storeAlfTemporalLayerInfo(ALFParam** alfPictureParam, int temporalLayer,double lambda)
{
	int compIdx,i;
	//store ALF parameters
	AlfCorrData* alfPicCorr[NUM_ALF_COMPONENT];
	AlfCorrData*** alfPreCorr;
	AlfCorrData* alfPreCorrLCU;
	int lcuHeight,lcuWidth, img_height,img_width;
	int NumCUsInFrame,numLCUInPicWidth,numLCUInPicHeight;

	lcuHeight         = 1<<g_MaxSizeInbit;
	lcuWidth          = lcuHeight;
	img_height        = img->height;
	img_width         = img->width;
	numLCUInPicWidth  = img_width / lcuWidth ;
	numLCUInPicHeight = img_height / lcuHeight ;
	numLCUInPicWidth  += (img_width % lcuWidth)? 1 : 0;
	numLCUInPicHeight += (img_height % lcuHeight)? 1 : 0;
	NumCUsInFrame = numLCUInPicHeight * numLCUInPicWidth;

	assert(Enc_ALF->m_alfLowLatencyEncoding);

	
	for(compIdx=0; compIdx< NUM_ALF_COMPONENT; compIdx++)
	{
		 AllocateAlfCorrData(&(alfPicCorr[compIdx]),compIdx);
	}

	accumulateLCUCorrelations(alfPicCorr, Enc_ALF->m_alfNonSkippedCorr, FALSE);
	decideAlfPictureParam(alfPictureParam, alfPicCorr,lambda);

	for(compIdx=0; compIdx< NUM_ALF_COMPONENT; compIdx++)
	{
		freeAlfCorrData(alfPicCorr[compIdx]);
	}

	//store correlations
	alfPreCorr= Enc_ALF->m_alfPrevCorr[temporalLayer];

	for(compIdx=0; compIdx < NUM_ALF_COMPONENT; compIdx++)
	{
		for(i=0; i< NumCUsInFrame; i++)
		{
			alfPreCorrLCU = alfPreCorr[compIdx][i];			
			reset_alfCorr(alfPreCorrLCU);
			//*alfPreCorrLCU += *(alfCorr[compIdx][i]);
			ADD_AlfCorrData(Enc_ALF->m_alfCorr[compIdx][i],alfPreCorrLCU, alfPreCorrLCU);
		}
	}
}
void ADD_AlfCorrData(AlfCorrData* A,AlfCorrData* B, AlfCorrData* C)
{
	int numCoef = ALF_MAX_NUM_COEF;
	int maxNumGroups = NO_VAR_BINS;
	int numGroups;
	int g,j,i;
	if(A->componentID >=0)
	{


		numGroups = (A->componentID == ALF_Y)?(maxNumGroups):(1);
		for(g=0; g< numGroups; g++)
		{
			C->pixAcc[g] = A->pixAcc[g] + B->pixAcc[g];

			for(j=0; j< numCoef; j++)
			{
				C->yCorr[g][j] = A->yCorr[g][j] + B->yCorr[g][j];
				for(i=0; i< numCoef; i++)
				{
					C->ECorr[g][j][i] = A->ECorr[g][j][i] + B->ECorr[g][j][i];
				}
			}
		}
	}

}
void accumulateLCUCorrelations(AlfCorrData** alfCorrAcc, AlfCorrData*** alfCorSrcLCU, Boolean useAllLCUs)
{
	int compIdx,numStatLCU,addr;
	AlfCorrData* alfCorrAccComp;
	int lcuHeight,lcuWidth, img_height,img_width;
	int NumCUsInFrame,numLCUInPicWidth,numLCUInPicHeight;

	lcuHeight         = 1<<g_MaxSizeInbit;
	lcuWidth          = lcuHeight;
	img_height        = img->height;
	img_width         = img->width;
	numLCUInPicWidth  = img_width / lcuWidth ;
	numLCUInPicHeight = img_height / lcuHeight ;
	numLCUInPicWidth  += (img_width % lcuWidth)? 1 : 0;
	numLCUInPicHeight += (img_height % lcuHeight)? 1 : 0;
	NumCUsInFrame = numLCUInPicHeight * numLCUInPicWidth;

	for(compIdx=0; compIdx < NUM_ALF_COMPONENT; compIdx++)
	{
		alfCorrAccComp= alfCorrAcc[compIdx];
		
		reset_alfCorr(alfCorrAccComp);

		if(!useAllLCUs)
		{
			numStatLCU =0;
			for(addr=0; addr < NumCUsInFrame; addr++)
			{
				if(Enc_ALF->m_AlfLCUEnabled[addr][compIdx])
				{
					numStatLCU++;
					break;
				}
			}
			if(numStatLCU ==0)
			{
				useAllLCUs = TRUE;
			}
		}

		for(addr=0; addr< (int)NumCUsInFrame; addr++)
		{
			if(useAllLCUs || Enc_ALF->m_AlfLCUEnabled[addr][compIdx] )
			{
				//*alfCorrAccComp += *(alfCorSrcLCU[compIdx][addr]);
				ADD_AlfCorrData(alfCorSrcLCU[compIdx][addr],alfCorrAccComp, alfCorrAccComp);
			}
		}
	}
}

#if ALF_ENC_IMPORVE_RATE_ESTIMATE
void decideAlfPictureParam(ALFParam** alfPictureParam, AlfCorrData** alfCorr,double lambdaLuma)
#else
void decideAlfPictureParam(ALFParam** alfPictureParam, AlfCorrData** alfCorr,double m_dLambdaLuma)
#endif
{
	double lambdaWeighting = (Enc_ALF->m_alfLowLatencyEncoding)?(1.5):(1.0);
	int compIdx;
	double lambda;
	ALFParam *alfParam;
	AlfCorrData* picCorr;
	for(compIdx =0; compIdx < NUM_ALF_COMPONENT; compIdx++)
	{
#if ALF_ENC_IMPORVE_RATE_ESTIMATE
    //VKTBD chroma need different lambdas? lambdaWeighting needed?
    lambda   = lambdaLuma*lambdaWeighting;
#else
		lambda   = m_dLambdaLuma*lambdaWeighting;
#endif
		alfParam   = alfPictureParam[compIdx];
		picCorr = alfCorr[compIdx];
		alfParam->alf_flag = 1;
		deriveFilterInfo(compIdx, picCorr, alfParam, NO_VAR_BINS, lambda);
	}
}

void deriveFilterInfo(int compIdx, AlfCorrData* alfCorr, ALFParam* alfFiltParam, int maxNumFilters, double lambda)
{
	int numCoeff = ALF_MAX_NUM_COEF;
	double coef[ALF_MAX_NUM_COEF];
	int lambdaForMerge,numFilters;

	switch(compIdx)
	{
	case ALF_Y:
		{
			lambdaForMerge = ((int) lambda) * (1<<(2*Enc_ALF->m_uiBitIncrement));

			memset(Enc_ALF->m_varIndTab, 0, sizeof(int)*NO_VAR_BINS);
			xfindBestFilterVarPred(alfCorr->yCorr, alfCorr->ECorr, alfCorr->pixAcc, Enc_ALF->m_filterCoeffSym, &numFilters, Enc_ALF->m_varIndTab, lambdaForMerge, maxNumFilters);
			xcodeFiltCoeff(Enc_ALF->m_filterCoeffSym,  Enc_ALF->m_varIndTab, numFilters, alfFiltParam);
		}
		break;
	case ALF_Cb:
	case ALF_Cr:
		{
			alfFiltParam->filters_per_group = 1;

			gnsSolveByChol(alfCorr->ECorr[0], alfCorr->yCorr[0], coef, numCoeff);
			xQuantFilterCoef(coef, Enc_ALF->m_filterCoeffSym[0]);
			memcpy(alfFiltParam->coeffmulti[0], Enc_ALF->m_filterCoeffSym[0], sizeof(int)*numCoeff);
			predictALFCoeff(alfFiltParam->coeffmulti, numCoeff, alfFiltParam->filters_per_group);
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
void xcodeFiltCoeff(int **filterCoeff, int* varIndTab, int numFilters, ALFParam* alfParam)
{
	int filterPattern[NO_VAR_BINS], startSecondFilter=0,i,g;
	memset(filterPattern, 0, NO_VAR_BINS * sizeof(int)); 

	alfParam->num_coeff = (int)ALF_MAX_NUM_COEF;
	alfParam->filters_per_group = numFilters;

	//merge table assignment
	if(alfParam->filters_per_group > 1)
	{
		for(i = 1; i < NO_VAR_BINS; ++i)
		{
			if(varIndTab[i] != varIndTab[i-1])
			{
				filterPattern[i] = 1;
				startSecondFilter = i;
			}
		}
	}
	memcpy (alfParam->filterPattern, filterPattern, NO_VAR_BINS * sizeof(int));

	//coefficient prediction
	for(g=0; g< alfParam->filters_per_group; g++)
	{
		for(i=0; i< alfParam->num_coeff; i++)
		{
			alfParam->coeffmulti[g][i] = filterCoeff[g][i];
		}
	}

	predictALFCoeff(alfParam->coeffmulti, alfParam->num_coeff, alfParam->filters_per_group);
}

void xQuantFilterCoef(double* h, int* qh)
{
	int i, N;
	int max_value, min_value;
	double dbl_total_gain;
	int total_gain, q_total_gain;
	int upper, lower;
	double *dh;
	int  *nc;
	const int    *pFiltMag;
	N = (int)ALF_MAX_NUM_COEF;
	pFiltMag = weightsShape1Sym;

	dh = (double *)malloc(N * sizeof(double));
	nc = (int *)malloc(N * sizeof(int));
	max_value =   (1<<(1+ALF_NUM_BIT_SHIFT))-1;
	min_value = 0-(1<<(1+ALF_NUM_BIT_SHIFT));
	dbl_total_gain=0.0;
	q_total_gain=0;

	for(i=0; i<N; i++)
	{
		if(h[i]>=0.0)
		{
			qh[i] =  (int)( h[i]*(1<<ALF_NUM_BIT_SHIFT)+0.5);
		}
		else
		{
			qh[i] = -(int)(-h[i]*(1<<ALF_NUM_BIT_SHIFT)+0.5);
		}
		dh[i] = (double)qh[i]/(double)(1<<ALF_NUM_BIT_SHIFT) - h[i];
		dh[i]*=pFiltMag[i];
		dbl_total_gain += h[i]*pFiltMag[i];
		q_total_gain   += qh[i]*pFiltMag[i];
		nc[i] = i;
	}

	// modification of quantized filter coefficients
	total_gain = (int)(dbl_total_gain*(1<<ALF_NUM_BIT_SHIFT)+0.5);
	if( q_total_gain != total_gain )
	{
		xFilterCoefQuickSort(dh, nc, 0, N-1);
		if( q_total_gain > total_gain )
		{
			upper = N-1;
			while( q_total_gain > total_gain+1 )
			{
				i = nc[upper%N];
				qh[i]--;
				q_total_gain -= pFiltMag[i];
				upper--;
			}

			if( q_total_gain == total_gain+1 )
			{
				if(dh[N-1]>0)
				{
					qh[N-1]--;
				}
				else
				{
					i=nc[upper%N];
					qh[i]--;
					qh[N-1]++;
				}
			}
		}
		else if( q_total_gain < total_gain )
		{
			lower = 0;
			while( q_total_gain < total_gain-1 )
			{
				i=nc[lower%N];
				qh[i]++;
				q_total_gain += pFiltMag[i];
				lower++;
			}

			if( q_total_gain == total_gain-1 )
			{
				if(dh[N-1]<0)
				{
					qh[N-1]++;
				}
				else
				{
					i=nc[lower%N];
					qh[i]++;
					qh[N-1]--;
				}
			}
		}
	}

	// set of filter coefficients
	for(i=0; i<N; i++)
	{
		qh[i] = max(min_value,min(max_value, qh[i]));
	}

	checkFilterCoeffValue(qh, N, TRUE);

	free(dh);
	dh = NULL;

	free(nc);
	nc = NULL;
}
void xFilterCoefQuickSort( double *coef_data, int *coef_num, int upper, int lower )
{
	double mid, tmp_data;
	int i, j, tmp_num;

	i = upper;
	j = lower;
	mid = coef_data[(lower+upper)>>1];

	do
	{
		while( coef_data[i] < mid ) i++;
		while( mid < coef_data[j] ) j--;
		if( i <= j )
		{
			tmp_data = coef_data[i];
			tmp_num  = coef_num[i];
			coef_data[i] = coef_data[j];
			coef_num[i]  = coef_num[j];
			coef_data[j] = tmp_data;
			coef_num[j]  = tmp_num;
			i++;
			j--;
		}
	} while( i <= j );

	if ( upper < j ) 
	{
		xFilterCoefQuickSort(coef_data, coef_num, upper, j);
	}
	if ( i < lower ) 
	{
		xFilterCoefQuickSort(coef_data, coef_num, i, lower);
	}
}
void xfindBestFilterVarPred(double **ySym, double ***ESym, double *pixAcc, int **filterCoeffSym, int *filters_per_fr_best, int varIndTab[], double lambda_val, int numMaxFilters)
{
	static Boolean isFirst = TRUE;
	static int* filterCoeffSymQuant[NO_VAR_BINS];
	int filter_shape = 0;
	int filters_per_fr, firstFilt, interval[NO_VAR_BINS][2], intervalBest[NO_VAR_BINS][2];
	int i,g;
	double  lagrangian, lagrangianMin;
	int sqrFiltLength;
	int *weights;
	double errorForce0CoeffTab[NO_VAR_BINS][2];
#if !FIX_LEAK
	if(isFirst)
#endif
	{
		for(g=0; g< NO_VAR_BINS; g++)
		{
			filterCoeffSymQuant[g] = (int*)malloc(ALF_MAX_NUM_COEF * sizeof(int));
		}
		isFirst = FALSE;
	}

	sqrFiltLength= (int)ALF_MAX_NUM_COEF;
	weights = weightsShape1Sym;
	// zero all variables 
	memset(varIndTab,0,sizeof(int)*NO_VAR_BINS);

	for(i = 0; i < NO_VAR_BINS; i++)
	{
		memset(filterCoeffSym[i],0,sizeof(int)*ALF_MAX_NUM_COEF);
		memset(filterCoeffSymQuant[i],0,sizeof(int)*ALF_MAX_NUM_COEF);
	}

	firstFilt=1;  lagrangianMin=0;
	filters_per_fr=NO_VAR_BINS;

	while(filters_per_fr>=1)
	{
		mergeFiltersGreedy(ySym, ESym, pixAcc, interval, sqrFiltLength, filters_per_fr);
		findFilterCoeff(ESym, ySym, pixAcc, filterCoeffSym, filterCoeffSymQuant, interval,
			varIndTab, sqrFiltLength, filters_per_fr, weights, errorForce0CoeffTab);

		lagrangian=xfindBestCoeffCodMethod(filterCoeffSymQuant, filter_shape, sqrFiltLength, filters_per_fr, errorForce0CoeffTab, lambda_val);
		if (lagrangian<lagrangianMin || firstFilt==1 || filters_per_fr == numMaxFilters)
		{
			firstFilt=0;
			lagrangianMin=lagrangian;

			(*filters_per_fr_best)=filters_per_fr;
			memcpy(intervalBest, interval, NO_VAR_BINS*2*sizeof(int));
		}
		filters_per_fr--;
	}

	findFilterCoeff(ESym, ySym, pixAcc, filterCoeffSym, filterCoeffSymQuant, intervalBest,
		varIndTab, sqrFiltLength, (*filters_per_fr_best), weights, errorForce0CoeffTab);

	if( *filters_per_fr_best == 1)
	{
		memset(varIndTab, 0, sizeof(int)*NO_VAR_BINS);
	}
#if FIX_LEAK
    for(g=0; g< NO_VAR_BINS; g++)
    {
        free(filterCoeffSymQuant[g]);// = (int*)malloc(ALF_MAX_NUM_COEF * sizeof(int));
    }
#endif
}

double mergeFiltersGreedy(double **yGlobalSeq, double ***EGlobalSeq, double *pixAccGlobalSeq, int intervalBest[NO_VAR_BINS][2], int sqrFiltLength, int noIntervals)
{
	int first, ind, ind1, ind2, i, j, bestToMerge ;
	double error, error1, error2, errorMin;
	static double pixAcc_temp, error_tab[NO_VAR_BINS],error_comb_tab[NO_VAR_BINS];
	static int indexList[NO_VAR_BINS], available[NO_VAR_BINS], noRemaining;

	if (noIntervals == NO_VAR_BINS)
	{
		noRemaining = NO_VAR_BINS;
		for (ind=0; ind<NO_VAR_BINS; ind++)
		{
			indexList[ind] = ind; 
			available[ind] = 1;
			Enc_ALF->m_pixAcc_merged[ind] = pixAccGlobalSeq[ind];
			memcpy(Enc_ALF->m_y_merged[ind], yGlobalSeq[ind], sizeof(double)*sqrFiltLength);
			for (i=0; i < sqrFiltLength; i++)
			{
				memcpy(Enc_ALF->m_E_merged[ind][i], EGlobalSeq[ind][i], sizeof(double)*sqrFiltLength);
			}
		}
	}

	// Try merging different matrices
	if (noIntervals == NO_VAR_BINS)
	{
		for (ind = 0; ind < NO_VAR_BINS; ind++)
		{
			error_tab[ind] = calculateErrorAbs(Enc_ALF->m_E_merged[ind], Enc_ALF->m_y_merged[ind], Enc_ALF->m_pixAcc_merged[ind], sqrFiltLength);
		}
		for (ind = 0; ind < NO_VAR_BINS - 1; ind++)
		{
			ind1 = indexList[ind];
			ind2 = indexList[ind+1];

			error1 = error_tab[ind1];
			error2 = error_tab[ind2];

			pixAcc_temp = Enc_ALF->m_pixAcc_merged[ind1] + Enc_ALF->m_pixAcc_merged[ind2];
			for (i = 0; i < sqrFiltLength; i++)
			{
				Enc_ALF->m_y_temp[i] = Enc_ALF->m_y_merged[ind1][i] + Enc_ALF->m_y_merged[ind2][i];
				for (j = 0; j < sqrFiltLength; j++)
				{
					Enc_ALF->m_E_temp[i][j] = Enc_ALF->m_E_merged[ind1][i][j] + Enc_ALF->m_E_merged[ind2][i][j];
				}
			}
			error_comb_tab[ind1] = calculateErrorAbs(Enc_ALF->m_E_temp, Enc_ALF->m_y_temp, pixAcc_temp, sqrFiltLength) - error1 - error2;
		}
	}

	while (noRemaining > noIntervals)
	{
		errorMin = 0; 
		first = 1;
		bestToMerge = 0;
		for (ind = 0; ind < noRemaining - 1; ind++)
		{
			error = error_comb_tab[indexList[ind]];
			if ((error < errorMin || first == 1))
			{
				errorMin = error;
				bestToMerge = ind;
				first = 0;
			}
		}

		ind1 = indexList[bestToMerge];
		ind2 = indexList[bestToMerge+1];
		Enc_ALF->m_pixAcc_merged[ind1] += Enc_ALF->m_pixAcc_merged[ind2];
		for (i = 0; i < sqrFiltLength; i++)
		{
			Enc_ALF->m_y_merged[ind1][i] += Enc_ALF->m_y_merged[ind2][i];
			for (j = 0; j < sqrFiltLength; j++)
			{
				Enc_ALF->m_E_merged[ind1][i][j] += Enc_ALF->m_E_merged[ind2][i][j];
			}
		}
		available[ind2] = 0;

		//update error tables
		error_tab[ind1] = error_comb_tab[ind1] + error_tab[ind1] + error_tab[ind2];
		if (indexList[bestToMerge] > 0)
		{
			ind1 = indexList[bestToMerge-1];
			ind2 = indexList[bestToMerge];
			error1 = error_tab[ind1];
			error2 = error_tab[ind2];
			pixAcc_temp = Enc_ALF->m_pixAcc_merged[ind1] + Enc_ALF->m_pixAcc_merged[ind2];
			for (i = 0; i < sqrFiltLength; i++)
			{
				Enc_ALF->m_y_temp[i] = Enc_ALF->m_y_merged[ind1][i] + Enc_ALF->m_y_merged[ind2][i];
				for (j = 0; j < sqrFiltLength; j++)
				{
					Enc_ALF->m_E_temp[i][j] = Enc_ALF->m_E_merged[ind1][i][j] + Enc_ALF->m_E_merged[ind2][i][j];
				}
			}
			error_comb_tab[ind1] = calculateErrorAbs(Enc_ALF->m_E_temp, Enc_ALF->m_y_temp, pixAcc_temp, sqrFiltLength) - error1 - error2;
		}

		if (indexList[bestToMerge+1] < NO_VAR_BINS - 1)
		{
			ind1 = indexList[bestToMerge];
			ind2 = indexList[bestToMerge+2];
			error1 = error_tab[ind1];
			error2 = error_tab[ind2];
			pixAcc_temp = Enc_ALF->m_pixAcc_merged[ind1] + Enc_ALF->m_pixAcc_merged[ind2];
			for (i=0; i<sqrFiltLength; i++)
			{
				Enc_ALF->m_y_temp[i] = Enc_ALF->m_y_merged[ind1][i] + Enc_ALF->m_y_merged[ind2][i];
				for (j=0; j < sqrFiltLength; j++)
				{
					Enc_ALF->m_E_temp[i][j] = Enc_ALF->m_E_merged[ind1][i][j] + Enc_ALF->m_E_merged[ind2][i][j];
				}
			}
			error_comb_tab[ind1] = calculateErrorAbs(Enc_ALF->m_E_temp, Enc_ALF->m_y_temp, pixAcc_temp, sqrFiltLength) - error1 - error2;
		}

		ind=0;
		for (i = 0; i < NO_VAR_BINS; i++)
		{
			if (available[i] == 1)
			{
				indexList[ind] = i;
				ind++;
			}
		}
		noRemaining--;
	}

	errorMin = 0;
	for (ind = 0; ind < noIntervals; ind++)
	{
		errorMin += error_tab[indexList[ind]];
	}

	for (ind = 0; ind < noIntervals - 1; ind++)
	{
		intervalBest[ind][0] = indexList[ind]; 
		intervalBest[ind][1] = indexList[ind+1] - 1;
	}

	intervalBest[noIntervals-1][0] = indexList[noIntervals-1]; 
	intervalBest[noIntervals-1][1] = NO_VAR_BINS-1;

	return(errorMin);
}

double calculateErrorAbs(double **A, double *b, double y, int size)
{
	int i;
	double error, sum;
	double c[ALF_MAX_NUM_COEF];

	gnsSolveByChol(A, b, c, size);

	sum = 0;
	for (i = 0; i < size; i++)
	{
		sum += c[i] * b[i];
	}
	error = y - sum;

	return error;
}
int gnsCholeskyDec(double **inpMatr, double outMatr[ALF_MAX_NUM_COEF][ALF_MAX_NUM_COEF], int noEq)
{ 
	int i, j, k;     /* Looping Variables */
	double scale;       /* scaling factor for each row */
	double invDiag[ALF_MAX_NUM_COEF];  /* Vector of the inverse of diagonal entries of outMatr */

	//  Cholesky decomposition starts

	for(i = 0; i < noEq; i++)
	{
		for(j = i; j < noEq; j++)
		{
			/* Compute the scaling factor */
			scale = inpMatr[i][j];
			if ( i > 0) 
			{
				for( k = i - 1 ; k >= 0 ; k--)
				{
					scale -= outMatr[k][j] * outMatr[k][i];
				}
			}
			/* Compute i'th row of outMatr */
			if(i == j)
			{
				if(scale <= REG_SQR ) // if(scale <= 0 )  /* If inpMatr is singular */
				{
					return 0;
				}
				else
				{
					/* Normal operation */
					invDiag[i] =  1.0 / (outMatr[i][i] = sqrt(scale));
				}
			}
			else
			{
				outMatr[i][j] = scale * invDiag[i]; /* Upper triangular part          */
				outMatr[j][i] = 0.0;              /* Lower triangular part set to 0 */
			}                    
		}
	}
	return 1; /* Signal that Cholesky factorization is successfully performed */
}

void gnsTransposeBacksubstitution(double U[ALF_MAX_NUM_COEF][ALF_MAX_NUM_COEF], double rhs[], double x[], int order)
{
	int i,j;              /* Looping variables */
	double sum;              /* Holds backsubstitution from already handled rows */

	/* Backsubstitution starts */
	x[0] = rhs[0] / U[0][0];               /* First row of U'                   */
	for (i = 1; i < order; i++)
	{         /* For the rows 1..order-1           */    
		for (j = 0, sum = 0.0; j < i; j++) /* Backsubst already solved unknowns */
		{
			sum += x[j] * U[j][i];
		}
		x[i] = (rhs[i] - sum) / U[i][i];       /* i'th component of solution vect.  */
	}
}
void gnsBacksubstitution(double R[ALF_MAX_NUM_COEF][ALF_MAX_NUM_COEF], double z[ALF_MAX_NUM_COEF], int R_size, double A[ALF_MAX_NUM_COEF])
{
	int i, j;
	double sum;

	R_size--;

	A[R_size] = z[R_size] / R[R_size][R_size];

	for (i = R_size-1; i >= 0; i--)
	{
		for (j = i + 1, sum = 0.0; j <= R_size; j++)
		{
			sum += R[i][j] * A[j];
		}

		A[i] = (z[i] - sum) / R[i][i];
	}
}
int gnsSolveByChol(double **LHS, double *rhs, double *x, int noEq)
{
	double aux[ALF_MAX_NUM_COEF];     /* Auxiliary vector */
	double U[ALF_MAX_NUM_COEF][ALF_MAX_NUM_COEF];    /* Upper triangular Cholesky factor of LHS */
	int  i, singular;          /* Looping variable */
    assert(noEq > 0);
  
  /* The equation to be solved is LHSx = rhs */
  
  /* Compute upper triangular U such that U'*U = LHS */
  if(gnsCholeskyDec(LHS, U, noEq)) /* If Cholesky decomposition has been successful */
  {
    singular = 1;
    /* Now, the equation is  U'*U*x = rhs, where U is upper triangular
     * Solve U'*aux = rhs for aux
     */
    gnsTransposeBacksubstitution(U, rhs, aux, noEq);         
    
    /* The equation is now U*x = aux, solve it for x (new motion coefficients) */
    gnsBacksubstitution(U, aux, noEq, x);   
    
  }
  else /* LHS was singular */ 
  {
    singular = 0;
    
    /* Regularize LHS */
    for(i=0; i < noEq; i++)
    {
      LHS[i][i] += REG;
    }
    /* Compute upper triangular U such that U'*U = regularized LHS */
    singular = gnsCholeskyDec(LHS, U, noEq);
    if ( singular == 1 )
    {
      /* Solve  U'*aux = rhs for aux */  
      gnsTransposeBacksubstitution(U, rhs, aux, noEq);   
      
      /* Solve U*x = aux for x */
      gnsBacksubstitution(U, aux, noEq, x);      
    }
    else
    {
      x[0] = 1.0;
      for (i = 1; i < noEq; i++ )
      {
        x[i] = 0.0;
      }
    }
  }  
  return singular;
}



double findFilterCoeff(double ***EGlobalSeq, double **yGlobalSeq, double *pixAccGlobalSeq, int **filterCoeffSeq, int **filterCoeffQuantSeq, int intervalBest[NO_VAR_BINS][2], int varIndTab[NO_VAR_BINS], int sqrFiltLength, int filters_per_fr, int *weights, double errorTabForce0Coeff[NO_VAR_BINS][2])
{
	static double pixAcc_temp;
	static Boolean isFirst = TRUE;
	static int* filterCoeffQuant = NULL;
	static double* filterCoeff = NULL;
	double error;
	int k, filtNo;
#if !FIX_LEAK
	if(isFirst)
#endif
	{
		get_mem1Dint(&filterCoeffQuant,ALF_MAX_NUM_COEF);
		filterCoeff = (double*)malloc(ALF_MAX_NUM_COEF * sizeof(double));
		isFirst = FALSE;
	}


	error = 0;
	for(filtNo = 0; filtNo < filters_per_fr; filtNo++)
	{
		add_A(Enc_ALF->m_E_temp, EGlobalSeq, intervalBest[filtNo][0], intervalBest[filtNo][1], sqrFiltLength);
		add_b(Enc_ALF->m_y_temp, yGlobalSeq, intervalBest[filtNo][0], intervalBest[filtNo][1], sqrFiltLength);

		pixAcc_temp = 0;    
		for(k = intervalBest[filtNo][0]; k <= intervalBest[filtNo][1]; k++)
			pixAcc_temp += pixAccGlobalSeq[k];

		// Find coeffcients
		errorTabForce0Coeff[filtNo][1] = pixAcc_temp + QuantizeIntegerFilterPP(filterCoeff, filterCoeffQuant, Enc_ALF->m_E_temp, Enc_ALF->m_y_temp, sqrFiltLength, weights);
		errorTabForce0Coeff[filtNo][0] = pixAcc_temp;
		error += errorTabForce0Coeff[filtNo][1];

		for(k = 0; k < sqrFiltLength; k++)
		{
			filterCoeffSeq[filtNo][k] = filterCoeffQuant[k];
			filterCoeffQuantSeq[filtNo][k] = filterCoeffQuant[k];
		}
	}

	for(filtNo = 0; filtNo < filters_per_fr; filtNo++)
	{
		for(k = intervalBest[filtNo][0]; k <= intervalBest[filtNo][1]; k++)
			varIndTab[k] = filtNo;
	}
#if FIX_LEAK
      free_mem1Dint(filterCoeffQuant);
      free(filterCoeff);
#endif
	return(error);
}
void add_A(double **Amerged, double ***A, int start, int stop, int size)
{ 
	int i, j, ind;          /* Looping variable */

	for (i = 0; i < size; i++)
	{
		for (j = 0; j < size; j++)
		{
			Amerged[i][j] = 0;
			for (ind = start; ind <= stop; ind++)
			{
				Amerged[i][j] += A[ind][i][j];
			}
		}
	}
}

void add_b(double *bmerged, double **b, int start, int stop, int size)
{ 
	int i, ind;          /* Looping variable */

	for (i = 0; i < size; i++)
	{
		bmerged[i] = 0;
		for (ind = start; ind <= stop; ind++)
		{
			bmerged[i] += b[ind][i];
		}
	}
}
void roundFiltCoeff(int *FilterCoeffQuan, double *FilterCoeff, int sqrFiltLength, int factor)
{
	int i;
	double diff; 
	int diffInt, sign; 

	for(i = 0; i < sqrFiltLength; i++)
	{
		sign = (FilterCoeff[i] > 0)? 1 : -1; 
		diff = FilterCoeff[i] * sign; 
		diffInt = (int)(diff * (double)factor + 0.5); 
		FilterCoeffQuan[i] = diffInt * sign;
	}
}
double calculateErrorCoeffProvided(double **A, double *b, double *c, int size)
{
	int i, j;
	double error, sum = 0;

	error = 0;
	for (i = 0; i < size; i++)   //diagonal
	{
		sum = 0;
		for (j = i + 1; j < size; j++)
		{
			sum += (A[j][i] + A[i][j]) * c[j];
		}
		error += (A[i][i] * c[i] + sum - 2 * b[i]) * c[i];
	}

	return error;
}
double QuantizeIntegerFilterPP(double *filterCoeff, int *filterCoeffQuant, double **E, double *y, int sqrFiltLength, int *weights)
{
	double error;
	static Boolean isFirst = TRUE;
	static int* filterCoeffQuantMod= NULL;
	int factor = (1<<  ((int)ALF_NUM_BIT_SHIFT)  );
	int i;
	int quantCoeffSum, minInd, targetCoeffSumInt, k, diff;
	double targetCoeffSum, errMin;
#if !FIX_LEAK
	if(isFirst)
#endif
	{
		//filterCoeffQuantMod = new int[ALF_MAX_NUM_COEF];
		get_mem1Dint(&filterCoeffQuantMod,ALF_MAX_NUM_COEF);
		isFirst = FALSE;
	}



	gnsSolveByChol(E, y, filterCoeff, sqrFiltLength);
	targetCoeffSum=0;
	for (i=0; i<sqrFiltLength; i++)
	{
		targetCoeffSum+=(weights[i]*filterCoeff[i]*factor);
	}

	targetCoeffSumInt=ROUND(targetCoeffSum);
	roundFiltCoeff(filterCoeffQuant, filterCoeff, sqrFiltLength, factor);

	quantCoeffSum=0;
	for (i=0; i<sqrFiltLength; i++)
	{
		quantCoeffSum+=weights[i]*filterCoeffQuant[i];
	}

	while(quantCoeffSum!=targetCoeffSumInt)
	{
		if (quantCoeffSum>targetCoeffSumInt)
		{
			diff=quantCoeffSum-targetCoeffSumInt;
			errMin=0; minInd=-1;
			for (k=0; k<sqrFiltLength; k++)
			{
				if (weights[k]<=diff)
				{
					for (i=0; i<sqrFiltLength; i++)
					{
						filterCoeffQuantMod[i]=filterCoeffQuant[i];
					}
					filterCoeffQuantMod[k]--;
					for (i=0; i<sqrFiltLength; i++)
					{
						filterCoeff[i]=(double)filterCoeffQuantMod[i]/(double)factor;
					}
					error=calculateErrorCoeffProvided(E, y, filterCoeff, sqrFiltLength);
					if (error<errMin || minInd==-1)
					{
						errMin=error;
						minInd=k;
					}
				} // if (weights(k)<=diff)
			} // for (k=0; k<sqrFiltLength; k++)
			filterCoeffQuant[minInd]--;
		}
		else
		{
			diff=targetCoeffSumInt-quantCoeffSum;
			errMin=0; minInd=-1;
			for (k=0; k<sqrFiltLength; k++)
			{
				if (weights[k]<=diff)
				{
					for (i=0; i<sqrFiltLength; i++)
					{
						filterCoeffQuantMod[i]=filterCoeffQuant[i];
					}
					filterCoeffQuantMod[k]++;
					for (i=0; i<sqrFiltLength; i++)
					{
						filterCoeff[i]=(double)filterCoeffQuantMod[i]/(double)factor;
					}
					error=calculateErrorCoeffProvided(E, y, filterCoeff, sqrFiltLength);
					if (error<errMin || minInd==-1)
					{
						errMin=error;
						minInd=k;
					}
				} // if (weights(k)<=diff)
			} // for (k=0; k<sqrFiltLength; k++)
			filterCoeffQuant[minInd]++;
		}

		quantCoeffSum=0;
		for (i=0; i<sqrFiltLength; i++)
		{
			quantCoeffSum+=weights[i]*filterCoeffQuant[i];
		}
	}

	checkFilterCoeffValue(filterCoeffQuant, sqrFiltLength, FALSE);

	for (i=0; i<sqrFiltLength; i++)
	{
		filterCoeff[i]=(double)filterCoeffQuant[i]/(double)factor;
	}

	error=calculateErrorCoeffProvided(E, y, filterCoeff, sqrFiltLength);
#if FIX_LEAK
    free_mem1Dint(filterCoeffQuantMod);
#endif
	return(error);
}

double xfindBestCoeffCodMethod(int **filterCoeffSymQuant, int filter_shape, int sqrFiltLength, int filters_per_fr, double errorForce0CoeffTab[NO_VAR_BINS][2],double lambda)
{
	int coeffBits, i;
	double error=0, lagrangian;
	static Boolean  isFirst = TRUE;
	static int** coeffmulti = NULL;
	int g;
#if !FIX_LEAK
	if(isFirst)
#endif
	{
		
		get_mem2Dint(&coeffmulti,NO_VAR_BINS ,ALF_MAX_NUM_COEF);
		
		isFirst = FALSE;
	}

	for(g=0; g< filters_per_fr; g++)
	{
		for(i=0; i< sqrFiltLength; i++)
		{
			coeffmulti[g][i] = filterCoeffSymQuant[g][i];
		}
	}
	predictALFCoeff(coeffmulti, sqrFiltLength, filters_per_fr);

	coeffBits = 0;
	for(g=0; g< filters_per_fr; g++)
	{
		coeffBits += filterCoeffBitrateEstimate(ALF_Y, coeffmulti[g]);
	}
	for(i=0;i<filters_per_fr;i++)
	{
		error += errorForce0CoeffTab[i][1];
	}
	lagrangian = error + lambda * coeffBits;
#if FIX_LEAK
    free_mem2Dint(coeffmulti);
#endif
	return (lagrangian);
}

void predictALFCoeff(int** coeff, int numCoef, int numFilters)
{
	int g,pred,sum,i;
	for(g=0; g< numFilters; g++ )
	{
		sum=0;
		for(i=0; i< numCoef-1;i++)
		{
			sum += (2* coeff[g][i]);
		}

		pred = (1<<ALF_NUM_BIT_SHIFT) - (sum);
		coeff[g][numCoef-1] = coeff[g][numCoef-1] - pred;
	}
}

long xCalcSSD(byte* pOrg, byte* pCmp, int iWidth, int iHeight, int iStride )
{
	long uiSSD = 0;
	int x, y;

	unsigned int  uiShift = Enc_ALF->m_uiBitIncrement<<1;
	int iTemp;

	for( y = 0; y < iHeight; y++ )
	{
		for( x = 0; x < iWidth; x++ )
		{
			iTemp = pOrg[x] - pCmp[x]; uiSSD += ( iTemp * iTemp ) >> uiShift;
		}
		pOrg += iStride;
		pCmp += iStride;
	}

	return uiSSD;;
}



void Copy_frame_for_ALF()
{
	int i, j, k;
	
	for (j = 0; j < img->height; j++)
	{
		for ( i = 0; i< img->width; i++)
		{
			imgY_alf_Org[j*(img->width)+i] = imgY_org[j][i];
			imgY_alf_Rec[j*(img->width)+i] = imgY[j][i];
			
		}
	}
	
	for (k = 0; k < 2; k++)
	{
		for (j = 0; j < img->height_cr; j++)
		{
			for ( i = 0; i< img->width_cr; i++)
			{
				imgUV_alf_Org[k][j*(img->width_cr)+i] = imgUV_org[k][j][i];
				imgUV_alf_Rec[k][j*(img->width_cr)+i] = imgUV[k][j][i];
				
			}
		}
	}
 
}

void copyToImage(byte* pDest,byte* pSrc,int stride_in,int img_height,int img_width,int formatShift)
{
	int j,width,height;
	byte* pdst;
	byte* psrc;
	height = img_height>>formatShift;
	width  = img_width>>formatShift;
	psrc = pSrc;
	pdst = pDest;
	for (j=0;j<height;j++)
	{
		memcpy(pdst,psrc,width*sizeof(byte));
		pdst = pdst + stride_in;
		psrc = psrc + stride_in;
	}
}

void AllocateAlfPar(ALFParam** alf_par,int cID)
{

	*alf_par = (ALFParam*)malloc(sizeof(ALFParam));
	(*alf_par)->alf_flag          = 0;
	(*alf_par)->num_coeff		   = ALF_MAX_NUM_COEF;
	(*alf_par)->filters_per_group = 1;
	(*alf_par)->componentID       = cID;
	(*alf_par)->coeffmulti        = NULL;
	(*alf_par)->filterPattern     = NULL;

	switch (cID)
	{
	case ALF_Y:

		get_mem2Dint(&((*alf_par)->coeffmulti),NO_VAR_BINS,ALF_MAX_NUM_COEF);
		get_mem1Dint(&((*alf_par)->filterPattern),NO_VAR_BINS);
		break;
	case ALF_Cb:
	case ALF_Cr:
		get_mem2Dint(&((*alf_par)->coeffmulti),1,ALF_MAX_NUM_COEF);
		break;
	default:
		{
			printf("Not a legal component ID\n");
			assert(0);
			exit(-1);
		}
	}	
}
void freeAlfPar(ALFParam* alf_par,int cID)
{
	switch (cID)
	{
	case ALF_Y:
		free_mem2Dint(alf_par->coeffmulti);		
		free_mem1Dint(alf_par->filterPattern);
		break;
	case ALF_Cb:
	case ALF_Cr:
		free_mem2Dint(alf_par->coeffmulti);
		break;
	default:
		{
			printf("Not a legal component ID\n");
			assert(0);
			exit(-1);
		}
	}
	free(alf_par); 
	alf_par = NULL;

}
void allocateAlfAPS(ALF_APS* pAPS)
{
	int i;
	for (i = 0; i<NUM_ALF_COMPONENT;i++)
	{
		pAPS->alf_par[i].alf_flag          = 0;
		pAPS->alf_par[i].num_coeff		   = ALF_MAX_NUM_COEF;
		pAPS->alf_par[i].filters_per_group = 1;
		pAPS->alf_par[i].componentID       = i;
		pAPS->alf_par[i].coeffmulti        = NULL;
		pAPS->alf_par[i].filterPattern     = NULL;
		switch (i)
		{
		case ALF_Y:
			get_mem2Dint(&(pAPS->alf_par[i].coeffmulti),NO_VAR_BINS,ALF_MAX_NUM_COEF);
			get_mem1Dint(&(pAPS->alf_par[i].filterPattern),NO_VAR_BINS);
			break;
		case ALF_Cb:
		case ALF_Cr:
			get_mem2Dint(&(pAPS->alf_par[i].coeffmulti),1,ALF_MAX_NUM_COEF);
			break;
		default:
			{
				printf("Not a legal component ID\n");
				assert(0);
				exit(-1);
			}
		}	
	}

}
void freeAlfAPS(ALF_APS* pAPS)
{
	int i;
	for (i = 0 ; i < NUM_ALF_COMPONENT; i++)
	{

		switch (i)
		{
		case ALF_Y:
			free_mem2Dint(pAPS->alf_par[i].coeffmulti);
			free_mem1Dint(pAPS->alf_par[i].filterPattern);

			break;
		case ALF_Cb:
		case ALF_Cr:
			free_mem2Dint(pAPS->alf_par[i].coeffmulti);
			break;
		default:
			{
				printf("Not a legal component ID\n");
				assert(0);
				exit(-1);
			}
		}	

	}
}


void deriveBoundaryAvail(int numLCUInPicWidth, int numLCUInPicHeight, int ctu, Boolean* isLeftAvail, Boolean* isRightAvail, Boolean* isAboveAvail, Boolean* isBelowAvail, Boolean* isAboveLeftAvail, Boolean* isAboveRightAvail)
{
	int  numLCUsInFrame = numLCUInPicHeight * numLCUInPicWidth;
	int  lcuHeight         = 1<<g_MaxSizeInbit;
	int  lcuWidth          = lcuHeight;
	int  img_height        = img->height;
	int  img_width         = img->width;

	int  NumCUInFrame;
	int  pic_x            ;
	int  pic_y            ;
	int  mb_x             ;
	int  mb_y             ;
	int  mb_nr            ;
	int  pic_mb_width     = img_width / MIN_CU_SIZE;
	int  cuCurrNum        ;

	codingUnit* cuCurr          ;
	codingUnit* cuLeft          ;
	codingUnit* cuRight         ;
	codingUnit* cuAbove         ;

  codingUnit* cuAboveLeft;
  codingUnit* cuAboveRight;

  int curSliceNr,neighorSliceNr;

  NumCUInFrame = numLCUInPicHeight * numLCUInPicWidth;

  pic_x        = (ctu % numLCUInPicWidth )* lcuWidth;
  pic_y        = (ctu / numLCUInPicWidth )* lcuHeight;

  pic_mb_width  += (img_width  % MIN_CU_SIZE)? 1 : 0;

  mb_x               = pic_x / MIN_CU_SIZE;
  mb_y               = pic_y / MIN_CU_SIZE;
  mb_nr              = mb_y * pic_mb_width + mb_x;
  cuCurrNum          = mb_nr;

  *isLeftAvail		= (ctu % numLCUInPicWidth != 0);
  *isRightAvail		= (ctu % numLCUInPicWidth != numLCUInPicWidth - 1);
  *isAboveAvail		= (ctu  >= numLCUInPicWidth);
  *isBelowAvail	= (ctu  < numLCUsInFrame - numLCUInPicWidth);				// 

  *isAboveLeftAvail = *isAboveAvail && *isLeftAvail;
  *isAboveRightAvail = *isAboveAvail && *isRightAvail;

  cuCurr          = &(img->mb_data[cuCurrNum]);
  cuLeft          = *isLeftAvail      ? &(img->mb_data[cuCurrNum-1]):NULL;
  cuRight         = *isRightAvail     ? &(img->mb_data[cuCurrNum+(lcuWidth>>3)]):NULL;
  cuAbove         = *isAboveAvail     ? &(img->mb_data[cuCurrNum-pic_mb_width]):NULL;

  cuAboveLeft = *isAboveLeftAvail ? &(img->mb_data[cuCurrNum - pic_mb_width - 1]) : NULL;
  cuAboveRight = *isAboveRightAvail ? &(img->mb_data[cuCurrNum - pic_mb_width + (lcuWidth >> 3)]) : NULL;

  if (!input->crossSliceLoopFilter)
  {
    *isLeftAvail = *isRightAvail = *isAboveAvail = FALSE;
    *isAboveLeftAvail = *isAboveRightAvail = FALSE;
    curSliceNr = cuCurr->slice_nr;
    if (cuLeft != NULL)
    {
      neighorSliceNr = cuLeft->slice_nr;
      if (curSliceNr == neighorSliceNr)
      {
        *isLeftAvail = TRUE;
      }
    }

    if (cuRight != NULL)
    {
      neighorSliceNr = cuRight->slice_nr;
      if (curSliceNr == neighorSliceNr)
      {
        *isRightAvail = TRUE;
      }
    }
    if (cuAbove != NULL)
    {
      neighorSliceNr  = cuAbove->slice_nr;
      if (curSliceNr == neighorSliceNr)
      {
        *isAboveAvail = TRUE;
      }
    }
    if (cuAboveLeft != NULL)
    {
      neighorSliceNr = cuAboveLeft->slice_nr;
      if (curSliceNr == neighorSliceNr)
      {
        *isAboveLeftAvail = TRUE;
      }
    }
    if (cuAboveRight != NULL)
    {
      neighorSliceNr = cuAboveRight->slice_nr;
      if (curSliceNr == neighorSliceNr)
      {
        *isAboveRightAvail = TRUE;
      }
    }
  }
}
