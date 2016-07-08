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
*               Li Zhang,		Xianguo Zhang, Long Zhao
******************************************************************************
*/



/*
*************************************************************************************
* File name:
* Function:
*
*************************************************************************************
*/

#include "../../lcommon/inc/contributors.h"
#include "../../lcommon/inc/commonStructures.h"
#include "../../lcommon/inc/commonVariables.h"
#include "../../lcommon/inc/defines.h"
#include "../../lcommon/inc/memalloc.h"
#include "global.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "memory.h"


#define max(a, b)                   ((a) > (b) ? (a) : (b))  //!< Macro returning max value 
#define min(a, b)                   ((a) < (b) ? (a) : (b))  //!< Macro returning min value 

/*
***************************************************************************
* Function:Background modeling using our proposed weight based running average algorithm
* Input:
* Output:
* Return: 
* Attention:
* Author:  Zhang Xianguo, Peking University, 2010.08
***************************************************************************
*/

#ifdef AVS2_S2_BGLONGTERM
#if EXTEND_BD
byte* imageData_current;
byte* imageData_next;
#else
unsigned char* imageData_current;
unsigned char* imageData_next;
#endif
float* bgmodel_bgbuffer;
float* bgmodel_bgbuffer_temp;
unsigned short* imageData_index;
unsigned char* weight;
int frameWidth;
int frameHeight;
int AverageNumber=0;
int threshold_before;
int g_bg_number;
int mark_train_over;
#if EXTEND_BD
byte* u_bgmodel_bgbuffer;
#else
unsigned char* u_bgmodel_bgbuffer;
#endif

//int global_mv_modeling=0;
#if EXTEND_BD
void bg_allocModel(byte * frame,int w,int h)
#else
void bg_allocModel(unsigned char * frame,int w,int h)
#endif
{
  int imgW=w;
  int imgH=h;
  int size_frame_int = imgW * imgH * 3/2 ;
  int size_y = imgW * imgH;
  int j;
  frameWidth = w;
  frameHeight= h;
  printf("alloc model");
  //////////////////FILE * LIST///////////////////
#if EXTEND_BD
  imageData_current = (byte*)malloc(size_frame_int * sizeof(byte));
  imageData_next = (byte*)malloc(size_frame_int * sizeof(byte));
#else
  imageData_current = (unsigned char*)malloc(size_frame_int * sizeof(unsigned char));
  imageData_next = (unsigned char*)malloc(size_frame_int * sizeof(unsigned char));
#endif
  if(input->bg_model_method<=1) {
	  bgmodel_bgbuffer  =  (float*)malloc(size_frame_int * sizeof(float));
	  bgmodel_bgbuffer_temp = (float*)malloc(size_frame_int * sizeof(float));
	  imageData_index = (unsigned short*)malloc(size_frame_int * sizeof(unsigned short));
	  weight = (unsigned char*)malloc(size_frame_int * sizeof(unsigned char));
#if EXTEND_BD
	  memcpy(imageData_current,frame,sizeof(byte) * size_frame_int);
	  memcpy(imageData_next,frame,sizeof(byte) * size_frame_int);
#else
      memcpy(imageData_current,frame,size_frame_int);
	  memcpy(imageData_next,frame,size_frame_int);
#endif
	  threshold_before=14;
	  g_bg_number=0;
	  for(j=0;j<size_frame_int;j++)
	  {
		bgmodel_bgbuffer[j] = 0;
		bgmodel_bgbuffer_temp[j] = imageData_current[j];
		imageData_index[j] = 0;
		weight[j] = 1;
		if(input->bg_model_method==0)
		{
		  bgmodel_bgbuffer[j]=imageData_current[j];
		  weight[j] = 1;
		}
	  }
  }
}

void bg_releaseModel()
{
	printf("release model");
    if(input->bg_model_method<=1) {
		free(bgmodel_bgbuffer);
		free(bgmodel_bgbuffer_temp);
		free(imageData_index);
		free(imageData_next);
		free(weight);
		free(imageData_current);
	}
	else
	{
		free(u_bgmodel_bgbuffer);
	}
}
#if EXTEND_BD
void bg_insert(byte * frame)
#else
void bg_insert(unsigned char * frame)
#endif
{
  int imgW=frameWidth;
  int imgH=frameHeight;
  int bg_number;
  int size_frame_int = imgW * imgH * 3/2 ;
  int size_y = imgW * imgH;
  int start,j,c,temp;
  float threshold;
  int diff_number=0;
#if EXTEND_BD
  byte * imageData=imageData_current;
#else
  unsigned char * imageData=imageData_current;
#endif
  int global_x=0;
  int global_y=0;
  threshold=0;
  bg_number=0;
  //if(global_mv_modeling)
  //{
	 // int sum_global_mv=0;
	 // int i;
	 // int best_x=0;
	 // int best_y=0;
	 // Macroblock    *currMB;
	 // for(i=0;i<32;i++)
	 // {
		//  mvx[i]=0;
		//  mvy[i]=0;
	 // }
	 // mvx[16]=1;
	 // mvy[16]=1;
	 // for(j=0;j<size_y;j++){
		//  int b8_y;
		//  int b8_x;
		//  int mb_x;
		//  int mb_y;
		//  int index=j;
		//  int width=frameWidth;
		//  mb_x=index%width/16*16;
		//  mb_y=index/width/16*16;
		//  b8_y=(index/width-mb_y)/8;
		//  b8_x=(index%width-mb_x)/8;
		//  currMB    = &img->mb_data[mb_x/16+mb_y/16*frameWidth/16];
		//  if(currMB->mb_type!=I8MB || input->usefme==0){
		//	  int value_x=all_mincost[mb_x/4+b8_x][mb_y/4+b8_y][0][4][1]/4;
		//	  int value_y=all_mincost[mb_x/4+b8_x][mb_y/4+b8_y][0][4][2]/4;
		//	  if(abs(value_x)+abs(value_y)<16)
		//	  {
		//		  mvx[value_x+16]++;
		//		  mvy[value_y+16]++;
		//	  }
		//  }
	 // }
	 // for(i=0;i<32;i++)
	 // {
		//  if(mvx[i]>mvx[best_x])
		//	  best_x=i;
		//  if(mvy[i]>mvy[best_y])
		//	  best_y=i;
	 // }
	 // global_x=best_x-16;
	 // global_y=best_y-16;

	 // printf("global_mv,global_x=%d,global_y=%d\n",global_x,global_y);
  //}
  if(input->bg_model_method<=1)
  {
	  //////////////////FILE * LIST///////////////////
#if EXTEND_BD
	  memcpy(imageData,imageData_next,sizeof(byte) * size_frame_int);
	  memcpy(imageData_next,frame,sizeof(byte) * size_frame_int);
#else
      memcpy(imageData,imageData_next,size_frame_int);
	  memcpy(imageData_next,frame,size_frame_int);
#endif

	  if(input->bg_model_method==1)
	  {
		for(j=0;j<size_frame_int;j++)
		{
		  int p_x;
		  int p_y;
		  int pos;
		  if(j<size_y){
			  int index=j;
			  int width=frameWidth;
			  p_x=index%width+global_x;
			  if(p_x>=width) 
				  p_x=width-1;
			  if(p_x<0) 
				  p_x=0;
			  p_y=index/width+global_y;
			  if(p_y>=frameHeight)
				  p_y=frameHeight-1;
			  if(p_y<0) 
				  p_y=0;
			  pos=p_y*width+p_x;
		  }
		  else if((j-size_y)<size_y/4){
			  int index=j-size_y;
			  int width=frameWidth/2;
			  p_x=index%width+global_x/2;
			  if(p_x>=width) 
				  p_x=width-1;
			  if(p_x<0) 
				  p_x=0;

			  p_y=index/width+global_y/2;
			  if(p_y>=frameHeight/2)
				  p_y=frameHeight/2-1;
			  if(p_y<0) 
				  p_y=0;

			  pos=size_y+p_y*width+p_x;
		  }
		  else{
			  int index=j-size_y-size_y/4;
			  int width=frameWidth/2;
			  p_x=index%width+global_x/2;
			  if(p_x>=width) 
				  p_x=width-1;
			  if(p_x<0) 
				  p_x=0;

			  p_y=index/width+global_y/2;
			  if(p_y>=frameHeight/2)
				  p_y=frameHeight/2-1;
			  if(p_y<0) 
				  p_y=0;

			  pos=size_y*5/4+p_y*width+p_x;
		  }
		  if((temp=abs(imageData_next[j] - imageData[pos])) < 30 && temp!=0)
		  {
			threshold = (diff_number*threshold + temp*temp)/((diff_number + 1)*1.0);
			diff_number++;
		  }

		  if((abs(imageData_next[j] - (imageData_index[j]?bgmodel_bgbuffer[pos]:bgmodel_bgbuffer_temp[pos]))) < 14 && j<size_y)
		  {
			bg_number++;
		  }	
		  c=threshold_before;
		  if(abs(imageData[pos] - imageData_next[j]) > c)
		  {
			if(weight[pos]>=input->bg_model_number/20)
			{
			  start=weight[pos]*(weight[pos]);
			  bgmodel_bgbuffer[pos] = (bgmodel_bgbuffer[pos] * (imageData_index[pos]) + start * bgmodel_bgbuffer_temp[pos])/(float)(imageData_index[pos] + start);
			  imageData_index[pos] += start;
			}
			bgmodel_bgbuffer_temp[pos]= imageData_next[j];
			weight[pos]=1;
		  }
		  else
		  {
			weight[pos]++;
			bgmodel_bgbuffer_temp[pos] = (bgmodel_bgbuffer_temp[pos] * (weight[pos]-1) + imageData_next[j])/(float)weight[pos];
		  }		
        
		}
	  }
	  else
	  {
		for(j=0;j<size_frame_int;j++)
		{
		  if((temp=abs(imageData_next[j] - imageData[j])) < 30 && temp!=0)
		  {
			threshold = (diff_number*threshold + temp*temp)/((diff_number + 1)*1.0);
			diff_number++;
		  }
		  if((abs(imageData_next[j] - bgmodel_bgbuffer[j])) < 14 && j<size_y)
		  {
			bg_number++;
		  }	
		  bgmodel_bgbuffer[j] = (bgmodel_bgbuffer[j]*weight[j] + imageData_next[j])/(weight[j]+1);
		  if(weight[j]>255)
			weight[j]=128;
		  weight[j]++;
		}
	  }
//	  if(bg_number > size_y/2)
		g_bg_number++;
	  threshold_before=min(15,(int)sqrt((double)threshold))*2;
	  
      //fwrite(bgmodel_bgbuffer,sizeof(unsigned char),size_frame_int,Fp1);
	  //printf("threshold_before=%d",threshold_before);
  }
}
#if EXTEND_BD
void bg_build(byte * imageData)
#else
void bg_build(unsigned char * imageData)
#endif
{
  int imgW=frameWidth;
  int imgH=frameHeight;
  int size_frame_int = imgW * imgH * 3/2 ;
  int start,j;
  float temp;
  if(input->bg_model_method==1)
  {
    for(j=0;j<size_frame_int;j++)
    {
      temp = bgmodel_bgbuffer[j];
      start=weight[j]*(weight[j]);
      if(start==0 && imageData_index[j]==0)
      {
        bgmodel_bgbuffer[j] = imageData_next[j];
      }
      else
      {
        bgmodel_bgbuffer[j] = (bgmodel_bgbuffer[j] * (imageData_index[j]) + 
          start * bgmodel_bgbuffer_temp[j])/(float)(imageData_index[j] + start);
      }
#if EXTEND_BD
      if(bgmodel_bgbuffer[j]+0.5<((1 << input->sample_bit_depth) - 1))
        imageData[j] = (byte)(bgmodel_bgbuffer[j] + 0.5);
      else
        imageData[j] = (1 << input->sample_bit_depth) - 1;
#else
      if(bgmodel_bgbuffer[j]+0.5<255)
        imageData[j] = (unsigned char)(bgmodel_bgbuffer[j] + 0.5);
      else
        imageData[j] = 255;
#endif
      bgmodel_bgbuffer[j]=temp;
    }
  }
  else if(input->bg_model_method==0)
  {
    for(j=0;j<size_frame_int;j++)
    {
#if EXTEND_BD
      if(bgmodel_bgbuffer[j]+0.5<((1 << input->sample_bit_depth) - 1))
        imageData[j] = (byte)(bgmodel_bgbuffer[j] + 0.5);
      else
        imageData[j] = (1 << input->sample_bit_depth) - 1;
#else
      if(bgmodel_bgbuffer[j]+0.5<255)
        imageData[j] = (unsigned char)(bgmodel_bgbuffer[j] + 0.5);
      else
        imageData[j] = 255;
#endif
    }
  }
  else if(input->bg_model_method==2)
  {
    for(j=0;j<size_frame_int;j++)
    {
		imageData[j] = ((u_bgmodel_bgbuffer[j]*g_bg_number)+imageData[j]+(g_bg_number+1)/2)/(g_bg_number+1);
    }
  }
  else
  {
	  for(j=0;j<size_frame_int;j++)
	  {
		  //assert(u_bgmodel_bgbuffer[j]!=0);
		  if(u_bgmodel_bgbuffer[j]==0)
			  u_bgmodel_bgbuffer[j]=imageData[j];
		  imageData[j]=u_bgmodel_bgbuffer[j];
	  }

  }
}
#endif
