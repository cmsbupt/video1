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
*************************************************************************************
*/

#include "memalloc.h"
#include "wquant.h"
#include "../../lcommon/inc/commonStructures.h"

#define max(a, b)                   ((a) > (b) ? (a) : (b))  //!< Macro returning max value 
#define min(a, b)                   ((a) < (b) ? (a) : (b))  //!< Macro returning min value 

int ***LevelScale4x4;
int ***LevelScale8x8;
int ***LevelScale16x16;
int ***LevelScale32x32;

int WeightQuantEnable;

short UseDefaultScalingMatrixFlag[2];


short UseDefaultScalingMatrixFlag[2];

#if !WQ_MATRIX_FCD
short wq_param_default[2][6]=
{
  {135,143,143,160,160,213},
  {128, 98,106,116,116,128}
};
#else
short wq_param_default[2][6]=
{
	{67,71,71,80,80,106},
	{64,49,53,58,58,64}
};
#endif

short cur_wq_matrix[4][64]; //cur_wq_matrix[matrix_id][coef]
short wq_matrix[2][2][64];  //wq_matrix[matrix_id][detail/undetail][coef]
short seq_wq_matrix[2][64];
short pic_user_wq_matrix[2][64];

short wq_param[2][6];

#if !WQ_MATRIX_FCD
int g_WqMDefault4x4[16] = {
  16, 	16, 	16, 	17,
  16, 	16, 	17, 	18,
  16, 	17, 	19, 	20,
  18, 	19, 	21, 	24
};


int g_WqMDefault8x8[64] = {
  16, 	16, 	16, 	16, 	17, 	17, 	18, 	19,
  16, 	16, 	16, 	17, 	18, 	19, 	21, 	23,
  16, 	16, 	17, 	18, 	19, 	20, 	22, 	25,
  16, 	17, 	18, 	20, 	21, 	23, 	25, 	28,
  17, 	18, 	20, 	21, 	23, 	26, 	28, 	32,
  19, 	20, 	21, 	23, 	26, 	29, 	33, 	38,
  24, 	25, 	26, 	29, 	31, 	35, 	41, 	47,
  26, 	27, 	29, 	32, 	38, 	43, 	48, 	54
};
#else
int g_WqMDefault4x4[16] = {
	64, 	64, 	64, 	68,
	64, 	64, 	68, 	72,
	64, 	68, 	76, 	80,
	72, 	76, 	84, 	96
};


int g_WqMDefault8x8[64] = {
	64, 	64, 	64, 	64, 	68, 	68, 	72, 	76,
	64, 	64, 	64, 	68, 	72, 	76, 	84, 	92,
	64, 	64, 	68, 	72, 	76, 	80, 	88, 	100,
	64, 	68, 	72, 	80, 	84, 	92, 	100, 	112,
	68, 	72, 	80, 	84, 	92, 	104, 	112, 	128,
	76, 	80, 	84, 	92, 	104, 	116, 	132, 	152,
	96, 	100, 	104, 	116, 	124, 	140, 	164, 	188,
	104, 	108, 	116, 	128, 	152, 	172, 	192, 	216
};
#endif


/*
*************************************************************************
* Function: Allocate and initialise Q matrix values.
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void Init_QMatrix (void)
{
	get_mem3Dint(&LevelScale4x4, 2, 4, 4);
	get_mem3Dint(&LevelScale8x8, 2, 8, 8);
	get_mem3Dint(&LevelScale16x16, 2, 16, 16);
	get_mem3Dint(&LevelScale32x32, 2, 32, 32);
}


/*
*************************************************************************
* Function: Free Q matrix arrays.
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void free_QMatrix ()
{
	free_mem3Dint(LevelScale4x4, 2);
	free_mem3Dint(LevelScale8x8, 2);
	free_mem3Dint(LevelScale16x16, 2);
	free_mem3Dint(LevelScale32x32, 2);
}

int *GetDefaultWQM(int sizeId)
{
	int *src = 0;
	if (sizeId == 0) {
		src = g_WqMDefault4x4;
	}
	else if (sizeId == 1)
	{
		src = g_WqMDefault8x8;
	}
	return src;
}

void InitFrameQuantParam()
{
  int i,j,k;
  int uiWQMSizeId,uiBlockSize;

  if(weight_quant_enable_flag&&pic_weight_quant_enable_flag)
    WeightQuantEnable=1;
  else
    WeightQuantEnable=0;

  if(WeightQuantEnable)
  {
    UseDefaultScalingMatrixFlag[0]=UseDefaultScalingMatrixFlag[1]=0;  // use weighed scaling matrix
  }
  else
  {
    UseDefaultScalingMatrixFlag[0]=UseDefaultScalingMatrixFlag[1]=1;  // use default scaling matrix
  }

	for (uiWQMSizeId=0;uiWQMSizeId<4;uiWQMSizeId++)
		for (i=0;i<64;i++)
			cur_wq_matrix[uiWQMSizeId][i]=1<<WQ_FLATBASE_INBIT;

	if(!WeightQuantEnable)
	{
		for(uiWQMSizeId=0;uiWQMSizeId<2;uiWQMSizeId++)
		{
			uiBlockSize=1<<(uiWQMSizeId+2);
			for(k=0;k<2;k++)
				for(j=0;j<uiBlockSize;j++)
					for (i=0;i<uiBlockSize;i++)
						wq_matrix[uiWQMSizeId][k][j*uiBlockSize+i]=1<<WQ_FLATBASE_INBIT;
		}
	}
  else
  {
    for(i=0;i<2;i++)
      for(j=0;j<6;j++)
        wq_param[i][j]=1<<WQ_FLATBASE_INBIT;

    if(weighting_quant_param==0)
    {
      for(i=0;i<6;i++)
        wq_param[DETAILED][i]=wq_param_default[DETAILED][i];
    }
    else if(weighting_quant_param==1)
    {
      for(i=0;i<6;i++)
        wq_param[UNDETAILED][i]=quant_param_undetail[i];
    }
    if(weighting_quant_param==2)
    {
      for(i=0;i<6;i++)
        wq_param[DETAILED][i]=quant_param_detail[i];
    }
    // Reconstruct the Weighting matrix
    for(k=0;k<2;k++)
      for(j=0;j<8;j++)
        for(i=0;i<8;i++)
          wq_matrix[1][k][j*8+i]=(wq_param[k][WeightQuantModel[CurrentSceneModel][j*8+i]]);

      for(k=0;k<2;k++)
        for(j=0;j<4;j++)
          for(i=0;i<4;i++)
            wq_matrix[0][k][j*4+i]=(wq_param[k][WeightQuantModel4x4[CurrentSceneModel][j*4+i]]);
  }
}


void FrameUpdateWQMatrix()
{
  int i;
  int uiWQMSizeId, uiWMQId;
  int uiBlockSize;

  if(WeightQuantEnable)
  {
#if AWQ_LARGE_BLOCK_ENABLE
	   for(uiWQMSizeId=0;uiWQMSizeId<4;uiWQMSizeId++)
#else
	   for(uiWQMSizeId=0;uiWQMSizeId<2;uiWQMSizeId++)
#endif
	   {
		   uiBlockSize=min(1<<(uiWQMSizeId+2),8);
		   uiWMQId=(uiWQMSizeId<2)?uiWQMSizeId:1;
      if (pic_weight_quant_data_index == 0)
      {
        for(i=0;i<(uiBlockSize*uiBlockSize);i++) 
        {
          cur_wq_matrix[uiWQMSizeId][i]=seq_wq_matrix[uiWMQId][i];
        }
      } // pic_weight_quant_data_index == 0
	    else if( pic_weight_quant_data_index == 1)
      {
        if(weighting_quant_param==0)
        {
          for(i=0;i<(uiBlockSize*uiBlockSize);i++)
            cur_wq_matrix[uiWQMSizeId][i]=wq_matrix[uiWMQId][DETAILED][i]; // Detailed weighted matrix
        }
        else if(weighting_quant_param==1)
        {
          for(i=0;i<(uiBlockSize*uiBlockSize);i++)
            cur_wq_matrix[uiWQMSizeId][i]=wq_matrix[uiWMQId][0][i];  // unDetailed weighted matrix
        }
        if(weighting_quant_param==2)
        {
          for(i=0;i<(uiBlockSize*uiBlockSize);i++)
            cur_wq_matrix[uiWQMSizeId][i]=wq_matrix[uiWMQId][1][i]; // Detailed weighted matrix
        }
      } // pic_weight_quant_data_index == 1
	    else if( pic_weight_quant_data_index == 2)
      {
        for(i=0;i<(uiBlockSize*uiBlockSize);i++) 
        {
          cur_wq_matrix[uiWQMSizeId][i]=pic_user_wq_matrix[uiWMQId][i];
        }
      } //pic_weight_quant_data_index == 2
	  }
  }

#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
#if AWQ_LARGE_BLOCK_ENABLE
  for (uiWQMSizeId = 0; uiWQMSizeId<2; uiWQMSizeId++)
#else
  for (uiWQMSizeId = 0; uiWQMSizeId < 2; uiWQMSizeId++)
#endif
  {
    uiBlockSize = min(1 << (uiWQMSizeId + 2), 8); 
    for (i = 0; i<(uiBlockSize*uiBlockSize); i++)
    {
      cur_wq_matrix[uiWQMSizeId][i] = seq_wq_matrix[uiWMQId][i];
      //fprintf(p_pps, "cur_wq_matrix[%2d] %08X\n",i, cur_wq_matrix[uiWQMSizeId][i]);
    }
    if (uiBlockSize == 4)
    {
      for (i = 0; i<(uiBlockSize*uiBlockSize); i++)
      {
        pps_WeightQuantMatrix4x4[i / 4][i % 4] = cur_wq_matrix[uiWQMSizeId][i];
        //fprintf(p_pps, "cur_wq_matrix[%2d] %08X\n", i, cur_wq_matrix[uiWQMSizeId][i]);
      }
      
    }
    else if(uiBlockSize == 8)
    {      
      for (i = 0; i<(uiBlockSize*uiBlockSize); i++)
      {
        pps_WeightQuantMatrix8x8[i / 8][i % 8] = cur_wq_matrix[uiWQMSizeId][i];
        //fprintf(p_pps, "cur_wq_matrix[%2d] %08X\n", i, cur_wq_matrix[uiWQMSizeId][i]);
      }
    }
  }
#endif
}

