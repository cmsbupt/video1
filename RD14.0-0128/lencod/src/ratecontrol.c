/*
*****************************************************************************
* COPYRIGHT AND WARRANTY INFORMATION
* Copyright 2014-2015
* Contributors: Yimin ZHOU, yiminzhou@uestc.edu.cn
*               Minke LUO,  luominke@hotmail.com 
*               Min ZHONG,  201321060446@std.uestc.edu.cn
*               Ce ZHU,     eczhu@uestc.edu.cn
* Institution:  University of Electronic Science and Technology of China
******************************************************************************
*/

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "ratecontrol.h"
#define max(a, b)      (((a) > (b)) ? (a) : (b))  //!< Macro returning max value
#define min(a, b)      (((a) < (b)) ? (a) : (b))

double QueryTable[13][13]=
{
	{ -4.80, -4.80, -4.80, -4.80, -3.57, -3.57, -3.17, -3.17, -2.00, -2.00, -0.25, -0.25,  0.00, },
	{ -4.80, -4.80, -4.80, -4.80, -3.57, -3.57, -3.17, -3.17, -2.00, -2.00, -0.25, -0.25,  0.00, },
	{ -4.80, -4.80, -3.57, -3.57, -3.57, -3.57, -2.00, -2.00, -1.10, -1.10,  0.00,  0.00,  0.25, },
	{ -4.80, -4.80, -3.57, -3.57, -3.57, -3.57, -2.00, -2.00, -1.10, -1.10,  0.00,  0.00,  0.25, },
	{ -3.57, -3.57, -3.57, -3.57, -2.00, -2.00, -1.10, -1.10,  0.00,  0.00,  1.10,  1.10,  2.00, },
	{ -3.57, -3.57, -3.57, -3.57, -2.00, -2.00, -1.10, -1.10,  0.00,  0.00,  1.10,  1.10,  2.00, },
	{ -3.17, -3.17, -2.00, -2.00, -1.10, -1.10,  0.00,  0.00,  1.10,  1.10,  2.00,  2.00,  3.17, },
	{ -3.17, -3.17, -2.00, -2.00, -1.10, -1.10,  0.00,  0.00,  1.10,  1.10,  2.00,  2.00,  3.17, },
	{ -2.00, -2.00, -1.10, -1.10,  0.00,  0.00,  1.10,  1.10,  2.00,  2.00,  3.57,  3.57,  3.57, },
	{ -2.00, -2.00, -1.10, -1.10,  0.00,  0.00,  1.10,  1.10,  2.00,  2.00,  3.57,  3.57,  3.57, },
	{ -0.25, -0.25,  0.00,  0.00,  1.10,  1.10,  2.44,  2.44,  3.57,  3.57,  3.57,  3.57,  4.80, },
	{ -0.25, -0.25,  0.00,  0.00,  1.10,  1.10,  2.44,  2.44,  3.57,  3.57,  3.57,  3.57,  4.80, },
	{  0.00,  0.00,  0.25,  0.25,  2.00,  2.00,  3.57,  3.57,  3.86,  3.86,  4.80,  4.80,  4.80, },
};

char RateControlQueryTable[13][13];

void Init_FuzzyController(double ScaFactor)
{
	int i,j;
	for(i = 0; i < 13; i++)
		for(j = 0; j < 13; j++)
			RateControlQueryTable[i][j] = QueryTable[i][j]>0.0F ? (char)ceil(QueryTable[i][j]*ScaFactor) : (char)floor(QueryTable[i][j]*ScaFactor);
}

int  GetNewFrameDeltaQP_FuzzyController(double ActualValue,double DeltaValue,double Amax,double Amin,double Bmax,double Bmin)
{
	double dFuzAct,dFuzDel;
	int iFuzAct,iFuzDel,ConVal;

	dFuzAct = (12.0/(Amax - Amin))*(ActualValue - (Amax + Amin)/2.0);
	dFuzDel = (12.0/(Bmax - Bmin))*(DeltaValue - (Bmax + Bmin)/2.0);

	dFuzAct = fabs(dFuzAct)>6 ? 6.0*dFuzAct/fabs(dFuzAct) : dFuzAct;
	dFuzDel = fabs(dFuzDel)>6 ? 6.0*dFuzDel/fabs(dFuzDel) : dFuzDel;

	iFuzAct = (int)((dFuzAct<0 ? floor(dFuzAct + 0.5) : ceil(dFuzAct - 0.5))+6);
	iFuzDel = (int)((dFuzDel<0 ? floor(dFuzDel + 0.5) : ceil(dFuzDel - 0.5))+6);  

	ConVal = RateControlQueryTable[iFuzAct][iFuzDel];
	return ConVal;
}

void Init_RateControl(RateControl * prc, int rconoff, int lcuonoff, int totalframes, int intraperiod, int targetbitrate, int framerate, int iniQP, int w, int h, char * filename)
{
	prc->prclcu = pRCLCU;
	prc->RConoff = rconoff;
	prc->LCURConoff = lcuonoff;
	prc->IntraPeriod = intraperiod;
	prc->TotalFrames = totalframes;
	prc->CodedFrameNumber = 0;
	prc->ImageQP = prc->RcQP = prc->RcMBQP= iniQP;
	prc->FrameWidth = w;
	prc->FrameHeight = h;
	prc->GopBpp = 0.0F;
	prc->GopAllKeyBpp = 0.0F;
	prc->GopAvgBaseQP = 0;
	prc->GopAvgBaseQPCount = 0;
	prc->TargetBitrate =  targetbitrate;
	prc->TargetBitPerPixel = (double)(1.0) * targetbitrate / framerate / w / h;
	prc->TargetBufferLevel = 0.0F;
	prc->FirstBufferSizeLevel = 0.0F;
	prc->PreBufferError = prc->BufferError = 0.0F;
	prc->CurrentBufferSizeBpp = 0.0;
	prc->LowestBufferSizeBpp = prc->HighestBufferSizeBpp = 0.0F;
	prc->GopFlag = -100;
	prc->DeltaQP = 0;
	intraperiod == 1 ? Init_FuzzyController(0.50) : Init_FuzzyController(0.25);
}

void Updata_RateControl(RateControl * prc, int framebits, int frameqp, int imgtype, int framenumber, int goplength)
{
	int LevelLength = 0;
	double PI = 3.1415926;
	prc->ImageQP = frameqp;
	prc->ImageType = imgtype;
	prc->GlobalImageNumber = framenumber;
	prc->ImageBits = framebits;
	prc->ImageBpp = (double)(1.0) * framebits / prc->FrameWidth / prc->FrameHeight;
	prc->IntraFrameBpp = imgtype==0 ? prc->ImageBpp : prc->IntraFrameBpp;
	prc->InterFrameBpp = imgtype==1 || imgtype==4 || imgtype==0 ? prc->ImageBpp : prc->InterFrameBpp;
	if (imgtype == 0 || imgtype == 4)
	{
		prc->GopAvgBaseQP = imgtype == 0 ? prc->RcQP : prc->GopAvgBaseQP + prc->RcQP - 1 ;
		prc->GopAllKeyBpp = imgtype == 0 ? prc->ImageBpp : prc->GopAllKeyBpp + prc->ImageBpp;
		prc->GopAvgBaseQPCount = imgtype == 0 ? 1 : prc->GopAvgBaseQPCount + 1;
	}

	prc->CodedFrameNumber++;

	if(imgtype!=2)
		prc->GopBpp = prc->ImageBpp;
	else
		prc->GopBpp += prc->ImageBpp;

	prc->CurrentBufferSizeBpp +=  prc->ImageBpp - prc->TargetBitPerPixel;

	prc->HighestBufferSizeBpp = prc->CurrentBufferSizeBpp > prc->HighestBufferSizeBpp ? prc->CurrentBufferSizeBpp : prc->HighestBufferSizeBpp;
	prc->LowestBufferSizeBpp  = prc->CurrentBufferSizeBpp < prc->LowestBufferSizeBpp  ? prc->CurrentBufferSizeBpp : prc->LowestBufferSizeBpp;

	if(prc->IntraPeriod == 1)
	{
		prc->TargetBufferLevel = prc->DeltaBufferLevel = 0.0;
	}
	else if(prc->IntraPeriod == 0)
	{
		if(imgtype==0)
		{
			prc->TargetBufferLevel = prc->CurrentBufferSizeBpp;
			prc->DeltaBufferLevel  = prc->TargetBufferLevel / (prc->TotalFrames + max(0,min(150,(prc->TotalFrames - 150) / 3)));
		}
		else
			prc->TargetBufferLevel = prc->TargetBufferLevel - prc->DeltaBufferLevel;
	}
	else if (prc->IntraPeriod == 3 || prc->IntraPeriod == 2)
	{
		if(imgtype==0 && prc->CodedFrameNumber<2)
		{
			prc->FirstBufferSizeLevel = prc->CurrentBufferSizeBpp;
			prc->TargetBufferLevel = prc->CurrentBufferSizeBpp;
		}
		else
			prc->TargetBufferLevel = prc->FirstBufferSizeLevel * cos(PI / 2 * prc->CodedFrameNumber / prc->TotalFrames);
	}
	else
	{
		if(imgtype==0 && prc->CodedFrameNumber<2)
		{
			prc->TargetBufferLevel = prc->CurrentBufferSizeBpp;
			LevelLength = prc->CodedFrameNumber<2 ? (prc->IntraPeriod-1)*goplength : prc->IntraPeriod*goplength-1;
			prc->DeltaBufferLevel = prc->TargetBufferLevel/(prc->IntraPeriod==0 ? prc->TotalFrames : min(LevelLength,prc->TotalFrames-framenumber));
		}
		else if (imgtype==0 && prc->CodedFrameNumber>=2 && ((prc->TotalFrames - prc->CodedFrameNumber) > (prc->IntraPeriod * goplength + goplength)))
		{
			prc->GopFlag = prc->CodedFrameNumber;
			prc->TargetBufferLevel = prc->DeltaBufferLevel = 0.0;
		}
		else if (prc->GopFlag == prc->CodedFrameNumber - goplength)
		{
			prc->TargetBufferLevel = prc->CurrentBufferSizeBpp;
			if((prc->TotalFrames - prc->CodedFrameNumber) <= (prc->IntraPeriod * goplength))
				LevelLength = prc->TotalFrames-prc->CodedFrameNumber;
			else
				LevelLength = (prc->IntraPeriod-1)*goplength-1;
			prc->DeltaBufferLevel = prc->TargetBufferLevel / LevelLength;
			prc->GopFlag = -100;
		}
		else if (imgtype==0 && prc->CodedFrameNumber>=2 && ((prc->TotalFrames - prc->CodedFrameNumber) <= (prc->IntraPeriod * goplength + goplength)))
		{
			prc->TargetBufferLevel = prc->CurrentBufferSizeBpp;
			prc->DeltaBufferLevel = prc->TargetBufferLevel / (prc->TotalFrames - prc->CodedFrameNumber);
		}
		else
			prc->TargetBufferLevel = prc->TargetBufferLevel - prc->DeltaBufferLevel;
	}
}

int  CalculateGopDeltaQP_RateControl(RateControl * prc, int imgtype, int framenumber, int goplength)
{
	int deltaQP = 0;
	double BufferRange,deltaBufferRange;
	double Tbpp;
	prc->Belta = imgtype==0 ? 0.12F : 0.12F;

	prc->BufferError = prc->CurrentBufferSizeBpp - prc->TargetBufferLevel;

	if ((prc->CodedFrameNumber % goplength == 1) || (pRC->IntraPeriod == 1))
	{		
		prc->BufferDifError = prc->BufferError - prc->PreBufferError;
		prc->PreBufferError = prc->BufferError;
	}
	if(prc->IntraPeriod == 1)
	{
		Tbpp = prc->IntraFrameBpp;
		BufferRange = Tbpp * prc->Belta * 2;
		BufferRange = prc->CodedFrameNumber < 2 ? Tbpp * 4 : BufferRange;
		BufferRange = BufferRange<0.0001 ? 0.0001 : BufferRange;
		deltaBufferRange = BufferRange * 2;
		deltaQP = GetNewFrameDeltaQP_FuzzyController(prc->BufferError, prc->BufferDifError, BufferRange, -BufferRange, deltaBufferRange, -deltaBufferRange);
		prc->DeltaQP = deltaQP;
		return deltaQP;
	}

	if(prc->IntraPeriod == 0)
	{
		Tbpp = prc->InterFrameBpp;
		BufferRange = Tbpp * prc->Belta * 2;
		BufferRange = prc->CodedFrameNumber < 2 ? Tbpp * 4 : BufferRange;
		BufferRange = BufferRange<0.0001 ? 0.0001 : BufferRange;
		deltaBufferRange = BufferRange * 2;
		deltaQP = GetNewFrameDeltaQP_FuzzyController(prc->BufferError, prc->BufferDifError, BufferRange, -BufferRange, deltaBufferRange, -deltaBufferRange);
		prc->DeltaQP = deltaQP;
		return deltaQP;
	}

	if(prc->IntraPeriod > 1)
	{
		if(imgtype==0)
		{
			Tbpp = prc->GopAllKeyBpp;
			BufferRange = Tbpp * prc->Belta * 2;
			BufferRange = prc->CodedFrameNumber < 2 ? Tbpp * 4 : BufferRange;
			BufferRange = BufferRange<0.0001 ? 0.0001 : BufferRange;
			deltaBufferRange = BufferRange * 2;
			deltaQP = GetNewFrameDeltaQP_FuzzyController(prc->BufferError, prc->BufferDifError, BufferRange, -BufferRange, deltaBufferRange, -deltaBufferRange);
			prc->DeltaQP = deltaQP;
			return deltaQP;
		}
		else
		{
			Tbpp = prc->GopBpp;
			BufferRange = Tbpp * prc->Belta * 2;
			BufferRange = prc->CodedFrameNumber < 2 ? Tbpp * 4 : BufferRange;
			BufferRange = BufferRange<0.0001 ? 0.0001 : BufferRange;
			deltaBufferRange = BufferRange / 2;
			deltaQP = GetNewFrameDeltaQP_FuzzyController(prc->BufferError, prc->BufferDifError, BufferRange, -BufferRange, deltaBufferRange, -deltaBufferRange);
			prc->DeltaQP = deltaQP;
			return deltaQP;
		}
	}
	return 0; // never goes here
}

void Init_LCURateControl(RateControl * prc, int NumUnitsLCU)
{
	prc->prclcu->LCUTBL = prc->TargetBufferLevel;
	prc->prclcu->LCUBuffer = prc->CurrentBufferSizeBpp;
	prc->prclcu->LCUTBLdelta = prc->DeltaBufferLevel / NumUnitsLCU;
	prc->prclcu->LCUBufferError = 0;
	prc->prclcu->LCUBufferDifError = 0;
	prc->prclcu->LCUPreBufferError = 0;
}
int	 CalculateLCUDeltaQP(RateControl * prc)
{
	int deltaQP = 0;
	double BufferRange,deltaBufferRange;
	double Tbpp;
	prc->Belta = 0.12F;

	if(prc->IntraPeriod <= 1) // lcu level RC does not support RA now.
	{
		Tbpp = prc->TargetBitPerPixel;
		BufferRange = Tbpp * prc->Belta * 2;
		BufferRange = BufferRange<0.0001 ? 0.0001 : BufferRange;
		deltaBufferRange = BufferRange * 2;
		deltaQP = GetNewFrameDeltaQP_FuzzyController(prc->prclcu->LCUBufferError, prc->prclcu->LCUBufferDifError, BufferRange, -BufferRange, deltaBufferRange, -deltaBufferRange);
	}
	return deltaQP;
}
void UpdataLCURateControl(RateControl * prc, int qp, double lambda, int bits, int NumLCU)
{
	prc->prclcu->PreBpp = prc->prclcu->CurBpp;
	prc->prclcu->CurBpp = 1.0 * bits / (prc->FrameHeight * prc->FrameWidth);
	prc->prclcu->LCUTBL -= prc->prclcu->LCUTBLdelta;
	prc->prclcu->LCUBuffer = prc->prclcu->LCUBuffer + prc->prclcu->CurBpp - prc->TargetBitPerPixel / NumLCU;
	prc->prclcu->LCUBufferError = prc->prclcu->LCUBuffer - prc->prclcu->LCUTBL;
	prc->prclcu->LCUBufferDifError = prc->prclcu->LCUBufferError - prc->prclcu->LCUPreBufferError;
	prc->prclcu->LCUPreBufferError = prc->prclcu->LCUBufferError;
	prc->prclcu->LCUPreLCUQP = prc->prclcu->LCUCurLCUQP;
	prc->prclcu->LCUCurLCUQP = qp;
	prc->prclcu->LCUCurLambda= lambda;
	return;
}
