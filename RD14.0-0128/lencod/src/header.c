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
#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "header.h"
#include "../../lcommon/inc/defines.h"
#include "../../lcommon/inc/transform.h"
#include "vlc.h"

#if FREQUENCY_WEIGHTING_QUANTIZATION
#include "wquant.h"
#endif

#if ROI_M3264
#include "pos_info.h"
#endif

#define max(a, b)                   ((a) > (b) ? (a) : (b))  //!< Macro returning max value 
#define min(a, b)                   ((a) < (b) ? (a) : (b))  //!< Macro returning min value 

const int  InterlaceSetting[] = { 4, 4, 0, 5, 0, 5, 0, 5, 3, 3, 3, 3, 0,
                                  0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 0, 0, 4, 0
                                };  //rm52k

/*
*************************************************************************
* Function:
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
#if !M3452_TC
int frametotc ( int frame, int dropflag )
#else 
int frametotc ( int frame, int reserve_bit )
#endif 
{
  int fps, pict, sec, minute, hour, tc;

  fps = ( int ) ( img->framerate + 0.5 );
  pict = frame % fps;
  frame = ( frame - pict ) / fps;
  sec = frame % 60;
  frame = ( frame - sec ) / 60;
  minute = frame % 60;
  frame = ( frame - minute ) / 60;
  hour = frame % 24;

#if M3452_TC
  if (fps>=64)
	  pict=(int)(pict*64/fps);
#endif

#if !M3452_TC
  tc = ( dropflag << 23 ) | ( hour << 18 ) | ( minute << 12 ) | ( sec << 6 ) | pict;
#else 
  tc = ( reserve_bit << 23 ) | ( hour << 18 ) | ( minute << 12 ) | ( sec << 6 ) | pict;
#endif 

  return tc;
}

int IsEqualRps(ref_man Rps1, ref_man Rps2)
{
	int i,sum = 0;
	sum += (Rps1.num_of_ref != Rps2.num_of_ref);
	for( i=0; i<Rps1.num_of_ref; i++ )
	{
		sum += (Rps1.ref_pic[i] != Rps2.ref_pic[i]);
	}
	sum += (Rps1.remove_pic != Rps2.remove_pic);
	for( i=0; i<Rps1.num_to_remove; i++ )
	{
		sum += (Rps1.remove_pic[i] != Rps2.remove_pic[i]);
	}
	return (sum==0);
}

#if QUANT_RANGE_FIX_REF
static int get_valid_qp(int i_qp)
{
    int i_max_qp = 63 + 8 * (input->sample_bit_depth - 8);
    return Clip3(0, i_max_qp, i_qp);
}
#endif

/*
*************************************************************************
* Function:
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
int IPictureHeader ( int frame )
{
  Bitstream *bitstream = currBitStream;
  int i;
  int len = 0;
  int tc;
  int time_code_flag;
  int bbv_delay, bbv_delay_extension;

  // Adaptive frequency weighting quantization
#if (FREQUENCY_WEIGHTING_QUANTIZATION&&COUNT_BIT_OVERHEAD)
  int bit_overhead;
#endif

  marker_bit = 1;
  time_code_flag = 0;

#if !INTERLACE_CODING
  interlaceIdx = input->progressive_sequence * 16 + img->progressive_frame * 8 + img->picture_structure * 4
                 + input->top_field_first * 2 + input->repeat_first_field; //rm52k
  CheckInterlaceSetting ( interlaceIdx );                                  //rm52k
#endif

  len += u_v ( 32, "I picture start code", 0x1B3, bitstream );

#if MB_DQP 
  if (input->useDQP)
  {
	  input->fixed_picture_qp = 0;/*lgp*/ 
  } 
  else
  {
	  input->fixed_picture_qp = 1;/*lgp*/ 
  }
#else
  input->fixed_picture_qp = 1;
#endif

  //added by mz, 2008.04
  if ( input->slice_set_enable )
  {
    input->fixed_picture_qp = 0;
  }

  bbv_delay = 0xFFFF;
  bbv_delay_extension = 0xFF;

  //xyji 12.23
  len += u_v ( 32, "bbv delay", bbv_delay, bitstream );

  len += u_v ( 1, "time_code_flag", 0, bitstream );

  if ( time_code_flag )
  {
#if !M3452_TC
    tc = frametotc ( frame, img->dropflag );
#else 
    tc = frametotc ( frame, img->tc_reserve_bit );
#endif 
    len += u_v ( 24, "time_code", tc, bitstream );
  }

#ifdef AVS2_S2_BGLONGTERM
#if AVS2_SCENE_CD
  if(input->bg_enable)
#else
  if(input->profile_id == 0x50 && input->bg_enable)
#endif
  {
	  len += u_v ( 1, "background_picture_flag", img->typeb == BACKGROUND_IMG, bitstream );
	  if(img->typeb == BACKGROUND_IMG)
	  {
		  len += u_v ( 1, "background_picture_output_flag", background_output_flag, bitstream );
#if AVS2_SCENE_GB_CD
		  if(duplicated_gb_flag == 0)
#endif
		  last_background_frame_number = img->number;
	  }
  }
#endif

  {
	  unsigned int ROI_coding_order = coding_order % 256;
	  int displaydelay;
	  if (input->low_delay == 0)
	  {
		  displaydelay = img->tr - coding_order + picture_reorder_delay;//add sixiaohua
	  }
	  len += u_v(8, "coding_order", ROI_coding_order, bitstream);
#if M3480_TEMPORAL_SCALABLE
	  if ( temporal_id_exist_flag == 1 )
	  {
		  if( img->typeb == BACKGROUND_IMG || ( img->type == INTER_IMG && img->typeb == BP_IMG ) )
			  cur_layer = 0;
		  len += u_v( TEMPORAL_MAXLEVEL_BIT, "temporal_id", cur_layer, bitstream );
	  }
#endif
	   if (input->low_delay == 0 && !(input->bg_enable && background_output_flag == 0))//cdp
	  {
		  len += ue_v("picture_output_delay", displaydelay, bitstream);
	  }
  }
  {
    int RPS_idx = (coding_order-1) % gop_size_all;

    int IsEqual = IsEqualRps( curr_RPS, cfg_ref_all[RPS_idx]);

    use_RPSflag = 0;
    if (input->intra_period == 1)
    {
      use_RPSflag = 1;
    }
    len += u_v( 1, "use RCS in SPS", (IsEqual||use_RPSflag), bitstream);
    if (IsEqual||use_RPSflag)
    {
      len += u_v( 5, "predict for RCS", RPS_idx, bitstream);
    }
    else
    {
      int j;
      len += u_v( 1, "refered by others ", curr_RPS.referd_by_others, bitstream);
      len += u_v( 3, "num of reference picture", curr_RPS.num_of_ref, bitstream);
      for ( j=0; j<curr_RPS.num_of_ref; j++)
      {
        len += u_v( 6, "delta COI of ref pic", curr_RPS.ref_pic[j], bitstream );
      }
      len += u_v( 3, "num of removed picture", curr_RPS.num_to_remove, bitstream);
      for ( j=0; j<curr_RPS.num_to_remove; j++)
      {
        len += u_v( 6, "delta COI of removed pic", curr_RPS.remove_pic[j], bitstream );
      }
#if DEMULATE_HLS
      len += u_v ( 1, "marker bit", 1, bitstream );
#endif
    }
    use_RPSflag = 1;
  }

  if ( input->low_delay )
  {
    len += ue_v ( "bbv check times", bbv_check_times, bitstream );
  }

  len += u_v ( 1, "progressive frame", img->progressive_frame, bitstream );

  if ( !img->progressive_frame )
  {
    len += u_v ( 1, "picture_structure", img->picture_structure, bitstream );
  }

  len += u_v ( 1, "top field first", input->top_field_first, bitstream );
  len += u_v ( 1, "repeat first field", input->repeat_first_field, bitstream );
#if INTERLACE_CODING
  if (input->InterlaceCodingOption == 3)
  {
    len += u_v ( 1, "is top field", img->is_top_field, bitstream );
    len += u_v ( 1, "reserved bit for interlace coding", 1, bitstream );
  }
#endif
  len += u_v ( 1, "fixed picture qp", input->fixed_picture_qp, bitstream );

#ifdef AVS2_S2_BGLONGTERM
#if AVS2_SCENE_CD
  if(input->bg_enable)
#else
  if(input->profile_id == 0x50)
#endif
  {
	  if(img->typeb==BACKGROUND_IMG && (background_output_flag==0)
#if AVS2_SCENE_GB_CD
		  && gb_is_ready == 1
#endif
		)
        img->qp = input->bg_qp;											//set GB frame's qp
#if QUANT_RANGE_FIX_REF
      img->qp = get_valid_qp(img->qp);
#endif
	  len += u_v ( 7, "I picture QP", img->qp, bitstream );
  }
  else
  {
#if EXTEND_BD
#if REFINED_QP
    if ( input->use_refineQP )
    {
#if QUANT_RANGE_FIX_REF
      img->qp = get_valid_qp(img->qp);
#endif
      len += u_v ( 7, "I picture QP", img->qp, bitstream );
    }
    else
    {
#if QUANT_RANGE_FIX_REF
      input->qpI = get_valid_qp(input->qpI);
#endif
      len += u_v ( 7, "I picture QP", input->qpI, bitstream );
    }
#else
		len += u_v ( 6, "I picture QP", input->qpI, bitstream );
#endif
#else
	  len += u_v ( 6, "I picture QP", input->qpI, bitstream );
#endif
#if REFINED_QP
    if ( !input->use_refineQP )
    {
#if RATECONTROL
		if(input->EncControl == 0)
#endif
      img->qp = input->qpI;
    }
#else
#if RATECONTROL
	  if(input->EncControl == 0)
#endif
    img->qp = input->qpI;
#endif
  }
#else
#ifdef EXTEND_BD
	len += u_v ( 6, "I picture QP", (input->qpI - (8 * (input->sample_bit_depth - 8))), bitstream );
#else
  len += u_v ( 6, "I picture QP", input->qpI, bitstream );
#endif
#if RATECONTROL
  if(input->EncControl == 0)
#endif
  img->qp = input->qpI;
#endif

  len += u_v ( 1, "loop filter disable", input->loop_filter_disable, bitstream );

  if ( !input->loop_filter_disable )
  {
    len += u_v ( 1, "loop filter parameter flag", input->loop_filter_parameter_flag, bitstream );

    if ( input->loop_filter_parameter_flag )
    {
      len += se_v ( "alpha offset", input->alpha_c_offset, bitstream );
      len += se_v ( "beta offset", input->beta_offset, bitstream );
    }
    else  // set alpha_offset, beta_offset to 0 when loop filter parameters aren't transmitted. encoder issue 
    {
      input->alpha_c_offset = 0;
      input->beta_offset = 0;
    }
  }

  // Adaptive frequency weighting quantization
#if FREQUENCY_WEIGHTING_QUANTIZATION
#if COUNT_BIT_OVERHEAD
  bit_overhead=len;
#endif

#if CHROMA_DELTA_QP
#if CHROMA_DELTA_QP_SYNTEX_OPT
  len+=u_v(1,"chroma_quant_param_disable", input->chroma_quant_param_disable,bitstream);
  if(!input->chroma_quant_param_disable)
  {
	  len+=se_v("chroma_quant_param_delta_cb", input->chroma_quant_param_delta_u,bitstream);
	  len+=se_v("chroma_quant_param_delta_cr", input->chroma_quant_param_delta_v,bitstream);
  }
  else
  {
    input->chroma_quant_param_delta_u = 0;
    input->chroma_quant_param_delta_v = 0;
  }
#endif
#endif

  if(input->WQEnable)
  {
	  len+=u_v(1,"pic_weight_quant_enable",input->PicWQEnable,bitstream);
	  if(input->PicWQEnable)
	  {
		  len+=u_v(2,"pic_weight_quant_data_index",input->PicWQDataIndex, bitstream);

		  if (input->PicWQDataIndex== 1)
		  {
			  len+=u_v(1,"reserved_bits", 0,bitstream); //M2331 2008-04
#if CHROMA_DELTA_QP
#if !CHROMA_DELTA_QP_SYNTEX_OPT
			  len+=u_v(1,"chroma_quant_param_disable", input->chroma_quant_param_disable,bitstream);
			  if(!input->chroma_quant_param_disable)
			  {
				  len+=se_v("chroma_quant_param_delta_cb", input->chroma_quant_param_delta_u,bitstream);
				  len+=se_v("chroma_quant_param_delta_cr", input->chroma_quant_param_delta_v,bitstream);
			  }
#endif
#endif
			  len+=u_v(2,"weighting_quant_param_index",input->WQParam,bitstream);
			  len+=u_v(2,"weighting_quant_model",input->WQModel,bitstream);

			  //M2331 2008-04
			  if((input->WQParam==1)||((input->MBAdaptQuant)&&(input->WQParam==3)))
			  {
				  for(i=0;i<6;i++)
					  len+=se_v("quant_param_delta_u",(wq_param[UNDETAILED][i]-wq_param_default[UNDETAILED][i]),bitstream);
			  }
			  if((input->WQParam==2)||((input->MBAdaptQuant)&&(input->WQParam==3)))
			  {
				  for(i=0;i<6;i++)
					  len+=se_v("quant_param_delta_d",(wq_param[DETAILED][i]-wq_param_default[DETAILED][i]),bitstream);
			  }

			  //M2331 2008-04
		  }//input->PicWQDataIndex== 1
		  else if (input->PicWQDataIndex== 2) 
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
						  len += ue_v( "weight_quant_coeff", pic_user_wq_matrix[sizeId][i++], bitstream );
					  }
				  }
			  }
		  } //input->PicWQDataIndex== 2
	  }
  }
#if COUNT_BIT_OVERHEAD
  g_count_overhead_bit+=len-bit_overhead;
  printf(" QM_overhead I : %8d\n",(len-bit_overhead));
#endif

#endif

#if !M3595_REMOVE_REFERENCE_FLAG
  picture_reference_flag  = 0;
#endif

  return len;
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

int PBPictureHeader()
{
  Bitstream *bitstream = currBitStream;
  int i;
  int len = 0;
  int bbv_delay, bbv_delay_extension;
  // Adaptive frequency weighting quantization
#if (FREQUENCY_WEIGHTING_QUANTIZATION&&COUNT_BIT_OVERHEAD)
  int bit_overhead;
#endif
#if !INTERLACE_CODING
  interlaceIdx = input->progressive_sequence * 16 + img->progressive_frame * 8 + img->picture_structure * 4
                 + input->top_field_first * 2 + input->repeat_first_field; //rm52k
  CheckInterlaceSetting ( interlaceIdx );                                  //rm52k
#endif

  if ( img->type == INTER_IMG )
  {
	  picture_coding_type = 1;
  }
  else if ( img->type == F_IMG )
  {
	  picture_coding_type = 3;
  }
  else
  {
    picture_coding_type = 2;  //add by wuzhongmou
  }

#ifdef AVS2_S2_S
#if AVS2_SCENE_CD
  if(img->type == P_IMG && img->typeb == BP_IMG && input->bg_enable)
#else
  if(img->type == P_IMG && img->typeb == BP_IMG && input->bg_enable && input->profile_id == 0x50)
#endif
	  background_pred_flag = 1;	//S frame picture coding type
	else
	  background_pred_flag = 0;
#endif

#if MB_DQP
  if (input->useDQP)
  {
	  input->fixed_picture_qp = 0;/*lgp*/ 
  } 
  else
  {
	  input->fixed_picture_qp = 1;/*lgp*/ 
  }

#else
  input->fixed_picture_qp = 1;
#endif


  if ( input->slice_set_enable ) //added by mz, 2008.04
  {
    input->fixed_picture_qp = 0;
  }

  if ( img->nb_references == 1 )
  {
#if !M3595_REMOVE_REFERENCE_FLAG
    picture_reference_flag = 1;
#endif 
	img->num_of_references = 1;
  }
  else if ( img->nb_references > 1 )
  {
#if !M3595_REMOVE_REFERENCE_FLAG
    picture_reference_flag = 0;
#endif 
  }

  bbv_delay = 0xFFFF;
  bbv_delay_extension = 0xFF;

  len += u_v ( 24, "start code prefix", 1, bitstream );
  len += u_v ( 8, "PB picture start code", 0xB6, bitstream );
  //xyji 12.23
  len += u_v ( 32, "bbv delay", bbv_delay, bitstream );

  len += u_v ( 2, "picture coding type", picture_coding_type, bitstream );

#ifdef AVS2_S2_BGLONGTERM
#if AVS2_SCENE_CD
  if( input->bg_enable && (picture_coding_type==1 || picture_coding_type == 3))
#else
  if( input->bg_enable && picture_coding_type==1 && input->profile_id == 0x50 )
#endif
  {
#ifdef AVS2_S2_S
#if AVS2_SCENE_CD
	if(picture_coding_type == 1)	//only P can be extended to S
#endif
    len += u_v( 1, "background_pred_flag", background_pred_flag,bitstream );
    if( background_pred_flag == 0 )
    {   
#endif
      len += u_v( 1, "background_reference_enable", background_reference_enable, bitstream );
#ifdef AVS2_S2_S
    }
#endif
  }
#endif

  {
	  unsigned int ROI_coding_order = coding_order % 256;
	  int displaydelay;
	  if (input->low_delay == 0)
	  {
		  displaydelay = img->tr - coding_order + picture_reorder_delay;//add sixiaohua
	  }
	  len += u_v(8, "coding_order", ROI_coding_order, bitstream);
#if M3480_TEMPORAL_SCALABLE
	  if ( temporal_id_exist_flag == 1 )
	  {
		  if( img->typeb == BACKGROUND_IMG || ( img->type == INTER_IMG && img->typeb == BP_IMG ) )
			  cur_layer = 0;
		  len += u_v( TEMPORAL_MAXLEVEL_BIT, "temporal_id", cur_layer, bitstream );
	  }
#endif
	  if (input->low_delay == 0)
	  {
		  len += ue_v("displaydelay", displaydelay, bitstream);
	  }
  }
  {
	  int RPS_idx = (coding_order-1) % gop_size_all;
	  
	  int IsEqual = 1;
	  if (curr_RPS.num_of_ref != cfg_ref_all[RPS_idx].num_of_ref || curr_RPS.num_to_remove != cfg_ref_all[RPS_idx].num_to_remove)
	  {
		  IsEqual = 0;
	  }
	  else
	  {
		  for (i=0; i<curr_RPS.num_of_ref; i++)
		  {
			  if (curr_RPS.ref_pic[i] != cfg_ref_all[RPS_idx].ref_pic[i])
			  {
				  IsEqual = 0;
			  }
		  }
	  }
	  len += u_v( 1, "use RPS in SPS", IsEqual, bitstream);
	  if (IsEqual)
	  {
		  len += u_v( 5, "predict for RPS", RPS_idx, bitstream);
	  }
	  else
	  {
		  int j;
		  len += u_v( 1, "refered by others ", curr_RPS.referd_by_others, bitstream);
		  len += u_v( 3, "num of reference picture", curr_RPS.num_of_ref, bitstream);
		  for ( j=0; j<curr_RPS.num_of_ref; j++)
		  {
			  len += u_v( 6, "delta COI of ref pic", curr_RPS.ref_pic[j], bitstream );
		  }
		  len += u_v( 3, "num of removed picture", curr_RPS.num_to_remove, bitstream);
		  for ( j=0; j<curr_RPS.num_to_remove; j++)
		  {
			  len += u_v( 6, "delta COI of removed pic", curr_RPS.remove_pic[j], bitstream );
		  }
#if DEMULATE_HLS
      len += u_v ( 1, "marker bit", 1, bitstream );
#endif
	  }
  }

  if ( input->low_delay ) //cjw 20070414
  {
    len += ue_v ( "bbv check times", bbv_check_times, bitstream );
  }

  len += u_v ( 1, "progressive frame", img->progressive_frame, bitstream );

  if ( !img->progressive_frame )
  {
    len += u_v ( 1, "picture_structure", img->picture_structure, bitstream );
  }

  len += u_v ( 1, "top field first", input->top_field_first, bitstream );
  len += u_v ( 1, "repeat first field", input->repeat_first_field, bitstream );
#if INTERLACE_CODING
  if (input->InterlaceCodingOption == 3)
  {
    len += u_v ( 1, "is top field", img->is_top_field, bitstream );
    len += u_v ( 1, "reserved bit for interlace coding", 1, bitstream );
  }
#endif
  len += u_v ( 1, "fixed qp", input->fixed_picture_qp, bitstream );

  //rate control
#if QUANT_RANGE_FIX_REF
  img->qp = get_valid_qp(img->qp);
#endif
  if ( img->type == INTER_IMG )
  {
#if EXTEND_BD
		len += u_v ( 7, "P picture QP", img->qp, bitstream );
#else
	  len += u_v ( 6, "P picture QP", img->qp, bitstream );
#endif
  }
  else if ( img->type == F_IMG )
  {
#if EXTEND_BD
		len += u_v ( 7, "F picture QP", img->qp, bitstream );
#else
     len += u_v ( 6, "F picture QP", img->qp, bitstream );
#endif
  }
  else if ( img->type == B_IMG )
  {
#if EXTEND_BD
		len += u_v ( 7, "B picture QP", img->qp, bitstream );
#else
    len += u_v ( 6, "B picture QP", img->qp, bitstream );
#endif
  }

  if ( ! ( picture_coding_type == 2 && img->picture_structure == 1 ) )
  {
#if !M3595_REMOVE_REFERENCE_FLAG
    len += u_v ( 1, "piture reference flag", picture_reference_flag, bitstream );
#else 
    len += u_v ( 1, "reserved_bit", 0, bitstream );
#endif 
  }

#if !M3475_RANDOM_ACCESS
  len += u_v ( 1, "no_forward_reference_flag", 0, bitstream ); // Added by cjw, 20070327
#else 
#if !RANDOM_BUGFIX
  if (img->tr>=curr_IDRtr)
#else 
  if (img->tr>=next_IDRtr)
#endif 
  {
    len += u_v ( 1, "random_access_decodable_flag", 1, bitstream );
  }
  else 
  {
   len += u_v ( 1, "random_access_decodable_flag", 0, bitstream );
  }
#endif 

  //len += u_v ( 3, "reserved bits", 0, bitstream );  // Added by cjw, 20070327


//
//   len+=u_v(1,"skip mode flag",input->skip_mode_flag, bitstream);  //qyu 0822 delete skip_mode_flag

  len += u_v ( 1, "loop filter disable", input->loop_filter_disable, bitstream );

  if ( !input->loop_filter_disable )
  {
    len += u_v ( 1, "loop filter parameter flag", input->loop_filter_parameter_flag, bitstream );

    if ( input->loop_filter_parameter_flag )
    {
      len += se_v ( "alpha offset", input->alpha_c_offset, bitstream );
      len += se_v ( "beta offset", input->beta_offset, bitstream );
    }
    else  // set alpha_offset, beta_offset to 0 when loop filter parameters aren't transmitted. encoder issue 
    {
      input->alpha_c_offset = 0;
      input->beta_offset = 0;
    }
  }

  // Adaptive frequency weighting quantization
#if FREQUENCY_WEIGHTING_QUANTIZATION
#if COUNT_BIT_OVERHEAD
  bit_overhead=len;
#endif

#if CHROMA_DELTA_QP
#if CHROMA_DELTA_QP_SYNTEX_OPT
  len+=u_v(1,"chroma_quant_param_disable", input->chroma_quant_param_disable,bitstream);
  if(!input->chroma_quant_param_disable)
  {
	  len+=se_v("chroma_quant_param_delta_cb", input->chroma_quant_param_delta_u,bitstream);
	  len+=se_v("chroma_quant_param_delta_cr", input->chroma_quant_param_delta_v,bitstream);
  }
  else
  {
    input->chroma_quant_param_delta_u = 0;
    input->chroma_quant_param_delta_v = 0;
  }
#endif
#endif

  if(input->WQEnable)
  {
	  len+=u_v(1,"pic_weight_quant_enable",input->PicWQEnable,bitstream);
	  if(input->PicWQEnable)
	  {
		  len+=u_v(2,"pic_weight_quant_data_index",input->PicWQDataIndex, bitstream);

		  if (input->PicWQDataIndex== 1)
		  {
			  len+=u_v(1,"reserved_bits", 0,bitstream); //M2331 2008-04
#if CHROMA_DELTA_QP
#if !CHROMA_DELTA_QP_SYNTEX_OPT
			  len+=u_v(1,"chroma_quant_param_disable", input->chroma_quant_param_disable,bitstream);
			  if(!input->chroma_quant_param_disable)
			  {
				  len+=se_v("chroma_quant_param_delta_cb", input->chroma_quant_param_delta_u,bitstream);
				  len+=se_v("chroma_quant_param_delta_cr", input->chroma_quant_param_delta_v,bitstream);
			  }
#endif
#endif
			  len+=u_v(2,"weighting_quant_param_index",input->WQParam,bitstream);
			  len+=u_v(2,"weighting_quant_model",input->WQModel,bitstream);

			  //M2331 2008-04
			  if((input->WQParam==1)||((input->MBAdaptQuant)&&(input->WQParam==3)))
			  {
				  for(i=0;i<6;i++)
					  len+=se_v("quant_param_delta_u",(wq_param[UNDETAILED][i]-wq_param_default[UNDETAILED][i]),bitstream);
			  }
			  if((input->WQParam==2)||((input->MBAdaptQuant)&&(input->WQParam==3)))
			  {
				  for(i=0;i<6;i++)
					  len+=se_v("quant_param_delta_d",(wq_param[DETAILED][i]-wq_param_default[DETAILED][i]),bitstream);
			  }

			  //M2331 2008-04
		  }//input->PicWQDataIndex== 1
		  else if (input->PicWQDataIndex== 2) 
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
						  len += ue_v( "weight_quant_coeff", pic_user_wq_matrix[sizeId][i++], bitstream );
					  }
				  }
			  }
		  } //input->PicWQDataIndex== 2
	  }
  }
#if COUNT_BIT_OVERHEAD
  g_count_overhead_bit+=len-bit_overhead;
  printf(" QM_overhead I : %8d\n",(len-bit_overhead));
#endif

#endif

  return len;
}


int SliceHeader ( int slice_nr, int slice_qp )
{
  Bitstream *bitstream = currBitStream;
  int len = 0;
  int mb_row;                  //added by mz, 2008.04
  int slice_set_index = img->mb_data[img->current_mb_nr].slice_set_index; //added by mz, 2008.04
  int slice_header_flag = img->mb_data[img->current_mb_nr].slice_header_flag; //added by mz, 2008.04

  if ( input->slice_set_enable ) //XZHENG, 2008.04
  {
    slice_qp = img->mb_data[img->current_mb_nr].sliceqp;
  }

  len += u_v ( 24, "start code prefix", 1, bitstream );
#if M3198_CU8
#if MultiSliceFix
  len += u_v ( 8, "slice vertical position", (img->current_mb_nr / ( img->width >> MIN_CU_SIZE_IN_BIT )) >> (input->g_uiMaxSizeInBit - MIN_CU_SIZE_IN_BIT), bitstream ); 
#else
  len += u_v ( 8, "slice vertical position", (img->current_mb_nr / ( img->width >> MIN_CU_SIZE_IN_BIT )) >> MIN_CU_SIZE_IN_BIT, bitstream );
#endif
#else
  len += u_v ( 8, "slice vertical position", img->current_mb_nr / ( img->width >> 4 ), bitstream ); //Added by Xiaozhen Zheng, HiSilicon, 20070327
#endif

  if ( img->height > (144*(1<<input->g_uiMaxSizeInBit)) )  //add by wuzhongmou 200612
  {
    len += u_v ( 3, "slice vertical position extension", slice_vertical_position_extension, bitstream );
  }

  if ( img->height > (144*(1<<input->g_uiMaxSizeInBit)) ) //added by mz, 2008.04
  {
#if M3198_CU8
    mb_row = ( slice_vertical_position_extension << 7 ) + img->current_mb_nr / ( img->width >> MIN_CU_SIZE_IN_BIT );
#else
    mb_row = ( slice_vertical_position_extension << 7 ) + img->current_mb_nr / ( img->width >> 4 );
#endif
  }
  else
  {
#if M3198_CU8
    mb_row = img->current_mb_nr / ( img->width >> MIN_CU_SIZE_IN_BIT );
#else
    mb_row = img->current_mb_nr / ( img->width >> 4 );
#endif
  }

#if MultiSliceFix
  len += u_v ( 8, "slice horizontal position", ( img->current_mb_nr - mb_row * ( img->width >> MIN_CU_SIZE_IN_BIT ) >> (input->g_uiMaxSizeInBit - MIN_CU_SIZE_IN_BIT)), bitstream );
#else
  len += u_v ( 8, "slice horizontal position", ( img->current_mb_nr - mb_row * ( img->width >> MIN_CU_SIZE_IN_BIT ) >> MIN_CU_SIZE_IN_BIT), bitstream );
#endif
  if ( img->width > (255*(1<<input->g_uiMaxSizeInBit)) )
  {
    len += u_v ( 2, "slice horizontal position extension", slice_horizontal_position_extension, bitstream );
  }

  if ( input->profile_id == RESERVED_PROFILE_ID )
  {
    if ( input->slice_set_enable ) //added by mz, 2008.04
    {
#if M3198_CU8
      len += u_v ( 8, "slice horizontal position", ( img->current_mb_nr - mb_row * ( img->width >> MIN_CU_SIZE_IN_BIT ) ), bitstream ); //Added by Xiaozhen Zheng, HiSilicon, 20070327
#else
      len += u_v ( 8, "slice horizontal position", ( img->current_mb_nr - mb_row * ( img->width >> 4 ) ), bitstream ); //Added by Xiaozhen Zheng, HiSilicon, 20070327
#endif
      if ( img->width > 4080 )
      {
        len += u_v ( 2, "slice horizontal position extension", slice_horizontal_position_extension, bitstream );
      }

      len += u_v ( 6, "slice set index", slice_set_index, bitstream );
      len += u_v ( 1, "slice header flag", slice_header_flag, bitstream );
    }
  }

  if ( !input->slice_set_enable || ( input->slice_set_enable && slice_header_flag ) ) //added by mz, 2008.04
  {
    if ( !input->fixed_picture_qp && input->slice_set_enable || !input->fixed_picture_qp && !input->slice_set_enable ) //added by mz, 2008.04.07
    {
#if MB_DQP
      len += u_v ( 1, "fixed_slice_qp", !input->useDQP, bitstream );
#else
      len += u_v ( 1, "fixed_slice_qp", 1, bitstream );
#endif

#if !BUGFIX20150607
      len += u_v ( 6, "slice_qp", slice_qp, bitstream );
#else 
      len += u_v ( 7, "slice_qp", slice_qp, bitstream );
#endif 
      img->qp = slice_qp;
    }
  }

  // end for 2004/08

  if ( input->slice_set_enable && !slice_header_flag )
  {
    img->qp = slice_qp;  //XZHENG, 2008.04
  }
  if ( input->sao_enable)
  {
	  len += u_v ( 1, "sao_slice_flag_Y", img->slice_sao_on[0], bitstream );
	  len += u_v ( 1, "sao_slice_flag_Cb", img->slice_sao_on[1], bitstream );
	  len += u_v ( 1, "sao_slice_flag_Cr", img->slice_sao_on[2], bitstream );
  }

  return len;
}

/*
*************************************************************************
* Function: Check the settings of progressive_sequence, progressive_frame,
picture_structure, top_field_first and repeat_first_field
* Input:    Index of the input parameters
* Output:
* Return:
* Attention:
* Author: Xiaozhen ZHENG, 200811, rm52k
*************************************************************************
*/
int CheckInterlaceSetting ( int idx )
{
  //0: OK
  //1: progressive_sequence
  //2: progressive_frame
  //3: picture_structure
  //4: top_field_first
  //5: repeat_first_field

  int status;
  status = InterlaceSetting[idx];

  switch ( status )
  {
  case 0:
    return 1;
  case 1:
    printf ( "Invalid setting of progressive_sequence!\n" );
    return 0;
  case 2:
    printf ( "Invalid setting of progressive_frame!\n" );
    return 0;
  case 3:
    printf ( "Invalid setting of picture_structure!\n" );
    return 0;
  case 4:
    printf ( "Invalid setting of top_field_first!\n" );
    return 0;
  case 5:
    printf ( "Invalid setting of repeat_first_field!\n" );
    return 0;
  default:
    printf ( "Invalid input!\n" );
    return 0;
  }

  return 1;
}

