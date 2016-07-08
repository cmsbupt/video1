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



#ifndef _AEC_H_
#define _AEC_H_

#include "global.h"
#include "../../lcommon/inc/commonVariables.h"
#include "../../lencod/inc/EncAdaptiveLoopFilter.h"
#if MB_DQP
void AEC_new_slice();
void lastdqp2tempdqp();
void tempdqp2lastdqp();
#endif

SyntaxInfoContexts* create_contexts_SyntaxInfo ( void );//ZHAOHAIWU
void delete_contexts_SyntaxInfo ( SyntaxInfoContexts *codec_ctx );//ZHAOHAIWU

void  init_contexts ();
// AEC
void arienco_start_encoding ( EncodingEnvironmentPtr eep, unsigned char *code_buffer, int *code_len, /* int *last_startcode, */int slice_type );
int  arienco_bits_written ( EncodingEnvironmentPtr eep );
void arienco_done_encoding ( EncodingEnvironmentPtr eep );

void biari_init_context_logac ( BiContextTypePtr ctx );
int biari_encode_symbol_est( unsigned char  symbol, BiContextTypePtr bi_ct );
int biari_encode_symbolW_est( unsigned char  symbol, BiContextTypePtr bi_ct1,  BiContextTypePtr bi_ct2 );
int biari_encode_symbol_eq_prob_est( unsigned char  symbol );
#if M3474_LEVEL
int biari_encode_symbol_final_est (  unsigned char symbol );
#endif 
int estRunLevelRef( codingUnit* currMB, int context );
void biari_encode_symbol ( EncodingEnvironmentPtr eep, unsigned char  symbol, BiContextTypePtr bi_ct );
void biari_encode_symbolW ( EncodingEnvironmentPtr eep, unsigned char  symbol, BiContextTypePtr bi_ct1,  BiContextTypePtr bi_ct2 );
void biari_encode_symbol_eq_prob ( EncodingEnvironmentPtr eep, unsigned char  symbol );
void biari_encode_symbol_final ( EncodingEnvironmentPtr eep, unsigned char  symbol );
SyntaxInfoContexts* create_contexts_SyntaxInfo ( void );
void delete_contexts_SyntaxInfo ( SyntaxInfoContexts *enco_ctx );

int  writeSyntaxElement_AEC ( codingUnit* currMB, SyntaxElement *se, DataPartition *this_dataPart, int uiPosition );
void writeCuTypeInfo ( codingUnit*, SyntaxElement *se, EncodingEnvironmentPtr eep_dp, int uiPosition );
#if S_FRAME_OPT
void writeCuTypeInfo_SFRAME ( codingUnit*, SyntaxElement *se, EncodingEnvironmentPtr eep_dp, int uiPosition );
#endif 
void writeCuTypeInfo_SDIP ( codingUnit* currMB, SyntaxElement *se, EncodingEnvironmentPtr eep_dp, int uiPosition );
void writeIntraPredMode ( codingUnit* currMB, SyntaxElement *se, EncodingEnvironmentPtr eep_dp, int uiPosition );
void writeB8TypeInfo ( codingUnit* currMB, SyntaxElement *se, EncodingEnvironmentPtr eep_dp, int uiPosition );
void writeRefFrame ( codingUnit* currMB, SyntaxElement *se, EncodingEnvironmentPtr eep_dp, int uiPosition );
void writeMVD_AEC ( codingUnit* currMB, SyntaxElement *se, EncodingEnvironmentPtr eep_dp, int uiPosition );

#ifdef AVS2_S2_SBD
int writeBRPFlag ( int splitFlag, codingUnit *currMB, int uiBitSize );
void writeBRPFlag_AEC ( codingUnit* currMB, SyntaxElement *se, EncodingEnvironmentPtr eep_dp, int uiPosition );
#endif

void writeDmhMode ( codingUnit* currMB, SyntaxElement *se, EncodingEnvironmentPtr eep_dp, int uiPosition );
void writePMVindex_AEC ( codingUnit* currMB, SyntaxElement *se, EncodingEnvironmentPtr eep_dp, int uiPosition );

void writeTrSize ( codingUnit* currMB, SyntaxElement *se, EncodingEnvironmentPtr eep_dp, int uiPosition );


void writePdir ( codingUnit*, SyntaxElement *se, EncodingEnvironmentPtr eep_dp, int uiPosition );
void writePdir_dhp ( codingUnit*, SyntaxElement *se, EncodingEnvironmentPtr eep_dp, int uiPosition );
void writeB8TypeInfo_dhp ( codingUnit* currMB, SyntaxElement *se, EncodingEnvironmentPtr eep_dp, int uiPosition );
#if WEIGHTED_SKIP
void writeWPM ( codingUnit* currMB, SyntaxElement *se, EncodingEnvironmentPtr eep_dp, int uiPosition );
#endif
void write_b_dir_skip ( codingUnit*, SyntaxElement *se, EncodingEnvironmentPtr eep_dp, int uiPosition );
void write_p_skip_mode ( codingUnit*, SyntaxElement *se, EncodingEnvironmentPtr eep_dp, int uiPosition );
void writeCBP ( codingUnit* currMB, SyntaxElement *se, EncodingEnvironmentPtr eep_dp, int uiPosition );

#if MB_DQP        
void writeDqp ( codingUnit* currMB, SyntaxElement *se, EncodingEnvironmentPtr eep_dp, int uiPosition );
#endif

void writeRunLevelRef ( codingUnit* currMB, SyntaxElement *se, EncodingEnvironmentPtr eep_dp, int uiPosition );

void writeCIPredMode ( codingUnit* currMB, SyntaxElement *se, EncodingEnvironmentPtr eep_dp, int uiPosition );
void writeCbpBit ( int b8, int bit, int cbp, codingUnit* currMB, int inter, EncodingEnvironmentPtr eep_dp, int uiPosition );
int writeSplitFlag ( int splitFlag, codingUnit *currMB, int uiBitSize );
void writeSplitFlag_AEC ( codingUnit* currMB, SyntaxElement *se, EncodingEnvironmentPtr eep_dp, int uiPosition );


void writePartBiPredictionFlag ( codingUnit* currMB, SyntaxElement *se, EncodingEnvironmentPtr eep_dp, int uiPosition );
int write_sao_mergeflag(int mergeleft_avail, int mergeup_avail, SAOBlkParam* saoBlkParam, int uiPositionInPic);
void write_sao_mergeflag_AEC(codingUnit* currMB, SyntaxElement* se, EncodingEnvironmentPtr eep_dp,int uiPositionInPic);
int write_sao_mode(SAOBlkParam* saoBlkParam,int uiPositionInPic);
void write_sao_mode_AEC(codingUnit* currMB, SyntaxElement* se, EncodingEnvironmentPtr eep_dp,int uiPositionInPic);
int write_sao_offset(SAOBlkParam* saoBlkParam,int uiPositionInPic, int offsetTh);
void write_sao_offset_AEC(codingUnit* currMB, SyntaxElement* se, EncodingEnvironmentPtr eep_dp,int uiPositionInPic);
int write_sao_type(SAOBlkParam* saoBlkParam,int uiPositionInPic);
void write_sao_type_AEC(codingUnit* currMB, SyntaxElement* se, EncodingEnvironmentPtr eep_dp,int uiPositionInPic);
extern int saoclip[NUM_SAO_OFFSET][3];
extern int EO_OFFSET_MAP[];
int writeAlfLCUCtrl(int iflag, DataPartition *this_dataPart,int compIdx,int ctx_idx);
void writeAlfCoeff(ALFParam* Alfp);
#endif  // AEC_H

