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
#include "math.h"
#include "global.h"
#include "../../lcommon/inc/commonVariables.h"
#include "../../lcommon/inc/memalloc.h"
#include <assert.h>



//AC ENGINE PARAMETERS
unsigned int t1, value_t;
unsigned char s1, value_s;
unsigned char cFlag; 

unsigned char value_s_bound = NUN_VALUE_BOUND;
unsigned char is_value_bound;
unsigned int max_value_s = 0;
#if Decoder_BYPASS_Final
unsigned char is_value_domain;//  is value in R domain 1 is R domain 0 is LG domain 
#endif
extern int symbolCount;

int binCount = 0;

#define Dbuffer         (dep->Dbuffer)
#define Dbits_to_go     (dep->Dbits_to_go)
#define Dcodestrm       (dep->Dcodestrm)
#define Dcodestrm_len   (dep->Dcodestrm_len)

#define B_BITS  10

#define LG_PMPS_SHIFTNO 2

#define HALF      (1 << (B_BITS-1))
#define QUARTER   (1 << (B_BITS-2))


/************************************************************************
* M a c r o s
************************************************************************
*/

// yin hai cang
#define get_byte(){                                         \
    Dbuffer = Dcodestrm[(*Dcodestrm_len)++];\
    Dbits_to_go = 7;                        \
  }
// yin hai cang


/************************************************************************
************************************************************************
init / exit decoder
************************************************************************
************************************************************************/


/*!
************************************************************************
* \brief
*    Allocates memory for the DecodingEnvironment struct
* \return DecodingContextPtr
*    allocates memory
************************************************************************
*/

/*!
************************************************************************
* \brief
*    Initializes the DecodingEnvironment for the arithmetic coder
************************************************************************
*/


void arideco_start_decoding ( DecodingEnvironmentPtr dep, unsigned char *cpixcode, int firstbyte, int *cpixcode_len, int slice_type )
{
  Dcodestrm = cpixcode;
  Dcodestrm_len = cpixcode_len;
  *Dcodestrm_len = firstbyte;

  s1 = 0;
  t1 = QUARTER - 1; //0xff
  value_s = 0;

  value_t = 0;

  {
    int i;
    Dbits_to_go = 0;

    for ( i = 0; i < B_BITS - 1 ; i++ )
    {
      if ( --Dbits_to_go < 0 )
      {
        get_byte();
      }

      value_t = ( value_t << 1 )  | ( ( Dbuffer >> Dbits_to_go ) & 0x01 );
    }
  }
#if Decoder_BYPASS_Final
  is_value_domain=1;
#else
  while ( value_t < QUARTER && value_s < value_s_bound)
  {
    if ( --Dbits_to_go < 0 )
    {
      get_byte();
	}

    // Shift in next bit and add to value
    value_t = ( value_t << 1 ) | ( ( Dbuffer >> Dbits_to_go ) & 0x01 );
    value_s++;
  }
  if(value_t < QUARTER)
    is_value_bound = 1;
  else
    is_value_bound = 0;    
  value_t = value_t & 0xff;
#endif
  cFlag = 1;
}



/*!
************************************************************************
* \brief
*    arideco_bits_read
************************************************************************
*/
int arideco_bits_read ( DecodingEnvironmentPtr dep )
{
  return 8 * ( ( *Dcodestrm_len ) - 1 ) + ( 8 - Dbits_to_go );
}

void update_ctx (DecodingEnvironmentPtr dep, BiContextTypePtr bi_ct, int is_LPS)
{
  register unsigned char cwr, cycno = bi_ct->cycno;
  register unsigned int lg_pmps = bi_ct->LG_PMPS;


  cwr = ( cycno <= 1 ) ? 3 : ( cycno == 2 ) ? 4 : 5; //FAST ADAPTION PARAMETER
  //update other parameters
  if ( is_LPS )
  {
    cycno = ( cycno <= 2 ) ? ( cycno + 1 ) : 3;
  }
  else if ( cycno == 0 )
  {
    cycno = 1;
  }

  bi_ct->cycno = cycno;

  //update probability estimation
  if ( is_LPS )
  {
    switch ( cwr )
    {
    case 3:
      lg_pmps = lg_pmps + 197;
      break;
    case 4:
      lg_pmps = lg_pmps + 95;
      break;
    default:
      lg_pmps = lg_pmps + 46;
    }

    if ( lg_pmps >= ( 256 << LG_PMPS_SHIFTNO ) )
    {
      lg_pmps = ( 512 << LG_PMPS_SHIFTNO ) - 1 - lg_pmps;
      bi_ct->MPS = ! ( bi_ct->MPS );
    }
  }
  else
  {
    lg_pmps = lg_pmps - ( unsigned int ) ( lg_pmps >> cwr ) - ( unsigned int ) ( lg_pmps >> ( cwr + 2 ) );
  }
#if EXTRACT
  fprintf(p_aec,"%d\n",lg_pmps);
  fprintf(p_aec,"%d\n",cycno);
#endif
  bi_ct->LG_PMPS = lg_pmps;

#if EXTRACT_lcu_info_cutype_aec_debug2   
  if (p_lcu_debug != NULL)
  {
    fprintf(p_lcu_debug, "update_ctx %3d", bi_ct->MPS);
    fprintf(p_lcu_debug, "%5d", bi_ct->cycno);
    fprintf(p_lcu_debug, "%5d\n", bi_ct->LG_PMPS);
  }
#endif	
}


unsigned int biari_decode_symbol ( DecodingEnvironmentPtr dep, BiContextTypePtr bi_ct )
{
  register unsigned char bit ;
  register unsigned char s_flag, is_LPS = 0;
  register unsigned int lg_pmps = bi_ct->LG_PMPS >> LG_PMPS_SHIFTNO;
  register unsigned int t_rlps;
  register unsigned int t2;
  register unsigned char s2;
  register unsigned char cycno = bi_ct->cycno;


  bit = bi_ct->MPS;
#if EXTRACT_lcu_info_cutype_aec_debug   
  if (p_lcu_debug!=NULL) 
  {
    fprintf(p_lcu_debug, "%d start\n", value_t);
    fprintf(p_lcu_debug, "%d\n", t1);
    fprintf(p_lcu_debug, "%d\n", s1);
    fprintf(p_lcu_debug, "%d\n", value_s);
    fprintf(p_lcu_debug, "%d\n", bi_ct->MPS);
    fprintf(p_lcu_debug, "%d\n", bi_ct->cycno);
    fprintf(p_lcu_debug, "%d\n", bi_ct->LG_PMPS);
  }
#endif	

#if EXTRACT_lcu_info_cutype_aec_debug2   
  if (p_lcu_debug != NULL)
  {
    fprintf(p_lcu_debug, "start %6d ", value_t);
    fprintf(p_lcu_debug, "%6d", value_s);
    fprintf(p_lcu_debug, "%6d", t1);
    fprintf(p_lcu_debug, "%6d", s1);
    fprintf(p_lcu_debug, "%3d", bi_ct->MPS);
    fprintf(p_lcu_debug, "%5d", bi_ct->cycno);
    fprintf(p_lcu_debug, "%5d\n", bi_ct->LG_PMPS);
  }
#endif	
#if EXTRACT
	fprintf(p_aec,"%d\n",value_t);
	fprintf(p_aec,"%d\n",t1);
	fprintf(p_aec,"%d\n",lg_pmps);
	fprintf(p_aec,"%d\n",cycno);
	fprintf(p_aec,"%d\n",s1);
	fprintf(p_aec,"%d\n",value_s);
	fflush(p_aec);
#endif

#if Decoder_BYPASS_Final
  if(is_value_domain == 1 || (s1 == value_s_bound && is_value_bound == 1) )//value_t is in R domain  s1=0 or s1 == value_s_bound
  {
	  s1 = 0; 
	  value_s = 0;

	  while ( value_t < QUARTER && value_s < value_s_bound)
	  {
		  int j;
		  if ( --Dbits_to_go < 0 )
		  {
			  get_byte();
		  }
		  j = ( Dbuffer >> Dbits_to_go ) & 0x01;
		  // Shift in next bit and add to value

		  value_t = ( value_t << 1 ) | j;
		  value_s++;
	  }
	  if(value_t < QUARTER)
		  is_value_bound = 1;
	  else
		  is_value_bound = 0;  

	  value_t = value_t & 0xff;
  }
#endif

  if ( t1 >=  lg_pmps )
  {
    s2 = s1;
    t2 = t1 -  lg_pmps ; //8bits
    s_flag = 0;
  }
  else
  {
    s2 = s1 + 1;
    t2 = 256 + t1 - lg_pmps ; //8bits
    s_flag = 1;
  }

  if(value_s>value_s_bound)
  {
    printf("value_s:%d\n",value_s);
	  exit(1);
  }
  if (value_s > max_value_s)
    max_value_s = value_s;

  if ( (s2 > value_s || ( s2 == value_s && value_t >= t2 ) ) && is_value_bound == 0) //LPS
  {
    is_LPS = 1;
    bit = !bit; //LPS
#if Decoder_BYPASS_Final
	is_value_domain=1;
#endif
    t_rlps = ( s_flag == 0 ) ? ( lg_pmps  )
             : ( t1 +  lg_pmps );

    if ( s2 == value_s )
    {
      value_t = ( value_t - t2 );
    }
    else
    {
      if ( --Dbits_to_go < 0 )
      {
        get_byte();
      }

      // Shift in next bit and add to value
      value_t = ( value_t << 1 ) | ( ( Dbuffer >> Dbits_to_go ) & 0x01 );
      value_t = 256 + value_t - t2;
    }

    //restore range
    while ( t_rlps < QUARTER )
    {
      t_rlps = t_rlps << 1;

      if ( --Dbits_to_go < 0 )
      {
        get_byte();
      }

      // Shift in next bit and add to value
      value_t = ( value_t << 1 ) | ( ( Dbuffer >> Dbits_to_go ) & 0x01 );
    }

    s1 = 0;
    t1 = t_rlps & 0xff;

  }
  else //MPS
  {
    s1 = s2;
    t1 = t2;
#if Decoder_BYPASS_Final
	  is_value_domain=0;
#endif
  }
#if !Decoder_BYPASS_Final
  if(is_LPS == 1 || (s1 == value_s_bound && is_value_bound == 1 && is_LPS==0) )
  {
    s1 = 0;
    value_s = 0;
    while ( value_t < QUARTER && value_s < value_s_bound)
    {
      int j;
      if ( --Dbits_to_go < 0 )
      {
        get_byte();
      }
      j = ( Dbuffer >> Dbits_to_go ) & 0x01;
      // Shift in next bit and add to value

      value_t = ( value_t << 1 ) | j;
      value_s++;
    }
    if(value_t < QUARTER)
      is_value_bound = 1;
    else
      is_value_bound = 0;    
    value_t = value_t & 0xff;
  }
#endif
  if ( cFlag )
  {
    update_ctx (dep, bi_ct, is_LPS);
  }

#if EXTRACT_lcu_info_cutype_aec_debug    
  if (p_lcu_debug != NULL)
  {
    fprintf(p_lcu_debug, "%d end\n", value_t);
    fprintf(p_lcu_debug, "%d\n", t1);
    fprintf(p_lcu_debug, "%d\n", s1);
    fprintf(p_lcu_debug, "%d\n", value_s);
    fprintf(p_lcu_debug, "%d\n", bi_ct->MPS);
    fprintf(p_lcu_debug, "%d\n", bi_ct->cycno);
    fprintf(p_lcu_debug, "%d\n", bi_ct->LG_PMPS);
  }
#endif	

#if EXTRACT_lcu_info_cutype_aec_debug2   
  if (p_lcu_debug != NULL)
  {
    fprintf(p_lcu_debug, "end   %6d ", value_t);
    fprintf(p_lcu_debug, "%6d", value_s);
    fprintf(p_lcu_debug, "%6d", t1);
    fprintf(p_lcu_debug, "%6d", s1);
    fprintf(p_lcu_debug, "%3d", bi_ct->MPS);
    fprintf(p_lcu_debug, "%5d", bi_ct->cycno);
    fprintf(p_lcu_debug, "%5d\n", bi_ct->LG_PMPS);
  }
#endif	
  return ( bit );
}

unsigned int biari_decode_symbolW ( DecodingEnvironmentPtr dep, BiContextTypePtr bi_ct1 , BiContextTypePtr bi_ct2 )
{
  register unsigned char bit1, bit2;
  register unsigned char pred_MPS, bit;
  register unsigned int  lg_pmps;
  register unsigned char cwr1, cycno1 = bi_ct1->cycno;
  register unsigned char cwr2, cycno2 = bi_ct2->cycno;
  register unsigned int lg_pmps1 = bi_ct1->LG_PMPS, lg_pmps2 = bi_ct2->LG_PMPS;
  register unsigned int t_rlps;
  register unsigned char s_flag;
  register unsigned int t2;
  register unsigned char s2, is_LPS;

  bit1 = bi_ct1->MPS;
  bit2 = bi_ct2->MPS;


  cwr1 = ( cycno1 <= 1 ) ? 3 : ( cycno1 == 2 ) ? 4 : 5;
  cwr2 = ( cycno2 <= 1 ) ? 3 : ( cycno2 == 2 ) ? 4 : 5;

  if ( bit1 == bit2 )
  {
    pred_MPS = bit1;
    lg_pmps = ( lg_pmps1 + lg_pmps2 ) / 2;
  }
  else
  {
    if ( lg_pmps1 < lg_pmps2 )
    {
      pred_MPS = bit1;
      lg_pmps = ( 256 << LG_PMPS_SHIFTNO ) - 1 - ( ( lg_pmps2 - lg_pmps1 ) >> 1 );
    }
    else
    {
      pred_MPS = bit2;
      lg_pmps = ( 256 << LG_PMPS_SHIFTNO ) - 1 - ( ( lg_pmps1 - lg_pmps2 ) >> 1 );
    }
  }

  if ( t1 >= ( lg_pmps >> LG_PMPS_SHIFTNO ) )
  {
    s2 = s1;
    t2 = t1 - ( lg_pmps >> LG_PMPS_SHIFTNO );
    s_flag = 0;
  }
  else
  {
    s2 = s1 + 1;
    t2 = 256 + t1 - ( lg_pmps >> LG_PMPS_SHIFTNO );
    s_flag = 1;
  }

  bit = pred_MPS;

  if(value_s>value_s_bound)
  {
    printf("value_s:%d\n",value_s);
	exit(1);
  }
  if (value_s > max_value_s)
    max_value_s = value_s;

  if ( (s2 > value_s || ( s2 == value_s && value_t >= t2 ) ) && is_value_bound == 0) //LPS
  {
  is_LPS = 1;
    bit = !bit; //LPS
    t_rlps = ( s_flag == 0 ) ? ( lg_pmps >> LG_PMPS_SHIFTNO ) : ( t1 + ( lg_pmps >> LG_PMPS_SHIFTNO ) );

    if ( s2 == value_s )
    {
      value_t = ( value_t - t2 );
    }
    else
    {
      if ( --Dbits_to_go < 0 )
      {
        get_byte();
      }

      // Shift in next bit and add to value
      value_t = ( value_t << 1 ) | ( ( Dbuffer >> Dbits_to_go ) & 0x01 );
      value_t = 256 + value_t - t2;
    }

    //restore range
    while ( t_rlps < QUARTER )
    {
      t_rlps = t_rlps << 1;

      if ( --Dbits_to_go < 0 )
      {
        get_byte();
      }

      // Shift in next bit and add to value
      value_t = ( value_t << 1 ) | ( ( Dbuffer >> Dbits_to_go ) & 0x01 );
    }

    s1 = 0;
    t1 = t_rlps & 0xff;

    //restore value
    value_s = 0;
  }//--LPS
  else //MPS
  {
    s1 = s2;
    t1 = t2;
  }
  if(is_LPS == 1 || (s1 == value_s_bound && is_value_bound == 1 && is_LPS==0) )
  {
    s1 = 0;
    value_s = 0;
    while ( value_t < QUARTER && value_s < value_s_bound)
    {
      int j;
      if ( --Dbits_to_go < 0 )
      {
        get_byte();
      }
      j = ( Dbuffer >> Dbits_to_go ) & 0x01;
      // Shift in next bit and add to value

      value_t = ( value_t << 1 ) | j;
      value_s++;
    }
    if(value_t < QUARTER)
      is_value_bound = 1;
    else
      is_value_bound = 0;    
    value_t = value_t & 0xff;
  }

  if ( bit != bit1 )
  {
    cycno1 = ( cycno1 <= 2 ) ? ( cycno1 + 1 ) : 3; //LPS occurs
  }
  else
  {
    if ( cycno1 == 0 )
    {
      cycno1 = 1;
    }
  }

  if ( bit != bit2 )
  {
    cycno2 = ( cycno2 <= 2 ) ? ( cycno2 + 1 ) : 3; //LPS occurs
  }
  else
  {
    if ( cycno2 == 0 )
    {
      cycno2 = 1;
    }
  }

  bi_ct1->cycno = cycno1;
  bi_ct2->cycno = cycno2;

  //update probability estimation
  {
    //bi_ct1
    if ( bit == bit1 )
    {
      lg_pmps1 = lg_pmps1 - ( unsigned int ) ( lg_pmps1 >> cwr1 ) - ( unsigned int ) ( lg_pmps1 >> ( cwr1 + 2 ) );
    }
    else
    {
      switch ( cwr1 )
      {
      case 3:
        lg_pmps1 = lg_pmps1 + 197;
        break;
      case 4:
        lg_pmps1 = lg_pmps1 + 95;
        break;
      default:
        lg_pmps1 = lg_pmps1 + 46;
      }

      if ( lg_pmps1 >= ( 256 << LG_PMPS_SHIFTNO ) )
      {
        lg_pmps1 = ( 512 << LG_PMPS_SHIFTNO ) - 1 - lg_pmps1;
        bi_ct1->MPS = ! ( bi_ct1->MPS );
      }
    }

    bi_ct1->LG_PMPS = lg_pmps1;

    //bi_ct2
    if ( bit == bit2 )
    {
      lg_pmps2 = lg_pmps2 - ( unsigned int ) ( lg_pmps2 >> cwr2 ) - ( unsigned int ) ( lg_pmps2 >> ( cwr2 + 2 ) );
    }
    else
    {
      switch ( cwr2 )
      {
      case 3:
        lg_pmps2 = lg_pmps2 + 197;
        break;
      case 4:
        lg_pmps2 = lg_pmps2 + 95;
        break;
      default:
        lg_pmps2 = lg_pmps2 + 46;
      }

      if ( lg_pmps2 >= ( 256 << LG_PMPS_SHIFTNO ) )
      {
        lg_pmps2 = ( 512 << LG_PMPS_SHIFTNO ) - 1 - lg_pmps2;
        bi_ct2->MPS = ! ( bi_ct2->MPS );
      }
    }

    bi_ct2->LG_PMPS = lg_pmps2;
  }

  return ( bit );
}
#if !AEC_LCU_STUFFINGBIT_FIX
#if Decoder_BYPASS_Final
void biari_decode_read(DecodingEnvironmentPtr dep)
{
	if(is_value_domain == 1 || (s1 == value_s_bound && is_value_bound == 1 && is_value_domain==0) )//value_t is in R domain  s1=0 or s1 == value_s_bound
	{
		s1 = 0; 
		value_s = 0;
		is_value_domain =0;
		while ( value_t < QUARTER && value_s < value_s_bound)
		{
			int j;
			if ( --Dbits_to_go < 0 )
			{
				get_byte();
			}
			j = ( Dbuffer >> Dbits_to_go ) & 0x01;
			// Shift in next bit and add to value

			value_t = ( value_t << 1 ) | j;
			value_s++;
		}
		if(value_t < QUARTER)
			is_value_bound = 1;
		else
			is_value_bound = 0;  

		value_t = value_t & 0xff;
	}

}
#endif
#endif

/*!
************************************************************************
* \brief
*    biari_decode_symbol_eq_prob():
* \return
*    the decoded symbol
************************************************************************
*/
unsigned int biari_decode_symbol_eq_prob ( DecodingEnvironmentPtr dep )
{
  unsigned char bit;
  BiContextType octx;
  BiContextTypePtr ctx = &octx;
#if Decoder_Bypass_Annex
  register unsigned int t2;
  register unsigned char s2;
  bit=0;

#if EXTRACT
  fprintf(p_aec,"%d\n",value_t);
  fprintf(p_aec,"%d\n",t1);
  fflush(p_aec);
#endif

  if(is_value_domain || (s1 == value_s_bound && is_value_bound == 1))//s2==1 t2==t1 s2=value_s+1 or s2==value_s
  {
	  s1=0;// value_t is in R domain
	  is_value_domain=1;
	  if ( --Dbits_to_go < 0 )
	  {
		  get_byte();
	  }
	  value_t = ( value_t << 1 ) | ( ( Dbuffer >> Dbits_to_go ) & 0x01 );////R
	  if(value_t>=(256+t1))  //LPS
	  {
		  bit = !bit; //LPS
		  value_t-=(256+t1); //R
	  }
	  else
		  bit = 0;
  }
  else 
  {
	  s2 = s1 + 1;
	  t2 =  t1 ; //8bits
	  if ( s2 > value_s || ( s2 == value_s && value_t >= t2 ) && is_value_bound == 0) //LPS
	  {      
		  bit = !bit; //LPS

		  if ( s2 == value_s )
		  {
			  value_t = ( value_t - t2 );
		  }
		  else
		  {
			  if ( --Dbits_to_go < 0 )
			  {
				  get_byte();
			  }
			  value_t = ( value_t << 1 ) | ( ( Dbuffer >> Dbits_to_go ) & 0x01 );////R
			  value_t = 256 + value_t - t2;
		  }

		  is_value_domain=1;
		  t1 = t1;
	  }
	  else
	  {
		  s1 = s2;
		  t1 = t2;
		  is_value_domain=0;
	  }
  }
#else
#if  By_PASS_1024
  ctx->LG_PMPS = ( QUARTER << LG_PMPS_SHIFTNO );
#else
  ctx->LG_PMPS = ( QUARTER << LG_PMPS_SHIFTNO )-1;
#endif
  ctx->MPS = 0;
  cFlag = 0;
  bit = biari_decode_symbol ( dep, ctx );
#endif
  cFlag = 1;
  return ( bit );
}

unsigned int biari_decode_final ( DecodingEnvironmentPtr dep )
{
  unsigned char bit;
  BiContextType octx;
  BiContextTypePtr ctx = &octx;

#if Decoder_Final_Annex
  register unsigned char s_flag, is_LPS = 0;
  register unsigned int t_rlps,rang;
  register unsigned int t2;
  register unsigned char s2;
  bit=0;
#if EXTRACT
  fprintf(p_aec,"%d\n",value_t);
  fprintf(p_aec,"%d\n",t1);
  fflush(p_aec);
#endif  
  if(is_value_domain == 1 || (s1 == value_s_bound && is_value_bound == 1) )//value_t is in R domain  s1=0 or s1 == value_s_bound
  {
	  s1 = 0; 
	  value_s = 0;

	  while ( value_t < QUARTER && value_s < value_s_bound)
	  {
		  int j;
		  if ( --Dbits_to_go < 0 )
		  {
			  get_byte();
		  }
		  j = ( Dbuffer >> Dbits_to_go ) & 0x01;
		  // Shift in next bit and add to value

		  value_t = ( value_t << 1 ) | j;
		  value_s++;
	  }
	  if(value_t < QUARTER)
		  is_value_bound = 1;
	  else
		  is_value_bound = 0;  

	  value_t = value_t & 0xff;
  }
  if(t1)
  {
	  s2=s1;
	  t2=t1-1;
  }
  else
  {
	  s2=s1+1;
	  t2=255;
  }
  if ( s2 > value_s || ( s2 == value_s && value_t >= t2 )  && is_value_bound == 0) //LPS
  {      
	  is_LPS = 1;
	  bit = !bit; //LPS

	  if ( s2 == value_s )
	  {
		  value_t = ( value_t - t2 );
	  }
	  else
	  {
		  if ( --Dbits_to_go < 0 )
		  {
			  get_byte();
		  }
		  value_t = ( value_t << 1 ) | ( ( Dbuffer >> Dbits_to_go ) & 0x01 );////R
		  value_t = 256 + value_t - t2;
	  }

	  // valueT = (valueT << 8 ) | read_bits(8)
	  t_rlps = 1;
	  while ( t_rlps < QUARTER )//for 8 times
	  {
		  t_rlps = t_rlps << 1;

		  if ( --Dbits_to_go < 0 )
		  {
			  get_byte();
		  }

		  // Shift in next bit and add to value
		  value_t = ( value_t << 1 ) | ( ( Dbuffer >> Dbits_to_go ) & 0x01 );
	  }

	  s1 = 0;
	  t1 = 0;
	  is_value_domain=1;

  }
  else
  {
	  s1 = s2;
	  t1 = t2;
	  is_value_domain=0;
  }
#else
  ctx->LG_PMPS = 1 << LG_PMPS_SHIFTNO;
  ctx->MPS = 0;
  cFlag = 0;
  bit = biari_decode_symbol ( dep, ctx );
#endif

#if EXTRACT_lcu_info_cutype_aec_debug   
  if (p_lcu_debug != NULL)
  {
    fprintf(p_lcu_debug, "%d start\n", value_t);
    fprintf(p_lcu_debug, "%d\n", t1);
    fprintf(p_lcu_debug, "%d\n", s1);
    fprintf(p_lcu_debug, "%d\n", value_s);
    fprintf(p_lcu_debug, "%d\n", bit);
  }
#endif	
  cFlag = 1;
  return ( bit );
}

void biari_init_context_logac ( BiContextTypePtr ctx )
{
  ctx->LG_PMPS = ( QUARTER << LG_PMPS_SHIFTNO ) - 1; //10 bits precision
  ctx->MPS   = 0;
  ctx->cycno  =  0;
}























