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

#ifndef _RATECONTRAL_H_
#define _RATECONTRAL_H_

#include <stdio.h>
#include <stdlib.h>

typedef struct RCLCU
{
	int		GlobeLCUNum;
	int		CurBits;
	int		PreBits;
	double	PreBpp;
	double	CurBpp;
	double	LCUTBL;
	double	LCUTBLdelta;
	double	LCUBuffer;
	double	LCUBufferError;
	double	LCUBufferDifError;
	double	LCUPreBufferError;
	int		LCUPreLCUQP;
	int		LCUCurLCUQP;
	int		LCUdeltaQP;
	double  LCUCurLambda;
}RCLCU;

typedef struct RateControl
{
	int		LCUbaseQP;
	int		RConoff;
	int		LCURConoff;
	int		IntraPeriod;
	int		TotalFrames;
	int		GlobalImageNumber;
	int		CodedFrameNumber;
	int		ImageType;
	int		ImageQP;
	int		DeltaQP;
	int		GopLeaderDeltaQP;
	int     RcQP;
	int		RcMBQP;
	int		SumMBQP;
	int		NumMB;
	int		ImageBits;
	int		FrameWidth;
	int		FrameHeight;
	double  ImageBpp;
	double  Belta;
	double  GopBpp;
	int     GopAvgBaseQP;
	int     GopAvgBaseQPCount;
	double  GopAllKeyBpp;
	double  FirstBufferSizeLevel;
	double	IntraFrameBpp;
	double	InterFrameBpp;
	double	TargetBitrate;
	double	TargetBitPerPixel;
	double	TargetBufferLevel;
	double	DeltaBufferLevel;
	double	CurrentBufferSizeBpp;
	double	BufferError;
	double	BufferDifError;
	double	PreBufferError;
	double	LowestBufferSizeBpp;
	double	HighestBufferSizeBpp;
	int     GopFlag;
	FILE    *FReportFrame;
	FILE	  *FReportLCU;
	RCLCU  	*prclcu;
}RateControl;

void Init_LCURateControl(RateControl * prc, int NumUnitsLCU);
int	 CalculateLCUDeltaQP(RateControl * prc);
void UpdataLCURateControl(RateControl * prc, int qp, double lambda, int bits, int NumLCU);
void Init_RateControl(RateControl * prc, int rconoff, int lcuonoff, int totalframes, int intraperiod, int targetbitrate, int framerate, int iniQP, int w, int h, char * filename);
void Updata_RateControl(RateControl * prc, int framebits, int frameqp, int imgtype, int framenumber, int goplength);
int  CalculateGopDeltaQP_RateControl(RateControl * prc, int imgtype, int framenumber, int goplength);
void Init_FuzzyController(double ScaFactor);
RateControl * pRC;
RCLCU		* pRCLCU;

#endif