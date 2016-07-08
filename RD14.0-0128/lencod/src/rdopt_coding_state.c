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
* File name: rdopt_coding_state.c
* Function:
Storing/restoring coding state for
Rate-Distortion optimized mode decision
*
*************************************************************************************
*/


#include <stdlib.h>
#include <math.h>
#include <memory.h>

//!EDIT START <added by lzhang AEC
#include "AEC.h"            // added by lzhang
//!EDIT end <added by lzhang AEC


#include "rdopt_coding_state.h"

/*
*************************************************************************
* Function:delete structure for storing coding state
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void delete_coding_state ( CSptr cs )
{
  if ( cs != NULL )
  {
    if ( cs->bitstream != NULL )
    {
      free ( cs->bitstream );
    }

    //=== coding state structure ===
    if ( cs->encenv !=  NULL )
    {
      free ( cs->encenv );
    }

    delete_contexts_SyntaxInfo ( cs->syn_ctx );

    free ( cs );
    cs = NULL;
  }

}

/*
*************************************************************************
* Function:create structure for storing coding state
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

CSptr create_coding_state ()
{
  CSptr cs;

  //=== coding state structure ===
  if ( ( cs = ( CSptr ) calloc ( 1, sizeof ( CSobj ) ) ) == NULL )
  {
    no_mem_exit ( "init_coding_state: cs" );
  }

  //=== important variables of data partition array ===

  if ( ( cs->bitstream = ( Bitstream* ) calloc ( 1, sizeof ( Bitstream ) ) ) == NULL )
  {
    no_mem_exit ( "init_coding_state: cs->bitstream" );
  }

  //=== important variables of data partition array ===
  cs->no_part = 1;

  if ( ( cs->encenv = ( EncodingEnvironment* ) calloc ( cs->no_part, sizeof ( EncodingEnvironment ) ) ) == NULL )
  {
    no_mem_exit ( "init_coding_state: cs->encenv" );
  }

  cs->syn_ctx = create_contexts_SyntaxInfo ();

  return cs;
}

/*
*************************************************************************
* Function:store coding state (for rd-optimized mode decision)
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void store_coding_state ( CSptr cs )
{
  int         i;
  Bitstream            *bs_src, *bs_dest;
  EncodingEnvironment  *ee_src, *ee_dest;
  SyntaxInfoContexts   *syn_src;  //  = img->currentSlice->syn_ctx;
  SyntaxInfoContexts   *syn_dest; // = cs->syn_ctx;

  syn_src  = img->currentSlice->syn_ctx;
  syn_dest = cs->syn_ctx;

  for ( i = 0; i < 1; i++ )
  {
    bs_src  =  currBitStream;
    bs_dest = cs->bitstream;

    ee_src  = & ( img->currentSlice->partArr[i].ee_AEC );
    ee_dest = & ( cs->encenv   [i] );

    memcpy ( ee_dest, ee_src, sizeof ( EncodingEnvironment ) );

    memcpy ( bs_dest, bs_src, sizeof ( Bitstream ) );
  }

  //=== contexts for binary arithmetic coding ===
  memcpy ( syn_dest, syn_src, sizeof ( SyntaxInfoContexts ) );
}

/*
*************************************************************************
* Function:restore coding state (for rd-optimized mode decision)
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void reset_coding_state ( CSptr cs )
{
  int  i;

  EncodingEnvironment  *ee_src, *ee_dest;
  Bitstream            *bs_src, *bs_dest;

  SyntaxInfoContexts   *syn_dest;// = img->currentSlice->syn_ctx;
  SyntaxInfoContexts   *syn_src  ;//= cs->syn_ctx;

  syn_dest = img->currentSlice->syn_ctx;
  syn_src  = cs->syn_ctx;

  //=== important variables of data partition array ===
  //only one partition for IDR img
  //  for (i = 0; i <(img->currentPicture->idr_flag? 1:cs->no_part); i++)
  for ( i = 0; i < 1; i++ )
  {
    ee_dest = & ( img->currentSlice->partArr[i].ee_AEC );
    ee_src  = & ( cs->encenv   [i] );

    bs_dest = currBitStream;

    bs_src  = & ( cs->bitstream[i] );

    //--- parameters of encoding environments ---
    memcpy ( ee_dest, ee_src, sizeof ( EncodingEnvironment ) );

    memcpy ( bs_dest, bs_src, sizeof ( Bitstream ) );
  }


  //=== contexts for binary arithmetic coding ===
  memcpy ( syn_dest, syn_src, sizeof ( SyntaxInfoContexts ) );
}

