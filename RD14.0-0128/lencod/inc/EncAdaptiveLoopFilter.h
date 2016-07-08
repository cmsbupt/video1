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
#ifndef ENCADAPTIVELOOPFILTER_H
#define ENCADAPTIVELOOPFILTER_H

#include "commonVariables.h"
#include "defines.h"
#include "contributors.h"
#include "../../lcommon/inc/ComAdaptiveLoopFilter.h"

typedef struct
{
	double*** ECorr; //!< auto-correlation matrix
	double**  yCorr; //!< cross-correlation
	double*   pixAcc;
	int componentID;
}AlfCorrData;

typedef struct
{
	AlfCorrData** m_alfCorr[NUM_ALF_COMPONENT];
	AlfCorrData** m_alfNonSkippedCorr[NUM_ALF_COMPONENT];
	AlfCorrData*  m_alfCorrMerged[NUM_ALF_COMPONENT];
	AlfCorrData**** m_alfPrevCorr;

	double*  m_y_merged[NO_VAR_BINS];
	double** m_E_merged[NO_VAR_BINS];
	double   m_y_temp[ALF_MAX_NUM_COEF];
	double   m_pixAcc_merged[NO_VAR_BINS];
	
	double** m_E_temp;
	ALFParam*** m_alfPictureParam;

	int *m_coeffNoFilter[NO_VAR_BINS];

	int **m_filterCoeffSym;
	int m_varIndTab[NO_VAR_BINS];

	int *m_numSlicesDataInOneLCU;
	Boolean m_alfLowLatencyEncoding ;
	byte** m_varImg;
	int m_alfReDesignIteration ;
	Boolean** m_AlfLCUEnabled;
	unsigned int m_uiBitIncrement ;
	CSptr m_cs_alf_cu_ctr;
#if ALF_ESTIMATE_AEC_RATE
  CSptr m_cs_alf_initial;
#endif
}EncALFVar;
extern EncALFVar *Enc_ALF;

void copyToImage(byte* pDest,byte* pSrc,int stride_in,int img_height,int img_width,int formatShift);
void reset_alfCorr(AlfCorrData* alfCorr);
unsigned int uvlcBitrateEstimate(int val);
void AllocateAlfCorrData(AlfCorrData **dst,int cIdx);
void freeAlfCorrData(AlfCorrData *dst);
void setCurAlfParam(ALFParam** alfPictureParam, double lambda);
void ALFProcess(ALFParam** alfPictureParam, ImageParameters* img, byte* imgY_org, byte** imgUV_org, byte* imgY_alf,byte** imgUV_alf, double lambda_mode);
void getStatistics(ImageParameters* img,byte* imgY_org,byte** imgUV_org,byte* yuvExtRecY,byte** yuvExtRecUV,int stride,double lambda);
void deriveLoopFilterBoundaryAvailibility(ImageParameters* img, int numLCUPicWidth, int numLCUPicHeight, int ctu,Boolean* isLeftAvail,Boolean* isRightAvail,Boolean* isAboveAvail,Boolean* isBelowAvail,double lambda);

void deriveBoundaryAvail(int numLCUPicWidth, int numLCUPicHeight, int ctu, Boolean* isLeftAvail, Boolean* isRightAvail, Boolean* isAboveAvail, Boolean* isBelowAvail, Boolean* isAboveLeftAvail, Boolean* isAboveRightAvail);

void createAlfGlobalBuffers(ImageParameters* img);
void destroyAlfGlobalBuffers(ImageParameters* img,unsigned int uiMaxSizeInbit);
void getStatisticsOneLCU(Boolean skipCUBoundaries, int compIdx, int lcuAddr
  , int ctuYPos, int ctuXPos, int ctuHeight, int ctuWidth
  , Boolean isAboveAvail, Boolean isBelowAvail, Boolean isLeftAvail, Boolean isRightAvail, Boolean isAboveLeftAvail, Boolean isAboveRightAvail
  , AlfCorrData* alfCorr, byte* pPicOrg, byte* pPicSrc
  , int stride, int formatShift, int numLCUPicWidth, int NumCUsInFrame);
void calcCorrOneCompRegionLuma(byte* imgOrg, byte* imgPad, int stride, int yPos, int xPos, int height, int width
  , double ***eCorr, double **yCorr, double *pixAcc, int isLeftAvail, int isRightAvail, int isAboveAvail, int isBelowAvail, int isAboveLeftAvail, int isAboveRightAvail);
void calcCorrOneCompRegionChma(byte* imgOrg, byte* imgPad, int stride, int yPos, int xPos, int height, int width, double **eCorr,
  double *yCorr, int isLeftAvail, int isRightAvail, int isAboveAvail, int isBelowAvail, int isAboveLeftAvail, int isAboveRightAvail);
#if ALF_ENC_IMPORVE_RATE_ESTIMATE

unsigned int estimateALFBitrateInPicHeader(ALFParam** alfPicParam);
double executePicLCUOnOffDecision(ALFParam** alfPictureParam
								  , double lambda
								  , Boolean isRDOEstimate
								  , AlfCorrData*** alfCorr
								  ,byte* imgY_org,byte** imgUV_org,byte* imgY_Dec,byte** imgUV_Dec,byte* imgY_Res,byte** imgUV_Res
								  ,int stride
								  );
#else
void executePicLCUOnOffDecision(ALFParam** alfPictureParam,
								byte* imgY_org,byte** imgUV_org,
								byte* imgY_Dec,byte** imgUV_Dec,
								byte* imgY_ExtRec,
								byte** imgUV_ExtRec, 
								int stride, 
								double lambda);
#endif
unsigned int ALFParamBitrateEstimate(ALFParam* alfParam);
long estimateFilterDistortion(int compIdx, AlfCorrData* alfCorr, int** coeffSet, int filterSetSize, int* mergeTable, Boolean doPixAccMerge);
void mergeFrom(AlfCorrData* dst, AlfCorrData* src, int* mergeTable, Boolean doPixAccMerge);
long xCalcSSD(byte* pOrg, byte* pCmp, int iWidth, int iHeight, int iStride );
double findFilterCoeff(double ***EGlobalSeq, double **yGlobalSeq, double *pixAccGlobalSeq, int **filterCoeffSeq, int **filterCoeffQuantSeq, int intervalBest[NO_VAR_BINS][2], int varIndTab[NO_VAR_BINS], int sqrFiltLength, int filters_per_fr, int *weights, double errorTabForce0Coeff[NO_VAR_BINS][2]);
double xfindBestCoeffCodMethod(int **filterCoeffSymQuant, int filter_shape, int sqrFiltLength, int filters_per_fr, double errorForce0CoeffTab[NO_VAR_BINS][2],double lambda);
#if !ALF_ENC_IMPORVE_RATE_ESTIMATE
double estimatePicCtrl(ALFParam** alfPictureParam, AlfCorrData*** alfLcuCorr, Boolean considerApsOverhead, int apsId,double lambda);
#endif
int getTemporalLayerNo(int curPoc, int picDistance);
void filterOneCTB(byte* pRest, byte* pDec, int stride, int compIdx, ALFParam* alfParam, int ctuYPos, int ctuHeight, int ctuXPos, int ctuWidth
  , Boolean isAboveAvail, Boolean isBelowAvail, Boolean isLeftAvail, Boolean isRightAvail, Boolean isAboveLeftAvail, Boolean isAboveRightAvail);
void copyOneAlfBlk(byte* picDst, byte* picSrc, int stride, int ypos, int xpos, int height, int width, int isAboveAvail, int isBelowAvail, int isChroma);
void ADD_AlfCorrData(AlfCorrData* A,AlfCorrData* B, AlfCorrData* C);
void accumulateLCUCorrelations(AlfCorrData** alfCorrAcc, AlfCorrData*** alfCorSrcLCU, Boolean useAllLCUs);
void decideAlfPictureParam(ALFParam** alfPictureParam, AlfCorrData** alfCorr,double m_dLambdaLuma);
void deriveFilterInfo(int compIdx, AlfCorrData* alfCorr, ALFParam* alfFiltParam, int maxNumFilters, double lambda);
void xcodeFiltCoeff(int **filterCoeff, int* varIndTab, int numFilters, ALFParam* alfParam);
void xQuantFilterCoef(double* h, int* qh);
void xFilterCoefQuickSort( double *coef_data, int *coef_num, int upper, int lower );
void xfindBestFilterVarPred(double **ySym, double ***ESym, double *pixAcc, int **filterCoeffSym, int *filters_per_fr_best, int varIndTab[], double lambda_val, int numMaxFilters);
double mergeFiltersGreedy(double **yGlobalSeq, double ***EGlobalSeq, double *pixAccGlobalSeq, int intervalBest[NO_VAR_BINS][2], int sqrFiltLength, int noIntervals);
#if !ALF_ENC_IMPORVE_RATE_ESTIMATE
void estimateLcuControl(ALFParam** alfPictureParam, AlfCorrData*** alfCorr, Boolean writeOnOff, long* encDist);
#endif
void storeAlfTemporalLayerInfo(ALFParam** alfPictureParam, int temporalLayer,double lambda);
double calculateErrorAbs(double **A, double *b, double y, int size);
void predictALFCoeff(int** coeff, int numCoef, int numFilters);


double QuantizeIntegerFilterPP(double *filterCoeff, int *filterCoeffQuant, double **E, double *y, int sqrFiltLength, int *weights);
void roundFiltCoeff(int *FilterCoeffQuan, double *FilterCoeff, int sqrFiltLength, int factor);
double calculateErrorCoeffProvided(double **A, double *b, double *c, int size);
void add_A(double **Amerged, double ***A, int start, int stop, int size);
void add_b(double *bmerged, double **b, int start, int stop, int size);
void Copy_frame_for_ALF();

int gnsSolveByChol(double **LHS, double *rhs, double *x, int noEq);
long xFastFiltDistEstimation(double** ppdE, double* pdy, int* piCoeff, int iFiltLength);
long calcAlfLCUDist(Boolean isSkipLCUBoundary, int compIdx, int lcuAddr, int ctuYPos, int ctuXPos, int ctuHeight, int ctuWidth, Boolean isAboveAvail, Boolean isBelowAvail, Boolean isLeftAvail, Boolean isRightAvail
					, byte* picSrc, byte* picCmp, int stride, int formatShift);

void AllocateAlfPar(ALFParam** alf_par,int cID);
void freeAlfPar(ALFParam* alf_par,int cID);
void allocateAlfAPS(ALF_APS* pAPS);
void freeAlfAPS(ALF_APS* pAPS);
#endif
