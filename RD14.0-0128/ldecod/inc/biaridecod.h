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

#ifndef _BIARIDECOD_H_
#define _BIARIDECOD_H_


/************************************************************************
* D e f i n i t i o n s
***********************************************************************
*/

void arideco_start_decoding ( DecodingEnvironmentPtr eep, unsigned char *code_buffer, int firstbyte, int *code_len, int slice_type );
int  arideco_bits_read ( DecodingEnvironmentPtr dep );

void biari_init_context_logac ( BiContextTypePtr ctx );

unsigned int biari_decode_symbol ( DecodingEnvironmentPtr dep, BiContextTypePtr bi_ct );
unsigned int biari_decode_symbolW ( DecodingEnvironmentPtr dep, BiContextTypePtr bi_ct1, BiContextTypePtr bi_ct2 );
unsigned int biari_decode_symbol_eq_prob ( DecodingEnvironmentPtr dep );
unsigned int biari_decode_final ( DecodingEnvironmentPtr dep );
#if Decoder_BYPASS_Final
void biari_decode_read(DecodingEnvironmentPtr dep);
#endif
#endif  // BIARIDECOD_H_

