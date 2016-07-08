/* The copyright in this software is being made available under the BSD
* License, included below. This software may be subject to other third party
* and contributor rights, including patent rights, and no such rights are
* granted under this license.
*
* Copyright (c) 2002-2016, Audio Video coding Standard Workgroup of China
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*  * Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*  * Neither the name of Audio Video coding Standard Workgroup of China
*    nor the names of its contributors maybe used to endorse or promote products
*    derived from this software without
*    specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
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
#include "pos_info.h"
// Adaptive frequency weighting quantization
#if FREQUENCY_WEIGHTING_QUANTIZATION
#include "wquant.h"
#endif


CameraParamters CameraParameter, *camera = &CameraParameter;

extern StatBits *StatBitsPtr;

const int  InterlaceSetting[] = { 4, 4, 0, 5, 0, 5, 0, 5, 3, 3, 3, 3, 0,
                                  0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 0, 0, 4, 0
                                };


Video_Dec_data  hd_cc;
Video_Dec_data *hd = &hd_cc;

/*
*************************************************************************
* Function:sequence header
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void SequenceHeader(char *buf, int startcodepos, int length)
{
    int i, j;

    fprintf(stdout, "Sequence Header\n");
    memcpy(currStream->streamBuffer, buf, length);
    currStream->code_len = currStream->bitstream_length = length;
    currStream->read_len = currStream->frame_bitoffset = (startcodepos + 1) * 8;

    input->profile_id           = u_v(8, "profile_id");
    input->level_id             = u_v(8, "level_id");
    hd->progressive_sequence        = u_v(1, "progressive_sequence");
#if INTERLACE_CODING
    hd->is_field_sequence           = u_v(1, "field_coded_sequence");
#endif

#if  EXTRACT_FULL   //2016-5-31 提取完整的参数
  sps_progressive_sequence = progressive_sequence;
  sps_field_coded_sequence = is_field_sequence;
#endif

#if HALF_PIXEL_COMPENSATION || HALF_PIXEL_CHROMA
    img->is_field_sequence = hd->is_field_sequence;
#endif
    hd->horizontal_size = u_v(14, "horizontal_size");
    hd->vertical_size = u_v(14, "vertical_size");
    input->chroma_format               = u_v(2, "chroma_format");
    input->output_bit_depth = 8;
    input->sample_bit_depth = 8;
    hd->sample_precision = 1;
    if (input->profile_id == BASELINE10_PROFILE) { // 10bit profile (0x52)
        input->output_bit_depth = u_v(3, "sample_precision");
        input->output_bit_depth = 6 + (input->output_bit_depth) * 2;
        input->sample_bit_depth = u_v(3, "encoding_precision");
        input->sample_bit_depth = 6 + (input->sample_bit_depth) * 2;
    } else { // other profile
        hd->sample_precision = u_v(3, "sample_precision");
    }
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
    hd->aspect_ratio_information    = u_v(4, "aspect_ratio_information");
    hd->frame_rate_code             = u_v(4, "frame_rate_code");

    hd->bit_rate_lower              = u_v(18, "bit_rate_lower");
    hd->marker_bit                  = u_v(1, "marker bit");
    //CHECKMARKERBIT
    hd->bit_rate_upper              = u_v(12, "bit_rate_upper");
    hd->low_delay                   = u_v(1, "low_delay");
    hd->marker_bit                  = u_v(1, "marker bit");
#if  EXTRACT_FULL   //2016-5-31 提取完整的参数
  sps_aspect_ratio    = hd->aspect_ratio_information;
  sps_frame_rate_code = hd->frame_rate_code;
  sps_bit_rate        = ((hd->bit_rate_upper<<18) + hd->bit_rate_lower);
  sps_low_delay       = hd->low_delay;
#endif
    //CHECKMARKERBIT
#if M3480_TEMPORAL_SCALABLE
    hd->temporal_id_exist_flag      = u_v(1, "temporal_id exist flag");                           //get Extention Flag
#endif
#if  EXTRACT_FULL   //2016-5-31 提取完整的参数
    sps_bbv_buffer_size = 0;
#endif
    sps_bbv_buffer_size =u_v(18, "bbv buffer size");

    input->g_uiMaxSizeInBit     = u_v(3, "Largest Coding Block Size");
#if  EXTRACT_FULL   //2016-5-31 提取完整的参数
    sps_temporal_id_enable = hd->temporal_id_exist_flag;
#endif

#if FREQUENCY_WEIGHTING_QUANTIZATION
    hd->weight_quant_enable_flag            = u_v(1, "weight_quant_enable");

#if  EXTRACT_FULL   //2016-5-31 提取完整的参数
  sps_weight_quant_enable = hd->weight_quant_enable_flag;
  sps_load_seq_weight_quant_data_flag = 0;
#endif

    if (hd->weight_quant_enable_flag) {
        int x, y, sizeId, uiWqMSize;
        int *Seq_WQM;
        hd->load_seq_weight_quant_data_flag        = u_v(1, "load_seq_weight_quant_data_flag");
#if  EXTRACT_FULL   //2016-5-31 提取完整的参数
    sps_load_seq_weight_quant_data_flag = hd->load_seq_weight_quant_data_flag;
#endif

        for (sizeId = 0; sizeId < 2; sizeId++) {
            uiWqMSize = min(1 << (sizeId + 2), 8);
            if (hd->load_seq_weight_quant_data_flag == 1) {
                for (y = 0; y < uiWqMSize; y++) {
                    for (x = 0; x < uiWqMSize; x++) {
                        seq_wq_matrix[sizeId][y * uiWqMSize + x] = ue_v("weight_quant_coeff");
                    }
                }
            } else if (hd->load_seq_weight_quant_data_flag == 0) {
                Seq_WQM = GetDefaultWQM(sizeId);
                for (i = 0; i < (uiWqMSize * uiWqMSize); i++) {
                    seq_wq_matrix[sizeId][i] = Seq_WQM[i];
                }

            }
        }

    }
#endif
    hd->background_picture_enable = 0x01 ^ u_v(1, "background_picture_disable");

#if  EXTRACT_FULL   //2016-5-31 提取完整的参数
  sps_scene_picture_disable = 0x01 ^ hd->background_picture_enable;
#endif

    hd->b_dmh_enabled           = 1;

    hd->b_mhpskip_enabled       = u_v(1, "mhpskip enabled");
    hd->dhp_enabled             = u_v(1, "dhp enabled");
    hd->wsm_enabled             = u_v(1, "wsm enabled");

    img->inter_amp_enable       = u_v(1, "Asymmetric Motion Partitions");
    input->useNSQT              = u_v(1, "useNSQT");
    input->useSDIP              = u_v(1, "useNSIP");

    hd->b_secT_enabled              = u_v(1, "secT enabled");

    input->sao_enable = u_v(1, "SAO Enable Flag");
    input->alf_enable = u_v(1, "ALF Enable Flag");

    hd->b_pmvr_enabled              = u_v(1, "pmvr enabled");

#if  EXTRACT_FULL   //2016-5-31 提取完整的参数
  sps_secondary_transform_enable_flag = hd->b_secT_enabled;
  sps_adaptive_loop_filter_enable = input->alf_enable;
#endif
    u_v(1, "marker bit");

    hd->gop_size = u_v(6, "num_of_RPS");
    for (i = 0; i < hd->gop_size; i++) {
        hd->decod_RPS[i].referd_by_others = u_v(1, "refered by others");
        hd->decod_RPS[i].num_of_ref = u_v(3, "num of reference picture");

        for (j = 0; j < hd->decod_RPS[i].num_of_ref; j++) {
            hd->decod_RPS[i].ref_pic[j] = u_v(6, "delta COI of ref pic");
        }
        hd->decod_RPS[i].num_to_remove = u_v(3, "num of removed picture");

        for (j = 0; j < hd->decod_RPS[i].num_to_remove; j++) {
            hd->decod_RPS[i].remove_pic[j] = u_v(6, "delta COI of removed pic");
        }
        u_v(1, "marker bit");

    }
#if  EXTRACT_FULL   //2016-5-31 提取完整的参数
    sps_output_reorder_delay = 0;
#endif
    if (hd->low_delay == 0) {
        hd->picture_reorder_delay = u_v(5, "picture_reorder_delay");
    }

    input->crossSliceLoopFilter = u_v(1, "Cross Loop Filter Flag");
#if  EXTRACT_FULL   //2016-5-31 提取完整的参数
  sps_num_of_rcs = hd->gop_size;
  sps_output_reorder_delay = hd->picture_reorder_delay;
  sps_cross_slice_loopfilter_enable = input->crossSliceLoopFilter;
#endif

    u_v(2, "reserved bits");

    img->width          = hd->horizontal_size;
    img->height         = hd->vertical_size;
    img->width_cr       = (img->width >> 1);

    if (input->chroma_format == 1) {
        img->height_cr      = (img->height >> 1);
    }

    img->PicWidthInMbs  = img->width / MIN_CU_SIZE;
    img->PicHeightInMbs = img->height / MIN_CU_SIZE;
    img->PicSizeInMbs   = img->PicWidthInMbs * img->PicHeightInMbs;
    img->buf_cycle      = input->buf_cycle + 1;
    img->max_mb_nr      = (img->width * img->height) / (MIN_CU_SIZE * MIN_CU_SIZE);
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
	fprintf(p_sps,"%08X\n", input->g_uiMaxSizeInBit );//lcu_size
	fprintf(p_sps,"%08X\n", hd->horizontal_size);
	fprintf(p_sps,"%08X\n", hd->vertical_size);
	fprintf(p_sps,"%08X\n", input->chroma_format);
  
	fprintf(p_sps,"%08X\n", input->sao_enable);
	fprintf(p_sps,"%08X\n", hd->b_mhpskip_enabled);
	fprintf(p_sps,"%08X\n", hd->dhp_enabled);
	fprintf(p_sps,"%08X\n", hd->wsm_enabled);
  
	fprintf(p_sps,"%08X\n",img->inter_amp_enable);
	fprintf(p_sps,"%08X\n",input->useSDIP);
	fprintf(p_sps,"%08X\n",input->useNSQT);
	fprintf(p_sps,"%08X\n", hd->b_pmvr_enabled);
#endif

#if EXTRACT_D	
	fprintf(p_sps,"%d\n",input->g_uiMaxSizeInBit );//lcu_size
	fprintf(p_sps,"%d\n", hd->horizontal_size);
	fprintf(p_sps,"%d\n", hd->vertical_size);
	fprintf(p_sps,"%d\n",input->chroma_format);
  
	fprintf(p_sps,"%d\n",input->sao_enable);
	fprintf(p_sps,"%d\n", hd->b_mhpskip_enabled);
	fprintf(p_sps,"%d\n", hd->dhp_enabled);
	fprintf(p_sps,"%d\n", hd->wsm_enabled);
  
	fprintf(p_sps,"%d\n",img->inter_amp_enable);
	fprintf(p_sps,"%d\n",input->useSDIP);
	fprintf(p_sps,"%d\n",input->useNSQT);
	fprintf(p_sps,"%d\n", hd->b_pmvr_enabled);
#endif
  if (p_sps != NULL)
  {
    fclose(p_sps);
    p_sps = NULL;
  }
	spsNum++;
	spsNewStart=1;//新的sps开始
#endif

    hc->seq_header ++;
}


void video_edit_code_data(char *buf, int startcodepos, int length)
{
    currStream->frame_bitoffset = currStream->read_len = (startcodepos + 1) * 8;
    currStream->code_len = currStream->bitstream_length = length;
    memcpy(currStream->streamBuffer, buf, length);
    hd->vec_flag = 1;
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

void I_Picture_Header(char *buf, int startcodepos, int length)
{
    int i;
    unsigned int tmpPPSii;
    currStream->frame_bitoffset = currStream->read_len = (startcodepos + 1) * 8;
    currStream->code_len = currStream->bitstream_length = length;
    memcpy(currStream->streamBuffer, buf, length);

    tmpPPSii = u_v(32, "bbv_delay");
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
    pps_bbv_delay = tmpPPSii;
#endif

    hd->time_code_flag       = u_v(1, "time_code_flag");
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
    pps_time_code = 0;//设置初始值
#endif

    if (hd->time_code_flag) {
        hd->time_code        = u_v(24, "time_code");
    }
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
  pps_time_code_flag = hd->time_code_flag;
  pps_time_code = hd->time_code;
#endif
    if (hd->background_picture_enable) {
        hd->background_picture_flag = u_v(1, "background_picture_flag");

        if (hd->background_picture_flag) {
            img->typeb = BACKGROUND_IMG;
        } else {
            img->typeb = 0;
        }

        if (img->typeb == BACKGROUND_IMG) {
            hd->background_picture_output_flag = u_v(1, "background_picture_output_flag");
        }
    }


    {
        img->coding_order         = u_v(8, "coding_order");
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
    pps_decode_order_index = img->coding_order;
    pps_temporal_id = 0;
    pps_picture_output_delay = 0;
#endif



#if M3480_TEMPORAL_SCALABLE
        if (hd->temporal_id_exist_flag == 1) {
            hd->cur_layer = u_v(TEMPORAL_MAXLEVEL_BIT, "temporal_id");
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
            pps_temporal_id = hd->cur_layer;
#endif
        }
#endif
        if (hd->low_delay == 0 && !(hd->background_picture_enable && !hd->background_picture_output_flag)) { //cdp
            hd->displaydelay = ue_v("picture_output_delay");
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
            pps_picture_output_delay = hd->displaydelay;
#endif
        }

    }
    {
        int RPS_idx;// = (img->coding_order-1) % gop_size;
        int predict;
        predict = u_v(1, "use RCS in SPS");
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
        pps_use_rcs_flag = predict;
#endif
        if (predict) {
            RPS_idx = u_v(5, "predict for RCS");
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
            pps_rcs_index = RPS_idx;
#endif
            hd->curr_RPS = hd->decod_RPS[RPS_idx];
        } else {
            //gop size16
            int j;
            hd->curr_RPS.referd_by_others = u_v(1, "refered by others");
            hd->curr_RPS.num_of_ref = u_v(3, "num of reference picture");
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
            pps_rcs_index = sps_num_of_rcs;
#endif
            for (j = 0; j < hd->curr_RPS.num_of_ref; j++) {
                hd->curr_RPS.ref_pic[j] = u_v(6, "delta COI of ref pic");
            }
            hd->curr_RPS.num_to_remove = u_v(3, "num of removed picture");
            for (j = 0; j < hd->curr_RPS.num_to_remove; j++) {
                hd->curr_RPS.remove_pic[j] = u_v(6, "delta COI of removed pic");
            }
            u_v(1, "marker bit");

        }
    }
    //xyji 12.23
    if (hd->low_delay) {
        ue_v("bbv check times");
    }

    hd->progressive_frame = u_v(1, "progressive_frame");

    if (!hd->progressive_frame) {
        img->picture_structure   = u_v(1, "picture_structure");
    } else {
        img->picture_structure = 1;
    }

    hd->top_field_first = u_v(1, "top_field_first");
    hd->repeat_first_field = u_v(1, "repeat_first_field");
#if INTERLACE_CODING
    if (hd->is_field_sequence) {
        hd->is_top_field         = u_v(1, "is_top_field");
#if HALF_PIXEL_COMPENSATION || HALF_PIXEL_CHROMA
        img->is_top_field       = hd->is_top_field;
#endif
        u_v(1, "reserved bit for interlace coding");
    }
#endif
    hd->fixed_picture_qp = u_v(1, "fixed_picture_qp");
    hd->picture_qp = u_v(7, "picture_qp");
    //picture_qp = picture_qp + (8 * (input->sample_bit_depth - 8));

    hd->loop_filter_disable = u_v(1, "loop_filter_disable");

    if (!hd->loop_filter_disable) {
        hd->loop_filter_parameter_flag = u_v(1, "loop_filter_parameter_flag");

        if (hd->loop_filter_parameter_flag) {
            input->alpha_c_offset = se_v("alpha_offset");
            input->beta_offset  = se_v("beta_offset");
        } else { // 20071009
            input->alpha_c_offset = 0;
            input->beta_offset  = 0;
        }
    }
#if CHROMA_DELTA_QP
    hd->chroma_quant_param_disable = u_v(1, "chroma_quant_param_disable");
    if (!hd->chroma_quant_param_disable) {
        hd->chroma_quant_param_delta_u = se_v("chroma_quant_param_delta_cb");
        hd->chroma_quant_param_delta_v = se_v("chroma_quant_param_delta_cr");
    } else {
        hd->chroma_quant_param_delta_u = 0;
        hd->chroma_quant_param_delta_v = 0;
    }
#endif
    // Adaptive frequency weighting quantization
#if FREQUENCY_WEIGHTING_QUANTIZATION

    if (hd->weight_quant_enable_flag) {
        hd->pic_weight_quant_enable_flag = u_v(1, "pic_weight_quant_enable");
        if (hd->pic_weight_quant_enable_flag) {
            hd->pic_weight_quant_data_index = u_v(2, "pic_weight_quant_data_index"); //M2331 2008-04
            if (hd->pic_weight_quant_data_index == 1) {
                hd->mb_adapt_wq_disable = u_v(1, "reserved_bits"); //M2331 2008-04
#if CHROMA_DELTA_QP
#endif
                hd->weighting_quant_param = u_v(2, "weighting_quant_param_index");

                hd->weighting_quant_model = u_v(2, "weighting_quant_model");
                hd->CurrentSceneModel = hd->weighting_quant_model;

                //M2331 2008-04
                if (hd->weighting_quant_param == 1) {
                    for (i = 0; i < 6; i++) {
                        hd->quant_param_undetail[i] = se_v("quant_param_delta_u") + wq_param_default[UNDETAILED][i];
                    }
                }
                if (hd->weighting_quant_param == 2) {
                    for (i = 0; i < 6; i++) {
                        hd->quant_param_detail[i] = se_v("quant_param_delta_d") + wq_param_default[DETAILED][i];
                    }
                }
                //M2331 2008-04
            } //pic_weight_quant_data_index == 1
            else if (hd->pic_weight_quant_data_index == 2) {
                int x, y, sizeId, uiWqMSize;
                for (sizeId = 0; sizeId < 2; sizeId++) {
                    int i = 0;
                    uiWqMSize = min(1 << (sizeId + 2), 8);
                    for (y = 0; y < uiWqMSize; y++) {
                        for (x = 0; x < uiWqMSize; x++) {
                            pic_user_wq_matrix[sizeId][i++] = ue_v("weight_quant_coeff");
                        }
                    }
                }
            }//pic_weight_quant_data_index == 2
        }
    }

#endif

    img->qp                = hd->picture_qp;

    img->type              = I_IMG;

#if EXTRACT
    sprintf(filename, "./%s/PicInfo/picture_%04d.txt", infile, img->coding_order);
    if (p_pps != NULL)
    {
      fclose(p_pps);
      p_pps = NULL;
    }
    if ((p_pps = fopen(filename, "w+")) == 0)
    {
      snprintf(errortext, ET_SIZE, "Error open PicInfo/picture_%04d.txt!", img->coding_order);
      error(errortext, 500);
    }

    sprintf(filename, "./%s/sps/sps_%04d.txt", infile, img->coding_order);
    if (p_pic_sps != NULL)
    {
      fclose(p_pic_sps);
      p_pic_sps = NULL;
    }
    if ((p_pic_sps = fopen(filename, "w+")) == 0)
    {
      snprintf(errortext, ET_SIZE, "Error open sps/sps_%04d.txt!", img->coding_order);
      error(errortext, 500);
    }
#if EXTRACT_PIC_SAO   //2016-5-20 提取整帧的sao最后参数
    sprintf(filename, "./%s/saoinfo/pic_%04d.txt", infile, img->coding_order);
    if ((p_pic_sao = fopen(filename, "w+")) == 0)
    {
      snprintf(errortext, ET_SIZE, "Error open saoinfo/pic_%04d.txt!", img->coding_order);
      error(errortext, 500);
    }
#endif
    sprintf(filename, "./%s/mv_refinfo/picture_%04d.txt", infile, img->coding_order);
    if (p_mv_col != NULL)
    {
      fclose(p_mv_col);
      p_mv_col = NULL;
    }
    if ((p_mv_col = fopen(filename, "w+")) == 0)
    {
      snprintf(errortext, ET_SIZE, "Error open mv_refinfo/picture_%04d.txt!", img->coding_order);
      error(errortext, 500);
    }
#if EXTRACT_08X	
    fprintf(p_pic_sps, "%08X\n", input->g_uiMaxSizeInBit);//lcu_size
    fprintf(p_pic_sps, "%08X\n", hd->horizontal_size);
    fprintf(p_pic_sps, "%08X\n", hd->vertical_size);
    fprintf(p_pic_sps, "%08X\n", input->chroma_format);

    fprintf(p_pic_sps, "%08X\n", input->sao_enable);
    fprintf(p_pic_sps, "%08X\n", hd->b_mhpskip_enabled);
    fprintf(p_pic_sps, "%08X\n", hd->dhp_enabled);
    fprintf(p_pic_sps, "%08X\n", hd->wsm_enabled);

    fprintf(p_pic_sps, "%08X\n", img->inter_amp_enable);
    fprintf(p_pic_sps, "%08X\n", input->useSDIP);
    fprintf(p_pic_sps, "%08X\n", input->useNSQT);
    fprintf(p_pic_sps, "%08X\n", hd->b_pmvr_enabled);
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
    spsNewStart = 0;
#if EXTRACT_08X
    fprintf(p_pps, "%08X\n", spsNewStart);
    fprintf(p_pps, "%08X\n", 0);
#endif

#if EXTRACT_D
    fprintf(p_pps, "spsNewStart %d\n", spsNewStart);
    fprintf(p_pps, "Intra %d\n", 0);
#endif

    sprintf(newdir, "./%s/lcu_info/pic_%04d", infile, img->coding_order);
    //system(newdir);
    mkdir(newdir, 0777);
    printf("creat dir:%s\n", newdir);


#if EXTRACT_lcu_info_debug  //控制lcu debug信息是否打印
    sprintf(newdir, "./%s/lcu_debug/pic_%04d", infile, img->coding_order);
    //system(newdir);
    mkdir(newdir, 0777);
    printf("creat dir:%s\n", newdir);
#endif


    sprintf(newdir, "./%s/lcu_coeff/pic_%04d", infile, img->coding_order);
    //system(newdir);
    mkdir(newdir, 0777);
    printf("creat dir:%s\n", newdir);

    sprintf(newdir, "./%s/lcu_mv/pic_%04d", infile, img->coding_order);
    //system(newdir);
    mkdir(newdir, 0777);
    printf("creat dir:%s\n", newdir);

    sliceNum = -1;
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
void PB_Picture_Header(char *buf, int startcodepos, int length)
{
    int i;
    int tmpPPSi32;
    currStream->frame_bitoffset = currStream->read_len = (startcodepos + 1) * 8;
    currStream->code_len = currStream->bitstream_length = length;
    memcpy(currStream->streamBuffer, buf, length);

    tmpPPSi32=u_v(32, "bbv delay");
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
    pps_bbv_delay = tmpPPSi32;
#endif

    hd->picture_coding_type       = u_v(2, "picture_coding_type");

#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
 	  pps_time_code = 0;//设置初始值
#endif
    if (hd->background_picture_enable && (hd->picture_coding_type == 1 || hd->picture_coding_type == 3)) {
        if (hd->picture_coding_type == 1) {
            hd->background_pred_flag      = u_v(1, "background_pred_flag");
        } else {
            hd->background_pred_flag = 0;
        }
        if (hd->background_pred_flag == 0) {

            hd->background_reference_enable = u_v(1, "background_reference_enable");

        } else {
            hd->background_reference_enable = 0;
        }

    } else {
        hd->background_pred_flag = 0;
        hd->background_reference_enable = 0;
    }



    if (hd->picture_coding_type == 1) {
        img->type = P_IMG;
    } else if (hd->picture_coding_type == 3) {
        img->type = F_IMG;
    } else {
        img->type = B_IMG;
    }


    if (hd->picture_coding_type == 1 && hd->background_pred_flag) {
        img->typeb = BP_IMG;
    } else {
        img->typeb = 0;
    }

#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
  pps_time_code_flag = 0;//帧间帧中 不存在该句法，设置为0
  pps_time_code = 0;//帧间帧中 不存在该句法，设置为0
#endif

    {
        img->coding_order         = u_v(8, "coding_order");
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
        pps_decode_order_index = img->coding_order;
        pps_temporal_id = 0;
        pps_picture_output_delay = 0;
#endif	  


#if M3480_TEMPORAL_SCALABLE
        if (hd->temporal_id_exist_flag == 1) {
            hd->cur_layer = u_v(TEMPORAL_MAXLEVEL_BIT, "temporal_id");
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
      pps_temporal_id = hd->cur_layer;
#endif   
        }
#endif

        if (hd->low_delay == 0) {
            hd->displaydelay = ue_v("displaydelay");
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
      pps_picture_output_delay = hd->displaydelay;
#endif      
        }
    }
    {
        int RPS_idx;// = (img->coding_order-1) % gop_size;
        int predict;
        predict = u_v(1, "use RPS in SPS");
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
    pps_use_rcs_flag = predict;
#endif    
        if (predict) {
            RPS_idx = u_v(5, "predict for RPS");
            hd->curr_RPS = hd->decod_RPS[RPS_idx];
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
      pps_rcs_index = RPS_idx;
#endif      
        } else {
            //gop size16
            int j;
            hd->curr_RPS.referd_by_others = u_v(1, "refered by others");
            hd->curr_RPS.num_of_ref = u_v(3, "num of reference picture");
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
      		  pps_rcs_index = sps_num_of_rcs;
#endif      
            for (j = 0; j < hd->curr_RPS.num_of_ref; j++) {
                hd->curr_RPS.ref_pic[j] = u_v(6, "delta COI of ref pic");
            }
            hd->curr_RPS.num_to_remove = u_v(3, "num of removed picture");
            for (j = 0; j < hd->curr_RPS.num_to_remove; j++) {
                hd->curr_RPS.remove_pic[j] = u_v(6, "delta COI of removed pic");
            }
            u_v(1, "marker bit");

        }
    }
    //xyji 12.23
    if (hd->low_delay) {
        ue_v("bbv check times");
    }

    hd->progressive_frame = u_v(1, "progressive_frame");

    if (!hd->progressive_frame) {
        img->picture_structure = u_v(1, "picture_structure");
    } else {
        img->picture_structure   = 1;
    }

    hd->top_field_first = u_v(1, "top_field_first");
    hd->repeat_first_field = u_v(1, "repeat_first_field");
#if INTERLACE_CODING
    if (hd->is_field_sequence) {
        hd->is_top_field = u_v(1, "is_top_field");
#if HALF_PIXEL_COMPENSATION || HALF_PIXEL_CHROMA
        img->is_top_field = hd->is_top_field;
#endif
        u_v(1, "reserved bit for interlace coding");
    }
#endif

    hd->fixed_picture_qp = u_v(1, "fixed_picture_qp");
    hd->picture_qp = u_v(7, "picture_qp");
    //picture_qp = picture_qp + (8 * (input->sample_bit_depth - 8));

    if (!(hd->picture_coding_type == 2 && img->picture_structure == 1)) {
        u_v(1, "reserved_bit");
    }
    img->random_access_decodable_flag = u_v(1, "random_access_decodable_flag");
    //u_v ( 3, "reserved bits" );

    /*  skip_mode_flag      = u_v(1,"skip_mode_flag");*/  //qyu0822 delete skip_mode_flag
    hd->loop_filter_disable = u_v(1, "loop_filter_disable");

    if (!hd->loop_filter_disable) {
        hd->loop_filter_parameter_flag = u_v(1, "loop_filter_parameter_flag");

        if (hd->loop_filter_parameter_flag) {
            input->alpha_c_offset = se_v("alpha_offset");
            input->beta_offset  = se_v("beta_offset");
        } else {
            input->alpha_c_offset = 0;
            input->beta_offset  = 0;
        }
    }

#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
  pps_loop_filter_disable = hd->loop_filter_disable;
  pps_AlphaCOffset  = input->alpha_c_offset;
  pps_BetaOffset    = input->alpha_c_offset;
#endif
#if CHROMA_DELTA_QP
    hd->chroma_quant_param_disable = u_v(1, "chroma_quant_param_disable");
    if (!hd->chroma_quant_param_disable) {
        hd->chroma_quant_param_delta_u = se_v("chroma_quant_param_delta_cb");
        hd->chroma_quant_param_delta_v = se_v("chroma_quant_param_delta_cr");
    } else {
        hd->chroma_quant_param_delta_u = 0;
        hd->chroma_quant_param_delta_v = 0;
    }
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
  pps_chroma_quant_param_delta_cb = hd->chroma_quant_param_delta_u;
  pps_chroma_quant_param_delta_cr = hd->chroma_quant_param_delta_v;
#endif
#endif
    // Adaptive frequency weighting quantization
#if FREQUENCY_WEIGHTING_QUANTIZATION
    if (hd->weight_quant_enable_flag) {
        hd->pic_weight_quant_enable_flag = u_v(1, "pic_weight_quant_enable");
        if (hd->pic_weight_quant_enable_flag) {
            hd->pic_weight_quant_data_index = u_v(2, "pic_weight_quant_data_index"); //M2331 2008-04
            if (hd->pic_weight_quant_data_index == 1) {
                hd->mb_adapt_wq_disable = u_v(1, "reserved_bits"); //M2331 2008-04
#if CHROMA_DELTA_QP
#endif
                hd->weighting_quant_param = u_v(2, "weighting_quant_param_index");

                hd->weighting_quant_model = u_v(2, "weighting_quant_model");
                hd->CurrentSceneModel = hd->weighting_quant_model;

                //M2331 2008-04
                if (hd->weighting_quant_param == 1) {
                    for (i = 0; i < 6; i++) {
                        hd->quant_param_undetail[i] = se_v("quant_param_delta_u") + wq_param_default[UNDETAILED][i];
                    }
                }
                if (hd->weighting_quant_param == 2) {
                    for (i = 0; i < 6; i++) {
                        hd->quant_param_detail[i] = se_v("quant_param_delta_d") + wq_param_default[DETAILED][i];
                    }
                }
                //M2331 2008-04
            } //pic_weight_quant_data_index == 1
            else if (hd->pic_weight_quant_data_index == 2) {
                int x, y, sizeId, uiWqMSize;
                for (sizeId = 0; sizeId < 2; sizeId++) {
                    int i = 0;
                    uiWqMSize = min(1 << (sizeId + 2), 8);
                    for (y = 0; y < uiWqMSize; y++) {
                        for (x = 0; x < uiWqMSize; x++) {
                            pic_user_wq_matrix[sizeId][i++] = ue_v("weight_quant_coeff");
                        }
                    }
                }
            }//pic_weight_quant_data_index == 2
        }
    }

#endif

    img->qp                = hd->picture_qp;

#if EXTRACT
  if(hd->picture_coding_type == 1 && hd->background_pred_flag)
		RefPicNum=1;//BP_IMG->S_IMG
	else 
		RefPicNum= hd->curr_RPS.num_of_ref;
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
	fprintf(p_pic_sps,"%08X\n", hd->horizontal_size);
	fprintf(p_pic_sps,"%08X\n", hd->vertical_size);
	fprintf(p_pic_sps,"%08X\n",input->chroma_format);
  
	fprintf(p_pic_sps,"%08X\n",input->sao_enable);
	fprintf(p_pic_sps,"%08X\n", hd->b_mhpskip_enabled);
	fprintf(p_pic_sps,"%08X\n", hd->dhp_enabled);
	fprintf(p_pic_sps,"%08X\n", hd->wsm_enabled);
  
	fprintf(p_pic_sps,"%08X\n",img->inter_amp_enable);
	fprintf(p_pic_sps,"%08X\n",input->useSDIP);
	fprintf(p_pic_sps,"%08X\n",input->useNSQT);
	fprintf(p_pic_sps,"%08X\n", hd->b_pmvr_enabled);

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

void extension_data(char *buf, int startcodepos, int length)
{
    int ext_ID;

    memcpy(currStream->streamBuffer, buf, length);
    currStream->code_len = currStream->bitstream_length = length;
    currStream->read_len = currStream->frame_bitoffset = (startcodepos + 1) * 8;

    ext_ID = u_v(4, "extension ID");

    switch (ext_ID) {
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
        OpenPosFile(input->out_datafile);
        roi_parameters_extension();
        break;
#endif
#if AVS2_HDR_HLS
    case MASTERING_DISPLAY_AND_CONTENT_METADATA_EXTENSION:
        break;
#endif
    default:
        printf("reserved extension start code ID %d\n", ext_ID);
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
    hd->video_format      = u_v(3, "video format ID");
    hd->video_range       = u_v(1, "video range");
    hd->color_description = u_v(1, "color description");

    if (hd->color_description) {
        hd->color_primaries          = u_v(8, "color primaries");
        hd->transfer_characteristics = u_v(8, "transfer characteristics");
        hd->matrix_coefficients      = u_v(8, "matrix coefficients");
    }

    hd->display_horizontal_size = u_v(14, "display_horizontaol_size");
    hd->marker_bit              = u_v(1, "marker bit");
    hd->display_vertical_size   = u_v(14, "display_vertical_size");
    hd->TD_mode                 = u_v(1, "3D_mode");

    if (hd->TD_mode) {

        hd->view_packing_mode         = u_v(8, "view_packing_mode");
        hd->view_reverse              = u_v(1, "view_reverse");

    }

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

    if (hd->progressive_sequence == 1) {
        if (hd->repeat_first_field == 1) {
            if (hd->top_field_first == 1) {
                NumberOfFrameCentreOffsets = 3;
            } else {
                NumberOfFrameCentreOffsets = 2;
            }
        } else {
            NumberOfFrameCentreOffsets = 1;
        }
    } else {
        if (img->picture_structure == 1) {
            if (hd->repeat_first_field == 1) {
                NumberOfFrameCentreOffsets = 3;
            } else {
                NumberOfFrameCentreOffsets = 2;
            }
        }
    }

    for (i = 0; i < NumberOfFrameCentreOffsets; i++) {
        hd->frame_centre_horizontal_offset[i] = u_v(16, "frame_centre_horizontal_offset");
        hd->marker_bit                        = u_v(1, "marker_bit");
        hd->frame_centre_vertical_offset[i]   = u_v(16, "frame_centre_vertical_offset");
        hd->marker_bit                        = u_v(1, "marker_bit");
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

void user_data(char *buf, int startcodepos, int length)
{
    int i;
    int user_data;
    memcpy(currStream->streamBuffer, buf, length);

    currStream->code_len = currStream->bitstream_length = length;
    currStream->read_len = currStream->frame_bitoffset = (startcodepos + 1) * 8;

    for (i = 0; i < length - 4; i++) {
        user_data = u_v(8, "user data");
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
    long long int copyright_number;
    hd->copyright_flag       =  u_v(1, "copyright_flag");
    hd->copyright_identifier = u_v(8, "copyright_identifier");
    hd->original_or_copy     =  u_v(1, "original_or_copy");

    /* reserved */
    reserved_data        = u_v(7, "reserved_data");
    marker_bit           = u_v(1, "marker_bit");
    hd->copyright_number_1 = u_v(20, "copyright_number_1");
    marker_bit             = u_v(1, "marker_bit");
    hd->copyright_number_2 = u_v(22, "copyright_number_2");
    marker_bit             = u_v(1, "marker_bit");
    hd->copyright_number_3 = u_v(22, "copyright_number_3");

    copyright_number = (hd->copyright_number_1 << 44) + (hd->copyright_number_2 << 22) + hd->copyright_number_3;

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

    u_v(1, "reserved");
    hd->camera_id               = u_v(7, "camera id");
    hd->marker_bit = u_v(1, "marker bit");
    hd->height_of_image_device  = u_v(22, "height_of_image_device");
    hd->marker_bit = u_v(1, "marker bit");
    hd->focal_length = u_v(22, "focal_length");
    hd->marker_bit = u_v(1, "marker bit");
    hd->f_number = u_v(22, "f_number");
    hd->marker_bit = u_v(1, "marker bit");
    hd->vertical_angle_of_view = u_v(22, "vertical_angle_of_view");
    hd->marker_bit = u_v(1, "marker bit");

    hd->camera_position_x_upper = u_v(16, "camera_position_x_upper");
    hd->marker_bit = u_v(1, "marker bit");
    hd->camera_position_x_lower = u_v(16, "camera_position_x_lower");
    hd->marker_bit = u_v(1, "marker bit");

    hd->camera_position_y_upper = u_v(16, "camera_position_y_upper");
    hd->marker_bit = u_v(1, "marker bit");
    hd->camera_position_y_lower = u_v(16, "camera_position_y_lower");
    hd->marker_bit = u_v(1, "marker bit");

    hd->camera_position_z_upper = u_v(16, "camera_position_z_upper");
    hd->marker_bit = u_v(1, "marker bit");
    hd->camera_position_z_lower = u_v(16, "camera_position_z_lower");
    hd->marker_bit = u_v(1, "marker bit");

    hd->camera_direction_x = u_v(22, "camera_direction_x");
    hd->marker_bit = u_v(1, "marker bit");

    hd->camera_direction_y = u_v(22, "camera_direction_y");
    hd->marker_bit = u_v(1, "marker bit");

    hd->camera_direction_z = u_v(22, "camera_direction_z");
    hd->marker_bit = u_v(1, "marker bit");

    hd->image_plane_vertical_x = u_v(22, "image_plane_vertical_x");
    hd->marker_bit = u_v(1, "marker bit");

    hd->image_plane_vertical_y = u_v(22, "image_plane_vertical_y");
    hd->marker_bit = u_v(1, "marker bit");

    hd->image_plane_vertical_z = u_v(22, "image_plane_vertical_z");
    hd->marker_bit = u_v(1, "marker bit");

    u_v(16, "reserved data");
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
    int max_temporal_level = u_v(3, "max temporal level");
    for (i = 0; i < max_temporal_level; i ++) {
        u_v(4, "fps code per temporal level");
        u_v(18, "bit rate lower");
        u_v(1, "marker bit");
        u_v(12, "bit rate upper");
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
    hd->display_primaries_x0 = u_v(16, "display_primaries_x0");
    u_v(1, "marker bit");
    hd->display_primaries_y0 = u_v(16, "display_primaries_y0");
    u_v(1, "marker bit");
    hd->display_primaries_x1 = u_v(16, "display_primaries_x1");
    u_v(1, "marker bit");
    hd->display_primaries_y1 = u_v(16, "display_primaries_y1");
    u_v(1, "marker bit");
    hd->display_primaries_x2 = u_v(16, "display_primaries_x2");
    u_v(1, "marker bit");
    hd->display_primaries_y2 = u_v(16, "display_primaries_y2");
    u_v(1, "marker bit");

    hd->white_point_x = u_v(16, "white_point_x");
    u_v(1, "marker bit");
    hd->white_point_y = u_v(16, "white_point_y");
    u_v(1, "marker bit");

    hd->max_display_mastering_luminance = u_v(16, "max_display_mastering_luminance");
    u_v(1, "marker bit");
    hd->min_display_mastering_luminance = u_v(16, "min_display_mastering_luminance");
    u_v(1, "marker bit");

    hd->maximum_content_light_level = u_v(16, "maximum_content_light_level");
    u_v(1, "marker bit");
    hd->maximum_frame_average_light_level = u_v(16, "maximum_frame_average_light_level");
    u_v(1, "marker bit");
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

void calc_picture_distance()
{
    // for POC mode 0:
    unsigned int        MaxPicDistanceLsb = (1 << 8);

    if (img->coding_order  <  img->PrevPicDistanceLsb)

    {
        int i, j;

        hc->total_frames++;
        for (i = 0; i < REF_MAXBUFFER; i++) {
            if (fref[i]->imgtr_fwRefDistance >= 0) {
                fref[i]->imgtr_fwRefDistance -= 256;
                fref[i]->imgcoi_ref -= 256;
            }
            for (j = 0; j < 4; j++) {
                fref[i]->ref_poc[j] -= 256;
            }
        }
        for (i = 0; i < outprint.buffer_num; i++) {
            outprint.stdoutdata[i].framenum -= 256;
            outprint.stdoutdata[i].tr -= 256;
        }

        hd->last_output -= 256;
        hd->curr_IDRtr -= 256;
        hd->curr_IDRcoi -= 256;
        hd->next_IDRtr -= 256;
        hd->next_IDRcoi -= 256;
    }
    if (hd->low_delay == 0) {
        img->tr = img->coding_order + hd->displaydelay - hd->picture_reorder_delay;
    } else {
        img->tr = img->coding_order;
    }


    img->pic_distance = img->tr % 256;
    hc->picture_distance = img->pic_distance;

}

void SliceHeader(char *buf, int startcodepos, int length)
{
    int mb_row;
    int mb_width;

    memcpy(currStream->streamBuffer, buf, length);
    currStream->code_len = currStream->bitstream_length = length;

    currStream->read_len = currStream->frame_bitoffset = (startcodepos) * 8;
    hd->slice_vertical_position              = u_v(8, "slice vertical position");
#if EXTRACT
	etr_slice_vertical_position=hd->slice_vertical_position;
#endif
    if (hd->vertical_size > (144 * (1 << input->g_uiMaxSizeInBit))) {
        hd->slice_vertical_position_extension = u_v(3, "slice vertical position extension");
    }

    if (hd->vertical_size > (144 * (1 << input->g_uiMaxSizeInBit))) {
        mb_row = (hd->slice_vertical_position_extension << 7) + hd->slice_vertical_position;
    } else {
        mb_row = hd->slice_vertical_position;
    }

    hd->slice_horizontal_positon = u_v(8, "slice horizontal position");

#if EXTRACT
	etr_slice_horizontal_position=hd->slice_horizontal_positon;
#endif	
    if (img->width > (255 * (1 << input->g_uiMaxSizeInBit))) {
        hd->slice_horizontal_positon_extension = u_v(2, "slice horizontal position extension");
    }

    mb_width = (hd->horizontal_size + MIN_CU_SIZE - 1) / MIN_CU_SIZE;

    if (!hd->fixed_picture_qp) {
        hd->fixed_slice_qp = u_v(1, "fixed_slice_qp");
        hd->slice_qp = u_v(7, "slice_qp");

        img->qp = hd->slice_qp;
        img->slice_set_qp[img->current_slice_set_index] = hd->slice_qp; //added by mz, 2008.04
#if MB_DQP
        input->useDQP = !hd->fixed_slice_qp;
#endif
    }
#if MB_DQP
    else {
        input->useDQP = 0;
    }
#endif

    if (input->sao_enable) {
        img->slice_sao_on[0] = u_v(1, "sao_slice_flag_Y");
        img->slice_sao_on[1] = u_v(1, "sao_slice_flag_Cb");
        img->slice_sao_on[2] = u_v(1, "sao_slice_flag_Cr");

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

    {
        int i, j;
        for (i = 0; i < img->width / (MIN_BLOCK_SIZE) + 2; i++) {
            for (j = 0; j < img->height / (MIN_BLOCK_SIZE) + 2; j++) {
                img->ipredmode[j][i] = -1;
            }
        }
    }
}

 
//Lou
int sign(int a , int b)
{
    int x;

    x = abs(a);

    if (b > 0) {
        return (x);
    } else {
        return (-x);
    }
}

