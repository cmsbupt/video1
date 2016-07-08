#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "global.h"
#include "../../lcommon/inc/commonVariables.h"
#include "../../lcommon/inc/commonStructures.h"
#include "../../lcommon/inc/ComAdaptiveLoopFilter.h"
#include "../../ldecod/inc/DecAdaptiveLoopFilter.h"
#include "../../ldecod/inc/AEC.h"
#include "../../lcommon/inc/memalloc.h"
#define Clip_post(high,val) ((val > high)? high: val)

DecALFVar *Dec_ALF;
/*
*************************************************************************
* Function: ALF decoding process top function
* Input: 
*         alfParam : ALF parameters
*             img  : The image parameter
*    imgY_alf_Dec  : The Y component of the SAO output image
*    imgUV_alf_Dec : The UV component of the SAO output image
*************************************************************************
*/
#if EXTEND_BD
void ALFProcess_dec(ALFParam** alfParam, ImageParameters* img, byte* imgY_alf_Dec,byte** imgUV_alf_Dec, int sample_bit_depth)
#else
void ALFProcess_dec(ALFParam** alfParam, ImageParameters* img, byte* imgY_alf_Dec,byte** imgUV_alf_Dec)
#endif
{
#if !ALF_FAST
	byte* imgY_ExtRes;
	byte** imgUV_ExtRes;
	byte* imgY_ExtDec;
	byte** imgUV_ExtDec;
	int LumaMarginX ;
	int LumaMarginY ;
	int ChromaMarginX;
	int ChromaMarginY;	
#endif

	byte *pResY,*pDecY;
	byte *pResUV[2],*pDecUV[2];

	int ctu,NumCUInFrame,numLCUInPicWidth,numLCUInPicHeight;
	int ctuYPos,ctuXPos, ctuHeight,ctuWidth; 
	int compIdx, lumaStride,chromaStride,lcuHeight,lcuWidth,img_height,img_width;
	Boolean  isLeftAvail,isRightAvail,isAboveAvail,isBelowAvail;

	Boolean  isAboveLeftAvail, isAboveRightAvail;


	lcuHeight         = 1<<g_MaxSizeInbit;
	lcuWidth          = lcuHeight;
	img_height        = img->height;
	img_width         = img->width;
	numLCUInPicWidth  = img_width / lcuWidth ;
	numLCUInPicHeight = img_height / lcuHeight ;
	numLCUInPicWidth  += (img_width % lcuWidth)? 1 : 0;
	numLCUInPicHeight += (img_height % lcuHeight)? 1 : 0;
	NumCUInFrame = numLCUInPicHeight * numLCUInPicWidth;

#if !ALF_FAST
	LumaMarginX = (1<<g_MaxSizeInbit) + 16;
	LumaMarginY = (1<<g_MaxSizeInbit) + 16;
	ChromaMarginX = LumaMarginX>>1;
	ChromaMarginY = LumaMarginY>>1;
	lumaStride  = img_width + (LumaMarginX<<1);
#else
	lumaStride = img_width;
#endif

	chromaStride = lumaStride>>1;

#if !ALF_FAST
	get_mem1D(&imgY_ExtDec,(img->height + (LumaMarginY<<1)) * (img->width + (LumaMarginX<<1)));
	get_mem2D(&imgUV_ExtDec,2,((img->height_cr + (ChromaMarginY<<1))*(img->width_cr + (ChromaMarginX<<1))));
	get_mem1D(&imgY_ExtRes,(img->height + (LumaMarginY<<1)) * (img->width + (LumaMarginX<<1)));
	get_mem2D(&imgUV_ExtRes,2,((img->height_cr + (ChromaMarginY<<1))*(img->width_cr + (ChromaMarginX<<1))));
	ExtendPicBorder(imgY_alf_Dec,img->height,img->width,LumaMarginY,LumaMarginX,imgY_ExtDec);
	ExtendPicBorder(imgUV_alf_Dec[0],img->height_cr,img->width_cr,ChromaMarginY,ChromaMarginX,imgUV_ExtDec[0]);
	ExtendPicBorder(imgUV_alf_Dec[1],img->height_cr,img->width_cr,ChromaMarginY,ChromaMarginX,imgUV_ExtDec[1]);

	ExtendPicBorder(imgY_alf_Dec,img->height,img->width,LumaMarginY,LumaMarginX,imgY_ExtRes);
	ExtendPicBorder(imgUV_alf_Dec[0],img->height_cr,img->width_cr,ChromaMarginY,ChromaMarginX,imgUV_ExtRes[0]);
	ExtendPicBorder(imgUV_alf_Dec[1],img->height_cr,img->width_cr,ChromaMarginY,ChromaMarginX,imgUV_ExtRes[1]);

	pResY     = imgY_ExtRes  + LumaMarginY * (img->width + (LumaMarginX<<1)) + LumaMarginX;
	pResUV[0] = imgUV_ExtRes[0] + (ChromaMarginY * (img->width_cr + (ChromaMarginX<<1)) + ChromaMarginX);
	pResUV[1] = imgUV_ExtRes[1] + (ChromaMarginY * (img->width_cr + (ChromaMarginX<<1)) + ChromaMarginX);

	pDecY     = imgY_ExtDec  + LumaMarginY * (img->width + (LumaMarginX<<1)) + LumaMarginX;
	pDecUV[0] = imgUV_ExtDec[0] + (ChromaMarginY * (img->width_cr + (ChromaMarginX<<1)) + ChromaMarginX);
	pDecUV[1] = imgUV_ExtDec[1] + (ChromaMarginY * (img->width_cr + (ChromaMarginX<<1)) + ChromaMarginX);
#else
	pResY     = imgY[0];
	pResUV[0] = imgUV[0][0];
	pResUV[1] = imgUV[1][0];

	pDecY     = imgY_alf_Dec;
	pDecUV[0] = imgUV_alf_Dec[0];
	pDecUV[1] = imgUV_alf_Dec[1];
#endif

	for(ctu=0; ctu< NumCUInFrame; ctu++)
	{
		ctuYPos   = (ctu / numLCUInPicWidth )* lcuHeight;
		ctuXPos   = (ctu % numLCUInPicWidth )* lcuWidth;
		ctuHeight = (ctuYPos + lcuHeight > img_height)?( img_height- ctuYPos):lcuHeight;
		ctuWidth  = (ctuXPos + lcuWidth  > img_width )?( img_width - ctuXPos):lcuWidth;

		//derive CTU boundary availabilities
    deriveBoundaryAvail( numLCUInPicWidth,numLCUInPicHeight, ctu,&isLeftAvail,&isRightAvail,&isAboveAvail, &isBelowAvail, &isAboveLeftAvail, &isAboveRightAvail);
		for(compIdx=0; compIdx < NUM_ALF_COMPONENT; compIdx++)
		{
			if(!Dec_ALF->m_AlfLCUEnabled[ctu][compIdx])
			{
				continue;
			}
			if (compIdx == ALF_Y)
			{
				filterOneCTB(pResY,pDecY,(compIdx == ALF_Y)?(lumaStride):(chromaStride)
					, compIdx, alfParam[compIdx], ctuYPos, ctuHeight, ctuXPos, ctuWidth
#if EXTEND_BD
          , isAboveAvail, isBelowAvail, isLeftAvail, isRightAvail, isAboveLeftAvail, isAboveRightAvail, sample_bit_depth);
#else
					, isAboveAvail, isBelowAvail, isLeftAvail, isRightAvail);
#endif
			}
			else
			{
				filterOneCTB(pResUV[compIdx-ALF_Cb],pDecUV[compIdx-ALF_Cb],(compIdx == ALF_Y)?(lumaStride):(chromaStride)
					, compIdx, alfParam[compIdx], ctuYPos, ctuHeight, ctuXPos, ctuWidth
#if EXTEND_BD
          , isAboveAvail, isBelowAvail, isLeftAvail, isRightAvail, isAboveLeftAvail, isAboveRightAvail, sample_bit_depth);
#else
					, isAboveAvail, isBelowAvail, isLeftAvail, isRightAvail);
#endif
			}		
			
		}
	}

#if !ALF_FAST
	setFilterImage(pResY,pResUV,lumaStride,imgY,imgUV,img_width,img_height);
	free_mem1D(imgY_ExtDec);
	free_mem2D(imgUV_ExtDec);
	free_mem1D(imgY_ExtRes);
	free_mem2D(imgUV_ExtRes);
#endif
}

/*
*************************************************************************
* Function: ALF filter on CTB
*************************************************************************
*/
void filterOneCTB(byte* pRest, byte* pDec, int stride, int compIdx, ALFParam* alfParam
				  , int ctuYPos, int ctuHeight, int ctuXPos, int ctuWidth
#if EXTEND_BD
          , Boolean isAboveAvail, Boolean isBelowAvail, Boolean isLeftAvail, Boolean isRightAvail, Boolean isAboveLeftAvail, Boolean isAboveRightAvail, int sample_bit_depth
#else
					, Boolean isAboveAvail, Boolean isBelowAvail, Boolean isLeftAvail, Boolean isRightAvail
#endif
				  )
{
	const int skipSize = (ALF_FOOTPRINT_SIZE>>1); //half size of 7x7cross+ 3x3square
	const int  formatShift  = (compIdx == ALF_Y)?0:1;
	int ypos,xpos,height,width;

	//reconstruct coefficients to m_filterCoeffSym and m_varIndTab
	reconstructCoefInfo(compIdx, alfParam, Dec_ALF->m_filterCoeffSym, Dec_ALF->m_varIndTab); //reconstruct ALF coefficients & related parameters 
#if EXTRACT_FULL_PPS_ALF_final  //2016-5-31 提取完整的参数
  if (compIdx == 0)//Y
  {
    if (1)
    {
      int i,j;
      for (i = 0; i < 16; i++) 
      { 
        for (j = 0; j < 9; j++)
        {
          pps_AlfCoeffLuma_Final[i][j]= Dec_ALF->m_filterCoeffSym[i][j];//16个region 每个有9个系数 处理过后的
          fprintf(p_pps, "reconstructCoefInfo pps_AlfCoeffLuma[%2d][%2d] %08X\n", i, j, Dec_ALF->m_filterCoeffSym[i][j]);
        }
      }
    }
  }
  else if(compIdx == 1)//Cb 
  {
    if (1)
    {
      int  j;
      for (j = 0; j < 9; j++)
      {
        pps_AlfCoeffChroma0_Final[j] = Dec_ALF->m_filterCoeffSym[0][j];//16个region 每个有9个系数 处理过后的
        fprintf(p_pps, "reconstructCoefInfo pps_AlfCoeffChroma0_Final[%2d][%2d] %08X\n", 0, j, Dec_ALF->m_filterCoeffSym[0][j]);
      }
    }
  }
  else if (compIdx == 2)//Cr
  {
    if (1)
    {
      int  j;
      for (j = 0; j < 9; j++)
      {
        pps_AlfCoeffChroma1_Final[j] = Dec_ALF->m_filterCoeffSym[0][j];//16个region 每个有9个系数 处理过后的
        fprintf(p_pps, "reconstructCoefInfo pps_AlfCoeffChroma0_Final[%2d][%2d] %08X\n", 0, j, Dec_ALF->m_filterCoeffSym[0][j]);
      }
    }
  }
#endif
	//derive CTB start positions, width, and height. If the boundary is not available, skip boundary samples.
	ypos         = (ctuYPos >> formatShift);
	height       = (ctuHeight >> formatShift);
	xpos         = (ctuXPos >> formatShift);
	width        = (ctuWidth >> formatShift );
	filterOneCompRegion(pRest, pDec, stride, (compIdx!=ALF_Y)
#if EXTEND_BD
    , ypos, height, xpos, width, Dec_ALF->m_filterCoeffSym, Dec_ALF->m_varIndTab, Dec_ALF->m_varImg, sample_bit_depth, isLeftAvail, isRightAvail, isAboveAvail, isBelowAvail, isAboveLeftAvail, isAboveRightAvail);
#else
		, ypos, ypos+height, xpos, xpos+width, Dec_ALF->m_filterCoeffSym, Dec_ALF->m_varIndTab, Dec_ALF->m_varImg, isLeftAvail, isRightAvail, isAboveAvail, isBelowAvail);
#endif
}
void deriveLoopFilterBoundaryAvailibility(ImageParameters* img,int numLCUInPicWidth, int numLCUInPicHeight, int ctu,Boolean* isLeftAvail,Boolean* isRightAvail,Boolean* isAboveAvail,Boolean* isBelowAvail)
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

void CreateAlfGlobalBuffer()
{
	int lcuHeight,lcuWidth, img_height,img_width;
	int NumCUInFrame,numLCUInPicWidth,numLCUInPicHeight;
	int n, i,g;
	int numCoef = (int) ALF_MAX_NUM_COEF;
	int maxNumTemporalLayer =   (int)(log10((float)(gop_size))/log10(2.0) + 1);

	int regionTable[NO_VAR_BINS] = {0, 1, 4, 5, 15, 2, 3, 6, 14, 11, 10, 7, 13, 12,  9,  8}; 
	int xInterval  ;  
	int yInterval  ;
	int yIndex, xIndex;
	int yIndexOffset;
#if INTERLACE_CODING
  int auto_crop_bottom, auto_crop_right;
  int alfWidth, alfHeight, alfWidth_cr, alfHeight_cr;

  if ( horizontal_size % MIN_CU_SIZE != 0 )
  {
    auto_crop_right =  MIN_CU_SIZE - ( horizontal_size % MIN_CU_SIZE );
  }
  else
  {
    auto_crop_right = 0;
  }

  if ( vertical_size %  MIN_CU_SIZE != 0 )
  {
    auto_crop_bottom =  MIN_CU_SIZE - ( vertical_size % MIN_CU_SIZE );
  }
  else
  {
    auto_crop_bottom = 0;
  }

  alfWidth          = ( horizontal_size + auto_crop_right );
  alfHeight         = ( vertical_size + auto_crop_bottom );
  alfWidth_cr       = ( alfWidth >> 1 );

  if ( input->chroma_format == 1 )
  {
    alfHeight_cr      = ( alfHeight >> 1 );
  }
#endif

	lcuHeight         = 1<<(input->g_uiMaxSizeInBit);
	lcuWidth          = lcuHeight;
#if INTERLACE_CODING
  img_height        = alfHeight;
  img_width         = alfWidth;
#else
	img_height        = img->height;
	img_width         = img->width;
#endif
	numLCUInPicWidth  = img_width / lcuWidth ;
	numLCUInPicHeight = img_height / lcuHeight ;
	numLCUInPicWidth  += (img_width % lcuWidth)? 1 : 0;
	numLCUInPicHeight += (img_height % lcuHeight)? 1 : 0;
	NumCUInFrame = numLCUInPicHeight * numLCUInPicWidth;

	xInterval = ((( (img_width +lcuWidth -1) / lcuWidth) +1) /4 *lcuWidth) ;  
	yInterval = ((((img_height +lcuHeight -1) / lcuHeight) +1) /4 *lcuHeight) ;
	Dec_ALF = (DecALFVar *) malloc(1*sizeof(DecALFVar));	
#if INTERLACE_CODING
	get_mem1D(&imgY_alf_Rec,  (alfHeight * alfWidth));
	get_mem2D(&imgUV_alf_Rec,  2, (alfHeight_cr * alfWidth_cr));
#else
	get_mem1D(&imgY_alf_Rec,  (img->height * img->width));
	get_mem2D(&imgUV_alf_Rec,  2, (img->height_cr * img->width_cr));
#endif
	Dec_ALF->m_AlfLCUEnabled = (Boolean**)malloc(NumCUInFrame * sizeof(Boolean*));
	for (n=0;n<NumCUInFrame;n++)
	{
		Dec_ALF->m_AlfLCUEnabled[n] = (Boolean*) malloc(NUM_ALF_COMPONENT * sizeof(Boolean));
		memset(Dec_ALF->m_AlfLCUEnabled[n],FALSE,NUM_ALF_COMPONENT * sizeof(Boolean));
	}

	Dec_ALF->m_varImg = (byte**) malloc(img_height * sizeof(byte*));
	for (n=0;n<img_height;n++)
	{
		Dec_ALF->m_varImg[n] = (byte*) malloc(img_width * sizeof(byte));
		memset(Dec_ALF->m_varImg[n],0,img_width * sizeof(byte));
	}
	Dec_ALF->m_filterCoeffSym = (int**) malloc(NO_VAR_BINS * sizeof(int*));
	for(g=0 ; g< (int)NO_VAR_BINS; g++)
	{
		Dec_ALF->m_filterCoeffSym[g] = (int*)malloc(ALF_MAX_NUM_COEF * sizeof(int));
		memset(Dec_ALF->m_filterCoeffSym[g],0,ALF_MAX_NUM_COEF * sizeof(int));
	}
	for(i = 0; i < img_height; i=i+4)
	{
		yIndex = (yInterval == 0)?(3):(Clip_post( 3, i / yInterval));
		yIndexOffset = yIndex * 4 ;
		for(g = 0; g < img_width; g=g+4)
		{
			xIndex = (xInterval==0)?(3):(Clip_post( 3, g / xInterval));
			Dec_ALF->m_varImg[i>>LOG2_VAR_SIZE_H][g>>LOG2_VAR_SIZE_W] = regionTable[yIndexOffset + xIndex];
		}
	}
	Dec_ALF->m_alfPictureParam = (ALFParam**) malloc((NUM_ALF_COMPONENT) * sizeof(ALFParam*));
	for (i = 0; i < NUM_ALF_COMPONENT;i++)
	{
		Dec_ALF->m_alfPictureParam[i] = NULL;
		AllocateAlfPar(&(Dec_ALF->m_alfPictureParam[i]),i);
	}
	if ( ( img->dp_ALF = ( AlfDataPart * ) calloc ( 1, sizeof ( AlfDataPart ) ) ) == NULL )
	{
		no_mem_exit ( "init_img: dp_ALF" );
	}
	img->dp_ALF->syn_ctx = create_contexts_SyntaxInfo();
}
void ReleaseAlfGlobalBuffer()
{
	int lcuHeight,lcuWidth, img_height,img_width;
	int NumCUInFrame,numLCUInPicWidth,numLCUInPicHeight;
	int n, i, g;
	int numCoef = (int) ALF_MAX_NUM_COEF;
//	int maxNumTemporalLayer =   (int)(logf((float)(gop_size))/logf(2.0) + 1);
	int maxNumTemporalLayer =   (int)(log10f((float)(gop_size))/log10f(2.0) + 1);//zhaohaiwu 20151011

	lcuHeight         = 1<<(input->g_uiMaxSizeInBit);
	lcuWidth          = lcuHeight;
	img_height        = img->height;
	img_width         = img->width;
	numLCUInPicWidth  = img_width / lcuWidth ;
	numLCUInPicHeight = img_height / lcuHeight ;
	numLCUInPicWidth  += (img_width % lcuWidth)? 1 : 0;
	numLCUInPicHeight += (img_height % lcuHeight)? 1 : 0;
	NumCUInFrame = numLCUInPicHeight * numLCUInPicWidth;

	free_mem1D(imgY_alf_Rec);
	free_mem2D(imgUV_alf_Rec);

	for (n=0;n<img_height;n++)
	{
		free(Dec_ALF->m_varImg[n]);
	}
	free(Dec_ALF->m_varImg); Dec_ALF->m_varImg = NULL;

	for (n=0;n<NumCUInFrame;n++)
	{
		free(Dec_ALF->m_AlfLCUEnabled[n]);
	}
	free(Dec_ALF->m_AlfLCUEnabled); Dec_ALF->m_AlfLCUEnabled = NULL;

	for(g=0 ; g< (int)NO_VAR_BINS; g++)
	{
		free(Dec_ALF->m_filterCoeffSym[g]);
	}
	free(Dec_ALF->m_filterCoeffSym); Dec_ALF->m_filterCoeffSym = NULL;
	for(g=0 ; g< (int)NUM_ALF_COMPONENT; g++)
	{
		//free(Dec_ALF->m_alfPictureParam[g]);
      freeAlfPar(Dec_ALF->m_alfPictureParam[g], g); 
    //freeAlfPar(&(Dec_ALF->m_alfPictureParam[g]),g);
	}
	free(Dec_ALF->m_alfPictureParam); Dec_ALF->m_alfPictureParam = NULL;

	delete_contexts_SyntaxInfo(img->dp_ALF->syn_ctx);
	free(img->dp_ALF);
	free(Dec_ALF);Dec_ALF=NULL;
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
void Read_ALF_param ( char *buf, int startcodepos, int length )
{
	int compIdx;
	if (input->alf_enable)
	{
		img->pic_alf_on[0] = u_v ( 1, "alf_pic_flag_Y" );
		img->pic_alf_on[1] = u_v ( 1, "alf_pic_flag_Cb" );
		img->pic_alf_on[2] = u_v ( 1, "alf_pic_flag_Cr" );
		Dec_ALF->m_alfPictureParam[ALF_Y]->alf_flag  = img->pic_alf_on[ALF_Y];
		Dec_ALF->m_alfPictureParam[ALF_Cb]->alf_flag = img->pic_alf_on[ALF_Cb];
		Dec_ALF->m_alfPictureParam[ALF_Cr]->alf_flag = img->pic_alf_on[ALF_Cr];
		if (img->pic_alf_on[0]||img->pic_alf_on[1]||img->pic_alf_on[2])
		{
			for (compIdx = 0; compIdx < NUM_ALF_COMPONENT; compIdx++)
			{
				if (img->pic_alf_on[compIdx])
				{
					readAlfCoeff(Dec_ALF->m_alfPictureParam[compIdx]);
				}
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
	int  smb_mb_width     ;
	int  smb_mb_height    ;
	int  pic_mb_width     = img_width / MIN_CU_SIZE;
	int  cuCurrNum        ;

	codingUnit* cuCurr          ;
	codingUnit* cuLeft          ;
	codingUnit* cuRight         ;
	codingUnit* cuAbove         ;
	int curSliceNr,neighorSliceNr;

  codingUnit* cuAboveLeft;
  codingUnit* cuAboveRight;

	NumCUInFrame = numLCUInPicHeight * numLCUInPicWidth;

	pic_x        = (ctu % numLCUInPicWidth )* lcuWidth;
	pic_y        = (ctu / numLCUInPicWidth )* lcuHeight;

	pic_mb_width  += (img_width  % MIN_CU_SIZE)? 1 : 0;

	mb_x               = pic_x / MIN_CU_SIZE;
	mb_y               = pic_y / MIN_CU_SIZE;
	mb_nr              = mb_y * pic_mb_width + mb_x;
	cuCurrNum          = mb_nr;

	*isLeftAvail  = (ctu % numLCUInPicWidth != 0);
	*isRightAvail = (ctu % numLCUInPicWidth != numLCUInPicWidth - 1);
	*isAboveAvail = (ctu  >= numLCUInPicWidth);
	*isBelowAvail	= (ctu  < numLCUsInFrame - numLCUInPicWidth);			

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
