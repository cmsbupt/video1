/*
***********************************************************************
* COPYRIGHT AND WARRANTY INFORMATION
*
* Copyright 2003, Advanced Audio Video Coding Standard, Part II
*
* DISCLAIMER OF WARRANTY
*
* These software programs are available to the users without any
* license fee or royalty on an "as is" basis. The AVS disclaims
* any and all warranties, whether express, implied, or statutory,
* including any implied warranties of merchantability or of fitness
* for a particular purpose. In no event shall the contributors or
* the AVS be liable for any incidental, punitive, or consequential
* damages of any kind whatsoever arising from the use of this program.
*
* This disclaimer of warranty extends to the user of this program
* and user's customers, employees, agents, transferees, successors,
* and assigns.
*
* The AVS does not represent or warrant that the program furnished
* hereunder are free of infringement of any third-party patents.
* Commercial implementations of AVS, including shareware, may be
* subject to royalty fees to patent holders. Information regarding
* the AVS patent policy is available from the AVS Web site at
* http://www.avs.org.cn
*
* THIS IS NOT A GRANT OF PATENT RIGHTS - SEE THE AVS PATENT POLICY.
************************************************************************
*/

/*
*************************************************************************************
* File name: wquant.h
* Function:  Frequency Weighting Quantization,include
*               a). Frequency weighting model, quantization
*               b). Picture level user-defined frequency weighting
*               c). Macroblock level adaptive frequency weighting mode Decision
*            According to adopted proposals: m1878,m2148,m2331
* Author:    Jianhua Zheng, Hisilicon
*            Xiaozhen Zheng,Hisilicon
*************************************************************************************
*/


#ifndef _WEIGHT_QUANT_H_
#define _WEIGHT_QUANT_H_


#define PARAM_NUM  6
#define WQ_MODEL_NUM 3

#define UNDETAILED 0
#define DETAILED   1

#define WQ_MODE_F  0  //M2331 2008-04
#define WQ_MODE_U  1  //M2331 2008-04
#define WQ_MODE_D  2  //M2331 2008-04



extern int ***LevelScale4x4;
extern int ***LevelScale8x8;
extern int ***LevelScale16x16;
extern int ***LevelScale32x32;

extern short UseDefaultScalingMatrixFlag[2];

extern short cur_wq_matrix[4][64];
extern short wq_matrix[2][2][64];
extern short seq_wq_matrix[2][64];
extern short pic_user_wq_matrix[2][64];

extern short wq_param[2][6];
extern short wq_param_default[2][6];

extern int WeightQuantEnable;



void Init_QMatrix (void);
void free_QMatrix(void);

int *GetDefaultWQM(int);

void InitFrameQuantParam();
void FrameUpdateWQMatrix();

void MBUpdateWQMatrix();             //M2331 2008-04


static const unsigned char WeightQuantModel[4][64]={
//   l a b c d h
//	 0 1 2 3 4 5
	{
     // Mode 0
		0,0,0,4,4,4,5,5,
		0,0,3,3,3,3,5,5,
		0,3,2,2,1,1,5,5,
		4,3,2,2,1,5,5,5,
		4,3,1,1,5,5,5,5,
		4,3,1,5,5,5,5,5,
		5,5,5,5,5,5,5,5,
		5,5,5,5,5,5,5,5},
	{
	  // Mode 1
		0,0,0,4,4,4,5,5,
		0,0,4,4,4,4,5,5,
		0,3,2,2,2,1,5,5,
		3,3,2,2,1,5,5,5,
		3,3,2,1,5,5,5,5,
		3,3,1,5,5,5,5,5,
		5,5,5,5,5,5,5,5,
		5,5,5,5,5,5,5,5},
	{
       // Mode 2
		0,0,0,4,4,3,5,5,
		0,0,4,4,3,2,5,5,
		0,4,4,3,2,1,5,5,
		4,4,3,2,1,5,5,5,
		4,3,2,1,5,5,5,5,
		3,2,1,5,5,5,5,5,
		5,5,5,5,5,5,5,5,
		5,5,5,5,5,5,5,5},
	{
		// Mode 3
		0,0,0,3,2,1,5,5,
		0,0,4,3,2,1,5,5,
		0,4,4,3,2,1,5,5,
		3,3,3,3,2,5,5,5,
		2,2,2,2,5,5,5,5,
		1,1,1,5,5,5,5,5,
		5,5,5,5,5,5,5,5,
		5,5,5,5,5,5,5,5}
};

static const unsigned char WeightQuantModel4x4[4][16]={
//   l a b c d h
//	 0 1 2 3 4 5
	{
     // Mode 0
		0, 4, 3, 5,
		4, 2, 1, 5,
		3, 1, 1, 5,
		5, 5, 5, 5},
	{
	  // Mode 1
		0, 4, 4, 5,
		3, 2, 2, 5,
		3, 2, 1, 5,
		5, 5, 5, 5},
	{
       // Mode 2
		0, 4, 3, 5,
		4, 3, 2, 5,
		3, 2, 1, 5,
		5, 5, 5, 5},
	{
		// Mode 3
		0, 3, 1, 5,
		3, 4, 2, 5,
		1, 2, 2, 5,
		5, 5, 5, 5}
};

#endif
