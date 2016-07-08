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
#include <string.h>

#include "../../lcommon/inc/commonVariables.h"
#include "../../lcommon/inc/block_info.h"
#include "../../lcommon/inc/memalloc.h"
#include "../../lcommon/inc/ComAdaptiveLoopFilter.h"
#include "../../ldecod//inc/DecAdaptiveLoopFilter.h"
#include "global.h"
#include "AEC.h"
#include "biaridecod.h"
#include "math.h"
#include "assert.h"
#define max(a, b)                   ((a) > (b) ? (a) : (b))  //!< Macro returning max value 
#define min(a, b)                   ((a) < (b) ? (a) : (b))  //!< Macro returning min value 
int symbolCount = 0;
#if !LAST_DQP_BUGFIX
int last_dquant = 0;
#endif 

/***********************************************************************
* L O C A L L Y   D E F I N E D   F U N C T I O N   P R O T O T Y P E S
***********************************************************************
*/
unsigned int unary_bin_decode ( DecodingEnvironmentPtr dep_dp, BiContextTypePtr ctx, int ctx_offset );
unsigned int unary_bin_max_decode ( DecodingEnvironmentPtr dep_dp, BiContextTypePtr ctx, int ctx_offset, unsigned int max_symbol );


void AEC_new_slice()
{
  last_dquant = 0;
#if MB_DQP
  lastQP = img->qp;
#endif
}

/*!
************************************************************************
* \brief
*    Allocation of contexts models for the motion info
*    used for arithmetic decoding
*
************************************************************************
*/
SyntaxInfoContexts* create_contexts_SyntaxInfo ( void )
{
  SyntaxInfoContexts *codec_ctx;

  codec_ctx = ( SyntaxInfoContexts* ) calloc ( 1, sizeof ( SyntaxInfoContexts ) );

  if ( codec_ctx == NULL )
  {
    no_mem_exit ( "create_contexts_SyntaxInfo: codec_ctx" );
  }

  return codec_ctx;
}




/*!
************************************************************************
* \brief
*    Frees the memory of the contexts models
*    used for arithmetic decoding of the motion info.
************************************************************************
*/
void delete_contexts_SyntaxInfo ( SyntaxInfoContexts *codec_ctx )
{
  if ( codec_ctx == NULL )
  {
    return;
  }

  free ( codec_ctx );

  return;
}
void readPMVindex_AEC ( SyntaxElement *se,   DecodingEnvironmentPtr dep_dp, codingUnit *currMB , int uiPosition )
{
	int act_sym;
	int counter = 0;
	int act=0;
	SyntaxInfoContexts  *ctx    = img->currentSlice->syn_ctx;
	BiContextTypePtr pCTX =ctx->pmv_idx_contexts[0];

	act_sym = biari_decode_symbol ( dep_dp, pCTX + act );
	counter++;
	act++;
	if ( act_sym == 1)
	{
		se->value1=0;
		return;
	}
	else
	{
		while (counter < se->value2 - 1)
		{
			act_sym = biari_decode_symbol ( dep_dp, pCTX + act );
			if ( act_sym == 1)
			{
				se->value1=counter;
				return;
			} 
			else
			{
				counter++;
				act++;
			}
		}
		se->value1 = counter;
	}
}
void readDmhMode ( SyntaxElement *se, DecodingEnvironmentPtr dep_dp, codingUnit *currMB , int uiPosition )
{
  int b1, b2, b3;
  int d = 0; // distance flag

  SyntaxInfoContexts *ctx = (img->currentSlice)->syn_ctx;
  int ctx_offset = (se->value2 - 3) * 4;
  int iDecMapTab[9] = {0, 3, 4, 7, 8, 1, 2, 5, 6};
  int iSymbol;
  b1 = biari_decode_symbol ( dep_dp, &ctx->mvd_contexts[2][ctx_offset + 0] );
  if (b1 == 0)
  {
	  iSymbol = 0;
  } 
  else
  {
	  b2 = biari_decode_symbol ( dep_dp, &ctx->mvd_contexts[2][ctx_offset + 1] );
	  if(b2 == 0)
	  {
		  b3 = biari_decode_symbol_eq_prob(dep_dp);
		  iSymbol = 1 + b3;
	  }
	  else
	  {
		  b2 = biari_decode_symbol ( dep_dp, &ctx->mvd_contexts[2][ctx_offset + 2] );
		  if (b2 == 0)
		  {
			  b3 = biari_decode_symbol_eq_prob(dep_dp);
			  iSymbol = 3 + b3;
		  } 
		  else
		  {
			  b2 = biari_decode_symbol_eq_prob(dep_dp);
			  b3 = biari_decode_symbol_eq_prob(dep_dp);
			  iSymbol = 5 + (b2 << 1) + b3;
		  }
	  }
  }
  se->value1 = iDecMapTab[iSymbol];


  return;
}



/*!
************************************************************************
* \brief
*    This function is used to arithmetically decode the motion
*    vector data of a B-frame MB.
************************************************************************
*/
void readMVD ( SyntaxElement *se, DecodingEnvironmentPtr dep_dp, codingUnit *currMB , int uiPosition )
{
  int i = img->subblock_x;
  int j = img->subblock_y;
  int a;
  int act_ctx;
  int act_sym;
  int mv_local_err;
  int mv_sign;
  int list_idx = se->value2 & 0x01;
  int k = ( se->value2 >> 1 ); // MVD component
  int sig, l, binary_symbol;
  int golomb_order = 0;

  PixelPos block_a, block_b;

  SyntaxInfoContexts *ctx = img->currentSlice->syn_ctx;

#if !HZC_BUGFIX
  getLuma8x8Neighbour (  i, j, -1,  0, &block_a,uiPosition, img->mb_data[uiPosition].ui_MbBitSize,&img->mb_data[uiPosition],list_idx );
  getLuma8x8Neighbour (  i, j,  0, -1, &block_b,uiPosition, img->mb_data[uiPosition].ui_MbBitSize,&img->mb_data[uiPosition], list_idx );


  if ( block_a.available )
  {
    a = absm ( img->mb_data[block_a.mb_addr].mvd[list_idx][block_a.y][block_a.x][k] );
  }
  else
  {
    a = 0;
  }
#endif 

#if !M3544_CONTEXT_OPT
  if ( ( mv_local_err = a ) < 2 )
  {
    act_ctx = 0;
  }
  else if ( mv_local_err < 16 )
  {
    act_ctx = 1;
  }
  else
  {
    act_ctx = 2;
  }
#else 
    act_ctx = 0;
#endif 

  se->context = act_ctx;

  binary_symbol = 0;

  if ( !biari_decode_symbol ( dep_dp, &ctx->mvd_contexts[k][act_ctx] ) )
  {
    act_sym = 0;
  }
  else if ( !biari_decode_symbol ( dep_dp, &ctx->mvd_contexts[k][3] ) )
  {
    act_sym = 1;
  }
  else if ( !biari_decode_symbol ( dep_dp, &ctx->mvd_contexts[k][4] ) )
  {
    act_sym = 2;
  }
  else if ( !biari_decode_symbol_eq_prob ( dep_dp ) ) //1110
  {
    act_sym = 0;

    do
    {
      l = biari_decode_symbol_eq_prob ( dep_dp );

      if ( l == 0 )
      {
        act_sym += ( 1 << golomb_order );
        golomb_order++;
      }
    }
    while ( l != 1 );

    while ( golomb_order-- )
    {
      //next binary part
      sig = biari_decode_symbol_eq_prob ( dep_dp );

      if ( sig == 1 )
      {
        binary_symbol |= ( 1 << golomb_order );
      }
    }

    act_sym += binary_symbol;

    act_sym = 3 + act_sym * 2;

  }
  else //1111
  {
    act_sym = 0;

    do
    {
      l = biari_decode_symbol_eq_prob ( dep_dp );

      if ( l == 0 )
      {
        act_sym += ( 1 << golomb_order );
        golomb_order++;
      }
    }
    while ( l != 1 );

    while ( golomb_order-- )
    {
      //next binary part
      sig = biari_decode_symbol_eq_prob ( dep_dp );

      if ( sig == 1 )
      {
        binary_symbol |= ( 1 << golomb_order );
      }
    }

    act_sym += binary_symbol;
    act_sym = 4 + act_sym * 2;

  }



  if ( act_sym != 0 )
  {
    mv_sign = biari_decode_symbol_eq_prob ( dep_dp );
    act_sym = ( mv_sign == 0 ) ? act_sym : -act_sym;
  }

  se->value1 = act_sym;

#if TRACE
  fprintf ( p_trace, "@%d %s\t\t\t%d \n", symbolCount++, se->tracestring, se->value1 );
  fflush ( p_trace );
#endif
}

/*!
************************************************************************
* \brief
*    This function is used to read the difference mode flag.
************************************************************************
*/
#ifdef AVS2_S2_SBD
int readBRPFlag ( int uiBitSize )
{
  Slice *currSlice    = img->currentSlice;
  DataPartition *dP;
  SyntaxElement currSE;

  dP = & ( currSlice->partArr[0] );
  currSE.value2 = uiBitSize;
  currSE.reading = readBRPFlag_AEC;
  dP->readSyntaxElement ( &currSE, dP, NULL, 0 );
#if TRACE
  fprintf ( p_trace, "BrpFlag = %3d\n", currSE.value1 );
  fflush ( p_trace );
#endif

  return currSE.value1;
}
void readBRPFlag_AEC ( SyntaxElement *se, DecodingEnvironmentPtr dep_dp, codingUnit *MB, int uiPosition )
{
  BiContextTypePtr pCTX;
  int ctx, symbol;
  pCTX = img->currentSlice->syn_ctx->brp_contexts;
  symbol = 0;
  ctx = se->value2;

  if ( biari_decode_symbol ( dep_dp, pCTX + ctx ) == 0 )
  {
    symbol = 1;
  }
  else
  {
    symbol = 0;
  }

  se->value1 = symbol;
}
#endif

/*!
************************************************************************
* \brief
*    This function is used to arithmetically decode the 8x8 block type.
************************************************************************
*/

#if WEIGHTED_SKIP
void readWPM ( SyntaxElement *se, DecodingEnvironmentPtr dep_dp, codingUnit *MB, int uiPosition )
{
  int act_sym = 0;
#if WPM_ACE_FIX
  int binIdx = 0;
  BiContextTypePtr pCTX = img->currentSlice->syn_ctx->wpm_contexts;
  while (act_sym < img->num_of_references - 1) {
      if (biari_decode_symbol(dep_dp, pCTX + binIdx)) {
          break;
      }
      act_sym++;
      binIdx = min(binIdx + 1, 2);
  }
#else
#if WPM_ACE_OPT
  if (se->type == SE_WPM1) {
      act_sym = biari_decode_symbol(dep_dp, img->currentSlice->syn_ctx->wpm_contexts);
  } else {
      act_sym = biari_decode_symbol(dep_dp, img->currentSlice->syn_ctx->wpm_contexts + 1);
  }
#else
  act_sym = biari_decode_symbol_eq_prob ( dep_dp );
#endif
#endif
  se->value1 = act_sym;
#if EXTRACT
		EcuInfoInterSyntax[4] = se->value1; //wighted_skip_mode Table73	默认0	
#if EXTRACT_lcu_info_BAC_inter
	    fprintf(p_lcu,"wighted_skip_mode  %d  F skip,direct numRef=%d \n",se->value1,img->num_of_references);//wighted_skip_mode  
#endif	    
#endif	

}
#endif

void read_b_dir_skip( SyntaxElement *se, DecodingEnvironmentPtr dep_dp, codingUnit *currMB, int uiPosition )
{
	SyntaxInfoContexts *ctx = ( img->currentSlice )->syn_ctx;
	BiContextTypePtr pCTX = ctx->b_dir_skip_contexts;
	int act_ctx = 0;
	int act_sym = 0;
	int counter = 0;
	int terminate_bit = 0;

	while ( counter < DIRECTION - 1 && ( terminate_bit = biari_decode_symbol ( dep_dp, pCTX + act_ctx ) ) == 0 )
	{
		act_sym++;
		act_ctx++;
		counter++;
	}
	if ( !terminate_bit )
	{
		act_sym += (!biari_decode_symbol ( dep_dp, pCTX + act_ctx ));
	}

	se->value1 = act_sym;

}

void read_p_skip_mode( SyntaxElement *se, DecodingEnvironmentPtr dep_dp, codingUnit *currMB, int uiPosition )
{
	SyntaxInfoContexts *ctx = ( img->currentSlice )->syn_ctx;
	BiContextTypePtr pCTX = ctx->p_skip_mode_contexts;
	int act_ctx = 0;
	int act_sym = 0;
	int counter = 0;
	int terminate_bit = 0;

	while ( counter < MH_PSKIP_NUM - 1 && ( terminate_bit = biari_decode_symbol ( dep_dp, pCTX + act_ctx ) ) == 0 )
	{
		act_sym++;
		act_ctx++;
		counter++;
	}
	if ( !terminate_bit )
	{
		act_sym += (!biari_decode_symbol ( dep_dp, pCTX + act_ctx ));
	}

	se->value1 = act_sym;

}

#if INTRA_2N
void readTrSize ( SyntaxElement *se, DecodingEnvironmentPtr dep_dp, codingUnit *MB, int uiPosition )
{
  int act_sym;
  int act_ctx;
  BiContextTypePtr pCTX;

  act_ctx = se->value2;
  pCTX    = img->currentSlice->syn_ctx->tu_contexts;
#if !HZC_TR_FLAG
  if(MB->ui_MbBitSize < B64X64_IN_BIT)
#else 
  if(MB->ui_MbBitSize==B8X8_IN_BIT||(( MB->ui_MbBitSize==B32X32_IN_BIT || MB->ui_MbBitSize==B16X16_IN_BIT)&& input->useSDIP))
#endif 
  {
    if(MB->ui_MbBitSize==B32X32_IN_BIT || MB->ui_MbBitSize==B16X16_IN_BIT)
    {
      act_ctx++;
    }
    act_sym = biari_decode_symbol( dep_dp, pCTX + act_ctx );
    se->value1 = act_sym;
  }
  else
  {
    se->value1 = 0;
  }
}
#endif

void readPartBiPredictionFlag ( SyntaxElement *se, DecodingEnvironmentPtr dep_dp, codingUnit *MB, int uiPosition )
{
  int act_sym;
  act_sym = biari_decode_symbol_eq_prob ( dep_dp );
  se->value1 = act_sym;
} 


/*!
************************************************************************
* \brief
*    This function is used to arithmetically decode the 8x8 block type.
************************************************************************
*/
void readB8TypeInfo ( SyntaxElement *se, DecodingEnvironmentPtr dep_dp, codingUnit *MB, int uiPosition )
{
  int act_sym = 0;

  SyntaxInfoContexts *ctx = ( img->currentSlice )->syn_ctx;

  BiContextTypePtr pCTX = ctx->b8_type_contexts;
  int act_ctx;

  if ( biari_decode_symbol ( dep_dp, pCTX + 0 ) )
  {
    act_sym = 2;
    act_ctx = 2;
  }
  else
  {
    act_sym = 0;
    act_ctx = 1;
  }

  if ( biari_decode_symbol ( dep_dp, pCTX + act_ctx ) )
  {
    act_sym ++;
  }

  if(act_sym == 3)
    act_sym = (act_sym << 1) + (biari_decode_symbol ( dep_dp, pCTX + 3 ));

  se->value1 = act_sym;
}
/*!
************************************************************************
* \brief
*    This function is used to arithmetically decode the P8x8 block type.
************************************************************************
*/
void readB8TypeInfo_dhp ( SyntaxElement *se, DecodingEnvironmentPtr dep_dp, codingUnit *MB, int uiPosition )
{
	int act_sym = 0;

	SyntaxInfoContexts *ctx = ( img->currentSlice )->syn_ctx;

	BiContextTypePtr pCTX = ctx->b8_type_dhp_contexts;

	act_sym = biari_decode_symbol ( dep_dp, pCTX );
	se->value1 = act_sym;
}
/*!
************************************************************************
* \brief
*    This function is used to arithmetically decode the codingUnit
*    type info of a given MB.
************************************************************************
*/
#if CU_TYPE_BINARIZATION
void readcuTypeInfo ( SyntaxElement *se, DecodingEnvironmentPtr dep_dp,codingUnit *currMB,int uiPosition )
{
  int act_ctx;
  int act_sym;
  int curr_cuType;
  SyntaxInfoContexts *ctx = ( img->currentSlice )->syn_ctx;

  const int MapPCUType[6] = {0, 9, 1, 2, PNXN, 3};

  const int MapBCUType[7] = {-1, 1, 2, 3, PNXN, 0, 9}; //{0, 1, 2, 3, 4, 10, 9};
  BiContextTypePtr pCTX = ctx->cuType_contexts;
  BiContextTypePtr pAMPCTX = ctx->amp_contexts;

  int  binidx = 0,symbol;


  int max_bit = 0;

  if ( img->type != I_IMG )   // INTRA-frame
  {
    act_ctx = 0;
    act_sym = 0;

    max_bit = img->type == B_IMG? 6:5 ;



    while(1)
    {
      if (binidx==4&&currMB->ui_MbBitSize!=4)
      {
        symbol = biari_decode_final ( dep_dp );
      }
      else 
      {
        symbol=biari_decode_symbol ( dep_dp, pCTX + act_ctx );
      }
      binidx++;



      if ( symbol == 0 )

      {
        act_sym++;
        act_ctx++;

        if ( act_ctx >= 5 )
        {
          act_ctx = 5;
        }

      }
      else
      {
        break;
      }
      if (act_sym>=max_bit)
      {
        break;
      }
    }
    if ( img->type == P_IMG || img->type == F_IMG )
    {
      act_sym =  MapPCUType[act_sym];
    }
    else
    {
      act_sym =  MapBCUType[act_sym];
    }

    curr_cuType = act_sym;

    //for AMP
    if ( img->inter_amp_enable )
    {
      if ( curr_cuType == P2NXN || curr_cuType == PNX2N )
      {
        if ( biari_decode_symbol ( dep_dp, pAMPCTX + 0 ) )
        {
          curr_cuType = curr_cuType * 2 + biari_decode_symbol ( dep_dp, pAMPCTX + 1 );
        }
      }
    }
  }
  else
  {
    curr_cuType = I8MB;
  }



#if CBP_MODIFICATION

  ///////////////////////  Intra 2Nx2N or NxN   ////////////////////////////
  if ( curr_cuType == I8MB )
  {
    int i,j;
    curr_cuType = ( biari_decode_symbol ( dep_dp, img->currentSlice->syn_ctx->tu_contexts+1  )==0 )? I16MB : I8MB;
  }
#endif

  se->value1 = curr_cuType;

#if TRACE
  tracebits2 ( "cuType", 1, curr_cuType );
#endif

}
#else

void readcuTypeInfo ( SyntaxElement *se, DecodingEnvironmentPtr dep_dp,codingUnit *currMB,int uiPosition )
{
  int act_ctx;
  int act_sym;
  int curr_cuType;
  SyntaxInfoContexts *ctx = ( img->currentSlice )->syn_ctx;

#if NEW_AEC_CUTYPE
  //const int MapPCUType[7] = {-1,0, 9, 1, 2, PNXN, 3};
  const int MapPCUType[7] = {-1, 0, 1, 2, 3, PNXN, 9};
#else
  const int MapPCUType[7] = {-1,0, 9, 1, 2, PNXN, 3};
#endif

#if NEW_AEC_CUTYPE
  //	const int MapBCUType[7] = {-1, 1, 2, 3, PNXN, 0, 9}; //{0, 1, 2, 3, 4, 10, 9};
  const int MapBCUType[7] = {-1, 0, 1, 2, 3, PNXN, 9}; //{0, 1, 2, 3, 4, 10, 9};
#else
  const int MapBCUType[7] = {-1, 1, 2, 3, PNXN, 0, 9}; //{0, 1, 2, 3, 4, 10, 9};
#endif

#if SIMP_MV
  const int MapPCUTypeMin[6] = {-1, 0, 1, 2, 3, 9};
  const int MapBCUTypeMin[6] = {-1, 0, 1, 2, 3, 9};
#endif


  BiContextTypePtr pCTX = ctx->cuType_contexts;
  BiContextTypePtr pAMPCTX = ctx->amp_contexts;

  int  binidx = 0,symbol;


  int max_bit = 0;

  if ( img->type != I_IMG )   // INTRA-frame
  {
    act_ctx = 0;
    act_sym = 0;

#if SIMP_MV
		if (currMB->ui_MbBitSize == B8X8_IN_BIT)
			max_bit = 5;
		else
			max_bit = 6;
#else
		max_bit =   6;
#endif

		while(1)
		{
#if NEW_AEC_CUTYPE
			if ((binidx == 5) && (currMB->ui_MbBitSize != MIN_CU_SIZE_IN_BIT))
#else
			if ((( binidx == 4 && img->type == B_IMG ) || ( binidx == 5 && ( img->type == P_IMG || img->type == F_IMG ))) && ( currMB->ui_MbBitSize != MIN_CU_SIZE_IN_BIT ))
#endif
      {
        symbol = biari_decode_final ( dep_dp );
#if EXTRACT_lcu_info_cutype_debug
        //此时的act_sym是码字中连续0的个数
        fprintf(p_lcu_debug, "cu_type_index act_sym=%d act_ctx=%d symbol=%d biari_decode_final\n", act_sym, act_ctx, symbol);//cu_type_index
#endif

      }
      else 
      {

        symbol=biari_decode_symbol ( dep_dp, pCTX + act_ctx );
#if EXTRACT_lcu_info_cutype_debug
        //此时的act_sym是码字中连续0的个数
        fprintf(p_lcu_debug, "cu_type_index act_sym=%d act_ctx=%d symbol=%d\n", act_sym, act_ctx, symbol);//cu_type_index
#endif	

      }
      binidx++;

      if ( symbol == 0 )
      {
        act_sym++;
        act_ctx++;

        if ( act_ctx >= 5 )
        {
          act_ctx = 5;
        }

      }
      else
      {
        break;
      }
      if (act_sym>=max_bit)
      {
        break;
      }
    }//while(1)

#if EXTRACT_lcu_info_BAC
		//此时的act_sym是码字中连续0的个数
	  fprintf(p_lcu,"cu_type_index act_sym=%d\n",act_sym);//cu_type_index
#endif	
#if EXTRACT_lcu_info_BAC_inter
		//此时的act_sym是码字中连续0的个数
		fprintf(p_lcu,"cu_type_index act_sym=%d\n",act_sym);//cu_type_index
#endif	
#if EXTRACT			
		EcuInfoInter[0]= act_sym;//cu_type_index 文档对应
#endif	

    if ((( img->type == F_IMG ))||(( img->type == P_IMG )))
    {
#if SIMP_MV
			if (currMB->ui_MbBitSize == B8X8_IN_BIT)
				act_sym = MapPCUTypeMin[act_sym];
			else
				act_sym = MapPCUType[act_sym];
#else
			act_sym =  MapPCUType[act_sym];
#endif
    }
    else
    {
#if SIMP_MV
      if (currMB->ui_MbBitSize == B8X8_IN_BIT)
        act_sym = MapBCUTypeMin[act_sym];
      else
        act_sym = MapBCUType[act_sym];
#else
      act_sym =  MapBCUType[act_sym];
#endif
    }

    curr_cuType = act_sym;
	
#if EXTRACT_lcu_info_BAC
		//此时的act_sym是码字中连续0的个数
		fprintf(p_lcu,"cu_type_index Map act_sym=%d\n",act_sym);//cu_type_index
#endif

		//下面是解析 shape_of_partion_index 码字对应0:1  1:01 2:00
    //for AMP 
#if EXTRACT
    EcuInfoInter[1]= -1;//shape_of_partion_index 文档对应
		//printf("EcuInfoInter[1]= -1\n");
		//printf("currMB->ui_MbBitSize = %d\n",currMB->ui_MbBitSize );
		//printf("B16X16_IN_BIT = %d\n",B16X16_IN_BIT );
#endif 

#if M3198_CU8
    if ( currMB->ui_MbBitSize >= B16X16_IN_BIT )
    {
#endif
    	if ( img->inter_amp_enable )
	    {
				//printf("if ( currMB->ui_MbBitSize >= B16X16_IN_BIT )\n");
	      if ( curr_cuType == 2 || curr_cuType == 3 )
	      {
#if UNIFY_BINARIZATION
	        if ( !biari_decode_symbol ( dep_dp, pAMPCTX + 0 ) )
	        {
	          curr_cuType = curr_cuType * 2 + (!biari_decode_symbol ( dep_dp, pAMPCTX + 1 ));
#if EXTRACT
	          EcuInfoInter[1]= 1+((curr_cuType&1)==1);//shape_of_partion_index 文档对应
#endif 				  
	        }
#if EXTRACT
	        else{
	          EcuInfoInter[1]= 0;//shape_of_partion_index 文档对应
	        }
#endif		
#else
	        if ( biari_decode_symbol ( dep_dp, pAMPCTX + 0 ) )
	        {
	          curr_cuType = curr_cuType * 2 + biari_decode_symbol ( dep_dp, pAMPCTX + 1 );
	        }
#endif
	      }
	    }
#if M3198_CU8    
    }//if ( currMB->ui_MbBitSize >= B16X16_IN_BIT )
#endif			
#if EXTRACT
		else	
		{
			//printf("if ( currMB->ui_MbBitSize >= B16X16_IN_BIT ) else\n");
			//printf("cuInfoInter[1]= 0\n");
			//cms cu为8x8时 不存在shape_of_partion_index
	  	EcuInfoInter[1]= 0;//shape_of_partion_index 文档对应
		}
#endif			
    curr_cuType = curr_cuType + ( img->type == B_IMG );
  }
  else
  {
    curr_cuType = I8MB;
#if EXTRACT
		//cms cu为8x8时 不存在shape_of_partion_index
    EcuInfoInter[1]= 0;//shape_of_partion_index 文档对应
#endif			
  }

#if EXTRACT_lcu_info_BAC
	fprintf(p_lcu,"cu_type_index  %d\n",curr_cuType);//cu_type_index  
	fprintf(p_lcu,"shape_of_partion_index %d\n",EcuInfoInter[1]);//shape_of_partion_index  
#endif
#if EXTRACT_lcu_info_BAC_inter
  fprintf(p_lcu,"shape_of_partion_index %d\n",EcuInfoInter[1]);//shape_of_partion_index  
#endif


  se->value1 = curr_cuType;

#if TRACE
  tracebits2 ( "cuType", 1, curr_cuType );
#endif

}

#endif

#if S_FRAME_OPT
void readcuTypeInfo_SFRAME ( SyntaxElement *se, DecodingEnvironmentPtr dep_dp,codingUnit *currMB,int uiPosition )
{
	int act_ctx;
	int act_sym;
	int curr_cuType;
	SyntaxInfoContexts *ctx = ( img->currentSlice )->syn_ctx;


	const int MapSCUType[7] = {-1, 0, 9};



	BiContextTypePtr pCTX = ctx->cuType_contexts;


	int  binidx = 0,symbol;


	int max_bit = 0;


		act_ctx = 0;
		act_sym = 0;


	   max_bit = 2;




		while(1)
		{

			symbol=biari_decode_symbol ( dep_dp, pCTX + act_ctx );
			
			binidx++;



			if ( symbol == 0 )
			{
				act_sym++;
				act_ctx++;

			}
			else
			{
				break;
			}
			if (act_sym>=max_bit)
			{
				break;
			}
		}

	     act_sym = MapSCUType[act_sym];


		curr_cuType = act_sym;






	se->value1 = curr_cuType;

#if TRACE
	tracebits2 ( "cuType", 1, curr_cuType );
#endif

}

#endif 
void readcuTypeInfo_SDIP ( SyntaxElement *se, DecodingEnvironmentPtr dep_dp,codingUnit *currMB,int uiPosition )
{
	int act_ctx;

	int curr_cuType;
	SyntaxInfoContexts *ctx = ( img->currentSlice )->syn_ctx;

	//const int MapPCUType[6] = {0, 9, 1, 2, PNXN, 3};

	//const int MapBCUType[7] = {-1, 1, 2, 3, PNXN, 0, 9}; //{0, 1, 2, 3, 4, 10, 9};
	BiContextTypePtr pCTX = ctx->cuType_contexts;
	//BiContextTypePtr pAMPCTX = ctx->amp_contexts;



	int symbol1=0,symbol2=1;
	int max_bit = 0;

	if((currMB->ui_MbBitSize==5||currMB->ui_MbBitSize==4)&&currMB->trans_size==1)
	{
		pCTX = ctx->cuType_contexts;
		act_ctx=0;
		symbol1=biari_decode_symbol ( dep_dp, pCTX + 15 );
#if EXTRACT
		cuInfoIntra[6]= (symbol1==0);//intra_pu_type_index 因为码字与值是相反的，表76
#endif
    if(currMB->ui_MbBitSize==MIN_CU_SIZE_IN_BIT)//这个判断不会为真，因为CU 肯定大于8x8
    {
      if(symbol1)
      {
        symbol2=biari_decode_symbol ( dep_dp, pCTX + 16 );
        if(symbol2)
          curr_cuType = InNxNMB;
        else
          curr_cuType = INxnNMB;
      }
      else
      {
        curr_cuType=I8MB;
      }
    }
    else
    {
      curr_cuType = symbol1==1 ? InNxNMB : INxnNMB;
    }
	}
	else{
		curr_cuType = I8MB;
#if EXTRACT
		cuInfoIntra[6]= 2;//intra_pu_type_index 默认值为2
#endif		
	}
	se->value1 = curr_cuType;

#if TRACE
	tracebits2 ( "cuType", 1, curr_cuType );
#endif
}

void readPdir( SyntaxElement *se,	DecodingEnvironmentPtr dep_dp, codingUnit *currMB,	int uiPosition )
{
  int act_ctx;
  int act_sym;
  int symbol;
  int pdir0,pdir1;
  int new_pdir[4]={3,1,0,2};
  static const int dir2offset[4][4] = {{ 0,  2,  4, 9},
  { 3,  1,  5, 10},
  { 6,  7,  8, 11},
  { 12, 13, 14, 15}
  };
  SyntaxInfoContexts *ctx = ( img->currentSlice )->syn_ctx;

#if SIMP_MV
  BiContextTypePtr pCTX;
#else
  BiContextTypePtr pCTX = ctx->pdir_contexts;
#endif


  int counter = 0;
  int terminate_bit = 0;

#if SIMP_MV
  if ((currMB->cuType >= P2NXN && currMB->cuType <= PVER_RIGHT) && currMB->ui_MbBitSize == B8X8_IN_BIT)
    pCTX = ctx->pdirMin_contexts;
  else
    pCTX = ctx->pdir_contexts;
#endif

  act_ctx = 0;
  act_sym = 0;
  if ( currMB->cuType == P2NX2N )
  {
    while ( counter < 2 && ( terminate_bit = biari_decode_symbol ( dep_dp, pCTX + act_ctx ) ) == 0 )
    {
      act_sym++;
      act_ctx++;
      counter++;
    }
    if ( !terminate_bit )
    {
      act_sym += (!biari_decode_symbol ( dep_dp, pCTX + act_ctx ));
    }
    pdir0=act_sym;
    se->value1 = act_sym;
#if EXTRACT

#if EXTRACT_lcu_info_BAC_inter
	fprintf(p_lcu,"b_pu_type_index  %d 2Nx2N\n",act_sym);//b_pu_type_index  
#endif	
	EcuInfoInterSyntax[2]=act_sym;// 存放句法b_pu_type_index, 把2N、2划分、2划分且cu=8x8的三种情况融合在一起		
#endif	
  }
#if SIMP_MV
  else if ((currMB->cuType >= P2NXN && currMB->cuType <= PVER_RIGHT) && currMB->ui_MbBitSize == B8X8_IN_BIT)
  {
    pdir0 = biari_decode_symbol ( dep_dp, pCTX + act_ctx);
    pdir1 = biari_decode_symbol ( dep_dp, pCTX + act_ctx + 1);
#if EXTRACT
	int symbol_act = (pdir0<<1)+pdir1;
	int symbol_act_out = (pdir0<<1)+pdir1;
	switch(symbol_act)
	{
	case 0:
		symbol_act_out = 3;
		break;
	case 1:
		symbol_act_out = 2;
		break;
	case 2:
		symbol_act_out = 1;
		break;
	case 3:
		symbol_act_out = 0;
		break;
	}

#if EXTRACT_lcu_info_BAC_inter	
    fprintf(p_lcu,"b_pu_type_min_index	%d 2PU miniCU\n",symbol_act_out);//b_pu_type_index  
#endif    
	EcuInfoInterSyntax[2]=symbol_act_out;// 存放句法b_pu_type_index, 把2N、2划分、2划分且cu=8x8的三种情况融合在一起		    
#endif	

    if (pdir1 == 1)
      pdir1 = pdir0;
	else
      pdir1 = 1 - pdir0;


    if (pdir0 == 0)  // BW
      pdir0 = 1;
    else
      pdir0 = 0;

    if (pdir1 == 0)  // BW
      pdir1 = 1;
    else
      pdir1 = 0;
	
    se->value1=dir2offset[pdir0][pdir1];
  }
#endif
  else if (currMB->cuType >= P2NXN || currMB->cuType <= PVER_RIGHT)
  {
    counter = 0;
    terminate_bit = 0;
    act_ctx = 0;
    act_sym = 0;
#if EXTRACT
	int symbol_act_1 = 0;
	int symbol_act_2 = 0;
	int symbol_act_out = 0;
#endif		

    while ( counter < 2 && ( terminate_bit = biari_decode_symbol ( dep_dp, pCTX + act_ctx +4 ) ) == 0 )
    {
      act_sym++;
      act_ctx++;
      counter++;
    }
    if ( !terminate_bit )
    {
      act_sym +=(!biari_decode_symbol ( dep_dp, pCTX + act_ctx +4 ));
    }
    pdir0 = act_sym;
#if EXTRACT
	symbol_act_1=act_sym;//0-3,0对应0-3, 1对应4-7, 2对应8-11,  3对应12-15
#endif	

    act_ctx=8;

    symbol = biari_decode_symbol ( dep_dp, pCTX + act_ctx  );
#if EXTRACT
	symbol_act_2=0;//
#endif		
    if (symbol)
    {
    	pdir1=pdir0;
#if EXTRACT
		symbol_act_2=0;//
#endif			
    }
    else 
    { 
#if EXTRACT
	symbol_act_2++;//
#endif	    
	    switch(pdir0)
	    {
	    
			case 0:
			  act_ctx=9;
			  symbol = biari_decode_symbol ( dep_dp, pCTX + act_ctx  );
			  if (symbol)
			  { pdir1=1;			  
			  }
			  else 
			  {
#if EXTRACT
				symbol_act_2++;//
#endif				  
			    act_ctx=10;
			    symbol = biari_decode_symbol ( dep_dp, pCTX + act_ctx  );
			    if (symbol)
			    { pdir1=2;
			    }
			    else 
			    { pdir1=3;
#if EXTRACT
				symbol_act_2++;//
#endif				
			    }
			  }
			  break;


			case 1:
			  act_ctx=11;
			  symbol = biari_decode_symbol ( dep_dp, pCTX + act_ctx  );
			  if (symbol)
			  { pdir1=0;
			  }
			  else 
			  {
#if EXTRACT
				symbol_act_2++;//
#endif				  
			    act_ctx=12;
			    symbol = biari_decode_symbol ( dep_dp, pCTX + act_ctx  );
			    if (symbol)
			    { pdir1=2;
			    }
			    else 
			    { pdir1=3;
#if EXTRACT
				symbol_act_2++;//
#endif					
			    }
			  }
			  break;

			case 2:
			  act_ctx=13;
			  symbol = biari_decode_symbol ( dep_dp, pCTX + act_ctx  );
			  if (symbol)
			  { pdir1=0;
			  }
			  else 
			  {
#if EXTRACT
				symbol_act_2++;//
#endif				  
			    act_ctx=14;
			    symbol = biari_decode_symbol ( dep_dp, pCTX + act_ctx  );
			    if (symbol)
			    { pdir1=1;
			    }
			    else 
			    { pdir1=3;
#if EXTRACT
				symbol_act_2++;//
#endif					
			    }
			  }
			  break;
			case 3:
			  act_ctx=15;
			  symbol = biari_decode_symbol ( dep_dp, pCTX + act_ctx  );
			  if (symbol)
			  { pdir1=0;
			  }
			  else 
			  {
#if EXTRACT
				symbol_act_2++;//
#endif				  
			    act_ctx=16;
			    symbol = biari_decode_symbol ( dep_dp, pCTX + act_ctx  );
			    if (symbol)
			    { pdir1=1;
			    }
			    else 
			    { pdir1=2;
#if EXTRACT
				symbol_act_2++;//
#endif					
			    }
			  }
			  break;
	    } 

    }
    pdir0=new_pdir[pdir0];
    pdir1=new_pdir[pdir1];
    se->value1=dir2offset[pdir0][pdir1];
#if EXTRACT
	symbol_act_out = symbol_act_1*4+symbol_act_2;
#if EXTRACT_lcu_info_BAC_inter
	fprintf(p_lcu,"b_pu_type_index  %d 2PU read\n",symbol_act_out);//b_pu_type_index  
#endif	
	EcuInfoInterSyntax[2]=symbol_act_out;// 存放句法b_pu_type_index, 把2N、2划分、2划分且cu=8x8的三种情况融合在一起		    	
#endif		
  }

#if TRACE
  if ( currMB->cuType >= P2NXN && currMB->cuType <= PVER_RIGHT )
  {
    tracebits2 ( "B_Pred_Dir0 ", 1, pdir0 );
    tracebits2 ( "B_Pred_Dir1 ", 1, pdir1 );
  }
  else if ( currMB->cuType == 1 )
  {
    tracebits2 ( "B_Pred_Dir ", 1, pdir0 );
  }
#endif

}



/*!
************************************************************************
* \brief
*    This function is used to arithmetically decode the PU type.
************************************************************************
*/
void readPdir_dhp( SyntaxElement *se,	DecodingEnvironmentPtr dep_dp, codingUnit *currMB,	int uiPosition )
{
	int act_ctx;
	int act_sym;
	int symbol;
	int pdir0,pdir1;
	static const int dir2offset[2][2] = {{ 0,  1},{ 2,  3}};

	SyntaxInfoContexts *ctx = ( img->currentSlice )->syn_ctx;
	BiContextTypePtr pCTX = ctx->pdir_dhp_contexts;

	int counter = 0;
	int terminate_bit = 0;
	act_ctx = 0;
	act_sym = 0;
	if ( currMB->cuType == P2NX2N )
	{
		act_sym = biari_decode_symbol ( dep_dp, pCTX + act_ctx ) ;

		pdir0=act_sym;
		se->value1 = act_sym;
#if EXTRACT
		EcuInfoInterSyntax[3] = se->value1; //f_pu_type_index Table73	默认0	
#if EXTRACT_lcu_info_BAC_inter
	    fprintf(p_lcu,"f_pu_type_index  %d  F_2Nx2N\n",se->value1);//f_pu_type_index  
#endif

#endif			
	}
	else if (currMB->cuType >= P2NXN || currMB->cuType <= PVER_RIGHT)
	{
		counter = 0;
		terminate_bit = 0;
		act_ctx = 0;
		act_sym = 0;

	    act_sym = biari_decode_symbol ( dep_dp, pCTX + act_ctx +1 ) ;
		pdir0 = act_sym;

		symbol = biari_decode_symbol ( dep_dp, pCTX + act_ctx +2  );
		if (symbol)
		{pdir1=pdir0;
		}
		else 
		{ 
			pdir1=1 - pdir0;
		}
        se->value1=dir2offset[pdir0][pdir1];
#if EXTRACT
		EcuInfoInterSyntax[3] = se->value1; //f_pu_type_index Table74	默认0	
#if EXTRACT_lcu_info_BAC_inter
	    fprintf(p_lcu,"f_pu_type_index  %d  F_2Pu \n",se->value1);//f_pu_type_index  
#endif
#endif			
	}

#if TRACE
	if ( currMB->cuType >= P2NXN && currMB->cuType <= PVER_RIGHT )
	{
		tracebits2 ( "P_Pred_Dir0 ", 1, pdir0 );
		tracebits2 ( "P_Pred_Dir1 ", 1, pdir1 );
	}
	else if ( currMB->cuType == 1 )
	{
		tracebits2 ( "P_Pred_Dir ", 1, pdir0 );
	}
#endif

}



/*!
************************************************************************
* \brief
*    This function is used to arithmetically decode a pair of
*    intra prediction modes of a given MB.
************************************************************************
*/
void readIntraPredMode ( SyntaxElement *se, DecodingEnvironmentPtr dep_dp, codingUnit *MB, int uiPosition )
{
  BiContextTypePtr pCTX;
  int ctx, symbol;
  pCTX = img->currentSlice->syn_ctx->l_intra_mode_contexts;
  symbol = 0;
  ctx = 0;
  if ( biari_decode_symbol ( dep_dp, pCTX + ctx ) == 1 )
  {
    symbol += biari_decode_symbol ( dep_dp, pCTX + ctx + 6 );
    symbol -= 2;
  }
  else
  {
    symbol += biari_decode_symbol ( dep_dp, pCTX + ctx + 1 ) << 4;
    symbol += biari_decode_symbol ( dep_dp, pCTX + ctx + 2 ) << 3;
    symbol += biari_decode_symbol ( dep_dp, pCTX + ctx + 3 ) << 2;
    symbol += biari_decode_symbol ( dep_dp, pCTX + ctx + 4 ) << 1;
    symbol += biari_decode_symbol ( dep_dp, pCTX + ctx + 5 );
  }
  se->value1 = symbol;
#if TRACE
  fprintf ( p_trace, "@%d %s\t\t\t%d\n", symbolCount++, se->tracestring, se->value1 );
  fflush ( p_trace );
#endif
}
/*!
************************************************************************
* \brief
*    This function is used to arithmetically decode the reference
*    parameter of a given MB.
************************************************************************
*/

void readRefFrame ( SyntaxElement *se, DecodingEnvironmentPtr dep_dp, codingUnit *currMB, int uiPosition )
{
  SyntaxInfoContexts *ctx = img->currentSlice->syn_ctx;  
  int   a, b;
  int   act_ctx;
  int   act_sym;
  int** refframe_array = ( ( img->type == B_IMG ) ? img->fw_refFrArr : refFrArr );
  int   b8a, b8b;

  PixelPos block_a, block_b;

  int bslice = ( img->type == B_IMG );

  int bw_flag = se->value2 & 0x01;

#if !HZC_BUGFIX
  getLuma8x8Neighbour (img->subblock_x, img->subblock_y, -1,  0, &block_a,uiPosition, img->mb_data[uiPosition].ui_MbBitSize,&img->mb_data[uiPosition], bw_flag );
  getLuma8x8Neighbour (  img->subblock_x, img->subblock_y,  0, -1, &block_b,uiPosition, img->mb_data[uiPosition].ui_MbBitSize,&img->mb_data[uiPosition], bw_flag );



  b8a = ( ( block_a.x ) % 2 ) + 2 * ( ( block_a.y ) % 2 );
  b8b = ( ( block_b.x ) % 2 ) + 2 * ( ( block_b.y ) % 2 );

  if ( !block_b.available )
  {
    b = 0;
  }
  else if ( IS_DIRECT ( &img->mb_data[block_b.mb_addr] ) || ( img->mb_data[block_b.mb_addr].b8mode[b8b] == 0 && img->mb_data[block_b.mb_addr].b8pdir[b8b] == 2 ) )
  {
    b = 0;
  }
  else
  {

    int b8_y = ( uiPosition / img->PicWidthInMbs ) << 1;
    if ( &img->mb_data[block_b.mb_addr] == currMB/*&&currMB->cuType==4&&currMB->ui_MbBitSize>4*/ )
    {
      {
        block_b.pos_y = b8_y;
      }
    }

    b = ( refframe_array[block_b.pos_y][block_b.pos_x] > 0 ? 1 : 0 );
  }

  if ( !block_a.available )
  {
    a = 0;
  }
  else if ( IS_DIRECT ( &img->mb_data[block_a.mb_addr] ) || ( img->mb_data[block_a.mb_addr].b8mode[b8a] == 0 && img->mb_data[block_a.mb_addr].b8pdir[b8a] == 2 ) )
  {
    a = 0;
  }
  else
  {

    int b8_x = ( uiPosition % img->PicWidthInMbs ) << 1;
    if ( &img->mb_data[block_a.mb_addr] == currMB/*&&currMB->cuType==6&&currMB->ui_MbBitSize>4*/ )
    {
      //       if(block_a.pos_x - b8_x == (1<<(currMB->ui_MbBitSize -4 ))-1)
      {
        block_a.pos_x = b8_x;
      }
    }
    a = ( refframe_array[block_a.pos_y][block_a.pos_x] > 0 ? 1 : 0 );
  }
#endif 

#if !M3578_CONTEXT_OPT
  act_ctx = a + 2 * b;
#else 
  act_ctx = 0;
#endif 
  se->context = act_ctx; // store context


  if ( biari_decode_symbol ( dep_dp, ctx->ref_no_contexts + act_ctx ) )
  {
    act_sym = 0;
  }
  else
  {
    act_sym = 1;

    if ( bslice == 0 )
    {
      BiContextTypePtr pCTX;
      pCTX = ctx->ref_no_contexts;
      act_ctx = 4;

	  while ((act_sym != img->num_of_references-1) && (!biari_decode_symbol ( dep_dp, pCTX + act_ctx )))
	  {
		  act_sym ++;
		  act_ctx ++;

		  if ( act_ctx >= 5 )
		  {
			  act_ctx = 5;
		  }
	  }
    }
  }

  se->value1 = act_sym;
  //! Edit End "AEC - SideInfo (NO MBAFF)" '10-01-2005 by Ning Zhang (McMaster Univ.)
}



/*!
************************************************************************
* \brief
*    This function is used to arithmetically decode the delta qp
*     of a given MB.
************************************************************************
*/
void readDquant ( SyntaxElement *se, DecodingEnvironmentPtr dep_dp, codingUnit *MB,int uiPosition )
{
  SyntaxInfoContexts *ctx = img->currentSlice->syn_ctx;

  int act_ctx;
  int act_sym;
  int dquant;

  act_ctx = ( ( last_dquant != 0 ) ? 1 : 0 );

  act_sym = 1 - biari_decode_symbol ( dep_dp, ctx->delta_qp_contexts + act_ctx );

#if EXTRACT_lcu_info_cutype_debug
  //此时的act_sym是码字中连续0的个数
  fprintf(p_lcu_debug, "readDquant act_ctx=%d symbol=%d\n", act_ctx, 1-act_sym);//cu_type_index
#endif	
  if ( act_sym != 0 )
  {
    act_ctx = 2;
    act_sym = unary_bin_decode ( dep_dp, ctx->delta_qp_contexts + act_ctx, 1 );
    act_sym++;
  }

  dquant = ( act_sym + 1 ) / 2;

  if ( ( act_sym & 0x01 ) == 0 )                    // lsb is signed bit
  {
    dquant = -dquant;
  }

  se->value1 = dquant;

  last_dquant = dquant;


#if TRACE
  fprintf ( p_trace, "@%d %s\t\t\t%d\n", symbolCount++, se->tracestring, se->value1 );
  fflush ( p_trace );
#endif
}
/*!
************************************************************************
* \brief
*    This function is used to arithmetically decode the coded
*    block pattern of a given MB.
************************************************************************
*/
#if CBP_MODIFICATION
int readCBP_BIT_AEC( int b8, int inter, DecodingEnvironmentPtr dep_dp, int uiPosition )
{
  int i = b8 % 2 ;
  int j = b8 / 2 ;
  int a, b;
#if TEXT_RD92_ALIGN
  int uiBitSize= img->mb_data[uiPosition].ui_MbBitSize;
  int width = ( 1 << uiBitSize );
  int height = ( 1 << uiBitSize );
  int x, y,sizeOfNeighborBlock;
  codingUnit *neighborMB;
#endif 
  PixelPos block_a, block_b;
  BiContextTypePtr pCTX;	

#if CTP_Y_BUG_FIX
  int block_a_nsqt_hor = 0,block_a_nsqt_ver = 0;
  int block_b_nsqt_hor = 0,block_b_nsqt_ver = 0;
  int currMB_hor=((img->mb_data[uiPosition].cuType==P2NXN && uiBitSize>3||img->mb_data[uiPosition].cuType==PHOR_UP||img->mb_data[uiPosition].cuType==PHOR_DOWN)&&img->mb_data[uiPosition].trans_size==1&&input->useNSQT==1 ||img->mb_data[uiPosition].cuType==InNxNMB);
  int currMB_ver=((img->mb_data[uiPosition].cuType==PNX2N && uiBitSize>3||img->mb_data[uiPosition].cuType==PVER_LEFT||img->mb_data[uiPosition].cuType==PVER_RIGHT)&&img->mb_data[uiPosition].trans_size==1&&input->useNSQT==1 ||img->mb_data[uiPosition].cuType==INxnNMB);
#endif 
  // 	int bit;
  if (b8==4)
  {
    i = j = 0;
  }
#if !TEXT_RD92_ALIGN
  getLuma8x8Neighbour ( i, j, -1,  0, &block_a,uiPosition, img->mb_data[uiPosition].ui_MbBitSize,&img->mb_data[uiPosition], 0 );
  getLuma8x8Neighbour ( i, j,  0, -1, &block_b,uiPosition, img->mb_data[uiPosition].ui_MbBitSize,&img->mb_data[uiPosition], 0 );
#else 
  i = ( i == 0 ) ? 0 : 1 ;
  j = ( j == 0 ) ? 0 : 1 ;

#if !CTP_Y_BUG_FIX
  x = width * i / 2  -1; //qyu night
  y = height * j / 2 + 0;
#else 
  if (currMB_hor)
  {
    x=-1;
    y= height * (i+2*j) / 4 + 0;
  }
  else if (currMB_ver)
  {
    x= width * (i+2*j) / 4  -1; 
    y= 0;
  }
  else 
  {
    x = width * i / 2  -1; //qyu night
    y = height * j / 2 + 0;
  }
#endif 

  getNeighbour ( x, y, 1, &block_a, uiPosition, uiBitSize, &img->mb_data[uiPosition] );

#if !CTP_Y_BUG_FIX
  x = width * i / 2 + 0; //qyu night
  y = height * j / 2  -1;
#else 
  if (currMB_hor)
  {
	  x= 0;
	  y= height * (i+2*j) / 4 -1;
  }
  else if (currMB_ver)
  {
	  x= width * (i+2*j) / 4  +0; 
	  y= -1;
  }
  else 
  {
	  x = width * i / 2  + 0; //qyu night
	  y = height * j / 2 - 1;
  }
#endif 

  getNeighbour ( x, y, 1, &block_b, uiPosition, uiBitSize, &img->mb_data[uiPosition] );

  if ( block_a.available )
  {

	  if ( block_a.mb_addr == uiPosition )
	  {
		  neighborMB = &img->mb_data[uiPosition] ;
	  }
	  else
	  {
		  neighborMB = &img->mb_data[block_a.mb_addr];
	  }

	  sizeOfNeighborBlock = neighborMB->ui_MbBitSize;
#if !CTP_Y_BUG_FIX
	  block_a.x = block_a.x / ( ( 1 << sizeOfNeighborBlock ) / 2 );
	  block_a.y = block_a.y / ( ( 1 << sizeOfNeighborBlock ) / 2 );

	  block_a.pos_x /= MIN_BLOCK_SIZE;
	  block_a.pos_y /= MIN_BLOCK_SIZE;
#else 
    block_a_nsqt_hor=((neighborMB->cuType==P2NXN && neighborMB->ui_MbBitSize>3||neighborMB->cuType==PHOR_UP||neighborMB->cuType==PHOR_DOWN)&&neighborMB->trans_size==1&&input->useNSQT==1 ||neighborMB->cuType==InNxNMB);
    block_a_nsqt_ver=((neighborMB->cuType==PNX2N && neighborMB->ui_MbBitSize>3||neighborMB->cuType==PVER_LEFT||neighborMB->cuType==PVER_RIGHT)&&neighborMB->trans_size==1&&input->useNSQT==1 ||neighborMB->cuType==INxnNMB);
	  if (block_a_nsqt_hor)
	  {
		  block_a.x = 0;
		  block_a.y = block_a.y / ( ( 1 << sizeOfNeighborBlock ) / 4 );

	  }
	  else if (block_a_nsqt_ver)
	  {
		  block_a.x = block_a.x / ( ( 1 << sizeOfNeighborBlock ) / 4 );
		  block_a.y = 0;
	  }
	  else 
	  {
		  block_a.x = block_a.x / ( ( 1 << sizeOfNeighborBlock ) / 2 );
		  block_a.y = block_a.y / ( ( 1 << sizeOfNeighborBlock ) / 2 );
	  }
#endif 
  }

  if ( block_b.available )
  {

	  if ( block_b.mb_addr == uiPosition )
	  {
		  neighborMB = &img->mb_data[uiPosition];
	  }
	  else
	  {
		  neighborMB = &img->mb_data[block_b.mb_addr];
	  }

	  sizeOfNeighborBlock = neighborMB->ui_MbBitSize;
#if !CTP_Y_BUG_FIX
	  block_b.x = block_b.x / ( ( 1 << sizeOfNeighborBlock ) / 2 );
	  block_b.y = block_b.y / ( ( 1 << sizeOfNeighborBlock ) / 2 );

	  block_b.pos_x /= MIN_BLOCK_SIZE;
	  block_b.pos_y /= MIN_BLOCK_SIZE;
#else 
    block_b_nsqt_hor = ( ( neighborMB->cuType == P2NXN && neighborMB->ui_MbBitSize>3 || neighborMB->cuType == PHOR_UP || neighborMB->cuType == PHOR_DOWN ) && neighborMB->trans_size == 1 && input->useNSQT == 1 || neighborMB->cuType==InNxNMB );
    block_b_nsqt_ver = ( ( neighborMB->cuType == PNX2N && neighborMB->ui_MbBitSize>3 || neighborMB->cuType == PVER_LEFT || neighborMB->cuType == PVER_RIGHT ) && neighborMB->trans_size == 1 && input->useNSQT == 1 || neighborMB->cuType==INxnNMB );
	  if (block_b_nsqt_hor)
	  {
		  block_b.x = 0;
		  block_b.y = block_b.y / ( ( 1 << sizeOfNeighborBlock ) / 4 );

	  }
	  else if (block_b_nsqt_ver)
	  {
		  block_b.x = block_b.x / ( ( 1 << sizeOfNeighborBlock ) / 4 );
		  block_b.y = 0;
	  }
	  else 
	  {
		  block_b.x = block_b.x / ( ( 1 << sizeOfNeighborBlock ) / 2 );
		  block_b.y = block_b.y / ( ( 1 << sizeOfNeighborBlock ) / 2 );
	  }
#endif 
  }
#endif 


  if (b8==4)
  {
#if !M3578_CONTEXT_OPT
    if ( block_a.available )
    {
      a = img->mb_data[block_a.mb_addr].cbp == 0 ;
    }
    else
    {
      a = 0;
    }
    if ( block_b.available )
    {
      b = img->mb_data[block_b.mb_addr].cbp == 0 ;
    }
    else
    {
      b = 0;
    }
    pCTX = img->currentSlice->syn_ctx->cbp_contexts[2] + a + 2 * b;
#else 
    pCTX = img->currentSlice->syn_ctx->cbp_contexts[2] ;
#endif 
  }
  else
  {
    if ( block_a.available )
    {
#if !CTP_Y_BUG_FIX
      a = (img->mb_data[block_a.mb_addr].cbp & ( 1<< ( block_a.x+block_a.y*2 ) ))!=0 ;
#else 
		if (block_a_nsqt_hor)
		{    
      a = (img->mb_data[block_a.mb_addr].cbp & ( 1<< ( block_a.y ) ))!=0 ;
    }
    else if (block_a_nsqt_ver)
    {
      a = (img->mb_data[block_a.mb_addr].cbp & ( 1<< ( block_a.x ) ))!=0 ;
    }
    else 
    {
      a = (img->mb_data[block_a.mb_addr].cbp & ( 1<< ( block_a.x+block_a.y*2 ) ))!=0 ;
		}
#endif 
    }
    else
    {
      a = 0;
    }
    if ( block_b.available )
    {
#if !CTP_Y_BUG_FIX
      b = (img->mb_data[block_b.mb_addr].cbp & ( 1<< ( block_b.x+block_b.y*2 ) ) )!=0;
#else 
		if (block_b_nsqt_hor)
		{    
			b = (img->mb_data[block_b.mb_addr].cbp & ( 1<< ( block_b.y ) ))!=0 ;
		}
		else if (block_b_nsqt_ver)
		{
			b = (img->mb_data[block_b.mb_addr].cbp & ( 1<< ( block_b.x ) ))!=0 ;
		}
		else 
		{
			b = (img->mb_data[block_b.mb_addr].cbp & ( 1<< ( block_b.x+block_b.y*2 ) ))!=0 ;
		}
#endif 
    }
    else
    {
      b = 0;
    }
    pCTX = img->currentSlice->syn_ctx->cbp_contexts[0] + a + 2 * b;
  }

  return biari_decode_symbol ( dep_dp, pCTX );
  //return bit;

}
void readCBP( SyntaxElement *se, DecodingEnvironmentPtr dep_dp, codingUnit *currMB, int uiPosition )
{
  SyntaxInfoContexts *ctx = img->currentSlice->syn_ctx;

  //   int mb_x, mb_y;
  //   int a, b;
  int curr_cbp_ctx;
  int cbp = 0;
  int cbp_bit=0;
  int tr_bit;
  //   int mask;

  if ( !IS_INTRA(currMB) )
  {
	  if ( !(IS_DIRECT(currMB)||IS_P_SKIP(currMB)))
    {
      cbp_bit = readCBP_BIT_AEC( 4, 1, dep_dp, uiPosition );
#if EXTRACT
      EcuInfoInterSyntax[44]= cbp_bit;//cu ctp_zero_flag
      //fprintf(p_lcu,"ctp_zero_flag  %d \n",EcuInfoInterSyntax[44]);//ctp_zero_flag
#endif	  
    }
    if ( cbp_bit == 0 )
    {
      ///////////////////////////  transform  size ///////////////////////////////////
      tr_bit =  biari_decode_symbol( dep_dp, ctx->tu_contexts );
      currMB->trans_size = tr_bit;
#if EXTRACT
      EcuInfoInterSyntax[37]= tr_bit;//cu transform_split_flag
      //fprintf(p_lcu,"transform_split_flag  %d \n",EcuInfoInterSyntax[37]);//transform_split_flag
#endif	

      ///////////////////////////  chroma  ///////////////////////////////////
      cbp_bit = biari_decode_symbol ( dep_dp, ctx->cbp_contexts[1] );
      if (tr_bit == 0 )
      {
        if( cbp_bit ==0 )
        {
          cbp = 15;
        }
        else
        {
          ////////////////////////////////   chroma  ////////////////////////////////
          cbp_bit = biari_decode_symbol ( dep_dp, ctx->cbp_contexts[1] + 2 );

          if ( cbp_bit )
          {
            cbp += 48;
          }
          else
          {
            cbp_bit = biari_decode_symbol ( dep_dp, ctx->cbp_contexts[1] + 2 );
            cbp += ( cbp_bit == 1 ) ? 32 : 16;
          }
          ////////////////////////////////   luma  ////////////////////////////////
          cbp_bit = readCBP_BIT_AEC ( 0, 1, dep_dp, uiPosition );
          cbp += cbp_bit? 15 : 0;
        }
      }
      else
      {
        ////////////////////////////////   chroma  ////////////////////////////////
        if ( cbp_bit ) 
        {
          cbp_bit = biari_decode_symbol ( dep_dp, ctx->cbp_contexts[1] + 2 );

          if ( cbp_bit )
          {
            cbp += 48;
          }
          else
          {
            cbp_bit = biari_decode_symbol ( dep_dp, ctx->cbp_contexts[1] + 2 );
            cbp += ( cbp_bit == 1 ) ? 32 : 16;
          }
        }

        ////////////////////////////////   luma  ////////////////////////////////
        cbp_bit = readCBP_BIT_AEC ( 0, 1, dep_dp, uiPosition );
        cbp += cbp_bit;
        currMB->cbp = cbp;
        cbp_bit = readCBP_BIT_AEC ( 1, 1, dep_dp, uiPosition );
        cbp += (cbp_bit<<1);
        currMB->cbp = cbp;
        cbp_bit = readCBP_BIT_AEC ( 2, 1, dep_dp, uiPosition );
        cbp += (cbp_bit<<2);
        currMB->cbp = cbp;
        cbp_bit = readCBP_BIT_AEC ( 3, 1, dep_dp, uiPosition );
        cbp += (cbp_bit<<3);
        currMB->cbp = cbp;
      }
    }
    else
    {
      currMB->trans_size = 1;
      currMB->cbp = cbp = 0;
    }
  }
  else
  {
    currMB->trans_size = currMB->cuType == I16MB? 0:1;
#if INTRA_2N
    //////////////////////////  intra luma  /////////////////////////////////
    if (currMB->cuType == I16MB )
    {
      cbp_bit = readCBP_BIT_AEC ( 0, 0, dep_dp, uiPosition );
      cbp = cbp_bit? 15 : 0;
    }
    else
#endif
    {
      cbp_bit = readCBP_BIT_AEC ( 0, 0, dep_dp, uiPosition );
      cbp += cbp_bit;
      currMB->cbp = cbp;
      cbp_bit = readCBP_BIT_AEC ( 1, 0, dep_dp, uiPosition );
      cbp += (cbp_bit<<1);
      currMB->cbp = cbp;
      cbp_bit = readCBP_BIT_AEC ( 2, 0, dep_dp, uiPosition );
      cbp += (cbp_bit<<2);
      currMB->cbp = cbp;
      cbp_bit = readCBP_BIT_AEC ( 3, 0, dep_dp, uiPosition );
      cbp += (cbp_bit<<3);
      currMB->cbp = cbp;
    }

#if EXTRACT_lcu_info_debug   
    if (p_lcu_debug != NULL)
    {
      fprintf(p_lcu_debug, "ffmpeg CU Y cbp %d\n", cbp);
    }
#endif	
    /////////////////////////////  chroma  decoding  //////////////////////////////
    cbp_bit = biari_decode_symbol ( dep_dp, ctx->cbp_contexts[1] + 1 );
    if ( cbp_bit ) 
    {
      curr_cbp_ctx = 1;    
      cbp_bit = biari_decode_symbol ( dep_dp, ctx->cbp_contexts[1] + 3 );

      if ( cbp_bit )
      {
        cbp += 48;
      }
      else
      {
        curr_cbp_ctx = 1;
        cbp_bit = biari_decode_symbol ( dep_dp, ctx->cbp_contexts[1] + 3 );
        cbp += ( cbp_bit == 1 ) ? 32 : 16;
      }
    }
  }

  se->value1 = cbp;
  
#if EXTRACT
		  EcuInfoInterSyntax[36]= cbp;//cu ctp
		  //fprintf(p_lcu,"ctp  %d \n",EcuInfoInterSyntax[36]);//ctp
#endif	
#if EXTRACT_lcu_info_debug   
      if (p_lcu_debug != NULL)
      {
        fprintf(p_lcu_debug, "ffmpeg CU cbp %d end\n", cbp);
      }
#endif	
  if ( !cbp )
  {
    last_dquant = 0;
#if EXTRACT_lcu_info_cutype_debug
    //此时的act_sym是码字中连续0的个数
    fprintf(p_lcu_debug, "readCBP cbp=%d last_dquant=%d\n", cbp, last_dquant);//cu_type_index
#endif	
  }

#if TRACE
  fprintf ( p_trace, "@%d %s\t\t\t\t%d\n", symbolCount++, se->tracestring, se->value1 );
  fflush ( p_trace );
#endif

}
#else
void readCBP( SyntaxElement *se, DecodingEnvironmentPtr dep_dp, codingUnit *currMB, int uiPosition )
{
  SyntaxInfoContexts *ctx = img->currentSlice->syn_ctx;

  int mb_x, mb_y;
  int a, b;
  int curr_cbp_ctx;
  int cbp = 0;
  int cbp_bit;
  int mask;
  //  coding of luma part (bit by bit)
  for ( mb_y = 0; mb_y < 4; mb_y += 2 )
  {
    for ( mb_x = 0; mb_x < 4; mb_x += 2 )
    {

      if ( mb_y == 0 )
      {
        if ( currMB->mb_available_up == NULL )
        {
          b = 0;
        }
        else
        {
          b = ( ( ( ( currMB->mb_available_up )->cbp & ( 1 << ( 2 + mb_x / 2 ) ) ) == 0 ) ? 1 : 0 );
        }

      }
      else
      {
        b = ( ( ( cbp & ( 1 << ( mb_x / 2 ) ) ) == 0 ) ? 1 : 0 );
      }

      if ( mb_x == 0 )
      {
        if ( currMB->mb_available_left == NULL )
        {
          a = 0;
        }
        else
        {
          a = ( ( ( ( currMB->mb_available_left )->cbp & ( 1 << ( 2 * ( mb_y / 2 ) + 1 ) ) ) == 0 ) ? 1 : 0 );
        }
      }
      else
      {
        a = ( ( ( cbp & ( 1 << mb_y ) ) == 0 ) ? 1 : 0 );
      }

      curr_cbp_ctx = a + 2 * b;
      mask = ( 1 << ( mb_y + mb_x / 2 ) );
      cbp_bit = biari_decode_symbol ( dep_dp, ctx->cbp_contexts[0] + curr_cbp_ctx );

      if ( cbp_bit )
      {
        cbp += mask;
      }
#if INTRA_2N
      if ( currMB->trans_size == 0 || currMB->cuType == I16MB )
      {
        mb_y += 2;
        mb_x += 2;
        if ( cbp_bit )
        {
          cbp = 15;
        }
        else
        {
          cbp = 0;
        }
      }
#endif
    }
  }

  curr_cbp_ctx = 0;
  cbp_bit = biari_decode_symbol ( dep_dp, ctx->cbp_contexts[1] + curr_cbp_ctx );

  // AEC decoding for BinIdx 5
  if ( cbp_bit ) // set the chroma bits
  {
    curr_cbp_ctx = 1;     //changed by lzhang
    cbp_bit = biari_decode_symbol ( dep_dp, ctx->cbp_contexts[1] + curr_cbp_ctx );

    if ( cbp_bit )
    {
      cbp += 48;
    }
    else
    {
      curr_cbp_ctx = 1;
      cbp_bit = biari_decode_symbol ( dep_dp, ctx->cbp_contexts[1] + curr_cbp_ctx );
      cbp += ( cbp_bit == 1 ) ? 32 : 16;
    }
  }

  se->value1 = cbp;

  if ( !cbp )
  {
    last_dquant = 0;
  }

#if TRACE
  fprintf ( p_trace, "@%d %s\t\t\t\t%d\n", symbolCount++, se->tracestring, se->value1 );
  fflush ( p_trace );
#endif

}
#endif
/*!
************************************************************************
* \brief
*    This function is used to arithmetically decode the chroma
*    intra prediction mode of a given MB.
************************************************************************
*/  //GB
void readCIPredMode ( SyntaxElement *se,DecodingEnvironmentPtr dep_dp,codingUnit *currMB,int uiPosition )
{
  SyntaxInfoContexts *ctx = img->currentSlice->syn_ctx;
  int                 act_ctx, a, b;
  int                 act_sym  = se->value1;
  int lmode = 0;
  Boolean is_redundant = FALSE;
  int block8_y = ( uiPosition / img->PicWidthInMbs ) << 1;
  int block8_x = ( uiPosition % img->PicWidthInMbs ) << 1;
  int LumaMode = img->ipredmode[1 + block8_y][1 + block8_x];

  if( LumaMode == VERT_PRED || LumaMode == HOR_PRED || LumaMode == DC_PRED || LumaMode == BI_PRED)
  {
    lmode = LumaMode==VERT_PRED ? VERT_PRED_C : ( LumaMode == HOR_PRED ? HOR_PRED_C : ( LumaMode == DC_PRED ?  DC_PRED_C : BI_PRED_C ) );
    is_redundant = TRUE;
  }

  if ( currMB->mb_available_up == NULL )
  {
    b = 0;
  }
  else
  {
    b = ( ( ( currMB->mb_available_up )->c_ipred_mode != 0 ) ? 1 : 0 );
  }


  if ( currMB->mb_available_left == NULL )
  {
    a = 0;
  }
  else
  {
    a = ( ( ( currMB->mb_available_left )->c_ipred_mode != 0 ) ? 1 : 0 );
  }

#if !M3544_CONTEXT_OPT
  act_ctx = a + b;
#else 
  act_ctx = a;
#endif 

#if EXTRACT_lcu_info_debug
  fprintf(p_lcu_debug, "intra_c_pred_mode a=%d \n", a);
#endif
#if UNIFY_BINARIZATION
  act_sym = !biari_decode_symbol ( dep_dp, ctx->c_intra_mode_contexts + act_ctx );
#else
  act_sym = biari_decode_symbol ( dep_dp, ctx->c_intra_mode_contexts + act_ctx );
#endif
  if ( act_sym != 0 )
  {
#if !HZC_INTRA_CHROMA_MODE
	  if( is_redundant )
	  {
		  act_sym = unary_bin_max_decode ( dep_dp, ctx->c_intra_mode_contexts + 3, 0, 2 ) + 1;
		  if( act_sym >= lmode )
			  act_sym++;
	  }
	  else
		  act_sym = unary_bin_max_decode ( dep_dp, ctx->c_intra_mode_contexts + 3, 0, 3 ) + 1;
#else 
	  act_sym = unary_bin_max_decode ( dep_dp, ctx->c_intra_mode_contexts + 3, 0, 3 ) + 1;
	  if( is_redundant &&  act_sym >= lmode)
	  {    
		  if (act_sym==4)
		  {
			  printf("\n  error in intra_chroma_pred_mode \n");
		  }
		  act_sym++;
	  }
#endif
  }


  se->value1 = act_sym;
#if EXTRACT_lcu_info_debug
  fprintf(p_lcu_debug, "intra_c_pred_mode act_sym=%d \n", act_sym);
#endif


#if TRACE
  fprintf ( p_trace, "@%d %s\t\t%d\n", symbolCount++, se->tracestring, se->value1 );
  fflush ( p_trace );
#endif

}


int DCT_Level[MAX_CU_SIZE * MAX_CU_SIZE + 1];
int DCT_Run[MAX_CU_SIZE * MAX_CU_SIZE + 1];
int DCT_CGFlag[CG_SIZE * CG_SIZE];
int DCT_PairsInCG[CG_SIZE * CG_SIZE];
int DCT_CGNum;
int Pair_Pos = 0;
int DCT_Pairs = -1;
const int T_Chr[5] = { 0, 1, 2, 4, 3000};

// 0: INTRA_PRED_VER
// 1: INTRA_PRED_HOR
// 2: INTRA_PRED_DC_DIAG
int g_intraModeClassified[NUM_INTRA_PMODE] = {2,2,2,1,1,2,2,2,0,0,0,0,0,0,0,0,0,
                                              2,2,2,1,1,1,1,1,1,1,1,1,2,2,2,0}; 

void readRunLevelRef ( SyntaxElement  *se, DecodingEnvironmentPtr dep_dp, codingUnit *currMB, int uiPosition )
{
  const int raster2ZZ_2x2[] = { 0,  1,  2,  3};
  const int raster2ZZ_4x4[] = { 0,  1,  5,  6,
    2,  4,  7, 12,
    3,  8, 11, 13,
    9, 10, 14, 15}; 
  const int raster2ZZ_8x8[] = { 0,  1,  5,  6, 14, 15, 27, 28,
    2,  4,  7, 13, 16, 26, 29, 42,
    3,  8, 12, 17, 25, 30, 41, 43,
    9, 11, 18, 24, 31, 40, 44, 53,
    10, 19, 23, 32, 39, 45, 52, 54,
    20, 22, 33, 38, 46, 51, 55, 60,
    21, 34, 37, 47, 50, 56, 59, 61,
    35, 36, 48, 49, 57, 58, 62, 63};
  const int raster2ZZ_2x8[] = { 0,  1,  4,  5 , 8,  9, 12, 13,  
    2,  3,  6,  7, 10, 11, 14, 15};     
  const int raster2ZZ_8x2[] = { 0,  1,
    2,  4,
    3,  5,
    6,  8,
    7,  9,
    10, 12,
    11, 13,
    14, 15};
  int pairs, rank, pos;
  int Run, Level, absLevel, symbol;
  int sigCGFlag = 1, firstCG = 0;
  int iCG, numOfCG;
  int i, numOfCoeffInCG;
  int pairsInCG = 0;
  int CGLastX = 0;
  int CGLastY = 0;
  int CGLast = 0;
  int CGx=0, CGy=0;
  int prevCGFlagSum;
  int absSum5, n, k;
  int pos2RunCtx[16] = { -1, 0, 0, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3 };
  int zigzag[4][4]={{0,1,5,6},{2,4,7,12},{3,8,11,13},{9,10,14,15}};
  int xx,yy;

  int numOfCoeff = ( 1 << ( currMB->ui_MbBitSize - 1 ) ) * ( 1 << ( currMB->ui_MbBitSize - 1 ) );
#if M3198_CU8
  int bitSize = currMB->ui_MbBitSize - MIN_CU_SIZE_IN_BIT;
#else
  int bitSize = currMB->ui_MbBitSize - 4;
#endif

  int px, py, moddiv, indiv, ctxpos;
  int isChroma = 0;
  int ctxmode = INTRA_PRED_DC_DIAG;

#if M3474_LEVEL
  int band=0;
  int  l,golomb_order=0,sig,binary_symbol=0;
  int bins;
#endif 
  int pairs_prev;


  if( se->context == LUMA_8x8 && se->type == SE_LUM_AC_INTRA)
  {
    ctxmode = g_intraModeClassified[currMB->l_ipred_mode];
    if( ctxmode == INTRA_PRED_HOR )
    {
      ctxmode = INTRA_PRED_VER;
    }
  }

#if INTRA_2N
  if ( currMB->trans_size == 0 && currMB->ui_MbBitSize != B64X64_IN_BIT && se->context == LUMA_8x8)
  {
    numOfCoeff = ( 1 << currMB->ui_MbBitSize ) * ( 1 << currMB->ui_MbBitSize );
    bitSize = bitSize + 1;
  }
#endif
  if (input->useNSQT && currMB->ui_MbBitSize > B8X8_IN_BIT &&((currMB->trans_size == 1) && (currMB->ui_MbBitSize == B64X64_IN_BIT) && (IS_INTER(currMB)) && (currMB->cuType >= P2NXN && currMB->cuType <= PVER_RIGHT ) &&( se->context == LUMA_8x8 )))
  {
    numOfCoeff = ( 1 << ( currMB->ui_MbBitSize - 2 ) ) * ( 1 << ( currMB->ui_MbBitSize - 2 ) );
    bitSize -=1;
  }
	

  numOfCG = (numOfCoeff >> 4);
  numOfCoeffInCG = 16;

  //--- read coefficients for whole block ---
  if ( DCT_Pairs < 0 )
  {
    BiContextType ( *Primary ) [NUM_MAP_CTX];
    BiContextTypePtr pCTX;
    BiContextTypePtr pCTXSigCG;
    BiContextTypePtr pCTXLastCG;
    BiContextTypePtr pCTXLastPosInCG;
    int ctx, offset, sigCGctx;

    if ( se->context == LUMA_8x8 )
    {
      Primary = img->currentSlice->syn_ctx->map_contexts;
      pCTXSigCG = img->currentSlice->syn_ctx->sigCG_contexts;
      pCTXLastCG = img->currentSlice->syn_ctx->lastCG_contexts;
      pCTXLastPosInCG = img->currentSlice->syn_ctx->lastPos_contexts;
    }
    else
    {
      isChroma = 1;
      Primary = img->currentSlice->syn_ctx->last_contexts;
      pCTXSigCG = img->currentSlice->syn_ctx->sigCG_contexts + NUM_SIGCG_CTX_LUMA;
      pCTXLastCG = img->currentSlice->syn_ctx->lastCG_contexts + NUM_LAST_CG_CTX_LUMA;
      pCTXLastPosInCG = img->currentSlice->syn_ctx->lastPos_contexts + NUM_LAST_POS_CTX_LUMA;
    }

    for ( iCG = 0; iCG < numOfCG; iCG ++ )
    {
      DCT_PairsInCG[ iCG ] = 0;
      DCT_CGFlag[ iCG ] = 0;
    }

    //! Decode
    rank = 0;
    pairs = 0;

    DCT_CGNum = 1;
    DCT_CGFlag[ 0 ] = 1;

#if EXTRACT_lcu_info_coeff_debug
    fprintf(p_lcu_debug, "numOfCG=%d \n", numOfCG);
#endif
    for ( iCG = 0; iCG < numOfCG; iCG ++ )
    {
      DCT_PairsInCG[ iCG ] = 0;

      //! Last CG POSITION
#if BBRY_CU8
      if (rank == 0 && bitSize)
#else
      if (rank == 0)
#endif
      {
        int CGLastBit;
#if BBRY_CU8
        int numCGminus1 = numOfCG - 1;
#else
        int numCGminus1 = (1 << (bitSize + 1)) - 1;
#endif
        // int numCGminus1 = (1 << (bitSize + 1)) - 1;
        int count = 0;
        int numCGminus1X;
        int numCGminus1Y;
        if (input->useNSQT && currMB->ui_MbBitSize > B8X8_IN_BIT &&(IS_INTER(currMB) && !(isChroma) && (currMB->trans_size == 1) && ((currMB->cuType == P2NXN) || (currMB->cuType == PHOR_UP) || (currMB->cuType == PHOR_DOWN))))
        {
#if !HZC_BUGFIX
          numCGminus1X = (bitSize == 2) ? (1 << (bitSize + 1)) - 1:(1 << (bitSize + 2)) - 1;
          numCGminus1Y = (bitSize == 2) ? (1 << (bitSize - 1)) - 1:(1 << (bitSize)) - 1;
#else 
          numCGminus1X = (1 << (bitSize + 1)) - 1;
          numCGminus1Y =  (1 << (bitSize - 1)) - 1;
#endif 
        }
        else if (input->useNSQT && currMB->ui_MbBitSize > B8X8_IN_BIT &&(IS_INTER(currMB) && !(isChroma) && (currMB->trans_size == 1) && ((currMB->cuType == PNX2N) || (currMB->cuType == PVER_LEFT) || (currMB->cuType == PVER_RIGHT))))
        {
#if !HZC_BUGFIX
          numCGminus1X = (bitSize == 2) ? (1 << (bitSize - 1)) - 1:(1 << (bitSize)) - 1;
          numCGminus1Y = (bitSize == 2) ? (1 << (bitSize + 1)) - 1:(1 << (bitSize + 2)) - 1;
#else 
          numCGminus1X =  (1 << (bitSize - 1)) - 1;
          numCGminus1Y =  (1 << (bitSize + 1)) - 1;
#endif 
        }
        else
          if(input->useSDIP && (IS_INTRA(currMB) && !(isChroma) && (currMB->trans_size == 1) && ((currMB->cuType == InNxNMB) || (currMB->cuType == INxnNMB) )))//add yuqh 20130825
          {

            numCGminus1X = (currMB->cuType == InNxNMB)? (1 << (bitSize + 1)) - 1 :(1 << (bitSize - 1)) - 1;
            numCGminus1Y =  currMB->cuType == INxnNMB?(1 << (bitSize + 1)) - 1 : (1 << (bitSize - 1))-1 ; 
          }
          else
          {
#if LASTCB_BUG_FIX
            numCGminus1X = numCGminus1Y = (1 << bitSize) - 1;
#else
            numCGminus1X = numCGminus1Y = numCGminus1;
#endif
          }
        if( se->context == LUMA_8x8 && ctxmode == INTRA_PRED_DC_DIAG )
        {
          SWAP(numCGminus1X, numCGminus1Y);
        }
#if BBRY_CU8
        if (bitSize == 0) //4x4
        {
          // no need to decode LastCG0Flag
          CGLast = 0;
          CGLastX = 0;
          CGLastY = 0;
        }
        else if (bitSize == 1) // 8x8
#else
        if (bitSize == 0) // 8x8
#endif
        {
          CGLast = 0;
          do
          {
            CGLastBit = !biari_decode_symbol ( dep_dp, pCTXLastCG + count );
            CGLast += CGLastBit;
            count++;
          } while(CGLastBit && count < 3);
#if EXTRACT_Coeff
			//EcuInfoInterSyntax[20+k+n]= currSE.value1;// 
			fprintf(p_lcu,"CGLast %d\n",CGLast);//
#endif	

#if !HZC_BUGFIX
		  CGLastX = CGLast & 1;
		  CGLastY = CGLast >> 1;
#else 

		  if (input->useNSQT && currMB->ui_MbBitSize > B8X8_IN_BIT &&(IS_INTER(currMB) && !(isChroma) && (currMB->trans_size == 1) && ((currMB->cuType == P2NXN) || (currMB->cuType == PHOR_UP) || (currMB->cuType == PHOR_DOWN))))
		  {
        CGLastX=CGLast;
        CGLastY = 0 ;        
		  }
		  else if (input->useNSQT && currMB->ui_MbBitSize > B8X8_IN_BIT &&(IS_INTER(currMB) && !(isChroma) && (currMB->trans_size == 1) && ((currMB->cuType == PNX2N) || (currMB->cuType == PVER_LEFT) || (currMB->cuType == PVER_RIGHT))))
		  {
        CGLastX=0;
        CGLastY =CGLast;
		  }
		  else if (input->useSDIP && (IS_INTRA(currMB) && !(isChroma) && (currMB->trans_size == 1) && ((currMB->cuType == InNxNMB)  )))
		  {
			  CGLastX=CGLast;
			  CGLastY =0;
		  }
		  else if (input->useSDIP && (IS_INTRA(currMB) && !(isChroma) && (currMB->trans_size == 1) && ((currMB->cuType == INxnNMB)  )))
		  {
			  CGLastX=0;
			  CGLastY = CGLast ;
		  }
		  else
		  {
			  CGLastX = CGLast & 1;
			  CGLastY = CGLast / 2;
		  }
#endif 

        }
        else // 16x16 and 32x32
        {
          int offset = isChroma ? 3 : 9;
          CGLastBit = biari_decode_symbol ( dep_dp, pCTXLastCG + offset );
#if EXTRACT_Coeff
          //EcuInfoInterSyntax[20+k+n]= currSE.value1;// 
          fprintf(p_lcu,"CGLastBit %d\n",CGLastBit);//
#endif			  
          if( CGLastBit == 0 )
          {
            CGLastX = 0;
            CGLastY = 0;
            CGLast  = 0;
          }
          else
          {
            offset = isChroma ? 4 : 10;
            do
            {
#if UNIFY_BINARIZATION
              CGLastBit = !biari_decode_symbol ( dep_dp, pCTXLastCG + offset );
#else
              CGLastBit = biari_decode_symbol ( dep_dp, pCTXLastCG + offset );
#endif
              CGLastX += CGLastBit;
              count++;
            } while(CGLastBit && count < numCGminus1X);
#if EXTRACT_Coeff
            //EcuInfoInterSyntax[20+k+n]= currSE.value1;// 
            fprintf(p_lcu,"CGLastX %d\n",CGLastX);//
#endif	

            count = 0;
            offset = isChroma ? 5 : 11;
            if( CGLastX == 0 )
            {    
              if (numCGminus1Y != 1)
              {
                do
                {
#if UNIFY_BINARIZATION
                  CGLastBit = !biari_decode_symbol ( dep_dp, pCTXLastCG + offset );
#else
                  CGLastBit = biari_decode_symbol ( dep_dp, pCTXLastCG + offset );
#endif
                  CGLastY += CGLastBit;
                  count++;
                } while(CGLastBit && count + 1 < numCGminus1Y);
#if EXTRACT_Coeff
                //EcuInfoInterSyntax[20+k+n]= currSE.value1;// 
                fprintf(p_lcu,"numCGminus1Y %d\n",CGLastY);//
#endif					
              }

              CGLastY ++;
#if EXTRACT_Coeff
              //EcuInfoInterSyntax[20+k+n]= currSE.value1;// 
              fprintf(p_lcu,"CGLastY %d\n",CGLastY);//
#endif					  
            }
            else
            {
              do
              {
#if UNIFY_BINARIZATION
                CGLastBit = !biari_decode_symbol ( dep_dp, pCTXLastCG + offset );
#else
                CGLastBit = biari_decode_symbol ( dep_dp, pCTXLastCG + offset );
#endif
                CGLastY += CGLastBit;
                count++;
              } while(CGLastBit && count < numCGminus1Y);
#if EXTRACT_Coeff
              //EcuInfoInterSyntax[20+k+n]= currSE.value1;// 
              fprintf(p_lcu,"CGLastY %d\n",CGLastY);//
              fflush(p_lcu);
#endif					  
            }

          }
          if( se->context == LUMA_8x8 && ctxmode == INTRA_PRED_DC_DIAG )
          {
            SWAP(CGLastX, CGLastY);
          }
#if M3198_CU8
          if (bitSize == 2) 
#else
          if (bitSize == 1)
#endif
          {
            if (input->useNSQT && currMB->ui_MbBitSize > B8X8_IN_BIT &&(IS_INTER(currMB) && !(isChroma) && (currMB->trans_size == 1) && ((currMB->cuType == P2NXN) || (currMB->cuType == PHOR_UP) || (currMB->cuType == PHOR_DOWN))))
            {
              CGLast = raster2ZZ_2x8[CGLastY * 8 + CGLastX];
            }
            else if (input->useNSQT && currMB->ui_MbBitSize > B8X8_IN_BIT &&(IS_INTER(currMB) && !(isChroma) && (currMB->trans_size == 1) && ((currMB->cuType == PNX2N) || (currMB->cuType == PVER_LEFT) || (currMB->cuType == PVER_RIGHT))))
            {
              CGLast = raster2ZZ_8x2[CGLastY * 2 + CGLastX];
            }
            else
              if(input->useSDIP && (IS_INTRA(currMB) && !(isChroma) && (currMB->trans_size == 1) && ((currMB->cuType == InNxNMB)  )))
              {
                CGLast = raster2ZZ_2x8[CGLastY * 8 + CGLastX];
              }
              else if(input->useSDIP && (IS_INTRA(currMB) && !(isChroma) && (currMB->trans_size == 1) && ((currMB->cuType == INxnNMB)  )))
              {
                CGLast = raster2ZZ_8x2[CGLastY * 2 + CGLastX];
              }
              else
              {
                CGLast = raster2ZZ_4x4[CGLastY * 4 + CGLastX];
              }

          }
          else
          {
            if (input->useNSQT && currMB->ui_MbBitSize > B8X8_IN_BIT &&(IS_INTER(currMB) && !(isChroma) && (currMB->trans_size == 1) && ((currMB->cuType == P2NXN) || (currMB->cuType == PHOR_UP) || (currMB->cuType == PHOR_DOWN))))
            {
              CGLast = raster2ZZ_2x8[CGLastY * 8 + CGLastX];
            }
            else if (input->useNSQT && currMB->ui_MbBitSize > B8X8_IN_BIT &&(IS_INTER(currMB) && !(isChroma) && (currMB->trans_size == 1) && ((currMB->cuType == PNX2N) || (currMB->cuType == PVER_LEFT) || (currMB->cuType == PVER_RIGHT))))
            {
              CGLast = raster2ZZ_8x2[CGLastY * 2 + CGLastX];
            }
            else
              if(input->useSDIP && (IS_INTRA(currMB) && !(isChroma) && (currMB->trans_size == 1) && ((currMB->cuType == InNxNMB)  )))
              {
                CGLast=1555555;
                printf("erro decoder run level \n");
              }
              else if(input->useSDIP && (IS_INTRA(currMB) && !(isChroma) && (currMB->trans_size == 1) && ((currMB->cuType == INxnNMB)  )))
              {
                CGLast=1555555;
                printf("erro decoder run level \n");
              }
              else
              {
                CGLast = raster2ZZ_8x8[CGLastY * 8 + CGLastX];
              }
          }
      }
        CGx=CGLastX;
        CGy=CGLastY;
      }  

      firstCG = (CGLast == 0);


      //! Sig CG Flag
      if( rank > 0 )
      {
        prevCGFlagSum = (iCG - 1 < 0 ? 0 : DCT_CGFlag[ iCG - 1 ])*2 + (iCG - 2 < 0 ? 0 : DCT_CGFlag[ iCG - 2 ]);
#if BBRY_CU8
        if (bitSize == 1)
#else
        if (bitSize == 0)
#endif
        {
#if !HZC_BUGFIX
          CGx = CGLast & 1;
          CGy = CGLast / 2;
#else 
          if (input->useNSQT && currMB->ui_MbBitSize > B8X8_IN_BIT &&(IS_INTER(currMB) && !(isChroma) && (currMB->trans_size == 1) && ((currMB->cuType == P2NXN) || (currMB->cuType == PHOR_UP) || (currMB->cuType == PHOR_DOWN))))
          {
            CGx=CGLast;
            CGy =0;
          }
          else if (input->useNSQT && currMB->ui_MbBitSize > B8X8_IN_BIT &&(IS_INTER(currMB) && !(isChroma) && (currMB->trans_size == 1) && ((currMB->cuType == PNX2N) || (currMB->cuType == PVER_LEFT) || (currMB->cuType == PVER_RIGHT))))
          {
            CGx= 0;
            CGy = CGLast ;
          }
          else if (input->useSDIP && (IS_INTRA(currMB) && !(isChroma) && (currMB->trans_size == 1) && ((currMB->cuType == InNxNMB)  )))
          {
            CGx=CGLast;
            CGy =0;
          }
          else if (input->useSDIP && (IS_INTRA(currMB) && !(isChroma) && (currMB->trans_size == 1) && ((currMB->cuType == INxnNMB)  )))
          {
            CGx= 0;
            CGy = CGLast ;
          }
          else
          {
            CGx = CGLast & 1;
            CGy = CGLast / 2;
          }
#endif
        }
#if BBRY_CU8
        else if (bitSize == 2)
#else
        else if (bitSize == 1)
#endif
        {
          if (input->useNSQT && currMB->ui_MbBitSize > B8X8_IN_BIT &&(IS_INTER(currMB) && !(isChroma) && (currMB->trans_size == 1) && ((currMB->cuType == P2NXN) || (currMB->cuType == PHOR_UP) || (currMB->cuType == PHOR_DOWN))))
          {
            CGx = AVS_SCAN2x8[CGLast][0];
            CGy = AVS_SCAN2x8[CGLast][1];
          }
          else if ((input->useNSQT && currMB->ui_MbBitSize > B8X8_IN_BIT &&IS_INTER(currMB) && !(isChroma) && (currMB->trans_size == 1) && ((currMB->cuType == PNX2N) || (currMB->cuType == PVER_LEFT) || (currMB->cuType == PVER_RIGHT))))
          {
            CGx = AVS_SCAN8x2[CGLast][0];
            CGy = AVS_SCAN8x2[CGLast][1];
          }
          else
            if(input->useSDIP && (IS_INTRA(currMB) && !(isChroma) && (currMB->trans_size == 1) && ((currMB->cuType == InNxNMB)  )))
            {
              CGx = AVS_SCAN2x8[CGLast][0];
              CGy = AVS_SCAN2x8[CGLast][1];
            }
            else if(input->useSDIP && (IS_INTRA(currMB) && !(isChroma) && (currMB->trans_size == 1) && ((currMB->cuType == INxnNMB)  )))
            {
              CGx = AVS_SCAN8x2[CGLast][0];
              CGy = AVS_SCAN8x2[CGLast][1];
            }
            else
            {
              CGx = AVS_SCAN4x4[CGLast][0];
              CGy = AVS_SCAN4x4[CGLast][1];
            }
        }
#if BBRY_CU8
        else if (bitSize == 3)
#else
        else if (bitSize == 2)
#endif
        {
#if !HZC_BUGFIX
          if (input->useNSQT && currMB->ui_MbBitSize > B8X8_IN_BIT &&(IS_INTER(currMB) && !(isChroma) && (currMB->trans_size == 1) && ((currMB->cuType == P2NXN) || (currMB->cuType == PHOR_UP) || (currMB->cuType == PHOR_DOWN))))
          {
            CGx = AVS_SCAN2x8[CGLast][0];
            CGy = AVS_SCAN2x8[CGLast][1];
          }
          else if ((input->useNSQT && currMB->ui_MbBitSize > B8X8_IN_BIT &&IS_INTER(currMB) && !(isChroma) && (currMB->trans_size == 1) && ((currMB->cuType == PNX2N) || (currMB->cuType == PVER_LEFT) || (currMB->cuType == PVER_RIGHT))))
          {
            CGx = AVS_SCAN8x2[CGLast][0];
            CGy = AVS_SCAN8x2[CGLast][1];
          }
          else
            if(input->useSDIP && (IS_INTRA(currMB) && !(isChroma) && (currMB->trans_size == 1) && ((currMB->cuType == InNxNMB)  )))
            {
              CGx = 1555555;
              CGy = 1555555;
              printf("erro decoder CGx  \n");
            }
            else if(input->useSDIP && (IS_INTRA(currMB) && !(isChroma) && (currMB->trans_size == 1) && ((currMB->cuType == INxnNMB)  )))
            {
              CGx = 1555555;
              CGy = 1555555;
              printf("erro decoder CGx  \n");
            }
            else
#endif 
            {
              CGx = AVS_SCAN8x8[CGLast][0];
              CGy = AVS_SCAN8x8[CGLast][1];
            }
        }
        sigCGctx = isChroma ? 0 : ( (CGLast == 0) ?  0 : 1 );

        {
          sigCGFlag  = biari_decode_symbol ( dep_dp, pCTXSigCG + sigCGctx );
          DCT_CGFlag[ iCG ]  = sigCGFlag;
        }
        DCT_CGNum ++;
      }

      if( sigCGFlag || rank == 0 )
      {
        pos = 0;
        pairsInCG = 0;
        pairs_prev = pairs;
        for ( i = 0; i < numOfCoeffInCG; i++, pairs++, pairsInCG++ )
        {
          pCTX = Primary[rank];

          //! LAST IN CG
          if(i == 0)
          {
            {

              symbol = 0;
              ctx = 0;
              if(se->context == LUMA_8x8)
              {
#if BBRY_CU8//???right?
#if !COEFF_BUGFIX
                offset = (bitSize == 0) ? (ctxmode/2)*4 : ((CGx > 0 && CGy > 0) ? 0 : (  ( ctxmode/2 ) * 4 +  ( firstCG ? 4 : 12 )) + 8);
#else 
                offset = (bitSize == 0) ? (ctxmode/2)*4 : (((CGx > 0 && CGy > 0) ? 0 : (  ( ctxmode/2 ) * 4 +  ( firstCG ? 4 : 12 ))) + 8);
#endif 
#else
                offset = ( bitSize > 0 ) ? ( 0 ) : ( (CGx > 0 && CGy > 0) ? 6 : ( firstCG * ( ctxmode/2 + 1 )* 6 + 12 ) );
#endif
                
              }
              else
              {
#if BBRY_CU8
                offset = (bitSize == 0) ? 0 : 4;

#else
                offset = ( bitSize > 0 ) ? ( 0 ) :  ( firstCG ? 6 : 12 ) ;
#endif
                
              }

              offset += (iCG==0 ? 0 : (isChroma ? NUM_LAST_POS_CTX_CHROMA/2 : NUM_LAST_POS_CTX_LUMA/2));

              while ( biari_decode_symbol ( dep_dp, pCTXLastPosInCG + offset + ctx ) == 0 )
              {
                symbol += 1;

#if EXTRACT_lcu_info_debug   
                if (p_lcu_debug != NULL)
                {
                  fprintf(p_lcu_debug, "symbol %d ctx=%d\n", symbol, ctx);
                }
#endif	
                if ( symbol == 3 ) break;

                ctx ++;
                if ( ctx >= 2 )
                {
                  ctx = 2;
                }
                if ( ctx >= 1 )
                {
                  ctx = 1;
                }
              }
              xx = symbol;

#if EXTRACT_lcu_info_debug   
              if (p_lcu_debug != NULL)
              {
                fprintf(p_lcu_debug, "last_coeff_pos_x %d\n", xx);
              }
#endif	
              symbol = 0;
              ctx = 0;
              while ( biari_decode_symbol ( dep_dp, pCTXLastPosInCG + offset + ctx + 2 ) == 0 )
              {
                symbol += 1;
                if ( symbol == 3 ) break;

                ctx ++;
                if ( ctx >= 2 )
                {
                  ctx = 2;
                }
                if ( ctx >= 1 )
                {
                  ctx = 1;
                }
              }
              yy = symbol;
#if EXTRACT_Coeff
              //EcuInfoInterSyntax[20+k+n]= currSE.value1;// 
              fprintf(p_lcu,"last_coeff_pos_x %d\n",xx);//
              fprintf(p_lcu,"last_coeff_pos_y %d\n",yy);//
              fflush(p_lcu);
#endif	
#if EXTRACT_lcu_info_debug   
              if (p_lcu_debug != NULL)
              {
                fprintf(p_lcu_debug, "last_coeff_pos_y %d\n", yy);//
              }
#endif	

#if BBRY_CU8
              if((CGx == 0 && CGy > 0 && ctxmode == 2 ) /*|| (ctxmode == 1)*/)
#else
              if( (CGx == 0 && CGy > 0) || (bitSize == 0 && firstCG && ( ctxmode == 1 )) )
#endif
              {
                yy=yy^xx;
                xx=yy^xx;
                yy=xx^yy;
              }
              if(rank!=0)
              {
                //if(ctxmode!=1)     
                {
                  xx = 3-xx;
                }
                if(ctxmode!=0)     
                {
                  yy = 3-yy;
                }
              }
              pos += 15-zigzag[yy][xx];

            }
          }

          if( pos == 16 )
          {
            break;
          }

          //! Level
          symbol = 0;

          indiv = min(2,(pairsInCG + 1)/2);
          pCTX = Primary[ min(rank,indiv+2) ];
#if BBRY_CU8
          offset = ((firstCG && pos > 12) ? 0 : 3) + indiv + 8;
#else
          offset = ((firstCG && pos > 12) ? 0 : 3) + indiv + 5;
#endif
        if (!isChroma)
        {
          offset += 3;
        }
#if !M3474_LEVEL
        while ( biari_decode_symbol ( dep_dp, pCTX + offset ) == 0 )
        {
          symbol += 1;
        }

        absLevel = symbol + 1;
#else
        band=biari_decode_final(dep_dp);
#if EXTRACT_Coeff
        //EcuInfoInterSyntax[20+k+n]= currSE.value1;// 
        fprintf(p_lcu,"coeff_level_minus_band %d\n",band);//
				fflush(p_lcu);
#endif	

        if (band)
        {
          golomb_order=0;
          binary_symbol=0;
          do
          {
            l = biari_decode_symbol_eq_prob ( dep_dp );

            if ( l == 0 )
            {
              symbol += ( 1 << golomb_order );
              golomb_order++;
            }
          }
          while ( l != 1 );

          while ( golomb_order-- )
          {
            //next binary part
            sig = biari_decode_symbol_eq_prob(dep_dp) ;

            if ( sig == 1 )
            {
              binary_symbol |= ( 1 << golomb_order );
            }
          }

          symbol += binary_symbol;
          symbol += 32;

        }
        else 
        {   
          bins=0;        
          while (  (biari_decode_symbol ( dep_dp, pCTX + offset ) == 0)  )
          {
            symbol += 1;
            bins++;
            if (bins==31)
              break;
          }

        }
        absLevel = symbol + 1;
#endif 
        Level = absLevel;
#if EXTRACT_Coeff
        //EcuInfoInterSyntax[20+k+n]= currSE.value1;// 
        fprintf(p_lcu,"Level %d\n",Level);//
				fflush(p_lcu);
#endif	

#if TRACE
          //tracebits2("level", 1, Level);
#endif
          absSum5 = 0;
          n = 0;
          for( k = pairs - 1; k >= pairs - pairsInCG; k -- )
          {
            n += DCT_Run[k];
            if( n >= 6 )
              break;
            absSum5 += abs(DCT_Level[k]);
            n ++;
          }

          pCTX = Primary[ min((absSum5+absLevel)/2, 2) ];
#if !BBRY_CU8   
          offset = (iCG == 0 && firstCG ? 0  + (pos > 12 ? 0 : 2) : 4 + (firstCG ? 0 : 2));
#endif

          //! Run
          symbol = 0;
          if( pos < numOfCoeffInCG - 1 )
          {
            ctxpos = 0;
            if(15-pos>0)
            {
              px = AVS_SCAN4x4[15-pos-1-ctxpos][0];
              py = AVS_SCAN4x4[15-pos-1-ctxpos][1];
			        moddiv = (ctxmode == INTRA_PRED_VER)? (py >> 1) :(/*(ctxmode == INTRA_PRED_HOR)?(px >> 1):*/(pos+ctxpos <= 9));
              // moddiv = (ctxmode == INTRA_PRED_VER)? (py >> 1) :((ctxmode == INTRA_PRED_HOR)?(px >> 1):(pos+ctxpos <= 9));
#if BBRY_CU8
#if !HZC_COEFF_RUN_FIX
              offset = (firstCG ? (pos+ctxpos == 14 ? 0 : (1 + moddiv)) : (se->context != LUMA_8x8 ? 2 : 3 + moddiv)) + (bitSize == 0 ? 0 : 3);
#else 
              offset = (firstCG ? (pos+ctxpos == 14 ? 0 : (1 + moddiv)) : (se->context == LUMA_8x8 ? 2 : 3 + moddiv)) + (bitSize == 0 ? 0 : 3);
#endif 
#else
              offset = (firstCG ? (pos+ctxpos == 14 ? 0 : (1 + moddiv)) : (se->context != LUMA_8x8 ? 2 : 3 + moddiv));
#endif
              if(se->context == LUMA_8x8)
              {
                moddiv = (ctxmode == INTRA_PRED_VER)? ((py+1)/2) :(/*(ctxmode == INTRA_PRED_HOR)?(((px+1)/2)+3):*/((pos+ctxpos) > 11 ? 6 : ((pos+ctxpos) > 4 ? 7:8)));
                offset = (firstCG ? (pos+ctxpos == 14 ? 0 : (1 + moddiv%3)) :  (4 + moddiv%3)) + (bitSize == 0 ? 0 : 4);
              }
            }

            while ( biari_decode_symbol ( dep_dp, pCTX + offset ) == 0 )
            {
              symbol += 1;

              if( symbol == numOfCoeffInCG - 1 - pos )
              {
                break;
              }

              ctxpos++;
              if( (15-pos-1-ctxpos)>=0 )
              {
                px = AVS_SCAN4x4[15-pos-1-ctxpos][0];
                py = AVS_SCAN4x4[15-pos-1-ctxpos][1];
                moddiv = (ctxmode == INTRA_PRED_VER)? (py >> 1) :(/*(ctxmode == INTRA_PRED_HOR)?(px >> 1):*/(pos+ctxpos <= 9));
#if BBRY_CU8
#if !HZC_COEFF_RUN_FIX
                offset = (firstCG ? (pos+ctxpos == 14 ? 0 : (1 + moddiv)) : (se->context != LUMA_8x8 ? 2 : 3 + moddiv)) + (bitSize == 0 ? 0 : 3);
#else 
                offset = (firstCG ? (pos+ctxpos == 14 ? 0 : (1 + moddiv)) : (se->context == LUMA_8x8 ? 2 : 3 + moddiv)) + (bitSize == 0 ? 0 : 3);
#endif 
#else
                offset = (firstCG ? (pos+ctxpos == 14 ? 0 : (1 + moddiv)) : (se->context != LUMA_8x8 ? 2 : 3 + moddiv));
#endif
                if(se->context == LUMA_8x8)
                {
                  moddiv = (ctxmode == INTRA_PRED_VER)? ((py+1)/2) :(/*(ctxmode == INTRA_PRED_HOR)?(((px+1)/2)+3):*/((pos+ctxpos) > 11 ? 6 : ((pos+ctxpos) > 4 ? 7:8)));
                  offset = (firstCG ? (pos+ctxpos == 14 ? 0 : (1 + moddiv%3)) :  (4 + moddiv%3)) + (bitSize == 0 ? 0 : 4);
                }
              }
            }
          }


          Run = symbol;
#if EXTRACT_Coeff
          //EcuInfoInterSyntax[20+k+n]= currSE.value1;// 
          fprintf(p_lcu,"Run %d\n",Run);//
          fflush(p_lcu);
#endif			  
#if TRACE
          //tracebits2("run", 1, Run);
#endif
          DCT_Level[pairs] = Level;
          DCT_Run[pairs] = Run;

          DCT_PairsInCG[ iCG ] ++;

          if ( absLevel > T_Chr[rank] )
          {
            if ( absLevel <= 2 )
            {
              rank = absLevel;
            }
            else if ( absLevel <= 4 )
            {
              rank = 3;
            }
            else
            {
              rank = 4;
            }
          }
          if( Run == numOfCoeffInCG - 1 - pos )
          {
            pairs ++;
            pairsInCG ++;
            break;
          }

          pos += ( Run + 1 );

        }
        //Sign of Level
        pairs = pairs_prev;
        for( i = DCT_PairsInCG[ iCG ]; i > 0; i--, pairs++ )
        {
          if ( biari_decode_symbol_eq_prob ( dep_dp ) )
          {
#if EXTRACT_Coeff
            //EcuInfoInterSyntax[20+k+n]= currSE.value1;// 
            fprintf(p_lcu,"sign %d\n",-1);//
            fflush(p_lcu);
#endif			
            DCT_Level[pairs] = - DCT_Level[pairs];
          }
          else
          {
#if EXTRACT_Coeff
            //EcuInfoInterSyntax[20+k+n]= currSE.value1;// 
            fprintf(p_lcu,"sign %d\n",1);//
            fflush(p_lcu);
#endif			
            DCT_Level[pairs] =  DCT_Level[pairs];
          }
#if EXTRACT_lcu_info_coeff_debug
          fprintf(p_lcu_debug, "DCT_Level[%2d]=%d\n", pairs, DCT_Level[pairs]);
#endif
        }
      }

      CGLast --;

      if( firstCG )
      {        
        break;
      }
    }
    DCT_Pairs = pairs;
    Pair_Pos = DCT_Pairs;

#if EXTRACT_lcu_info_coeff_debug
    fprintf(p_lcu_debug, "DCT_Pairs=%d \n", DCT_Pairs);
#endif
#if EXTRACT_Coeff
    //EcuInfoInterSyntax[20+k+n]= currSE.value1;// 
    fprintf(p_lcu,"DCT_Pairs %d\n",DCT_Pairs);//
    fflush(p_lcu);
#endif	

  }
  //--- set run and level ---
  if ( DCT_Pairs > 0 )
  {
    se->value1 = DCT_Level[Pair_Pos - 1];
    se->value2 = DCT_Run[Pair_Pos - 1];
    Pair_Pos --;
  }
  else
  {
    //--- set run and level (EOB) ---
    se->value1 = se->value2 = 0;
  }

  //--- decrement coefficient counter and re-set position ---
  if ( ( DCT_Pairs-- ) == 0 )
  {
    Pair_Pos = 0;
  }
}

/*!
************************************************************************
* \brief
*    arithmetic decoding
************************************************************************
*/
int readSyntaxElement_AEC ( SyntaxElement *se, DataPartition *this_dataPart,  codingUnit *MB, int uiPosition )

{
  int curr_len;
  DecodingEnvironmentPtr dep_dp = & ( this_dataPart->de_AEC );

  curr_len = arideco_bits_read ( dep_dp );

  // perform the actual decoding by calling the appropriate method
  se->reading ( se, dep_dp, MB, uiPosition );

  return ( se->len = ( arideco_bits_read ( dep_dp ) - curr_len ) );
}

/*!
************************************************************************
* \brief
*    decoding of unary binarization using one or 2 distinct
*    models for the first and all remaining bins; no terminating
*    "0" for max_symbol
***********************************************************************
*/
unsigned int unary_bin_max_decode ( DecodingEnvironmentPtr dep_dp, BiContextTypePtr ctx, int ctx_offset, unsigned int max_symbol )
{
  unsigned int l;
  unsigned int symbol;
  BiContextTypePtr ictx;

  symbol =  biari_decode_symbol ( dep_dp, ctx );
#if UNIFY_BINARIZATION
  if ( symbol == 1 )
  {
    return 0;
  }
  else
  {
    if ( max_symbol == 1 )
    {
      return symbol;
    }

    symbol = 0;
    ictx = ctx + ctx_offset;

    do
    {
      l = biari_decode_symbol ( dep_dp, ictx );
      symbol++;
    }
    while ( ( l != 1 ) && ( symbol < max_symbol - 1 ) );

    if ( ( l != 1 ) && ( symbol == max_symbol - 1 ) )
    {
      symbol++;
    }

    return symbol;
  }
#else
  if ( symbol == 0 )
  {
    return 0;
  }
  else
  {
    if ( max_symbol == 1 )
    {
      return symbol;
    }

    symbol = 0;
    ictx = ctx + ctx_offset;

    do
    {
      l = biari_decode_symbol ( dep_dp, ictx );
      symbol++;
    }
    while ( ( l != 0 ) && ( symbol < max_symbol - 1 ) );

    if ( ( l != 0 ) && ( symbol == max_symbol - 1 ) )
    {
      symbol++;
    }

    return symbol;
  }
#endif
}


/*!
************************************************************************
* \brief
*    decoding of unary binarization using one or 2 distinct
*    models for the first and all remaining bins
***********************************************************************
*/
unsigned int unary_bin_decode ( DecodingEnvironmentPtr dep_dp, BiContextTypePtr ctx, int ctx_offset )
{
  unsigned int l;
  unsigned int symbol;
  BiContextTypePtr ictx;

  symbol = 1 - biari_decode_symbol ( dep_dp, ctx );
#if EXTRACT_lcu_info_cutype_debug
  //此时的act_sym是码字中连续0的个数
  fprintf(p_lcu_debug, " symbol=%d\n",  1 - symbol);//cu_type_index
#endif	

  if ( symbol == 0 )
  {
    return 0;
  }
  else
  {
    symbol = 0;
    ictx = ctx + ctx_offset;

    do
    {
      l = 1 - biari_decode_symbol ( dep_dp, ictx );
#if EXTRACT_lcu_info_cutype_debug
      //此时的act_sym是码字中连续0的个数
      fprintf(p_lcu_debug, " symbol=%d\n", 1 - l);//cu_type_index
#endif	
      symbol++;
    }
    while ( l != 0 );

    return symbol;
  }
}


/*!
************************************************************************
* \brief
*    finding end of a slice in case this is not the end of a frame
*
* Unsure whether the "correction" below actually solves an off-by-one
* problem or whether it introduces one in some cases :-(  Anyway,
* with this change the bit stream format works with AEC again.
* StW, 8.7.02
************************************************************************
*/
int AEC_startcode_follows ( int eos_bit )
{
  Slice         *currSlice  = img->currentSlice;
  DataPartition *dP;
  unsigned int  bit;
  DecodingEnvironmentPtr dep_dp;

  dP = & ( currSlice->partArr[0] );
  dep_dp = & ( dP->de_AEC );

  if ( eos_bit )
  {
    bit = biari_decode_final ( dep_dp ); //GB

#if TRACE
    //  strncpy(se->tracestring, "Decode Sliceterm", TRACESTRING_SIZE);
    fprintf ( p_trace, "@%d %s\t\t%d\n", symbolCount++, "Decode Sliceterm", bit );
    fflush ( p_trace );
#endif
  }
  else
  {
    bit = 0;
  }

  return ( bit == 1 ? 1 : 0 );
}

int readSplitFlag ( int uiBitSize )
{
  Slice *currSlice    = img->currentSlice;
  DataPartition *dP;
  SyntaxElement currSE;

  dP = & ( currSlice->partArr[0] );
  currSE.value2 = uiBitSize;
  currSE.reading = readSplitFlag_AEC;
  dP->readSyntaxElement ( &currSE, dP, NULL, 0 );
#if TRACE
  fprintf ( p_trace, "SplitFlag = %3d\n", currSE.value1 );
  fflush ( p_trace );
#endif

  return currSE.value1;
}

extern unsigned int t1, value_t;
extern unsigned char s1, value_s;
void readSplitFlag_AEC ( SyntaxElement *se, DecodingEnvironmentPtr dep_dp, codingUnit *MB, int uiPosition )
{
  BiContextTypePtr pCTX;
  int ctx, symbol;
  pCTX = img->currentSlice->syn_ctx->split_contexts;
  symbol = 0;
  ctx = se->value2;

#if !D20141230_BUG_FIX
  if ( biari_decode_symbol ( dep_dp, pCTX + ctx ) == 0 )
  {
    symbol = 1;
  }
  else
  {
    symbol = 0;
  }
#else 

  symbol =  biari_decode_symbol ( dep_dp, pCTX + ctx );
#if EXTRACT_lcu_info_cutype_debug
  //此时的act_sym是码字中连续0的个数
  fprintf(p_lcu_debug, "readSplitFlag_AEC ctx=%d symbol=%d\n", ctx, symbol);//cu_type_index
#endif	
  
#endif 

  se->value1 = symbol;
}

int read_sao_mergeflag( int mergeleft_avail, int mergeup_avail, int uiPositionInPic)
{
	int MergeLeft = 0;
	int MergeUp = 0;
	Slice *currSlice    = img->currentSlice;
	SyntaxElement   currSE ;
	DataPartition*  dataPart;	
	dataPart = & ( currSlice->partArr[0] );

	currSE.reading = read_sao_mergeflag_AEC;
	currSE.value2 = mergeleft_avail + mergeup_avail;
	dataPart->readSyntaxElement (&currSE, dataPart,  NULL, uiPositionInPic );
	assert(currSE.value1<=2);	
	if (mergeleft_avail)
	{
		MergeLeft = currSE.value1 & 0x01;
		currSE.value1 = currSE.value1 >> 1;
	}
	if (mergeup_avail && !MergeLeft)
	{
		MergeUp = currSE.value1 & 0x01;
	} 
	return (MergeLeft << 1) + MergeUp;
}
void read_sao_mergeflag_AEC( SyntaxElement* se,  DecodingEnvironmentPtr dep_dp,codingUnit* currMB,int uiPositionInPic)
{
	SyntaxInfoContexts  *ctx    = img->currentSlice->syn_ctx;
	int act_ctx = se->value2;
	int act_sym;
	if (act_ctx==1)
	{
		//assert(act_sym<=1);
		act_sym = biari_decode_symbol ( dep_dp, &ctx->saomergeflag_context[0]);
	}
	else
		if (act_ctx==2)
		{
			act_sym = biari_decode_symbol ( dep_dp,  &ctx->saomergeflag_context[1]);
#if SAO_MERGE_OPT
			if (act_sym!=1)
#endif 
			act_sym += (biari_decode_symbol ( dep_dp, &ctx->saomergeflag_context[2]) << 1);
		}

		se->value1 = act_sym;
}
int read_sao_mode(int uiPositionInPic)
{
	Slice *currSlice    = img->currentSlice;
	SyntaxElement   currSE ;
	DataPartition*  dataPart;
	dataPart = & ( currSlice->partArr[0] );

	currSE.reading = read_sao_mode_AEC;
	dataPart->readSyntaxElement ( &currSE, dataPart,NULL, uiPositionInPic );	
	return currSE.value1; 

}
void read_sao_mode_AEC(SyntaxElement* se, DecodingEnvironmentPtr dep_dp,codingUnit* currMB, int uiPositionInPic)
{
	SyntaxInfoContexts  *ctx    = img->currentSlice->syn_ctx;
	int  act_sym = 0;
	int t1,t2=0;
#if UNIFY_BINARIZATION
  t2 = !biari_decode_symbol(dep_dp,ctx->saomode_context );
  if(t2)
  {
    t1 = !biari_decode_symbol_eq_prob(dep_dp);
    act_sym = t2  +  (t1 << 1);
  }
#else
	t2 = biari_decode_symbol(dep_dp,ctx->saomode_context );
	if(t2)
	{
		t1 = biari_decode_symbol_eq_prob(dep_dp);
		act_sym = t2  +  (t1 << 1);
	}
#endif
	se->value1 = act_sym;
}
int read_sao_offset(SAOBlkParam* saoBlkParam, int uiPositionInPic, int offsetTh,int* offset)
{
	Slice *currSlice    = img->currentSlice;
	SyntaxElement   currSE ;
	DataPartition*  dataPart;
	int i;
	dataPart = & ( currSlice->partArr[0] );
  assert(saoBlkParam->modeIdc == SAO_MODE_NEW);

  currSE.reading = read_sao_offset_AEC;

  for ( i = 0; i < 4; i++)
	{
		if(saoBlkParam->typeIdc == SAO_TYPE_BO)
		{
			currSE.value2 = SAO_CLASS_BO;
		}
		else
		{
			currSE.value2 = (i>=2) ? (i + 1) : i;
		}
    dataPart->readSyntaxElement (  &currSE, dataPart, NULL,uiPositionInPic );
		offset[i] = currSE.value1;
		
	}
	return 1;
}
void read_sao_offset_AEC( SyntaxElement* se, DecodingEnvironmentPtr dep_dp,codingUnit* currMB,int uiPositionInPic)
{
	SyntaxInfoContexts  *ctx    = img->currentSlice->syn_ctx;
	int  act_sym,sym,cnt ;
	int signsymbol=0;
	int maxvalue;
	int offset_type = se->value2;

	maxvalue = saoclip[offset_type][2];
	cnt=0;

#if UNIFY_BINARIZATION
  if (offset_type == SAO_CLASS_BO)
  {
    sym = !biari_decode_symbol ( dep_dp,  &ctx->saooffset_context[0]);
  }
  else
  {
    sym  = !biari_decode_symbol_eq_prob( dep_dp);
  }
  while(sym)
  {
    cnt++;
    if (cnt == maxvalue)
    {
      break;
    }
    sym = !biari_decode_symbol_eq_prob( dep_dp);
  }
#else
	if (offset_type == SAO_CLASS_BO)
	{
		sym = biari_decode_symbol ( dep_dp,  &ctx->saooffset_context[0]);
	}
	else
	{
		sym  = biari_decode_symbol_eq_prob( dep_dp);
	}
	while(sym)
	{
		cnt++;
		if (cnt == maxvalue)
		{
			break;
		}
		sym = biari_decode_symbol_eq_prob( dep_dp);
	}
#endif
	if(offset_type == SAO_CLASS_EO_FULL_VALLEY)
	{
		act_sym = EO_OFFSET_INV__MAP[cnt];
	}
	else if(offset_type == SAO_CLASS_EO_FULL_PEAK)
	{
		act_sym = -EO_OFFSET_INV__MAP[cnt];
	}
	else if(offset_type == SAO_CLASS_EO_HALF_PEAK)
	{
		act_sym = -cnt;
	}
	else 
	{
		act_sym = cnt;
	}

	if (offset_type == SAO_CLASS_BO && act_sym)
	{
		signsymbol = biari_decode_symbol_eq_prob(dep_dp);
	}
	if (signsymbol)
	{
		act_sym = -act_sym;
	}
	se->value1 = act_sym;
}
int read_sao_type(SAOBlkParam* saoBlkParam, int uiPositionInPic)
{
	Slice *currSlice    = img->currentSlice;
	SyntaxElement   currSE ;
	DataPartition*  dataPart;
  int stBnd[2], i;
	dataPart = & ( currSlice->partArr[0] );
  	assert(saoBlkParam->modeIdc == SAO_MODE_NEW);
	if (saoBlkParam->typeIdc == SAO_TYPE_BO)
	{
		currSE.value2 = 1;
		currSE.reading = read_sao_type_AEC;
		dataPart->readSyntaxElement ( &currSE, dataPart,NULL, uiPositionInPic );
		stBnd[0] = currSE.value1;

		currSE.value2 = 2;//read delta start band for BO
		currSE.reading = read_sao_type_AEC;
		dataPart->readSyntaxElement ( &currSE, dataPart,NULL, uiPositionInPic );
		stBnd[1] = currSE.value1 + 2;
    return ( stBnd[0] + (stBnd[1] << NUM_SAO_BO_CLASSES_LOG2) );
	} 
	else
	{
		assert(saoBlkParam->typeIdc == SAO_TYPE_EO_0);
		currSE.value2 = 0;
    currSE.reading = read_sao_type_AEC;
    dataPart->readSyntaxElement ( &currSE, dataPart,NULL, uiPositionInPic );
    return currSE.value1;
	}
}
void read_sao_type_AEC( SyntaxElement* se, DecodingEnvironmentPtr dep_dp,codingUnit* currMB,int uiPositionInPic)
{
	SyntaxInfoContexts  *ctx    = img->currentSlice->syn_ctx;
	int  act_sym=0;
	int temp;
	int i,length;
	int cmax;
	int golomb_order, rest;
	temp = 0;

	golomb_order = 1;
	if(se->value2 == 0)
	{
		length = NUM_SAO_EO_TYPES_LOG2;
	}
	else if(se->value2 == 1) 
	{
		length = NUM_SAO_BO_CLASSES_LOG2;
	}
	else
	{
		length = NUM_SAO_BO_CLASSES_LOG2 - 1;
		assert(se->value2 == 2);

	}
	if(se->value2 == 2)
	{

		do
		{
			temp = biari_decode_symbol_eq_prob ( dep_dp );

			if ( temp == 0 )
			{
				act_sym += ( 1 << golomb_order );
				golomb_order++;
			}

#if SAO_INTERVAL_OPT
			if (golomb_order==4)
			{
				golomb_order=0;
				temp=1;
			}
#endif 

		}
		while ( temp != 1 );
		rest = 0;



		while ( golomb_order-- )
		{
      //next binary part
			temp= biari_decode_symbol_eq_prob ( dep_dp );

			if ( temp == 1 )
			{
				 rest |= ( temp << golomb_order );
			}
		}

		act_sym += rest;
		se->value1 = act_sym;
	}
	else
	{
		for ( i = 0; i < length; i++)
		{
			act_sym = act_sym  + (biari_decode_symbol_eq_prob(dep_dp) << i);
		}
		se->value1 = act_sym;
	}
}

void readAlfCoeff(ALFParam* Alfp)
{
	int pos;

	int f = 0,symbol,pre_symbole;
	const int numCoeff = (int)ALF_MAX_NUM_COEF;

	switch(Alfp->componentID)
	{
	case ALF_Cb:
	case ALF_Cr:
		{
			for(pos=0; pos< numCoeff; pos++)
			{
				Alfp->coeffmulti[0][pos] = se_v("Chroma ALF coefficients");
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
        if (Alfp->componentID == ALF_Cb)
        {
#if  EXTRACT_FULL_PPS_ALF_debug  //2016-5-31 提取完整的参数
          fprintf(p_pps, "Cb coeffmulti[%2d][%d]%08X\n", f, pos, Alfp->coeffmulti[0][pos]);
#endif
          pps_AlfCoeffChroma0[pos] = Alfp->coeffmulti[0][pos];//16个
        }
        else
        {
#if  EXTRACT_FULL_PPS_ALF_debug  //2016-5-31 提取完整的参数
          fprintf(p_pps, "Cr coeffmulti[%2d][%d]%08X\n", f, pos, Alfp->coeffmulti[0][pos]);
#endif
          pps_AlfCoeffChroma1[pos] = Alfp->coeffmulti[0][pos];//16个
        }
#endif
			}
		}
		break;
	case ALF_Y:
		{			
			Alfp->filters_per_group = ue_v("ALF filter number");

#if  EXTRACT_FULL_PPS_ALF_debug  //2016-5-31 提取完整的参数
      fprintf(p_pps, "Y ALF filter number -1=%08X\n", Alfp->filters_per_group);
#endif
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
      pps_alf_filter_num_minus1 = Alfp->filters_per_group;
#endif
			Alfp->filters_per_group = Alfp->filters_per_group+1;

			memset(Alfp->filterPattern,0,NO_VAR_BINS * sizeof(int));
			pre_symbole = 0;
			symbol = 0;
			for(f=0; f< Alfp->filters_per_group; f++)
			{

#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
        pps_alf_region_distance[f]=0;//16个
#endif
				if (f > 0)
				{
#if !ALF_REGION_OPT
					symbol = ue_v("Region distance");
#else 
          if (Alfp->filters_per_group!=16)
					{
					  symbol = ue_v("Region distance");
#if  EXTRACT_FULL_PPS_ALF_debug  //2016-5-31 提取完整的参数
            fprintf(p_pps, "Y Region distance symbol=%08X\n", symbol);
#endif
					}
					else 
					{
            symbol = 1;
					}
#endif 

#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
          pps_alf_region_distance[f] = symbol;//16个
#endif
					Alfp->filterPattern[symbol + pre_symbole] = 1;
					pre_symbole = symbol + pre_symbole;
				}

				for( pos=0; pos< numCoeff; pos++)
				{
					Alfp->coeffmulti[f][pos] = se_v("Luma ALF coefficients");

#if  EXTRACT_FULL_PPS_ALF_debug  //2016-5-31 提取完整的参数
          fprintf(p_pps, "Y coeffmulti[%2d][%d]%08X\n",f,pos, Alfp->coeffmulti[f][pos]);
#endif
#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
          pps_AlfCoeffLuma[f][pos] = Alfp->coeffmulti[f][pos];//16个
#endif
				}
			}
		}
		break;
	default:
		{
			printf("Not a legal component ID\n");
			assert(0);
			exit(-1);
		}
	}
}


unsigned int readAlfLCUCtrl (ImageParameters *img,DecodingEnvironmentPtr dep_dp,int compIdx,int ctx_idx)
{
	unsigned int ruiVal;
	SyntaxInfoContexts  *ctx    = img->currentSlice->syn_ctx;
#if !M3544_CONTEXT_OPT
	ruiVal=biari_decode_symbol(dep_dp,&(ctx->m_cALFLCU_Enable_SCModel[compIdx][ctx_idx]));
#else 
	ruiVal=biari_decode_symbol(dep_dp,&(ctx->m_cALFLCU_Enable_SCModel[0][0]));
#endif 

	return ruiVal;

}
