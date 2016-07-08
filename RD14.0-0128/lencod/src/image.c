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




/*
*************************************************************************************
* File name:
* Function:
*
*************************************************************************************
*/
#include "../../lcommon/inc/contributors.h"

#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/timeb.h>
#include <string.h>
#include <memory.h>
#include <assert.h>
#ifdef WIN32
#include <IO.H>
#endif

#include "global.h"
#include "../../lcommon/inc/commonVariables.h"
#include "image.h"
#include "refbuf.h"
#include "header.h"
#include "../../lcommon/inc/memalloc.h"
#include "bitstream.h"
#include "vlc.h"
#include "configfile.h"
#include "AEC.h" 

#if FREQUENCY_WEIGHTING_QUANTIZATION
#include "wquant.h"
#endif

#if TDRDO
#include "tdrdo.h"
#endif

#if RATECONTROL
#include "ratecontrol.h"
#endif

#ifdef MD5
extern unsigned int   MD5val[4];
extern char		      MD5str[33];
#endif

#include "pos_info.h" //ZHAOHAIWU

#define TO_SAVE 4711
#define FROM_SAVE 4712
#define Clip(min,max,val) (((val)<(min))?(min):(((val)>(max))?(max):(val)))

static void code_a_picture ( Picture *frame );
#if ENC_FRAME_SKIP
static void ReadOneFrame ( int FrameNoInFile, int FrameSkip,int xs, int ys );
#else
static void ReadOneFrame ( int FrameNoInFile, int HeaderSize, int xs, int ys );
#endif

static void write_reconstructed_image();
static int  writeout_picture();
static int  writeout_slice();
static void find_snr();
static void init_frame();

int terminate_picture();

void copy_Pframe ();
void write_prev_Pframe (); 

void put_buffer_frame();
void interpolate_frame_to_fb();

static void CopyFrameToOldImgOrgVariables ();
static void UnifiedOneForthPix ( pel_t ** imgY, pel_t ** imgU, pel_t ** imgV,
                                 pel_t ** out4Y );
static void ReportFirstframe ( int tmp_time );
//static void ReportIntra ( int tmp_time );
//static void ReportP ( int tmp_time );
//static void ReportB ( int tmp_time );
static void Report_frame ( int tmp_time );

static int CalculateFrameNumber();  // Calculates the next frame number
static int FrameNumberInFile;       // The current frame number in the input file

extern void DecideMvRange();  // mv_range, 20071009

#define  IClip( Min, Max, Val) (((Val)<(Min))? (Min):(((Val)>(Max))? (Max):(Val)))



/*
*************************************************************************
* Function:
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

static void picture_header()
{
  int len = 0;

#if DEMULATE_FIX
  current_slice_bytepos = currBitStream->byte_pos;
  current_slice_bytepos_alf = current_slice_bytepos;
#endif

  if ( img->type == INTRA_IMG )
  {
    len = len + IPictureHeader ( img->number );
  }
  else
  {
    len = len + PBPictureHeader();
  }

#if DEMULATE_FIX
  // Bug fix
  if (input->alf_enable)
  {
	  BitStreamCopy(currBitStream_ALF,currBitStream);
  }
  demulateFunction();
#endif

  // Update statistics
  stat->bit_slice += len;
  stat->bit_use_header[img->type] += len;

}
void picture_header_ALF(ALFParam** alfPictureParam)
{
	int len = 0,compIdx;
	
	if ( input->alf_enable)
	{
		len += u_v ( 1, "alf_pic_flag_Y", img->pic_alf_on[0], currBitStream );
		len += u_v ( 1, "alf_pic_flag_Cb", img->pic_alf_on[1], currBitStream );
		len += u_v ( 1, "alf_pic_flag_Cr", img->pic_alf_on[2], currBitStream );
	}


	if (img->pic_alf_on[0] || img->pic_alf_on[1] || img->pic_alf_on[2])
	{
		for (compIdx=0;compIdx<NUM_ALF_COMPONENT;compIdx++)
		{
			if (img->pic_alf_on[compIdx])
			{
				writeAlfCoeff(alfPictureParam[compIdx]);
			}
		}
	}

	stat->bit_slice += len;
	stat->bit_use_header[img->type] += len;
}

void report_frame( int tmp_time )
{
#if MD5
	if(input->MD5Enable & 0x02)
	{
		int j,k;
		int img_width = ( img->width - img->auto_crop_right );
		int img_height = ( img->height - img->auto_crop_bottom );
		int img_width_cr = ( img_width / 2 );
		int img_height_cr = ( img_height / ( input->chroma_format == 1 ? 2 : 1 ) );
		int nSampleSize = input->input_sample_bit_depth == 8 ? 1 : 2;
		int shift1 = input->sample_bit_depth - input->input_sample_bit_depth;
		unsigned char * pbuf;
		unsigned char * md5buf;
		md5buf = (unsigned char *)malloc(img_height*img_width*nSampleSize + img_height_cr*img_width_cr*nSampleSize*2);

		if(md5buf!=NULL)
		{
			if (!shift1 && input->input_sample_bit_depth == 8) // 8bit input -> 8bit encode
			{
				pbuf = md5buf;
				for (j = 0; j < img_height; j++)
					for (k = 0; k < img_width; k++)
						pbuf[j * img_width + k] = (unsigned char)imgY[j][k];

				pbuf = md5buf + img_height*img_width*sizeof(unsigned char);
				for ( j = 0; j < img_height_cr; j++ )
					for (k = 0; k < img_width_cr; k++)
						pbuf[j * img_width_cr + k] = (unsigned char)imgUV[0][j][k];

				pbuf = md5buf + img_height*img_width*sizeof(unsigned char) + img_height_cr*img_width_cr*sizeof(unsigned char);
				for ( j = 0; j < img_height_cr; j++ )
					for (k = 0; k < img_width_cr; k++)
						pbuf[j * img_width_cr + k] = (unsigned char)imgUV[1][j][k];
			}
			else if (!shift1 && input->input_sample_bit_depth > 8) // 10/12bit input -> 10/12bit encode
			{
				pbuf = md5buf;
				for (j = 0; j < img_height; j++)
					memcpy ( pbuf + j * img_width * nSampleSize, & ( imgY[j][0] ), img_width * sizeof(byte) );

				pbuf = md5buf + img_height*img_width*sizeof(byte);
				for ( j = 0; j < img_height_cr; j++ )
					memcpy ( pbuf + j * img_width_cr * nSampleSize, & ( imgUV[0][j][0] ), img_width_cr * sizeof(byte) );

				pbuf = md5buf + img_height*img_width*sizeof(byte) + img_height_cr*img_width_cr*sizeof(byte);
				for ( j = 0; j < img_height_cr; j++ )
					memcpy ( pbuf + j * img_width_cr * nSampleSize, & ( imgUV[1][j][0] ), img_width_cr * sizeof(byte) );
			}
			else if (shift1 && input->input_sample_bit_depth == 8) // 8bit input -> 10/12bit encode
			{
				pbuf = md5buf;
				for (j = 0; j < img_height; j++)
					for (k = 0; k < img_width; k++)
						pbuf[j * img_width + k] = (unsigned char)Clip1((imgY[j][k] + (1 << (shift1 - 1))) >> shift1);

				pbuf = md5buf + img_height*img_width*sizeof(unsigned char);
				for ( j = 0; j < img_height_cr; j++ )
					for (k = 0; k < img_width_cr; k++)
						pbuf[j * img_width_cr + k] = (unsigned char)Clip1((imgUV[0][j][k] + (1 << (shift1 - 1))) >> shift1);

				pbuf = md5buf + img_height*img_width*sizeof(unsigned char) + img_height_cr*img_width_cr*sizeof(unsigned char);
				for ( j = 0; j < img_height_cr; j++ )
					for (k = 0; k < img_width_cr; k++)
						pbuf[j * img_width_cr + k] = (unsigned char)Clip1((imgUV[1][j][k] + (1 << (shift1 - 1))) >> shift1);
			}
			BufferMD5(md5buf, img_height*img_width*nSampleSize + img_height_cr*img_width_cr*nSampleSize*2, MD5val);
		}
		else
		{
			printf("malloc md5 buffer error!\n");
			memset(MD5val,0,16);
		}
		if(md5buf)
			free(md5buf);
		sprintf(MD5str,"%08X%08X%08X%08X\0",MD5val[0],MD5val[1],MD5val[2],MD5val[3]);
	}
	else
	{
		memset(MD5val,0,16);
		memset(MD5str,0,33);
	}
#endif
	if ( img->number == 0 )
	{
		ReportFirstframe ( tmp_time );
	}
	else
	{

		switch(img->type)
		{
		case INTRA_IMG: 
#if AVS2_SCENE_STAT_DUP_GB
			if (input->bg_enable && img->typeb == BACKGROUND_IMG && duplicated_gb_flag == 1){
				stat->bit_ctr_dup_gb += stat->bit_ctr - stat->bit_ctr_n;
			}
#endif
			stat->bit_ctr_0 += stat->bit_ctr - stat->bit_ctr_n;
			break;
		case B_IMG:
			stat->bit_ctr_B += stat->bit_ctr - stat->bit_ctr_n;
			break;
		default:
			stat->bit_ctr_P += stat->bit_ctr - stat->bit_ctr_n;
			break;
		}
		Report_frame ( tmp_time );
	}
	fflush(stdout);
}

/*
*************************************************************************
* Function:Encodes one frame
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

int encode_one_frame ()
{
  time_t ltime1;
  time_t ltime2;
#ifdef WIN32
  struct _timeb tstruct1;
  struct _timeb tstruct2;
#else
  struct timeb tstruct1;
  struct timeb tstruct2;
#endif
  int tmp_time;

#ifdef WIN32
  _ftime ( &tstruct1 );         // start time ms
#else
  ftime ( &tstruct1 );
#endif
  time ( &ltime1 );             // start time s

  // Adaptive frequency weighting quantization
#if FREQUENCY_WEIGHTING_QUANTIZATION
  if((input->WQEnable)&&(input->PicWQEnable))
  {
	  InitFrameQuantParam();
	  FrameUpdateWQMatrix();		//M2148 2007-09
  }
#endif

#ifdef AVS2_S2_BGLONGTERM
  if(img->typeb!=BACKGROUND_IMG || input->bg_input_number!=0) //I or P or output G
  {
	  init_frame();
	  FrameNumberInFile = CalculateFrameNumber();
#if ENC_FRAME_SKIP
    ReadOneFrame ( FrameNumberInFile, input->infile_frameskip,input->img_width, input->img_height ); //modify by wuzhongmou 0610
#else
    ReadOneFrame ( FrameNumberInFile, input->infile_header, input->img_width, input->img_height ); //modify by wuzhongmou 0610
#endif

#if AVS2_SCENE_GB_CD
	  if(img->typeb == BACKGROUND_IMG && background_output_flag == 1)
	  {
		  gb_is_ready = 0;
		  memcpy(imgY_GB_org_buffer, imgY_org_buffer, sizeof(byte)*img->width*img->height*3/2);  //back up the org GB
	  }
#endif
  }

#if AVS2_SCENE_CD
 if(img->typeb==BACKGROUND_IMG && input->bg_input_number==0 && img->number!=0 && input->always_no_bgmodel == 0)	//not output G
#else
 if(img->typeb==BACKGROUND_IMG && input->bg_input_number==0 && img->number!=0 && input->profile_id == 0x50 && input->always_no_bgmodel == 0)	//not output G
#endif
  {
	  if(g_bg_number >= input->bg_model_number/2
#if AVS2_SCENE_GB_CD
		  || duplicated_gb_flag == 1
#endif
		  )
	  {
		  init_frame();
		  FrameNumberInFile=CalculateFrameNumber();
#if AVS2_SCENE_GB_CD	
		  if(duplicated_gb_flag == 1)
		  {
			  memcpy(imgY_org_buffer, imgY_GB_org_buffer, sizeof(byte)*img->width*img->height*3/2);  //encode the duplicated GB
		  }
		  else
		  {
#endif
		  bg_build(imgY_org_buffer);
#if AVS2_SCENE_GB_CD
		  memcpy(imgY_GB_org_buffer, imgY_org_buffer, sizeof(byte)*img->width*img->height*3/2);  //back up the org GB
		  gb_is_ready = 1;
		  }
#endif
#if AVS2_S2_FASTMODEDECISION
		  if (input->bg_input_number==1)
		  {
			  bg_flag=1;
		  }
#endif
#if AVS2_SCENE_GB_CD
		  if(duplicated_gb_flag == 0)
		  {
#endif
		  last_input_frame=FrameNumberInFile;		
		  bg_releaseModel();
		  train_start=0;						
		  printf("\ntrain close g_bg_number=%d\n",g_bg_number);
#if AVS2_SCENE_GB_CD
		  }
#endif
		 
#if AVS2_SCENE_CD
		  if(p_org_background)
		  {
			  write_GB_org_frame(p_org_background);
		  }
#else
		   if(fp_background){
#if EXTEND_BD
			  fwrite(imgY_org_buffer,sizeof(byte),input->img_width*input->img_height*3/2,fp_background);
#else
			  fwrite(imgY_org_buffer,sizeof(unsigned char),input->img_width*input->img_height*3/2,fp_background);
#endif
			  fflush(fp_background);
		   }
#endif //AVS2_SCENE_CD
		  
	  }
	  else
	  {
		  printf("\nnot surveillance g_bg_number=%d\n",g_bg_number);
		  bg_releaseModel();
		  input->always_no_bgmodel=1;
		  input->bg_input_number=1;
		  background_number=0;
		  background_output_flag=1;
		  init_frame();
		  FrameNumberInFile=CalculateFrameNumber();
	  }
  }
#else
  init_frame ();           // initial frame variables

  FrameNumberInFile = CalculateFrameNumber();

#if ENC_FRAME_SKIP
  ReadOneFrame ( FrameNumberInFile, input->infile_frameskip,input->img_width, input->img_height ); //modify by wuzhongmou 0610
#else
  ReadOneFrame ( FrameNumberInFile, input->infile_header, input->img_width, input->img_height ); //modify by wuzhongmou 0610
#endif

#endif

#if TDRDO
  if(input->TDEnable!=0)
  {
    GlobeFrameNumber = img->number; //FrameNumberInFile;
#if AQPO	
	if(input->AQPEnable!=0 && !input->successive_Bframe )
	{
		FNIndex = GlobeFrameNumber % AQPStepLength;
		if(FNIndex==0)
			FNIndex = AQPStepLength-1;
		else
			FNIndex = FNIndex-1;
		if(GlobeFrameNumber>0)
		{
			preLogMAD[FNIndex] = LogMAD[FNIndex];
		}
	}
#endif
    if(input->successive_Bframe_all)
      pRealFD = &RealDList->FrameDistortionArray[GlobeFrameNumber];
    else
      pRealFD = &RealDList->FrameDistortionArray[globenumber];
    pRealFD->BlockDistortionArray = (BD *)calloc(pRealFD->TotalNumOfBlocks,sizeof(BD)); 
    if(GlobeFrameNumber % StepLength == 0)
    {
      if(FrameNumberInFile==0)
      {  
        porgF->base = imgY_org_buffer;
        porgF->Y = porgF->base;
        porgF->U = porgF->Y + porgF->FrameWidth * porgF->FrameHeight;
        porgF->V = porgF->U + porgF->FrameWidth * porgF->FrameHeight / 4 ;
        ppreF->base = imgY_pre_buffer;
        ppreF->Y = ppreF->base;
        ppreF->U = ppreF->Y + ppreF->FrameWidth * ppreF->FrameHeight;
        ppreF->V = ppreF->U + ppreF->FrameWidth * ppreF->FrameHeight / 4 ;
        memcpy(imgY_pre_buffer,imgY_org_buffer,input->img_width*input->img_height*3/2*sizeof(byte));
      }
      else  if(FrameNumberInFile<input->no_frames)
      {
        pOMCPFD = &OMCPDList->FrameDistortionArray[GlobeFrameNumber-1]; //FrameNumberInFile-1
        pOMCPFD->BlockDistortionArray = (BD *)calloc(pOMCPFD->TotalNumOfBlocks,sizeof(BD));
        MotionDistortion(pOMCPFD, ppreF, porgF, SEARCHRANGE);
        memcpy(imgY_pre_buffer,imgY_org_buffer,input->img_width*input->img_height*3/2*sizeof(byte));

#if AQPO
		{
			int b;
			LogMAD[FNIndex] = 0.0;	
			for(b=0; b<pOMCPFD->TotalNumOfBlocks; b++)
			{
				LogMAD[FNIndex] += pOMCPFD->BlockDistortionArray[b].MAD;
			}
			LogMAD[FNIndex] = log(LogMAD[FNIndex]);
		}
#endif
      }
      pOMCPFD=NULL;
    }
  }
#endif

#if AQPO
  if(input->AQPEnable && !input->successive_Bframe)
  {
	  if(FrameNumberInFile > AQPStepLength && img->type!=0)
	  {
		  FNIndex = GlobeFrameNumber % AQPStepLength;
		  if(FNIndex==0)
			  FNIndex = AQPStepLength-1;
		  else
			  FNIndex = FNIndex-1;
#if AQPOM3762
		  if(FNIndex%2 != 0)
		  {
			  int i;
			  double sumpreLogMAD;
			  int deltaQP;
			  int gopdeltaQp;
			  double GopdeltaQp;
			  double sumLogMAD = 0.0;
			  int factor = FNIndex==(AQPStepLength-1) ? 180 : 105;

			  sumpreLogMAD = 0.0;
			  for(i=0; i<AQPStepLength; i++)
				  sumpreLogMAD += preLogMAD[i];

			  if(input->AQPEnable & 0x01 )
			  {
				  if(FNIndex == 3)
				  {
					  GopdeltaQp = ((LogMAD[FNIndex]-preLogMAD[FNIndex])/(sumpreLogMAD+ sumLogMAD)*8*130);
					  gopdeltaQp = (GopdeltaQp<0?(int)(GopdeltaQp-0.5):(int)(GopdeltaQp+0.5));
					  gopdeltaQp = min(2,max(-2, gopdeltaQp));
					  GopQpbase = (int)(GopQpbase+gopdeltaQp);
					  if((GlobeFrameNumber/4)%AQPStepLength==0)
					  {
						  GopQpbase = min(input->qpI+1,max(input->qpI-2, GopQpbase));
						  preGopQPF[0] = GopQpbase;
					  }
					  else if((GlobeFrameNumber/4)%AQPStepLength==1)
					  {
						  GopQpbase = max(input->qpI-1,min(input->qpI+2,max(preGopQPF[0], GopQpbase)));
						  preGopQPF[1] = GopQpbase;
					  }
					  else if((GlobeFrameNumber/4)%AQPStepLength==2)
					  {
						  GopQpbase = max(input->qpI,min(preGopQPF[1]-1,max(preGopQPF[0], GopQpbase)));
						  preGopQPF[2] = GopQpbase;
					  }
					  else if((GlobeFrameNumber/4)%AQPStepLength==3)
					  {
						  GopQpbase = max(input->qpI-1,min(input->qpI+2,max(preGopQPF[2], GopQpbase)));
						  preGopQPF[3] = GopQpbase;
					  }
				  }
			  }

			  if(input->AQPEnable & 0x02)
			  {
				  deltaQP = (int)((LogMAD[FNIndex]-preLogMAD[FNIndex])/(sumpreLogMAD+LogMAD[FNIndex])*5*factor);
				  AQPoffset[FNIndex] = cfg_ref[FNIndex].qp_offset + deltaQP;
				  if(AQPStepLength>=4)
					  if(FNIndex==AQPStepLength-1)
						  AQPoffset[FNIndex] = min(AQPoffset[FNIndex-2],max(1, AQPoffset[FNIndex]));
					  else
						  AQPoffset[FNIndex] = min(MaxQPoffset  ,max(2, AQPoffset[FNIndex]));
			  }

		  }
		  QpOffset[FNIndex] = AQPoffset[FNIndex];

		  if(input->AQPEnable & 0x01)
			  img->qp = GopQpbase + AQPoffset[OffsetIndex];

		  else if(input->AQPEnable & 0x02)
			  img->qp = input->qpI + AQPoffset[OffsetIndex];
#else
		  if(FNIndex%2 != 0)
		  {
			  int i;
			  double sumpreLogMAD = 0.0;
			  int deltaQP;
			  int factor = FNIndex==(AQPStepLength-1) ? 180 : 105;
			  for(i=0; i<AQPStepLength; i++)
				  sumpreLogMAD += preLogMAD[i];

			  deltaQP = (int)((LogMAD[FNIndex]-preLogMAD[FNIndex])/(sumpreLogMAD+LogMAD[FNIndex])*5*factor);
			  AQPoffset[FNIndex] = cfg_ref[FNIndex].qp_offset + deltaQP;

			  if(AQPStepLength>=4)
			  if(FNIndex==AQPStepLength-1)
				  AQPoffset[FNIndex] = min(AQPoffset[FNIndex-2],max(1, AQPoffset[FNIndex]));
			  else
				  AQPoffset[FNIndex] = min(MaxQPoffset  ,max(2, AQPoffset[FNIndex]));
		  }
		  QpOffset[FNIndex] = AQPoffset[FNIndex];
		  img->qp = input->qpI + AQPoffset[OffsetIndex];
#endif
	  }
  }
#endif

  CopyFrameToOldImgOrgVariables();//image padding
#if !DEMULATE_FIX
  current_slice_bytepos = 0; //qhg 20060327 for de-emulation
#endif

#if TDRDO
  if(input->TDEnable!=0 && GlobeFrameNumber % StepLength == 0 && GlobeFrameNumber<input->no_frames-1  && input->intra_period==0)
  {
	  CaculateKappaTableLDP(OMCPDList, RealDList, GlobeFrameNumber, img->qp);
  }
#endif

#if RATECONTROL
  if(input->EncControl==1)
  {
	  if(pRC->IntraPeriod==0)
	  {
		  if(FrameNumberInFile%gop_size_all == 1)
		  {
			  pRC->DeltaQP = CalculateGopDeltaQP_RateControl(pRC,img->type,FrameNumberInFile,gop_size_all);
		  }
	  }
	  else if (pRC->IntraPeriod==1)
	  {
		  if((FrameNumberInFile%gop_size_all == 0) && (FrameNumberInFile != 0))
		  {
			  pRC->DeltaQP = CalculateGopDeltaQP_RateControl(pRC,img->type,FrameNumberInFile,gop_size_all);
		  }
	  }
	  else
	  {
		  if ((img->type == 0) && ((pRC->TotalFrames - pRC->CodedFrameNumber) <= (2 * pRC->IntraPeriod * gop_size_all)))
		  {
			  Init_FuzzyController(0.50);//enhance adjusting strength of the last GOP group by lmk
		  }
		  if((FrameNumberInFile%gop_size_all == 0) && (FrameNumberInFile != 0))
		  {
			  pRC->DeltaQP = CalculateGopDeltaQP_RateControl(pRC,img->type,FrameNumberInFile,gop_size_all);
		  }
		  else if (pRC->TotalFrames - pRC->CodedFrameNumber ==(pRC->TotalFrames - 1) % gop_size_all)
		  {
			  pRC->DeltaQP = CalculateGopDeltaQP_RateControl(pRC,img->type,FrameNumberInFile,gop_size_all);
		  }
	  }

	  if ((pRC->CodedFrameNumber % gop_size_all == 1) || (pRC->IntraPeriod == 1))
	  {
		  if (pRC->IntraPeriod > 1 && img->type == 0)
		  {
			  int remainGOPnum;
			  if((pRC->TotalFrames - pRC->CodedFrameNumber) > (pRC->IntraPeriod * gop_size_all + gop_size_all))
				  remainGOPnum = pRC->IntraPeriod;
			  else
				  remainGOPnum = (int)ceil(1.0F*(pRC->TotalFrames - pRC->CodedFrameNumber) / gop_size_all);

			  // handle final GOP by ymzhou
			  if((pRC->TotalFrames - pRC->CodedFrameNumber) <= (pRC->IntraPeriod * gop_size_all + gop_size_all))
			  {
				  if((1.0 * remainGOPnum / pRC->IntraPeriod) <= 1.0/3) 
					  pRC->DeltaQP += 8;//as bitrate halve
				  else if((1.0 * remainGOPnum / pRC->IntraPeriod) <= 1.0/2)
					  pRC->DeltaQP += 3;
				  else if((1.0 * remainGOPnum / pRC->IntraPeriod) <= 2.0/3)
					  pRC->DeltaQP += 4;
				  else if((1.0 * remainGOPnum / pRC->IntraPeriod) <= 3.0/4)
					  pRC->DeltaQP += 0;
				  else if((1.0 * remainGOPnum / pRC->IntraPeriod) <= 4.0/4)
					  pRC->DeltaQP += 2;
				  else
					  pRC->DeltaQP += 0;
			  }

			  img->qp = pRC->GopAvgBaseQP / pRC->GopAvgBaseQPCount + pRC->DeltaQP;
			  input->qpI = img->qp;
		  }
		  else
		  {
			  img->qp    += pRC->DeltaQP;
			  input->qpI += pRC->DeltaQP;
		  }
#if QUANT_FIX
      img->qp    = max(MIN_QP,min(img->qp,    MAX_QP + (input->sample_bit_depth - 8)*8 - 5));
      input->qpI = max(MIN_QP,min(input->qpI, MAX_QP + (input->sample_bit_depth - 8)*8 - 5));
#else
		  img->qp    = max(MIN_QP,min(img->qp,    MAX_QP-5));
		  input->qpI = max(MIN_QP,min(input->qpI, MAX_QP-5));
#endif
		  input->qpP = input->qpI + 1;
		  input->qpB = input->qpI + 4;
	  }
	  pRC->RcQP = input->qpI;
  }
#endif

#if !INTERLACE_CODING
  if ( input->InterlaceCodingOption == FRAME_CODING ) // !! frame coding
#endif
  {
    put_buffer_frame ();     //initialize frame buffer
    //frame picture
#if INTERLACE_CODING    
    if (input->InterlaceCodingOption == 3)
    {
      img->is_top_field = (img->tr + total_frames*256)%2 == 0;
    }
#endif
#if INTERLACE_CODING_FIX
    if (input->InterlaceCodingOption == 0) // frame picture coding
    {
      if ( input->progressive_frame ) 
      {
        img->progressive_frame = 1;
        img->picture_structure = 1;
      }
      else
      {
        img->progressive_frame = 0;
        img->picture_structure = 1;
      }
    }
    else if (input->InterlaceCodingOption == 3) // field picture coding
    {
      img->progressive_frame = 0;
      img->picture_structure = 0;
    }
#else
    if ( input->progressive_frame ) //add by wuzhongmou 0610
    {
      img->progressive_frame = 1;
      img->picture_structure = 1;
    }
    else
    {
      img->progressive_frame = 0;
      img->picture_structure = 1;
    }                 //add by wuzhongmou 0610
#endif
    DecideMvRange();  // 20071224

    if ( img->type == B_IMG )
    {
      Bframe_ctr++;  // Bframe_ctr only used for statistics, should go to stat->
    }

    if ( input->slice_set_enable ) //XZHENG, 2008.04
    {
      init_slice_set ( 0 );  //qyu 0817 0: frame     1: field
    }

    code_a_picture ( frame_pic );

	if (img->type==B_IMG)
	{
		img->imgtr_fwRefDistance[0] = trtmp;
	}
	if (curr_RPS.referd_by_others)
	{
		int i;
		for ( i =0; i<REF_MAXBUFFER;i++)
		{
			if (img->tr == img->imgtr_fwRefDistance[i])
			{
				break;
			}
		}
#ifdef AVS2_S2_BGLONGTERM
		if(!(img->typeb == BACKGROUND_IMG && background_output_flag == 0))
#endif
		UnifiedOneForthPix(imgY,imgUV[0],imgUV[1],oneForthRefY[i]);
	}

#ifdef AVS2_S2_BGLONGTERM
#if AVS2_SCENE_CD
	if(img->typeb == BACKGROUND_IMG && input->bg_enable)
#else
	if(img->typeb == BACKGROUND_IMG && input->bg_enable && input->profile_id == 0x50)
#endif
	{
		int l;
		UnifiedOneForthPix(imgY,imgUV[0],imgUV[1],background_frame_quarter);
		for(l=0; l<img->height; l++)
#if EXTEND_BD
			memcpy(background_frame[0][l],imgY[l],sizeof(byte) * img->width);
#else
			memcpy(background_frame[0][l],imgY[l],img->width);
#endif
		for(l=0; l<img->height_cr; l++)
		{
#if EXTEND_BD
			memcpy(background_frame[1][l],imgUV[0][l],sizeof(byte) * img->width_cr);
			memcpy(background_frame[2][l],imgUV[1][l],sizeof(byte) * img->width_cr);
#else
			memcpy(background_frame[1][l],imgUV[0][l],img->width_cr);
			memcpy(background_frame[2][l],imgUV[1][l],img->width_cr);
#endif
		}
	}
#endif

  }

  writeout_picture ();//qyu delete PAFF

#ifdef AVS2_S2_BGLONGTERM
#if AVS2_SCENE_CD
  if(input->bg_enable && input->always_no_bgmodel==0 
#if AVS2_SCENE_GB_CD
	  && !(img->typeb == BACKGROUND_IMG && background_output_flag == 0)
#endif
	  )
#else
  if(input->profile_id == 0x50 && input->always_no_bgmodel==0)
#endif
  {
	  if((img->number==0||FrameNumberInFile-last_input_frame+input->bg_model_number>=input->bg_period*(1+input->successive_Bframe)&&background_number<total_bg_number&&train_start==0)) //total_bg_number
	  {
		  bg_allocModel(imgY_org_buffer,input->img_width,input->img_height);
		  train_start=1;
		  printf("\ntrain start\n");
	  }
	  else
	  {
		  if(train_start==1&&input->always_no_bgmodel==0)
			  bg_insert(imgY_org_buffer);
	  }
  }
#endif

  free_slice();
  FreeBitstream_ALF();
  FreeBitstream();

  find_snr ();

#if TDRDO
  if(input->TDEnable!=0)
  {
	  int DelFDNumber;
	  FD *pDelFD;
	  precF->base = NULL;
	  precF->Y = imgY[0];
	  precF->U = imgUV[0][0];
	  precF->V = imgUV[1][0];
	  if((FrameNumberInFile % StepLength == 0 && !input->successive_Bframe_all)||input->successive_Bframe_all)
		  MotionDistortion(pRealFD, porgF, precF, 0);
	  pRealFD->FrameNumber=FrameNumberInFile;
	  globenumber++;

	  DelFDNumber=globenumber-StepLength-1;
	  if(DelFDNumber>=0)
	  {
		  pDelFD = &RealDList->FrameDistortionArray[DelFDNumber];
		  if(pDelFD->BlockDistortionArray!=NULL)
			  free(pDelFD->BlockDistortionArray);
		  pDelFD->BlockDistortionArray = NULL;
	  }
	  if(FrameNumberInFile % StepLength==0)
	  {
		  DelFDNumber = FrameNumberInFile/StepLength - 2;
		  if(DelFDNumber>=0)
		  {
			  pDelFD = &OMCPDList->FrameDistortionArray[DelFDNumber];
			  if(pDelFD->BlockDistortionArray!=NULL)
				  free(pDelFD->BlockDistortionArray);
			  pDelFD->BlockDistortionArray = NULL;
			  if(pDelFD->subFrameDistortionArray!=NULL)
				  free(pDelFD->subFrameDistortionArray);
			  pDelFD->subFrameDistortionArray = NULL;
		  }
	  }
  }
#endif

#if RATECONTROL
  if (input->useDQP)
  {
	  img->qp = (int) ((0.5 + pRC->SumMBQP) / pRC->NumMB);
  }
  if (input->EncControl!=0)
    Updata_RateControl(pRC, (int)(stat->bit_ctr - stat->bit_ctr_n), img->qp, img->type, FrameNumberInFile, gop_size_all);
#endif

  time ( &ltime2 );             // end time sec
#ifdef WIN32
  _ftime ( &tstruct2 );         // end time ms
#else
  ftime ( &tstruct2 );          // end time ms
#endif

  tmp_time = ( ltime2 * 1000 + tstruct2.millitm ) - ( ltime1 * 1000 + tstruct1.millitm );
  tot_time = tot_time + tmp_time;



  // Write reconstructed images
  //if ( input->output_enc_pic ) //output_enc_pic
  //{
  //  write_reconstructed_image ();
  //}

  if (curr_RPS.referd_by_others && img->type != INTRA_IMG)
  {
	  addCurrMvtoBuf();
#if MV_COMPRESSION
      compressMotion();
#endif
  }

  if (input->output_enc_pic)
  {
#ifdef AVS2_S2_BGLONGTERM
	  if (img->typeb == BACKGROUND_IMG && background_output_flag == 0 && p_dec_background)
		write_GB_frame( p_dec_background );
	  else
#endif
		write_reconstructed_image();
  }

  report_frame(tmp_time);

  stat->bit_ctr_n = stat->bit_ctr;
  if (img->type != B_IMG
#if AVS2_SCENE_GB_CD
	  && !(img->typeb == BACKGROUND_IMG && background_output_flag == 0)
#endif
	  )
  {
	  img->numIPFrames++;
  }

  if ( img->number == 0 )
  {
    return 0;
  }
  else
  {
    return 1;
  }
}

/*
*************************************************************************
* Function:This function write out a picture
* Input:
* Output:
* Return: 0 if OK,                                                         \n
1 in case of error
* Attention:
*************************************************************************
*/

static int writeout_picture()
{

  assert ( currBitStream->bits_to_go == 8 );  //! should always be the case,
  //! the byte alignment is done in terminate_slice
  WriteBitstreamtoFile();

  return 0;
}

/*
*************************************************************************
* Function:Encodes a frame picture
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

static void code_a_picture ( Picture *frame )
{
  AllocateBitstream();
  AllocateBitstream_ALF();

#if REFINED_QP
  if ( input->use_refineQP )
  {
	int	  lambdaQP;		//M2331 2008-04
	double  lambdaF;    //M2331 2008-04
    int qp;
    int i;
    int prevP_no, RPS_idx, p_interval, no_IPframes;
    //double qp     = ( double ) img->qp - SHIFT_QP;
#if FREQUENCY_WEIGHTING_QUANTIZATION
	if(WeightQuantEnable)
	{  
		mb_wq_mode=WQ_MODE_D;
    }
#endif
    if(img->type == F_IMG)
    {
      lambdaQP = LambdaQPTab[P_IMG][mb_wq_mode];
      lambdaF  = LambdaFTab[P_IMG][mb_wq_mode];
    }
    else
    {
      lambdaQP = LambdaQPTab[img->type][mb_wq_mode];
      lambdaF  = LambdaFTab[img->type][mb_wq_mode];
    }

#if (FREQUENCY_WEIGHTING_QUANTIZATION && AWQ_WEIGHTING)
    if(WeightQuantEnable)
    {
      qp     = img->qp - SHIFT_QP + lambdaQP;
    }
    else
    {
      qp     = img->qp - SHIFT_QP;
    }
#else
    qp     = img->qp - SHIFT_QP;
#endif

    p_interval = 0;
    for (i=0; i<=subGopID; i++)
    {
      p_interval +=(input->jumpd_sub[i]+1);
    }
    prevP_no      = ( img->number - 1 ) * input->jumpd_all+(p_interval-input->jumpd-1);
    RPS_idx       = subGopNum==1? ((coding_order-1) % gop_size):(coding_order-1-prevP_no);
    no_IPframes = ( input->no_frames + (input->successive_Bframe_all + subGopNum-1) - 1 ) / ( input->successive_Bframe_all + subGopNum ) + 1;
    if(input->intra_period == 1){
      lambda_mode   = 0.85 * pow ( 2, qp / 4.0 ) *  LAM_2Level_TU;
    }
    else 
    {
      //lambda_mode   = 0.68 * pow ( 2, qp / 4.0 ) * ( img->type == B_IMG ? 1.2 : 0.8 );
#if (FREQUENCY_WEIGHTING_QUANTIZATION && AWQ_WEIGHTING)
      if(WeightQuantEnable)
      {
        lambda_mode   =lambdaF * pow ( 2, qp / 4.0 ) * ( img->type == B_IMG ? 1.2 : 0.8 );
      }
      else
      {
        lambda_mode   = 0.68 * pow ( 2, qp / 4.0 ) * ( img->type == B_IMG ? 1.2 : 0.8 );
      }
#else
      lambda_mode   = 0.68 * pow ( 2, qp / 4.0 ) * ( img->type == B_IMG ? 1.2 : 0.8 );
#endif

      if ( input->successive_Bframe > 0 )
      {
        if( (img->type != I_IMG && RPS_idx != 0) || (img->number-background_number == no_IPframes-1) ){
          lambda_mode *= max( 2.00, min( 4.00, qp / 8.0 ) );
        } 
        else if (img->type == INTER_IMG || img->type == F_IMG)
        {
          lambda_mode *= 1.25;
        }
      }
      else
      {
        if( (RPS_idx+1) % gop_size != 0 
#if AVS2_SCENE_GB_CD
			&& img->typeb != BACKGROUND_IMG
#endif
			)
        {
#if LAMBDA_CHANGE    
          lambda_mode *= max( 2.00, min( 4.00, qp / 8.0 ) ) * 0.8;
#else
          lambda_mode *= max( 2.00, min( 4.00, qp / 8.0 ) ) * 0.6;
#endif
        } 
      } 
    }

    if ( input->intra_period != 1 )
    {
#if REFINED_QP_FIX
      img->qp = 5.661 * log(lambda_mode) + 13.131 + 0.5;
#else
      if ( img->type == I_IMG )
      {
        img->qp = 5.661 * log(lambda_mode) + 13.131 + 0.5;
      }
      else if ( img->type == F_IMG )
      {
        img->qp = 5.2051 * log(lambda_mode) + 14.260 + 0.5;
      }
      else if ( img->type == B_IMG )
      {
        img->qp = 4.1497 * log(lambda_mode) + 16.489 + 0.5;
      }
#endif
    }
  }
#endif

  picture_header();

#if PicExtensionData
  if (input->PicExtentData && !input->alf_enable)
  {
    picture_copyright_extension(currBitStream);
    picture_cameraparameters_extension(currBitStream);
    picture_display_extension(currBitStream);
  }
#endif
#if ROI_M3264
  if (input->ROI_Coding && !input->alf_enable)
    roi_parameters_extension(img->tr, currBitStream);
#endif

  picture_data ( frame );

  frame->bits_per_picture = 8 * ( currBitStream->byte_pos );
}
#if M3480_TEMPORAL_SCALABLE
void cleanRefMVBufRef( int pos )
{
	int k, x, y;
	//re-init mvbuf
	for (k=0; k<2; k++)
	{
		for (y=0; y<img->height/MIN_BLOCK_SIZE; y++)
		{
			for (x=0; x<img->width/MIN_BLOCK_SIZE; x++)
			{
				mvbuf[pos][y][x][k] = 0;
			}
		}
	}
	//re-init refbuf
	for ( y = 0; y < img->height / MIN_BLOCK_SIZE; y++ )
	{
		for ( x = 0; x < img->width / MIN_BLOCK_SIZE ; x++ )
		{
			refbuf[pos][y][x] = -1;
		}
	}
}
#endif
void prepare_RefInfo()
{
	int i, j, tmp_coi, tmp_poc ,tmp_refered, tmp_tlevel, tmp_is_output;
#if EXTEND_BD
	byte ***tmp_yuv;
	byte **tmp_4yuv;
#else
	unsigned char ***tmp_yuv;
	unsigned char **tmp_4yuv;
#endif
	int **tmp_ref;
	int ***tmp_mv;
	int flag=0;

	int tmp_ref_poc[4];

	double *tmp_saorate;
#if M3480_TEMPORAL_SCALABLE
	int RPS_idx;
	int layer;
	if( temporal_id_exist_flag == 1 )
	{
		int prevP_no;
		int p_interval;

		p_interval = 0;
		for (i=0; i<=subGopID; i++)
		{
			p_interval +=(input->jumpd_sub[i]+1);
		}
		prevP_no=( img->number - 1 ) * input->jumpd_all+(p_interval-input->jumpd-1);

		RPS_idx = subGopNum==1? ((coding_order-1) % gop_size):(coding_order-1-prevP_no);
	}
	if ( temporal_id_exist_flag == 1 && img->type != INTRA_IMG )
	{
		if( RPS_idx >= 0 && RPS_idx < gop_size_all )
		{
			if( cfg_ref_all[ RPS_idx ].temporal_id < 0 || cfg_ref_all[ RPS_idx ].temporal_id >= TEMPORAL_MAXLEVEL )
				cfg_ref_all[ RPS_idx ].temporal_id = TEMPORAL_MAXLEVEL - 1;//the lowest level
			layer = cfg_ref_all[ RPS_idx ].temporal_id;
		}
		else
			layer = TEMPORAL_MAXLEVEL - 1;
	}
	else
		layer = 0;
#endif
	//////////////////////////////////////////////////////////////////////////
	//update IDR frame
	if (img->tr > next_IDRtr && curr_IDRtr != next_IDRtr)
	{
		curr_IDRtr = next_IDRtr;
		curr_IDRcoi = next_IDRcoi;
	}

	//////////////////////////////////////////////////////////////////////////
	// re-order the ref buffer according to RPS
	img->nb_references = curr_RPS.num_of_ref;//

	for ( i=0; i<curr_RPS.num_of_ref; i++ )
	{
		int accumulate = 0;
		tmp_yuv = ref[i];
		tmp_4yuv = oneForthRefY[i];
		tmp_coi = img->imgcoi_ref[i];
#if M3480_TEMPORAL_SCALABLE
		tmp_tlevel = img->temporal_id[i];
#endif
		tmp_is_output = img->is_output[i];
		tmp_poc = img->imgtr_fwRefDistance[i];
		tmp_refered = img->refered_by_others[i];
		tmp_ref = refbuf[i];
		tmp_mv = mvbuf[i];

		tmp_saorate = saorate[i];
		memcpy(tmp_ref_poc,ref_poc[i],4*sizeof(int));

		for ( j=i; j<REF_MAXBUFFER; j++)///////////////  
		{
			int k ,tmp_tr;
			for (k=0;k<REF_MAXBUFFER;k++)//
			{
				if (((int)coding_order - (int)curr_RPS.ref_pic[i]) == img->imgcoi_ref[k] && img->imgcoi_ref[k]>=-256)
				{
					break;
				}
			}
			if (k==REF_MAXBUFFER)
			{
				tmp_tr = -1;
			}
			else
			{
				tmp_tr = img->imgtr_fwRefDistance[k];
			}
			if ( tmp_tr < curr_IDRtr)
			{
				curr_RPS.ref_pic[i] = coding_order - curr_IDRcoi;

				for ( k=0;k<i;k++)
				{
					if (curr_RPS.ref_pic[k]==curr_RPS.ref_pic[i])
					{
						accumulate++;
						break;
					}
				}
			}
			if ( img->imgcoi_ref[j] == coding_order - curr_RPS.ref_pic[i])
			{
				break;
			}
		}
		if (j==REF_MAXBUFFER || accumulate)
		{
			img->nb_references--;
		}
		if ( j != REF_MAXBUFFER )
		{
			ref[i] = ref[j];
			oneForthRefY[i] =oneForthRefY[j];
			refbuf[i] = refbuf[j];
			mvbuf[i] = mvbuf[j];
			img->imgcoi_ref[i] = img->imgcoi_ref[j];
#if M3480_TEMPORAL_SCALABLE
			img->temporal_id[i] = img->temporal_id[j];
			if( img->temporal_id[i] > layer )
				assert("Frame with lower temporal_i can not ref higher frames!");
#endif
			img->is_output[i] = img->is_output[j];
			img->imgtr_fwRefDistance[i] = img->imgtr_fwRefDistance[j];
			img->refered_by_others[i] = img->refered_by_others[j];

			saorate[i] = saorate[j];
			saorate[j] = tmp_saorate;

			memcpy(ref_poc[i],ref_poc[j],4*sizeof(int));
			ref[j] = tmp_yuv;
			oneForthRefY[j] = tmp_4yuv;
			refbuf[j] = tmp_ref;
			mvbuf[j] = tmp_mv;
			img->imgcoi_ref[j] = tmp_coi;
#if M3480_TEMPORAL_SCALABLE
			img->temporal_id[j] = tmp_tlevel;
#endif
			img->is_output[j] = tmp_is_output;
			img->imgtr_fwRefDistance[j] = tmp_poc;
			img->refered_by_others[j] = tmp_refered;

			memcpy(ref_poc[j],tmp_ref_poc,4*sizeof(int));
		}
	}
	curr_RPS.num_of_ref = img->nb_references;

	if (img->type==B_IMG&&(img->imgtr_fwRefDistance[0]<=img->tr||img->imgtr_fwRefDistance[1]>=img->tr))
	{
#if REF_BUG_FIX
        //******************************************//	
        int max_for = 1, k;
        int min_bck = 0;
        int tmp_max_for = 0;
        int tmp_min_bck = 1 << 30;
        for (i = 1; i<REF_MAXBUFFER; i++)
        {
            int tmp_ref = img->imgtr_fwRefDistance[i];
            if (tmp_ref<img->tr && tmp_ref>tmp_max_for && tmp_ref >= curr_IDRtr)
            {
                max_for = i;//find forward ref
                tmp_max_for = tmp_ref;
            }
            if (tmp_ref>img->tr && tmp_ref<tmp_min_bck)
            {
                min_bck = i;//find backward ref
                tmp_min_bck = tmp_ref;
            }
        }
        for (k = 0; k<2; k++)
        {
            i = (k == 0 ? max_for : min_bck);
            j = (k == 0 ? 1 : 0);
            tmp_yuv = ref[i];
            tmp_4yuv = oneForthRefY[i];
            tmp_coi = img->imgcoi_ref[i];
#if M3480_TEMPORAL_SCALABLE
            tmp_tlevel = img->temporal_id[i];
#endif
            tmp_is_output = img->is_output[i];
            tmp_poc = img->imgtr_fwRefDistance[i];
            tmp_refered = img->refered_by_others[i];
            tmp_ref = refbuf[i];
            tmp_mv = mvbuf[i];
            tmp_saorate = saorate[i];
            memcpy(tmp_ref_poc, ref_poc[i], 4 * sizeof(int));
            ref[i] = ref[j];
            oneForthRefY[i] = oneForthRefY[j];
            refbuf[i] = refbuf[j];
            mvbuf[i] = mvbuf[j];
            img->imgcoi_ref[i] = img->imgcoi_ref[j];
#if M3480_TEMPORAL_SCALABLE
            img->temporal_id[i] = img->temporal_id[j];
#endif

            img->is_output[i] = img->is_output[j];
            img->imgtr_fwRefDistance[i] = img->imgtr_fwRefDistance[j];
            img->refered_by_others[i] = img->refered_by_others[j];

            saorate[i] = saorate[j];
            saorate[j] = tmp_saorate;

            memcpy(ref_poc[i], ref_poc[j], 4 * sizeof(int));
            ref[j] = tmp_yuv;
            oneForthRefY[j] = tmp_4yuv;
            refbuf[j] = tmp_ref;
            mvbuf[j] = tmp_mv;
            img->imgcoi_ref[j] = tmp_coi;
#if M3480_TEMPORAL_SCALABLE
            img->temporal_id[j] = tmp_tlevel;
#endif
            img->is_output[j] = tmp_is_output;
            img->imgtr_fwRefDistance[j] = tmp_poc;
            img->refered_by_others[j] = tmp_refered;

            memcpy(ref_poc[j], tmp_ref_poc, 4 * sizeof(int));
        }
        //change the wrong RPS
        for (i = 0; i < curr_RPS.num_of_ref; i++)
        {
            curr_RPS.ref_pic[i] = coding_order - img->imgcoi_ref[i];
        }
        printf("wrong reference configuration for frame:%d has been corrected\n",img->tr);
#else
#if REF_CODEC_DELETE    
        printf("wrong reference configuration for B frame");
        exit(-1);
#else
		//******************************************//	
		int max_for=1,k;
		int min_bck=0;
		int tmp_max_for = 0;
		int tmp_min_bck = 1<<30;
		for(i=1;i<REF_MAXBUFFER;i++)
		{
			int tmp_ref = img->imgtr_fwRefDistance[i];
			if (tmp_ref<img->tr && tmp_ref>tmp_max_for && tmp_ref>=curr_IDRtr)
			{
				max_for = i;//find forward ref
				tmp_max_for = tmp_ref;
			}
			if (tmp_ref>img->tr && tmp_ref<tmp_min_bck)
			{
				min_bck = i;//find backward ref
				tmp_min_bck = tmp_ref;
			}
		}
		for (k=0;k<2;k++)
		{
			i = (k==0? max_for:min_bck);
			j = (k==0? 1:0);
			tmp_yuv = ref[i];
			tmp_4yuv = oneForthRefY[i];
			tmp_coi = img->imgcoi_ref[i];
#if M3480_TEMPORAL_SCALABLE
			tmp_tlevel = img->temporal_id[i];
#endif
			tmp_is_output = img->is_output[i];
			tmp_poc = img->imgtr_fwRefDistance[i];
			tmp_refered = img->refered_by_others[i];
			tmp_ref = refbuf[i];
			tmp_mv = mvbuf[i];
			tmp_saorate = saorate[i];
			memcpy(tmp_ref_poc,ref_poc[i],4*sizeof(int));
			ref[i] = ref[j];
			oneForthRefY[i] =oneForthRefY[j];
			refbuf[i] = refbuf[j];
			mvbuf[i] = mvbuf[j];
			img->imgcoi_ref[i] = img->imgcoi_ref[j];
#if M3480_TEMPORAL_SCALABLE
			img->temporal_id[i] = img->temporal_id[j];
#endif

			img->is_output[i] = img->is_output[j];
			img->imgtr_fwRefDistance[i] = img->imgtr_fwRefDistance[j];
			img->refered_by_others[i] = img->refered_by_others[j];

			saorate[i] = saorate[j];
			saorate[j] = tmp_saorate;

			memcpy(ref_poc[i],ref_poc[j],4*sizeof(int));
			ref[j] = tmp_yuv;
			oneForthRefY[j] = tmp_4yuv;
			refbuf[j] = tmp_ref;
			mvbuf[j] = tmp_mv;
			img->imgcoi_ref[j] = tmp_coi;
#if M3480_TEMPORAL_SCALABLE
			img->temporal_id[j] = tmp_tlevel;
#endif
			img->is_output[j] = tmp_is_output;
			img->imgtr_fwRefDistance[j] = tmp_poc;
			img->refered_by_others[j] = tmp_refered;

			memcpy(ref_poc[j],tmp_ref_poc,4*sizeof(int));
		}
#endif
#endif 
	}
    if ((img->type==P_IMG)||(img->type==F_IMG))
	{
		for (i=img->nb_references;i<input->no_multpred;i++)
		{
			int max_for=img->nb_references;
			int tmp_max_for = curr_IDRtr;
			int flag = 0;
			for (j=img->nb_references; j<REF_MAXBUFFER; j++)
			{
				int tmp_ref = img->imgtr_fwRefDistance[j];
#if M3480_TEMPORAL_SCALABLE
				if ( tmp_ref<img->tr && tmp_ref>curr_IDRtr && ( temporal_id_exist_flag != 1 || layer >= img->temporal_id[ j ] ) && abs(img->imgtr_fwRefDistance[j] - img->tr)<128 )
#else
				if ( tmp_ref<img->tr && tmp_ref>curr_IDRtr && abs(img->imgtr_fwRefDistance[j] - img->tr)<128 )
#endif
				{
					flag = 1;

					if (tmp_max_for < tmp_ref)
					{
						max_for = j;
						tmp_max_for = tmp_ref;
					}
				}
			}
			if (flag)
			{
				{
					j=max_for;
					tmp_yuv = ref[i];
					tmp_4yuv = oneForthRefY[i];
					tmp_coi = img->imgcoi_ref[i];
#if M3480_TEMPORAL_SCALABLE
					tmp_tlevel = img->temporal_id[i];
#endif
					tmp_is_output = img->is_output[i];
					tmp_poc = img->imgtr_fwRefDistance[i];
					tmp_refered = img->refered_by_others[i];
					tmp_ref = refbuf[i];
					tmp_mv = mvbuf[i];
					tmp_saorate = saorate[i];
					memcpy(tmp_ref_poc,ref_poc[i],4*sizeof(int));
					ref[i] = ref[j];
					oneForthRefY[i] =oneForthRefY[j];
					refbuf[i] = refbuf[j];
					mvbuf[i] = mvbuf[j];
					img->imgcoi_ref[i] = img->imgcoi_ref[j];
#if M3480_TEMPORAL_SCALABLE
					img->temporal_id[i] = img->temporal_id[j];
#endif
					img->is_output[i] = img->is_output[j];
					img->imgtr_fwRefDistance[i] = img->imgtr_fwRefDistance[j];
					img->refered_by_others[i] = img->refered_by_others[j];
					saorate[i] = saorate[j];
					saorate[j] = tmp_saorate;
					memcpy(ref_poc[i],ref_poc[j],4*sizeof(int));

					curr_RPS.ref_pic[i] = coding_order - img->imgcoi_ref[j];

					ref[j] = tmp_yuv;
					oneForthRefY[j] = tmp_4yuv;
					refbuf[j] = tmp_ref;
					mvbuf[j] = tmp_mv;
					img->imgcoi_ref[j] = tmp_coi;
#if M3480_TEMPORAL_SCALABLE
					img->temporal_id[j] = tmp_tlevel;
#endif
					img->is_output[j] = tmp_is_output;
					img->imgtr_fwRefDistance[j] = tmp_poc;
					img->refered_by_others[j] = tmp_refered;

					memcpy(ref_poc[j],tmp_ref_poc,4*sizeof(int));
				}
				flag = 0;
				img->nb_references++;
			}
			else
			{
				break;
			}
		}
    /*  //bug fixed for motion compression
		for (j=0; j<img->height/MIN_BLOCK_SIZE; j++)
		{
			for (i=0; i<img->width/MIN_BLOCK_SIZE; i++)
			{
				refbuf[0][j][i] = refFrArr[j][i];
			}
		}
    */
	}
  curr_RPS.num_of_ref = img->nb_references;
#if 0
  for (i=0;i<img->nb_references;i++)
  {
	  curr_RPS.ref_pic[i] = coding_order - img->imgcoi_ref[i];
  }
#endif

	if ( img->nb_references > 1 )
	{
		img->num_ref_pic_active_fwd_minus1 = 1;  //yling
	}
	//////////////////////////////////////////////////////////////////////////
	// delete the frame that will never be used
  {
    int actualRemovePic[MAXREF]={-1};
    int removeNum=0;
	for ( i=0; i< curr_RPS.num_to_remove; i++)
	{
		for ( j=0; j<REF_MAXBUFFER; j++)
		{
			if ( img->imgcoi_ref[j]>=-256 && img->imgcoi_ref[j] == coding_order - curr_RPS.remove_pic[i])
			{
				break;
			}
		}
		if (j<REF_MAXBUFFER && j>=img->nb_references)
		{
#if M3480_TEMPORAL_SCALABLE
			if( img->temporal_id[j] < layer )
				assert("Can not remove lower-temporal frame.");
			img->temporal_id[j] = -1;
#endif
			img->imgcoi_ref[j] = -257;
			if( img->is_output[j] == -1 )
				img->imgtr_fwRefDistance[j] = -256;
			actualRemovePic[removeNum]=curr_RPS.remove_pic[i];
			removeNum++;
		}
	}
  curr_RPS.num_to_remove = removeNum;
  for (i=0;i<removeNum;i++)
  {
    curr_RPS.remove_pic[i]=actualRemovePic[i];
  }
  }
	//////////////////////////////////////////////////////////////////////////
	//   add current frame to ref buffer
	for ( i=0; i<REF_MAXBUFFER; i++ )
	{
		if ( ( img->imgcoi_ref[i] < -256 || abs(img->imgtr_fwRefDistance[i] - img->tr)>=128 ) && img->is_output[i] == -1 )
		{
			break;
		}
	}
	if ( i == REF_MAXBUFFER )
	{
		i--;
	}
	currentFrame = ref[i];
	img->imgtr_fwRefDistance[i] = img->tr;
	img->imgcoi_ref[i] = coding_order;
#if M3480_TEMPORAL_SCALABLE
	img->temporal_id[i] = cur_layer = layer;
#endif
	img->is_output[i] = 1;
	img->refered_by_others[i] = curr_RPS.referd_by_others;

	if (img->type != B_IMG)
	{
		for (j=0;j<img->nb_references;j++)
		{
			ref_poc[i][j] = img->imgtr_fwRefDistance[j];
		}
#if M3480_TEMPORAL_SCALABLE
		for (j=img->nb_references; j<4; j++)
		{
			ref_poc[i][j] = 0;
		}
#endif
	}
	else
	{
		ref_poc[i][0] = img->imgtr_fwRefDistance[1];
		ref_poc[i][1] = img->imgtr_fwRefDistance[0];
#if M3480_TEMPORAL_SCALABLE
		ref_poc[i][2] = 0;
		ref_poc[i][3] = 0;
#endif
	}
#if M3480_TEMPORAL_SCALABLE
	if( img->type == INTRA_IMG )
	{
		int l;
		for( l = 0; l < 4; l ++ )
			ref_poc[i][l] = img->tr;
	}
	cleanRefMVBufRef( i );
#endif
	for (j=0; j<NUM_SAO_COMPONENTS; j++)
	{
		saorate[i][j] = img->cur_saorate[j];
	}
	//////////////////////////////////////////////////////////////////////////
	// updata ref pointer

	if ( img->type!=INTRA_IMG )
	{
		img->imgtr_next_P = img->type == B_IMG? img->imgtr_fwRefDistance[0] : img->tr;
		if (img->type==B_IMG)
		{
			trtmp = img->imgtr_fwRefDistance[0];
			img->imgtr_fwRefDistance[0] = img->imgtr_fwRefDistance[1];
		}
	}
#ifdef AVS2_S2_S
	{
		int k,x,y,ii;
		for(ii = 0; ii < 8; ii++)
		{
			for ( k = 0; k < 2; k++ )
			{
				for ( y = 0; y < img->height / MIN_BLOCK_SIZE; y++ )
				{
					for ( x = 0; x < img->width / MIN_BLOCK_SIZE ; x++ )
					{
						if(img->typeb == BP_IMG)
						{
							mvbuf[ii][y][x][k] = 0;
							refbuf[ii][y][x] = 0;
						}
					}
				}
			}
		}
	}
#endif
}

/*
*************************************************************************
* Function:Initializes the parameters for a new frame
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
static void init_frame ()
{
  int i, j, k;
  int prevP_no, nextP_no;
  int b_interval,p_interval;

  p_interval = 0;
  for (i=0; i<=subGopID; i++)
  {
	  p_interval +=(input->jumpd_sub[i]+1);
  }

  img->current_mb_nr = 0;
  img->current_slice_nr = 0;

  //---Yulj 2004.07.15
  if ( !input->slice_set_enable ) //added by mz, 2008.04.07
  {
    int widthMB = img->width  / MIN_CU_SIZE;
    int heightMB = img->height / MIN_CU_SIZE;
    int minCUsInLCU = (1 << ( input->g_uiMaxSizeInBit - MIN_CU_SIZE_IN_BIT )) * (1 << ( input->g_uiMaxSizeInBit - MIN_CU_SIZE_IN_BIT ));
    int lcusInOneRow = (img->width >> input->g_uiMaxSizeInBit) + (img->width % (1 << input->g_uiMaxSizeInBit) !=0 ? 1 : 0);
    int lcuSize = 1 << input->g_uiMaxSizeInBit;
    img->mb_no_currSliceLastMB = ( input->slice_row_nr != 0 )
                                 ? min ( ( (input->slice_row_nr / lcusInOneRow) * widthMB * (1 << ( input->g_uiMaxSizeInBit - MIN_CU_SIZE_IN_BIT ) ) )
                                 + ( (input->slice_row_nr % lcusInOneRow) << (2*( input->g_uiMaxSizeInBit - MIN_CU_SIZE_IN_BIT )) ), widthMB * heightMB )
                                 : widthMB * heightMB ;
    input->slice_row_nr = input->slice_row_nr != 0 ? input->slice_row_nr : img->mb_no_currSliceLastMB + 1;
  }

  //---end.
#ifdef AVS2_S2_BGLONGTERM
  if(!(img->typeb == BACKGROUND_IMG && background_output_flag==0))
#endif
	coding_order++;
  if (coding_order == 256)
  {
	  coding_order = 0;
	  for (i=0; i<REF_MAXBUFFER; i++)
	  {
		  if (img->imgtr_fwRefDistance[i] >= 0)
		  {
			  img->imgtr_fwRefDistance[i] -= 256;
			  img->imgcoi_ref[i] -= 256;
			  for (j=0; j<4; j++)
			  {
				  ref_poc[i][j] -= 256;
			  }
		  }
	  }
	  last_output -= 256;
	  total_frames++;
	  curr_IDRtr  -= 256;
	  curr_IDRcoi -= 256;
	  next_IDRtr  -= 256;
	  next_IDRcoi -= 256;
  }
  stat->bit_slice = 0;
  img->coded_mb_nr = 0;
  img->slice_offset = 0;

  img->mb_y = img->mb_x = 0;
  img->pix_y = img->pix_c_y = 0; // img->block_y =
  img->pix_x = img->pix_c_x = 0;//img->block_x =


  if ( img->type != B_IMG )
  {
    if ( img->number == 0 )
    {
      img->tr = 0;
    }
    else
    {
#ifdef AVS2_S2_BGLONGTERM
      prevP_no=( img->number - background_number-1 ) * input->jumpd_all+(p_interval-input->jumpd-1);
      if(img->typeb == BACKGROUND_IMG && background_output_flag==0)
        img->tr = (img->number-background_number-1-1) * input->jumpd_all+p_interval;
      else
        img->tr = (img->number-background_number-1) * input->jumpd_all+p_interval;
#else
      prevP_no=( img->number - 1 ) * input->jumpd_all+(p_interval-input->jumpd-1);
      img->tr = (img->number-1) * input->jumpd_all+p_interval;
#endif
      img->tr = min ( img->tr, input->no_frames - 1 );
    }
  }
  else
  {
#ifdef AVS2_S2_BGLONGTERM
	  prevP_no = ( img->number - background_number - 1 ) * input->jumpd_all+(p_interval-input->jumpd-1);
	  nextP_no = ( img->number - background_number - 1 ) * input->jumpd_all+p_interval;
#else
	  prevP_no = ( img->number - 1 ) * input->jumpd_all+(p_interval-input->jumpd-1);
	  nextP_no = ( img->number - 1 ) * input->jumpd_all+p_interval;
#endif
    nextP_no         = min ( nextP_no, input->no_frames - 1 );

    b_interval = ( int ) ( ( double ) ( input->jumpd + 1 ) / ( input->successive_Bframe + 1.0 ) + 0.49999 );

	{
		int offset = coding_order-1-prevP_no+flag_gop+total_frames*256;
		img->tr = prevP_no + cfg_ref[offset].poc; 
	}
#ifdef AVS2_S2_BGLONGTERM
	while (img->tr >= input->no_frames + background_number -1)
#else
	while (img->tr >= input->no_frames - 1)
#endif
	{
		int offset_tmp;
		flag_gop++;
		offset_tmp = coding_order-1-prevP_no+flag_gop+total_frames*256;
		img->tr = prevP_no + cfg_ref[offset_tmp].poc;
	}
    // initialize arrays

  for ( k = 0; k < 2; k++ )
  {
    for ( j = 0; j < img->height / MIN_BLOCK_SIZE; j++ )
    {
      for ( i = 0; i < img->width / MIN_BLOCK_SIZE ; i++ )
      {
        img->fw_mv[j][i][k] = 0;
        img->bw_mv[j][i][k] = 0;
      }
    }
  }

  for ( j = 0; j < img->height / MIN_BLOCK_SIZE; j++ )
  {
    for ( i = 0; i < img->width / MIN_BLOCK_SIZE; i++ )
    {
      img->fw_refFrArr[j][i] = img->bw_refFrArr[j][i] = -1;
    }
  }    
  }

  img->tr -= total_frames*256;


  for ( i = 0; i < img->PicWidthInMbs * img->height / MIN_CU_SIZE; i++ )
  {
    img->mb_data[i].slice_nr = -1;
  }

  img->PicSizeInMbs = ( img->width * img->height ) / ( MIN_CU_SIZE * MIN_CU_SIZE );

  picture_distance = img->tr % 256;          //yling

#if AQPOM3694 // RA
  if(input->successive_Bframe_all && input->AQPEnable)
  {
	  if(img->tr!=0)
	  {
		  if(img->tr%StepLength==0)
		  {
			  int index = (img->tr/StepLength)%input->intra_period;
			  int nextIndex;
			  int offsetTable[4][8]={{0,0,1},{0,0,1,2},{0,1,1,2,2,2},{0,1,1,1,2,2,2,2}};    
			  if(img->tr < (8*input->intra_period))
				  index -= 1;
			  nextIndex = (index+1)%input->intra_period;
			  if(input->intra_period==3)
				  GopQpbase = input->qpI + offsetTable[0][index];
			  if(input->intra_period==4)
				  GopQpbase = input->qpI + offsetTable[1][index];
			  if(input->intra_period==6)
				  GopQpbase = input->qpI + offsetTable[2][index];
			  if(input->intra_period==8)
				  GopQpbase = input->qpI + offsetTable[3][index];
		  }
	  }
	  else
		  GopQpbase = input->qpI;
  }
#endif

  if ( img->type == INTRA_IMG )
  {
    if(input->intra_period==1)
    {
      curr_RPS = cfg_ref[0];
      img->qp =  input->qpI + cfg_ref[0].qp_offset;
    }
    else
    {
	  curr_RPS.num_of_ref = 0;
	  curr_RPS.referd_by_others = 1;
#if AVS2_SCENE_GB_CD
	  if(input->bg_enable && img->typeb == BACKGROUND_IMG && background_output_flag == 0) //bugfix: GB does not remove any pictures and does not call Prepare_RefInfo() 
		  curr_RPS.num_to_remove = 0;
#endif
	  img->qp = input->qpI;
    }
  }
  else
  {
	  int RPS_idx = subGopNum==1? ((coding_order+total_frames*256-1) % gop_size):(coding_order+total_frames*256-1-prevP_no);
	  curr_RPS = cfg_ref[RPS_idx];
	  img->qp = input->qpI + cfg_ref[RPS_idx+flag_gop].qp_offset;

#if AQPOM3694 // RA
	  if(input->successive_Bframe_all && input->AQPEnable)
		  img->qp = GopQpbase + cfg_ref[RPS_idx+flag_gop].qp_offset;
#endif

#if AQPO
	  OffsetIndex=RPS_idx;
#endif
  }
#if TH_ME
  if (img->type != INTRA_IMG && input->usefme)
  {
	  DefineThreshold();
  }
#endif

#ifdef AVS2_S2_BGLONGTERM
  if(img->typeb == BACKGROUND_IMG && background_output_flag == 0)
	  currentFrame = background_ref;
  else
#endif
	  prepare_RefInfo();
}

int min_tr( int ref[REF_MAXBUFFER], int last_out, int *pos)
{
	int i,tmp_min;
	tmp_min = 1<<20;
	*pos =-1;
	for ( i=0; i<REF_MAXBUFFER; i++)
	{
		if (ref[i]<tmp_min && ref[i]>last_out)
		{
			tmp_min = ref[i];
			*pos = i;
		}
	}
	return tmp_min;
}
void addCurrMvtoBuf()
{
	int i, k, x, y;
	for ( i=0; i<REF_MAXBUFFER; i++ )
	{
		if ( coding_order == img->imgcoi_ref[i] )
		{
			break;
		}
	}
	for (k=0; k<2; k++)
	{
		for (y=0; y<img->height/MIN_BLOCK_SIZE; y++)
		{
			for (x=0; x<img->width/MIN_BLOCK_SIZE; x++)
			{
				mvbuf[i][y][x][k] = (((img->type==F_IMG)||(img->type==P_IMG)) ? img->tmp_mv:img->fw_mv)[y][x][k];
			}
		}
	}
	for ( y = 0; y < img->height / MIN_BLOCK_SIZE; y++ )
	{
		for ( x = 0; x < img->width / MIN_BLOCK_SIZE ; x++ )
		{
			refbuf[i][y][x] = (((img->type==F_IMG)||(img->type==P_IMG))? refFrArr:img->fw_refFrArr)[y][x];
		}
	}
}

// Sub-sampling of the stored reference index and motion vector
void compressMotion()
{
	int i, x, y;
	int width = img->width;
	int height = img->height;
	int xPos, yPos;

	for ( i=0; i<REF_MAXBUFFER; i++ )
	{
		if ( coding_order == img->imgcoi_ref[i] )
		{
			break;
		}
	}

	for (y = 0; y < height / MIN_BLOCK_SIZE; y ++)
	{
		for (x = 0; x < width / MIN_BLOCK_SIZE; x ++)
		{
			// middle pixel's motion information
			yPos = y / MV_DECIMATION_FACTOR * MV_DECIMATION_FACTOR + 2;
			xPos = x / MV_DECIMATION_FACTOR * MV_DECIMATION_FACTOR + 2;
			if(yPos >= height / MIN_BLOCK_SIZE){
				yPos = (y / MV_DECIMATION_FACTOR * MV_DECIMATION_FACTOR + height / MIN_BLOCK_SIZE) / 2;
			}
			if(xPos >= width / MIN_BLOCK_SIZE){
				xPos = (x / MV_DECIMATION_FACTOR * MV_DECIMATION_FACTOR + width / MIN_BLOCK_SIZE) / 2;
			}
			mvbuf[i][y][x][0] = mvbuf[i][yPos][xPos][0];
			mvbuf[i][y][x][1] = mvbuf[i][yPos][xPos][1];
			refbuf[i][y][x] = refbuf[i][yPos][xPos];
		}
	}
}

/*
*************************************************************************
* Function:Copy decoded P frame to temporary image array
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
void copy_Pframe ()
{
  int i, j;

  /*
  * the mmin, mmax macros are taken out
  * because it makes no sense due to limited range of data type
  */

  for ( i = 0; i < img->height; i++ )
  {
    for ( j = 0; j < img->width; j++ )
    {
      imgYPrev[i][j] = imgY[i][j];
    }
  }

  for ( i = 0; i < img->height_cr; i++ )
  {
    for ( j = 0; j < img->width_cr; j++ )
    {
      imgUVPrev[0][i][j] = imgUV[0][i][j];
    }
  }

  for ( i = 0; i < img->height_cr; i++ )
  {
    for ( j = 0; j < img->width_cr; j++ )
    {
      imgUVPrev[1][i][j] = imgUV[1][i][j];
    }
  }
}
#if AVS2_SCENE_CD
/*
*************************************************************************
* Function:Writes ORG GB frame to file
This can be done more elegant!
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
#ifdef WIN32
void write_GB_org_frame ( int p_dec)         //!< filestream to output file
#else
void write_GB_org_frame ( FILE *p_dec)         //!< filestream to output file
#endif
{
	int j;
#ifndef WIN32
  int i;
#endif
	int img_width = ( img->width - img->auto_crop_right );
	int img_height = ( img->height - img->auto_crop_bottom );
	int img_width_cr = ( img_width / 2 );
	int img_height_cr = ( img_height / ( input->chroma_format == 1 ? 2 : 1 ) );

#ifdef WIN32
	unsigned char *buf;
#endif
#if EXTEND_BD
	int nSampleSize;
	int shift1;
#else
	unsigned char **imgY_out ;
	unsigned char ***imgUV_out ;
#endif
	unsigned char bufc;
	if ( p_dec != -1 )
	{

#ifdef WIN32
#if EXTEND_BD
		// assume "input->sample_bit_depth" is greater or equal to "input->input_sample_bit_depth"
		if (input->input_sample_bit_depth == 8)
		{
			nSampleSize = 1;
		}
		else if (input->input_sample_bit_depth > 8)
		{
			nSampleSize = 2;
		}
		buf = malloc ( img_height * img_width *  nSampleSize * 3 /2  );
		shift1 = input->sample_bit_depth - input->input_sample_bit_depth;
		
		if (!shift1 && input->input_sample_bit_depth == 8) // 8bit input -> 8bit encode
		{
			for (j = 0; j < img_height*img_width*3/2; j++)
			{
				buf[j] = (unsigned char)imgY_org_buffer[j];
			}

			_write ( p_dec, buf, img_height * img_width *3 /2 );
		}
#if REF_OUTPUT_BUG_F
		free ( buf );
#endif
#else
		buf = malloc ( img_height * img_width );

		for ( j = 0; j < img_height; j++ )
		{
			memcpy ( buf + j * img_width, & ( imgY_out[j][0] ), img_width );
		}

		_write ( p_dec, buf, img_height * img_width );

		for ( j = 0; j < img_height_cr; j++ )
		{
			memcpy ( buf + j * img_width_cr, & ( imgUV_out[0][j][0] ), img_width_cr );
		}

		_write ( p_dec, buf, img_height_cr * img_width_cr );

		for ( j = 0; j < img_height_cr; j++ )
		{
			memcpy ( buf + j * img_width_cr, & ( imgUV_out[1][j][0] ), img_width_cr );
		}

		_write ( p_dec, buf, img_height_cr * img_width_cr );
#if REF_OUTPUT_BUG_F
#ifdef WIN32
		free ( buf );
#endif
#endif
#endif
#else
#if EXTEND_BD
	shift1 = input->sample_bit_depth - input->input_sample_bit_depth;
	if (!shift1 && input->input_sample_bit_depth == 8) // 8bit input -> 8bit encoding
	{
		for ( i=0; i < img_height * img_width *3 /2; i++ )
		{
				bufc = (unsigned char)(imgY_org_buffer[i]);
				fputc ( bufc, p_dec );
		}
	}
	fflush ( p_dec );
#else
	for ( i=0; i < img_height; i++ )
     {
       for ( j=0; j < img_width; j++ )
       {
         fputc( imgY_out[i][j], p_dec );
       }
     }
     for ( i=0; i < img_height_cr; i++ )
     {
       for ( j=0; j < img_width_cr; j++ )
       {
         fputc( imgUV_out[0][i][j], p_dec );
       }
     }
    for ( i=0; i < img_height_cr; i++ )
     {
       for ( j=0; j < img_width_cr; j++ )
       {
         fputc( imgUV_out[1][i][j], p_dec );
       }
     }
     fflush ( p_dec );
#endif
#endif

	}
#if !REF_OUTPUT_BUG_F
#ifdef WIN32
	free ( buf );
#endif
#endif
}

/*
*************************************************************************
* Function:Writes reconstructed GB frame to file
This can be done more elegant!
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
#ifdef WIN32
void write_GB_frame ( int p_dec)         //!< filestream to output file
#else
void write_GB_frame ( FILE *p_dec)         //!< filestream to output file
#endif
{
	int j;
#ifndef WIN32
  int i;
#endif
	int img_width = ( img->width - img->auto_crop_right );
	int img_height = ( img->height - img->auto_crop_bottom );
	int img_width_cr = ( img_width / 2 );
	int img_height_cr = ( img_height / ( input->chroma_format == 1 ? 2 : 1 ) );

#ifdef WIN32
	unsigned char *buf;
#endif
#if EXTEND_BD
	byte **imgY_out ;
	byte ***imgUV_out ;
	int nSampleSize;
	int k;
	int shift1;
#else
	unsigned char **imgY_out ;
	unsigned char ***imgUV_out ;
#endif
	unsigned char bufc;
	if ( p_dec != -1 )
	{
		imgY_out = imgY;
		imgUV_out= imgUV;

#ifdef WIN32
#if EXTEND_BD
		// assume "input->sample_bit_depth" is greater or equal to "input->input_sample_bit_depth"
		if (input->input_sample_bit_depth == 8)
		{
			nSampleSize = 1;
		}
		else if (input->input_sample_bit_depth > 8)
		{
			nSampleSize = 2;
		}
		buf = malloc ( img_height * img_width * nSampleSize );
		shift1 = input->sample_bit_depth - input->input_sample_bit_depth;
		
		if (!shift1 && input->input_sample_bit_depth == 8) // 8bit input -> 8bit encode
		{
			for (j = 0; j < img_height; j++)
			{
				for (k = 0; k < img_width; k++)
				{
					buf[j * img_width + k] = (unsigned char)imgY_out[j][k];
				}
			}
			_write ( p_dec, buf, img_height * img_width );
			for ( j = 0; j < img_height_cr; j++ )
			{
				for (k = 0; k < img_width_cr; k++)
				{
					buf[j * img_width_cr + k] = (unsigned char)imgUV_out[0][j][k];
				}
			}
			_write ( p_dec, buf, img_height_cr * img_width_cr );

			for ( j = 0; j < img_height_cr; j++ )
			{
				for (k = 0; k < img_width_cr; k++)
				{
					buf[j * img_width_cr + k] = (unsigned char)imgUV_out[1][j][k];
				}
			}
			_write ( p_dec, buf, img_height_cr * img_width_cr );
		}
		else if (!shift1 && input->input_sample_bit_depth > 8) // 10/12bit input -> 10/12bit encode
		{
			for (j = 0; j < img_height; j++)
			{
				memcpy ( buf + j * img_width * nSampleSize, & ( imgY_out[j][0] ), img_width * sizeof(byte) );
			}
			_write ( p_dec, buf, img_height * img_width * sizeof(byte));
			for ( j = 0; j < img_height_cr; j++ )
			{
				memcpy ( buf + j * img_width_cr * nSampleSize, & ( imgUV_out[0][j][0] ), img_width_cr * sizeof(byte) );
			}
			_write ( p_dec, buf, img_height_cr * img_width_cr * sizeof(byte) );

			for ( j = 0; j < img_height_cr; j++ )
			{
				memcpy ( buf + j * img_width_cr * nSampleSize, & ( imgUV_out[1][j][0] ), img_width_cr * sizeof(byte) );
			}
			_write ( p_dec, buf, img_height_cr * img_width_cr * sizeof(byte) );
		}
		else if (shift1 && input->input_sample_bit_depth == 8) // 8bit input -> 10/12bit encode
		{
			for (j = 0; j < img_height; j++)
			{
				for (k = 0; k < img_width; k++)
				{
					buf[j * img_width + k] = (unsigned char)Clip1((imgY_out[j][k] + (1 << (shift1 - 1))) >> shift1);
				}
			}
			_write ( p_dec, buf, img_height * img_width );
			for ( j = 0; j < img_height_cr; j++ )
			{
				for (k = 0; k < img_width_cr; k++)
				{
					buf[j * img_width_cr + k] = (unsigned char)Clip1((imgUV_out[0][j][k] + (1 << (shift1 - 1))) >> shift1);
				}
			}
			_write ( p_dec, buf, img_height_cr * img_width_cr );

			for ( j = 0; j < img_height_cr; j++ )
			{
				for (k = 0; k < img_width_cr; k++)
				{
					buf[j * img_width_cr + k] = (unsigned char)Clip1((imgUV_out[1][j][k] + (1 << (shift1 - 1))) >> shift1);
				}
			}
			_write ( p_dec, buf, img_height_cr * img_width_cr );
		}
#if REF_OUTPUT_BUG_F
		free ( buf );
#endif
#else
		buf = malloc ( img_height * img_width );

		for ( j = 0; j < img_height; j++ )
		{
			memcpy ( buf + j * img_width, & ( imgY_out[j][0] ), img_width );
		}

		_write ( p_dec, buf, img_height * img_width );

		for ( j = 0; j < img_height_cr; j++ )
		{
			memcpy ( buf + j * img_width_cr, & ( imgUV_out[0][j][0] ), img_width_cr );
		}

		_write ( p_dec, buf, img_height_cr * img_width_cr );

		for ( j = 0; j < img_height_cr; j++ )
		{
			memcpy ( buf + j * img_width_cr, & ( imgUV_out[1][j][0] ), img_width_cr );
		}

		_write ( p_dec, buf, img_height_cr * img_width_cr );
#if REF_OUTPUT_BUG_F
#ifdef WIN32
		free ( buf );
#endif
#endif
#endif
#else
#if EXTEND_BD
	shift1 = input->sample_bit_depth - input->input_sample_bit_depth;
	if (!shift1 && input->input_sample_bit_depth == 8) // 8bit input -> 8bit encoding
	{
		for ( i=0; i < img_height; i++ )
		{
			for ( j=0; j < img_width; j++ )
			{
				bufc = (unsigned char)(imgY_out[i][j]);
				fputc ( bufc, p_dec );
			}
		}
		for ( i=0; i < img_height_cr; i++ )
		{
			for ( j=0; j < img_width_cr; j++ )
			{
				bufc = (unsigned char)(imgUV_out[0][i][j]);
				fputc ( bufc, p_dec );
			}
		}
		for ( i=0; i < img_height_cr; i++ )
		{
			for ( j=0; j < img_width_cr; j++ )
			{
				bufc = (unsigned char)(imgUV_out[1][i][j]);
				fputc ( bufc, p_dec );
			}
		}
	}
	else if (!shift1 && input->input_sample_bit_depth > 8) // 10/12bit input -> 10/12bit encoding
	{
		for ( i=0; i < img_height; i++ )
		{
			for ( j=0; j < img_width; j++ )
			{
				bufc = (unsigned char)(imgY_out[i][j]);
				fputc ( bufc, p_dec );
				bufc = (unsigned char)(imgY_out[i][j] >> 8);
				fputc (bufc, p_dec);
			}
		}
		for ( i=0; i < img_height_cr; i++ )
		{
			for ( j=0; j < img_width_cr; j++ )
			{
				bufc = (unsigned char)(imgUV_out[0][i][j]);
				fputc ( bufc, p_dec );
				bufc = (unsigned char)(imgUV_out[0][i][j] >> 8);
				fputc (bufc, p_dec);
			}
		}
		for ( i=0; i < img_height_cr; i++ )
		{
			for ( j=0; j < img_width_cr; j++ )
			{
				bufc = (unsigned char)(imgUV_out[1][i][j]);
				fputc ( bufc, p_dec );
				bufc = (unsigned char)(imgUV_out[1][i][j] >> 8);
				fputc (bufc, p_dec);
			}
		}
	}
	else if (shift1 && input->input_sample_bit_depth == 8) // 8bit input -> 10/12bit encoding
	{
		for ( i=0; i < img_height; i++ )
		{
			for ( j=0; j < img_width; j++ )
			{
				bufc = (unsigned char)Clip1((imgY_out[i][j] + (1 << (shift1 - 1))) >> shift1);
				fputc ( bufc, p_dec );
			}
		}
		for ( i=0; i < img_height_cr; i++ )
		{
			for ( j=0; j < img_width_cr; j++ )
			{
				bufc = (unsigned char)Clip1((imgUV_out[0][i][j] + (1 << (shift1 - 1))) >> shift1);
				fputc ( bufc, p_dec );
			}
		}
		for ( i=0; i < img_height_cr; i++ )
		{
			for ( j=0; j < img_width_cr; j++ )
			{
				bufc = (unsigned char)Clip1((imgUV_out[1][i][j] + (1 << (shift1 - 1))) >> shift1);
				fputc ( bufc, p_dec );
			}
		}
	}
	fflush ( p_dec );
#else
	for ( i=0; i < img_height; i++ )
     {
       for ( j=0; j < img_width; j++ )
       {
         fputc( imgY_out[i][j], p_dec );
       }
     }
     for ( i=0; i < img_height_cr; i++ )
     {
       for ( j=0; j < img_width_cr; j++ )
       {
         fputc( imgUV_out[0][i][j], p_dec );
       }
     }
    for ( i=0; i < img_height_cr; i++ )
     {
       for ( j=0; j < img_width_cr; j++ )
       {
         fputc( imgUV_out[1][i][j], p_dec );
       }
     }
     fflush ( p_dec );
#endif
#endif

	}
#if !REF_OUTPUT_BUG_F
#ifdef WIN32
	free ( buf );
#endif
#endif
}
#else
#ifdef AVS2_S2_BGLONGTERM
/*
*************************************************************************
* Function:Writes GB reconstructed image(s) to file
This can be done more elegant!
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
void write_GB_frame(int p_dec) // not working in 10bit profile
{
#ifdef WIN32
	int l;
	for(l=0; l<img->height; l++)
	{
		_write(p_dec,imgY[l],img->width);
	}
	for(l=0; l<img->height_cr; l++)
	{
		_write(p_dec,imgUV[0][l],img->width_cr);
	}
	for(l=0; l<img->height_cr; l++)
	{
		_write(p_dec,imgUV[1][l],img->width_cr);
	}
#else
	int i,j;
    for ( i=0; i < img->height; i++ )
    {
      for ( j=0; j < img->width; j++ )
      {
        fputc( imgY[i][j], p_dec );
      }
    }
    for ( i=0; i < img->height_cr; i++ )
    {
      for ( j=0; j < img->width_cr; j++ )
      {
        fputc( imgUV[0][i][j], p_dec );
      }
    }
    for ( i=0; i < img->height_cr; i++ )
    {
      for ( j=0; j < img->width_cr; j++ )
      {
        fputc( imgUV[1][i][j], p_dec );
      }
    }
#endif

}
#endif //AVS2_S2_BGLONGTERM
#endif //AVS2_SCENE_CD
/*
*************************************************************************
* Function:Writes reconstructed image(s) to file
This can be done more elegant!
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
// Fix by Sunil for RD5.0 test in Linux (2013.11.06)
#ifdef WIN32
void write_frame ( int p_dec , int pos)         //!< filestream to output file
#else
void write_frame ( FILE *p_dec , int pos)         //!< filestream to output file
#endif
{
	int j;
// Fix by Sunil for RD5.0 test in Linux (2013.11.06)
#ifndef WIN32
  int i;
#endif
	int img_width = ( img->width - img->auto_crop_right );
	int img_height = ( img->height - img->auto_crop_bottom );
	int img_width_cr = ( img_width / 2 );
	int img_height_cr = ( img_height / ( input->chroma_format == 1 ? 2 : 1 ) );
// Fix by Sunil for RD5.0 test in Linux (2013.11.06)
#ifdef WIN32
	unsigned char *buf;
#endif
#if EXTEND_BD
	byte **imgY_out ;
	byte ***imgUV_out ;
	int nSampleSize;
	int k;
	int shift1;
#else
	unsigned char **imgY_out ;
	unsigned char ***imgUV_out ;
#endif
	unsigned char bufc;
	if ( p_dec != -1 )
	{
		imgY_out = ref[pos][0];
		imgUV_out= &ref[pos][1];
		img->is_output[pos] = -1;
		if ( img->refered_by_others[pos]==0 || img->imgcoi_ref[pos] == -257 )
		{
			img->imgtr_fwRefDistance[pos] = -256;
			img->imgcoi_ref[pos] = -257;
#if M3480_TEMPORAL_SCALABLE
			img->temporal_id[pos] = -1;
#endif
		}

// Fix by Sunil for RD5.0 test in Linux (2013.11.06)
#ifdef WIN32
#if EXTEND_BD
		// assume "input->sample_bit_depth" is greater or equal to "input->input_sample_bit_depth"
		if (input->input_sample_bit_depth == 8)
		{
			nSampleSize = 1;
		}
		else if (input->input_sample_bit_depth > 8)
		{
			nSampleSize = 2;
		}
		buf = malloc ( img_height * img_width * nSampleSize );
		shift1 = input->sample_bit_depth - input->input_sample_bit_depth;
		
		if (!shift1 && input->input_sample_bit_depth == 8) // 8bit input -> 8bit encode
		{
			for (j = 0; j < img_height; j++)
			{
				for (k = 0; k < img_width; k++)
				{
					buf[j * img_width + k] = (unsigned char)imgY_out[j][k];
				}
			}
			_write ( p_dec, buf, img_height * img_width );
			for ( j = 0; j < img_height_cr; j++ )
			{
				for (k = 0; k < img_width_cr; k++)
				{
					buf[j * img_width_cr + k] = (unsigned char)imgUV_out[0][j][k];
				}
			}
			_write ( p_dec, buf, img_height_cr * img_width_cr );

			for ( j = 0; j < img_height_cr; j++ )
			{
				for (k = 0; k < img_width_cr; k++)
				{
					buf[j * img_width_cr + k] = (unsigned char)imgUV_out[1][j][k];
				}
			}
			_write ( p_dec, buf, img_height_cr * img_width_cr );
		}
		else if (!shift1 && input->input_sample_bit_depth > 8) // 10/12bit input -> 10/12bit encode
		{
			for (j = 0; j < img_height; j++)
			{
				memcpy ( buf + j * img_width * nSampleSize, & ( imgY_out[j][0] ), img_width * sizeof(byte) );
			}
			_write ( p_dec, buf, img_height * img_width * sizeof(byte));
			for ( j = 0; j < img_height_cr; j++ )
			{
				memcpy ( buf + j * img_width_cr * nSampleSize, & ( imgUV_out[0][j][0] ), img_width_cr * sizeof(byte) );
			}
			_write ( p_dec, buf, img_height_cr * img_width_cr * sizeof(byte) );

			for ( j = 0; j < img_height_cr; j++ )
			{
				memcpy ( buf + j * img_width_cr * nSampleSize, & ( imgUV_out[1][j][0] ), img_width_cr * sizeof(byte) );
			}
			_write ( p_dec, buf, img_height_cr * img_width_cr * sizeof(byte) );
		}
		else if (shift1 && input->input_sample_bit_depth == 8) // 8bit input -> 10/12bit encode
		{
			for (j = 0; j < img_height; j++)
			{
				for (k = 0; k < img_width; k++)
				{
					buf[j * img_width + k] = (unsigned char)Clip1((imgY_out[j][k] + (1 << (shift1 - 1))) >> shift1);
				}
			}
			_write ( p_dec, buf, img_height * img_width );
			for ( j = 0; j < img_height_cr; j++ )
			{
				for (k = 0; k < img_width_cr; k++)
				{
					buf[j * img_width_cr + k] = (unsigned char)Clip1((imgUV_out[0][j][k] + (1 << (shift1 - 1))) >> shift1);
				}
			}
			_write ( p_dec, buf, img_height_cr * img_width_cr );

			for ( j = 0; j < img_height_cr; j++ )
			{
				for (k = 0; k < img_width_cr; k++)
				{
					buf[j * img_width_cr + k] = (unsigned char)Clip1((imgUV_out[1][j][k] + (1 << (shift1 - 1))) >> shift1);
				}
			}
			_write ( p_dec, buf, img_height_cr * img_width_cr );
		}
#if REF_OUTPUT_BUG_F
		free ( buf );
#endif
#else
		buf = malloc ( img_height * img_width );

		for ( j = 0; j < img_height; j++ )
		{
			memcpy ( buf + j * img_width, & ( imgY_out[j][0] ), img_width );
		}

		_write ( p_dec, buf, img_height * img_width );

		for ( j = 0; j < img_height_cr; j++ )
		{
			memcpy ( buf + j * img_width_cr, & ( imgUV_out[0][j][0] ), img_width_cr );
		}

		_write ( p_dec, buf, img_height_cr * img_width_cr );

		for ( j = 0; j < img_height_cr; j++ )
		{
			memcpy ( buf + j * img_width_cr, & ( imgUV_out[1][j][0] ), img_width_cr );
		}

		_write ( p_dec, buf, img_height_cr * img_width_cr );
#if REF_OUTPUT_BUG_F
#ifdef WIN32
		free ( buf );
#endif
#endif
#endif
#else
#if EXTEND_BD
	shift1 = input->sample_bit_depth - input->input_sample_bit_depth;
	if (!shift1 && input->input_sample_bit_depth == 8) // 8bit input -> 8bit encoding
	{
		for ( i=0; i < img_height; i++ )
		{
			for ( j=0; j < img_width; j++ )
			{
				bufc = (unsigned char)(imgY_out[i][j]);
				fputc ( bufc, p_dec );
			}
		}
		for ( i=0; i < img_height_cr; i++ )
		{
			for ( j=0; j < img_width_cr; j++ )
			{
				bufc = (unsigned char)(imgUV_out[0][i][j]);
				fputc ( bufc, p_dec );
			}
		}
		for ( i=0; i < img_height_cr; i++ )
		{
			for ( j=0; j < img_width_cr; j++ )
			{
				bufc = (unsigned char)(imgUV_out[1][i][j]);
				fputc ( bufc, p_dec );
			}
		}
	}
	else if (!shift1 && input->input_sample_bit_depth > 8) // 10/12bit input -> 10/12bit encoding
	{
		for ( i=0; i < img_height; i++ )
		{
			for ( j=0; j < img_width; j++ )
			{
				bufc = (unsigned char)(imgY_out[i][j]);
				fputc ( bufc, p_dec );
				bufc = (unsigned char)(imgY_out[i][j] >> 8);
				fputc (bufc, p_dec);
			}
		}
		for ( i=0; i < img_height_cr; i++ )
		{
			for ( j=0; j < img_width_cr; j++ )
			{
				bufc = (unsigned char)(imgUV_out[0][i][j]);
				fputc ( bufc, p_dec );
				bufc = (unsigned char)(imgUV_out[0][i][j] >> 8);
				fputc (bufc, p_dec);
			}
		}
		for ( i=0; i < img_height_cr; i++ )
		{
			for ( j=0; j < img_width_cr; j++ )
			{
				bufc = (unsigned char)(imgUV_out[1][i][j]);
				fputc ( bufc, p_dec );
				bufc = (unsigned char)(imgUV_out[1][i][j] >> 8);
				fputc (bufc, p_dec);
			}
		}
	}
	else if (shift1 && input->input_sample_bit_depth == 8) // 8bit input -> 10/12bit encoding
	{
		for ( i=0; i < img_height; i++ )
		{
			for ( j=0; j < img_width; j++ )
			{
				bufc = (unsigned char)Clip1((imgY_out[i][j] + (1 << (shift1 - 1))) >> shift1);
				fputc ( bufc, p_dec );
			}
		}
		for ( i=0; i < img_height_cr; i++ )
		{
			for ( j=0; j < img_width_cr; j++ )
			{
				bufc = (unsigned char)Clip1((imgUV_out[0][i][j] + (1 << (shift1 - 1))) >> shift1);
				fputc ( bufc, p_dec );
			}
		}
		for ( i=0; i < img_height_cr; i++ )
		{
			for ( j=0; j < img_width_cr; j++ )
			{
				bufc = (unsigned char)Clip1((imgUV_out[1][i][j] + (1 << (shift1 - 1))) >> shift1);
				fputc ( bufc, p_dec );
			}
		}
	}
	fflush ( p_dec );
#else
	for ( i=0; i < img_height; i++ )
     {
       for ( j=0; j < img_width; j++ )
       {
         fputc( imgY_out[i][j], p_dec );
       }
     }
     for ( i=0; i < img_height_cr; i++ )
     {
       for ( j=0; j < img_width_cr; j++ )
       {
         fputc( imgUV_out[0][i][j], p_dec );
       }
     }
    for ( i=0; i < img_height_cr; i++ )
     {
       for ( j=0; j < img_width_cr; j++ )
       {
         fputc( imgUV_out[1][i][j], p_dec );
       }
     }
     fflush ( p_dec );
#endif
#endif

	}
#if !REF_OUTPUT_BUG_F
#ifdef WIN32
	free ( buf );
#endif
#endif
}

static void write_reconstructed_image ()
{
	int i;

#if !REF_OUTPUT
	for ( i=0; i<REF_MAXBUFFER; i++)
	{
		int output_next = min_tr(img->imgtr_fwRefDistance,last_output,&pos);
		if ( output_next < img->tr || output_next == (last_output+1) )
		{
			last_output = output_next;
			write_frame( p_dec, pos );
			i--;
		}
		else
		{
			break;
		}
	}
#else 
	if (coding_order + total_frames*256 >= picture_reorder_delay)
	{
		int tmp_min,pos=-1;
		tmp_min = 1<<20;
		for ( i=0; i<REF_MAXBUFFER; i++)
		{
			if (img->imgtr_fwRefDistance[i]<tmp_min && img->imgtr_fwRefDistance[i]>last_output)
			{
				tmp_min = img->imgtr_fwRefDistance[i];
				pos=i;
			}
		}
		if (pos!=-1)
		{
			last_output=tmp_min;
			write_frame( p_dec, pos );
		}

	}
#endif 
}
/*
*************************************************************************
* Function:Write previous decoded P frame to output file
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

#ifdef WIN32
void write_prev_Pframe ()
#else
void write_prev_Pframe ()
#endif
{
  int j;
  int writeout_width = input->img_width;
  int writeout_height = input->img_height;
  int writeout_width_cr =  input->img_width/ 2;
  int writeout_height_cr = input->img_height/2;

#ifdef WIN32

  unsigned char *buf;
  buf = malloc ( writeout_height * writeout_width );

  for ( j = 0; j < writeout_height; j++ )
  {
    memcpy ( buf + j * writeout_width, & ( imgYPrev[j][0] ), writeout_width );
  }

  _write ( p_dec, buf, writeout_height * writeout_width );

  for ( j = 0; j < writeout_height_cr; j++ )
  {
    memcpy ( buf + j * writeout_width_cr, & ( imgUVPrev[0][j][0] ), writeout_width_cr );
  }

  _write ( p_dec, buf, writeout_height_cr * writeout_width_cr );

  for ( j = 0; j < writeout_height_cr; j++ )
  {
    memcpy ( buf + j * writeout_width_cr, & ( imgUVPrev[1][j][0] ), writeout_width_cr );
  }

  _write ( p_dec, buf, writeout_height_cr * writeout_width_cr );

  free ( buf );

#else
  {
    int i;

    for ( i = 0; i < writeout_height; i++ )
    {
      for ( j = 0; j < writeout_width; j++ )
      {
        fputc ( imgYPrev[i][j], p_dec );
      }
    }

    for ( i = 0; i < writeout_height_cr; i++ )
    {
      for ( j = 0; j < writeout_width_cr; j++ )
      {
        fputc ( imgUVPrev[0][i][j], p_dec );
      }
    }

    for ( i = 0; i < writeout_height_cr; i++ )
    {
      for ( j = 0; j < writeout_width_cr; j++ )
      {
        fputc ( imgUVPrev[1][i][j], p_dec );
      }
    }

    fflush ( p_dec );
  }
#endif
}

/*
*************************************************************************
* Function:Choose interpolation method depending on MV-resolution
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void interpolate_frame_to_fb ()
{
  // write to oneForthRefY[]
  UnifiedOneForthPix ( imgY, imgUV[0], imgUV[1], oneForthRefY[0] );
}

/*
*************************************************************************
* Function:Upsample 4 times, store them in out4x.  Color is simply copied
* Input:srcy, srcu, srcv, out4y, out4u, out4v
* Output:
* Return:
* Attention:Side Effects_
Uses (writes) img4Y_tmp.  This should be moved to a static variable
in this module
*************************************************************************
*/
#define  IClip( Min, Max, Val) (((Val)<(Min))? (Min):(((Val)>(Max))? (Max):(Val)))

static void UnifiedOneForthPix ( pel_t ** imgY, pel_t ** imgU, pel_t ** imgV,
                                 pel_t ** out4Y )
{
#if EXTEND_BD
 	int max_pel_value = (1 << input->sample_bit_depth) - 1;
	int shift1 = input->sample_bit_depth - 8;
	int shift2 = 14 - input->sample_bit_depth;
	int shift3 = 20 - input->sample_bit_depth;
#else
	int max_pel_value = 255;
#endif
  int is;
  int temp;
  int dx, dy;
  int x, y;
  int i, j;// j4;
  int ii, jj;

  static const int COEF[3][8] =
  {
    { -1, 4, -10, 57, 19,  -7, 3, -1 },
    { -1, 4, -11, 40, 40, -11, 4, -1 },
    { -1, 3,  -7, 19, 57, -10, 4, -1 }
  };



  //    A  a  1  b  B
  //    c  d  e  f
  //    2  h  3  i
  //    j  k  l  m
  //    C           D

  //fullpel position: A
  for ( j = -IMG_PAD_SIZE; j < img->height + IMG_PAD_SIZE; j++ )
  {
    for ( i = -IMG_PAD_SIZE; i < img->width + IMG_PAD_SIZE; i++ )
    {
      img4Y_tmp[ ( j + IMG_PAD_SIZE ) * 4][ ( i + IMG_PAD_SIZE ) * 4] =
        imgY[IClip ( 0, img->height - 1, j ) ][IClip ( 0, img->width - 1, i ) ];
    }
  }

  //horizontal positions: a,1,b
  for ( j = -IMG_PAD_SIZE; j < img->height + IMG_PAD_SIZE; j++ )
  {
    for ( i = -IMG_PAD_SIZE; i < img->width + IMG_PAD_SIZE; i++ )
    {
      jj = IClip ( 0, img->height - 1, j );

      for ( dx = 1; dx < 4; dx++ )
      {
        for ( is = 0, x = -3; x < 5; x++ )
        {
          is += imgY[jj][IClip ( 0, img->width - 1, i + x ) ] * COEF[dx - 1][x + 3];
        }
        img4Y_tmp[ ( j + IMG_PAD_SIZE ) * 4][ ( i + IMG_PAD_SIZE ) * 4 + dx] = is;
      }
    }
  }

  //vertical positions: c,2,j
  for ( j = -IMG_PAD_SIZE; j < img->height + IMG_PAD_SIZE; j++ )
  {
    for ( i = -IMG_PAD_SIZE; i < img->width + IMG_PAD_SIZE; i++ )
    {
      ii = IClip ( 0, img->width - 1, i );

      for ( dy = 1; dy < 4; dy++ )
      {

        for ( is = 0, y = -3; y < 5; y++ )
        {
          is += imgY[IClip ( 0, img->height - 1, j + y ) ][ii] * COEF[dy - 1][y + 3];
        }

        is = IClip ( 0, max_pel_value, ( is + 32 ) / 64 );

        img4Y_tmp[ ( j + IMG_PAD_SIZE ) * 4 + dy][ ( i + IMG_PAD_SIZE ) * 4] = is;
      }
    }
  }


  //vertical positions: d,h,k; e,3,1; f,i,m
  for ( i = 0; i < ( img->width  + 2 * IMG_PAD_SIZE ) * 4; i += 4 )
  {
    for ( j = 0; j < ( img->height  + 2 * IMG_PAD_SIZE ) * 4; j += 4 )
    {
      for ( dx = 1; dx < 4; dx++ )
      {
        for ( dy = 1; dy < 4; dy++ )
        {

          for ( is = 0, y = -3; y < 5; y++ )
          {
            jj = IClip ( 0, ( img->height + 2 * IMG_PAD_SIZE ) * 4 - 4, j + 4 * y );
            ii = IClip ( 0, ( img->width + 2 * IMG_PAD_SIZE ) * 4 - 4, i + dx );
          //  is += img4Y_tmp[jj][ii] * COEF[dy - 1][y + 3];
#if EXTEND_BD
			if (shift1)
            {
              is += ((img4Y_tmp[jj][ii] + (1 << (shift1 - 1))) >> shift1) * COEF[dy - 1][y + 3];
            }
            else
            {
#endif
              is += img4Y_tmp[jj][ii] * COEF[dy - 1][y + 3];
#if EXTEND_BD
            }
#endif
          }

       //   img4Y_tmp[j  + dy][i + dx] = IClip ( 0, max_pel_value, ( is + 2048 ) / 4096 );
#if EXTEND_BD
		  img4Y_tmp[j  + dy][i + dx] = IClip ( 0, max_pel_value, ( is + (1 << (shift3 - 1)) ) >> shift3 );
#else
			img4Y_tmp[j  + dy][i + dx] = IClip ( 0, max_pel_value, ( is + 2048 ) >> 12 );
#endif
        }
      }
    }
  }

  //positions: a,1,b
  for ( j = 0; j < ( img->height  + 2 * IMG_PAD_SIZE ) * 4; j += 4 )
  {
    for ( i = 0; i < ( img->width  + 2 * IMG_PAD_SIZE ) * 4; i ++ )
    {
      if ( i % 4 != 0 )
      {
        temp = IClip ( 0, max_pel_value, ( img4Y_tmp[j][i] + 32 ) >> 6 );
        img4Y_tmp[j][i] = temp;
      }
    }
  }


  for ( j = 0; j < ( img->height  + 2 * IMG_PAD_SIZE ) * 4; j ++ )
  {
    for ( i = 0; i < ( img->width  + 2 * IMG_PAD_SIZE ) * 4; i ++ )
    {
      PutPel_14 ( out4Y, j - IMG_PAD_SIZE * 4, i - IMG_PAD_SIZE * 4, ( pel_t ) img4Y_tmp[j][i] );
    }
  }

}

/*
*************************************************************************
* Function:Find SNR for all three components
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

static void find_snr ()
{
  int i, j;
#if EXTEND_BD
#ifdef WIN32
  __int64 diff_y, diff_u, diff_v;
#else
  int64_t diff_y, diff_u, diff_v;
#endif
#else
	int diff_y, diff_u, diff_v;
#endif
  int impix;
  int uvformat = input->chroma_format == 1 ? 4 : 2;

#if EXTEND_BD
  double maxSignal = (double)((1 << input->input_sample_bit_depth) - 1) * (double)((1 << input->input_sample_bit_depth) - 1);
  int shift1 = input->sample_bit_depth - input->input_sample_bit_depth;
#endif
  //  Calculate  PSNR for Y, U and V.
  //     Luma.
  impix = input->img_height * input->img_width;

	diff_y = 0;

#if !EXTEND_BD
  for ( i = 0; i < input->img_width; ++i )
  {
    for ( j = 0; j < input->img_height; ++j )
    {
      diff_y += img->quad[imgY_org[j][i] - imgY[j][i]];
    }
  }

	//     Chroma. 
#endif

	diff_u = 0;
	diff_v = 0;

#if EXTEND_BD
	if (shift1 != 0)
	{
		for ( i = 0; i < input->img_width; ++i )
		{
			for ( j = 0; j < input->img_height; ++j )
			{
				diff_y += img->quad[(imgY_org[j][i] >> shift1) - Clip1((imgY[j][i] + (1 << (shift1 - 1))) >> shift1)];
			}
		}
		for ( i = 0; i < input->img_width / 2; i++ )
		{
			for ( j = 0; j < input->img_height / 2; j++ )
			{
				diff_u += img->quad[(imgUV_org[0][j][i] >> shift1) - Clip1((imgUV[0][j][i] + (1 << (shift1 - 1))) >> shift1)];
				diff_v += img->quad[(imgUV_org[1][j][i] >> shift1) - Clip1((imgUV[1][j][i] + (1 << (shift1 - 1))) >> shift1)];
			}
		}
	}
	else
	{
		for ( i = 0; i < input->img_width; ++i )
		{
			for ( j = 0; j < input->img_height; ++j )
			{
				diff_y += img->quad[imgY_org[j][i] - imgY[j][i]];
			}
		}
#endif
		for ( i = 0; i < input->img_width / 2; i++ )
		{
			for ( j = 0; j < input->img_height / 2; j++ )
			{
				diff_u += img->quad[imgUV_org[0][j][i] - imgUV[0][j][i]];
				diff_v += img->quad[imgUV_org[1][j][i] - imgUV[1][j][i]];
			}
		}
#if EXTEND_BD
	}
#endif

  //  Collecting SNR statistics
	if ( diff_y != 0 )
  {
#if EXTEND_BD
    snr->snr_y = ( 10.0 * log10 ( maxSignal * ( double ) impix / ( double ) diff_y ) ); // luma snr for current frame
    snr->snr_u = ( 10.0 * log10 ( maxSignal * ( double ) impix / ( double ) ( /*4*/uvformat * diff_u ) ) ); // u croma snr for current frame, 1/4 of luma samples
    snr->snr_v = ( 10.0 * log10 ( maxSignal * ( double ) impix / ( double ) ( /*4*/uvformat * diff_v ) ) ); // v croma snr for current frame, 1/4 of luma samples
#else
    snr->snr_y = ( 10.0 * log10 ( 65025.0 * ( double ) impix / ( double ) diff_y ) ); // luma snr for current frame
    snr->snr_u = ( 10.0 * log10 ( 65025.0 * ( double ) impix / ( double ) ( /*4*/uvformat * diff_u ) ) ); // u croma snr for current frame, 1/4 of luma samples
    snr->snr_v = ( 10.0 * log10 ( 65025.0 * ( double ) impix / ( double ) ( /*4*/uvformat * diff_v ) ) ); // v croma snr for current frame, 1/4 of luma samples
#endif
	}

  if ( img->number == 0 )
  {
#if EXTEND_BD
    snr->snr_y1 = ( 10.0 * log10 ( maxSignal * ( double ) impix / ( double ) diff_y ) ); // keep luma snr for first frame
    snr->snr_u1 = ( 10.0 * log10 ( maxSignal * ( double ) impix / ( double ) ( /*4*/uvformat * diff_u ) ) ); // keep croma u snr for first frame
    snr->snr_v1 = ( 10.0 * log10 ( maxSignal * ( double ) impix / ( double ) ( /*4*/uvformat * diff_v ) ) ); // keep croma v snr for first frame
#else
    snr->snr_y1 = ( 10.0 * log10 ( 65025.0 * ( double ) impix / ( double ) diff_y ) ); // keep luma snr for first frame
    snr->snr_u1 = ( 10.0 * log10 ( 65025.0 * ( double ) impix / ( double ) ( /*4*/uvformat * diff_u ) ) ); // keep croma u snr for first frame
    snr->snr_v1 = ( 10.0 * log10 ( 65025.0 * ( double ) impix / ( double ) ( /*4*/uvformat * diff_v ) ) ); // keep croma v snr for first frame
#endif
		snr->snr_ya = snr->snr_y1;
    snr->snr_ua = snr->snr_u1;
    snr->snr_va = snr->snr_v1;
  }
  // B pictures
  else
  {
#ifdef AVS2_S2_BGLONGTERM
#if AVS2_SCENE_CD
      if(input->bg_enable)
#else
	  if(input->profile_id == 0x50 )
#endif
	  {
		  if(!(img->typeb == BACKGROUND_IMG && background_output_flag == 0))
		  {
			snr->snr_ya = ( snr->snr_ya * (double) ( img->number - background_number + Bframe_ctr ) + snr->snr_y ) / (double) ( img->number - background_number + Bframe_ctr + 1 ); // average snr lume for all frames inc. first
			snr->snr_ua = ( snr->snr_ua * (double) ( img->number - background_number + Bframe_ctr ) + snr->snr_u ) / (double) ( img->number - background_number + Bframe_ctr + 1 ); // average snr u croma for all frames inc. first
			snr->snr_va = ( snr->snr_va * (double) ( img->number - background_number + Bframe_ctr ) + snr->snr_v ) / (double) ( img->number - background_number + Bframe_ctr + 1 ); // average snr v croma for all frames inc. first
		  }
	  }
	  else
	  {
        snr->snr_ya = ( snr->snr_ya * (double) ( img->number + Bframe_ctr ) + snr->snr_y ) / (double) ( img->number + Bframe_ctr + 1 ); // average snr lume for all frames inc. first
        snr->snr_ua = ( snr->snr_ua * (double) ( img->number + Bframe_ctr ) + snr->snr_u ) / (double) ( img->number + Bframe_ctr + 1 ); // average snr u croma for all frames inc. first    
        snr->snr_va = ( snr->snr_va * (double) ( img->number + Bframe_ctr ) + snr->snr_v ) / (double) ( img->number + Bframe_ctr + 1 ); // average snr v croma for all frames inc. first
	  }
#else
      snr->snr_ya = ( snr->snr_ya * (double) ( img->number + Bframe_ctr ) + snr->snr_y ) / (double) ( img->number + Bframe_ctr + 1 ); // average snr lume for all frames inc. first
      snr->snr_ua = ( snr->snr_ua * (double) ( img->number + Bframe_ctr ) + snr->snr_u ) / (double) ( img->number + Bframe_ctr + 1 ); // average snr u croma for all frames inc. first
      snr->snr_va = ( snr->snr_va * (double) ( img->number + Bframe_ctr ) + snr->snr_v ) / (double) ( img->number + Bframe_ctr + 1 ); // average snr v croma for all frames inc. first
#endif
  }
}

/*
*************************************************************************
* Function:
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

static void ReportFirstframe ( int tmp_time )
{
  char *Frmfld;
  char Frm[] = "FRM";
  char Fld[] = "FLD";
  FILE *file = fopen ( "stat.dat", "at" );
  fprintf ( file, "\n -------------------- DEBUG_INFO_START -------------------- \n" );
  if (img->picture_structure == 1)
  {
    Frmfld = Frm;
  }
  else
  {
    Frmfld = Fld;
  }
#if INTERLACE_CODING
  if (input->InterlaceCodingOption == 3)
  {
    Frmfld = Fld;
  }
#endif

#if MD5
  if(img->typeb == BACKGROUND_IMG && input->bg_enable)
  {
	  if(input->bg_input_number==0)
	  {
		  fprintf ( file, "    %3d(GB) %8lld %4d %7.4f %7.4f %7.4f  %5d          %3d      %3s\t\t%s\n",
			  img->tr, stat->bit_ctr - stat->bit_ctr_n,
			  img->qp, snr->snr_y, snr->snr_u, snr->snr_v, tmp_time,
			  intras, Frmfld, MD5str );

		  printf ( "    %3d(GB) %8lld %4d %7.4f %7.4f %7.4f  %5d       %s\t\t%s\n",
			  img->tr, stat->bit_ctr - stat->bit_ctr_n,
			  img->qp, snr->snr_y, snr->snr_u, snr->snr_v, tmp_time, Frmfld, MD5str );
	  }
	  else
	  {
		  fprintf ( file, "     %3d(G) %8lld %4d %7.4f %7.4f %7.4f  %5d          %3d      %3s\t\t%s\n",
			  img->tr, stat->bit_ctr - stat->bit_ctr_n,
			  img->qp, snr->snr_y, snr->snr_u, snr->snr_v, tmp_time,
			  intras, Frmfld, MD5str );

		  printf ( "     %3d(G) %8lld %4d %7.4f %7.4f %7.4f  %5d       %s\t\t%s\n",
			  img->tr, stat->bit_ctr - stat->bit_ctr_n,
			  img->qp, snr->snr_y, snr->snr_u, snr->snr_v, tmp_time, Frmfld, MD5str );
	  }
	  fclose( file );
  }
  else
  {
	  fprintf ( file, "%3d(I) %8lld %4d %7.4f %7.4f %7.4f  %5d          %3d      %3s\t\t%s\n",
		  img->tr, stat->bit_ctr - stat->bit_ctr_n,
		  img->qp, snr->snr_y, snr->snr_u, snr->snr_v, tmp_time,
		  intras, Frmfld, MD5str );

	  fclose ( file );
	  printf ( "%3d(I) %8lld %4d %7.4f %7.4f %7.4f  %5d       %s\t\t%s\n",
		  img->tr, stat->bit_ctr - stat->bit_ctr_n,
		  img->qp, snr->snr_y, snr->snr_u, snr->snr_v, tmp_time, Frmfld, MD5str );
  }

#else

#ifdef AVS2_S2_BGLONGTERM
#if AVS2_SCENE_CD
  if(img->typeb == BACKGROUND_IMG && input->bg_enable)
#else
  if(img->typeb == BACKGROUND_IMG && input->bg_enable && input->profile_id == 0x50)
#endif
  {
	  if(input->bg_input_number==0)
	  {
		  fprintf ( file, "%3d(GB) %8lld %4d %7.4f %7.4f %7.4f  %5d          %3d      %3s\n",
					img->tr, stat->bit_ctr - stat->bit_ctr_n,
					img->qp, snr->snr_y, snr->snr_u, snr->snr_v, tmp_time,
					intras, Frmfld );

		  printf ( "%3d(GB) %8lld %4d %7.4f %7.4f %7.4f  %5d       %s \n",
				   img->tr, stat->bit_ctr - stat->bit_ctr_n,
				   img->qp, snr->snr_y, snr->snr_u, snr->snr_v, tmp_time, Frmfld );
	  }
	  else
	  {
		  fprintf ( file, "%3d(G) %8lld %4d %7.4f %7.4f %7.4f  %5d          %3d      %3s\n",
					img->tr, stat->bit_ctr - stat->bit_ctr_n,
					img->qp, snr->snr_y, snr->snr_u, snr->snr_v, tmp_time,
					intras, Frmfld );

		  printf ( "%3d(G) %8lld %4d %7.4f %7.4f %7.4f  %5d       %s \n",
				   img->tr, stat->bit_ctr - stat->bit_ctr_n,
				   img->qp, snr->snr_y, snr->snr_u, snr->snr_v, tmp_time, Frmfld );
	  }

	  fclose( file );
  }
  else
  {
#endif
  fprintf ( file, "%3d(I) %8lld %4d %7.4f %7.4f %7.4f  %5d          %3d      %3s\n",
	  img->tr, stat->bit_ctr - stat->bit_ctr_n,
	  img->qp, snr->snr_y, snr->snr_u, snr->snr_v, tmp_time,
	  intras, Frmfld );

  fclose ( file );
  printf ( "%3d(I) %8lld %4d %7.4f %7.4f %7.4f  %5d       %s \n",
	  img->tr, stat->bit_ctr - stat->bit_ctr_n,
	  img->qp, snr->snr_y, snr->snr_u, snr->snr_v, tmp_time, Frmfld );
#ifdef AVS2_S2_BGLONGTERM
  }
#endif

#endif
  stat->bitr0 = stat->bitr;
  stat->bit_ctr_0 = stat->bit_ctr;
  //  stat->bit_ctr = 0;   //rm52k

}

/*
*************************************************************************
* Function:
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
static void Report_frame ( int tmp_time )
{

  FILE *file = fopen ( "stat.dat", "at" );
  char *Frmfld;
  char Frm[] = "FRM";
  char Fld[] = "FLD";
  if (img->picture_structure)
  {
    Frmfld = Frm;
  }
  else
  {
    Frmfld = Fld;
  }
#if INTERLACE_CODING  
  if (input->InterlaceCodingOption == 3)  
  {
    Frmfld = Fld;
  }
#endif

#if MD5
  if(img->typeb == BACKGROUND_IMG && input->bg_enable)
  {
	  if(input->bg_input_number==0)
	  {
		  fprintf ( file, "    %3d(GB) %8lld %4d %7.4f %7.4f %7.4f  %5d             %s\n",
			  img->tr, stat->bit_ctr - stat->bit_ctr_n,
			  img->qp, snr->snr_y, snr->snr_u, snr->snr_v, tmp_time, MD5str);

		  fclose( file );
		  printf ( "    %3d(GB) %8lld %4d %7.4f %7.4f %7.4f  %5d       %3s     %s\n",
			  img->tr+total_frames*256, stat->bit_ctr - stat->bit_ctr_n,
			  img->qp, snr->snr_y, snr->snr_u, snr->snr_v, tmp_time, Frmfld, MD5str);
	  }
	  else
	  {
		  fprintf ( file, "     %3d(G) %8lld %4d %7.4f %7.4f %7.4f  %5d             %s\n",
			  img->tr, stat->bit_ctr - stat->bit_ctr_n,
			  img->qp, snr->snr_y, snr->snr_u, snr->snr_v, tmp_time, MD5str );

		  fclose( file );
		  printf ( "     %3d(G) %8lld %4d %7.4f %7.4f %7.4f  %5d       %3s     %s\n",
			  img->tr+total_frames*256, stat->bit_ctr - stat->bit_ctr_n,
			  img->qp, snr->snr_y, snr->snr_u, snr->snr_v, tmp_time, Frmfld, MD5str );
	  }
  }
  else
  {
	  char typ = (img->type == INTRA_IMG)? 'I' : (img->type == INTER_IMG) ? ((img->typeb == BP_IMG)? 'S':'P') : (img->type == F_IMG ? 'F' :'B');
	  if( img->type == INTER_IMG || img->type == F_IMG )
	  {
		  fprintf ( file, "%3d(%c) %8lld %4d %7.4f %7.4f %7.4f  %5d        %3d\t%s\n",
			  img->tr, typ, stat->bit_ctr - stat->bit_ctr_n, img->qp, snr->snr_y,
			  snr->snr_u, snr->snr_v, tmp_time, intras, MD5str );
		  fclose ( file );
		  printf ( "%3d(%c) %8lld %4d %7.4f %7.4f %7.4f  %5d       %3s     %3d\t%s\n",
			  img->tr+total_frames*256, typ, stat->bit_ctr - stat->bit_ctr_n, img->qp, snr->snr_y,
			  snr->snr_u, snr->snr_v, tmp_time, Frmfld, intras, MD5str );
	  }
	  else
	  {
		  fprintf ( file, "%3d(%c) %8lld %4d %7.4f %7.4f %7.4f  %5d        %s\n",
			  img->tr, typ, stat->bit_ctr - stat->bit_ctr_n, img->qp,
			  snr->snr_y, snr->snr_u, snr->snr_v, tmp_time, MD5str );
		  fclose ( file );
		  printf ( "%3d(%c) %8lld %4d %7.4f %7.4f %7.4f  %5d       %3s\t\t%s\n",
			  img->tr+total_frames*256, typ, stat->bit_ctr - stat->bit_ctr_n, img->qp,
			  snr->snr_y, snr->snr_u, snr->snr_v, tmp_time, Frmfld, MD5str );
	  }
  }
#else
#ifdef AVS2_S2_BGLONGTERM
#if AVS2_SCENE_CD
  if(img->typeb == BACKGROUND_IMG && input->bg_enable)
#else
  if(img->typeb == BACKGROUND_IMG && input->bg_enable && input->profile_id == 0x50)
#endif
  {
	  if(input->bg_input_number==0)
	  {
		 fprintf ( file, "%3d(GB) %8lld %4d %7.4f %7.4f %7.4f  %5d             \n",
				img->tr, stat->bit_ctr - stat->bit_ctr_n,
				img->qp, snr->snr_y, snr->snr_u, snr->snr_v, tmp_time );
		 
		 fclose( file );

		 printf ( "%3d(GB) %8lld %4d %7.4f %7.4f %7.4f  %5d       %3s     \n",
			 img->tr+total_frames*256, stat->bit_ctr - stat->bit_ctr_n,
			 img->qp, snr->snr_y, snr->snr_u, snr->snr_v, 
			 tmp_time, Frmfld );
	  }
	  else
	  {
		 fprintf ( file, "%3d(G) %8lld %4d %7.4f %7.4f %7.4f  %5d             \n",
				img->tr, stat->bit_ctr - stat->bit_ctr_n,
				img->qp, snr->snr_y, snr->snr_u, snr->snr_v, tmp_time );
		 
		 fclose( file );
		 printf ( "%3d(G) %8lld %4d %7.4f %7.4f %7.4f  %5d       %3s     \n",
			 img->tr+total_frames*256, stat->bit_ctr - stat->bit_ctr_n,
			 img->qp, snr->snr_y, snr->snr_u, snr->snr_v, 
			 tmp_time, Frmfld );
	  }
  }
  else
  {
#ifdef AVS2_S2_S
  char typ = (img->type == INTRA_IMG)? 'I' : (img->type == INTER_IMG) ? ((img->typeb == BP_IMG)? 'S':'P') : (img->type == F_IMG ? 'F' :'B');
#else
  char typ = ( img->type == INTRA_IMG ) ? 'I' : (img->type == F_IMG ) ? 'F' : (img->type == P_IMG ) ? 'P' : 'B';
#endif
  if( img->type == INTER_IMG || img->type == F_IMG )
  {
	  fprintf ( file, "%3d(%c) %8lld %4d %7.4f %7.4f %7.4f  %5d        %3d\n",
		  img->tr, typ, stat->bit_ctr - stat->bit_ctr_n, img->qp, snr->snr_y,
		  snr->snr_u, snr->snr_v, tmp_time,
		  intras );
	  fclose ( file );
	  printf ( "%3d(%c) %8lld %4d %7.4f %7.4f %7.4f  %5d       %3s     %3d    \n",
		  img->tr+total_frames*256, typ, stat->bit_ctr - stat->bit_ctr_n, img->qp, snr->snr_y,
		  snr->snr_u, snr->snr_v, tmp_time,
		  Frmfld, intras );
  }
  else
  {
	  fprintf ( file, "%3d(%c) %8lld %4d %7.4f %7.4f %7.4f  %5d        \n",
		  img->tr, typ, stat->bit_ctr - stat->bit_ctr_n, img->qp,
		  snr->snr_y, snr->snr_u, snr->snr_v, tmp_time );

	  fclose ( file );
	  printf ( "%3d(%c) %8lld %4d %7.4f %7.4f %7.4f  %5d       %3s\n",
		  img->tr+total_frames*256, typ, stat->bit_ctr - stat->bit_ctr_n, img->qp,
		  snr->snr_y, snr->snr_u, snr->snr_v, tmp_time, Frmfld );
  }
  }
#else
  char typ = ( img->type == INTRA_IMG ) ? 'I' : (img->type == F_IMG ) ? 'F' : (img->type == P_IMG ) ? 'P' : 'B';
  if( img->type == INTER_IMG || img->type == F_IMG )
  {
	  fprintf ( file, "%3d(%c) %8d %4d %7.4f %7.4f %7.4f  %5d        %3d\n",
		  img->tr, typ, stat->bit_ctr - stat->bit_ctr_n, img->qp, snr->snr_y,
		  snr->snr_u, snr->snr_v, tmp_time,
		  intras );
	  fclose ( file );
	  printf ( "%3d(%c) %8d %4d %7.4f %7.4f %7.4f  %5d       %3s     %3d    \n",
		  img->tr, typ, stat->bit_ctr - stat->bit_ctr_n, img->qp, snr->snr_y,
		  snr->snr_u, snr->snr_v, tmp_time,
		  Frmfld, intras );
  }
  else
  {
	  fprintf ( file, "%3d(%c) %8d %4d %7.4f %7.4f %7.4f  %5d        \n",
		  img->tr, typ, stat->bit_ctr - stat->bit_ctr_n, img->qp,
		  snr->snr_y, snr->snr_u, snr->snr_v, tmp_time );

	  fclose ( file );

	  printf ( "%3d(%c) %8d %4d %7.4f %7.4f %7.4f  %5d       %3s\n",
		  img->tr, typ, stat->bit_ctr - stat->bit_ctr_n, img->qp,
		  snr->snr_y, snr->snr_u, snr->snr_v, tmp_time, Frmfld );
  }
#endif
#endif
}

/*
*************************************************************************
* Function:Copies contents of a Sourceframe structure into the old-style
*    variables imgY_org and imgUV_org.  No other side effects
* Input:  sf the source frame the frame is to be taken from
* Output:
* Return:
* Attention:
*************************************************************************
*/

// image padding, 20071009
static void CopyFrameToOldImgOrgVariables ()
{
  int x, y;
  byte *u_buffer, *v_buffer;
  int input_height_cr = input->chroma_format == 1 ? input->img_height / 2 : input->img_height;
  int img_height_cr   = input->chroma_format == 1 ? img->height / 2 : img->height;
#if INTERLACE_CODING
  if (input->InterlaceCodingOption == 3)
    u_buffer = imgY_org_buffer + input->org_img_width * input->org_img_height;
  else
#endif
  u_buffer = imgY_org_buffer + input->img_width * input->img_height;

  if ( input->chroma_format == 1 )
  {
#if INTERLACE_CODING
    if (input->InterlaceCodingOption == 3)
      v_buffer = imgY_org_buffer + input->org_img_width * input->org_img_height * 5 / 4;
    else
#endif
    v_buffer = imgY_org_buffer + input->img_width * input->img_height * 5 / 4;
  }

  // Y
  if ( ! ( input->chroma_format == 2 && input->yuv_structure == 1 ) ) //20080721,422
  {
    for ( y = 0; y < input->img_height; y++ )
    {
      for ( x = 0; x < input->img_width; x++ )
      {
#if INTERLACE_CODING
        if (input->InterlaceCodingOption == 3)
          imgY_org [y][x] = imgY_org_buffer[(2 * y + (img->tr + total_frames*256)%2) * input->img_width + x];  //img->tr%2==0: top field, img->tr%2=1: bot field
        else
          imgY_org [y][x] = imgY_org_buffer[y * input->img_width + x];
#else
        imgY_org [y][x] = imgY_org_buffer[y * input->img_width + x];
#endif
      }
    }
  }

  // UV
  if ( ! ( input->chroma_format == 2 && input->yuv_structure == 1 ) ) //20080721,422
  {
    for ( y = 0; y < input_height_cr; y++ ) //X ZHENG, 422
    {
      for ( x = 0; x < input->img_width / 2; x++ )
      {
#if INTERLACE_CODING
        if (input->InterlaceCodingOption == 3)
        {
          imgUV_org[0][y][x] = u_buffer[(2 * y + (img->tr + total_frames*256)%2) * input->img_width / 2 + x];
          imgUV_org[1][y][x] = v_buffer[(2 * y + (img->tr + total_frames*256)%2) * input->img_width / 2 + x];
        }
        else
        {
          imgUV_org[0][y][x] = u_buffer[y * input->img_width / 2 + x];
          imgUV_org[1][y][x] = v_buffer[y * input->img_width / 2 + x];
        }
#else
        imgUV_org[0][y][x] = u_buffer[y * input->img_width / 2 + x];
        imgUV_org[1][y][x] = v_buffer[y * input->img_width / 2 + x];
#endif
      }
    }
  }

  // Y's width padding
  for ( y = 0; y < input->img_height; y++ )
  {
    for ( x = input->img_width; x < img->width; x++ )
    {
      imgY_org [y][x] = imgY_org [y][x - 1];
    }
  }

  //Y's padding bottom border
  for ( y = input->img_height; y < img->height; y++ )
  {
    for ( x = 0; x < img->width; x++ )
    {
      imgY_org [y][x] = imgY_org [y - 1][x];
    }
  }

  // UV's padding width
  for ( y = 0; y < input_height_cr; y++ )
  {
    for ( x = input->img_width / 2; x < img->width / 2; x++ )
    {
      imgUV_org [0][y][x] = imgUV_org [0][y][x - 1];
      imgUV_org [1][y][x] = imgUV_org [1][y][x - 1];
    }
  }

  // UV's padding bottom border
  for ( y = input_height_cr; y < img_height_cr; y++ )
  {
    for ( x = 0; x < img->width / 2; x++ )
    {
      imgUV_org [0][y][x] = imgUV_org [0][y - 1][x];
      imgUV_org [1][y][x] = imgUV_org [1][y - 1][x];
    }
  }
}
// image padding, 20071009

/*
*************************************************************************
* Function: Calculates the absolute frame number in the source file out
of various variables in img-> and input->
* Input:
* Output:
* Return: frame number in the file to be read
* Attention: \side effects
global variable frame_no updated -- dunno, for what this one is necessary
*************************************************************************
*/


static int CalculateFrameNumber()
{
  // printf the position of frame in input sequence
	printf ( "%5d", img->tr+total_frames*256 );
	return (img->tr+total_frames*256);
}

/*
*************************************************************************
* Function:Reads one new frame from file
* Input: FrameNoInFile: Frame number in the source file
HeaderSize: Number of bytes in the source file to be skipped
xs: horizontal size of frame in pixels, must be divisible by 16
ys: vertical size of frame in pixels, must be divisible by 16 or
32 in case of MB-adaptive frame/field coding
sf: Sourceframe structure to which the frame is written
* Output:
* Return:
* Attention:
*************************************************************************
*/
#if ENC_FRAME_SKIP
static void ReadOneFrame ( int FrameNoInFile, int FrameSkip,int xs, int ys )
#else
static void ReadOneFrame ( int FrameNoInFile, int HeaderSize, int xs, int ys )
#endif
{
  //const unsigned int  bytes_y = input->img_width *input->stuff_height;    //modify by wuzhongmou 0610
  int input_width_cr  = ( input->img_width / 2 );                           //20080721
  int input_height_cr = ( input->img_height / ( input->chroma_format == 1 ? 2 : 1 ) );
  unsigned int  bytes_y = input->img_width * input->img_height;             //add by wuzhongmou 0610
#if INTERLACE_CODING
  unsigned int  bytes_uv = input_width_cr * input_height_cr;
#ifdef WIN32
  __int64 framesize_in_bytes = bytes_y + 2 * bytes_uv;
#else
#if EXTEND_BD
  int64_t framesize_in_bytes = bytes_y + 2 * bytes_uv;
#else
  int framesize_in_bytes = bytes_y + 2 * bytes_uv;
#endif
#endif
#else
  const unsigned int  bytes_uv = input_width_cr * input_height_cr;
#ifdef WIN32
  const __int64 framesize_in_bytes = bytes_y + 2 * bytes_uv;
#else
#if EXTEND_BD
  const int64_t framesize_in_bytes = bytes_y + 2 * bytes_uv;
#else
  const int framesize_in_bytes = bytes_y + 2 * bytes_uv;
#endif
#endif
#endif
  //int stuff_height_cr = (input->img_height-input->stuff_height)/2;
  int off_y  = input->img_width * input->img_height;
  int off_uv = input_width_cr * input_height_cr;
#if EXTEND_BD
  unsigned char *tempPix;
  int nSampleSize = (input->input_sample_bit_depth == 8 ? 1 : 2);
  int offSet;
  int i;
  int shift1 = input->sample_bit_depth - input->input_sample_bit_depth;
#endif

#if INTERLACE_CODING
  int RealFrameNoInFile = FrameNumberInFile / (input->InterlaceCodingOption == 3 ? 2 : 1);  //In case of field picture coding, two field picture share one picture buffer that is stored as interleave format
  if (input->InterlaceCodingOption == 3)
  {
    input_width_cr  = ( input->org_img_width / 2 );                           //20080721
    input_height_cr = ( input->org_img_height / ( input->chroma_format == 1 ? 2 : 1 ) );
    bytes_y = input->org_img_width * input->org_img_height;             //add by wuzhongmou 0610
    bytes_uv = input_width_cr * input_height_cr;
    framesize_in_bytes = bytes_y + 2 * bytes_uv;
    off_y  = input->org_img_width * input->org_img_height;
    off_uv = input_width_cr * input_height_cr;
  }
#endif

  assert ( FrameNumberInFile == FrameNoInFile );

#ifdef WIN32
#if ENC_FRAME_SKIP
#if INTERLACE_CODING
#if EXTEND_BD
  if ( _lseeki64 ( p_in, framesize_in_bytes * (RealFrameNoInFile+FrameSkip) * nSampleSize, SEEK_SET ) == -1 )
#else
  if ( _lseeki64 ( p_in, framesize_in_bytes * (RealFrameNoInFile+FrameSkip), SEEK_SET ) == -1 )
#endif
#else
#if EXTEND_BD
  if ( _lseeki64 ( p_in, framesize_in_bytes * (FrameNoInFile+FrameSkip) * nSampleSize, SEEK_SET ) == -1 )
#else
  if ( _lseeki64 ( p_in, framesize_in_bytes * (FrameNoInFile+FrameSkip), SEEK_SET ) == -1 )
#endif
#endif
#else
#if INTERLACE_CODING
#if EXTEND_BD
  if ( _lseeki64 ( p_in, framesize_in_bytes * RealFrameNoInFile * nSampleSize + HeaderSize, SEEK_SET ) == -1 )
#else
  if ( _lseeki64 ( p_in, framesize_in_bytes * RealFrameNoInFile + HeaderSize, SEEK_SET ) == -1 )
#endif
#else
#if EXTEND_BD
  if ( _lseeki64 ( p_in, framesize_in_bytes * FrameNoInFile * nSampleSize + HeaderSize, SEEK_SET ) == -1 )
#else
  if ( _lseeki64 ( p_in, framesize_in_bytes * FrameNoInFile + HeaderSize, SEEK_SET ) == -1 )
#endif
#endif
#endif
  {
    error ( "ReadOneFrame: cannot fseek to (Header size) in p_in", -1 );
  }

#if EXTEND_BD
  if ( _read ( p_in, (void *)(imgY_org_buffer), (bytes_y * nSampleSize) ) != ( int ) (bytes_y * nSampleSize) )
  {
    printf ( "ReadOneFrame: cannot read %d bytes from input file, unexpected EOF?, exiting", bytes_y * nSampleSize);
    exit ( -1 );
  }

  if ( _read ( p_in, (void *)(imgY_org_buffer + off_y), (bytes_uv * nSampleSize) ) != ( int ) (bytes_uv * nSampleSize))
  {
    printf ( "ReadOneFrame: cannot read %d bytes from input file, unexpected EOF?, exiting", bytes_uv * nSampleSize );
    exit ( -1 );
  }

  if ( _read ( p_in, (void *)(imgY_org_buffer + off_y + off_uv), (bytes_uv * nSampleSize) ) != ( int ) (bytes_uv * nSampleSize) )
  {
    printf ( "ReadOneFrame: cannot read %d bytes from input file, unexpected EOF?, exiting", bytes_uv * nSampleSize );
    exit ( -1 );
  }
#else
  if ( _read ( p_in, imgY_org_buffer, bytes_y ) != ( int ) bytes_y )
  {
    printf ( "ReadOneFrame: cannot read %d bytes from input file, unexpected EOF?, exiting", bytes_y );
    exit ( -1 );
  }

  if ( _read ( p_in, imgY_org_buffer + off_y, bytes_uv ) != ( int ) bytes_uv )
  {
    printf ( "ReadOneFrame: cannot read %d bytes from input file, unexpected EOF?, exiting", bytes_uv );
    exit ( -1 );
  }

  if ( _read ( p_in, imgY_org_buffer + off_y + off_uv, bytes_uv ) != ( int ) bytes_uv )
  {
    printf ( "ReadOneFrame: cannot read %d bytes from input file, unexpected EOF?, exiting", bytes_uv );
    exit ( -1 );
  }
#endif

#if EXTEND_BD
  if (input->input_sample_bit_depth == 8)
  {
    for (i = 0; i < bytes_y; i ++)
    {
      tempPix = (unsigned char *)(&imgY_org_buffer[bytes_y - i - 1]);
      tempPix[0] = *((unsigned char *)(&imgY_org_buffer[(bytes_y - i - 1) / 2]) + ((i + 1) % 2));
      tempPix[1] = 0; // reset high byte
    }
    offSet = off_y;
    for (i = 0; i < bytes_uv; i ++)
    {
      tempPix = (unsigned char *)(&imgY_org_buffer[offSet + (bytes_uv - i - 1)]);
      tempPix[0] = *((unsigned char *)(&imgY_org_buffer[offSet + (bytes_uv - i - 1) / 2]) + ((i + 1) % 2));
      tempPix[1] = 0; // reset high byte
    }
    offSet = (offSet + off_uv);
    for (i = 0; i < bytes_uv; i ++)
    {
      tempPix = (unsigned char *)(&imgY_org_buffer[offSet + (bytes_uv - i - 1)]);
      tempPix[0] = *((unsigned char *)(&imgY_org_buffer[offSet + (bytes_uv - i - 1) / 2]) + ((i + 1) % 2));
      tempPix[1] = 0; // reset high byte
    }

    for (i = 0; i < bytes_y + bytes_uv * 2; i ++)
    {
      imgY_org_buffer[i] = imgY_org_buffer[i] << shift1; // shift 8 bit samples to 10/12 bit samples
    }
  }
  if (input->input_sample_bit_depth == 10)
  {
    for (i = 0; i < bytes_y + bytes_uv * 2; i ++)
    {
      imgY_org_buffer[i] = imgY_org_buffer[i] & 0x3ff; // only keep 10 bits
    }
  }
  if (input->input_sample_bit_depth == 12)
  {
    for (i = 0; i < bytes_y + bytes_uv * 2; i ++)
    {
      imgY_org_buffer[i] = imgY_org_buffer[i] & 0xfff; // only keep 12 bits
    }
  }
#endif
#else
#if ENC_FRAME_SKIP
#if INTERLACE_CODING
#if EXTEND_BD
  if ( fseek ( p_in, (off_t)(framesize_in_bytes * (RealFrameNoInFile+FrameSkip) * nSampleSize), SEEK_SET ) != 0 )
#else
  if ( fseek ( p_in, framesize_in_bytes * (RealFrameNoInFile+FrameSkip), SEEK_SET ) != 0 )
#endif
#else
#if EXTEND_BD
  if ( fseeko ( p_in, (off_t)(framesize_in_bytes * (FrameNoInFile+FrameSkip) * nSampleSize), SEEK_SET ) != 0 )
#else
  if ( fseek ( p_in, framesize_in_bytes * (FrameNoInFile+FrameSkip), SEEK_SET ) != 0 )
#endif
#endif
#else
#if INTERLACE_CODING
#if EXTEND_BD
  if ( fseeko ( p_in, (off_t)(framesize_in_bytes * RealFrameNoInFile * nSampleSize + HeaderSize), SEEK_SET ) != 0 )
#else
  if ( fseek ( p_in, framesize_in_bytes * RealFrameNoInFile + HeaderSize, SEEK_SET ) != 0 )
#endif
#else
#if EXTEND_BD
  if ( fseeko ( p_in, (off_t)(framesize_in_bytes * FrameNoInFile * nSampleSize + HeaderSize), SEEK_SET ) != 0 )
#else
  if ( fseek ( p_in, framesize_in_bytes * FrameNoInFile + HeaderSize, SEEK_SET ) != 0 )
#endif
#endif
#endif
  {
    error ( "ReadOneFrame: cannot fseek to (Header size) in p_in", -1 );
  }
#if EXTEND_BD
  if ( fread ( (void *)imgY_org_buffer, 1, (bytes_y * nSampleSize), p_in ) != ( unsigned ) (bytes_y * nSampleSize) )
  {
    printf ( "ReadOneFrame: cannot read %d bytes from input file, unexpected EOF?, exiting", (bytes_y * nSampleSize) );
    exit ( -1 );
  }

  if ( fread ( (void *)(imgY_org_buffer + off_y), 1, (bytes_uv * nSampleSize), p_in ) != ( unsigned ) (bytes_uv * nSampleSize) )
  {
    printf ( "ReadOneFrame: cannot read %d bytes from input file, unexpected EOF?, exiting", (bytes_uv * nSampleSize) );
    exit ( -1 );
  }

  if ( fread ( (void *)(imgY_org_buffer + off_y + off_uv), 1, (bytes_uv * nSampleSize), p_in ) != ( unsigned ) (bytes_uv * nSampleSize) )
  {
    printf ( "ReadOneFrame: cannot read %d bytes from input file, unexpected EOF?, exiting", (bytes_uv * nSampleSize) );
    exit ( -1 );
  }
#else
  if ( fread ( imgY_org_buffer, 1, bytes_y, p_in ) != ( unsigned ) bytes_y )
  {
    printf ( "ReadOneFrame: cannot read %d bytes from input file, unexpected EOF?, exiting", bytes_y );
    exit ( -1 );
  }

  if ( fread ( imgY_org_buffer + off_y, 1, bytes_uv, p_in ) != ( unsigned ) bytes_uv )
  {
    printf ( "ReadOneFrame: cannot read %d bytes from input file, unexpected EOF?, exiting", bytes_uv );
    exit ( -1 );
  }

  if ( fread ( imgY_org_buffer + off_y + off_uv, 1, bytes_uv, p_in ) != ( unsigned ) bytes_uv )
  {
    printf ( "ReadOneFrame: cannot read %d bytes from input file, unexpected EOF?, exiting", bytes_uv );
    exit ( -1 );
  }
#endif

#if EXTEND_BD
  if (input->input_sample_bit_depth == 8)
  {
    for (i = 0; i < bytes_y; i ++)
    {
      tempPix = (unsigned char *)(&imgY_org_buffer[bytes_y - i - 1]);
      tempPix[0] = *((unsigned char *)(&imgY_org_buffer[(bytes_y - i - 1) / 2]) + ((i + 1) % 2));
      tempPix[1] = 0; // reset high byte
    }
    offSet = off_y;
    for (i = 0; i < bytes_uv; i ++)
    {
      tempPix = (unsigned char *)(&imgY_org_buffer[offSet + (bytes_uv - i - 1)]);
      tempPix[0] = *((unsigned char *)(&imgY_org_buffer[offSet + (bytes_uv - i - 1) / 2]) + ((i + 1) % 2));
      tempPix[1] = 0; // reset high byte
    }
    offSet = (offSet + off_uv);
    for (i = 0; i < bytes_uv; i ++)
    {
      tempPix = (unsigned char *)(&imgY_org_buffer[offSet + (bytes_uv - i - 1)]);
      tempPix[0] = *((unsigned char *)(&imgY_org_buffer[offSet + (bytes_uv - i - 1) / 2]) + ((i + 1) % 2));
      tempPix[1] = 0; // reset high byte
    }

    for (i = 0; i < bytes_y + bytes_uv * 2; i ++)
    {
      imgY_org_buffer[i] = imgY_org_buffer[i] << shift1; // shift 8 bit samples to 10 bit samples
    }
  }
  if (input->input_sample_bit_depth == 10)
  {
    for (i = 0; i < bytes_y + bytes_uv * 2; i ++)
    {
      imgY_org_buffer[i] = imgY_org_buffer[i] & 0x3ff; // only keep 10 bits
    }
  }
  if (input->input_sample_bit_depth == 12)
  {
    for (i = 0; i < bytes_y + bytes_uv * 2; i ++)
    {
      imgY_org_buffer[i] = imgY_org_buffer[i] & 0xfff; // only keep 12 bits
    }
  }
#endif
#endif
}

/*
*************************************************************************
* Function:point to frame coding variables
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void put_buffer_frame()
{
  int i, j;

  //integer pixel for chroma
  for ( j = 0; j < 4; j++ )
  {
    for ( i = 0; i < 2; i++ )
    {
      integerRefUV[j][i]   = ref[j][i + 1];
    }
  }

  //integer pixel for luma
  for ( i = 0; i < 4; i++ )
  {
    Refbuf11[i] = &ref[i][0][0][0];
  }

  //current reconstructed image

  imgY  = currentFrame[0];
  imgUV =  &currentFrame[1];

}
/*
*************************************************************************
* Function:update the decoder picture buffer
* Input:frame number in the bitstream and the video sequence
* Output:
* Return:
* Attention:
*************************************************************************
*/
//edit start added by lzhang for AEC
void write_terminating_bit ( unsigned char bit )
{
  DataPartition*          dataPart;
  EncodingEnvironmentPtr  eep_dp;

  //--- write non-slice termination symbol if the codingUnit is not the first one in its slice ---
  dataPart = & ( img->currentSlice->partArr[0] );
  dataPart->bitstream->write_flag = 1;
  eep_dp   = & ( dataPart->ee_AEC );

  biari_encode_symbol_final ( eep_dp, bit );
#if TRACE
  if ( AEC_writting )
  {
    fprintf ( p_trace, "      aec_mb_stuffing_bit = %d\n", bit );
  }
#endif

}


void free_slice()
{
  Slice * slice = img->currentSlice;

  if ( slice != NULL )
  {
    if ( slice->partArr != NULL )
    {
      free ( slice->partArr );
    }

    delete_contexts_SyntaxInfo ( slice->syn_ctx );

    //free(img->currentSlice);
    free ( slice );
  }
}
//edit end added by lzhang for AEC


//Initialized SliceSet, added by XZHENG, 2008.04
void init_slice_set ( int type )
{
  int  i;

  for ( i = 0; i < ( img->width / MIN_CU_SIZE ) * ( img->height / MIN_CU_SIZE ); i++ )
  {
    img->mb_data[i].slice_set_index = img->mb_data[i].slice_nr = 0;
    img->mb_data[i].slice_header_flag = 0;
    img->mb_data[i].sliceqp = img->type == B_IMG ? input->qpB : ( img->type == INTRA_IMG ? input->qpI : input->qpP );

#ifdef AVS2_S2_BGLONGTERM
#if AVS2_SCENE_CD
	if(img->typeb==BACKGROUND_IMG && (background_output_flag==0) && input->bg_enable
#if AVS2_SCENE_GB_CD
		&& gb_is_ready == 1
#endif
		)
#else
	if(img->typeb==BACKGROUND_IMG && (background_output_flag==0) && input->bg_enable && input->profile_id == 0x50)
#endif
		img->mb_data[i].sliceqp = input->bg_qp;
#endif
  }



  for ( i = 0; i < MAX_REGION_NUM; i++ )
  {
    sliceregion[i].slice_set_index = sliceregion[i].pos[0] = sliceregion[i].pos[1] = sliceregion[i].pos[2] = sliceregion[i].pos[3] = -1;
  }

  SliceSetConfigure();
  SliceSetAllocate ( type ); //0: frame    1: field
}

//Added by XZHENG, 2008.04
void SliceSetAllocate ( int type )
{
  int  startx, starty, endx, endy;
  int  mb_num = 0;
  int  mb_num_hor;
  int  regionnum = 0;
  int  mb_width = img->width / MIN_CU_SIZE;
  int  rec_region[MAX_REGION_NUM][2];  //0: first mb; 1: last mb in the raster scan
  int  rec_pos;
  int  i = 0;

  for ( i = 0; i < MAX_REGION_NUM; i++ )
  {
    //initialization
    rec_region[i][0] = 8640;
    rec_region[i][1] = -1;
  }

  if ( type == 0 )   //frame coding
  {
    while ( sliceregion[regionnum].slice_set_index != -1 )
    {
      startx =  sliceregion[regionnum].pos[0] / MIN_CU_SIZE;
      endx   =  sliceregion[regionnum].pos[1] / MIN_CU_SIZE - 1;
      starty =  sliceregion[regionnum].pos[2] / MIN_CU_SIZE;
      endy   =  sliceregion[regionnum].pos[3] / MIN_CU_SIZE - 1;
      mb_num = ( endx - startx + 1 ) * ( endy - starty + 1 );
      mb_num_hor = endx - startx + 1;

      if ( rec_region[sliceregion[regionnum].slice_set_index][0] > starty * mb_width + startx )
      {
        rec_region[sliceregion[regionnum].slice_set_index][0] = starty * mb_width + startx;
      }

      if ( rec_region[sliceregion[regionnum].slice_set_index][1] < endy * mb_width + endx )
      {
        rec_region[sliceregion[regionnum].slice_set_index][1] = endy * mb_width + endx;
      }

      //Allocate the slice_set_index to every codingUnit
      for ( i = 0; i < mb_num; i++ )
      {
        img->mb_data[starty * mb_width + startx + ( i / mb_num_hor ) *mb_width + ( i % mb_num_hor ) ].slice_set_index = sliceregion[regionnum].slice_set_index;
        img->mb_data[starty * mb_width + startx + ( i / mb_num_hor ) *mb_width + ( i % mb_num_hor ) ].sliceqp = sliceregion[regionnum].sliceqp;
      }

      regionnum++;
    }

    //Process the background slice set
    for ( i = 0; i < ( img->width * img->height / ( MIN_CU_SIZE * MIN_CU_SIZE ) ); i++ )
    {
      if ( img->mb_data[i].slice_set_index == 0 )
      {
        if ( rec_region[0][0] > i )
        {
          rec_region[0][0] = i;
        }

        if ( rec_region[0][1] < i )
        {
          rec_region[0][1] = i;
        }
      }
    }

    //Allocate slice_nr
    for ( i = 1; i < ( ( img->height / MIN_CU_SIZE ) * ( img->width / MIN_CU_SIZE ) ); i++ )
    {
      if ( img->mb_data[i].slice_set_index != img->mb_data[i - 1].slice_set_index )
      {
        img->mb_data[i].slice_nr = img->mb_data[i - 1].slice_nr + 1;
      }
      else
      {
        img->mb_data[i].slice_nr = img->mb_data[i - 1].slice_nr;
      }
    }

    //for the backgound
    rec_pos = rec_region[0][0];
    img->mb_data[rec_pos].slice_header_flag = 1;
    rec_pos = rec_region[0][1];

    //for the foreground
    regionnum = 0;

    while ( sliceregion[regionnum].slice_set_index != -1 )
    {
      rec_pos = rec_region[sliceregion[regionnum].slice_set_index][0];
      img->mb_data[rec_pos].slice_header_flag = 1;

      rec_pos = rec_region[sliceregion[regionnum].slice_set_index][1];

      regionnum++;
    }
  }
}
