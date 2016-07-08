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


#define CONTEXT_INI_C

#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include "../../lcommon/inc/defines.h"
#include "global.h"
#include "../../lcommon/inc/commonVariables.h"
#include "AEC.h"


#define BIARI_CTX_INIT1_LOG(jj,ctx)\
  {\
    for (j=0; j<jj; j++)\
    {\
      biari_init_context_logac(&(ctx[j]));\
    }\
  }

#define BIARI_CTX_INIT2_LOG(ii,jj,ctx)\
  {\
    for (i=0; i<ii; i++)\
      for (j=0; j<jj; j++)\
      {\
        biari_init_context_logac(&(ctx[i][j]));\
      }\
  }

void init_contexts ()
{
  SyntaxInfoContexts*  syn = img->currentSlice->syn_ctx;
  int i, j;
  
  BIARI_CTX_INIT1_LOG ( NUM_CuType_CTX,                syn->cuType_contexts );
  BIARI_CTX_INIT1_LOG ( NUM_INTER_DIR_CTX,             syn->pdir_contexts );
  BIARI_CTX_INIT1_LOG ( NUM_AMP_CTX,                   syn->amp_contexts );
  BIARI_CTX_INIT1_LOG ( NUM_B8_TYPE_CTX,               syn->b8_type_contexts );
  BIARI_CTX_INIT1_LOG ( NUM_INTER_DIR_DHP_CTX,             syn->pdir_dhp_contexts );
  BIARI_CTX_INIT1_LOG ( NUM_B8_TYPE_DHP_CTX,               syn->b8_type_dhp_contexts );
  BIARI_CTX_INIT1_LOG ( DIRECTION,                     syn->b_dir_skip_contexts);
  BIARI_CTX_INIT1_LOG ( MH_PSKIP_NUM,                  syn->p_skip_mode_contexts);
#if WPM_ACE_OPT
  BIARI_CTX_INIT1_LOG(WPM_NUM,                         syn->wpm_contexts);
#endif
  BIARI_CTX_INIT2_LOG ( 3,               NUM_MVD_CTX,  syn->mvd_contexts );
  BIARI_CTX_INIT2_LOG ( 2,               NUM_PMV_IDX_CTX,  syn->pmv_idx_contexts );
  BIARI_CTX_INIT1_LOG ( NUM_REF_NO_CTX,                syn->ref_no_contexts );
  BIARI_CTX_INIT1_LOG ( NUM_DELTA_QP_CTX,              syn->delta_qp_contexts );
  BIARI_CTX_INIT1_LOG ( NUM_INTRA_MODE_CTX,            syn->l_intra_mode_contexts );
  BIARI_CTX_INIT1_LOG ( NUM_C_INTRA_MODE_CTX,          syn->c_intra_mode_contexts );
  BIARI_CTX_INIT2_LOG ( 3,               NUM_CBP_CTX,  syn->cbp_contexts );
  BIARI_CTX_INIT2_LOG ( NUM_BLOCK_TYPES, NUM_MAP_CTX,  syn->map_contexts );
  BIARI_CTX_INIT2_LOG ( NUM_BLOCK_TYPES, NUM_LAST_CTX, syn->last_contexts );
  BIARI_CTX_INIT1_LOG ( NUM_SPLIT_CTX,                 syn->split_contexts );
  BIARI_CTX_INIT1_LOG ( NUM_TU_CTX,                    syn->tu_contexts );
  BIARI_CTX_INIT1_LOG ( NUM_SIGCG_CTX,                 syn->sigCG_contexts );
  BIARI_CTX_INIT1_LOG ( NUM_LAST_CG_CTX,               syn->lastCG_contexts );
  BIARI_CTX_INIT1_LOG ( NUM_LAST_POS_CTX,          syn->lastPos_contexts );
  BIARI_CTX_INIT1_LOG ( NUM_SAO_MERGE_FLAG_CTX,               syn->saomergeflag_context ); 
  BIARI_CTX_INIT1_LOG ( NUM_SAO_MODE_CTX,               syn->saomode_context ); 
  BIARI_CTX_INIT1_LOG ( NUM_SAO_OFFSET_CTX,               syn->saooffset_context ); 
  BIARI_CTX_INIT2_LOG ( NUM_ALF_COMPONENT, NUM_ALF_LCU_CTX,   syn->m_cALFLCU_Enable_SCModel );
#ifdef AVS2_S2_SBD
  BIARI_CTX_INIT1_LOG ( NUM_BRP_CTX,                  syn->brp_contexts );
#endif
#if SIMP_MV
  BIARI_CTX_INIT1_LOG ( NUM_INTER_DIR_MIN_CTX,         syn->pdirMin_contexts );
#endif
}

