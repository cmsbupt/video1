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
#include "../../lcommon/inc/commonStructures.h"
void  init_contexts ();

SyntaxInfoContexts* create_contexts_SyntaxInfo ( void );

void delete_contexts_SyntaxInfo ( SyntaxInfoContexts *enco_ctx );

void AEC_new_slice();

void readcuTypeInfo ( SyntaxElement *se, DecodingEnvironmentPtr dep_dp, codingUnit *MB, int uiPosition );
#if S_FRAME_OPT
void readcuTypeInfo_SFRAME ( SyntaxElement *se, DecodingEnvironmentPtr dep_dp, codingUnit *MB, int uiPosition );
#endif 
void readcuTypeInfo_SDIP ( SyntaxElement *se, DecodingEnvironmentPtr dep_dp,codingUnit *currMB,int uiPosition );
void readPdir ( SyntaxElement *se, DecodingEnvironmentPtr dep_dp, codingUnit *MB, int uiPosition );
void readPdir_dhp ( SyntaxElement *se, DecodingEnvironmentPtr dep_dp, codingUnit *MB, int uiPosition );
void readB8TypeInfo_dhp ( SyntaxElement *se, DecodingEnvironmentPtr dep_dp, codingUnit *MB, int uiPosition );
void readB8TypeInfo ( SyntaxElement *se, DecodingEnvironmentPtr dep_dp, codingUnit *MB, int uiPosition );
void readIntraPredMode ( SyntaxElement *se, DecodingEnvironmentPtr dep_dp, codingUnit *MB, int uiPosition );
void readRefFrame ( SyntaxElement *se, DecodingEnvironmentPtr dep_dp, codingUnit *MB, int uiPosition );
void readDmhMode ( SyntaxElement *se, DecodingEnvironmentPtr dep_dp, codingUnit *MB, int uiPosition );
void readMVD ( SyntaxElement *se, DecodingEnvironmentPtr dep_dp, codingUnit *MB, int uiPosition );

#ifdef AVS2_S2_SBD
int readBRPFlag ( int uiBitSize );
void readBRPFlag_AEC ( SyntaxElement *se, DecodingEnvironmentPtr dep_dp, codingUnit *MB, int uiPosition );
#endif

#if WEIGHTED_SKIP
void readWPM ( SyntaxElement *se, DecodingEnvironmentPtr dep_dp, codingUnit *MB, int uiPosition );
#endif
void read_b_dir_skip( SyntaxElement *se, DecodingEnvironmentPtr dep_dp, codingUnit *MB, int uiPosition );
void read_p_skip_mode( SyntaxElement *se, DecodingEnvironmentPtr dep_dp, codingUnit *MB, int uiPosition );
void readCBP ( SyntaxElement *se, DecodingEnvironmentPtr dep_dp, codingUnit *MB, int uiPosition );
void readRunLevelRef ( SyntaxElement *se,  DecodingEnvironmentPtr dep_dp, codingUnit *MB, int uiPosition );
unsigned int biari_decode_symbolW ( DecodingEnvironmentPtr dep, BiContextTypePtr bi_ct1, BiContextTypePtr bi_ct2 );
void readDquant ( SyntaxElement *se, DecodingEnvironmentPtr dep_dp, codingUnit *MB, int uiPosition );
void readCIPredMode ( SyntaxElement *se, DecodingEnvironmentPtr dep_dp, codingUnit *MB, int uiPosition );

int  readSyntaxElement_AEC ( SyntaxElement *se, DataPartition *this_dataPart, codingUnit *MB, int uiPosition );


int readSplitFlag ( int uiBitSize );
void readSplitFlag_AEC ( SyntaxElement *se, DecodingEnvironmentPtr dep_dp, codingUnit *MB, int uiPosition );
#if INTRA_2N
void readTrSize ( SyntaxElement *se, DecodingEnvironmentPtr dep_dp, codingUnit *MB, int uiPosition );
#endif

void readPartBiPredictionFlag ( SyntaxElement *se, DecodingEnvironmentPtr dep_dp, codingUnit *MB, int uiPosition );
void readPMVindex_AEC ( SyntaxElement *se,   DecodingEnvironmentPtr dep_dp, codingUnit *currMB , int uiPosition );
int read_sao_mergeflag( int mergeleft_avail, int mergeup_avail, int uiPositionInPic);
void read_sao_mergeflag_AEC( SyntaxElement* se,  DecodingEnvironmentPtr dep_dp,codingUnit* currMB,int uiPositionInPic);
int read_sao_mode(int uiPositionInPic);
void read_sao_mode_AEC(SyntaxElement* se, DecodingEnvironmentPtr dep_dp,codingUnit* currMB, int uiPositionInPic);
int read_sao_offset(SAOBlkParam* saoBlkParam, int uiPositionInPic, int offsetTh,int* offset);
void read_sao_offset_AEC( SyntaxElement* se, DecodingEnvironmentPtr dep_dp,codingUnit* currMB,int uiPositionInPic);
int read_sao_type(SAOBlkParam* saoBlkParam, int uiPositionInPic);
void read_sao_type_AEC( SyntaxElement* se, DecodingEnvironmentPtr dep_dp,codingUnit* currMB,int uiPositionInPic);
extern int saoclip[NUM_SAO_OFFSET][3];
extern int EO_OFFSET_INV__MAP[];
void readAlfCoeff(ALFParam* Alfp);
unsigned int readAlfLCUCtrl (ImageParameters *img,DecodingEnvironmentPtr dep_dp,int compIdx,int ctx_idx);

#endif