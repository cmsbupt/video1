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


#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "../../lcommon/inc/memalloc.h"
#include "global.h"
#include "../../lcommon/inc/commonVariables.h"
#include "../../lcommon/inc/transform.h"
#include "../../lcommon/inc/defines.h"
#include "vlc.h"
#include "header.h"
#include "AEC.h"  
#include "math.h"
#include "DecAdaptiveLoopFilter.h"
#define max(a, b)                   ((a) > (b) ? (a) : (b))  //!< Macro returning max value 
#define min(a, b)                   ((a) < (b) ? (a) : (b))  //!< Macro returning min value 
// Adaptive frequency weighting quantization
#if FREQUENCY_WEIGHTING_QUANTIZATION
#include "wquant.h"
#endif


CameraParamters CameraParameter, *camera = &CameraParameter;

extern StatBits *StatBitsPtr;  

const int  InterlaceSetting[] = { 4, 4, 0, 5, 0, 5, 0, 5, 3, 3, 3, 3, 0,
                                  0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 0, 0, 4, 0
                                };


/*
*************************************************************************
* Function:sequence header
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void SequenceHeader ( char *buf, int startcodepos, int length )
{
	int i, j;

  memcpy ( currStream->streamBuffer, buf, length );
  currStream->code_len = currStream->bitstream_length = length;
  currStream->read_len = currStream->frame_bitoffset = ( startcodepos + 1 ) * 8;

  input->profile_id           = u_v ( 8, "profile_id" );
  input->level_id             = u_v ( 8, "level_id" );
  progressive_sequence        = u_v ( 1, "progressive_sequence" );
#if INTERLACE_CODING
  is_field_sequence           = u_v ( 1, "field_coded_sequence");
#endif

#if  EXTRACT_FULL   //2016-5-31 提取完整的参数
  sps_progressive_sequence = progressive_sequence;
  sps_field_coded_sequence = is_field_sequence;
#endif

#if HALF_PIXEL_COMPENSATION || HALF_PIXEL_CHROMA
  img->is_field_sequence    = is_field_sequence;
#endif
  horizontal_size             = u_v ( 14, "horizontal_size" );
  vertical_size               = u_v ( 14, "vertical_size" );
  input->chroma_format        = u_v ( 2, "chroma_format" );
#if EXTEND_BD
	input->output_bit_depth = 8;
	input->sample_bit_depth = 8;
  sample_precision            = 1;
  if (input->profile_id == BASELINE10_PROFILE) // 10bit profile (0x52)
  {
		input->output_bit_depth = u_v (3, "sample_precision" );
		input->output_bit_depth = 6 + (input->output_bit_depth) * 2;
		input->sample_bit_depth = u_v (3, "encoding_precision" );
		input->sample_bit_depth = 6 + (input->sample_bit_depth) * 2;
  }
	else // other profile
	{
#endif
		sample_precision            = u_v ( 3, "sample_precision" );
#if EXTEND_BD
	}
#endif

#if  EXTRACT_FULL   //2016-5-31 提取完整的参数
  if(input->sample_bit_depth == 8)
    sps_encoding_precision = 1;//1-8bit 2-10bit
  else
    sps_encoding_precision = 2;//1-8bit 2-10bit

  if (input->output_bit_depth == 8)
    sps_sample_precision = 1;//1-8bit 2-10bit
  else
    sps_sample_precision = 2;//1-8bit 2-10bit

#endif
  aspect_ratio_information    = u_v ( 4, "aspect_ratio_information" );
  frame_rate_code             = u_v ( 4, "frame_rate_code" );

  bit_rate_lower              = u_v ( 18, "bit_rate_lower" );
  marker_bit                  = u_v ( 1,  "marker bit" );
  //CHECKMARKERBIT
  bit_rate_upper              = u_v ( 12, "bit_rate_upper" );
  low_delay                   = u_v ( 1, "low_delay" );
  marker_bit                  = u_v ( 1, "marker bit" );
#if  EXTRACT_FULL   //2016-5-31 提取完整的参数
  sps_aspect_ratio    = aspect_ratio_information;
  sps_frame_rate_code = frame_rate_code;
  sps_bit_rate        = ((bit_rate_upper<<18) + bit_rate_lower);
  sps_low_delay       = low_delay;
#endif
  //CHECKMARKERBIT
#if M3480_TEMPORAL_SCALABLE
  temporal_id_exist_flag      = u_v(1, "temporal_id exist flag");							//get Extention Flag
#endif
  bbv_buffer_size = u_v ( 18, "bbv buffer size" );
  input->g_uiMaxSizeInBit     = u_v ( 3, "Largest Coding Block Size" );
#if  EXTRACT_FULL   //2016-5-31 提取完整的参数
  sps_temporal_id_enable = temporal_id_exist_flag;
  sps_bbv_buffer_size = bbv_buffer_size;
#endif
#if FREQUENCY_WEIGHTING_QUANTIZATION
  weight_quant_enable_flag		      = u_v ( 1, "weight_quant_enable" );
#if  EXTRACT_FULL   //2016-5-31 提取完整的参数
  sps_weight_quant_enable = weight_quant_enable_flag;
  sps_load_seq_weight_quant_data_flag = 0;
#endif
  if (weight_quant_enable_flag)
  {
	  int x,y,sizeId,uiWqMSize;
	  int *Seq_WQM;
    load_seq_weight_quant_data_flag        = u_v ( 1, "load_seq_weight_quant_data_flag");
#if  EXTRACT_FULL   //2016-5-31 提取完整的参数
    sps_load_seq_weight_quant_data_flag = load_seq_weight_quant_data_flag;
#endif

      for (sizeId=0;sizeId<2;sizeId++)
      {
        uiWqMSize=min(1<<(sizeId+2),8);
		    if (load_seq_weight_quant_data_flag == 1) 
		    {
           for(y=0; y<uiWqMSize; y++)
           {
             for(x=0; x<uiWqMSize; x++)
             {
               seq_wq_matrix[sizeId][y*uiWqMSize+x]= ue_v("weight_quant_coeff");
             }
           }
		    }
		    else if(load_seq_weight_quant_data_flag == 0)
		    {
			    Seq_WQM=GetDefaultWQM(sizeId);
			    for(i=0;i<(uiWqMSize*uiWqMSize);i++)
				    seq_wq_matrix[sizeId][i]=Seq_WQM[i];

		    }
      }

  }
#endif

#ifdef AVS2_S2_BGLONGTERM
#if AVS2_SCENE_CD
  background_picture_enable = 0x01 ^ u_v(1,"background_picture_disable");
#if  EXTRACT_FULL   //2016-5-31 提取完整的参数
  sps_scene_picture_disable = 0x01 ^ background_picture_enable;
#endif
#else
  if(input->profile_id == 0x50)
  {
    background_picture_enable = 0x01 ^ u_v(1,"background_picture_disable");
  }
  else
  {
    background_picture_enable = 0;
  }
#endif
#endif

#if !D20141230_BUG_FIX
  b_dmh_enabled		      = u_v ( 1, "dmh enabled" );
#else 
  b_dmh_enabled		      = 1;
#endif 

#if MH_PSKIP_NEW
  b_mhpskip_enabled       = u_v ( 1, "mhpskip enabled");
#endif
  dhp_enabled		      = u_v ( 1, "dhp enabled" );
  wsm_enabled		      = u_v ( 1, "wsm enabled" );

  img->inter_amp_enable       = u_v ( 1, "Asymmetric Motion Partitions" );
  input->useNSQT              = u_v ( 1, "useNSQT" );
  input->useSDIP              =u_v ( 1, "useNSIP" );
  
  b_secT_enabled		      = u_v ( 1, "secT enabled" );

  input->sao_enable = u_v ( 1, "SAO Enable Flag" );
  input->alf_enable = u_v(1,"ALF Enable Flag");

  b_pmvr_enabled		      = u_v ( 1, "pmvr enabled" );

  
#if  EXTRACT_FULL   //2016-5-31 提取完整的参数
  sps_secondary_transform_enable_flag = b_secT_enabled;
  sps_adaptive_loop_filter_enable = input->alf_enable;
#endif
#if DEMULATE_HLS
  u_v ( 1, "marker bit" );
#endif

  gop_size = u_v( 6, "num_of_RPS");
  for ( i=0; i<gop_size; i++)
  {
	  decod_RPS[i].referd_by_others =u_v(1,"refered by others");
	  decod_RPS[i].num_of_ref = u_v( 3, "num of reference picture" );

	  for ( j=0; j<decod_RPS[i].num_of_ref; j++)
	  {
		  decod_RPS[i].ref_pic[j]= u_v( 6, "delta COI of ref pic" );
	  }
	  decod_RPS[i].num_to_remove = u_v( 3, "num of removed picture" );

	  for ( j=0; j<decod_RPS[i].num_to_remove; j++)
	  {
		  decod_RPS[i].remove_pic[j] = u_v( 6, "delta COI of removed pic" );
	  }
#if DEMULATE_HLS
    u_v ( 1, "marker bit" );
#endif
  }
#if  EXTRACT_FULL   //2016-5-31 提取完整的参数
  sps_output_reorder_delay = 0;
#endif
  if (low_delay == 0)
  {
	  picture_reorder_delay =  u_v( 5, "picture_reorder_delay" );
  }
  input->crossSliceLoopFilter = u_v(1, "Cross Loop Filter Flag");
#if  EXTRACT_FULL   //2016-5-31 提取完整的参数
  sps_num_of_rcs = gop_size;
  sps_output_reorder_delay = picture_reorder_delay;
  sps_cross_slice_loopfilter_enable = input->crossSliceLoopFilter;
#endif

  if ( input->profile_id == RESERVED_PROFILE_ID )
  {
    input->slice_set_enable = /*slice_set_enable =*/ 0x01 ^ u_v ( 1, "slice set flag" );         //added by mz, 2008.04
  }
  else
  {
    u_v ( 2, "reserved bits" );
  }

  img->width          = horizontal_size;
  img->height         = vertical_size;
  img->width_cr       = ( img->width >> 1 );

  if ( input->chroma_format == 1 )
  {
    img->height_cr      = ( img->height >> 1 );
  }

  img->PicWidthInMbs  = img->width / MIN_CU_SIZE;
  img->PicHeightInMbs = img->height / MIN_CU_SIZE;
  img->PicSizeInMbs   = img->PicWidthInMbs * img->PicHeightInMbs;
  img->buf_cycle      = input->buf_cycle + 1;
  img->max_mb_nr      = ( img->width * img->height ) / ( MIN_CU_SIZE * MIN_CU_SIZE );
#if EXTRACT
	sprintf(filename,"./%s/sps/sps_%d.txt",infile,spsNum);
  if (p_sps != NULL)
  {
    fclose(p_sps);
    p_sps = NULL;
  }
	if ( ( p_sps=fopen(filename,"w+"))==0)
	{
	  snprintf ( errortext, ET_SIZE, "Error open sequence.txt!");
	  error ( errortext, 500 );
	}
#if EXTRACT_08X	
	fprintf(p_sps,"%08X\n",input->g_uiMaxSizeInBit );//lcu_size
	fprintf(p_sps,"%08X\n",horizontal_size);
	fprintf(p_sps,"%08X\n",vertical_size);
	fprintf(p_sps,"%08X\n",input->chroma_format);
  
	fprintf(p_sps,"%08X\n",input->sao_enable);
	fprintf(p_sps,"%08X\n",b_mhpskip_enabled);
	fprintf(p_sps,"%08X\n",dhp_enabled);
	fprintf(p_sps,"%08X\n",wsm_enabled);
  
	fprintf(p_sps,"%08X\n",img->inter_amp_enable);
	fprintf(p_sps,"%08X\n",input->useSDIP);
	fprintf(p_sps,"%08X\n",input->useNSQT);
	fprintf(p_sps,"%08X\n",b_pmvr_enabled);
#endif

#if EXTRACT_D	
	fprintf(p_sps,"%d\n",input->g_uiMaxSizeInBit );//lcu_size
	fprintf(p_sps,"%d\n",horizontal_size);
	fprintf(p_sps,"%d\n",vertical_size);
	fprintf(p_sps,"%d\n",input->chroma_format);
  
	fprintf(p_sps,"%d\n",input->sao_enable);
	fprintf(p_sps,"%d\n",b_mhpskip_enabled);
	fprintf(p_sps,"%d\n",dhp_enabled);
	fprintf(p_sps,"%d\n",wsm_enabled);
  
	fprintf(p_sps,"%d\n",img->inter_amp_enable);
	fprintf(p_sps,"%d\n",input->useSDIP);
	fprintf(p_sps,"%d\n",input->useNSQT);
	fprintf(p_sps,"%d\n",b_pmvr_enabled);
#endif
  if (p_sps != NULL)
  {
    fclose(p_sps);
    p_sps = NULL;
  }
	spsNum++;
	spsNewStart=1;//新的sps开始
#endif

  seq_header ++;
}


void video_edit_code_data ( char *buf, int startcodepos, int length )
{
  currStream->frame_bitoffset = currStream->read_len = ( startcodepos + 1 ) * 8;
  currStream->code_len = currStream->bitstream_length = length;
  memcpy ( currStream->streamBuffer, buf, length );
  vec_flag = 1;
}
/*
*************************************************************************
* Function:I picture header  //sw
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void I_Picture_Header ( char *buf, int startcodepos, int length )
{
  int i;
  currStream->frame_bitoffset = currStream->read_len = ( startcodepos + 1 ) * 8;
  currStream->code_len = currStream->bitstream_length = length;
  memcpy ( currStream->streamBuffer, buf, length );

  bbv_delay = u_v ( 32, "bbv_delay" );

  time_code_flag       = u_v ( 1, "time_code_flag" );
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
  pps_time_code = 0;//设置初始值
#endif
  if ( time_code_flag )
  {
    time_code        = u_v ( 24, "time_code" );
  }
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
  pps_bbv_delay = bbv_delay;
  pps_time_code_flag = time_code_flag;
  pps_time_code = time_code;
#endif
#ifdef AVS2_S2_BGLONGTERM
#if AVS2_SCENE_CD
  if(background_picture_enable )
#else
  if(input->profile_id == 0x50 && background_picture_enable )
#endif
  {
	  background_picture_flag = u_v ( 1, "background_picture_flag" );

	  if(background_picture_flag)
		  img->typeb = BACKGROUND_IMG;				
	  else
		  img->typeb = 0;

	  if(img->typeb == BACKGROUND_IMG)
		  background_picture_output_flag = u_v ( 1, "background_picture_output_flag" );  
  }
#endif

  {
    img->coding_order         = u_v ( 8, "coding_order" );
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
    pps_decode_order_index = img->coding_order;
    pps_temporal_id = 0;
    pps_picture_output_delay = 0;
#endif
#if M3480_TEMPORAL_SCALABLE
	  if ( temporal_id_exist_flag == 1 )
	  {
		  cur_layer = u_v( TEMPORAL_MAXLEVEL_BIT, "temporal_id" );
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
      pps_temporal_id = cur_layer;
#endif
	  }
#endif
	  if (low_delay == 0 && !(background_picture_enable && !background_picture_output_flag))//cdp
	  {
		  displaydelay =   ue_v("picture_output_delay");
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
      pps_picture_output_delay = displaydelay;
#endif
	  }

  }
  {
    int RPS_idx;// = (img->coding_order-1) % gop_size;
    int predict;
    predict = u_v( 1, "use RCS in SPS");
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
    pps_use_rcs_flag = predict;
#endif
    printf("======%d\n", predict);
    if (predict)
    {
      RPS_idx = u_v( 5, "predict for RCS");
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
      pps_rcs_index = RPS_idx;
#endif
      curr_RPS = decod_RPS[RPS_idx];
    }
    else
    {
      //gop size16
      int j;
      curr_RPS.referd_by_others =u_v(1,"refered by others");
      curr_RPS.num_of_ref = u_v( 3, "num of reference picture" );
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
      pps_rcs_index = sps_num_of_rcs;
#endif
      for ( j=0; j<curr_RPS.num_of_ref; j++)
      {
        curr_RPS.ref_pic[j] = u_v( 6, "delta COI of ref pic");
      }
      curr_RPS.num_to_remove = u_v( 3, "num of removed picture" );
      for ( j=0; j<curr_RPS.num_to_remove; j++)
      {
        curr_RPS.remove_pic[j] = u_v( 6, "delta COI of removed pic" );
      }
#if DEMULATE_HLS
      u_v ( 1, "marker bit" );
#endif
    }
  }
  //xyji 12.23
  if ( low_delay )
  {
    ue_v ( "bbv check times" );
  }

  progressive_frame    = u_v ( 1, "progressive_frame" );

  if ( !progressive_frame )
  {
    img->picture_structure   = u_v ( 1, "picture_structure" );
  }
  else
  {
    img->picture_structure = 1;
  }

  top_field_first      = u_v ( 1, "top_field_first" );
  repeat_first_field   = u_v ( 1, "repeat_first_field" );
#if INTERLACE_CODING
  if (is_field_sequence)
  {
    is_top_field         = u_v ( 1, "is_top_field" );
#if HALF_PIXEL_COMPENSATION || HALF_PIXEL_CHROMA
    img->is_top_field       = is_top_field;
#endif
    u_v ( 1, "reserved bit for interlace coding");
  }
#endif
  fixed_picture_qp     = u_v ( 1, "fixed_picture_qp" );
  picture_qp           = u_v ( 7, "picture_qp" );
#if EXTEND_BD
	//picture_qp = picture_qp + (8 * (input->sample_bit_depth - 8));
#endif
 
  loop_filter_disable = u_v ( 1, "loop_filter_disable" );

  if ( !loop_filter_disable )
  {
    loop_filter_parameter_flag = u_v ( 1, "loop_filter_parameter_flag" );

    if ( loop_filter_parameter_flag )
    {
      input->alpha_c_offset = se_v ( "alpha_offset" );
      input->beta_offset  = se_v ( "beta_offset" );
    }
    else  // 20071009
    {
      input->alpha_c_offset = 0;
      input->beta_offset  = 0;
    }
  }
#if CHROMA_DELTA_QP
#if CHROMA_DELTA_QP_SYNTEX_OPT
  chroma_quant_param_disable=u_v(1,"chroma_quant_param_disable");
  if(!chroma_quant_param_disable)
  {
	  chroma_quant_param_delta_u=se_v("chroma_quant_param_delta_cb");
	  chroma_quant_param_delta_v=se_v("chroma_quant_param_delta_cr");
  }
  else
  {
    chroma_quant_param_delta_u = 0;
    chroma_quant_param_delta_v = 0;
  }
#endif
#endif
  // Adaptive frequency weighting quantization
#if FREQUENCY_WEIGHTING_QUANTIZATION

  if (weight_quant_enable_flag)
  {
	  pic_weight_quant_enable_flag=u_v(1,"pic_weight_quant_enable");
	  if(pic_weight_quant_enable_flag)
	  {
		  pic_weight_quant_data_index=u_v(2,"pic_weight_quant_data_index");  //M2331 2008-04
		  if (pic_weight_quant_data_index == 1)
		  {
			  mb_adapt_wq_disable=u_v(1,"reserved_bits");  //M2331 2008-04
#if CHROMA_DELTA_QP
#if !CHROMA_DELTA_QP_SYNTEX_OPT
			  chroma_quant_param_disable=u_v(1,"chroma_quant_param_disable");
			  if(!chroma_quant_param_disable)
			  {
				  chroma_quant_param_delta_u=se_v("chroma_quant_param_delta_cb");
				  chroma_quant_param_delta_v=se_v("chroma_quant_param_delta_cr");
			  }
#endif
#endif
			  weighting_quant_param=u_v(2,"weighting_quant_param_index");

			  weighting_quant_model=u_v(2,"weighting_quant_model");
			  CurrentSceneModel = weighting_quant_model;

			  //M2331 2008-04
			  if(weighting_quant_param==1)
			  {
				  for( i=0;i<6;i++)
					  quant_param_undetail[i]=se_v("quant_param_delta_u")+wq_param_default[UNDETAILED][i];
			  }
			  if(weighting_quant_param==2)
			  {
				  for( i=0;i<6;i++)
					  quant_param_detail[i]=se_v("quant_param_delta_d")+wq_param_default[DETAILED][i];
			  }
			  //M2331 2008-04
		  } //pic_weight_quant_data_index == 1
		  else if (pic_weight_quant_data_index == 2)
		  {
			  int x,y,sizeId,uiWqMSize;
			  for (sizeId=0;sizeId<2;sizeId++)
			  {
				  int i=0;
				  uiWqMSize=min(1<<(sizeId+2),8);
				  for(y=0; y<uiWqMSize; y++)
				  {
					  for(x=0; x<uiWqMSize; x++)
					  {
						  pic_user_wq_matrix[sizeId][i++]= ue_v("weight_quant_coeff");
					  }
				  }
			  }
		  }//pic_weight_quant_data_index == 2
	  }
  }

#endif

  img->qp                = picture_qp;

  img->type              = I_IMG;
#if EXTRACT
    sprintf(filename,"./%s/PicInfo/picture_%04d.txt",infile,img->coding_order);
    if (p_pps != NULL)
    {
      fclose(p_pps);
      p_pps = NULL;
    }
	if( ( p_pps=fopen(filename,"w+"))==0 )
	{
		 snprintf ( errortext, ET_SIZE, "Error open PicInfo/picture_%04d.txt!",img->coding_order);
         error ( errortext, 500 );
	}

	sprintf(filename,"./%s/sps/sps_%04d.txt",infile,img->coding_order); 
  if (p_pic_sps != NULL)
  {
    fclose(p_pic_sps);
    p_pic_sps = NULL;
  }
	if( ( p_pic_sps=fopen(filename,"w+"))==0 )
	{
		 snprintf ( errortext, ET_SIZE, "Error open sps/sps_%04d.txt!",img->coding_order);
		 error ( errortext, 500 );
	}
#if EXTRACT_PIC_SAO   //2016-5-20 提取整帧的sao最后参数
	sprintf(filename,"./%s/saoinfo/pic_%04d.txt",infile,img->coding_order);
	if( ( p_pic_sao=fopen(filename,"w+"))==0 )
	{
		 snprintf ( errortext, ET_SIZE, "Error open saoinfo/pic_%04d.txt!",img->coding_order);
		 error ( errortext, 500 );
	}
#endif
    sprintf(filename,"./%s/mv_refinfo/picture_%04d.txt",infile,img->coding_order);
    if (p_mv_col != NULL)
    {
      fclose(p_mv_col);
      p_mv_col = NULL;
    }
	if( ( p_mv_col=fopen(filename,"w+"))==0 )
	{
		 snprintf ( errortext, ET_SIZE, "Error open mv_refinfo/picture_%04d.txt!",img->coding_order);
         error ( errortext, 500 );
	}
#if EXTRACT_08X	
	fprintf(p_pic_sps,"%08X\n",input->g_uiMaxSizeInBit );//lcu_size
	fprintf(p_pic_sps,"%08X\n",horizontal_size);
	fprintf(p_pic_sps,"%08X\n",vertical_size);
	fprintf(p_pic_sps,"%08X\n",input->chroma_format);
  
	fprintf(p_pic_sps,"%08X\n",input->sao_enable);
	fprintf(p_pic_sps,"%08X\n",b_mhpskip_enabled);
	fprintf(p_pic_sps,"%08X\n",dhp_enabled);
	fprintf(p_pic_sps,"%08X\n",wsm_enabled);
  
	fprintf(p_pic_sps,"%08X\n",img->inter_amp_enable);
	fprintf(p_pic_sps,"%08X\n",input->useSDIP);
	fprintf(p_pic_sps,"%08X\n",input->useNSQT);
	fprintf(p_pic_sps,"%08X\n",b_pmvr_enabled);
#if  EXTRACT_FULL   //2016-5-31 提取完整的参数
  fprintf(p_pic_sps, "%08X\n", input->profile_id);
  fprintf(p_pic_sps, "%08X\n", input->level_id);
  fprintf(p_pic_sps, "%08X\n", sps_progressive_sequence);
  fprintf(p_pic_sps, "%08X\n", sps_field_coded_sequence);
  fprintf(p_pic_sps, "%08X\n", sps_sample_precision);
  fprintf(p_pic_sps, "%08X\n", sps_encoding_precision);
  fprintf(p_pic_sps, "%08X\n", sps_aspect_ratio);
  fprintf(p_pic_sps, "%08X\n", sps_frame_rate_code);

  fprintf(p_pic_sps, "%08X\n", sps_bit_rate);
  fprintf(p_pic_sps, "%08X\n", sps_low_delay);
  fprintf(p_pic_sps, "%08X\n", sps_temporal_id_enable);
  fprintf(p_pic_sps, "%08X\n", sps_bbv_buffer_size);
  fprintf(p_pic_sps, "%08X\n", sps_weight_quant_enable);
  fprintf(p_pic_sps, "%08X\n", sps_load_seq_weight_quant_data_flag);
  fprintf(p_pic_sps, "%08X\n", sps_scene_picture_disable);
  fprintf(p_pic_sps, "%08X\n", sps_secondary_transform_enable_flag);

  fprintf(p_pic_sps, "%08X\n", sps_adaptive_loop_filter_enable);
  fprintf(p_pic_sps, "%08X\n", sps_num_of_rcs);
  fprintf(p_pic_sps, "%08X\n", sps_output_reorder_delay);
  fprintf(p_pic_sps, "%08X\n", sps_cross_slice_loopfilter_enable);
#endif 
  if (p_pic_sps != NULL)
  {
    fclose(p_pic_sps);
    p_pic_sps = NULL;
  }
#endif	
  //spsNewStart=1
  spsNewStart=0;
#if EXTRACT_08X
  fprintf(p_pps,"%08X\n",spsNewStart);
  fprintf(p_pps,"%08X\n",0);
#endif
	
#if EXTRACT_D
  fprintf(p_pps,"spsNewStart %d\n",spsNewStart);
  fprintf(p_pps,"Intra %d\n",0);
#endif

sprintf(newdir,"./%s/lcu_info/pic_%04d",infile,img->coding_order);
//system(newdir);
mkdir(newdir,0777);
printf("creat dir:%s\n",newdir);


#if EXTRACT_lcu_info_debug  //控制lcu debug信息是否打印
  sprintf(newdir,"./%s/lcu_debug/pic_%04d",infile,img->coding_order);
  //system(newdir);
  mkdir(newdir,0777);
  printf("creat dir:%s\n",newdir);
#endif


  sprintf(newdir,"./%s/lcu_coeff/pic_%04d",infile,img->coding_order);
  //system(newdir);
  mkdir(newdir,0777);
  printf("creat dir:%s\n",newdir);

  sprintf(newdir,"./%s/lcu_mv/pic_%04d",infile,img->coding_order);
  //system(newdir);
  mkdir(newdir,0777);
  printf("creat dir:%s\n",newdir);

  sliceNum=-1;
#endif  
}

/*
*************************************************************************
* Function:pb picture header
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
void PB_Picture_Header ( char *buf, int startcodepos, int length )
{
  int i;
  currStream->frame_bitoffset = currStream->read_len = ( startcodepos + 1 ) * 8;
  currStream->code_len = currStream->bitstream_length = length;
  memcpy ( currStream->streamBuffer, buf, length );

  u_v ( 32, "bbv delay" );
  
  picture_coding_type       = u_v ( 2, "picture_coding_type" );

#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
  pps_time_code = 0;//设置初始值
#endif
#ifdef AVS2_S2_BGLONGTERM
#if AVS2_SCENE_CD
  if( background_picture_enable && (picture_coding_type == 1 || picture_coding_type == 3) )
#else
  if( background_picture_enable && picture_coding_type == 1 && input->profile_id == 0x50 )
#endif
  {
#ifdef AVS2_S2_S
#if AVS2_SCENE_CD
	if(picture_coding_type ==1){
#endif
    background_pred_flag      = u_v ( 1, "background_pred_flag" ); 
#if AVS2_SCENE_CD
	}else{
		background_pred_flag = 0;
	}
#endif
    if( background_pred_flag == 0 ) 
    {
#endif
      background_reference_enable = u_v ( 1, "background_reference_enable" );
#ifdef AVS2_S2_S
    }
    else
    {
        background_reference_enable = 0;
    }
#endif
  }
#ifdef AVS2_S2_SBD
  else
  {
    background_pred_flag = 0;
    background_reference_enable = 0;
  }
#endif
#endif

#ifdef AVS2_S2_S
  if(picture_coding_type == 1 && background_pred_flag)
  {
	  img->typeb = BP_IMG;
  }
  else
  {
	  img->typeb = 0;
  }
#endif
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
  pps_bbv_delay = bbv_delay;
  pps_time_code_flag = 0;//帧间帧中 不存在该句法，设置为0
  pps_time_code = 0;//帧间帧中 不存在该句法，设置为0
#endif
  {
	  img->coding_order         = u_v ( 8, "coding_order" );
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
    pps_decode_order_index = img->coding_order;
    pps_temporal_id = 0;
    pps_picture_output_delay = 0;
#endif	  
#if M3480_TEMPORAL_SCALABLE
	  if ( temporal_id_exist_flag == 1 ){
		  cur_layer = u_v( TEMPORAL_MAXLEVEL_BIT, "temporal_id" );
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
      pps_temporal_id = cur_layer;
#endif   
    }
#endif

	  if (low_delay == 0)
	  {
		  displaydelay =   ue_v("displaydelay");
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
      pps_picture_output_delay = displaydelay;
#endif      
	  }
 
  }
  {
	  int RPS_idx;// = (img->coding_order-1) % gop_size;
	  int predict;
	  predict = u_v( 1, "use RPS in SPS");
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
    pps_use_rcs_flag = predict;
#endif    
	  if (predict)
	  {
		  RPS_idx = u_v( 5, "predict for RPS");
		  curr_RPS = decod_RPS[RPS_idx];
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
      pps_rcs_index = RPS_idx;
#endif      
	  }
	  else
	  {
		  //gop size16
		  int j;
		  curr_RPS.referd_by_others =u_v(1,"refered by others");
		  curr_RPS.num_of_ref = u_v( 3, "num of reference picture" );
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
      pps_rcs_index = sps_num_of_rcs;
#endif      
		  for ( j=0; j<curr_RPS.num_of_ref; j++)
		  {
			  curr_RPS.ref_pic[j] = u_v( 6, "delta COI of ref pic");
		  }
		  curr_RPS.num_to_remove = u_v( 3, "num of removed picture" );
		  for ( j=0; j<curr_RPS.num_to_remove; j++)
		  {
			  curr_RPS.remove_pic[j] = u_v( 6, "delta COI of removed pic" );
		  }
#if DEMULATE_HLS
      u_v ( 1, "marker bit" );
#endif
	  }
  }
  //xyji 12.23
  if ( low_delay )
  {
    ue_v ( "bbv check times" );
  }

  progressive_frame        = u_v ( 1, "progressive_frame" );

  if ( !progressive_frame )
  {
    img->picture_structure = u_v ( 1, "picture_structure" );
  }
  else
  {
    img->picture_structure   = 1;
  }

  top_field_first        = u_v ( 1, "top_field_first" );
  repeat_first_field     = u_v ( 1, "repeat_first_field" );
#if INTERLACE_CODING
  if (is_field_sequence)
  {
    is_top_field         = u_v ( 1, "is_top_field" );
#if HALF_PIXEL_COMPENSATION || HALF_PIXEL_CHROMA
    img->is_top_field       = is_top_field;
#endif
    u_v ( 1, "reserved bit for interlace coding");
  }
#endif

  fixed_picture_qp       = u_v ( 1, "fixed_picture_qp" );
  picture_qp             = u_v ( 7, "picture_qp" );
#if EXTEND_BD
	//picture_qp = picture_qp + (8 * (input->sample_bit_depth - 8));
#endif

  if ( ! ( picture_coding_type == 2 && img->picture_structure == 1 ) )
  {
#if !M3595_REMOVE_REFERENCE_FLAG
    picture_reference_flag = u_v ( 1, "picture_reference_flag" );
#else 
    u_v ( 1, "reserved_bit" );
#endif 
  }
#if !M3475_RANDOM_ACCESS
  img->no_forward_reference = u_v ( 1, "no_forward_reference_flag" );
#else 
  img->random_access_decodable_flag = u_v ( 1, "random_access_decodable_flag" );
#endif 
  //u_v ( 3, "reserved bits" );

  /*  skip_mode_flag      = u_v(1,"skip_mode_flag");*/  //qyu0822 delete skip_mode_flag
  loop_filter_disable = u_v ( 1, "loop_filter_disable" );

  if ( !loop_filter_disable )
  {
    loop_filter_parameter_flag = u_v ( 1, "loop_filter_parameter_flag" );

    if ( loop_filter_parameter_flag )
    {
      input->alpha_c_offset = se_v ( "alpha_offset" );
      input->beta_offset  = se_v ( "beta_offset" );
    }
    else
    {
      input->alpha_c_offset = 0;
      input->beta_offset  = 0;
    }
  }
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
  pps_loop_filter_disable =loop_filter_disable;
  pps_AlphaCOffset  = input->alpha_c_offset;
  pps_BetaOffset    = input->alpha_c_offset;
#endif
#if CHROMA_DELTA_QP
#if CHROMA_DELTA_QP_SYNTEX_OPT
  chroma_quant_param_disable=u_v(1,"chroma_quant_param_disable");
  if(!chroma_quant_param_disable)
  {
	  chroma_quant_param_delta_u=se_v("chroma_quant_param_delta_cb");
	  chroma_quant_param_delta_v=se_v("chroma_quant_param_delta_cr");
  }
  else
  {
    chroma_quant_param_delta_u = 0;
    chroma_quant_param_delta_v = 0;
  }
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
  pps_chroma_quant_param_delta_cb = chroma_quant_param_delta_u;
  pps_chroma_quant_param_delta_cr = chroma_quant_param_delta_v;
#endif
#endif
#endif
  // Adaptive frequency weighting quantization
#if FREQUENCY_WEIGHTING_QUANTIZATION
  if (weight_quant_enable_flag)
  {
	  pic_weight_quant_enable_flag=u_v(1,"pic_weight_quant_enable");
	  if(pic_weight_quant_enable_flag)
	  {
		  pic_weight_quant_data_index=u_v(2,"pic_weight_quant_data_index");  //M2331 2008-04
		  if (pic_weight_quant_data_index == 1)
		  {
			  mb_adapt_wq_disable=u_v(1,"reserved_bits");  //M2331 2008-04
#if CHROMA_DELTA_QP
#if !CHROMA_DELTA_QP_SYNTEX_OPT
			  chroma_quant_param_disable=u_v(1,"chroma_quant_param_disable");
			  if(!chroma_quant_param_disable)
			  {
				  chroma_quant_param_delta_u=se_v("chroma_quant_param_delta_cb");
				  chroma_quant_param_delta_v=se_v("chroma_quant_param_delta_cr");
			  }
#endif
#endif
			  weighting_quant_param=u_v(2,"weighting_quant_param_index");

			  weighting_quant_model=u_v(2,"weighting_quant_model");
			  CurrentSceneModel = weighting_quant_model;

			  //M2331 2008-04
			  if(weighting_quant_param==1)
			  {
				  for( i=0;i<6;i++)
					  quant_param_undetail[i]=se_v("quant_param_delta_u")+wq_param_default[UNDETAILED][i];
			  }
			  if(weighting_quant_param==2)
			  {
				  for( i=0;i<6;i++)
					  quant_param_detail[i]=se_v("quant_param_delta_d")+wq_param_default[DETAILED][i];
			  }
			  //M2331 2008-04
		  } //pic_weight_quant_data_index == 1
		  else if (pic_weight_quant_data_index == 2)
		  {
			  int x,y,sizeId,uiWqMSize;
			  for (sizeId=0;sizeId<2;sizeId++)
			  {
				  int i=0;
				  uiWqMSize=min(1<<(sizeId+2),8);
				  for(y=0; y<uiWqMSize; y++)
				  {
					  for(x=0; x<uiWqMSize; x++)
					  {
						  pic_user_wq_matrix[sizeId][i++]= ue_v("weight_quant_coeff");
					  }
				  }
			  }
		  }//pic_weight_quant_data_index == 2
	  }
  }

#endif

  img->qp                = picture_qp;

  if ( picture_coding_type == 1 )
  {
	  img->type = P_IMG;
  }
  else if ( picture_coding_type == 3 )
  {
	  img->type = F_IMG;
  }
  else
  {
    img->type = B_IMG;
  }
#if EXTRACT
  if(picture_coding_type == 1 && background_pred_flag)
		RefPicNum=1;//BP_IMG->S_IMG
	else 
		RefPicNum=curr_RPS.num_of_ref;
  sprintf(filename,"./%s/PicInfo/picture_%04d.txt",infile,img->coding_order); if (p_pps != NULL)
  {
    fclose(p_pps);
    p_pps = NULL;
  }
	if( ( p_pps=fopen(filename,"w+"))==0 )
	{
		 snprintf ( errortext, ET_SIZE, "Error open PicInfo/picture_%04d.txt!",img->coding_order);
         error ( errortext, 500 );
	}
	
	sprintf(filename,"./%s/sps/sps_%04d.txt",infile,img->coding_order);
  if (p_pic_sps != NULL)
  {
    fclose(p_pic_sps);
    p_pic_sps = NULL;
  }
	if( ( p_pic_sps=fopen(filename,"w+"))==0 )
	{
		 snprintf ( errortext, ET_SIZE, "Error open sps/sps_%04d.txt!",img->coding_order);
		 error ( errortext, 500 );
	}
	
#if EXTRACT_PIC_SAO   //2016-5-20 提取整帧的sao最后参数
		sprintf(filename,"./%s/saoinfo/pic_%04d.txt",infile,img->coding_order);
		if( ( p_pic_sao=fopen(filename,"w+"))==0 )
		{
			 snprintf ( errortext, ET_SIZE, "Error open saoinfo/pic_%04d.txt!",img->coding_order);
			 error ( errortext, 500 );
		}
#endif

	sprintf(filename,"./%s/mv_refinfo/picture_%04d.txt",infile,img->coding_order);
  if (p_mv_col != NULL)
  {
    fclose(p_mv_col);
    p_mv_col = NULL;
  }
	if( ( p_mv_col=fopen(filename,"w+"))==0 )
	{
		 snprintf ( errortext, ET_SIZE, "Error open mv_refinfo/picture_%04d.txt!",img->coding_order);
         error ( errortext, 500 );
	}
#if EXTRACT_08X	
	fprintf(p_pic_sps,"%08X\n",input->g_uiMaxSizeInBit );//lcu_size
	fprintf(p_pic_sps,"%08X\n",horizontal_size);
	fprintf(p_pic_sps,"%08X\n",vertical_size);
	fprintf(p_pic_sps,"%08X\n",input->chroma_format);
  
	fprintf(p_pic_sps,"%08X\n",input->sao_enable);
	fprintf(p_pic_sps,"%08X\n",b_mhpskip_enabled);
	fprintf(p_pic_sps,"%08X\n",dhp_enabled);
	fprintf(p_pic_sps,"%08X\n",wsm_enabled);
  
	fprintf(p_pic_sps,"%08X\n",img->inter_amp_enable);
	fprintf(p_pic_sps,"%08X\n",input->useSDIP);
	fprintf(p_pic_sps,"%08X\n",input->useNSQT);
	fprintf(p_pic_sps,"%08X\n",b_pmvr_enabled);

#if  EXTRACT_FULL   //2016-5-31 提取完整的参数
  fprintf(p_pic_sps, "%08X\n", input->profile_id);
  fprintf(p_pic_sps, "%08X\n", input->level_id);
  fprintf(p_pic_sps, "%08X\n", sps_progressive_sequence);
  fprintf(p_pic_sps, "%08X\n", sps_field_coded_sequence);
  fprintf(p_pic_sps, "%08X\n", sps_sample_precision);
  fprintf(p_pic_sps, "%08X\n", sps_encoding_precision);
  fprintf(p_pic_sps, "%08X\n", sps_aspect_ratio);
  fprintf(p_pic_sps, "%08X\n", sps_frame_rate_code);

  fprintf(p_pic_sps, "%08X\n", sps_bit_rate);
  fprintf(p_pic_sps, "%08X\n", sps_low_delay);
  fprintf(p_pic_sps, "%08X\n", sps_temporal_id_enable);
  fprintf(p_pic_sps, "%08X\n", sps_bbv_buffer_size);
  fprintf(p_pic_sps, "%08X\n", sps_weight_quant_enable);
  fprintf(p_pic_sps, "%08X\n", sps_load_seq_weight_quant_data_flag);
  fprintf(p_pic_sps, "%08X\n", sps_scene_picture_disable);
  fprintf(p_pic_sps, "%08X\n", sps_secondary_transform_enable_flag);

  fprintf(p_pic_sps, "%08X\n", sps_adaptive_loop_filter_enable);
  fprintf(p_pic_sps, "%08X\n", sps_num_of_rcs);
  fprintf(p_pic_sps, "%08X\n", sps_output_reorder_delay);
  fprintf(p_pic_sps, "%08X\n", sps_cross_slice_loopfilter_enable);
#endif
  if (p_pic_sps != NULL)
  {
    fclose(p_pic_sps);
    p_pic_sps = NULL;
  }
#endif	

#if EXTRACT_08X
  fprintf(p_pps,"%08X\n",spsNewStart);
	fprintf(p_pps,"%08X\n",1);
#endif

#if EXTRACT_D
  fprintf(p_pps,"%d\n",spsNewStart);
	fprintf(p_pps,"%d\n",1);
#endif

  spsNewStart=0;

  sprintf(newdir,"./%s/lcu_info/pic_%04d",infile,img->coding_order);
  //system(newdir);
  mkdir(newdir,0777);
  printf("creat dir:%s\n",newdir);

#if EXTRACT_lcu_info_debug  //控制lcu debug信息是否打印
		sprintf(newdir,"./%s/lcu_debug/pic_%04d",infile,img->coding_order);
		//system(newdir);
		mkdir(newdir,0777);
		printf("creat dir:%s\n",newdir);
#endif

  sprintf(newdir,"./%s/lcu_coeff/pic_%04d",infile,img->coding_order);
  //system(newdir);
  mkdir(newdir,0777);
  printf("creat dir:%s\n",newdir);

  sprintf(newdir,"./%s/lcu_mv/pic_%04d",infile,img->coding_order);
  //system(newdir);
  mkdir(newdir,0777);
  printf("creat dir:%s\n",newdir);


  sliceNum=-1;
#endif  
}
/*
*************************************************************************
* Function:decode extension and user data
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void extension_data ( char *buf, int startcodepos, int length )
{
  int ext_ID;

  memcpy ( currStream->streamBuffer, buf, length );
  currStream->code_len = currStream->bitstream_length = length;
  currStream->read_len = currStream->frame_bitoffset = ( startcodepos + 1 ) * 8;

  ext_ID = u_v ( 4, "extension ID" );

  switch ( ext_ID )
  {
  case SEQUENCE_DISPLAY_EXTENSION_ID:
    sequence_display_extension();
    break;
  case COPYRIGHT_EXTENSION_ID:
    copyright_extension();
    break;
  case PICTURE_DISPLAY_EXTENSION_ID:
    picture_display_extension();
    break;    //rm52k_r1
  case CAMERAPARAMETERS_EXTENSION_ID:
    cameraparameters_extension();
    break;
#if M3480_TEMPORAL_SCALABLE
  case TEMPORAL_SCALABLE_EXTENSION_ID:
	  scalable_extension();
	break;
#endif
#if ROI_M3264  
  case LOCATION_DATA_EXTENSION_ID:
    input->ROI_Coding = 1;
    OpenPosFile( input->out_datafile );	
    roi_parameters_extension();
    break; 
#endif
#if AVS2_HDR_HLS
  case MASTERING_DISPLAY_AND_CONTENT_METADATA_EXTENSION:
  break;
#endif
  default:
    printf ( "reserved extension start code ID %d\n", ext_ID );
    break;
  }

}
/*
*************************************************************************
* Function: decode sequence display extension
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
void sequence_display_extension()
{
  video_format      = u_v ( 3, "video format ID" );
  video_range       = u_v ( 1, "video range" );
  color_description = u_v ( 1, "color description" );

  if ( color_description )
  {
    color_primaries          = u_v ( 8, "color primaries" );
    transfer_characteristics = u_v ( 8, "transfer characteristics" );
    matrix_coefficients      = u_v ( 8, "matrix coefficients" );
  }

  display_horizontal_size = u_v ( 14, "display_horizontaol_size" );
  marker_bit              = u_v ( 1, "marker bit" );
  display_vertical_size   = u_v ( 14, "display_vertical_size" );
#if !M3472
  u_v ( 2, "reserved bits" );
#else
  TD_mode                = u_v ( 1, "3D_mode" );

  if ( TD_mode )
  {
    view_packing_mode         = u_v ( 8, "view_packing_mode" );
    view_reverse              = u_v ( 1, "view_reverse" );
  }
#endif

}

/*
*************************************************************************
* Function: decode picture display extension
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
void picture_display_extension()
{
  int NumberOfFrameCentreOffsets;
  int i;

  if ( progressive_sequence == 1 )
  {
    if ( repeat_first_field == 1 )
    {
      if ( top_field_first == 1 )
      {
        NumberOfFrameCentreOffsets = 3;
      }
      else
      {
        NumberOfFrameCentreOffsets = 2;
      }
    }
    else
    {
      NumberOfFrameCentreOffsets = 1;
    }
  }
  else
  {
    if ( img->picture_structure == 1 )
    {
      if ( repeat_first_field == 1 )
      {
        NumberOfFrameCentreOffsets = 3;
      }
      else
      {
        NumberOfFrameCentreOffsets = 2;
      }
    }
  }

  for ( i = 0; i < NumberOfFrameCentreOffsets; i++ )
  {
    frame_centre_horizontal_offset[i] = u_v ( 16, "frame_centre_horizontal_offset" );
    marker_bit = u_v ( 1, "marker_bit" );
    frame_centre_vertical_offset[i]   = u_v ( 16, "frame_centre_vertical_offset" );
    marker_bit = u_v ( 1, "marker_bit" );
  }
}

/*
*************************************************************************
* Function:user data //sw
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void user_data ( char *buf, int startcodepos, int length )
{
  int i;
  int user_data;
  memcpy ( currStream->streamBuffer, buf, length );

  currStream->code_len = currStream->bitstream_length = length;
  currStream->read_len = currStream->frame_bitoffset = ( startcodepos + 1 ) * 8;

  for ( i = 0; i < length - 4; i++ )
  {
    user_data = u_v ( 8, "user data" );
  }
}

/*
*************************************************************************
* Function:Copyright extension //sw
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void copyright_extension()
{
  int reserved_data;
  int marker_bit;
  int copyright_number;

  copyright_flag       =  u_v ( 1, "copyright_flag" );
  copyright_identifier =  u_v ( 8, "copyright_identifier" );
  original_or_copy     =  u_v ( 1, "original_or_copy" );

  /* reserved */
  reserved_data        = u_v ( 7, "reserved_data" );
  marker_bit           = u_v ( 1, "marker_bit" );
  copyright_number_1 = u_v ( 20, "copyright_number_1" );
  marker_bit         = u_v ( 1, "marker_bit" );
  copyright_number_2 = u_v ( 22, "copyright_number_2" );
  marker_bit         = u_v ( 1, "marker_bit" );
  copyright_number_3 = u_v ( 22, "copyright_number_3" );

  copyright_number = ( copyright_number_1 << 44 ) + ( copyright_number_2 << 22 ) + copyright_number_3;

}


/*
*************************************************************************
* Function:camera parameters extension //sw
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void cameraparameters_extension()
{

  u_v ( 1, "reserved" );
  camera_id               = u_v ( 7, "camera id" );
  marker_bit = u_v ( 1, "marker bit" );
  height_of_image_device  = u_v ( 22, "height_of_image_device" );
  marker_bit = u_v ( 1, "marker bit" );
  focal_length            = u_v ( 22, "focal_length" );
  marker_bit = u_v ( 1, "marker bit" );
  f_number                = u_v ( 22, "f_number" );
  marker_bit = u_v ( 1, "marker bit" );
  vertical_angle_of_view  = u_v ( 22, "vertical_angle_of_view" );
  marker_bit = u_v ( 1, "marker bit" );

  camera_position_x_upper = u_v ( 16, "camera_position_x_upper" );
  marker_bit = u_v ( 1, "marker bit" );
  camera_position_x_lower = u_v ( 16, "camera_position_x_lower" );
  marker_bit = u_v ( 1, "marker bit" );

  camera_position_y_upper = u_v ( 16, "camera_position_y_upper" );
  marker_bit = u_v ( 1, "marker bit" );
  camera_position_y_lower = u_v ( 16, "camera_position_y_lower" );
  marker_bit = u_v ( 1, "marker bit" );

  camera_position_z_upper = u_v ( 16, "camera_position_z_upper" );
  marker_bit = u_v ( 1, "marker bit" );
  camera_position_z_lower = u_v ( 16, "camera_position_z_lower" );
  marker_bit = u_v ( 1, "marker bit" );

  camera_direction_x      = u_v ( 22, "camera_direction_x" );
  marker_bit = u_v ( 1, "marker bit" );

  camera_direction_y      = u_v ( 22, "camera_direction_y" );
  marker_bit = u_v ( 1, "marker bit" );

  camera_direction_z      = u_v ( 22, "camera_direction_z" );
  marker_bit = u_v ( 1, "marker bit" );

  image_plane_vertical_x  = u_v ( 22, "image_plane_vertical_x" );
  marker_bit = u_v ( 1, "marker bit" );

  image_plane_vertical_y  = u_v ( 22, "image_plane_vertical_y" );
  marker_bit = u_v ( 1, "marker bit" );

  image_plane_vertical_z  = u_v ( 22, "image_plane_vertical_z" );
  marker_bit = u_v ( 1, "marker bit" );

  u_v ( 16, "reserved data" );
}

#if M3480_TEMPORAL_SCALABLE
/*
*************************************************************************
* Function:temporal scalable extension data
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
void scalable_extension()
{
	int i;
	int max_temporal_level = u_v ( 3, "max temporal level" );
	for( i = 0; i < max_temporal_level; i ++ )
	{
		u_v ( 4, "fps code per temporal level" );
		u_v ( 18, "bit rate lower" );
		u_v ( 1, "marker bit" );
		u_v ( 12, "bit rate upper" );
	}
}
#endif

/*
*************************************************************************
* Function:mastering display and content metadata extension data
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
void mastering_display_and_content_metadata_extension()
{
  display_primaries_x0 = u_v (16, "display_primaries_x0");
  u_v ( 1, "marker bit" );
  display_primaries_y0 = u_v (16, "display_primaries_y0");
  u_v ( 1, "marker bit" );
  display_primaries_x1 = u_v (16, "display_primaries_x1");
  u_v ( 1, "marker bit" );
  display_primaries_y1 = u_v (16, "display_primaries_y1");
  u_v ( 1, "marker bit" );
  display_primaries_x2 = u_v (16, "display_primaries_x2");
  u_v ( 1, "marker bit" );
  display_primaries_y2 = u_v (16, "display_primaries_y2");
  u_v ( 1, "marker bit" );

  white_point_x = u_v (16, "white_point_x");
  u_v ( 1, "marker bit" );
  white_point_y = u_v (16, "white_point_y");
  u_v ( 1, "marker bit" );

  max_display_mastering_luminance = u_v (16, "max_display_mastering_luminance");
  u_v ( 1, "marker bit" );
  min_display_mastering_luminance = u_v (16, "min_display_mastering_luminance");
  u_v ( 1, "marker bit" );

  maximum_content_light_level = u_v (16, "maximum_content_light_level");
  u_v ( 1, "marker bit" );
  maximum_frame_average_light_level = u_v (16, "maximum_frame_average_light_level");
  u_v ( 1, "marker bit" );
}
/*
*************************************************************************
* Function:To calculate the poc values
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
//cms  计算新的poc
void calc_picture_distance ()
{
  // for POC mode 0:
  unsigned int        MaxPicDistanceLsb = ( 1 << 8 );
  //cms 触发修正DPB 中当前doi小于参考帧的doi条件需要修正
  if ( img->coding_order  <  img->PrevPicDistanceLsb)
  {
	  {
		  int i, j;
		  total_frames++;
		  for (i=0; i<REF_MAXBUFFER; i++)
		  {
			  if (img->imgtr_fwRefDistance[i] >= 0)
			  {
				  img->imgtr_fwRefDistance[i] -= 256;
				  img->imgcoi_ref[i] -= 256;
			  }
			  for (j=0; j<4; j++)
			  {
				  ref_poc[i][j] -= 256;
			  }
		  }
		  last_output -= 256;
		  for (i=0; i<outprint.buffer_num; i++)
		  {
			  outprint.stdoutdata[i].framenum -= 256;
			  outprint.stdoutdata[i].tr -= 256;
		  }
		  curr_IDRtr  -= 256;
		  curr_IDRcoi -= 256;
		  next_IDRtr  -= 256;
		  next_IDRcoi -= 256;
	  }
  }
  if (low_delay == 0)
  {
	  img->tr = img->coding_order + displaydelay -picture_reorder_delay;
  } 
  else
  {
	  img->tr = img->coding_order;
  }


  img->pic_distance = img->tr%256;
  picture_distance = img->pic_distance;

}

void SliceHeader ( char *buf, int startcodepos, int length )
{
  int mb_row;
  int mb_column;
  int mb_width;

  memcpy ( currStream->streamBuffer, buf, length );
  currStream->code_len = currStream->bitstream_length = length;

  currStream->read_len = currStream->frame_bitoffset = ( startcodepos ) * 8;
  slice_vertical_position              = u_v ( 8, "slice vertical position" );
#if EXTRACT
	etr_slice_vertical_position=slice_vertical_position;
#endif
  if ( vertical_size > (144*(1<<input->g_uiMaxSizeInBit)) )
  {
    slice_vertical_position_extension  = u_v ( 3, "slice vertical position extension" );
		etr_slice_vertical_position_ext = slice_vertical_position_extension;
  }
  else
  	etr_slice_vertical_position_ext = 0; 

  if ( vertical_size > (144*(1<<input->g_uiMaxSizeInBit)) )
  {
    mb_row = ( slice_vertical_position_extension << 7 ) + slice_vertical_position;
  }
  else
  {
    mb_row = slice_vertical_position;
  }

  slice_horizontal_positon = u_v ( 8, "slice horizontal position" );
#if EXTRACT
	etr_slice_horizontal_position=slice_horizontal_positon;
#endif	
  if ( img->width > (255*(1<<input->g_uiMaxSizeInBit)) )
  {
    slice_horizontal_positon_extension = u_v ( 2, "slice horizontal position extension" );
		etr_slice_horizontal_position_ext= slice_horizontal_positon_extension;
  }  
  else
  	etr_slice_horizontal_position_ext = 0; 

  if ( input->profile_id == RESERVED_PROFILE_ID )
  {
    if ( input->slice_set_enable )
    {
      slice_horizontal_positon = u_v ( 8, "slice horizontal position" );

      if ( horizontal_size > 4080 )
      {
        slice_horizontal_positon_extension = u_v ( 2, "slice horizontal position extension" );
      }

      img->current_slice_set_index = u_v ( 6, "slice set index" );
      img->current_slice_header_flag = u_v ( 1, "slice header flag" );

      if ( horizontal_size > 4080 )
      {
        mb_column = ( slice_horizontal_positon_extension << 8 ) + slice_horizontal_positon;
      }
      else
      {
        mb_column = slice_horizontal_positon;
      }
    }
  }

#if M3198_CU8
  mb_width  = ( horizontal_size + MIN_CU_SIZE - 1) / MIN_CU_SIZE;
#else
  mb_width  = ( horizontal_size + 15 ) / 16;
#endif

  if ( !input->slice_set_enable || ( input->slice_set_enable && img->current_slice_header_flag ) ) //added by mz, 2008.04
  {
    if ( !fixed_picture_qp )
    {
      fixed_slice_qp   = u_v ( 1, "fixed_slice_qp" );
	      etr_fixed_slice_qp = fixed_slice_qp;
#if !BUGFIX20150607
      slice_qp         = u_v ( 6, "slice_qp" );
#else 
      slice_qp         = u_v ( 7, "slice_qp" );
#endif 
	  etr_slice_qp = slice_qp;

      img->qp = slice_qp;
      img->slice_set_qp[img->current_slice_set_index] = slice_qp; //added by mz, 2008.04
#if MB_DQP
      input->useDQP = !fixed_slice_qp;
#endif
    }
#if MB_DQP
    else
    {
      input->useDQP = 0;
	  etr_slice_qp = picture_qp ;
	  etr_fixed_slice_qp = fixed_picture_qp;
    }
#endif
  }
#if MB_DQP
  else {
    input->useDQP = 0;
  }
#endif

  //added by mz, 2008.04
  if ( input->slice_set_enable && !img->current_slice_header_flag )
  {
    img->qp = img->slice_set_qp[img->current_slice_set_index];
  }
  if (input->sao_enable)
  {
	  img->slice_sao_on[0] = u_v ( 1, "sao_slice_flag_Y" );
	  img->slice_sao_on[1] = u_v ( 1, "sao_slice_flag_Cb" );
	  img->slice_sao_on[2] = u_v ( 1, "sao_slice_flag_Cr" );
#if  EXTRACT_FULL_SLICE  //2016-5-31 提取完整的参数
    slice_slice_sao_enable[0] = img->slice_sao_on[0];
    slice_slice_sao_enable[1] = img->slice_sao_on[1];
    slice_slice_sao_enable[2] = img->slice_sao_on[2];
#endif
  }
#if  EXTRACT_FULL_SLICE  //2016-5-31 提取完整的参数
  else 
  {
    slice_slice_sao_enable[0] = 0;
    slice_slice_sao_enable[1] = 0;
    slice_slice_sao_enable[2] = 0;
  }
#endif
#if MultiSliceFix
  {
    int i, j;
    for ( i = 0; i < img->width / ( MIN_BLOCK_SIZE ) + 2; i++ )
    {
      for ( j = 0; j < img->height / ( MIN_BLOCK_SIZE ) + 2; j++ )
      {
        img->ipredmode[j][i] = -1;
      }
    }
  }
#endif
}

/*
*************************************************************************
* Function:Error handling procedure. Print error message to stderr and exit
with supplied code.
* Input:text
Error message
* Output:
* Return:
* Attention:
*************************************************************************
*/

void error ( char *text, int code )
{
  fprintf ( stderr, "%s\n", text );
  exit ( code );
}

//Lou
int sign ( int a , int b )
{
  int x;

  x = abs ( a );

  if ( b > 0 )
  {
    return ( x );
  }
  else
  {
    return ( -x );
  }
}

