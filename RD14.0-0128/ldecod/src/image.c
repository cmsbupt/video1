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
* File name: image.c
* Function: Decode a Slice
*
*************************************************************************************
*/


#include "../../lcommon/inc/contributors.h"

#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <sys/timeb.h>
#include <string.h>
#include <assert.h>
#ifdef WIN32
#include <IO.H>
#include <STDIO.H>
#endif

#include "global.h"
#include "../../lcommon/inc/commonVariables.h"
#include "header.h"
#include "annexb.h"
#include "../../lcommon/inc/memalloc.h"
#include "AEC.h"
#include "biaridecod.h"
#include "../../lcommon/inc/loop-filter.h"
#include "../../lcommon/inc/ComAdaptiveLoopFilter.h"
#include "../../ldecod/inc/DecAdaptiveLoopFilter.h"

#if EXTRACT
  char * aPicTypeStr[6]={"I_IMG","P_IMG","B_IMG","BACKGROUND_IMG","F_IMG","BP_IMG"};
//  char * aInterPredStr[5]={"FORWARD","BACKWARD","SYM","BID","DUAL"};
#endif	

void Copy_frame_for_ALF();
#if ROI_M3264
#include "pos_info.h"
#endif

// Adaptive frequency weighting quantization
#if FREQUENCY_WEIGHTING_QUANTIZATION
#include "wquant.h"
#endif
#define max(a, b)                   ((a) > (b) ? (a) : (b))  //!< Macro returning max value 
#define min(a, b)                   ((a) < (b) ? (a) : (b))  //!< Macro returning min value 
void DecideMvRange(); // rm52k

/* 08.16.2007--for user data after pic header */
unsigned char *temp_slice_buf;
int first_slice_length;
int first_slice_startpos;
/* 08.16.2007--for user data after pic header */
extern void readParaSAO_one_SMB(int smb_index,int mb_y, int mb_x, int smb_mb_height, int smb_mb_width, int *slice_sao_on, SAOBlkParam* saoBlkParam, SAOBlkParam* rec_saoBlkParam);
extern StatBits *StatBitsPtr;

#ifdef MD5
extern unsigned int   MD5val[4];
extern char		      MD5str[33];
#endif
/*
*************************************************************************
* Function:decodes one I- or P-frame
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

int decode_one_frame ( SNRParameters *snr )
{
  int current_header;
  int N8_SizeScale;

  time_t ltime1;                  // for time measurement
  time_t ltime2;

#ifdef WIN32
  struct _timeb tstruct1;
  struct _timeb tstruct2;
#else
  struct timeb tstruct1;
  struct timeb tstruct2;
#endif

  double framerate[8] = {24000.0 / 1001.0, 24.0, 25.0, 30000.0 / 1001.0, 30.0, 50.0, 60000.0 / 1001.0, 60.0}; 

#ifdef WIN32
  _ftime ( &tstruct1 );           // start time ms
#else
  ftime ( &tstruct1 );            // start time ms
#endif
  time ( &ltime1 );               // start time s

  img->current_mb_nr = -4711; // initialized to an impossible value for debugging -- correct value is taken from slice header

  current_header = Header();
  DecideMvRange();  //rm52k
	printf("current_header = Header(); img->tr=%d\n",img->tr);

  if ( current_header == EOS )
  {
    return EOS;
  }


  {
    N8_SizeScale = 1;

    if ( horizontal_size % ( MIN_CU_SIZE * N8_SizeScale ) != 0 )
    {
      img->auto_crop_right = ( MIN_CU_SIZE * N8_SizeScale ) - ( horizontal_size % ( MIN_CU_SIZE * N8_SizeScale ) );
    }
    else
    {
      img->auto_crop_right = 0;
    }
#if !INTERLACE_CODING
    if ( progressive_sequence )
#endif
    {
      if ( vertical_size % ( MIN_CU_SIZE * N8_SizeScale ) != 0 )
      {
        img->auto_crop_bottom = ( MIN_CU_SIZE * N8_SizeScale ) - ( vertical_size % ( MIN_CU_SIZE * N8_SizeScale ) );
      }
      else
      {
        img->auto_crop_bottom = 0;
      }
    }

    // Reinit parameters (NOTE: need to do before init_frame //
    img->width          = ( horizontal_size + img->auto_crop_right );
    img->height         = ( vertical_size + img->auto_crop_bottom );
    img->width_cr       = ( img->width >> 1 );

    if ( input->chroma_format == 1 )
    {
      img->height_cr      = ( img->height >> 1 );
    }

    img->PicWidthInMbs  = img->width / MIN_CU_SIZE;
    img->PicHeightInMbs = img->height / MIN_CU_SIZE;
    img->PicSizeInMbs   = img->PicWidthInMbs * img->PicHeightInMbs;
    img->max_mb_nr      = ( img->width * img->height ) / ( MIN_CU_SIZE * MIN_CU_SIZE );
  }

  if (img->new_sequence_flag && img->sequence_end_flag)
  {
	  end_SeqTr = img->tr;
	  img->sequence_end_flag = 0;
  }
  if( img->new_sequence_flag )
  {
	  next_IDRtr = img->tr;
	  next_IDRcoi = img->coding_order;
	  img->new_sequence_flag = 0;
  }
  // allocate memory for frame buffers
  if ( img->number == 0 )
  {
	  init_global_buffers ();
  }

  img->current_mb_nr = 0;
  init_frame ();

  img->types = img->type;   // jlzheng 7.15

  if ( img->type != B_IMG )
  {
    pre_img_type  = img->type;
    pre_img_types = img->types;
  }
  picture_data ();


#if EXTRACT
  int b16_i,b16_j;
  int **col_ref = refbuf[0];
  int ***col_mv = mvbuf[0];
  //get_mem3Dint ( &mvbuf[i], img->height / MIN_BLOCK_SIZE, img->width / MIN_BLOCK_SIZE, 2 );
  //MIN_BLOCK_SIZE=4
  if (img->type !=B_IMG){
	  for(b16_i=0; b16_i<(img->height+3)/MIN_BLOCK_SIZE; b16_i=b16_i+4)
	    for(b16_j=0; b16_j<(img->width+3)/MIN_BLOCK_SIZE; b16_j=b16_j+4)
	    {
#if EXTRACT_08X	    
	      fprintf(p_mv_col,"%08X\n",col_ref[b16_i][b16_j]);
	      fprintf(p_mv_col,"%08X\n",col_mv[b16_i][b16_j][0]);
	      fprintf(p_mv_col,"%08X\n",col_mv[b16_i][b16_j][1]);
#endif

#if EXTRACT_D	    
	      fprintf(p_mv_col,"%d\n",col_ref[b16_i][b16_j]);
	      fprintf(p_mv_col,"col_mv x:%d\n",col_mv[b16_i][b16_j][0]);
	      fprintf(p_mv_col,"col_mv y:%d\n",col_mv[b16_i][b16_j][1]);
#endif
	    }
  }
  if (img->type ==B_IMG){
#if EXTRACT_DPB_ColMV_B
		int col_poc;
		int col_type = refPicTypebuf[ img->imgtr_fwRefDistance[0]];//获取时域帧的类型
		printf("cur poc=%4d col_poc=%4d col_type=%4d \n",img->tr,img->imgtr_fwRefDistance[0],refPicTypebuf[ img->imgtr_fwRefDistance[0]]);
		printf("cur poc=%4d col_poc=%4d col_type=%4d \n",img->tr,img->imgtr_fwRefDistance[1],refPicTypebuf[ img->imgtr_fwRefDistance[1]]);

		col_poc = ref_poc[img->refBufIdx][0];
		col_type = refPicTypebuf[ col_poc ];
		
		printf("cur poc=%4d col_poc=%4d col_type=%4d \n",img->tr,ref_poc[img->refBufIdx][0],refPicTypebuf[ ref_poc[img->refBufIdx][0]]);
		printf("cur poc=%4d col_poc=%4d col_type=%4d \n",img->tr,ref_poc[img->refBufIdx][1],refPicTypebuf[ ref_poc[img->refBufIdx][1]]);

		
		col_poc = ref_poc[img->refBufIdx][1];
		col_type = refPicTypebuf[ col_poc ];
#endif		
	  col_ref = refbuf[0];//img->bw_refFrArr;
	  col_mv = mvbuf[0];//img->bw_mv;
	  if(col_type == B_IMG)
	  {
		  for(b16_i=0; b16_i<(img->height+3)/MIN_BLOCK_SIZE; b16_i=b16_i+4)
		    for(b16_j=0; b16_j<(img->width+3)/MIN_BLOCK_SIZE; b16_j=b16_j+4)
		    {
#if EXTRACT_08X	    
		      //fprintf(p_mv_col,"redfidx %08X\n",col_ref[b16_i][b16_j]);
					if(col_ref[b16_i][b16_j]==1)
						fprintf(p_mv_col,"%08X\n",0);
					else if(col_ref[b16_i][b16_j]==0)
						fprintf(p_mv_col,"%08X\n",1);
					else						
						fprintf(p_mv_col,"%08X\n",col_ref[b16_i][b16_j]);
		      fprintf(p_mv_col,"%08X\n",col_mv[b16_i][b16_j][0]);
		      fprintf(p_mv_col,"%08X\n",col_mv[b16_i][b16_j][1]);
#endif

#if EXTRACT_D	    
		      fprintf(p_mv_col,"%d\n",col_ref[b16_i][b16_j]);
		      fprintf(p_mv_col,"col_mv x:%d\n",col_mv[b16_i][b16_j][0]);
		      fprintf(p_mv_col,"col_mv y:%d\n",col_mv[b16_i][b16_j][1]);
#endif
		    }
	  
	  }
		else
		{
		  for(b16_i=0; b16_i<(img->height+3)/MIN_BLOCK_SIZE; b16_i=b16_i+4)
		    for(b16_j=0; b16_j<(img->width+3)/MIN_BLOCK_SIZE; b16_j=b16_j+4)
		    {
#if EXTRACT_08X	    
		      fprintf(p_mv_col,"%08X\n",col_ref[b16_i][b16_j]);
					//if(col_ref[b16_i][b16_j]==1)
		      fprintf(p_mv_col,"%08X\n",col_mv[b16_i][b16_j][0]);
		      fprintf(p_mv_col,"%08X\n",col_mv[b16_i][b16_j][1]);
#endif

#if EXTRACT_D	    
		      fprintf(p_mv_col,"%d\n",col_ref[b16_i][b16_j]);
		      fprintf(p_mv_col,"col_mv x:%d\n",col_mv[b16_i][b16_j][0]);
		      fprintf(p_mv_col,"col_mv y:%d\n",col_mv[b16_i][b16_j][1]);
#endif
		    }
		
		}//else
  }
  if (p_pps != NULL)
  {
    fclose(p_pps);
    p_pps = NULL;
  }
  if (p_mv_col != NULL)
  {
    fclose(p_mv_col);
    p_mv_col = NULL;
  }
#endif


#ifdef AVS2_S2_BGLONGTERM
#if AVS2_SCENE_CD
  if(img->typeb == BACKGROUND_IMG && background_picture_enable)
#else
  if(img->typeb == BACKGROUND_IMG && background_picture_enable && input->profile_id == 0x50)
#endif
  {
	  int l;
	  for(l=0; l<img->height; l++)
#if EXTEND_BD
		  memcpy(background_frame[0][l],imgY[l],img->width * sizeof(byte));
#else
          memcpy(background_frame[0][l],imgY[l],img->width);
#endif
	  for(l=0; l<img->height_cr; l++)
	  {
#if EXTEND_BD
		  memcpy(background_frame[1][l],imgUV[0][l],img->width_cr * sizeof(byte));
		  memcpy(background_frame[2][l],imgUV[1][l],img->width_cr * sizeof(byte));
#else
      memcpy(background_frame[1][l],imgUV[0][l],img->width_cr);
      memcpy(background_frame[2][l],imgUV[1][l],img->width_cr);
#endif
	  }
  }
#endif

#ifdef AVS2_S2_BGLONGTERM
  if(img->typeb == BACKGROUND_IMG && background_picture_output_flag == 0 )
	  background_number++;
#endif

  if (img->type==B_IMG)
  {
	  img->imgtr_fwRefDistance[0] = trtmp;
  }

  img->height = ( vertical_size + img->auto_crop_bottom );
  img->height_cr =  img->height / ( input->chroma_format == 1 ? 2 : 1 );
  img->PicWidthInMbs  = img->width / MIN_CU_SIZE;
  img->PicHeightInMbs = img->height / MIN_CU_SIZE;
  img->PicSizeInMbs   = img->PicWidthInMbs * img->PicHeightInMbs;


#ifdef WIN32
  _ftime ( &tstruct2 ); // end time ms
#else
  ftime ( &tstruct2 );  // end time ms
#endif

  time ( &ltime2 );                               // end time sec
  tmp_time = ( ltime2 * 1000 + tstruct2.millitm ) - ( ltime1 * 1000 + tstruct1.millitm );
  tot_time = tot_time + tmp_time;

  //rm52k_r2
  StatBitsPtr->curr_frame_bits = StatBitsPtr->curr_frame_bits * 8 + StatBitsPtr->emulate_bits - StatBitsPtr->last_unit_bits;
  StatBitsPtr->bitrate += StatBitsPtr->curr_frame_bits;
  StatBitsPtr->coded_pic_num++;

  if ( ( int ) ( StatBitsPtr->coded_pic_num - ( StatBitsPtr->time_s + 1 ) *framerate[frame_rate_code - 1] + 0.5 ) == 0 )
  {
    StatBitsPtr->total_bitrate[StatBitsPtr->time_s++] = StatBitsPtr->bitrate;
    StatBitsPtr->bitrate = 0;
  }

  frame_postprocessing();
  StatBitsPtr->curr_frame_bits = 0;
  StatBitsPtr->emulate_bits    = 0;
  StatBitsPtr->last_unit_bits  = 0;

  //! TO 19.11.2001 Known Problem: for init_frame we have to know the picture type of the actual frame
  //! in case the first slice of the P-Frame following the I-Frame was lost we decode this P-Frame but//! do not write it because it was assumed to be an I-Frame in init_frame.So we force the decoder to
  //! guess the right picture type. This is a hack a should be removed by the time there is a clean
  //! solution where we do not have to know the picture type for the function init_frame.
  //! End TO 19.11.2001//Lou
  if ( img->type == I_IMG || img->type == P_IMG || img->type == F_IMG ) 
  {
    img->number++;
  }
  else
  {
    Bframe_ctr++;  // B pictures
  }

  return ( SOP );
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
void report_frame ( outdata data, int pos )
{
	FILE *file;
  char *Frmfld;
  char Frm[] = "FRM";
  char Fld[] = "FLD";
	file = fopen ( "stat.dat", "at" );

#if MD5
	if(input->MD5Enable & 0x02)
	{
		sprintf(MD5str,"%08X%08X%08X%08X\0",
			data.stdoutdata[pos].DecMD5Value[0],
			data.stdoutdata[pos].DecMD5Value[1],
			data.stdoutdata[pos].DecMD5Value[2],
			data.stdoutdata[pos].DecMD5Value[3]);
	}
	else
	{
		memset(MD5val,0,16);
		memset(MD5str,0,33);
	}
#endif

  if (data.stdoutdata[pos].picture_structure)
  {
    Frmfld = Frm;
  }
  else
  {
    Frmfld = Fld;
  }
#if INTERLACE_CODING  
  if (img->is_field_sequence)   //rcs??
  {
    Frmfld = Fld;
  }
#endif
  if ( (data.stdoutdata[pos].tr + total_frames*256) == end_SeqTr ) // I picture
	{
		//if ( img->new_sequence_flag == 1 )
		{
			img->sequence_end_flag = 0;
			fprintf ( stdout, "Sequence End\n\n" );
		}
	}
	if ( (data.stdoutdata[pos].tr + total_frames*256) == next_IDRtr )
	{
		if ( vec_flag )
		{
			vec_flag = 0;
			fprintf ( stdout, "Video Edit Code\n" );
		}
		fprintf ( stdout, "Sequence Header\n" );
	}

#if MD5

	if ( data.stdoutdata[pos].type == I_IMG ) // I picture
	{
		if(data.stdoutdata[pos].typeb == BACKGROUND_IMG)
		{
			if(background_picture_output_flag)
			{
				printf ( "%3d(G)  %3d %5d %7.4f %7.4f %7.4f %5d\t\t%s %8d %6d\t%s\n",
					data.stdoutdata[pos].framenum+total_frames*256, data.stdoutdata[pos].tr+total_frames*256, data.stdoutdata[pos].qp, data.stdoutdata[pos].snr_y, data.stdoutdata[pos].snr_u, data.stdoutdata[pos].snr_v, data.stdoutdata[pos].tmp_time, Frmfld, data.stdoutdata[pos].curr_frame_bits, data.stdoutdata[pos].emulate_bits, MD5str );
				fprintf ( file, "%3d(G)  %3d %5d %7.4f %7.4f %7.4f %5d\t\t%s\t%s\n",
					data.stdoutdata[pos].framenum+total_frames*256, data.stdoutdata[pos].tr+total_frames*256, data.stdoutdata[pos].qp, data.stdoutdata[pos].snr_y, data.stdoutdata[pos].snr_u, data.stdoutdata[pos].snr_v, data.stdoutdata[pos].tmp_time, Frmfld, MD5str );
			}
			else
			{
				printf ( "%3d(GB) %3d %5d %7.4f %7.4f %7.4f %5d\t\t%s %8d %6d\t%s\n",
					data.stdoutdata[pos].framenum+total_frames*256, data.stdoutdata[pos].tr+total_frames*256, data.stdoutdata[pos].qp, data.stdoutdata[pos].snr_y, data.stdoutdata[pos].snr_u, data.stdoutdata[pos].snr_v, data.stdoutdata[pos].tmp_time, Frmfld, data.stdoutdata[pos].curr_frame_bits, data.stdoutdata[pos].emulate_bits, MD5str );
				fprintf ( file, "%3d(GB) %3d %5d %7.4f %7.4f %7.4f %5d\t\t%s\t%s\n",
					data.stdoutdata[pos].framenum+total_frames*256, data.stdoutdata[pos].tr+total_frames*256, data.stdoutdata[pos].qp, data.stdoutdata[pos].snr_y, data.stdoutdata[pos].snr_u, data.stdoutdata[pos].snr_v, data.stdoutdata[pos].tmp_time, Frmfld, MD5str );
			}
		}
		else
		{
			printf ( "%3d(I)  %3d %5d %7.4f %7.4f %7.4f %5d\t\t%s %8d %6d\t%s\n",
				data.stdoutdata[pos].framenum+total_frames*256, data.stdoutdata[pos].tr+total_frames*256, data.stdoutdata[pos].qp, data.stdoutdata[pos].snr_y, data.stdoutdata[pos].snr_u, data.stdoutdata[pos].snr_v, data.stdoutdata[pos].tmp_time, Frmfld, data.stdoutdata[pos].curr_frame_bits, data.stdoutdata[pos].emulate_bits, MD5str );
			fprintf ( file, "%3d(I)  %3d %5d %7.4f %7.4f %7.4f %5d\t\t%s\t%s\n",
				data.stdoutdata[pos].framenum+total_frames*256, data.stdoutdata[pos].tr+total_frames*256, data.stdoutdata[pos].qp, data.stdoutdata[pos].snr_y, data.stdoutdata[pos].snr_u, data.stdoutdata[pos].snr_v, data.stdoutdata[pos].tmp_time, Frmfld, MD5str );
		}
	}
	else if ( data.stdoutdata[pos].type == P_IMG && data.stdoutdata[pos].typeb != BP_IMG)
	{
		printf ( "%3d(P)  %3d %5d %7.4f %7.4f %7.4f %5d\t\t%s %8d %6d\t%s\n",
			data.stdoutdata[pos].framenum+total_frames*256, data.stdoutdata[pos].tr+total_frames*256, data.stdoutdata[pos].qp, data.stdoutdata[pos].snr_y, data.stdoutdata[pos].snr_u, data.stdoutdata[pos].snr_v, data.stdoutdata[pos].tmp_time, Frmfld, data.stdoutdata[pos].curr_frame_bits, data.stdoutdata[pos].emulate_bits, MD5str );
		fprintf ( file, "%3d(P)  %3d %5d %7.4f %7.4f %7.4f %5d\t\t%s\t%s\n",
			data.stdoutdata[pos].tr+total_frames*256, data.stdoutdata[pos].tr+total_frames*256, data.stdoutdata[pos].qp, data.stdoutdata[pos].snr_y, data.stdoutdata[pos].snr_u, data.stdoutdata[pos].snr_v, data.stdoutdata[pos].tmp_time, Frmfld, MD5str );
	}
	else if ( data.stdoutdata[pos].type == P_IMG && data.stdoutdata[pos].typeb == BP_IMG)
	{
		printf ( "%3d(S)  %3d %5d %7.4f %7.4f %7.4f %5d\t\t%s %8d %6d\t%s\n",
			data.stdoutdata[pos].framenum+total_frames*256, data.stdoutdata[pos].tr+total_frames*256, data.stdoutdata[pos].qp, data.stdoutdata[pos].snr_y, data.stdoutdata[pos].snr_u, data.stdoutdata[pos].snr_v, data.stdoutdata[pos].tmp_time, Frmfld, data.stdoutdata[pos].curr_frame_bits, data.stdoutdata[pos].emulate_bits, MD5str );

		fprintf ( file, "%3d(S)  %3d %5d %7.4f %7.4f %7.4f %5d\t\t%s\t%s\n",
			data.stdoutdata[pos].tr+total_frames*256, data.stdoutdata[pos].tr+total_frames*256, data.stdoutdata[pos].qp, data.stdoutdata[pos].snr_y, data.stdoutdata[pos].snr_u, data.stdoutdata[pos].snr_v, data.stdoutdata[pos].tmp_time, Frmfld, MD5str ); 
	}
	else if ( data.stdoutdata[pos].type ==F_IMG ) // F pictures
	{
		printf ( "%3d(F)  %3d %5d %7.4f %7.4f %7.4f %5d\t\t%s %8d %6d\t%s\n",
			data.stdoutdata[pos].framenum+total_frames*256, data.stdoutdata[pos].tr+total_frames*256, data.stdoutdata[pos].qp, data.stdoutdata[pos].snr_y, data.stdoutdata[pos].snr_u, data.stdoutdata[pos].snr_v, data.stdoutdata[pos].tmp_time, Frmfld, data.stdoutdata[pos].curr_frame_bits, data.stdoutdata[pos].emulate_bits, MD5str );

		fprintf ( file, "%3d(F)  %3d %5d %7.4f %7.4f %7.4f %5d\t\t%s\t%s\n",
			data.stdoutdata[pos].tr+total_frames*256, data.stdoutdata[pos].tr+total_frames*256, data.stdoutdata[pos].qp, data.stdoutdata[pos].snr_y, data.stdoutdata[pos].snr_u, data.stdoutdata[pos].snr_v, data.stdoutdata[pos].tmp_time, Frmfld, MD5str );
	}
	else // B pictures
	{
		printf ( "%3d(B)  %3d %5d %7.4f %7.4f %7.4f %5d\t\t%s %8d %6d\t%s\n",
			data.stdoutdata[pos].framenum+total_frames*256, data.stdoutdata[pos].tr+total_frames*256, data.stdoutdata[pos].qp, data.stdoutdata[pos].snr_y, data.stdoutdata[pos].snr_u, data.stdoutdata[pos].snr_v, data.stdoutdata[pos].tmp_time, Frmfld, data.stdoutdata[pos].curr_frame_bits, data.stdoutdata[pos].emulate_bits, MD5str );
		fprintf ( file, "%3d(B)  %3d %5d %7.4f %7.4f %7.4f %5d\t\t%s\t%s\n",
			data.stdoutdata[pos].tr+total_frames*256, data.stdoutdata[pos].tr+total_frames*256, data.stdoutdata[pos].qp, data.stdoutdata[pos].snr_y, data.stdoutdata[pos].snr_u, data.stdoutdata[pos].snr_v, data.stdoutdata[pos].tmp_time, Frmfld, MD5str ); //20080721
	}

#else

	if ( data.stdoutdata[pos].type == I_IMG ) // I picture
	{
#ifdef AVS2_S2_BGLONGTERM
#if AVS2_SCENE_CD
		if(data.stdoutdata[pos].typeb == BACKGROUND_IMG)
#else
		if(data.stdoutdata[pos].typeb == BACKGROUND_IMG && input->profile_id == 0x50)
#endif
		{
			if(background_picture_output_flag)
			{
				printf ( "%3d(G)  %3d %5d %7.4f %7.4f %7.4f %5d\t\t%s %8d %6d\n",
					data.stdoutdata[pos].framenum+total_frames*256, data.stdoutdata[pos].tr+total_frames*256, data.stdoutdata[pos].qp, data.stdoutdata[pos].snr_y, data.stdoutdata[pos].snr_u, data.stdoutdata[pos].snr_v, data.stdoutdata[pos].tmp_time, Frmfld, data.stdoutdata[pos].curr_frame_bits, data.stdoutdata[pos].emulate_bits );

				fprintf ( file, "%3d(G)  %3d %5d %7.4f %7.4f %7.4f %5d\t\t%s\n",
					data.stdoutdata[pos].framenum+total_frames*256, data.stdoutdata[pos].tr+total_frames*256, data.stdoutdata[pos].qp, data.stdoutdata[pos].snr_y, data.stdoutdata[pos].snr_u, data.stdoutdata[pos].snr_v, data.stdoutdata[pos].tmp_time, Frmfld ); //20080721
			}
			else
			{
				printf ( "%3d(GB) %3d %5d %7.4f %7.4f %7.4f %5d\t\t%s %8d %6d\n",
					data.stdoutdata[pos].framenum+total_frames*256, data.stdoutdata[pos].tr+total_frames*256, data.stdoutdata[pos].qp, data.stdoutdata[pos].snr_y, data.stdoutdata[pos].snr_u, data.stdoutdata[pos].snr_v, data.stdoutdata[pos].tmp_time, Frmfld, data.stdoutdata[pos].curr_frame_bits, data.stdoutdata[pos].emulate_bits );

				fprintf ( file, "%3d(GB) %3d %5d %7.4f %7.4f %7.4f %5d\t\t%s\n",
					data.stdoutdata[pos].framenum+total_frames*256, data.stdoutdata[pos].tr+total_frames*256, data.stdoutdata[pos].qp, data.stdoutdata[pos].snr_y, data.stdoutdata[pos].snr_u, data.stdoutdata[pos].snr_v, data.stdoutdata[pos].tmp_time, Frmfld ); //20080721
			}
		}
		else
		{
			printf ( "%3d(I)  %3d %5d %7.4f %7.4f %7.4f %5d\t\t%s %8d %6d\n",
				data.stdoutdata[pos].framenum+total_frames*256, data.stdoutdata[pos].tr+total_frames*256, data.stdoutdata[pos].qp, data.stdoutdata[pos].snr_y, data.stdoutdata[pos].snr_u, data.stdoutdata[pos].snr_v, data.stdoutdata[pos].tmp_time, Frmfld, data.stdoutdata[pos].curr_frame_bits, data.stdoutdata[pos].emulate_bits );

			fprintf ( file, "%3d(I)  %3d %5d %7.4f %7.4f %7.4f %5d\t\t%s\n",
				data.stdoutdata[pos].framenum+total_frames*256, data.stdoutdata[pos].tr+total_frames*256, data.stdoutdata[pos].qp, data.stdoutdata[pos].snr_y, data.stdoutdata[pos].snr_u, data.stdoutdata[pos].snr_v, data.stdoutdata[pos].tmp_time, Frmfld ); //20080721
		}
#else
		printf ( "%3d(I)  %3d %5d %7.4f %7.4f %7.4f %5d\t\t%s %8d %6d\n",
			data.stdoutdata[pos].framenum, data.stdoutdata[pos].tr, data.stdoutdata[pos].qp, data.stdoutdata[pos].snr_y, data.stdoutdata[pos].snr_u, data.stdoutdata[pos].snr_v, data.stdoutdata[pos].tmp_time, Frmfld, data.stdoutdata[pos].curr_frame_bits, data.stdoutdata[pos].emulate_bits );

		fprintf ( file, "%3d(I)  %3d %5d %7.4f %7.4f %7.4f %5d\t\t%s\n",
			data.stdoutdata[pos].framenum, data.stdoutdata[pos].tr, data.stdoutdata[pos].qp, data.stdoutdata[pos].snr_y, data.stdoutdata[pos].snr_u, data.stdoutdata[pos].snr_v, data.stdoutdata[pos].tmp_time, Frmfld ); //20080721
#endif
	}

#ifdef AVS2_S2_S
	else if ( data.stdoutdata[pos].type == P_IMG && data.stdoutdata[pos].typeb != BP_IMG)
#else
	else if ( data.stdoutdata[pos].type ==P_IMG ) // P pictures
#endif
	{
		printf ( "%3d(P)  %3d %5d %7.4f %7.4f %7.4f %5d\t\t%s %8d %6d\n",
			data.stdoutdata[pos].framenum+total_frames*256, data.stdoutdata[pos].tr+total_frames*256, data.stdoutdata[pos].qp, data.stdoutdata[pos].snr_y, data.stdoutdata[pos].snr_u, data.stdoutdata[pos].snr_v, data.stdoutdata[pos].tmp_time, Frmfld, data.stdoutdata[pos].curr_frame_bits, data.stdoutdata[pos].emulate_bits );

		fprintf ( file, "%3d(P)  %3d %5d %7.4f %7.4f %7.4f %5d\t\t%s\n",
			data.stdoutdata[pos].tr+total_frames*256, data.stdoutdata[pos].tr+total_frames*256, data.stdoutdata[pos].qp, data.stdoutdata[pos].snr_y, data.stdoutdata[pos].snr_u, data.stdoutdata[pos].snr_v, data.stdoutdata[pos].tmp_time, Frmfld ); //20080721
	}
#ifdef AVS2_S2_S
	else if ( data.stdoutdata[pos].type == P_IMG && data.stdoutdata[pos].typeb == BP_IMG)
	{
		printf ( "%3d(S)  %3d %5d %7.4f %7.4f %7.4f %5d\t\t%s %8d %6d\n",
			data.stdoutdata[pos].framenum+total_frames*256, data.stdoutdata[pos].tr+total_frames*256, data.stdoutdata[pos].qp, data.stdoutdata[pos].snr_y, data.stdoutdata[pos].snr_u, data.stdoutdata[pos].snr_v, data.stdoutdata[pos].tmp_time, Frmfld, data.stdoutdata[pos].curr_frame_bits, data.stdoutdata[pos].emulate_bits );

		fprintf ( file, "%3d(S)  %3d %5d %7.4f %7.4f %7.4f %5d\t\t%s\n",
			data.stdoutdata[pos].tr+total_frames*256, data.stdoutdata[pos].tr+total_frames*256, data.stdoutdata[pos].qp, data.stdoutdata[pos].snr_y, data.stdoutdata[pos].snr_u, data.stdoutdata[pos].snr_v, data.stdoutdata[pos].tmp_time, Frmfld ); 
	}
#endif
	else if ( data.stdoutdata[pos].type ==F_IMG ) // F pictures
	{
		printf ( "%3d(F)  %3d %5d %7.4f %7.4f %7.4f %5d\t\t%s %8d %6d\n",
			data.stdoutdata[pos].framenum+total_frames*256, data.stdoutdata[pos].tr+total_frames*256, data.stdoutdata[pos].qp, data.stdoutdata[pos].snr_y, data.stdoutdata[pos].snr_u, data.stdoutdata[pos].snr_v, data.stdoutdata[pos].tmp_time, Frmfld, data.stdoutdata[pos].curr_frame_bits, data.stdoutdata[pos].emulate_bits );

		fprintf ( file, "%3d(F)  %3d %5d %7.4f %7.4f %7.4f %5d\t\t%s\n",
			data.stdoutdata[pos].tr+total_frames*256, data.stdoutdata[pos].tr+total_frames*256, data.stdoutdata[pos].qp, data.stdoutdata[pos].snr_y, data.stdoutdata[pos].snr_u, data.stdoutdata[pos].snr_v, data.stdoutdata[pos].tmp_time, Frmfld ); //20080721
	}
	else // B pictures
	{
		printf ( "%3d(B)  %3d %5d %7.4f %7.4f %7.4f %5d\t\t%s %8d %6d\n",
			data.stdoutdata[pos].framenum+total_frames*256, data.stdoutdata[pos].tr+total_frames*256, data.stdoutdata[pos].qp, data.stdoutdata[pos].snr_y, data.stdoutdata[pos].snr_u, data.stdoutdata[pos].snr_v, data.stdoutdata[pos].tmp_time, Frmfld, data.stdoutdata[pos].curr_frame_bits, data.stdoutdata[pos].emulate_bits );

		fprintf ( file, "%3d(B)  %3d %5d %7.4f %7.4f %7.4f %5d\t\t%s\n",
			data.stdoutdata[pos].tr+total_frames*256, data.stdoutdata[pos].tr+total_frames*256, data.stdoutdata[pos].qp, data.stdoutdata[pos].snr_y, data.stdoutdata[pos].snr_u, data.stdoutdata[pos].snr_v, data.stdoutdata[pos].tmp_time, Frmfld ); //20080721
	}

#endif
	fclose ( file );
	fflush ( stdout );
	FrameNum++;
}
/*
*************************************************************************
* Function:Find PSNR for all three components.Compare decoded frame with
the original sequence. Read input->jumpd frames to reflect frame skipping.
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
#ifdef WIN32
void find_snr (SNRParameters *snr, int p_ref )
#else
void find_snr (SNRParameters *snr, FILE *p_ref )          //!< filestream to reference YUV file
#endif
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
  int uv;
  int uvformat = input->chroma_format == 1 ? 4 : 2;

#if EXTEND_BD
  int nOutputSampleSize = (input->output_bit_depth > 8) ? 2 : 1;
  unsigned char chTemp[2];
  int nBitDepthDiff = input->sample_bit_depth - input->output_bit_depth; // assume coding bit depth no less than output bit depth
#endif

#ifdef WIN32
  __int64 status;
#else
  int  status;
#endif

  int img_width = ( img->width - img->auto_crop_right );
  int img_height = ( img->height - img->auto_crop_bottom );
  int img_width_cr = ( img_width / 2 );
  int img_height_cr = ( img_height / ( input->chroma_format == 1 ? 2 : 1 ) );
#ifdef WIN32
  const unsigned int  bytes_y = img_width * img_height;
  const unsigned int  bytes_uv = img_width_cr * img_height_cr;
  const __int64 framesize_in_bytes = ( __int64 ) ( bytes_y + 2 * bytes_uv );
#else
  const unsigned int  bytes_y = img_width * img_height;
  const unsigned int  bytes_uv = img_width_cr * img_height_cr;
  const int64_t framesize_in_bytes = bytes_y + 2 * bytes_uv;
#endif

  unsigned char *buf;

  int   offset_units;

  if ( !input->yuv_structure )
  {
#if EXTEND_BD
    buf = malloc ( nOutputSampleSize * bytes_y );
#else
    buf = malloc ( bytes_y );
#endif
  }
  else
  {
#if EXTEND_BD
    buf = malloc ( nOutputSampleSize * (bytes_y + 2 * bytes_uv) );
#else
    buf = malloc ( bytes_y + 2 * bytes_uv );
#endif
  }

  if ( NULL == buf )
  {
    no_mem_exit ( "find_snr: buf" );
  }

  snr->snr_y = 0.0;
  snr->snr_u = 0.0;
  snr->snr_v = 0.0;

#ifndef WIN32
  rewind ( p_ref );
#endif

  if ( RefPicExist )
  {
#ifdef WIN32

    if ( !input->ref_pic_order ) //ref order
    {
#if EXTEND_BD
      status = _lseeki64 ( p_ref, nOutputSampleSize * framesize_in_bytes * (img->tr+total_frames*256), SEEK_SET );
#else
      status = _lseeki64 ( p_ref, framesize_in_bytes * img->tr , SEEK_SET );
#endif
    }
    else
    {
#if EXTEND_BD
      status = _lseeki64 ( p_ref, nOutputSampleSize * framesize_in_bytes * dec_ref_num, SEEK_SET );
#else
      status = _lseeki64 ( p_ref, framesize_in_bytes * dec_ref_num, SEEK_SET );
#endif
    }

#else

    if ( !input->ref_pic_order ) //ref order
    {
#if EXTEND_BD
	  status = fseeko ( p_ref, (off_t)(nOutputSampleSize * framesize_in_bytes * (img->tr+total_frames*256)/*( img->type == B_IMG ? img->tr : FrameNum )*/), 0 );
#else
      status = fseek ( p_ref, framesize_in_bytes * img->tr/*( img->type == B_IMG ? img->tr : FrameNum )*/, 0 );
#endif
    }
    else
    {
#if EXTEND_BD
      status = fseeko ( p_ref, (off_t)(nOutputSampleSize * framesize_in_bytes * dec_ref_num), 0 );
#else
      status = fseek ( p_ref, framesize_in_bytes * dec_ref_num, 0 );
#endif
    }

#endif

#ifdef WIN32

    if ( status == -1 )
#else
    if ( status != 0 )
#endif
    {
      snprintf ( errortext, ET_SIZE, "Error in seeking img->tr: %d", img->tr );
      RefPicExist = 0;
    }
  }

#ifdef WIN32

  if ( !input->yuv_structure )
  {
#if EXTEND_BD
    _read ( p_ref, buf, nOutputSampleSize * bytes_y );
#else
    _read ( p_ref, buf, bytes_y );
#endif
  }
  else
  {
#if EXTEND_BD
    _read ( p_ref, buf, nOutputSampleSize * (bytes_y + 2 * bytes_uv) );
#else
    _read ( p_ref, buf, bytes_y + 2 * bytes_uv );
#endif
  }

  if ( !input->yuv_structure ) //YUV
  {
    for ( j = 0; j < img_height; j++ )
    {
#if EXTEND_BD
      if (input->output_bit_depth == 8)
      {
        for ( i = 0; i < img_width; i++)
        {
          imgYRef[j][i] = buf[j * img_width + i];
          imgYRef[j][i] &= 0xff; // reset high 8 bits
        }
      }
      else if (input->output_bit_depth == 10)
      {
        for ( i = 0; i < img_width; i++)
        {
          imgYRef[j][i] = ((byte *)buf)[j * img_width + i] & 0x3ff;
        }
      }
      else if (input->output_bit_depth == 12)
      {
        for ( i = 0; i < img_width; i++)
        {
          imgYRef[j][i] = ((byte *)buf)[j * img_width + i] & 0xfff;
        }
      }

#else
      memcpy ( &imgYRef[j][0], buf + j * img_width, img_width );
#endif
    }
  }
  else  //U0Y0 V1Y1
  {
    for ( j = 0; j < img_height; j++ )
    {
      for ( i = 0; i < img_width; i++ )
      {
#if EXTEND_BD
        if (input->output_bit_depth == 8)
        {
          offset_units = j * img_width + i;
          imgYRef[j][i] = buf[2 * offset_units + 1];
          imgYRef[j][i] &= 0xff;
          if ( offset_units % 2 == 0 ) //U component
          {
            imgUVRef[0][j][i / 2] = buf[2 * offset_units];
          }
          else                  //V component
          {
            imgUVRef[1][j][i / 2] = buf[2 * offset_units];
          }
        }
        else if (input->output_bit_depth == 10)
        {
          offset_units = j * img_width + i;
          imgYRef[j][i] = ((byte *)buf)[2 * offset_units + 1];
          imgYRef[j][i] = imgYRef[j][i] & 0x3ff;
          if ( offset_units % 2 == 0 ) //U component
          {
            imgUVRef[0][j][i / 2] = ((byte *)buf)[2 * offset_units];
            imgUVRef[0][j][i / 2] &= 0x3ff;
          }
          else                  //V component
          {
            imgUVRef[1][j][i / 2] = ((byte *)buf)[2 * offset_units];
            imgUVRef[1][j][i / 2] &= 0x3ff;
          }
        }
        else if (input->output_bit_depth == 12)
        {
          offset_units = j * img_width + i;
          imgYRef[j][i] = ((byte *)buf)[2 * offset_units + 1];
          imgYRef[j][i] = imgYRef[j][i] & 0xfff;
          if ( offset_units % 2 == 0 ) //U component
          {
            imgUVRef[0][j][i / 2] = ((byte *)buf)[2 * offset_units];
            imgUVRef[0][j][i / 2] &= 0xfff;
          }
          else                  //V component
          {
            imgUVRef[1][j][i / 2] = ((byte *)buf)[2 * offset_units];
            imgUVRef[1][j][i / 2] &= 0xfff;
          }
        }

#else
        offset_units = j * img_width + i;
        imgYRef[j][i] = buf[2 * offset_units + 1];

        if ( offset_units % 2 == 0 ) //U component
        {
          imgUVRef[0][j][i / 2] = buf[2 * offset_units];
        }
        else                  //V component
        {
          imgUVRef[1][j][i / 2] = buf[2 * offset_units];
        }
#endif
      }
    }
  }

#else
#if EXTEND_BD
  if ( !input->yuv_structure )
  {
    for ( j = 0; j < img_height; j++ )
    {
      for ( i = 0; i < img_width; i++ )
      {
        if (input->output_bit_depth == 8)
        {
          chTemp[0] = fgetc( p_ref );
          imgYRef[j][i] = chTemp[0];
        }
        else if (input->output_bit_depth == 10)
        {
          chTemp[0] = fgetc( p_ref );
          chTemp[1] = fgetc( p_ref );
          imgYRef[j][i] = ((byte *)(&(chTemp[0])))[0] & 0x3ff;
        }
        else if (input->output_bit_depth == 12)
        {
          chTemp[0] = fgetc( p_ref );
          chTemp[1] = fgetc( p_ref );
          imgYRef[j][i] = ((byte *)(&(chTemp[0])))[0] & 0xfff;
        }
      }
    }
  }
  else
  {
    for ( j = 0; j < img_height; j++ )
    {
      for ( i = 0; i < img_width; i++ )
      {
        if (input->output_bit_depth == 8)
        {
          offset_units = j * img_width + i;
          if ( offset_units % 2 == 0 ) //U component
          {
            imgUVRef[0][j][i / 2] = fgetc ( p_ref );
          }
          else          //V component
          {
            imgUVRef[1][j][i / 2] = fgetc ( p_ref );
          }

          imgYRef[j][i] = fgetc ( p_ref );
        }
        else if (input->output_bit_depth == 10)
        {
          offset_units = j * img_width + i;
          if ( offset_units % 2 == 0 ) //U component
          {
            chTemp[0] = fgetc( p_ref );
            chTemp[1] = fgetc( p_ref );
            imgUVRef[0][j][i / 2] = ((byte *)(&(chTemp[0])))[0] & 0x3ff;
          }
          else          //V component
          {
            chTemp[0] = fgetc( p_ref );
            chTemp[1] = fgetc( p_ref );
            imgUVRef[1][j][i / 2] = ((byte *)(&(chTemp[0])))[0] & 0x3ff;
          }
          chTemp[0] = fgetc( p_ref );
          chTemp[1] = fgetc( p_ref );
          imgYRef[j][i] = ((byte *)(&(chTemp[0])))[0] & 0x3ff;
        }
        else if (input->output_bit_depth == 12)
        {
          offset_units = j * img_width + i;
          if ( offset_units % 2 == 0 ) //U component
          {
            chTemp[0] = fgetc( p_ref );
            chTemp[1] = fgetc( p_ref );
            imgUVRef[0][j][i / 2] = ((byte *)(&(chTemp[0])))[0] & 0xfff;
          }
          else          //V component
          {
            chTemp[0] = fgetc( p_ref );
            chTemp[1] = fgetc( p_ref );
            imgUVRef[1][j][i / 2] = ((byte *)(&(chTemp[0])))[0] & 0xfff;
          }
          chTemp[0] = fgetc( p_ref );
          chTemp[1] = fgetc( p_ref );
          imgYRef[j][i] = ((byte *)(&(chTemp[0])))[0] & 0xfff;
        }
      }
    }
  }
#else
  if ( !input->yuv_structure )
  {
    for ( j = 0; j < img_height; j++ )
    {
      for ( i = 0; i < img_width; i++ )
      {
        imgYRef[j][i] = fgetc ( p_ref );
      }
    }
  }
  else
  {
    for ( j = 0; j < img_height; j++ )
    {
      for ( i = 0; i < img_width; i++ )
      {
        offset_units = j * img_width + i;

        if ( offset_units % 2 == 0 ) //U component
        {
          imgUVRef[0][j][i / 2] = fgetc ( p_ref );
        }
        else          //V component
        {
          imgUVRef[1][j][i / 2] = fgetc ( p_ref );
        }

        imgYRef[j][i] = fgetc ( p_ref );
      }
    }
  }
#endif
#endif

  for ( uv = 0; uv < 2; uv++ )
  {
    if ( !input->yuv_structure )
    {
#ifdef WIN32
#if EXTEND_BD
      _read ( p_ref, buf, nOutputSampleSize * img_height_cr * img_width_cr );
#else
      _read ( p_ref, buf, img_height_cr * img_width_cr );
#endif

      for ( j = 0; j < img_height_cr; j++ )
      {
#if EXTEND_BD
        for ( i = 0; i < img_width_cr; i++)
        {
          if (input->output_bit_depth == 8)
          {            
            imgUVRef[uv][j][i] = buf[j * img_width_cr + i];
            imgUVRef[uv][j][i] &= 0xff;
          }
          else if (input->output_bit_depth == 10)
          {
            imgUVRef[uv][j][i] = ((byte *)buf)[j * img_width_cr + i] & 0x3ff;
          }
          else if (input->output_bit_depth == 12)
          {
            imgUVRef[uv][j][i] = ((byte *)buf)[j * img_width_cr + i] & 0xfff;
          }
        }
#else
        memcpy ( &imgUVRef[uv][j][0], buf + j * img_width_cr, img_width_cr );
#endif
      }

#else

      for ( j = 0; j < img_height_cr; j++ )
      {
        for ( i = 0; i < img_width_cr; i++ )
        {
#if EXTEND_BD
          if (input->output_bit_depth == 8)
          {
            imgUVRef[uv][j][i] = fgetc ( p_ref );
          }
          else if (input->output_bit_depth == 10)
          {
            chTemp[0] = fgetc ( p_ref );
            chTemp[1] = fgetc ( p_ref );
            imgUVRef[uv][j][i] = ((byte *)(&(chTemp[0])))[0] & 0x3ff;
          }
          else if (input->output_bit_depth == 12)
          {
            chTemp[0] = fgetc ( p_ref );
            chTemp[1] = fgetc ( p_ref );
            imgUVRef[uv][j][i] = ((byte *)(&(chTemp[0])))[0] & 0xfff;
          }
#else
          imgUVRef[uv][j][i] = fgetc ( p_ref );
#endif
        }
      }

#endif
    }
  }

  img->quad[0] = 0;
  diff_y = 0;

  for ( j = 0; j < img_height; ++j )
  {
    for ( i = 0; i < img_width; ++i )
    {
#if EXTEND_BD
      if (nBitDepthDiff == 0)
      {
        diff_y += img->quad[abs ( imgY[j][i] - imgYRef[j][i] ) ];
      }
      else if (nBitDepthDiff > 0)
      {
        diff_y += img->quad[abs ( Clip1((imgY[j][i] + (1 << (nBitDepthDiff - 1))) >> nBitDepthDiff)  - imgYRef[j][i] ) ];
      }

#else
      diff_y += img->quad[abs ( imgY[j][i] - imgYRef[j][i] ) ];
#endif
    }
  }

  // Chroma
  diff_u = 0;
  diff_v = 0;

  for ( j = 0; j < img_height_cr; ++j )
  {
    for ( i = 0; i < img_width_cr; ++i )
    {
#if EXTEND_BD
      if (nBitDepthDiff == 0)
      {
        diff_u += img->quad[abs ( imgUVRef[0][j][i] - imgUV[0][j][i] ) ];
        diff_v += img->quad[abs ( imgUVRef[1][j][i] - imgUV[1][j][i] ) ];
      }
      else if (nBitDepthDiff > 0)
      {
        diff_u += img->quad[abs ( imgUVRef[0][j][i] - Clip1((imgUV[0][j][i] + (1 << (nBitDepthDiff - 1))) >> nBitDepthDiff) ) ];
        diff_v += img->quad[abs ( imgUVRef[1][j][i] - Clip1((imgUV[1][j][i] + (1 << (nBitDepthDiff - 1))) >> nBitDepthDiff) ) ];
      }
#else
      diff_u += img->quad[abs ( imgUVRef[0][j][i] - imgUV[0][j][i] ) ];
      diff_v += img->quad[abs ( imgUVRef[1][j][i] - imgUV[1][j][i] ) ];
#endif
    }
  }

  // Collecting SNR statistics
  if ( diff_y != 0 )
  {
#if EXTEND_BD
    snr->snr_y = ( float ) ( 10 * log10 ( ((1 << input->output_bit_depth) - 1) * ((1 << input->output_bit_depth) - 1) * ( float ) ( img_width/*img->width*/ ) * ( img_height/*img->height*/ ) / ( float ) diff_y ) );  // luma snr for current frame
#else
    snr->snr_y = ( float ) ( 10 * log10 ( 65025 * ( float ) ( img_width/*img->width*/ ) * ( img_height/*img->height*/ ) / ( float ) diff_y ) );  // luma snr for current frame
#endif
  }

  if ( diff_u != 0 )
  {
#if EXTEND_BD
    snr->snr_u = ( float ) ( 10 * log10 ( ((1 << input->output_bit_depth) - 1) * ((1 << input->output_bit_depth) - 1) * ( float ) ( img_width/*img->width*/ ) * ( img_height/*img->height*/ ) / ( float ) ( /*4*/uvformat * diff_u ) ) );  //  chroma snr for current frame,422
#else
    snr->snr_u = ( float ) ( 10 * log10 ( 65025 * ( float ) ( img_width/*img->width*/ ) * ( img_height/*img->height*/ ) / ( float ) ( /*4*/uvformat * diff_u ) ) );  //  chroma snr for current frame,422
#endif
  }

  if ( diff_v != 0 )
  {
#if EXTEND_BD
    snr->snr_v = ( float ) ( 10 * log10 ( ((1 << input->output_bit_depth) - 1) * ((1 << input->output_bit_depth) - 1) * ( float ) ( img_width/*img->width*/ ) * ( img_height/*img->height*/ ) / ( float ) ( /*4*/uvformat * diff_v ) ) );  //  chroma snr for current frame,422
#else
    snr->snr_v = ( float ) ( 10 * log10 ( 65025 * ( float ) ( img_width/*img->width*/ ) * ( img_height/*img->height*/ ) / ( float ) ( /*4*/uvformat * diff_v ) ) );  //  chroma snr for current frame,422
#endif
  }

  if ( img->number == 0 ) // first
  {
#if EXTEND_BD
    snr->snr_y1 = ( float ) ( 10 * log10 ( ((1 << input->output_bit_depth) - 1) * ((1 << input->output_bit_depth) - 1) * ( float ) ( img_width/*img->width*/ ) * ( img_height/*img->height*/ ) / ( float ) diff_y ) ); // keep luma snr for first frame
    snr->snr_u1 = ( float ) ( 10 * log10 ( ((1 << input->output_bit_depth) - 1) * ((1 << input->output_bit_depth) - 1) * ( float ) ( img_width/*img->width*/ ) * ( img_height/*img->height*/ ) / ( float ) ( /*4*/uvformat * diff_u ) ) ); // keep chroma snr for first frame,422
    snr->snr_v1 = ( float ) ( 10 * log10 ( ((1 << input->output_bit_depth) - 1) * ((1 << input->output_bit_depth) - 1) * ( float ) ( img_width/*img->width*/ ) * ( img_height/*img->height*/ ) / ( float ) ( /*4*/uvformat * diff_v ) ) ); // keep chroma snr for first frame,422
#else
    snr->snr_y1 = ( float ) ( 10 * log10 ( 65025 * ( float ) ( img_width/*img->width*/ ) * ( img_height/*img->height*/ ) / ( float ) diff_y ) ); // keep luma snr for first frame
    snr->snr_u1 = ( float ) ( 10 * log10 ( 65025 * ( float ) ( img_width/*img->width*/ ) * ( img_height/*img->height*/ ) / ( float ) ( /*4*/uvformat * diff_u ) ) ); // keep chroma snr for first frame,422
    snr->snr_v1 = ( float ) ( 10 * log10 ( 65025 * ( float ) ( img_width/*img->width*/ ) * ( img_height/*img->height*/ ) / ( float ) ( /*4*/uvformat * diff_v ) ) ); // keep chroma snr for first frame,422
#endif
    snr->snr_ya = snr->snr_y1;
    snr->snr_ua = snr->snr_u1;
    snr->snr_va = snr->snr_v1;

    if ( diff_y == 0 )
    {
      snr->snr_ya =/*50*/0;
    }

    if ( diff_u == 0 )
    {
      snr->snr_ua =/*50*/0;
    }

    if ( diff_v == 0 )
    {
      snr->snr_va =/*50*/0;
    }
  }
  else
  {
	  snr->snr_ya = ( snr->snr_ya * (double) ( img->number + Bframe_ctr ) + snr->snr_y ) / (double) ( img->number + Bframe_ctr + 1 ); // average snr lume for all frames inc. first
	  snr->snr_ua = ( snr->snr_ua * (double) ( img->number + Bframe_ctr ) + snr->snr_u ) / (double) ( img->number + Bframe_ctr + 1 ); // average snr u croma for all frames inc. first    
	  snr->snr_va = ( snr->snr_va * (double) ( img->number + Bframe_ctr ) + snr->snr_v ) / (double) ( img->number + Bframe_ctr + 1 ); // average snr v croma for all frames inc. first
  }

  free ( buf );
}

#if AVS2_SCENE_CD
/*
*************************************************************************
* Function:Find PSNR for BACKGROUND PICTURE.Compare decoded frame with
the original sequence. Read input->jumpd frames to reflect frame skipping.
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
#ifdef WIN32
void find_snr_background (SNRParameters *snr, int p_ref )
#else
void find_snr_background (SNRParameters *snr, FILE *p_ref )          //!< filestream to reference YUV file
#endif
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
  int uv;
  int uvformat = input->chroma_format == 1 ? 4 : 2;

#if EXTEND_BD
  int nOutputSampleSize = (input->output_bit_depth > 8) ? 2 : 1;
  unsigned char chTemp[2];
  int nBitDepthDiff = input->sample_bit_depth - input->output_bit_depth; // assume coding bit depth no less than output bit depth
#endif

#ifdef WIN32
  __int64 status;
#else
  int  status;
#endif

  int img_width = ( img->width - img->auto_crop_right );
  int img_height = ( img->height - img->auto_crop_bottom );
  int img_width_cr = ( img_width / 2 );
  int img_height_cr = ( img_height / ( input->chroma_format == 1 ? 2 : 1 ) );
#ifdef WIN32
  const unsigned int  bytes_y = img_width * img_height;
  const unsigned int  bytes_uv = img_width_cr * img_height_cr;
  const __int64 framesize_in_bytes = ( __int64 ) ( bytes_y + 2 * bytes_uv );
#else
  const unsigned int  bytes_y = img_width * img_height;
  const unsigned int  bytes_uv = img_width_cr * img_height_cr;
  const int64_t framesize_in_bytes = bytes_y + 2 * bytes_uv;
#endif

  unsigned char *buf;

  int   offset_units;

  if ( !input->yuv_structure )
  {
#if EXTEND_BD
    buf = malloc ( nOutputSampleSize * bytes_y );
#else
    buf = malloc ( bytes_y );
#endif
  }
  else
  {
#if EXTEND_BD
    buf = malloc ( nOutputSampleSize * (bytes_y + 2 * bytes_uv) );
#else
    buf = malloc ( bytes_y + 2 * bytes_uv );
#endif
  }

  if ( NULL == buf )
  {
    no_mem_exit ( "find_snr_background: buf" );
  }

  snr->snr_y = 0.0;
  snr->snr_u = 0.0;
  snr->snr_v = 0.0;

#ifndef WIN32
  rewind ( p_ref );
#endif

  if ( BgRefPicExist )
  {
#ifdef WIN32

    if ( !input->ref_pic_order ) //ref order
    {
#if EXTEND_BD
      status = _lseeki64 ( p_ref, nOutputSampleSize * framesize_in_bytes * (background_number-1) , SEEK_SET );
#else
      status = _lseeki64 ( p_ref, framesize_in_bytes * (background_number-1) , SEEK_SET );
#endif
    }
    else
    {
#if EXTEND_BD
      status = _lseeki64 ( p_ref, nOutputSampleSize * framesize_in_bytes * (background_number-1), SEEK_SET );
#else
      status = _lseeki64 ( p_ref, framesize_in_bytes * (background_number-1), SEEK_SET );
#endif
    }

#else

    if ( !input->ref_pic_order ) //ref order
    {
#if EXTEND_BD
      status = fseeko ( p_ref, (off_t)(nOutputSampleSize * framesize_in_bytes * (background_number-1)), 0 );
#else
      status = fseek ( p_ref, framesize_in_bytes * (background_number-1), 0 );
#endif
    }
    else
    {
#if EXTEND_BD
      status = fseeko ( p_ref, (off_t)(nOutputSampleSize * framesize_in_bytes * (background_number-1)), 0 );
#else
      status = fseek ( p_ref, framesize_in_bytes * (background_number-1), 0 );
#endif
    }

#endif

#ifdef WIN32

    if ( status == -1 )
#else
    if ( status != 0 )
#endif
    {
      snprintf ( errortext, ET_SIZE, "Error in seeking img->tr: %d", img->tr );
      BgRefPicExist = 0;
    }
  }

#ifdef WIN32

  if ( !input->yuv_structure )
  {
#if EXTEND_BD
    _read ( p_ref, buf, nOutputSampleSize * bytes_y );
#else
    _read ( p_ref, buf, bytes_y );
#endif
  }
  else
  {
#if EXTEND_BD
    _read ( p_ref, buf, nOutputSampleSize * (bytes_y + 2 * bytes_uv) );
#else
    _read ( p_ref, buf, bytes_y + 2 * bytes_uv );
#endif
  }

  if ( !input->yuv_structure ) //YUV
  {
    for ( j = 0; j < img_height; j++ )
    {
#if EXTEND_BD
      if (input->output_bit_depth == 8)
      {
        for ( i = 0; i < img_width; i++)
        {
          imgYRef[j][i] = buf[j * img_width + i];
          imgYRef[j][i] &= 0xff; // reset high 8 bits
        }
      }
      else if (input->output_bit_depth == 10)
      {
        for ( i = 0; i < img_width; i++)
        {
          imgYRef[j][i] = ((byte *)buf)[j * img_width + i] & 0x3ff;
        }
      }
      else if (input->output_bit_depth == 12)
      {
        for ( i = 0; i < img_width; i++)
        {
          imgYRef[j][i] = ((byte *)buf)[j * img_width + i] & 0xfff;
        }
      }

#else
      memcpy ( &imgYRef[j][0], buf + j * img_width, img_width );
#endif
    }
  }
  else  //U0Y0 V1Y1
  {
    for ( j = 0; j < img_height; j++ )
    {
      for ( i = 0; i < img_width; i++ )
      {
#if EXTEND_BD
        if (input->output_bit_depth == 8)
        {
          offset_units = j * img_width + i;
          imgYRef[j][i] = buf[2 * offset_units + 1];
          imgYRef[j][i] &= 0xff;
          if ( offset_units % 2 == 0 ) //U component
          {
            imgUVRef[0][j][i / 2] = buf[2 * offset_units];
          }
          else                  //V component
          {
            imgUVRef[1][j][i / 2] = buf[2 * offset_units];
          }
        }
        else if (input->output_bit_depth == 10)
        {
          offset_units = j * img_width + i;
          imgYRef[j][i] = ((byte *)buf)[2 * offset_units + 1];
          imgYRef[j][i] = imgYRef[j][i] & 0x3ff;
          if ( offset_units % 2 == 0 ) //U component
          {
            imgUVRef[0][j][i / 2] = ((byte *)buf)[2 * offset_units];
            imgUVRef[0][j][i / 2] &= 0x3ff;
          }
          else                  //V component
          {
            imgUVRef[1][j][i / 2] = ((byte *)buf)[2 * offset_units];
            imgUVRef[1][j][i / 2] &= 0x3ff;
          }
        }
        else if (input->output_bit_depth == 12)
        {
          offset_units = j * img_width + i;
          imgYRef[j][i] = ((byte *)buf)[2 * offset_units + 1];
          imgYRef[j][i] = imgYRef[j][i] & 0xfff;
          if ( offset_units % 2 == 0 ) //U component
          {
            imgUVRef[0][j][i / 2] = ((byte *)buf)[2 * offset_units];
            imgUVRef[0][j][i / 2] &= 0xfff;
          }
          else                  //V component
          {
            imgUVRef[1][j][i / 2] = ((byte *)buf)[2 * offset_units];
            imgUVRef[1][j][i / 2] &= 0xfff;
          }
        }

#else
        offset_units = j * img_width + i;
        imgYRef[j][i] = buf[2 * offset_units + 1];

        if ( offset_units % 2 == 0 ) //U component
        {
          imgUVRef[0][j][i / 2] = buf[2 * offset_units];
        }
        else                  //V component
        {
          imgUVRef[1][j][i / 2] = buf[2 * offset_units];
        }
#endif
      }
    }
  }

#else
#if EXTEND_BD
  if ( !input->yuv_structure )
  {
    for ( j = 0; j < img_height; j++ )
    {
      for ( i = 0; i < img_width; i++ )
      {
        if (input->output_bit_depth == 8)
        {
          chTemp[0] = fgetc( p_ref );
          imgYRef[j][i] = chTemp[0];
        }
        else if (input->output_bit_depth == 10)
        {
          chTemp[0] = fgetc( p_ref );
          chTemp[1] = fgetc( p_ref );
          imgYRef[j][i] = ((byte *)(&(chTemp[0])))[0] & 0x3ff;
        }
        else if (input->output_bit_depth == 12)
        {
          chTemp[0] = fgetc( p_ref );
          chTemp[1] = fgetc( p_ref );
          imgYRef[j][i] = ((byte *)(&(chTemp[0])))[0] & 0xfff;
        }
      }
    }
  }
  else
  {
    for ( j = 0; j < img_height; j++ )
    {
      for ( i = 0; i < img_width; i++ )
      {
        if (input->output_bit_depth == 8)
        {
          offset_units = j * img_width + i;
          if ( offset_units % 2 == 0 ) //U component
          {
            imgUVRef[0][j][i / 2] = fgetc ( p_ref );
          }
          else          //V component
          {
            imgUVRef[1][j][i / 2] = fgetc ( p_ref );
          }

          imgYRef[j][i] = fgetc ( p_ref );
        }
        else if (input->output_bit_depth == 10)
        {
          offset_units = j * img_width + i;
          if ( offset_units % 2 == 0 ) //U component
          {
            chTemp[0] = fgetc( p_ref );
            chTemp[1] = fgetc( p_ref );
            imgUVRef[0][j][i / 2] = ((byte *)(&(chTemp[0])))[0] & 0x3ff;
          }
          else          //V component
          {
            chTemp[0] = fgetc( p_ref );
            chTemp[1] = fgetc( p_ref );
            imgUVRef[1][j][i / 2] = ((byte *)(&(chTemp[0])))[0] & 0x3ff;
          }
          chTemp[0] = fgetc( p_ref );
          chTemp[1] = fgetc( p_ref );
          imgYRef[j][i] = ((byte *)(&(chTemp[0])))[0] & 0x3ff;
        }
        else if (input->output_bit_depth == 12)
        {
          offset_units = j * img_width + i;
          if ( offset_units % 2 == 0 ) //U component
          {
            chTemp[0] = fgetc( p_ref );
            chTemp[1] = fgetc( p_ref );
            imgUVRef[0][j][i / 2] = ((byte *)(&(chTemp[0])))[0] & 0xfff;
          }
          else          //V component
          {
            chTemp[0] = fgetc( p_ref );
            chTemp[1] = fgetc( p_ref );
            imgUVRef[1][j][i / 2] = ((byte *)(&(chTemp[0])))[0] & 0xfff;
          }
          chTemp[0] = fgetc( p_ref );
          chTemp[1] = fgetc( p_ref );
          imgYRef[j][i] = ((byte *)(&(chTemp[0])))[0] & 0xfff;
        }
      }
    }
  }
#else
  if ( !input->yuv_structure )
  {
    for ( j = 0; j < img_height; j++ )
    {
      for ( i = 0; i < img_width; i++ )
      {
        imgYRef[j][i] = fgetc ( p_ref );
      }
    }
  }
  else
  {
    for ( j = 0; j < img_height; j++ )
    {
      for ( i = 0; i < img_width; i++ )
      {
        offset_units = j * img_width + i;

        if ( offset_units % 2 == 0 ) //U component
        {
          imgUVRef[0][j][i / 2] = fgetc ( p_ref );
        }
        else          //V component
        {
          imgUVRef[1][j][i / 2] = fgetc ( p_ref );
        }

        imgYRef[j][i] = fgetc ( p_ref );
      }
    }
  }
#endif
#endif

  for ( uv = 0; uv < 2; uv++ )
  {
    if ( !input->yuv_structure )
    {
#ifdef WIN32
#if EXTEND_BD
      _read ( p_ref, buf, nOutputSampleSize * img_height_cr * img_width_cr );
#else
      _read ( p_ref, buf, img_height_cr * img_width_cr );
#endif

      for ( j = 0; j < img_height_cr; j++ )
      {
#if EXTEND_BD
        for ( i = 0; i < img_width_cr; i++)
        {
          if (input->output_bit_depth == 8)
          {            
            imgUVRef[uv][j][i] = buf[j * img_width_cr + i];
            imgUVRef[uv][j][i] &= 0xff;
          }
          else if (input->output_bit_depth == 10)
          {
            imgUVRef[uv][j][i] = ((byte *)buf)[j * img_width_cr + i] & 0x3ff;
          }
          else if (input->output_bit_depth == 12)
          {
            imgUVRef[uv][j][i] = ((byte *)buf)[j * img_width_cr + i] & 0xfff;
          }
        }
#else
        memcpy ( &imgUVRef[uv][j][0], buf + j * img_width_cr, img_width_cr );
#endif
      }

#else

      for ( j = 0; j < img_height_cr; j++ )
      {
        for ( i = 0; i < img_width_cr; i++ )
        {
#if EXTEND_BD
          if (input->output_bit_depth == 8)
          {
            imgUVRef[uv][j][i] = fgetc ( p_ref );
          }
          else if (input->output_bit_depth == 10)
          {
            chTemp[0] = fgetc ( p_ref );
            chTemp[1] = fgetc ( p_ref );
            imgUVRef[uv][j][i] = ((byte *)(&(chTemp[0])))[0] & 0x3ff;
          }
          else if (input->output_bit_depth == 12)
          {
            chTemp[0] = fgetc ( p_ref );
            chTemp[1] = fgetc ( p_ref );
            imgUVRef[uv][j][i] = ((byte *)(&(chTemp[0])))[0] & 0xfff;
          }
#else
          imgUVRef[uv][j][i] = fgetc ( p_ref );
#endif
        }
      }

#endif
    }
  }

  img->quad[0] = 0;
  diff_y = 0;

  for ( j = 0; j < img_height; ++j )
  {
    for ( i = 0; i < img_width; ++i )
    {
#if EXTEND_BD
      if (nBitDepthDiff == 0)
      {
        diff_y += img->quad[abs ( imgY[j][i] - imgYRef[j][i] ) ];
      }
      else if (nBitDepthDiff > 0)
      {
        diff_y += img->quad[abs ( Clip1((imgY[j][i] + (1 << (nBitDepthDiff - 1))) >> nBitDepthDiff)  - imgYRef[j][i] ) ];
      }

#else
      diff_y += img->quad[abs ( imgY[j][i] - imgYRef[j][i] ) ];
#endif
    }
  }

  // Chroma
  diff_u = 0;
  diff_v = 0;

  for ( j = 0; j < img_height_cr; ++j )
  {
    for ( i = 0; i < img_width_cr; ++i )
    {
#if EXTEND_BD
      if (nBitDepthDiff == 0)
      {
        diff_u += img->quad[abs ( imgUVRef[0][j][i] - imgUV[0][j][i] ) ];
        diff_v += img->quad[abs ( imgUVRef[1][j][i] - imgUV[1][j][i] ) ];
      }
      else if (nBitDepthDiff > 0)
      {
        diff_u += img->quad[abs ( imgUVRef[0][j][i] - Clip1((imgUV[0][j][i] + (1 << (nBitDepthDiff - 1))) >> nBitDepthDiff) ) ];
        diff_v += img->quad[abs ( imgUVRef[1][j][i] - Clip1((imgUV[1][j][i] + (1 << (nBitDepthDiff - 1))) >> nBitDepthDiff) ) ];
      }
#else
      diff_u += img->quad[abs ( imgUVRef[0][j][i] - imgUV[0][j][i] ) ];
      diff_v += img->quad[abs ( imgUVRef[1][j][i] - imgUV[1][j][i] ) ];
#endif
    }
  }

  // Collecting SNR statistics
  if ( diff_y != 0 )
  {
#if EXTEND_BD
    snr->snr_y = ( float ) ( 10 * log10 ( ((1 << input->output_bit_depth) - 1) * ((1 << input->output_bit_depth) - 1) * ( float ) ( img_width/*img->width*/ ) * ( img_height/*img->height*/ ) / ( float ) diff_y ) );  // luma snr for current frame
#else
    snr->snr_y = ( float ) ( 10 * log10 ( 65025 * ( float ) ( img_width/*img->width*/ ) * ( img_height/*img->height*/ ) / ( float ) diff_y ) );  // luma snr for current frame
#endif
  }

  if ( diff_u != 0 )
  {
#if EXTEND_BD
    snr->snr_u = ( float ) ( 10 * log10 ( ((1 << input->output_bit_depth) - 1) * ((1 << input->output_bit_depth) - 1) * ( float ) ( img_width/*img->width*/ ) * ( img_height/*img->height*/ ) / ( float ) ( /*4*/uvformat * diff_u ) ) );  //  chroma snr for current frame,422
#else
    snr->snr_u = ( float ) ( 10 * log10 ( 65025 * ( float ) ( img_width/*img->width*/ ) * ( img_height/*img->height*/ ) / ( float ) ( /*4*/uvformat * diff_u ) ) );  //  chroma snr for current frame,422
#endif
  }

  if ( diff_v != 0 )
  {
#if EXTEND_BD
    snr->snr_v = ( float ) ( 10 * log10 ( ((1 << input->output_bit_depth) - 1) * ((1 << input->output_bit_depth) - 1) * ( float ) ( img_width/*img->width*/ ) * ( img_height/*img->height*/ ) / ( float ) ( /*4*/uvformat * diff_v ) ) );  //  chroma snr for current frame,422
#else
    snr->snr_v = ( float ) ( 10 * log10 ( 65025 * ( float ) ( img_width/*img->width*/ ) * ( img_height/*img->height*/ ) / ( float ) ( /*4*/uvformat * diff_v ) ) );  //  chroma snr for current frame,422
#endif
  }

  if ( img->number == /*0*/1 ) // first
  {
#if EXTEND_BD
    snr->snr_y1 = ( float ) ( 10 * log10 ( ((1 << input->output_bit_depth) - 1) * ((1 << input->output_bit_depth) - 1) * ( float ) ( img_width/*img->width*/ ) * ( img_height/*img->height*/ ) / ( float ) diff_y ) ); // keep luma snr for first frame
    snr->snr_u1 = ( float ) ( 10 * log10 ( ((1 << input->output_bit_depth) - 1) * ((1 << input->output_bit_depth) - 1) * ( float ) ( img_width/*img->width*/ ) * ( img_height/*img->height*/ ) / ( float ) ( /*4*/uvformat * diff_u ) ) ); // keep chroma snr for first frame,422
    snr->snr_v1 = ( float ) ( 10 * log10 ( ((1 << input->output_bit_depth) - 1) * ((1 << input->output_bit_depth) - 1) * ( float ) ( img_width/*img->width*/ ) * ( img_height/*img->height*/ ) / ( float ) ( /*4*/uvformat * diff_v ) ) ); // keep chroma snr for first frame,422
#else
    snr->snr_y1 = ( float ) ( 10 * log10 ( 65025 * ( float ) ( img_width/*img->width*/ ) * ( img_height/*img->height*/ ) / ( float ) diff_y ) ); // keep luma snr for first frame
    snr->snr_u1 = ( float ) ( 10 * log10 ( 65025 * ( float ) ( img_width/*img->width*/ ) * ( img_height/*img->height*/ ) / ( float ) ( /*4*/uvformat * diff_u ) ) ); // keep chroma snr for first frame,422
    snr->snr_v1 = ( float ) ( 10 * log10 ( 65025 * ( float ) ( img_width/*img->width*/ ) * ( img_height/*img->height*/ ) / ( float ) ( /*4*/uvformat * diff_v ) ) ); // keep chroma snr for first frame,422
#endif
    snr->snr_ya = snr->snr_y1;
    snr->snr_ua = snr->snr_u1;
    snr->snr_va = snr->snr_v1;

    if ( diff_y == 0 )
    {
      snr->snr_ya =/*50*/0;
    }

    if ( diff_u == 0 )
    {
      snr->snr_ua =/*50*/0;
    }

    if ( diff_v == 0 )
    {
      snr->snr_va =/*50*/0;
    }
  }
  else
  {
    snr->snr_ya = ( float ) ( snr->snr_ya * ( img->number + Bframe_ctr - 1 ) + snr->snr_y ) / ( img->number + Bframe_ctr ); // average snr chroma for all frames 20080721
    snr->snr_ua = ( float ) ( snr->snr_ua * ( img->number + Bframe_ctr - 1 ) + snr->snr_u ) / ( img->number + Bframe_ctr ); // average snr luma for all frames 20080721
    snr->snr_va = ( float ) ( snr->snr_va * ( img->number + Bframe_ctr - 1 ) + snr->snr_v ) / ( img->number + Bframe_ctr ); // average snr luma for all frames 20080721
  }

  free ( buf );
}
#else
#ifdef AVS2_S2_BGLONGTERM
/*
*************************************************************************
* Function:Find PSNR for background frame.Compare decoded frame with
the original sequence. Read input->jumpd frames to reflect frame skipping.
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
#ifdef WIN32
void find_snr_background (SNRParameters *snr, int p_ref )
#else
void find_snr_background (SNRParameters *snr, FILE *p_ref )          //!< filestream to reference YUV file
#endif
{
  int i, j;
  int diff_y, diff_u, diff_v;
  int uv;
  int uvformat = input->chroma_format == 1 ? 4 : 2;
#ifdef WIN32
  __int64 status;
#else
  int  status;
#endif

  int img_width = ( img->width - img->auto_crop_right );
  int img_height = ( img->height - img->auto_crop_bottom );
  int img_width_cr = ( img_width / 2 );
  int img_height_cr = ( img_height / ( input->chroma_format == 1 ? 2 : 1 ) );
#ifdef WIN32
  const unsigned int  bytes_y = img_width * img_height;
  const unsigned int  bytes_uv = img_width_cr * img_height_cr;
  const __int64 framesize_in_bytes = ( __int64 ) ( bytes_y + 2 * bytes_uv );
#else
  const unsigned int  bytes_y = img_width * img_height;
  const unsigned int  bytes_uv = img_width_cr * img_height_cr;
  const int framesize_in_bytes = bytes_y + 2 * bytes_uv;
#endif
  unsigned char *buf;
  int   offset_units;

  if ( !input->yuv_structure )
  {
    buf = malloc ( bytes_y );
  }
  else
  {
    buf = malloc ( bytes_y + 2 * bytes_uv );
  }

  if ( NULL == buf )
  {
    no_mem_exit ( "find_snr: buf" );
  }

  snr->snr_y = 0.0;
  snr->snr_u = 0.0;
  snr->snr_v = 0.0;

#ifndef WIN32
  rewind ( p_ref );
#endif

  if ( RefPicExist )
  {
#ifdef WIN32

    if ( !input->ref_pic_order ) //ref order
    {
     status = _lseeki64 ( p_ref, framesize_in_bytes * (background_number-1) , SEEK_SET );
    }
    else
    {
      status = _lseeki64 ( p_ref, framesize_in_bytes * (background_number-1), SEEK_SET );
    }

#else

    if ( !input->ref_pic_order ) //ref order
    {
      status = fseek ( p_ref, framesize_in_bytes * ( img->type == B_IMG ? img->tr : FrameNum ), 0 );
    }
    else
    {
      status = fseek ( p_ref, framesize_in_bytes * dec_ref_num, 0 );
    }

#endif

#ifdef WIN32

    if ( status == -1 )
#else
    if ( status != 0 )
#endif
    {
      snprintf ( errortext, ET_SIZE, "Error in seeking img->tr: %d", img->tr );
      RefPicExist = 0;
    }
  }

#ifdef WIN32

  if ( !input->yuv_structure )
  {
    _read ( p_ref, buf, bytes_y );
  }
  else
  {
    _read ( p_ref, buf, bytes_y + 2 * bytes_uv );
  }

  if ( !input->yuv_structure ) //YUV
  {
    for ( j = 0; j < img_height; j++ )
    {
      memcpy ( &imgYRef[j][0], buf + j * img_width, img_width );
    }
  }
  else  //U0Y0 V1Y1
  {
    for ( j = 0; j < img_height; j++ )
    {
      for ( i = 0; i < img_width; i++ )
      {
        offset_units = j * img_width + i;
        imgYRef[j][i] = buf[2 * offset_units + 1];

        if ( offset_units % 2 == 0 ) //U component
        {
          imgUVRef[0][j][i / 2] = buf[2 * offset_units];
        }
        else                  //V component
        {
          imgUVRef[1][j][i / 2] = buf[2 * offset_units];
        }
      }
    }
  }

#else

  if ( !input->yuv_structure )
  {
    for ( j = 0; j < img_height; j++ )
    {
      for ( i = 0; i < img_width; i++ )
      {
        imgYRef[j][i] = fgetc ( p_ref );
      }
    }
  }
  else
  {
    for ( j = 0; j < img_height; j++ )
    {
      for ( i = 0; i < img_width; i++ )
      {
        offset_units = j * img_width + i;

        if ( offset_units % 2 == 0 ) //U component
        {
          imgUVRef[0][j][i / 2] = fgetc ( p_ref );
        }
        else          //V component
        {
          imgUVRef[1][j][i / 2] = fgetc ( p_ref );
        }

        imgYRef[j][i] = fgetc ( p_ref );
      }
    }
  }

#endif

  for ( uv = 0; uv < 2; uv++ )
  {
    if ( !input->yuv_structure )
    {
#ifdef WIN32
      _read ( p_ref, buf, img_height_cr * img_width_cr );

      for ( j = 0; j < img_height_cr; j++ )
      {
        memcpy ( &imgUVRef[uv][j][0], buf + j * img_width_cr, img_width_cr );
      }

#else

      for ( j = 0; j < img_height_cr; j++ )
      {
        for ( i = 0; i < img_width_cr; i++ )
        {
          imgUVRef[uv][j][i] = fgetc ( p_ref );
        }
      }

#endif
    }
  }

  img->quad[0] = 0;
  diff_y = 0;

  for ( j = 0; j < img_height; ++j )
  {
    for ( i = 0; i < img_width; ++i )
    {
		diff_y += img->quad[abs ( imgY[j][i] - imgYRef[j][i] ) ];
	}
  }

  // Chroma
  diff_u = 0;
  diff_v = 0;

  for ( j = 0; j < img_height_cr; ++j )
  {
	  for ( i = 0; i < img_width_cr; ++i )
	  {
		  diff_u += img->quad[abs ( imgUVRef[0][j][i] - imgUV[0][j][i] ) ];
		  diff_v += img->quad[abs ( imgUVRef[1][j][i] - imgUV[1][j][i] ) ];
	  }
  }

  // Collecting SNR statistics
  if ( diff_y != 0 )
  {
	  snr->snr_y = ( float ) ( 10 * log10 ( 65025 * ( float ) ( img_width/*img->width*/ ) * ( img_height/*img->height*/ ) / ( float ) diff_y ) );  // luma snr for current frame
  }

  if ( diff_u != 0 )
  {
	  snr->snr_u = ( float ) ( 10 * log10 ( 65025 * ( float ) ( img_width/*img->width*/ ) * ( img_height/*img->height*/ ) / ( float ) ( /*4*/uvformat * diff_u ) ) );  //  chroma snr for current frame,422
  }

  if ( diff_v != 0 )
  {
	  snr->snr_v = ( float ) ( 10 * log10 ( 65025 * ( float ) ( img_width/*img->width*/ ) * ( img_height/*img->height*/ ) / ( float ) ( /*4*/uvformat * diff_v ) ) );  //  chroma snr for current frame,422
  }

  if ( img->number == /*0*/1 ) // first
  {
	  snr->snr_y1 = ( float ) ( 10 * log10 ( 65025 * ( float ) ( img_width/*img->width*/ ) * ( img_height/*img->height*/ ) / ( float ) diff_y ) ); // keep luma snr for first frame
	  snr->snr_u1 = ( float ) ( 10 * log10 ( 65025 * ( float ) ( img_width/*img->width*/ ) * ( img_height/*img->height*/ ) / ( float ) ( /*4*/uvformat * diff_u ) ) ); // keep chroma snr for first frame,422
	  snr->snr_v1 = ( float ) ( 10 * log10 ( 65025 * ( float ) ( img_width/*img->width*/ ) * ( img_height/*img->height*/ ) / ( float ) ( /*4*/uvformat * diff_v ) ) ); // keep chroma snr for first frame,422
	  snr->snr_ya = snr->snr_y1;
	  snr->snr_ua = snr->snr_u1;
	  snr->snr_va = snr->snr_v1;

	  if ( diff_y == 0 )
	  {
		  snr->snr_ya =/*50*/0;
	  }

	  if ( diff_u == 0 )
	  {
		  snr->snr_ua =/*50*/0;
	  }

	  if ( diff_v == 0 )
	  {
		  snr->snr_va =/*50*/0;
	  }
  }
  else
  {
	  snr->snr_ya = ( float ) ( snr->snr_ya * ( img->number + Bframe_ctr - 1 ) + snr->snr_y ) / ( img->number + Bframe_ctr ); // average snr chroma for all frames 20080721
	  snr->snr_ua = ( float ) ( snr->snr_ua * ( img->number + Bframe_ctr - 1 ) + snr->snr_u ) / ( img->number + Bframe_ctr ); // average snr luma for all frames 20080721
	  snr->snr_va = ( float ) ( snr->snr_va * ( img->number + Bframe_ctr - 1 ) + snr->snr_v ) / ( img->number + Bframe_ctr ); // average snr luma for all frames 20080721
  }

  free ( buf );
}
#endif //AVS2_S2_BGLONGTERM
#endif //AVS2_SCENE_CD

#if M3480_TEMPORAL_SCALABLE
void cleanRefMVBufRef( int pos )
{
	int i, k, x, y;
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
	int i, j, tmp_coi, tmp_poc ,tmp_refered, tmp_temporal, tmp_is_output;
#if EXTEND_BD
	byte ***tmp_yuv;
#else
	unsigned char ***tmp_yuv;
#endif
	int **tmp_ref;
	int ***tmp_mv;
	int flag=0;
	int tmp_ref_poc[4];

	double *tmp_saorate;

#if cms_dpb
	printf("curr_IDRtr=%d\n",curr_IDRtr);
	printf("next_IDRtr=%d\n",next_IDRtr);
#endif	

	//////////////////////////////////////////////////////////////////////////
	//update IDR frame
	if (img->tr>next_IDRtr&&curr_IDRtr!=next_IDRtr)
	{
		curr_IDRtr = next_IDRtr;
		curr_IDRcoi = next_IDRcoi;
	}
	//////////////////////////////////////////////////////////////////////////
	// re-order the ref buffer according to RPS
	img->num_of_references = curr_RPS.num_of_ref;
	
#if cms_dpb
	int dpb_i=0;
	for ( dpb_i=0; dpb_i<REF_MAXBUFFER; dpb_i++ )
	{
		printf("imgcoi_ref[%d]=%d\n",dpb_i,img->imgcoi_ref[dpb_i]);
	}
  printf("re-order DPB: num_of_ref=%d\n",curr_RPS.num_of_ref);
#endif

	for ( i=0; i<curr_RPS.num_of_ref; i++ )
	{
#if cms_dpb
    printf("i=%d\n",i);
#endif	
		int accumulate = 0;
		tmp_yuv = ref[i];
		tmp_coi = img->imgcoi_ref[i];
#if M3480_TEMPORAL_SCALABLE
		tmp_temporal = img->temporal_id[i];
#endif
		tmp_is_output = img->is_output[i];
		tmp_poc = img->imgtr_fwRefDistance[i];
		tmp_refered = img->refered_by_others[i];
		tmp_ref = refbuf[i];
		tmp_mv = mvbuf[i];
		tmp_saorate = saorate[i];

		memcpy(tmp_ref_poc,ref_poc[i],4*sizeof(int));
		for ( j=i; j<REF_MAXBUFFER; j++)///////////////to be modified  IDR
		{
			int k ,tmp_tr;
			for (k=0;k<REF_MAXBUFFER;k++)
			{
				if (((int)img->coding_order - (int)curr_RPS.ref_pic[i]) == img->imgcoi_ref[k] && img->imgcoi_ref[k]>=-256 )
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
				curr_RPS.ref_pic[i] = img->coding_order - curr_IDRcoi;

				for ( k=0;k<i;k++)
				{
					if (curr_RPS.ref_pic[k]==curr_RPS.ref_pic[i])
					{
						accumulate++;
						break;
					}
				}
			}
			if ( img->imgcoi_ref[j] == img->coding_order - curr_RPS .ref_pic[i])
			{
				break;
			}
		}
		
		if (j==REF_MAXBUFFER || accumulate)
		{
			img->num_of_references--;
		}
		
#if cms_dpb
    printf("j=%d\n",j);
#endif		
		if ( j != REF_MAXBUFFER )
		{
			ref[i] = ref[j];
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
			refbuf[j] = tmp_ref;
			mvbuf[j] = tmp_mv;
			img->imgcoi_ref[j] = tmp_coi;
#if M3480_TEMPORAL_SCALABLE
			img->temporal_id[j] = tmp_temporal;
#endif
			img->is_output[j] = tmp_is_output;
			img->imgtr_fwRefDistance[j] = tmp_poc;
			img->refered_by_others[j] = tmp_refered;
			memcpy(ref_poc[j],tmp_ref_poc,4*sizeof(int));
		}
	}

#if EXTRACT_DPB_ColMV_B
	printf("prepare_RefInfo() refPicTypebuf[%d] =%d \n",img->tr,img->type );
	refPicTypebuf[img->tr] =img->type ;//cms DPB 存放参考帧的图像类型，用来指示每个帧的类型，若是B帧，打印时域参考mv时，0改为1，1改为0，-1不变
#endif
	if (img->type==B_IMG&&(img->imgtr_fwRefDistance[0]<=img->tr||img->imgtr_fwRefDistance[1]>=img->tr))
	{

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
			tmp_coi = img->imgcoi_ref[i];
#if M3480_TEMPORAL_SCALABLE
			tmp_temporal = img->temporal_id[i];
#endif
			tmp_is_output = img->is_output[i];
			tmp_poc = img->imgtr_fwRefDistance[i];
			tmp_refered = img->refered_by_others[i];
			tmp_ref = refbuf[i];
			tmp_mv = mvbuf[i];
			tmp_saorate = saorate[i];
			memcpy(tmp_ref_poc,ref_poc[i],4*sizeof(int));
			ref[i] = ref[j];
			integerRefY[i] =integerRefY[j];
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
			refbuf[j] = tmp_ref;
			mvbuf[j] = tmp_mv;
			img->imgcoi_ref[j] = tmp_coi;
#if M3480_TEMPORAL_SCALABLE
			img->temporal_id[j] = tmp_temporal;
#endif
			img->is_output[j] = tmp_is_output;
			img->imgtr_fwRefDistance[j] = tmp_poc;
			img->refered_by_others[j] = tmp_refered;
			memcpy(ref_poc[j],tmp_ref_poc,4*sizeof(int));
		}
#endif
		//******************************************//	
	}

	
#if cms_dpb
		//int dpb_i=0;
		for ( dpb_i=0; dpb_i<REF_MAXBUFFER; dpb_i++ )
		{
			printf("imgcoi_ref[%d]=%d\n",dpb_i,img->imgcoi_ref[dpb_i]);
		}
#endif

	//////////////////////////////////////////////////////////////////////////
	// delete the frame that will never be used
	for ( i=0; i< curr_RPS.num_to_remove; i++)
	{
		for ( j=0; j<REF_MAXBUFFER; j++)
		{
			if ( img->imgcoi_ref[j]>=-256 && img->imgcoi_ref[j] == img->coding_order - curr_RPS.remove_pic[i])
			{
				break;
			}
		}
		if (j<REF_MAXBUFFER && j>=img->num_of_references)
		{
			img->imgcoi_ref[j] = -257;
#if M3480_TEMPORAL_SCALABLE
			img->temporal_id[j] = -1;
#endif
			if( img->is_output[j] == -1 )
				img->imgtr_fwRefDistance[j] = -256;
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
	
#if EXTRACT_DPB
     fprintf(p_pps,"add current frame to ref buffer: DPB idx=%d\n",i);
#endif
#if EXTRACT_DPB_ColMV_B
		img->refBufIdx = i;//cms 当前帧在refbuf中的索引
#endif

	currentFrame = ref[i];
	img->imgtr_fwRefDistance[i] = img->tr;
	img->imgcoi_ref[i] = img->coding_order;
#if M3480_TEMPORAL_SCALABLE
	img->temporal_id[i] = cur_layer;
#endif
	img->is_output[i] = 1;
	img->refered_by_others[i] = curr_RPS.referd_by_others;

	if (img->type != B_IMG)
	{
		for (j=0;j<img->num_of_references;j++)
		{
			ref_poc[i][j] = img->imgtr_fwRefDistance[j];
		}
#if M3480_TEMPORAL_SCALABLE
		for (j=img->num_of_references; j<4; j++)
		{
			ref_poc[i][j] = 0;
		}
#endif
	}
	else
	{
		ref_poc[i][0] = img->imgtr_fwRefDistance[1];
		ref_poc[i][1] = img->imgtr_fwRefDistance[0];
#if EXTRACT_DPB_POC_B
		printf("ref_poc[i][0] = img->imgtr_fwRefDistance[1] = %d  0-1\n",ref_poc[i][0]);
		printf("ref_poc[i][1] = img->imgtr_fwRefDistance[0] = %d	1-0\n",ref_poc[i][1]);
#endif
#if M3480_TEMPORAL_SCALABLE
		ref_poc[i][2] = 0;
		ref_poc[i][3] = 0;
#endif
	}
#if EXTRACT
#if EXTRACT_DPB_POC_B
	//fprintf(p_pps,"ref_poc[%d]:\t",i);
		if(img->type==B_IMG)
		{
			printf("B_IMG ref_poc[%d]: curr_RPS.num_of_ref=%d\n",i,curr_RPS.num_of_ref);
	    for (j=curr_RPS.num_of_ref-1; j>=0; j--)
	    {
		  	fprintf(p_pps,"%08X\n",ref_poc[i][j]);
				printf("ref_poc[i][%d]=%d refPicTypebuf[%d] =%d \n",j,ref_poc[i][j],ref_poc[i][j],refPicTypebuf[ref_poc[i][j]] );
	    }     
	    for (j=curr_RPS.num_of_ref; j<16; j++)
	    {
		  	fprintf(p_pps,"%08X\n",0);
	    }    
		}
		else
		{			
			for (j=0; j<curr_RPS.num_of_ref; j++)
			{
				fprintf(p_pps,"%08X\n",ref_poc[i][j]);
			} 		
			for (j=curr_RPS.num_of_ref; j<16; j++)
			{
				fprintf(p_pps,"%08X\n",0);
			}
		}
#else
	//fprintf(p_pps,"ref_poc[%d]:\t",i);
#endif		
#endif	



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

	//printf("%d %d %d %d %d %d %d %d %d\n",img->num_of_references,img->imgtr_fwRefDistance[0],img->imgtr_fwRefDistance[1],img->imgtr_fwRefDistance[2],img->imgtr_fwRefDistance[3],img->imgtr_fwRefDistance[4],img->imgtr_fwRefDistance[5],img->imgtr_fwRefDistance[6],img->imgtr_fwRefDistance[7]);

	//////////////////////////////////////////////////////////////////////////
	// updata ref pointer

	if ( img->type!=I_IMG )
	{
		for ( j = 0; j < 4; j++ ) //ref_index = 0
		{
			integerRefY[j] = ref[j][0];
		}
		for ( j = 0; j < 4; j++ ) //ref_index = 0
		{
			for ( i = 0; i < 2; i++ ) // chroma uv =0,1; 1,2 for referenceFrame
			{
				integerRefUV[j][i] = ref[j][i + 1];
			}
		}
		//forward/backward reference buffer
		f_ref[0] = ref[1]; //f_ref[ref_index][yuv][height(height/2)][width] ref_index=0 for B frame, ref_index = 0,1 for B field
		b_ref[0] = ref[0]; //b_ref[ref_index][yuv][height(height/2)][width] ref_index=0 for B frame, ref_index = 0,1 for B field
		for ( j = 0; j < 1; j++ ) //ref_index = 0 luma = 0
		{
			integerRefY_fref[j] = f_ref[j][0];
			integerRefY_bref[j] = b_ref[j][0];
		}
		//chroma for backward
		for ( j = 0; j < 1; j++ ) //ref_index = 0
		{
			for ( i = 0; i < 2; i++ ) // chroma uv =0,1; 1,2 for referenceFrame
			{
				integerRefUV_fref[j][i] = f_ref[j][i + 1];
				integerRefUV_bref[j][i] = b_ref[j][i + 1];
			}
		}
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
* Function:Interpolation of 1/4 subpixel
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
#if EXTEND_BD
void get_block ( int ref_frame, int x_pos, int y_pos, int step_h, int step_v, int block[MAX_CU_SIZE][MAX_CU_SIZE], byte **ref_pic 
#else
void get_block ( int ref_frame, int x_pos, int y_pos, int step_h, int step_v, int block[MAX_CU_SIZE][MAX_CU_SIZE], unsigned char **ref_pic 
#endif
#ifdef AVS2_S2_SBD
				,int ioff, int joff
#endif				
				)
{
#if EXTEND_BD
	int max_pel_value = (1 << input->sample_bit_depth) - 1;
	int shift1 = input->sample_bit_depth - 8;
	int shift2 = 14 - input->sample_bit_depth;
	int shift3 = 20 - input->sample_bit_depth;
#else
	int max_pel_value = 255;
#endif
  int dx, dy;
  int x, y;
  int i, j;
  int maxold_x, maxold_y;
  int result;
  int tmp_res[140][140];
#ifdef AVS2_S2_SBD
  int tmp_bgr[140][140];
#endif

  static const int COEF_8tap[3][8] =
  {
      { -1, 4, -10, 57, 19,  -7, 3, -1 },
      { -1, 4, -11, 40, 40, -11, 4, -1 },
      { -1, 3, -7, 19, 57,  -10, 4, -1 }
  };

  const int *COEF[3];
  byte **p_ref;


  COEF[0] = COEF_8tap[0];
  COEF[1] = COEF_8tap[1];
  COEF[2] = COEF_8tap[2];


  //    A  a  1  b  B
  //    c  d  e  f
  //    2  h  3  i
  //    j  k  l  m
  //    C           D

  dx = x_pos & 3;
  dy = y_pos & 3;
  x_pos = ( x_pos - dx ) / 4;
  y_pos = ( y_pos - dy ) / 4;
  maxold_x = img->width - 1;
  maxold_y = img->height - 1;

#ifdef AVS2_S2_BGLONGTERM
#if AVS2_SCENE_CD
  if (background_reference_enable && img->num_of_references >= 2 && ref_frame == img->num_of_references - 1 && (img->type == P_IMG || img->type == F_IMG) && img->typeb != BP_IMG)
#else
  if(input->profile_id == 0x50 && background_reference_enable && img->num_of_references>=2 && ref_frame ==  img->num_of_references-1 && img->type == P_IMG && img->typeb != BP_IMG)
#endif
  {
      p_ref = background_frame[0];
  }
#ifdef AVS2_S2_S
#if AVS2_SCENE_CD
  else if (img->typeb == BP_IMG)
#else
  else if (input->profile_id == 0x50 && img->typeb == BP_IMG)
#endif
  {
      p_ref = background_frame[0];
  }
#endif
  else
  {
      p_ref = ref_pic;
  }
#else
  p_ref = ref_pic;
#endif

  if (dx == 0 && dy == 0) {
      for (j = 0; j < step_v; j++) {
          for (i = 0; i < step_h; i++) {
              block[j][i] = p_ref[max(0, min(maxold_y, y_pos + j))][max(0, min(maxold_x, x_pos + i))];
          }
      }
  } else {
      if (dy == 0) {
          for (j = 0; j < step_v; j++) {
              for (i = 0; i < step_h; i++) {
                  int y = max(0, min(maxold_y, y_pos + j));
                  int x_base = x_pos + i;
                  for (result = 0, x = -3; x < 5; x++) {
                      result += p_ref[y][max(0, min(maxold_x, x_base + x))] * COEF[dx - 1][x + 3];
                  }
#if EXTEND_BD
                  block[j][i] = max(0, min(max_pel_value, (result + (1 << (shift1 + shift2 - 1))) >> (shift1 + shift2)));
#else
                  block[j][i] = max(0, min(max_pel_value, (result + 32) >> 6));
#endif
              }
          }
      } else if (dx == 0) {
          for (j = 0; j < step_v; j++) {
              for (i = 0; i < step_h; i++) {
                  int x = max(0, min(maxold_x, x_pos + i));
                  int y_base = y_pos + j;
                  for (result = 0, y = -3; y < 5; y++) {
                      result += p_ref[max(0, min(maxold_y, y_base + y))][x] * COEF[dy - 1][y + 3];
                  }
#if EXTEND_BD
                  block[j][i] = max(0, min(max_pel_value, (result + (1 << (shift1 + shift2 - 1))) >> (shift1 + shift2)));
#else
                  block[j][i] = max(0, min(max_pel_value, (result + 32) >> 6));
#endif
              }
          }
      } else  {
          for (j = -3; j < step_v + 4; j++) {
              for (i = 0; i < step_h; i++) {
                  int y = max(0, min(maxold_y, y_pos + j));
                  int x_base = x_pos + i;
                  for (result = 0, x = -3; x < 5; x++) {
                      result += p_ref[y][max(0, min(maxold_x, x_base + x))] * COEF[dx - 1][x + 3];
                  }
#if EXTEND_BD
                  tmp_res[j + 3][i] = shift1 ? (result + (1 << (shift1 - 1))) >> shift1 : result;
#else
                  tmp_res[j + 3][i] = result;
#endif
              }
          }

          for (j = 0; j < step_v; j++) {
              for (i = 0; i < step_h; i++) {
                  for (result = 0, y = -3; y < 5; y++) {
                      result += tmp_res[j + 3 + y][i] * COEF[dy - 1][y + 3];
                  }
#if EXTEND_BD
                  block[j][i] = max(0, min(max_pel_value, (result + (1 << (shift3 - 1))) >> shift3));
#else
                  block[j][i] = max(0, min(max_pel_value, (result + 2048) >> 12));
#endif
              }
          }
      }
  }
}

/*
*************************************************************************
* Function:Reads the content of two successive startcode from the bitstream
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

int Header()
{
  unsigned char *Buf;
  int startcodepos, length;
  static unsigned long prev_pos = 0; // 08.16.2007
  int ext_ID;
  
#if EXTRACT
  int j=0;
#endif

  if ( ( Buf = ( char* ) calloc ( MAX_CODED_FRAME_SIZE , sizeof ( char ) ) ) == NULL )
  {
    no_mem_exit ( "GetAnnexbNALU: Buf" );
  }

  while ( 1 )
  {
    StartCodePosition = GetOneUnit ( Buf, &startcodepos, &length ); //jlzheng  7.5

    switch ( Buf[startcodepos] )
    {
    case SEQUENCE_HEADER_CODE:

      img->new_sequence_flag = 1;
      SequenceHeader ( Buf, startcodepos, length );

	    if (input->alf_enable && alfParAllcoated == 0)
	    {
		    CreateAlfGlobalBuffer();
		    alfParAllcoated = 1;
	    }

      img->seq_header_indicate = 1;
      break;
    case EXTENSION_START_CODE:
      extension_data ( Buf, startcodepos, length );
      break;
    case USER_DATA_START_CODE:
      user_data ( Buf, startcodepos, length );
      break;
    case VIDEO_EDIT_CODE:
      video_edit_code_data ( Buf, startcodepos, length );
      break;
    case I_PICTURE_START_CODE:
      I_Picture_Header ( Buf, startcodepos, length );
      calc_picture_distance ();

	    Read_ALF_param(Buf, startcodepos, length );	  

      if ( !img->seq_header_indicate )
      {
        img->B_discard_flag = 1;
        fprintf ( stdout, "    I   %3d\t\tDIDSCARD!!\n", img->tr );
        break;
      }
#if EXTRACT

#if EXTRACT_08X
		   fprintf(p_pps,"%08X\n",img->pic_alf_on[0]);
		   fprintf(p_pps,"%08X\n",img->pic_alf_on[1]);
		   fprintf(p_pps,"%08X\n",img->pic_alf_on[2]);
		   fprintf(p_pps,"%08X\n",0);//pictrue_coding_type 0:I  1:P 2:B 3:f 4:S 
		   fprintf(p_pps,"%08X\n",img->typeb);//scene_pred_flag
		   fprintf(p_pps,"%08X\n",fixed_picture_qp);
		   fprintf(p_pps,"%08X\n",picture_qp);//picture_qp
		   fprintf(p_pps,"%08X\n",0);//RefPicNum
		   fprintf(p_pps,"%08X\n",0);//scene_reference_enable PB图像才有的标志
		   fprintf(p_pps,"%08X\n",img->pic_distance);//pic_distance poc
#endif
		   
#if EXTRACT_D
		   fprintf(p_pps,"%d\n",img->pic_alf_on[0]);
		   fprintf(p_pps,"%d\n",img->pic_alf_on[1]);
		   fprintf(p_pps,"%d\n",img->pic_alf_on[2]);
		   fprintf(p_pps,"%d\n",0);//pictrue_coding_type 0:I  1:P 2:B 3:f 4:S 
		   fprintf(p_pps,"typeb:%d\n",img->typeb);//scene_pred_flag
		   fprintf(p_pps,"%d\n",fixed_picture_qp);
		   fprintf(p_pps,"picture_qp:%d\n",picture_qp);//picture_qp
		   fprintf(p_pps,"num_of_ref:%d\n",0);//RefPicNum
		   fprintf(p_pps,"%d\n",0);//scene_reference_enable PB图像才有的标志
		   fprintf(p_pps,"pic_distance:%d\n",img->pic_distance);//pic_distance poc
		
#endif   
/*
			//fprintf(p_pps,"num_of_ref:%d\n",curr_RPS.num_of_ref);//num_of_ref
			for ( j=0; j<curr_RPS.num_of_ref; j++)
			{
			  //curr_RPS.ref_pic[j] = u_v( 6, "delta COI of ref pic");
			  //fprintf(p_pps,"%d\n",curr_RPS.ref_pic[j]);//num_of_ref
			  fprintf(p_pps,"%d\n",img->ref ref_pic[j]);//num_of_ref
			}     
			for ( j=curr_RPS.num_of_ref; j<16; j++)
			{
			  fprintf(p_pps,"%d\n",0);//num_of_ref
			} 
			*/
		   //fclose(p_pps);
#endif

      break;
    case PB_PICTURE_START_CODE:
      PB_Picture_Header ( Buf, startcodepos, length );
      calc_picture_distance ();

	    Read_ALF_param(Buf, startcodepos, length );	 
#if EXTRACT

#if EXTRACT_08X
     fprintf(p_pps,"%08X\n",img->pic_alf_on[0]);
     fprintf(p_pps,"%08X\n",img->pic_alf_on[1]);
     fprintf(p_pps,"%08X\n",img->pic_alf_on[2]);
	 if( img->type == P_IMG )
     	fprintf(p_pps,"%08X\n",1);//pictrue_coding_type 0:I  1:P 2:B 3:f 4:S 
	 else if( img->type == B_IMG )
     	fprintf(p_pps,"%08X\n",2);//pictrue_coding_type 0:I  1:P 2:B 3:f 4:S 
	 else if( img->type == F_IMG )
     	fprintf(p_pps,"%08X\n",3);//pictrue_coding_type 0:I  1:P 2:B 3:f 4:S 
     else
     	fprintf(p_pps,"S Img\n");//pictrue_coding_type 0:I  1:P 2:B 3:f 4:S 
     fprintf(p_pps,"%08X\n",img->typeb);//scene_pred_flag
     fprintf(p_pps,"%08X\n",fixed_picture_qp);
     fprintf(p_pps,"%08X\n",picture_qp);//picture_qp
     fprintf(p_pps,"%08X\n",curr_RPS.num_of_ref);//RefPicNum
     fprintf(p_pps,"%08X\n",0);//scene_reference_enable PB图像才有的标志
     fprintf(p_pps,"%08X\n",img->pic_distance);//pic_distance poc	
#endif
	 
#if EXTRACT_D
     fprintf(p_pps,"%d\n",img->pic_alf_on[0]);
     fprintf(p_pps,"%d\n",img->pic_alf_on[1]);
     fprintf(p_pps,"%d\n",img->pic_alf_on[2]);
	 if( img->type == P_IMG )
     	fprintf(p_pps,"%d\n",1);//pictrue_coding_type 0:I  1:P 2:B 3:f 4:S 
	 else if( img->type == B_IMG )
     	fprintf(p_pps,"%d\n",2);//pictrue_coding_type 0:I  1:P 2:B 3:f 4:S 
	 else if( img->type == F_IMG )
     	fprintf(p_pps,"%d\n",3);//pictrue_coding_type 0:I  1:P 2:B 3:f 4:S 
     else
     	fprintf(p_pps,"S Img\n");//pictrue_coding_type 0:I  1:P 2:B 3:f 4:S 
     fprintf(p_pps,"%d\n",img->typeb);//scene_pred_flag
     fprintf(p_pps,"%d\n",fixed_picture_qp);
     fprintf(p_pps,"picture_qp:%d\n",picture_qp);//picture_qp
     fprintf(p_pps,"num_of_ref:%d\n",curr_RPS.num_of_ref);//RefPicNum
     fprintf(p_pps,"%d\n",0);//scene_reference_enable PB图像才有的标志
     fprintf(p_pps,"pic_distance:%d\n",img->pic_distance);//pic_distance poc	
#endif    

#if EXTRACT_DPB_RCS
   fprintf(p_pps,"num_of_ref:%d\n",curr_RPS.num_of_ref);//num_of_ref
	 for ( j=0; j<curr_RPS.num_of_ref; j++)
	 {
	   //curr_RPS.ref_pic[j] = u_v( 6, "delta COI of ref pic");
	   fprintf(p_pps,"rcs[%d] ref_pic=%d doi=%d\n",j,curr_RPS.ref_pic[j],img->coding_order-curr_RPS.ref_pic[j]);//num_of_ref
	 }  
	 for ( j=curr_RPS.num_of_ref; j<16; j++)
	 {
	   fprintf(p_pps,"%d\n",0);//num_of_ref
	 } 
#endif	 
	 //fclose(p_pps);
#endif		
      // xiaozhen zheng, 20071009
      if ( !img->seq_header_indicate )
      {
        img->B_discard_flag = 1;

        if ( img->type == P_IMG )
        {
          fprintf ( stdout, "    P   %3d\t\tDIDSCARD!!\n", img->tr );
        }
        if ( img->type == F_IMG )
        {
          fprintf ( stdout, "    F   %3d\t\tDIDSCARD!!\n", img->tr );
        }
        else
        {
          fprintf ( stdout, "    B   %3d\t\tDIDSCARD!!\n", img->tr );
        }

        break;
      }

      if ( img->seq_header_indicate == 1 && img->type != B_IMG )
      {
        img->B_discard_flag = 0;
      }
#if !M3475_RANDOM_ACCESS
      if ( img->type == B_IMG && img->B_discard_flag == 1 && !img->no_forward_reference )
#else 
      if ( img->type == B_IMG && img->B_discard_flag == 1 && !img->random_access_decodable_flag )
#endif 
      {
        fprintf ( stdout, "    B   %3d\t\tDIDSCARD!!\n", img->tr );
        break;
      }

      break;
    case SEQUENCE_END_CODE:
      img->new_sequence_flag = 1;
      img->sequence_end_flag = 1;
      free ( Buf );
      return EOS;
      break;
    default:

#if !M3475_RANDOM_ACCESS
      if ( ( Buf[startcodepos] >= SLICE_START_CODE_MIN && Buf[startcodepos] <= SLICE_START_CODE_MAX )
           && ( ( !img->seq_header_indicate ) || ( img->type == B_IMG && img->B_discard_flag == 1 && !img->no_forward_reference ) ) )
#else 
		if ( ( Buf[startcodepos] >= SLICE_START_CODE_MIN && Buf[startcodepos] <= SLICE_START_CODE_MAX )
			&& ( ( !img->seq_header_indicate ) || ( img->type == B_IMG && img->B_discard_flag == 1 && !img->random_access_decodable_flag ) ) )
#endif 
      {
        break;
      }
      else if ( Buf[startcodepos] >= SLICE_START_CODE_MIN )   // modified by jimmy 2009.04.01
      {
        if ( ( temp_slice_buf = ( char* ) calloc ( MAX_CODED_FRAME_SIZE , sizeof ( char ) ) ) == NULL ) // modified 08.16.2007
        {
          no_mem_exit ( "GetAnnexbNALU: Buf" );
        }

        first_slice_length = length;
        first_slice_startpos = startcodepos;
        memcpy ( temp_slice_buf, Buf, length );
        free ( Buf );
        return SOP;
      }
      else
      {
        printf ( "Can't find start code" );
        free ( Buf );
        return EOS;
      }
    }
  }

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


void init_frame ()
{
  static int first_P = TRUE;
  int i, j;

  for ( i = 0; i < img->width / ( MIN_BLOCK_SIZE ) + 1; i++ ) // set edge to -1, indicate nothing to predict from
  {
    img->ipredmode[0][i + 1] = -1;
    img->ipredmode[img->height / ( MIN_BLOCK_SIZE ) + 1][i + 1] = -1;
  }

  for ( j = 0; j < img->height / ( MIN_BLOCK_SIZE ) + 1; j++ )
  {
    img->ipredmode[j + 1][0] = -1;
    img->ipredmode[j + 1][img->width / ( MIN_BLOCK_SIZE ) + 1] = -1;
  }

  img->ipredmode    [0][0] = -1;

  for ( i = 0; i < img->max_mb_nr; i++ )
  {
    img->mb_data[i].slice_nr = -1;
  }

#ifdef AVS2_S2_BGLONGTERM
	if(img->typeb == BACKGROUND_IMG && background_picture_output_flag == 0)
		currentFrame = background_ref;
	else
#endif

	prepare_RefInfo();//cms 准备参考信息  rcs列表

  imgY =  currentFrame[0];
  imgUV =  &currentFrame[1];

  // Adaptive frequency weighting quantization  
#if FREQUENCY_WEIGHTING_QUANTIZATION
  if(weight_quant_enable_flag&&pic_weight_quant_enable_flag)
  {
	  InitFrameQuantParam();
	  FrameUpdateWQMatrix();		//M2148 2007-09
  }
#if  EXTRACT_FULL_PPS_debug  //2016-5-31 提取完整的参数
  fprintf(p_pps, "weight_quant_enable_flag %08X\n", weight_quant_enable_flag);
  fprintf(p_pps, "pic_weight_quant_enable_flag %08X\n", pic_weight_quant_enable_flag);
  fprintf(p_pps, "pps_loop_filter_disable %08X\n", pps_loop_filter_disable);
  fprintf(p_pps, "pps_AlphaCOffset %08X\n", pps_AlphaCOffset);
  fprintf(p_pps, "pps_BetaOffset %08X\n", pps_BetaOffset);
  fprintf(p_pps, "pps_chroma_quant_param_delta_cb %08X\n", pps_chroma_quant_param_delta_cb);
  fprintf(p_pps, "pps_chroma_quant_param_delta_cr %08X\n", pps_chroma_quant_param_delta_cr);
  if (weight_quant_enable_flag&&pic_weight_quant_enable_flag)
  {
    int r, c;
    for (r = 0; r < 4; r++)
    {
      for (c = 0; c < 4; c++)
      {
        fprintf(p_pps, "pps_WeightQuantMatrix4x4[%2d][%2d] %08X\n", r, c, pps_WeightQuantMatrix4x4[r][c]);
      }
    }
    for (r = 0; r < 8; r++)
    {
      for (c = 0; c < 8; c++)
      {
        fprintf(p_pps, "pps_WeightQuantMatrix8x8[%2d][%2d] %08X\n", r, c, pps_WeightQuantMatrix8x8[r][c]);
      }
    }
  }
  else
  {
    int r, c;
    for (r = 0; r < 4; r++)
    {
      for (c = 0; c < 4; c++)
      {
        fprintf(p_pps, "pps_WeightQuantMatrix4x4[%2d][%2d] %08X\n", r, c, 0);
      }
    }
    for (r = 0; r < 8; r++)
    {
      for (c = 0; c < 8; c++)
      {
        fprintf(p_pps, "pps_WeightQuantMatrix8x8[%2d][%2d] %08X\n", r, c,0);
      }
    }
  }
  //ALF
  if (1)
  {
    int r, c;
    fprintf(p_pps, "pps_alf_filter_num_minus1 %08X\n", pps_alf_filter_num_minus1);
    for (r = 0; r < 16; r++)
    {
      fprintf(p_pps, "alf_region_distance %08X\n", pps_alf_region_distance[r]);
      for (c = 0; c < 9; c++)
      {
        fprintf(p_pps, "pps_AlfCoeffLuma[%2d][%2d] %08X\n", r, c, pps_AlfCoeffLuma[r][c]);
      }
    }
    //AlfCoeffChroma 0
    for (c = 0; c < 9; c++)
    {
      fprintf(p_pps, "AlfCoeffChroma0[%2d] %08X\n",  c, pps_AlfCoeffChroma0[c]);
    }
    //AlfCoeffChroma 1
    for (c = 0; c < 9; c++)
    {
      fprintf(p_pps, "AlfCoeffChroma1[%2d] %08X\n", c, pps_AlfCoeffChroma1[c]);
    }
  }

  //ALF
  if (1)
  {
    int r, c;
    fprintf(p_pps, "pps_alf_filter_num_minus1 %08X\n", pps_alf_filter_num_minus1);
    for (r = 0; r < 16; r++)
    {
      fprintf(p_pps, "alf_region_distance %08X\n", pps_alf_region_distance[r]);
      for (c = 0; c < 9; c++)
      {
        fprintf(p_pps, "pps_AlfCoeffLuma[%2d][%2d] %08X\n", r, c, pps_AlfCoeffLuma_Final[r][c]);
      }
    }
    //AlfCoeffChroma 0
    for (c = 0; c < 9; c++)
    {
      fprintf(p_pps, "AlfCoeffChroma0[%2d] %08X\n", c, pps_AlfCoeffChroma0_Final[c]);
    }
    //AlfCoeffChroma 1
    for (c = 0; c < 9; c++)
    {
      fprintf(p_pps, "AlfCoeffChroma1[%2d] %08X\n", c, pps_AlfCoeffChroma1_Final[c]);
    }
  }
  fprintf(p_pps, "pps_bbv_delay %08X\n", pps_bbv_delay);
  fprintf(p_pps, "pps_time_code_flag %08X\n", pps_time_code_flag);
  fprintf(p_pps, "pps_time_code %08X\n", pps_time_code);
  fprintf(p_pps, "pps_decode_order_index %08X\n", pps_decode_order_index);
  fprintf(p_pps, "pps_temporal_id %08X\n", pps_temporal_id);
  fprintf(p_pps, "pps_picture_output_delay %08X\n", pps_picture_output_delay);

  fprintf(p_pps, "pps_random_access_decodable_flag %08X\n", pps_random_access_decodable_flag);
  fprintf(p_pps, "pps_bbv_check_times %08X\n", pps_bbv_check_times);
  fprintf(p_pps, "pps_progressive_frame %08X\n", pps_progressive_frame);

  fprintf(p_pps, "pps_top_field_first %08X\n", pps_top_field_first);
  fprintf(p_pps, "pps_repeat_first_field %08X\n", pps_repeat_first_field);

  fprintf(p_pps, "pps_use_rcs_flag %08X\n", pps_use_rcs_flag);
  fprintf(p_pps, "pps_rcs_index %08X\n", pps_rcs_index);
  if (1)
  {
    int idx, i;
    for (idx = 0; idx < MAXGOP; idx++)//MAXGOP=32
    {
      ref_man tmp_RPS = decod_RPS[idx];
      int refIdx;
      fprintf(p_pps, "RPS[%4d] referd_by_others %08X\n", idx, tmp_RPS.referd_by_others);
      fprintf(p_pps, "RPS[%4d] num_of_ref %08X\n", idx, tmp_RPS.num_of_ref);
      for (refIdx = 0; refIdx < MAXREF; refIdx++)//MAXREF=4
      {
        fprintf(p_pps, "RPS[%4d] delta_doi_of_reference_picture[%4d] %08X\n", idx, refIdx, tmp_RPS.ref_pic[refIdx]);
      }
      for (refIdx = MAXREF; refIdx < 7; refIdx++)//MAXREF=4
      {
        fprintf(p_pps, "RPS[%4d] delta_doi_of_reference_picture[%4d] %08X\n", idx, refIdx, 0);
      }
      fprintf(p_pps, "RPS[%4d] num_to_remove %08X\n", idx, tmp_RPS.num_to_remove);
      for (refIdx = 0; refIdx < MAXREF; refIdx++)//MAXREF=4
      {
        fprintf(p_pps, "RPS[%4d] delta_doi_of_removed_picture[%4d] %08X\n", idx, refIdx, tmp_RPS.remove_pic[refIdx]);
      }
      for (refIdx = MAXREF; refIdx < 7; refIdx++)//MAXREF=4
      {
        fprintf(p_pps, "RPS[%4d] delta_doi_of_removed_picture[%4d] %08X\n", idx, refIdx, 0);
      }
    }
  }

#endif
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
  fprintf(p_pps, "%08X\n", pps_loop_filter_disable);
  fprintf(p_pps, "%08X\n", pps_AlphaCOffset);
  fprintf(p_pps, "%08X\n", pps_BetaOffset);
  fprintf(p_pps, "%08X\n", pps_chroma_quant_param_delta_cb);
  fprintf(p_pps, "%08X\n", pps_chroma_quant_param_delta_cr);
  if (weight_quant_enable_flag&&pic_weight_quant_enable_flag)
  {
    int r, c;
    for (r = 0; r < 4; r++)
    {
      for (c = 0; c < 4; c++)
      {
        fprintf(p_pps, "%08X\n", pps_WeightQuantMatrix4x4[r][c]);
      }
    }
    for (r = 0; r < 8; r++)
    {
      for (c = 0; c < 8; c++)
      {
        fprintf(p_pps, "%08X\n", pps_WeightQuantMatrix8x8[r][c]);
      }
    }
  }
  else
  {
    int r, c;
    for (r = 0; r < 4; r++)
    {
      for (c = 0; c < 4; c++)
      {
        fprintf(p_pps, "%08X\n", 0);
      }
    }
    for (r = 0; r < 8; r++)
    {
      for (c = 0; c < 8; c++)
      {
        fprintf(p_pps, "%08X\n", 0);
      }
    }
  }
  //ALF
  if (1)
  {
    int r, c;
    fprintf(p_pps, "%08X\n", pps_alf_filter_num_minus1);
    for (r = 0; r < 16; r++)
    {
      fprintf(p_pps, "%08X\n", pps_alf_region_distance[r]);
      for (c = 0; c < 9; c++)
      {
        fprintf(p_pps, "%08X\n", pps_AlfCoeffLuma[r][c]);
      }
    }
    //AlfCoeffChroma 0
    for (c = 0; c < 9; c++)
    {
      fprintf(p_pps, "%08X\n", pps_AlfCoeffChroma0[c]);
    }
    //AlfCoeffChroma 1
    for (c = 0; c < 9; c++)
    {
      fprintf(p_pps, "%08X\n", pps_AlfCoeffChroma1[c]);
    }
  }
  fprintf(p_pps, "%08X\n", pps_bbv_delay);
  fprintf(p_pps, "%08X\n", pps_time_code_flag);
  fprintf(p_pps, "%08X\n", pps_time_code);
  fprintf(p_pps, "%08X\n", pps_decode_order_index);
  fprintf(p_pps, "%08X\n", pps_temporal_id);
  fprintf(p_pps, "%08X\n", pps_picture_output_delay);

  fprintf(p_pps, "%08X\n", pps_random_access_decodable_flag);
  fprintf(p_pps, "%08X\n", pps_bbv_check_times);
  fprintf(p_pps, "%08X\n", pps_progressive_frame);

  fprintf(p_pps, "%08X\n", pps_top_field_first);
  fprintf(p_pps, "%08X\n", pps_repeat_first_field);

  fprintf(p_pps, "%08X\n", pps_use_rcs_flag);
  fprintf(p_pps, "%08X\n", pps_rcs_index);
  if (1)
  {
    int idx, i;
    for (idx = 0; idx < MAXGOP; idx++)//MAXGOP=32
    {
      ref_man tmp_RPS = decod_RPS[idx];
      int refIdx;
      fprintf(p_pps, "%08X\n",tmp_RPS.referd_by_others);
      fprintf(p_pps, "%08X\n", tmp_RPS.num_of_ref);
      for (refIdx = 0; refIdx < MAXREF; refIdx++)//MAXREF=4
      {
        fprintf(p_pps, "%08X\n", tmp_RPS.ref_pic[refIdx]);
      }
      for (refIdx = MAXREF; refIdx < 7; refIdx++)//MAXREF=4
      {
        fprintf(p_pps, "%08X\n",  0);
      }
      fprintf(p_pps, "%08X\n" ,tmp_RPS.num_to_remove);
      for (refIdx = 0; refIdx < MAXREF; refIdx++)//MAXREF=4
      {
        fprintf(p_pps, "%08X\n",  tmp_RPS.remove_pic[refIdx]);
      }
      for (refIdx = MAXREF; refIdx < 7; refIdx++)//MAXREF=4
      {
        fprintf(p_pps, "%08X\n", 0);
      }
    }
  }


#endif
#if  EXTRACT_FULL_PPS_1  //2016-5-31 提取完整的参数
  if (1)
  {
    int i;
    int uiWQMSizeId, uiWMQId;
    int uiBlockSize;

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
        fprintf(p_pps, "cur_wq_matrix[%2d] %08X\n", i, cur_wq_matrix[uiWQMSizeId][i]);
      }
    }
  }
#endif
#endif

}

/*
*************************************************************************
* Function:decodes one picture
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void picture_data ()
{
  unsigned char *Buf;
  int startcodepos, length;
  const int mb_nr = img->current_mb_nr;
#if M3198_CU8
  int mb_width = img->width / MIN_CU_SIZE;
#else
  int mb_width = img->width / 16;
#endif
  int first_slice = 1; //08.16.2007 tianf
  int N8_SizeScale = 1 << ( input->g_uiMaxSizeInBit - MIN_CU_SIZE_IN_BIT );
  int j;
  int num_of_orgMB_in_row ;//4:1  5:2  6:4
  int num_of_orgMB_in_col ;
  int size = 1 << input->g_uiMaxSizeInBit;
  int pix_x = ( img->current_mb_nr % img->PicWidthInMbs ) << MIN_CU_SIZE_IN_BIT;
  int pix_y = ( img->current_mb_nr / img->PicWidthInMbs ) << MIN_CU_SIZE_IN_BIT;
  Slice *currSlice = img->currentSlice;      //lzhang for AEC
  int    ByteStartPosition;
  int    i;
  int    new_slice = 0;
  int    smb_index;
  Boolean currAlfEnable;
  DataPartition *dP;
  int compIdx,ctx_idx;
  int img_height,img_width,lcuHeight,lcuWidth,numLCUInPicWidth,numLCUInPicHeight,NumCUInFrame;
  Boolean   aec_mb_stuffing_bit;

#if EXTRACT_PIC_YUV_PRED   //2016-5-23 提取整帧图像的经过预测后的数据
  //int *pPicYPred;
  //int *pPicUVPred[2];
  if ((pPicYPred = (int*)malloc(sizeof(int)*img->width*img->height)) == NULL)
  {
    no_mem_exit("picture_data(): pPicYPred");  //
  }
  if ((pPicUVPred[0]= (int*)malloc(sizeof(int)*img->width*img->height)) == NULL)
  {
    no_mem_exit("picture_data(): pPicUVPred");  //
  }
  if ((pPicUVPred[1] = (int*)malloc(sizeof(int)*img->width*img->height)) == NULL)
  {
    no_mem_exit("picture_data(): pPicUVPred");  //
  }
#endif
  img->current_slice_nr = -1;                 // jlzheng 6.30
  currentbitoffset = currStream->frame_bitoffset;   // jlzheng 6.30
  currStream->frame_bitoffset = 0;                  // jlzheng 6.30

  if ( ( Buf = ( char* ) calloc ( MAX_CODED_FRAME_SIZE , sizeof ( char ) ) ) == NULL )
  {
    no_mem_exit ( "GetAnnexbNALU: Buf" );  //jlzheng  6.30
  }

  smb_index = 0;
  lcuHeight         = 1<<input->g_uiMaxSizeInBit;
  lcuWidth          = lcuHeight;
  img_height        = img->height;
  img_width         = img->width;
  numLCUInPicWidth  = img_width / lcuWidth ;
  numLCUInPicHeight = img_height / lcuHeight ;
  numLCUInPicWidth  += (img_width % lcuWidth)? 1 : 0;
  numLCUInPicHeight += (img_height % lcuHeight)? 1 : 0;
  NumCUInFrame = numLCUInPicHeight * numLCUInPicWidth;
  g_MaxSizeInbit = input->g_uiMaxSizeInBit;

#if EXTRACT_PIC_YUV_PRED   //2016-5-23 提取整帧图像的经过预测后的数据

  printf(" EXTRACT_PIC_YUV_PRED\n");
  sprintf(filename, "./%s/img/pred_pic_%04d.yuv", infile, img->coding_order);
  if ((p_pic_yuv_pred = fopen(filename, "wb")) == 0)
  {
    snprintf(errortext, ET_SIZE, "Error open img/pred_pic_%04d.txt!", img->coding_order);
    error(errortext, 500);
  }
#endif	
	//cms 循环整个帧的所有 LCU
  while ( img->current_mb_nr < img->PicSizeInMbs ) // loop over super codingUnits
  {
    //decode slice header   jlzheng 6.30
		if ( img->current_mb_nr < img->PicSizeInMbs ) // check every LCU
    {
      if ( first_slice )
      {
        SliceHeader ( temp_slice_buf, first_slice_startpos, first_slice_length );
        free ( temp_slice_buf );
        img->current_slice_nr++;
        first_slice = 0;
        new_slice = 1;
      }
      else
      {
        if ( checkstartcode() )
        {
          GetOneUnit ( Buf, &startcodepos, &length );
          StatBitsPtr->curr_frame_bits += length;
          SliceHeader ( Buf, startcodepos, length );
          img->current_slice_nr++;
          new_slice = 1;
        }
        else
        {
          new_slice = 0;
        }
      }

      if ( new_slice )
      {
#if EXTRACT
		    lcuNum=0;//新的slice，将LCU序号置0
	        //新建 slice 信息文件
		    sprintf(filename,"./%s/slice_info/slice_%04d_%04d.txt",infile,img->coding_order,sliceNum);
		    //printf("creat file: %s\n",filename);
        if (p_slice != NULL)
        {
          fclose(p_slice);
          p_slice = NULL;
        }
		    if((p_slice=fopen(filename,"w"))==0)
		    {
				  snprintf ( errortext, ET_SIZE, "Error open %s/slice_info/slice_%04d_%04d.txt",infile,img->coding_order,sliceNum);
				  error ( errortext, 500 );
		    } 
        else
        {
          printf("open %s/slice_info/slice_%04d_%04d.txt\n", infile, img->coding_order, sliceNum);
        }
#if EXTRACT_08X		
				fprintf(p_slice,"%08X\n",etr_slice_vertical_position);
				fprintf(p_slice,"%08X\n",etr_slice_vertical_position_ext);
				fprintf(p_slice,"%08X\n",etr_slice_horizontal_position);
				fprintf(p_slice,"%08X\n",etr_slice_horizontal_position_ext);
				fprintf(p_slice,"%08X\n",etr_fixed_slice_qp);
				fprintf(p_slice,"%08X\n",etr_slice_qp);
#endif
#if  EXTRACT_FULL_SLICE   //2016-5-31 提取完整的参数
        fprintf(p_slice, "%08X\n", slice_slice_sao_enable[0]);
        fprintf(p_slice, "%08X\n", slice_slice_sao_enable[1]);
        fprintf(p_slice, "%08X\n", slice_slice_sao_enable[2]);
        fprintf(p_slice, "slice_slice_sao_enable %08X\n", slice_slice_sao_enable[0]);
        fprintf(p_slice, "slice_slice_sao_enable %08X\n", slice_slice_sao_enable[1]);
        fprintf(p_slice, "slice_slice_sao_enable %08X\n", slice_slice_sao_enable[2]);
#endif
#if EXTRACT_D		
				fprintf(p_slice,"ver = %d\n",etr_slice_vertical_position);
				fprintf(p_slice,"ver_ext = %d\n",etr_slice_vertical_position_ext);
				fprintf(p_slice,"hor = %d\n",etr_slice_horizontal_position);
				fprintf(p_slice,"hor_ext = %d\n",etr_slice_horizontal_position_ext);
				fprintf(p_slice,"fixed_slice_qp = %d\n",etr_fixed_slice_qp);
				fprintf(p_slice,"slice_qp = %d\n",etr_slice_qp);
#endif
        if(p_slice != NULL)
        {
          fclose(p_slice);
          p_slice = NULL;
        }
#endif

        init_contexts ();
        AEC_new_slice();
        ByteStartPosition = ( currStream->frame_bitoffset ) / 8;


        for ( i = 0; i < 1; i++ )
        {
          img->currentSlice->partArr[i].readSyntaxElement = readSyntaxElement_AEC;
          img->currentSlice->partArr[i].bitstream = currStream ;
        }

        currStream = currSlice->partArr[0].bitstream;

        if ( ( currStream->frame_bitoffset ) % 8 != 0 )
        {
          ByteStartPosition++;
        }

        arideco_start_decoding ( &img->currentSlice->partArr[0].de_AEC, currStream->streamBuffer, ( ByteStartPosition ), & ( currStream->read_len ), img->type );
      }
    }  //decode slice header


    pix_x = ( img->current_mb_nr % img->PicWidthInMbs ) << MIN_CU_SIZE_IN_BIT;
    pix_y = ( img->current_mb_nr / img->PicWidthInMbs ) << MIN_CU_SIZE_IN_BIT;
    num_of_orgMB_in_col = N8_SizeScale;
    num_of_orgMB_in_row = N8_SizeScale;

    if ( pix_x + size >= img->width )
    {
      num_of_orgMB_in_col = ( img->width - pix_x ) >> MIN_CU_SIZE_IN_BIT;
    }

    if ( pix_y + size >= img->height )
    {
      num_of_orgMB_in_row = ( img->height - pix_y ) >> MIN_CU_SIZE_IN_BIT;
    }

    for ( i = 0; i < num_of_orgMB_in_row; i++ )
    {
      int pos = img->current_mb_nr + i * img->PicWidthInMbs;

      for ( j = 0; j < num_of_orgMB_in_col; j++, pos++ )
      {
        img->mb_data[pos].slice_nr = img->current_slice_nr;
      }
    }

#if TRACE
    fprintf ( p_trace, "\n*********** Pic: %i (I/P) MB: %i Slice: %i Type %d **********\n", img->tr, img->current_mb_nr, img->current_slice_nr, img->type );
#endif

    start_codingUnit ( input->g_uiMaxSizeInBit );

#if EXTRACT
		//新建 LCU信息文件
		sprintf(filename,"./%s/lcu_info/pic_%04d/lcu_%06d_%06d.txt",infile,img->coding_order,sliceNum,lcuNum);
		//printf("creat file: %s\n",filename);
    if (p_lcu != NULL)
    {
      fclose(p_lcu);
      p_lcu = NULL;
    }
	  if((p_lcu=fopen(filename,"w"))==0)
	  {
		  snprintf ( errortext, ET_SIZE, "Error open %s/lcu_info/pic_%04d/lcu_%06d_%06d.txt",infile,img->coding_order,sliceNum,lcuNum);
		  error ( errortext, 500 );
	  } 
	  //fprintf(p_lcu,"%d\n",lcuNum);
	  //printf("lcuNum %d\n",lcuNum);
	  //fprintf(p_lcu,"Img %s  numRef= %d\n",aPicTypeStr[img->type],img->num_of_references);
	  
	  //fprintf(p_lcu,"%d %d\n",pix_y,pix_x);//LCU 在图像中的坐标，光栅扫描，单位是像素
	  fprintf(p_lcu,"%08X\n",((pix_y>>4)<<16)+pix_x>>4);//LCU 在图像中的坐标，光栅扫描，单位是像素

#if EXTRACT_lcu_info_debug   //控制lcu debug信息是否打印
		//新建 LCU debug 信息文件
		sprintf(filename,"./%s/lcu_debug/pic_%04d/lcu_%06d_%06d.txt",infile,img->coding_order,sliceNum,lcuNum);
    if (p_lcu_debug != NULL)
    {
      fclose(p_lcu_debug);
      p_lcu_debug = NULL;
    }
		if((p_lcu_debug=fopen(filename,"w"))==0)
		{
			snprintf ( errortext, ET_SIZE, "Error open %s/lcu_debug/pic_%04d/lcu_%06d_%06d.txt",infile,img->coding_order,sliceNum,lcuNum);
			error ( errortext, 500 );
		} 
		
		fprintf(p_lcu_debug,"%08X\n",((pix_y>>4)<<16)+pix_x>>4);//LCU 在图像中的坐标，光栅扫描，单位是像素
    if (img->type==I_IMG)
    {
      fprintf(p_lcu_debug, "PicType %4d I_IMG\n", img->type);
    }
    else if (img->type == P_IMG)
    {
      fprintf(p_lcu_debug, "PicType %4d P_IMG\n", img->type);
    }
    else if (img->type == B_IMG)
    {
      fprintf(p_lcu_debug, "PicType %4d B_IMG\n", img->type);
    }
    else if (img->type == F_IMG)
    {
      fprintf(p_lcu_debug, "PicType %4d F_IMG\n", img->type);
    }
#endif


		//新建 LCU系数文件
		sprintf(filename,"./%s/lcu_coeff/pic_%04d/lcu_%06d_%06d.txt",infile,img->coding_order,sliceNum,lcuNum);
		//printf("creat file: %s\n",filename);
    if (p_lcu_coeff != NULL)
    {
      fclose(p_lcu_coeff);
      p_lcu_coeff = NULL;
    }
	  if((p_lcu_coeff=fopen(filename,"w"))==0)
	  {
		  snprintf ( errortext, ET_SIZE, "Error open %s/lcu_coeff/pic_%04d/lcu_%06d_%06d.txt",infile,img->coding_order,sliceNum,lcuNum);
		  error ( errortext, 500 );
	  } 
	  //fprintf(p_lcu,"lcuNum %d\n",lcuNum);
	  //fprintf(p_lcu_coeff,"Img %s  numRef= %d\n",aPicTypeStr[img->type],img->num_of_references);

		//新建 LCU MV 文件
		sprintf(filename,"./%s/lcu_mv/pic_%04d/lcu_%06d_%06d.txt",infile,img->coding_order,sliceNum,lcuNum);
		//printf("creat file: %s\n",filename);
    if (p_lcu_mv != NULL)
    {
      fclose(p_lcu_mv);
      p_lcu_mv = NULL;
    }
	  if((p_lcu_mv=fopen(filename,"w"))==0)
	  {
		  snprintf ( errortext, ET_SIZE, "Error open %s/lcu_mv/pic_%04d/lcu_%06d_%06d.txt",infile,img->coding_order,sliceNum,lcuNum);
		  error ( errortext, 500 );
	  } 
	  //fprintf(p_lcu_mv,"lcuNum=%d pix_y=%d pix_x=%d\n",lcuNum,pix_y,pix_x);
	  //fprintf(p_lcu_mv,"pix_y=%d pix_x=%d\n",pix_y,pix_x);
	  
	  EcuInfoInterMv[0]=((pix_y>>4)<<16)+((pix_x>>4));////{LCUaddr_y>>4, LCUaddr_x>>4}，各占16bits。 
	  lcuPosPixX=pix_x;
	  lcuPosPixY=pix_y;//LCU 在图像中的像素坐标	  
	  //fprintf(p_lcu_mv,"%08X\n",EcuInfoInterMv[0]);
	  //fprintf(p_lcu_coeff,"Img %s  numRef= %d\n",aPicTypeStr[img->type],img->num_of_references);
	  	
#endif
	
#if EXTRACT	
		for(i=0;i<12;i++)
		{	
			lcuSAOInfo[0][i]=0;	
			lcuSAOInfo[1][i]=0;	
			lcuSAOInfo[2][i]=0;	
		}
#endif
		 //[0-2]是3 个分量
		 //lcuSAOInfo[i][0] 是0=off   1=BO 2=Edge
		 //lcuSAOInfo[i][1-4] 是4个offset 有符号
		 //lcuSAOInfo[i][5] 是区间模式时起始
		 //lcuSAOInfo[i][6] 是区间模式时起始偏移 minus2
		 //lcuSAOInfo[i][7] 是边缘模式时的类型0=0度，1=90度，2=135度，3=45度
		 //lcuSAOInfo[i][8] 是merge FLAG,0 =noMerge,1=merge-left,2=merge-up

		if (input->sao_enable)
		{
#if SAO_CROSS_SLICE_FLAG
			sao_cross_slice = input->crossSliceLoopFilter;
#endif
			readParaSAO_one_SMB(smb_index, pix_y >> MIN_CU_SIZE_IN_BIT, pix_x >> MIN_CU_SIZE_IN_BIT,  num_of_orgMB_in_col, num_of_orgMB_in_row, img->slice_sao_on, img->saoBlkParams[smb_index], img->rec_saoBlkParams[smb_index]);	

#if EXTRACT
			//存放整个图像的SAO参数 SAOBlkParam** rec_saoBlkParams;//[SMB][comp]
			SAOBlkParam curSAO  = (img->rec_saoBlkParams[smb_index][0]);
			SAOBlkParam curSAOCb= (img->rec_saoBlkParams[smb_index][1]);
			SAOBlkParam curSAOCr= (img->rec_saoBlkParams[smb_index][2]);
			/*
			typedef struct	
			{
			int modeIdc; //NEW, MERGE, OFF
			int typeIdc; //NEW: EO_0, EO_90, EO_135, EO_45, BO. MERGE: left, above
			int startBand; //BO: starting band index
			int startBand2;
			int deltaband;
			int offset[MAX_NUM_SAO_CLASSES];
			}SAOBlkParam;
			modeIdc:  SAO_MODE_OFF = 0, SAO_MODE_MERGE, SAO_MODE_NEW,	NUM_SAO_MODES
			typeIdc:  SAO_TYPE_EO_0,	SAO_TYPE_EO_90, SAO_TYPE_EO_135,	SAO_TYPE_EO_45, SAO_TYPE_BO ,	NUM_SAO_NEW_TYPES
			*/
			if( lcuSAOInfo[0][8] == 0 )
			{
				fprintf(p_lcu,"%08X\n",lcuSAOInfo[0][0]);//8个 SAO 
				for(i=1;i<8;i++)
				  fprintf(p_lcu,"%08X\n",lcuSAOInfo[0][i]);//8个 SAO 
				  
				fprintf(p_lcu,"%08X\n",lcuSAOInfo[1][0]);//8个 SAO 
				for(i=1;i<8;i++)
				  fprintf(p_lcu,"%08X\n",lcuSAOInfo[1][i]);//8个 SAO 
					
				fprintf(p_lcu,"%08X\n",lcuSAOInfo[2][0]);//8个 SAO 
				for(i=1;i<8;i++)
				  fprintf(p_lcu,"%08X\n",lcuSAOInfo[2][i]);//8个 SAO 
			}
			else
			{
				fprintf(p_lcu,"%08X\n",(lcuSAOInfo[0][8]<<16));//8个 SAO 
				for(i=1;i<8;i++)
				  fprintf(p_lcu,"%08X\n",0);//8个 SAO 
				fprintf(p_lcu,"%08X\n",(lcuSAOInfo[1][8]<<16));//8个 SAO 
				for(i=1;i<8;i++)
				  fprintf(p_lcu,"%08X\n",0);//8个 SAO 
				fprintf(p_lcu,"%08X\n",(lcuSAOInfo[2][8]<<16));//8个 SAO 
				for(i=1;i<8;i++)
				  fprintf(p_lcu,"%08X\n",0);//8个 SAO 
			}
#if EXTRACT_lcu_info_debug
			  fprintf(p_lcu_debug,"Y mode %4d\n",lcuSAOInfo[0][0]);//8个 SAO
			  fprintf(p_lcu_debug,"Y SaoOffset 0 %4d\n",lcuSAOInfo[0][1]);//8个 SAO
			  fprintf(p_lcu_debug,"Y SaoOffset 1 %4d\n",lcuSAOInfo[0][2]);//8个 SAO
			  fprintf(p_lcu_debug,"Y SaoOffset 2 %4d\n",lcuSAOInfo[0][3]);//8个 SAO
			  fprintf(p_lcu_debug,"Y SaoOffset 3 %4d\n",lcuSAOInfo[0][4]);//8个 SAO
			  fprintf(p_lcu_debug,"Y SaoIntervalStartPos %4d\n",lcuSAOInfo[0][5]);//8个 SAO 
			  fprintf(p_lcu_debug,"Y SaoIntervalDeltaPosMinus2 %4d\n",lcuSAOInfo[0][6]);//8个 SAO
			  fprintf(p_lcu_debug,"Y SaoEdgeType %4d\n",lcuSAOInfo[0][7]);//8个 SAO

				
			  fprintf(p_lcu_debug,"U mode %4d\n",lcuSAOInfo[1][0]);//8个 SAO
			  fprintf(p_lcu_debug,"U SaoOffset 0 %4d\n",lcuSAOInfo[1][1]);//8个 SAO
			  fprintf(p_lcu_debug,"U SaoOffset 1 %4d\n",lcuSAOInfo[1][2]);//8个 SAO
			  fprintf(p_lcu_debug,"U SaoOffset 2 %4d\n",lcuSAOInfo[1][3]);//8个 SAO
			  fprintf(p_lcu_debug,"U SaoOffset 3 %4d\n",lcuSAOInfo[1][4]);//8个 SAO
			  fprintf(p_lcu_debug,"U SaoIntervalStartPos %4d\n",lcuSAOInfo[1][5]);//8个 SAO 
			  fprintf(p_lcu_debug,"U SaoIntervalDeltaPosMinus2 %4d\n",lcuSAOInfo[1][6]);//8个 SAO
			  fprintf(p_lcu_debug,"U SaoEdgeType %4d\n",lcuSAOInfo[1][7]);//8个 SAO

			  fprintf(p_lcu_debug,"V mode %4d\n",lcuSAOInfo[2][0]);//8个 SAO
			  fprintf(p_lcu_debug,"V SaoOffset 0 %4d\n",lcuSAOInfo[2][1]);//8个 SAO
			  fprintf(p_lcu_debug,"V SaoOffset 1 %4d\n",lcuSAOInfo[2][2]);//8个 SAO
			  fprintf(p_lcu_debug,"V SaoOffset 2 %4d\n",lcuSAOInfo[2][3]);//8个 SAO
			  fprintf(p_lcu_debug,"V SaoOffset 3 %4d\n",lcuSAOInfo[2][4]);//8个 SAO
			  fprintf(p_lcu_debug,"V SaoIntervalStartPos %4d\n",lcuSAOInfo[2][5]);//8个 SAO 
			  fprintf(p_lcu_debug,"V SaoIntervalDeltaPosMinus2 %4d\n",lcuSAOInfo[2][6]);//8个 SAO
			  fprintf(p_lcu_debug,"V SaoEdgeType %4d\n",lcuSAOInfo[2][7]);//8个 SAO

#endif
		    /*
			switch (curSAO.modeIdc)
			{
			case 0:
				  for(i=0;i<8;i++)
					  fprintf(p_lcu,"%08X\n",0);//SAO Off
			  break;
			default:
			  if(curSAO.typeIdc == SAO_TYPE_BO)
			  {
					  fprintf(p_lcu,"%08X\n",1);//SAO BO
					for(i=0;i<4;i++)
				  fprintf(p_lcu,"%08X\n",curSAO.sao_offset[i]);//4个 SAO Offset
				fprintf(p_lcu,"%08X\n",curSAO.startPos);//1个 SAO BO band start idx
				fprintf(p_lcu,"%08X\n",curSAO.startPosM2);//第2个 SAO BO band start idx的minus 2
					  fprintf(p_lcu,"%08X\n",0);//SAO EO 
			  }
			  else//EO
			  {
					  fprintf(p_lcu,"%08X\n",2);//SAO EO
					for(i=0;i<4;i++)
				  fprintf(p_lcu,"%08X\n",curSAO.sao_offset[i]);//4个 SAO Offset
				fprintf(p_lcu,"%08X\n",0);//1个 SAO BO band start idx
				fprintf(p_lcu,"%08X\n",0);//第2个 SAO BO band start idx的minus 2
				fprintf(p_lcu,"%08X\n",curSAO.typeIdc);//SAO EO 的方向 SAO_TYPE_EO_0, SAO_TYPE_EO_90, SAO_TYPE_EO_135,	SAO_TYPE_EO_45,
			  }
			  break;
			}
			
			curSAO= (img->rec_saoBlkParams[smb_index][1]);
			switch (curSAO.modeIdc)
			{
			case 0:
				  for(i=0;i<8;i++)
					  fprintf(p_lcu,"%08X\n",0);//SAO Off
			  break;
			default:
			  if(curSAO.typeIdc == SAO_TYPE_BO)
			  {
					  fprintf(p_lcu,"%08X\n",1);//SAO BO
					for(i=0;i<4;i++)
				  fprintf(p_lcu,"%08X\n",curSAO.sao_offset[i]);//4个 SAO Offset
				fprintf(p_lcu,"%08X\n",curSAO.startPos);//1个 SAO BO band start idx
				fprintf(p_lcu,"%08X\n",curSAO.startPosM2);//第2个 SAO BO band start idx的minus 2
					  fprintf(p_lcu,"%08X\n",0);//SAO EO 
			  }
			  else//EO
			  {
					  fprintf(p_lcu,"%08X\n",2);//SAO EO
					for(i=0;i<4;i++)
				  fprintf(p_lcu,"%08X\n",curSAO.sao_offset[i]);//4个 SAO Offset
				fprintf(p_lcu,"%08X\n",0);//1个 SAO BO band start idx
				fprintf(p_lcu,"%08X\n",0);//第2个 SAO BO band start idx的minus 2
				fprintf(p_lcu,"%08X\n",curSAO.typeIdc);//SAO EO 的方向 SAO_TYPE_EO_0, SAO_TYPE_EO_90, SAO_TYPE_EO_135,	SAO_TYPE_EO_45,
			  }
			  break;
			}
			curSAO= (img->rec_saoBlkParams[smb_index][2]);
			switch (curSAO.modeIdc)
			{
			case 0:
				  for(i=0;i<8;i++)
					  fprintf(p_lcu,"%08X\n",0);//SAO Off
			  break;
			default:
			  if(curSAO.typeIdc == SAO_TYPE_BO)
			  {
					  fprintf(p_lcu,"%08X\n",1);//SAO BO
					for(i=0;i<4;i++)
				  fprintf(p_lcu,"%08X\n",curSAO.sao_offset[i]);//4个 SAO Offset
				fprintf(p_lcu,"%08X\n",curSAO.startPos);//1个 SAO BO band start idx
				fprintf(p_lcu,"%08X\n",curSAO.startPosM2);//第2个 SAO BO band start idx的minus 2
					  fprintf(p_lcu,"%08X\n",0);//SAO EO 
			  }
			  else//EO
			  {
					  fprintf(p_lcu,"%08X\n",2);//SAO EO
					for(i=0;i<4;i++)
				  fprintf(p_lcu,"%08X\n",curSAO.sao_offset[i]);//4个 SAO Offset
				fprintf(p_lcu,"%08X\n",0);//1个 SAO BO band start idx
				fprintf(p_lcu,"%08X\n",0);//第2个 SAO BO band start idx的minus 2
				fprintf(p_lcu,"%08X\n",curSAO.typeIdc);//SAO EO 的方向 SAO_TYPE_EO_0, SAO_TYPE_EO_90, SAO_TYPE_EO_135,	SAO_TYPE_EO_45,
			  }
			  break;
			}
			*/
#endif

		}
#if EXTRACT
		else//SAO 不开启时，直接打印24个0，
		{
			for(i=0;i<24;i++)
				fprintf(p_lcu,"%08X\n",0);
		}
#endif	

		if (input->alf_enable)
		{
			dP = &(currSlice->partArr[0]);
			for (compIdx=0;compIdx<NUM_ALF_COMPONENT;compIdx++)
			{
				if (img->pic_alf_on[compIdx])
				{
#if !M3544_CONTEXT_OPT
					ctx_idx = getLCUCtrCtx_Idx(smb_index,numLCUInPicWidth,numLCUInPicHeight,NumCUInFrame,compIdx,Dec_ALF->m_AlfLCUEnabled);
#else 
	        ctx_idx =  0;
#endif
					Dec_ALF->m_AlfLCUEnabled[smb_index][compIdx] = readAlfLCUCtrl(img,&(dP->de_AEC),compIdx,ctx_idx);
				}
				else
				{
					Dec_ALF->m_AlfLCUEnabled[smb_index][compIdx] = FALSE;
				}
#if EXTRACT
#if EXTRACT_08X
			    //cms 打印ALF 开启标记
				fprintf(p_lcu,"%08X\n",Dec_ALF->m_AlfLCUEnabled[smb_index][compIdx]);
#endif
				
#if EXTRACT_D
			    //cms 打印ALF 开启标记
				fprintf(p_lcu,"ALF[%d] %d\n",compIdx,Dec_ALF->m_AlfLCUEnabled[smb_index][compIdx]);
#endif
#endif			

			}
			
		}
#if EXTRACT
		else //ALF 不开启时，直接打印3个0
		{
			for(i=0;i<3;i++)
				fprintf(p_lcu,"%08X\n",0);
		}
#endif	
		if (input->sao_enable || input->alf_enable)
		{
			smb_index++;
		}

		//进入CU 解码
		decode_SMB ( input->g_uiMaxSizeInBit, img->current_mb_nr );


  	if ( img->current_mb_nr % img->PicWidthInMbs + N8_SizeScale >= img->PicWidthInMbs )
	  {
	    if ( img->current_mb_nr / img->PicWidthInMbs + N8_SizeScale >= img->PicHeightInMbs )
	    {
	      img->current_mb_nr = img->max_mb_nr;
	    }
	    else
	    {
	      img->current_mb_nr = ( img->current_mb_nr / img->PicWidthInMbs + N8_SizeScale ) * img->PicWidthInMbs;
	    }
	  }
  	else
	  {
	    img->current_mb_nr = img->current_mb_nr + N8_SizeScale;
	  }


    aec_mb_stuffing_bit = AEC_startcode_follows ( 1 );
#if EXTRACT_lcu_info_cutype_aec_debug   
    if (p_lcu_debug != NULL)
    {
      fprintf(p_lcu_debug, "aec_mb_stuffing_bit %d end of slice \n", aec_mb_stuffing_bit);
    }
#endif	

#if EXTRACT
    if (p_lcu != NULL)
    {
      fclose(p_lcu);
      p_lcu = NULL;
    }
    if (p_lcu_coeff != NULL)
    {
      fclose(p_lcu_coeff);
      p_lcu_coeff = NULL;
    }
    if (p_lcu_mv != NULL)
    {
      fclose(p_lcu_mv);
      p_lcu_mv = NULL;
    }
#if EXTRACT_lcu_info_debug   //控制lcu debug信息是否打印
    if (p_lcu_debug != NULL)
    {
      fclose(p_lcu_debug);
      p_lcu_debug = NULL;
    }
#endif

    lcuNum++;
#endif	
  }

  free ( Buf );

#if EXTRACT_PIC_YUV_PRED   //2016-5-23 提取整帧图像的经过预测后的数据

  printf(" EXTRACT_PIC_YUV_PRED Copy_frame_for_file start\n");

  Copy_frame_for_file_pred(p_pic_yuv_pred, pPicYPred, pPicUVPred);
  if (p_pic_yuv_pred != NULL)
  {
    fflush(p_pic_yuv_pred);
    fclose(p_pic_yuv_pred);
    p_pic_yuv_pred = NULL;
  }
  if ( pPicYPred  != NULL)
  {
    free(pPicYPred);
  }
  if (pPicUVPred[0] != NULL)
  {
    free(pPicUVPred[0]);
  }
  if (pPicUVPred[1] != NULL)
  {
    free(pPicUVPred[1]);
  }
#endif

#if EXTRACT_PIC_YUV_TQ   //2016-5-29 提取整帧图像的经过预测+残差后的数据
  printf(" EXTRACT_PIC_YUV_TQ\n");
  sprintf(filename, "./%s/img/tq_pic_%04d.yuv", infile, img->coding_order);
  if ((p_pic_yuv_tq = fopen(filename, "wb")) == 0)
  {
    snprintf(errortext, ET_SIZE, "Error open img/tq_pic_%04d.txt!", img->coding_order);
    error(errortext, 500);
  }
  printf(" EXTRACT_PIC_YUV_TQ Copy_frame_for_file start\n");

  Copy_frame_for_file(p_pic_yuv_tq, imgY, imgUV);

  printf(" EXTRACT_PIC_YUV_TQ Copy_frame_for_file end\n");

  if (p_pic_yuv_tq != NULL)
  {
    fflush(p_pic_yuv_tq);
    fclose(p_pic_yuv_tq);
    p_pic_yuv_tq = NULL;
  }

#endif	
#if EXTRACT_PIC_DF_BS   ////2016-6-1 提取整帧的去块滤波Bs参数
  printf(" EXTRACT_PIC_DF_BS\n");
  sprintf(filename, "./%s/bsinfo/pic_%04d.txt", infile, img->coding_order);
  if ((p_pic_df_bs = fopen(filename, "w+")) == 0)
  {
    snprintf(errortext, ET_SIZE, "Error open bsinfo/pic_%04d.txt!", img->coding_order);
    error(errortext, 500);
  }
  //建立2个数组，分别存放垂直方向、水平方向的Bs值
  //垂直方向 picVerEdgeBsY[Height][Width / 8]; 每隔8个点有一个垂直边界
  //水平方向 picHorEdgeBsY[Height / 8][Width]; 每隔8个点有一个水平边界
  if (1)
  {
    int i, j;
    //Y
    get_mem2D(&picVerEdgeBsY, img->height+64, img->width / 8+8);
    get_mem2D(&picHorEdgeBsY, img->height / 8 + 8, img->width+64);
    printf(" get_mem2D end\n");
    for (i = 0; i < img->height+64; i++)
    {
      for (j = 0; j < img->width / 8+8; j++)
      {
        picVerEdgeBsY[i][j] = 0;
      }
    }
    for (i = 0; i < img->height / 8 + 8; i++)
    {
      for (j = 0; j < img->width + 64; j++)
      {
        picHorEdgeBsY[i][j] = 0;
      }
    }
    //Cb
    get_mem2D(&picVerEdgeBsCb, img->height + 64, img->width / 8 + 8);
    get_mem2D(&picHorEdgeBsCb, img->height / 8 + 8, img->width + 64);
    printf(" get_mem2D end\n");
    for (i = 0; i < img->height + 64; i++)
    {
      for (j = 0; j < img->width / 8 + 8; j++)
      {
        picVerEdgeBsCb[i][j] = 0;
      }
    }
    for (i = 0; i < img->height / 8 + 8; i++)
    {
      for (j = 0; j < img->width + 64; j++)
      {
        picHorEdgeBsCb[i][j] = 0;
      }
    }
    
    //Cr
    get_mem2D(&picVerEdgeBsCr, img->height + 64, img->width / 8 + 8);
    get_mem2D(&picHorEdgeBsCr, img->height / 8 + 8, img->width + 64);
    printf(" get_mem2D end\n");
    for (i = 0; i < img->height + 64; i++)
    {
      for (j = 0; j < img->width / 8 + 8; j++)
      {
        picVerEdgeBsCr[i][j] = 0;
      }
    }
    for (i = 0; i < img->height / 8 + 8; i++)
    {
      for (j = 0; j < img->width + 64; j++)
      {
        picHorEdgeBsCr[i][j] = 0;
      }
    }
  }
  printf(" get_mem2D init\n");

#endif
	//cms 进行当前帧的 去块滤波
  if ( !loop_filter_disable )
  {
	  CreateEdgeFilter();
	  SetEdgeFilter(); 
    DeblockFrame ( imgY, imgUV );//cms 直接在重建后的YUV上操作
  }

#if EXTRACT_PIC_DF_BS   ////2016-6-1 提取整帧的去块滤波Bs参数
  //建立2个数组，分别存放垂直方向、水平方向的Bs值
  //垂直方向 picVerEdgeBsY[Height][Width / 8]; 每隔8个点有一个垂直边界
  //水平方向 picHorEdgeBsY[Height / 8][Width]; 每隔8个点有一个水平边界
#if EXTRACT_PIC_DF_BS_Print_1  ////2016-6-1 提取整帧的去块滤波Bs参数
  if (1)
  {
    int i, j;
    int bsLCUW = (1 << input->g_uiMaxSizeInBit);//LCU pixel
    int bsWidthLCU =( (img->width + bsLCUW -1)/bsLCUW)*bsLCUW;
    int bsHeightLCU = ((img->height + bsLCUW - 1) / bsLCUW)*bsLCUW;
    //fprintf(p_pic_df_bs, " fprintf p_pic_df_bs Cr Ver bsHeightLCU=%d\n", bsHeightLCU);
    for (j = 0; j < img->width / 8; j++)
    {
      for (i = 0; i < bsHeightLCU; i++)
      {
        fprintf(p_pic_df_bs, "%08X\n", picVerEdgeBsY[i][j]);
      }
    }
    //fprintf(p_pic_df_bs, " fprintf p_pic_df_bs Cr Ver bsWidthLCU=%d\n", bsWidthLCU);
    for (i = 0; i < img->height / 8; i++)
    {
      for (j = 0; j < bsWidthLCU; j++)
      {
        fprintf(p_pic_df_bs, "%08X\n", picHorEdgeBsY[i][j]);
      }
    }
    //Cb
    //fprintf(p_pic_df_bs, " fprintf p_pic_df_bs Cr Ver \n");
    for (j = 0; j < img->width / 8; j += 2)
    {
      for (i = 0; i < bsHeightLCU / 2; i++)
      {
        fprintf(p_pic_df_bs, "%08X\n", picVerEdgeBsCb[i][j]);
      }
    }
    //fprintf(p_pic_df_bs, " fprintf p_pic_df_bs Cr Ver \n");
    for (i = 0; i < img->height / 8; i += 2)
    {
      for (j = 0; j < bsWidthLCU / 2; j++)
      {
        fprintf(p_pic_df_bs, "%08X\n", picHorEdgeBsCb[i][j]);
      }
    }
    //Cr
    //fprintf(p_pic_df_bs, " fprintf p_pic_df_bs Cr Ver \n");
    for (j = 0; j < img->width / 8; j += 2)
    {
      for (i = 0; i < bsHeightLCU / 2; i++)
      {
        fprintf(p_pic_df_bs, "%08X\n", picVerEdgeBsCr[i][j]);
      }
    }
    //fprintf(p_pic_df_bs, " fprintf p_pic_df_bs Cr Hor \n");
    for (i = 0; i < img->height / 8; i += 2)
    {
      for (j = 0; j < bsWidthLCU / 2; j++)
      {
        fprintf(p_pic_df_bs, "%08X\n", picHorEdgeBsCr[i][j]);
      }
    }
    //fprintf(p_pic_df_bs, " fprintf p_pic_df_bs Cr Ver \n");
#if EXTRACT_PIC_DF_BS_debug   ////2016-6-1 提取整帧的去块滤波Bs参数
    fprintf(p_pic_df_bs, "fprintf p_pic_df_bs \n");
    for (j = 0; j < img->width / 8; j++)
    {
      for (i = 0; i < bsHeightLCU; i++)
      {
        fprintf(p_pic_df_bs, "y,x=%4d %4d Bs=%d\n", i,j,picVerEdgeBsY[i][j]);
      }
    }
    for (i = 0; i < img->height/8; i++)
    {
      for (j = 0; j < bsWidthLCU; j++)
      {
        fprintf(p_pic_df_bs, "y,x=%4d %4d Bs=%d\n", i, j, picHorEdgeBsY[i][j]);
      }
    }

    //Cb
    for (j = 0; j < img->width / 8; j+=2)
    {
      for (i = 0; i < bsHeightLCU/2; i++)
      {
        fprintf(p_pic_df_bs, "Cb y,x=%4d %4d Bs=%d\n", i, j, picVerEdgeBsCb[i][j]);
      }
    }
    for (i = 0; i < img->height / 8; i+=2)
    {
      for (j = 0; j < bsWidthLCU/2; j++)
      {
        fprintf(p_pic_df_bs, "Cb y,x=%4d %4d Bs=%d\n", i, j, picHorEdgeBsCb[i][j]);
      }
    }


    fprintf(p_pic_df_bs, " fprintf p_pic_df_bs Y Ver \n");
    for (j = 0; j < img->width / 8; j++)
    {
      for (i = 0; i < bsHeightLCU; i++)
      {
        fprintf(p_pic_df_bs, "%d,", picVerEdgeBsY[i][j]);
      }
      fprintf(p_pic_df_bs, "\n");
    }
    fprintf(p_pic_df_bs, " fprintf p_pic_df_bs Y Hor \n");
    for (i = 0; i < img->height / 8; i++)
    {
      for (j = 0; j < bsWidthLCU; j++)
      {
        fprintf(p_pic_df_bs, "%d,", picHorEdgeBsY[i][j]);
      }
      fprintf(p_pic_df_bs, "\n");
    }
    //Cb
    fprintf(p_pic_df_bs, " fprintf p_pic_df_bs Cb Ver \n");
    for (j = 0; j < img->width / 8; j += 2)
    {
      for (i = 0; i < bsHeightLCU / 2; i++)
      {
        fprintf(p_pic_df_bs, "%d,",  picVerEdgeBsCb[i][j]);
      }
      fprintf(p_pic_df_bs, "\n");
    }
    fprintf(p_pic_df_bs, " fprintf p_pic_df_bs Cb Hor \n");
    for (i = 0; i < img->height / 8; i += 2)
    {
      for (j = 0; j < bsWidthLCU / 2; j++)
      {
        fprintf(p_pic_df_bs, "%d,",  picHorEdgeBsCb[i][j]);
      }
      fprintf(p_pic_df_bs, "\n");
    }
    //Cr
    fprintf(p_pic_df_bs, " fprintf p_pic_df_bs Cr Ver \n");
    for (j = 0; j < img->width / 8; j += 2)
    {
      for (i = 0; i < bsHeightLCU / 2; i++)
      {
        fprintf(p_pic_df_bs, "%d,", picVerEdgeBsCr[i][j]);
      }
      fprintf(p_pic_df_bs, "\n");
    }
    fprintf(p_pic_df_bs, " fprintf p_pic_df_bs Cr Hor \n");
    for (i = 0; i < img->height / 8; i += 2)
    {
      for (j = 0; j < bsWidthLCU / 2; j++)
      {
        fprintf(p_pic_df_bs, "%d,", picHorEdgeBsCr[i][j]);
      }
      fprintf(p_pic_df_bs, "\n");
    }
#endif
  }
#endif
#if EXTRACT_PIC_DF_BS_Print_2  ////2016-6-1 提取整帧的去块滤波Bs参数
  if (1)
  {
    int i, j;
    int bsLCUW = (1 << input->g_uiMaxSizeInBit);//LCU pixel
    int bsLCUB8_col_shift= ( input->g_uiMaxSizeInBit -3);//LCU pixel
    int bsMaxB8_row = (img->height /8);//
    int bsMaxB8_col = (img->width / 8);//
    int bsEdgeNum = (bsLCUW / 8);// LCU 的边界数 
    int bsLCU_row, bsLCU_col;
    int bsLCUB8_row, bsLCUB8_col;
    //从左往右 ，从上往下遍历LCU
    for (bsLCU_row = 0; bsLCU_row < (img->height+ bsLCUW-1)/ bsLCUW; bsLCU_row++)
    {
      for (bsLCU_col = 0; bsLCU_col < (img->width + bsLCUW - 1) / bsLCUW; bsLCU_col++)
      {
        bsLCUB8_col = (bsLCU_col << bsLCUB8_col_shift);
        bsLCUB8_row = (bsLCU_row << bsLCUB8_col_shift);
        //亮度Y
        for (i = bsLCUB8_col; i < bsLCUB8_col+ bsEdgeNum; i++)//bsEdgeNum 条垂直边界
        {
          for (j = bsLCUB8_row; j < (bsLCUB8_row+ bsLCUW); j++)//从上往下
          {
            fprintf(p_pic_df_bs, "y,x=%4d %4d Bs=%d\n", j, i, picVerEdgeBsY[j][i]);
          }
        }
        for (j = bsLCUB8_row; j < (bsLCUB8_row + 8); j++)//8条水平边界，从上往下
        {
          for (i = bsLCUB8_col; i < bsLCUB8_col + bsLCUW; i++)//从左往右
          {
            fprintf(p_pic_df_bs, "y,x=%4d %4d Bs=%d\n", j, i, picHorEdgeBsY[j][i]);
          }
        }
        //色度Cb
        for (i = bsLCUB8_col; i < bsLCUB8_col + 8; i=i+2)//4条垂直边界
        {
          for (j = bsLCUB8_row; j < (bsLCUB8_row + bsLCUW/2); j++)//从上往下，
          {
            fprintf(p_pic_df_bs, "y,x=%4d %4d Bs=%d\n", j, i, picVerEdgeBsCb[j][i]);
          }
        }
        for (j = bsLCUB8_row; j < (bsLCUB8_row + 8); i = i + 2)//4条水平边界，从上往下
        {
          for (i = bsLCUB8_col; i < bsLCUB8_col + bsLCUW/2; i++)//从左往右
          {
            fprintf(p_pic_df_bs, "y,x=%4d %4d Bs=%d\n", j, i, picHorEdgeBsCb[j][i]);
          }
        }
        //色度Cb
        for (i = bsLCUB8_col; i < bsLCUB8_col + 8; i = i + 2)//4条垂直边界
        {
          for (j = bsLCUB8_row; j < (bsLCUB8_row + bsLCUW / 2); j++)//从上往下，
          {
            fprintf(p_pic_df_bs, "y,x=%4d %4d Bs=%d\n", j, i, picVerEdgeBsCr[j][i]);
          }
        }
        for (j = bsLCUB8_row; j < (bsLCUB8_row + 8); i = i + 2)//4条水平边界，从上往下
        {
          for (i = bsLCUB8_col; i < bsLCUB8_col + bsLCUW / 2; i++)//从左往右
          {
            fprintf(p_pic_df_bs, "y,x=%4d %4d Bs=%d\n", j, i, picHorEdgeBsCr[j][i]);
          }
        }
      }
    }
    for (i = 0; i < img->height; i++)
    {
      for (j = 0; j < img->width / 8; j++)
      {
        fprintf(p_pic_df_bs, "y,x=%4d %4d Bs=%d\n", i, j, picVerEdgeBsY[i][j]);
      }
    }
    for (i = 0; i < img->height / 8; i++)
    {
      for (j = 0; j < img->width; j++)
      {
        fprintf(p_pic_df_bs, "y,x=%4d %4d Bs=%d\n", i, j, picHorEdgeBsY[i][j]);
      }
    }
  }
#endif
  if (p_pic_df_bs != NULL)
  {
    fflush(p_pic_df_bs);
    fclose(p_pic_df_bs);
    p_pic_df_bs = NULL;
  }
  //Y
  free_mem2D(picVerEdgeBsY);
  free_mem2D(picHorEdgeBsY);
  //Cb
  free_mem2D(picVerEdgeBsCb);
  free_mem2D(picHorEdgeBsCb);
  //Cr
  free_mem2D(picVerEdgeBsCr);
  free_mem2D(picHorEdgeBsCr);
#endif
#if EXTRACT_PIC_YUV_DF   //2016-5-23 提取整帧图像的经过预测+残差后的数据

  printf(" EXTRACT_PIC_YUV_DF\n");
  sprintf(filename, "./%s/img/df_pic_%04d.yuv", infile, img->coding_order);
  if ((p_pic_yuv_df = fopen(filename, "wb")) == 0)
  {
    snprintf(errortext, ET_SIZE, "Error open img/df_pic_%04d.txt!", img->coding_order);
    error(errortext, 500);
  }
  Copy_frame_for_file(p_pic_yuv_df, imgY, imgUV);

  if (p_pic_yuv_df != NULL)
  {
    fflush(p_pic_yuv_df);
    fclose(p_pic_yuv_df);
    p_pic_yuv_df = NULL;
  }
#endif	
	//cms 进行当前帧的 sao
#if EXTRACT_PIC_YUV_SAO   //2016-5-23 提取整帧图像的经过sao处理后的数据

  printf(" EXTRACT_PIC_YUV_SAO\n");
	sprintf(filename,"./%s/img/sao_pic_%04d.yuv",infile,img->coding_order);
	if( ( p_pic_yuv_sao=fopen(filename,"wb"))==0 )
	{
		 snprintf ( errortext, ET_SIZE, "Error open img/sao_pic_%04d.txt!",img->coding_order);
		 error ( errortext, 500 );
	}

#endif	

	printf(" input->sao_enable=%d\n",input->sao_enable);
  if (input->sao_enable)
  {
		printf(" if (input->sao_enable) %d\n",input->sao_enable);
	  Copy_frame_for_SAO();
#if EXTEND_BD
	  SAOFrame(input->g_uiMaxSizeInBit, input->slice_set_enable, img->rec_saoBlkParams, img->slice_sao_on, input->sample_bit_depth);
#else
    SAOFrame(input->g_uiMaxSizeInBit, input->slice_set_enable, img->rec_saoBlkParams, img->slice_sao_on);
#endif
	}

#if EXTRACT_PIC_YUV_SAO   //2016-5-23 提取整帧图像的经过sao处理后的数据
		if( p_pic_yuv_sao != NULL )
		{
			fflush(p_pic_yuv_sao);
			fclose(p_pic_yuv_sao);
			p_pic_yuv_sao=NULL;
		}
#endif	

	//cms 进行当前帧的 ALF
  if (input->alf_enable)
  {	 
	  currAlfEnable = !(Dec_ALF->m_alfPictureParam[ALF_Y]->alf_flag==0 && Dec_ALF->m_alfPictureParam[ALF_Cb]->alf_flag==0 && Dec_ALF->m_alfPictureParam[ALF_Cr]->alf_flag==0 );

	  if (currAlfEnable)
	  {
		  Copy_frame_for_ALF();
#if EXTEND_BD
      ALFProcess_dec(Dec_ALF->m_alfPictureParam,img,imgY_alf_Rec,imgUV_alf_Rec, input->sample_bit_depth);
#else
      ALFProcess_dec(Dec_ALF->m_alfPictureParam,img,imgY_alf_Rec,imgUV_alf_Rec);
#endif

	  }	
  }

#if EXTRACT_PIC_YUV_ALF   //2016-5-23 提取整帧图像的经过 ALF 后的数据

  printf(" EXTRACT_PIC_YUV_ALF\n");
  sprintf(filename, "./%s/img/alf_pic_%04d.yuv", infile, img->coding_order);
  if ((p_pic_yuv_alf = fopen(filename, "wb")) == 0)
  {
    snprintf(errortext, ET_SIZE, "Error open img/alf_pic_%04d.txt!", img->coding_order);
    error(errortext, 500);
    }
  Copy_frame_for_file(p_pic_yuv_alf, imgY, imgUV);

  if (p_pic_yuv_alf != NULL)
  {
    fflush(p_pic_yuv_alf);
    fclose(p_pic_yuv_alf);
    p_pic_yuv_alf = NULL;
  }
#endif	
#if EXTRACT_PIC_SAO   //2016-5-20 提取整帧的sao最后参数
	if(1)
	{
		int cur_mb_nr = 0;
		int lcuPosX,lcuPosY;
		printf("p_pic_sao img->PicSizeInMbs=%d\n",img->PicSizeInMbs );
		//cms 循环整个帧的所有 LCU
	  while ( cur_mb_nr < (img->picWidthInLCU*img->picHeightInLCU) ) // loop over super codingUnits
	  {	
	  	//存放整个图像的SAO参数 SAOBlkParam** rec_saoBlkParams;//[SMB][comp]
			SAOBlkParam curSAO  = (img->rec_saoBlkParams[cur_mb_nr][0]);
			SAOBlkParam curSAOCb= (img->rec_saoBlkParams[cur_mb_nr][1]);
			SAOBlkParam curSAOCr= (img->rec_saoBlkParams[cur_mb_nr][2]);	
#if EXTRACT_PIC_SAO_debug
			fprintf(p_pic_sao,"cur_mb_nr=%d\n",cur_mb_nr);//SAO BO
#endif
			lcuPosX = (cur_mb_nr%img->picWidthInLCU)*img->maxCUWidth;
			lcuPosY = (cur_mb_nr/img->picWidthInLCU)*img->maxCUWidth;			

			fprintf(p_pic_sao,"%08X\n",((lcuPosY>>4)<<16)+(lcuPosX>>4));//SAO BO

			switch (curSAO.modeIdc)
			{
				case 0:
					  for(i=0;i<8;i++)
						  fprintf(p_pic_sao,"%08X\n",0);//SAO Off
				  break;
				default:
				  if(curSAO.typeIdc == SAO_TYPE_BO)
				  {
						fprintf(p_pic_sao,"%08X\n",1);//SAO BO
						for(i=0;i<4;i++)
					  	fprintf(p_pic_sao,"%08X\n",curSAO.sao_offset[i]);//4个 SAO Offset
						fprintf(p_pic_sao,"%08X\n",curSAO.startPos);//1个 SAO BO band start idx
						fprintf(p_pic_sao,"%08X\n",curSAO.startPosM2);//第2个 SAO BO band start idx的minus 2
						fprintf(p_pic_sao,"%08X\n",0);//SAO EO 
				  }
				  else//EO
				  {
						fprintf(p_pic_sao,"%08X\n",2);//SAO EO
						for(i=0;i<4;i++)
							fprintf(p_pic_sao,"%08X\n",curSAO.sao_offset[i]);//4个 SAO Offset
						fprintf(p_pic_sao,"%08X\n",0);//1个 SAO BO band start idx
						fprintf(p_pic_sao,"%08X\n",0);//第2个 SAO BO band start idx的minus 2
						fprintf(p_pic_sao,"%08X\n",curSAO.typeIdc);//SAO EO 的方向 SAO_TYPE_EO_0, SAO_TYPE_EO_90, SAO_TYPE_EO_135,	SAO_TYPE_EO_45,
				  }
				break;
			}
			
#if EXTRACT_PIC_SAO_debug
			fprintf(p_pic_sao,"U cur_mb_nr=%d\n",cur_mb_nr);//SAO BO
#endif

			curSAO= (img->rec_saoBlkParams[cur_mb_nr][1]);
			switch (curSAO.modeIdc)
			{
				case 0:
					  for(i=0;i<8;i++)
						  fprintf(p_pic_sao,"%08X\n",0);//SAO Off
				  break;
				default:
				  if(curSAO.typeIdc == SAO_TYPE_BO)
				  {
						fprintf(p_pic_sao,"%08X\n",1);//SAO BO
						for(i=0;i<4;i++)
							fprintf(p_pic_sao,"%08X\n",curSAO.sao_offset[i]);//4个 SAO Offset
						fprintf(p_pic_sao,"%08X\n",curSAO.startPos);//1个 SAO BO band start idx
						fprintf(p_pic_sao,"%08X\n",curSAO.startPosM2);//第2个 SAO BO band start idx的minus 2
						fprintf(p_pic_sao,"%08X\n",0);//SAO EO 
				  }
				  else//EO
				  {
						fprintf(p_pic_sao,"%08X\n",2);//SAO EO
						for(i=0;i<4;i++)
							fprintf(p_pic_sao,"%08X\n",curSAO.sao_offset[i]);//4个 SAO Offset
						fprintf(p_pic_sao,"%08X\n",0);//1个 SAO BO band start idx
						fprintf(p_pic_sao,"%08X\n",0);//第2个 SAO BO band start idx的minus 2
						fprintf(p_pic_sao,"%08X\n",curSAO.typeIdc);//SAO EO 的方向 SAO_TYPE_EO_0, SAO_TYPE_EO_90, SAO_TYPE_EO_135,	SAO_TYPE_EO_45,
				  }
			  break;				
			}
			
#if EXTRACT_PIC_SAO_debug
			fprintf(p_pic_sao,"V cur_mb_nr=%d\n",cur_mb_nr);//SAO BO
#endif
			
			curSAO= (img->rec_saoBlkParams[cur_mb_nr][2]);
			switch (curSAO.modeIdc)
			{
				case 0:
					  for(i=0;i<8;i++)
						  fprintf(p_pic_sao,"%08X\n",0);//SAO Off
				  break;
				default:
				  if(curSAO.typeIdc == SAO_TYPE_BO)
				  {
						fprintf(p_pic_sao,"%08X\n",1);//SAO BO
						for(i=0;i<4;i++)
							fprintf(p_pic_sao,"%08X\n",curSAO.sao_offset[i]);//4个 SAO Offset
						fprintf(p_pic_sao,"%08X\n",curSAO.startPos);//1个 SAO BO band start idx
						fprintf(p_pic_sao,"%08X\n",curSAO.startPosM2);//第2个 SAO BO band start idx的minus 2
						fprintf(p_pic_sao,"%08X\n",0);//SAO EO 
				  }
				  else//EO
				  {
						fprintf(p_pic_sao,"%08X\n",2);//SAO EO
						for(i=0;i<4;i++)
							fprintf(p_pic_sao,"%08X\n",curSAO.sao_offset[i]);//4个 SAO Offset
						fprintf(p_pic_sao,"%08X\n",0);//1个 SAO BO band start idx
						fprintf(p_pic_sao,"%08X\n",0);//第2个 SAO BO band start idx的minus 2
						fprintf(p_pic_sao,"%08X\n",curSAO.typeIdc);//SAO EO 的方向 SAO_TYPE_EO_0, SAO_TYPE_EO_90, SAO_TYPE_EO_135,	SAO_TYPE_EO_45,
				  }
			  break;
			}
			//printf("p_pic_sao cur_mb_nr=%d\n",cur_mb_nr );
	  	cur_mb_nr++;
	  }
	}
	fflush(p_pic_sao);
	if(p_pic_sao != NULL)
	{
		fclose(p_pic_sao);
		p_pic_sao=NULL;
	}
	printf("fclose(p_pic_sao) \n" );
	
#endif

#ifdef AVS2_S2_BGLONGTERM
#if AVS2_SCENE_CD
  if((img->type == P_IMG || img->type == F_IMG) && background_picture_enable && background_reference_enable)
#else
  if(img->type == P_IMG && background_picture_enable && background_reference_enable && input->profile_id == 0x50)
#endif
  {
	  int j,i;
#if !M3476_BUF_FIX
	  for(j = 0; j < img->height/8; j++)
		  for(i = 0; i < img->width/8; i++)
#else 
	  for(j = 0; j < img->height/MIN_BLOCK_SIZE; j++)
		  for(i = 0; i < img->width/MIN_BLOCK_SIZE; i++)
#endif
		  {
			  int refframe  = refFrArr[j][i];
			  if(refframe==img->num_of_references-1)
				  refFrArr[j][i] = -1;
		  }
  }
#endif
}
void Copy_frame_for_ALF()
{
	int i, j, k;

	for (j = 0; j < img->height; j++)
	{
		for ( i = 0; i< img->width; i++)
		{
			imgY_alf_Rec[j*(img->width)+i] = imgY[j][i];
		}
	}

	for (k = 0; k < 2; k++)
	{
		for (j = 0; j < img->height_cr; j++)
		{
			for ( i = 0; i< img->width_cr; i++)
			{
				imgUV_alf_Rec[k][j*(img->width_cr)+i] = imgUV[k][j][i];
			}
		}
	}
}
void min_tr( outdata data, int* pos)
{
	int i,tmp_min;
	tmp_min = data.stdoutdata[0].tr;
	*pos = 0;
	for ( i=1; i<data.buffer_num; i++)
	{
		if (data.stdoutdata[i].tr<tmp_min)
		{
			tmp_min = data.stdoutdata[i].tr;
			*pos = i;
		}
	}
}
void delete_trbuffer( outdata *data, int pos)
{
	int i;
	for ( i=pos; i< data->buffer_num-1; i++ )
	{
		data->stdoutdata[i] = data->stdoutdata[i+1];
	}
	data->buffer_num--;
}
//cms 保存当前的mv 为参考mv
void addCurrMvtoBuf()
{
	int i, k, x, y;
	for ( i=0; i<REF_MAXBUFFER; i++ )
	{
		if ( img->coding_order == img->imgcoi_ref[i] )
		{
			break;
		}
	}
	for ( k = 0; k < 2; k++ )
	{
		for ( y = 0; y < img->height / MIN_BLOCK_SIZE; y++ )
		{
			for ( x = 0; x < img->width / MIN_BLOCK_SIZE ; x++ )
			{
				mvbuf[i][y][x][k] = (((img->type==F_IMG)||(img->type==P_IMG))? img->tmp_mv:img->fw_mv)[y][x][k];
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
#if MV_COMPRESSION

void compressMotion()
{
	int i, k, x, y;
	int width = img->width;
	int height = img->height;
	int xPos, yPos;

	for ( i=0; i<REF_MAXBUFFER; i++ )
	{
		if ( img->coding_order == img->imgcoi_ref[i] )
		{
			break;
		}
	}

	for (y = 0; y < height / MIN_BLOCK_SIZE; y ++)
	{
		for (x = 0; x < width / MIN_BLOCK_SIZE; x ++)
		{
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
#endif
/*
*************************************************************************
* Function:Prepare field and frame buffer after frame decoding
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void frame_postprocessing ()
{
  //pic dist by Grandview Semi. @ [06-07-20 15:25]
	img->PrevPicDistanceLsb = (img->coding_order%256);
  if ( p_ref && !input->ref_pic_order )
  {
#ifdef AVS2_S2_BGLONGTERM
	  if(img->typeb == BACKGROUND_IMG && background_picture_output_flag==0)
		  find_snr_background( snr, p_ref_background);
	  else
#endif
		find_snr ( snr, p_ref ); // if ref sequence exist
  }
  {
	  int pointer_tmp = outprint.buffer_num;
	  int i,pos;
	  outprint.stdoutdata[pointer_tmp].type = img->type;
#ifdef AVS2_S2_BGLONGTERM
	  outprint.stdoutdata[pointer_tmp].typeb = img->typeb;
#endif
	  outprint.stdoutdata[pointer_tmp].framenum = img->tr;
	  outprint.stdoutdata[pointer_tmp].tr = img->tr;
	  outprint.stdoutdata[pointer_tmp].qp = img->qp;
	  outprint.stdoutdata[pointer_tmp].snr_y = snr->snr_y;
	  outprint.stdoutdata[pointer_tmp].snr_u = snr->snr_u;
	  outprint.stdoutdata[pointer_tmp].snr_v = snr->snr_v;
	  outprint.stdoutdata[pointer_tmp].tmp_time = tmp_time;
	  outprint.stdoutdata[pointer_tmp].picture_structure = img->picture_structure;
	  outprint.stdoutdata[pointer_tmp].curr_frame_bits = StatBitsPtr->curr_frame_bits;
	  outprint.stdoutdata[pointer_tmp].emulate_bits = StatBitsPtr->emulate_bits;
	  outprint.buffer_num++;

#if MD5
	  if(input->MD5Enable & 0x02)
	  {
		  int j,k;
		  int img_width = ( img->width - img->auto_crop_right );
		  int img_height = ( img->height - img->auto_crop_bottom );
		  int img_width_cr = ( img_width / 2 );
		  int img_height_cr = ( img_height / ( input->chroma_format == 1 ? 2 : 1 ) );
		  int nSampleSize = input->output_bit_depth == 8 ? 1 : 2;
		  int shift1 = input->sample_bit_depth - input->output_bit_depth;
		  unsigned char * pbuf;
		  unsigned char * md5buf;
		  md5buf = (unsigned char *)malloc(img_height*img_width*nSampleSize + img_height_cr*img_width_cr*nSampleSize*2);
		  
		  if(md5buf!=NULL)
		  {
			  if (!shift1 && input->output_bit_depth == 8) // 8bit input -> 8bit encode
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
			  else if (!shift1 && input->output_bit_depth > 8) // 10/12bit input -> 10/12bit encode
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
			  else if (shift1 && input->output_bit_depth == 8) // 8bit input -> 10/12bit encode
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
	  }
	  else
	  {
		  memset(MD5val,0,16);
	  }
	  {
		  int j;
		  for(j=0; j<4; j++)
			  outprint.stdoutdata[pointer_tmp].DecMD5Value[j] = MD5val[j];
	  }
#endif

	  if (curr_RPS.referd_by_others && img->type!=I_IMG )
	  {
		  addCurrMvtoBuf();
#if MV_COMPRESSION
      compressMotion();
#endif
	  }

#ifdef AVS2_S2_BGLONGTERM

#if AVS2_SCENE_GB_CD
#else
	  if(img->typeb == BACKGROUND_IMG && background_picture_output_flag==0)
	  {
		  write_GB_frame(p_out_background);
		  report_frame( outprint, 0 );
		  delete_trbuffer( & outprint, 0 );
	  }
	  else
#endif

#endif
#if !REF_OUTPUT
		  for ( i=0; i< outprint.buffer_num; i++)
		  {
			  min_tr(outprint,&pos);
			  if ( outprint.stdoutdata[pos].tr < img->tr ||  outprint.stdoutdata[pos].tr == (last_output+1) )
			  {
				  last_output = outprint.stdoutdata[pos].tr;
				  report_frame( outprint, pos );
				  write_frame( p_out, outprint.stdoutdata[pos].tr );
				  delete_trbuffer( &outprint, pos );
				  i--;
			  }
			  else
			  {
				  break;
			  }
		  }
#else 
		  if (img->coding_order+total_frames*256>= picture_reorder_delay )
		  {

			  int tmp_min,pos=-1;
			  tmp_min = 1<<20;

			  for ( i=0; i<outprint.buffer_num; i++)
			  {
#if AVS2_SCENE_GB_CD
				  if (outprint.stdoutdata[i].tr<tmp_min && outprint.stdoutdata[i].tr>=last_output) //GB has the same "tr" with "last_output"
#else
				  if (outprint.stdoutdata[i].tr<tmp_min && outprint.stdoutdata[i].tr>last_output)
#endif
				  {
					  pos=i;
					  tmp_min=outprint.stdoutdata[i].tr;

				  }
			  }
			  if (pos!=-1)
			  {
				  last_output = outprint.stdoutdata[pos].tr;
				  report_frame( outprint, pos );
#if AVS2_SCENE_GB_CD
				  if(outprint.stdoutdata[pos].typeb == BACKGROUND_IMG && background_picture_output_flag == 0)
					  write_GB_frame( p_out_background );
				  else
#endif
				  write_frame( p_out, outprint.stdoutdata[pos].tr );
				  delete_trbuffer( &outprint, pos );

			  }

		  }
#endif 
  }
}

/*
******************************************************************************
*  Function: Determine the MVD's value (1/4 pixel) is legal or not.
*  Input:
*  Output:
*  Return:   0: out of the legal mv range; 1: in the legal mv range
*  Attention:
*  Author: Xiaozhen ZHENG, rm52k
******************************************************************************
*/
void DecideMvRange()
{
#ifdef AVS2_S2_BGLONGTERM
#if EXTEND_BD
	if ( input->profile_id == BASELINE_PROFILE || input->profile_id == BASELINE10_PROFILE )
#else
	if ( input->profile_id == 0x20 || input->profile_id == 0x40 || input->profile_id == 0x50 )
#endif
#else
#if EXTEND_BD
	if ( input->profile_id == 0x20 || input->profile_id == 0x40 || input->profile_id == 0x52 )
#else
  if ( input->profile_id == 0x20 || input->profile_id == 0x40 )
#endif
#endif
  {
    switch ( input->level_id )
    {
    case 0x10:
      Min_V_MV = -512;
      Max_V_MV =  511;
      Min_H_MV = -8192;
      Max_H_MV =  8191;
      break;
    case 0x20:
      Min_V_MV = -1024;
      Max_V_MV =  1023;
      Min_H_MV = -8192;
      Max_H_MV =  8191;
      break;
    case 0x22:
      Min_V_MV = -1024;
      Max_V_MV =  1023;
      Min_H_MV = -8192;
      Max_H_MV =  8191;
      break;
    case 0x40:
      Min_V_MV = -2048;
      Max_V_MV =  2047;
      Min_H_MV = -8192;
      Max_H_MV =  8191;
      break;
    case 0x42:
      Min_V_MV = -2048;
      Max_V_MV =  2047;
      Min_H_MV = -8192;
      Max_H_MV =  8191;
      break;
    }
  }
}

