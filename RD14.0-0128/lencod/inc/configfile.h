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

#ifndef _CONFIGFILE_H_
#define _CONFIGFILE_H_

typedef struct
{
  char *TokenName;
  void *Place;
  int Type;
} Mapping;

InputParameters configinput;

#ifdef INCLUDED_BY_CONFIGFILE_C

Mapping Map[] =
{
  {"MaxSizeInBit",             &configinput.g_uiMaxSizeInBit,        0},
  {"ProfileID",                &configinput.profile_id,              0},
  {"LevelID",                  &configinput.level_id,                0},
  {"IntraPeriod",              &configinput.intra_period,            0},
  {"VECPeriod",                &configinput.vec_period,              0},
  {"SeqHeaderPeriod",          &configinput.seqheader_period,        0},
  {"FramesToBeEncoded",        &configinput.no_frames,               0},
  {"QPIFrame",                 &configinput.qpI,                     0},
  {"QPPFrame",                 &configinput.qpP,                     0},
  {"FrameSkip",                &configinput.jumpd_all,               0},
  {"UseHadamard",              &configinput.hadamard,                0},
  {"FME",                      &configinput.usefme,                  0},
  {"SearchRange",              &configinput.search_range,            0},
  {"NumberReferenceFrames",    &configinput.no_multpred,             0},
  {"SourceWidth",              &configinput.img_width,               0},
  {"SourceHeight",             &configinput.img_height,              0},
#if M3472
  {"TDMode",                   &configinput.TD_mode,                 0},
  {"ViewPackingMode",          &configinput.view_packing_mode,       0},
  {"ViewReverse",              &configinput.view_reverse,            0},
#endif
  {"InputFile",                &configinput.infile,                  1},
#if ROI_M3264
  {"InputROIDataFile",         &configinput.infile_data,             1},
  {"ROICoding",                &configinput.ROI_Coding,              0},
#endif  
#if ENC_FRAME_SKIP
  {"FrameSkipNums",            &configinput.infile_frameskip,        0},
#else
  {"InputHeaderLength",        &configinput.infile_header,           0},
#endif
  {"OutputFile",               &configinput.outfile,                 1},
  {"ReconFile",                &configinput.ReconFile,               1},
  {"TraceFile",                &configinput.TraceFile,               1},
#if M3480_TEMPORAL_SCALABLE
  {"TemporalScalableFlag",     &configinput.TemporalScalableFlag,    0},
#endif
  {"FFRAMEEnable",             &configinput.fframe_enabled,          0},
  {"DHPEnable",	               &configinput.dhp_enabled,             0},
#if MH_PSKIP_NEW
  {"MHPSKIPEnable",            &configinput.b_mhpskip_enabled,       0},
#endif

#if !D20141230_BUG_FIX
  {"DMHEnable",                &configinput.b_dmh_enabled,           0},
#endif 

  {"WSMEnable",                &configinput.wsm_enabled,             0},
  {"NumberBFrames",            &configinput.successive_Bframe_all,   0},
  {"HierarchicalCoding",       &configinput.Hierarchical_B,          0},
  {"QPBFrame",                 &configinput.qpB,                     0},
  {"InterSearch16x16",         &configinput.InterSearch16x16,        0},
  {"InterSearch16x8",          &configinput.InterSearch16x8 ,        0},
  {"InterSearch8x16",          &configinput.InterSearch8x16,         0},
  {"InterSearch8x8",           &configinput.InterSearch8x8 ,         0},
  {"InterSearchAMP",           &configinput.InterSearchAMP ,         0},
#if PicExtensionData
  {"PicExtensionData",         &configinput.PicExtentData ,          0},
#endif
  {"RDOptimization",           &configinput.rdopt,                   0},
  {"InterlaceCodingOption",    &configinput.InterlaceCodingOption,   0},
  {"repeat_first_field",       &configinput.repeat_first_field,      0},
  {"top_field_first",          &configinput.top_field_first,         0},
#if INTERLACE_CODING
  {"OutputMergedPicture",      &configinput.output_merged_picture,   0},
#endif
#if AVS2_HDR_HLS
  {"HDRMetaDataExtension",     &configinput.hdr_metadata,            0},
#endif
  {"LoopFilterDisable",        &configinput.loop_filter_disable,     0},
  {"LoopFilterParameter",      &configinput.loop_filter_parameter_flag,     0},
  {"LoopFilterAlphaOffset",    &configinput.alpha_c_offset,          0},
  {"LoopFilterBetaOffset",     &configinput.beta_offset,             0},
  {"SAOEnable",                &configinput.sao_enable,             0},
  {"ALFEnable",                &configinput.alf_enable,             0},
  {"ALF_LowLatencyEncodingEnable",                &configinput.alf_LowLatencyEncodingEnable,             0},

  {"CrossSliceLoopFilter",	   &configinput.crossSliceLoopFilter,    0},

  {"Progressive_sequence",     &configinput.progressive_sequence,    0},
  {"Progressive_frame",        &configinput.progressive_frame,       0},
  {"NumberOfLCUsInSlice",      &configinput.slice_row_nr,            0},
  {"SliceParameter",           &configinput.slice_parameter,         0},
  {"FrameRate",                &configinput.frame_rate_code,         0},
  {"ChromaFormat",             &configinput.chroma_format,           0},
  {"YUVStructure",             &configinput.yuv_structure,           0},

  // Adaptive frequency weighting quantization
#if FREQUENCY_WEIGHTING_QUANTIZATION
  {"WQEnable",                 &configinput.WQEnable,               0},
  {"SeqWQM",                   &configinput.SeqWQM,                 0},
  {"SeqWQFile",                &configinput.SeqWQFile,              1},

  {"PicWQEnable",              &configinput.PicWQEnable,            0},
  {"WQParam",                  &configinput.WQParam,                0},
  {"WQModel",                  &configinput.WQModel,                0},
  {"WeightParamDetailed",      &configinput.WeightParamDetailed,    1},
  {"WeightParamUnDetailed",    &configinput.WeightParamUnDetailed,  1},
  //{"MBAdaptQuant",            &configinput.MBAdaptQuant,  0},
  //#if CHROMA_DELTA_QP
  {"ChromaDeltaQPDisable",     &configinput.chroma_quant_param_disable, 0},
  {"ChromaDeltaU",             &configinput.chroma_quant_param_delta_u, 0},
  {"ChromaDeltaV",             &configinput.chroma_quant_param_delta_v, 0},
  //#endif
  {"PicWQDataIndex",           &configinput.PicWQDataIndex, 0},
  {"PicWQFile",                &configinput.PicWQFile, 1},
#endif


  {"SliceSetEnable",           &configinput.slice_set_enable,        0},
  {"SliceSetConfig",           &configinput.slice_set_config,        1},

  {"RDOQEnable",               &configinput.use_rdoq,                0},
  {"LambdaFactor",             &configinput.lambda_factor_rdoq,      0},
  {"LambdaFactorP",            &configinput.lambda_factor_rdoq_p,    0},
  {"LambdaFactorB",            &configinput.lambda_factor_rdoq_b,    0},

  {"PMVREnable",               &configinput.b_pmvr_enabled,          0},
  {"NSQT",                     &configinput.useNSQT,                 0},
#if MB_DQP
  {"DeltaQP",                  &configinput.useDQP,                  0},
#endif
  {"SDIP",                     &configinput.useSDIP,                 0},
#if REFINED_QP
  {"RefineQP",                 &configinput.use_refineQP,            0},
#endif
  {"OutPutEncPic",             &configinput.output_enc_pic,          0},
#ifdef AVS2_S2_BGLONGTERM
  {"BackgroundEnable",         &configinput.bg_enable,               0},
  {"BGFileName",               &configinput.bg_file_name,            1},
  {"BGInputNumber",            &configinput.bg_input_number,         0},
  {"BackgroundPeriod",         &configinput.bg_period,               0},
  {"ModelNumber",              &configinput.bg_model_number,         0},
  {"BackgroundQP",             &configinput.bg_qp,                   0},
  {"ModelMethod",              &configinput.bg_model_method,         0},
#if AVS2_S2_FASTMODEDECISION
  {"BGFastMode",               &configinput.bg_fast_mode,            1},
#endif
#endif
  {"SECTEnable",               &configinput.b_secT_enabled,           0},
#if EXTEND_BD
  {"SampleBitDepth",           &configinput.sample_bit_depth,         0},
  {"InputSampleBitDepth",      &configinput.input_sample_bit_depth,   0},
#endif
#ifdef TDRDO
  {"TDRDOEnable",              &configinput.TDEnable,	             0},
#endif
#ifdef AQPO
  {"AQPOEnable",               &configinput.AQPEnable,	             0},
#endif
#ifdef RATECONTROL
  {"RateControl",			   &configinput.EncControl,              0},
  {"TargetBitRate",			   &configinput.TargetBitRate,           0},
  {"RCInitialQP",              &configinput.ECInitialQP,		     0},
#endif
#ifdef MD5
  {"MD5Enable",                &configinput.MD5Enable,	             0},
#endif
  {NULL,                       NULL,                                 -1}
};

#endif

#ifndef INCLUDED_BY_CONFIGFILE_C
extern Mapping Map[];
#endif

void Configure ( int ac, char *av[] );
void SliceSetConfigure();
void DecideMvRange();

#endif

