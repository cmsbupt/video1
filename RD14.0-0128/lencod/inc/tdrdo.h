/*
*****************************************************************************
* COPYRIGHT AND WARRANTY INFORMATION
* Copyright 2014-2015
* Contributors: Yimin ZHOU, yiminzhou@uestc.edu.cn
*               Ce ZHU,     eczhu@uestc.edu.cn
*               Yanbo GAO,
*               Shuai LI
* Institution:  University of Electronic Science and Technology of China
******************************************************************************
*/

#include "../../lcommon/inc/commonStructures.h"

#ifndef _TDRDO_H_
#define _TDRDO_H_

#define MAXBLOCKSIZE  64
#define WORKBLOCKSIZE 64
#define SEARCHRANGE   64

typedef struct Block
{
	unsigned int	BlockWidth;
	unsigned int	BlockHeight;
	unsigned int	OriginX;
	unsigned int	OriginY;
	byte			lume[MAXBLOCKSIZE*MAXBLOCKSIZE];	// 4096==64*64
	byte			cr[MAXBLOCKSIZE*MAXBLOCKSIZE/4];	// 1024==4096/4
	byte			cb[MAXBLOCKSIZE*MAXBLOCKSIZE/4];	// 1024==4096/4
}Block;

typedef struct Frame
{
	unsigned int    FrameWidth;
	unsigned int    FrameHeight;
	unsigned int    nStrideY;
	unsigned int    nStrideC;
	byte	* base;
	byte	* Y;
	byte	* U;
	byte	* V;
}Frame;

typedef struct BlockDistortion
{
	unsigned int	GlobalBlockNumber;
	unsigned short	BlockNumInHeight;
	unsigned short	BlockNumInWidth;
	unsigned short	BlockWidth;
	unsigned short	BlockHeight;
	unsigned short	OriginX;
	unsigned short	OriginY;
	unsigned short	SearchRange;
	short			MVx;
	short			MVy;
	double			MSE;
	double			MAD;
	double			MVL;
	short			BlockQP;
	double			BlockLambda;
	short			BlockType;
}BlockDistortion,BD;

typedef struct FrameDistortion
{
	unsigned int	FrameNumber;
	unsigned int	BlockSize;
	unsigned int	CUSize;
	unsigned int	TotalNumOfBlocks;
	unsigned int	TotalBlockNumInHeight;
	unsigned int	TotalBlockNumInWidth;
	BD				*BlockDistortionArray;
	struct FrameDistortion *subFrameDistortionArray; 
}FrameDistortion,FD;

typedef struct DistortionList
{
	unsigned int	TotalFrameNumber;
	unsigned int	FrameWidth;
	unsigned int	FrameHeight;
	unsigned int	BlockSize;
	FD				*FrameDistortionArray;
}DistortionList,DL;

DL * CreatDistortionList(unsigned int totalframenumber, unsigned int w, unsigned int h, unsigned int blocksize, unsigned int cusize);
void DestroyDistortionList(DL * SeqD);
void MotionDistortion(FD *currentFD, Frame * FA, Frame * FB, unsigned int searchrange);
void StoreLCUInf(FD *curRealFD, int LeaderBlockNumber, int cuinwidth, int iqp, double dlambda, int curtype);
void CaculateKappaTableLDP(DL *omcplist, DL *realDlist, int keyframenum, int FrameQP);
void AdjustLcuQPLambdaLDP(FD * curOMCPFD, int LeaderBlockNumber, int cuinwidth, int *pQP, double *plambda);
FD *SearchFrameDistortionArray(DL *omcplist,int FrameNumberInFile, int StepLength, int IntraPeriod);

Frame *porgF,*ppreF,*precF,*prefF;

DL *OMCPDList;
DL *RealDList;
FD *pOMCPFD, *pRealFD,*subpOMCPFD ;

int		StepLength;
double	AvgBlockMSE;
double  *KappaTable;
double  *KappaTable1;
double  GlobeLambdaRatio;
int		GlobeFrameNumber;
int		CurMBQP;
int		QpOffset[32];
int		GroupSize;

#if AQPO
int		AQPStepLength;
double	LogMAD[32];
double	preLogMAD[32];
int		AQPoffset[32];
int		MaxQPoffset;
int		OffsetIndex;
int		FNIndex;
#if AQPOM3762
int     GopQpbase;
#else
int     subGopqpbase;
#endif
int     preGopQPF[4];
#endif

#endif
