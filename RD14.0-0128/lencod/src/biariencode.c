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
#include <math.h>
#include "global.h"
#include "../../lcommon/inc/commonVariables.h"
#include "AEC.h"
#include "biariencode.h"
#include <assert.h>

//AC ENGINE PARAMETERS
unsigned int s1, t1, s2, t2;

/*!
************************************************************************
* Macro for writing bytes of code
***********************************************************************
*/

#define put_byte() { \
    Ecodestrm[(*Ecodestrm_len)++] = Ebuffer; \
    Ebits_to_go = 8; \
    while (eep->C > 7) { \
      eep->C-=8; \
      eep->E++; \
    } \
  }

#define put_one_bit(b) { \
    Ebuffer <<= 1; Ebuffer |= (b); \
    if (--Ebits_to_go == 0) \
      put_byte(); \
  }

#define put_one_bit_plus_outstanding(b) { \
    put_one_bit(b); \
    while (Ebits_to_follow > 0) \
    { \
      Ebits_to_follow--; \
      put_one_bit(!(b)); \
    } \
  }

unsigned int LPSbits[256] =
{
  2184, 2184, 1928, 1779, 1673, 1591, 1525, 1468, 1419, 1376, 1338,
  1303, 1272, 1243, 1216, 1191, 1167, 1145, 1125, 1105, 1087, 1069,
  1053, 1037, 1022, 1007, 993, 980, 967, 954, 942, 930, 919, 908,
  898, 888, 878, 868, 859, 850, 841, 832, 824, 816, 808, 800, 792, 785,
  777, 770, 763, 756, 750, 743, 737, 730, 724, 718, 712, 707, 701, 695,
  690, 684, 679, 674, 669, 663, 658, 654, 649, 644, 639, 635, 630, 626,
  621, 617, 613, 608, 604, 600, 596, 592, 588, 584, 580, 577, 573, 569,
  566, 562, 558, 555, 551, 548, 545, 541, 538, 535, 531, 528, 525, 522,
  519, 516, 513, 510, 507, 504, 501, 498, 495, 492, 490, 487, 484, 482,
  479, 476, 474, 471, 468, 466, 463, 461, 458, 456, 454, 451, 449, 446,
  444, 442, 439, 437, 435, 433, 430, 428, 426, 424, 422, 420, 418, 415,
  413, 411, 409, 407, 405, 403, 401, 399, 397, 395, 394, 392, 390, 388,
  386, 384, 382, 381, 379, 377, 375, 373, 372, 370, 368, 367, 365, 363,
  362, 360, 358, 357, 355, 353, 352, 350, 349, 347, 346, 344, 342, 341,
  339, 338, 336, 335, 333, 332, 331, 329, 328, 326, 325, 323, 322, 321, 319,
  318, 317, 315, 314, 313, 311, 310, 309, 307, 306, 305, 303, 302, 301,
  300, 298, 297, 296, 295, 293, 292, 291, 290, 289, 287, 286, 285, 284,
  283, 282, 281, 279, 278, 277, 276, 275, 274, 273, 272, 271, 269, 268,
  267, 266, 265, 264, 263, 262, 261, 260, 259, 258, 257
};

/*!
************************************************************************
* \brief
*    Initializes the EncodingEnvironment for the arithmetic coder
************************************************************************
*/

void arienco_start_encoding ( EncodingEnvironmentPtr eep,
                              unsigned char *code_buffer,
                              int *code_len, int slice_type )
{
  Ecodestrm = code_buffer;
  Ecodestrm_len = code_len;
  Elow = 0;
  E_s1 = 0;
  E_t1 = 0xFF;
  Ebits_to_follow  = 0 ;
  Ebuffer = 0;
  Ebits_to_go = 9;// to swallow first redundant bit
  s2 = 0;
  t2 = 0xff;
  eep->C = 0;
  eep->B = *code_len;
  eep->E = 0;
}

/*!
************************************************************************
* \brief
*    Returns the number of currently written bits
************************************************************************
*/
int arienco_bits_written ( EncodingEnvironmentPtr eep )
{
  return ( 8 * ( *Ecodestrm_len /*-*Ecodestrm_laststartcode*/ ) + Ebits_to_follow + 8 - Ebits_to_go + E_s1 );
}


/*!
************************************************************************
* \brief
*    Terminates the arithmetic codeword, writes stop bit and stuffing bytes (if any)
************************************************************************
*/

void arienco_done_encoding ( EncodingEnvironmentPtr eep )
{
  int i;
  put_one_bit_plus_outstanding ( ( Elow >> ( B_BITS - 1 ) ) & 1 ); //Ð´ÈëEbits_to_follow+1Î»
  put_one_bit ( ( Elow >> ( B_BITS - 2 ) ) & 1 ); //1bit
  //put_one_bit(0);   //1 bit         //multiple slice, yuanyuan, XZHENG
  put_one_bit ( 1 );
#if AEC_LCU_STUFFINGBIT_FIX
  for ( i = 0; i < 7; i++ )    //just encoder method to guarantee the last lps in a slice can be decoded successfully, 2015.07.02
#else
  for ( i = 0; i < 8; i++ )
#endif
  {
    put_one_bit ( 0 );
  }

  stat->bit_use_stuffingBits[img->type] += ( 8 - Ebits_to_go );

  eep->E = eep->E * 8 + eep->C; // no of processed bins
  eep->B = ( *Ecodestrm_len - eep->B ); // no of written bytes
  eep->E -= ( img->current_mb_nr - img->currentSlice->start_mb_nr );
  eep->E = ( eep->E + 31 ) >> 5;
  // eep->E now contains the minimum number of bytes for the NAL unit
}


/*!
************************************************************************
* \brief
*    Actually arithmetic encoding of one binary symbol by using
*    the probability estimate of its associated context model
************************************************************************
*/

int  biari_encode_symbol_est( unsigned char symbol, BiContextTypePtr bi_ct )
{
  int  estBits;
  unsigned int lg_pmps = bi_ct->LG_PMPS;
#if HZC_TEST
  unsigned char cycno = bi_ct->cycno;
  unsigned char cwr = 0;
#endif

  symbol = ( short ) ( symbol != 0 );
  estBits = ( symbol == bi_ct->MPS ) ? ( lg_pmps >> 2 ) : LPSbits[lg_pmps >> 2];
#if HZC_TEST
  cwr = ( cycno <= 1 ) ? 3 : ( cycno == 2 ) ? 4 : 5;

  if ( symbol == bi_ct->MPS )
  {
    if ( cycno == 0 )
    {
      cycno = 1;
    }
    lg_pmps = lg_pmps - ( unsigned int )( lg_pmps >> cwr ) - ( unsigned int )( lg_pmps >> ( cwr + 2 ) );
  }
  else
  {
    cycno = ( cycno <= 2 ) ? ( cycno + 1 ) : 3;
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
      bi_ct->MPS = !( bi_ct->MPS );
    }

  }
  bi_ct->cycno = cycno;
  bi_ct->LG_PMPS = lg_pmps;
#endif
  return( estBits );
}

int  biari_encode_symbolW_est( unsigned char symbol, BiContextTypePtr bi_ct1, BiContextTypePtr bi_ct2 )
{
  int  estBits;
  unsigned int lg_pmps1 = bi_ct1->LG_PMPS, lg_pmps2 = bi_ct2->LG_PMPS;
  unsigned int  lg_pmps = 0;
  unsigned char bit1 = bi_ct1->MPS, bit2 = bi_ct2->MPS;
  unsigned char pred_MPS = 0;
#if HZC_TEST
  unsigned int cwr1 = 0, cycno1 = bi_ct1->cycno;
  unsigned int cwr2 = 0, cycno2 = bi_ct2->cycno;
  cwr1 = ( cycno1 <= 1 ) ? 3 : ( cycno1 == 2 ) ? 4 : 5;
  cwr2 = ( cycno2 <= 1 ) ? 3 : ( cycno2 == 2 ) ? 4 : 5;
#endif
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
  symbol = ( short ) ( symbol != 0 );
#if HZC_TEST
  if ( symbol != bit1 )
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

  if ( symbol != bit2 )
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
#endif
  estBits = ( symbol == pred_MPS ) ? ( lg_pmps >> 2 ) : LPSbits[lg_pmps >> 2];
#if HZC_TEST
  //bi_ct1
  if ( symbol == bit1 )
  {
    lg_pmps1 = lg_pmps1 - ( unsigned int )( lg_pmps1 >> cwr1 ) - ( unsigned int )( lg_pmps1 >> ( cwr1 + 2 ) );
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
      bi_ct1->MPS = !( bi_ct1->MPS );
    }
  }
  bi_ct1->LG_PMPS = lg_pmps1;

  //bi_ct2
  if ( symbol == bit2 )
  {
    lg_pmps2 = lg_pmps2 - ( unsigned int )( lg_pmps2 >> cwr2 ) - ( unsigned int )( lg_pmps2 >> ( cwr2 + 2 ) );
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
      bi_ct2->MPS = !( bi_ct2->MPS );
    }
  }
  bi_ct2->LG_PMPS = lg_pmps2;
#endif
  return( estBits );
}

int  biari_encode_symbol_eq_prob_est( unsigned char  symbol )
{
  int  estBits;
  BiContextType octx;
  BiContextTypePtr ctx = &octx;
  ctx->LG_PMPS = ( QUARTER << LG_PMPS_SHIFTNO ) - 1;
  ctx->MPS = 0;
  ctx->cycno = 0;
  estBits = biari_encode_symbol_est( symbol, ctx );

  return( estBits );
}


#if M3474_LEVEL
int biari_encode_symbol_final_est (  unsigned char symbol )
{
	int estBits;
	BiContextType octx;
	BiContextTypePtr ctx = &octx;
	ctx->LG_PMPS = 1 << LG_PMPS_SHIFTNO;
	ctx->MPS = 0;
	ctx->cycno = 0;
	estBits= biari_encode_symbol_est ( symbol, ctx );
	return( estBits );
}
#endif 

//Logarithm arithmetic coder by Tsinghua
void biari_encode_symbol ( EncodingEnvironmentPtr eep, unsigned char symbol, BiContextTypePtr bi_ct ) //type 0 normal,1 equal,2 final
{
  unsigned char cycno = bi_ct->cycno;
  unsigned int low = Elow;
  unsigned char cwr = 0;
  unsigned int lg_pmps = bi_ct->LG_PMPS;
  unsigned int t_rLPS = 0;
  unsigned char s_flag = 0, is_LPS = 0;
  unsigned int  curr_byte = 0;
  short int bitstogo = 0;
  unsigned char bit_o = 0, bit_oa = 0, byte_no = 0;
  unsigned int low_byte[3] = {0};
  static int bits_counter = 0;

  if ( AEC_writting == 1 )
  {
    bits_counter++;
  }

  s1 = E_s1;
  t1 = E_t1;

  if ( AEC_writting == 1 )
  {
    low_byte[0] = 0;
  }

  low_byte[1] = 0;
  low_byte[2] = 0;

  assert ( eep != NULL );

  cwr = ( cycno <= 1 ) ? 3 : ( cycno == 2 ) ? 4 : 5;

  if ( symbol != 0 )
  {
    symbol = 1;
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

  if ( symbol == bi_ct->MPS ) //MPS happens
  {

    if ( cycno == 0 )
    {
      cycno = 1;
    }

    s1 = s2;
    t1 = t2;

    //no updating of interval range and low here or
    //renorm to guarantee s1<8, after renorm, s1 --;
#if Encoder_BYPASS_Final
	if ( s1) //renorm
#else
    if ( s1 == 8 ) //renorm
#endif
    {
      //left shift 1 bit

      bit_o = ( low >> 9 ) & 1;
      {
        bit_oa = ( low >> 8 ) & 1;
        if ( bit_o )
        {
          put_one_bit_plus_outstanding ( 1 );
        }
        else
        {
          if ( !bit_oa ) //00
          {
            put_one_bit_plus_outstanding ( 0 );
          }
          else  //01
          {
            Ebits_to_follow++;
            bit_oa = 0;
          }
        }
        s1--;
      }
      //restore low
      low = ( ( bit_oa << 8 ) | ( low & 0xff ) ) << 1;
    }
  }
  else //--LPS
  {
    is_LPS = 1;
    cycno = ( cycno <= 2 ) ? ( cycno + 1 ) : 3;

    if ( s_flag == 0 )
    {
      t_rLPS = lg_pmps >> LG_PMPS_SHIFTNO;  //t_rLPS -- 9bits
    }
    else //s2=s1 + 1
    {
      t_rLPS = t1 + ( lg_pmps >> LG_PMPS_SHIFTNO );  //t_rLPS<HALF
    }

    low_byte[0] = low + ( ( t2 + 256 ) >> s2 ); //first low_byte: 10bits
    low_byte[1] = ( t2 << ( 8 - s2 ) ) & 0xff;

    //restore range
    while ( t_rLPS < QUARTER )
    {
      t_rLPS = t_rLPS << 1;
      s2 ++;
    }
    //left shift s2 bits
    {
      curr_byte = low_byte[0];
      bitstogo = 9;
      bit_oa = ( curr_byte >> bitstogo ) & 1;
      byte_no = 0;

      while ( s2 > 0 )
      {
        bit_o = bit_oa;
        bitstogo--;

        if ( bitstogo < 0 )
        {
          curr_byte = low_byte[++byte_no];
          bitstogo = 7;
        }

        bit_oa = ( curr_byte >> bitstogo ) & 1;

        if ( bit_o )
        {
          put_one_bit_plus_outstanding ( 1 );
        }
        else
        {
          if ( !bit_oa ) //00
          {
            put_one_bit_plus_outstanding ( 0 );
          }
          else  //01
          {
            Ebits_to_follow++;
            bit_oa = 0;
          }
        }

        s2--;
      }
#if Encoder_BYPASS_Final
	  if(bitstogo>=1) {
		  low=((low_byte[0]<<(9-bitstogo))|(low_byte[1]>>(bitstogo-1)));
		  low=low&((bit_oa<<9)|0x1ff);
	  }
	  else
	  {
		  low=((low_byte[0]<<(9))|(low_byte[1]<<(1)));
		  low=low&((bit_oa<<9)|0x1ff);
	  }
	}
	s1 = 0;
	t1 = t_rLPS & 0xff;
#else
      //restore low
      low = bit_oa;
      s2 = 9;

      while ( s2 > 0 )
      {
        bitstogo--;

        if ( bitstogo < 0 )
        {
          curr_byte = low_byte[++byte_no];
          bitstogo = 7;
        }

        bit_oa = ( curr_byte >> bitstogo ) & 1;
        low = ( low << 1 ) | bit_oa;
        s2--;
      }
    }

    s1 = 0;
    t1 = t_rLPS & 0xff;
#endif
  }

  //updating other parameters
  bi_ct->cycno = cycno;
  Elow = low;
  E_s1 = s1;
  E_t1 = t1;
  eep->C++;

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

  bi_ct->LG_PMPS = lg_pmps;
}



void biari_encode_symbolW ( EncodingEnvironmentPtr eep, unsigned char symbol, BiContextTypePtr bi_ct1, BiContextTypePtr bi_ct2 )
{

  unsigned int low = Elow;
  unsigned int cwr1 = 0, cycno1 = bi_ct1->cycno;
  unsigned int cwr2 = 0, cycno2 = bi_ct2->cycno;
  unsigned int lg_pmps1 = bi_ct1->LG_PMPS, lg_pmps2 = bi_ct2->LG_PMPS;
  unsigned char bit1 = bi_ct1->MPS, bit2 = bi_ct2->MPS;
  unsigned int t_rLPS = 0;
  unsigned char s_flag = 0;
  unsigned char pred_MPS = 0;
  unsigned int  lg_pmps = 0;
  unsigned int low_byte[3] = {0};
  unsigned int  curr_byte = 0;
  short int bitstogo = 0;
  unsigned char bit_o = 0, bit_oa = 0, byte_no = 0;

  if ( AEC_writting == 1 )
  {
    low_byte[0] = 0;
  }

  low_byte[1] = 0;
  low_byte[2] = 0;
  s1 = E_s1;
  t1 = E_t1;

  assert ( eep != NULL );
  cwr1 = ( cycno1 <= 1 ) ? 3 : ( cycno1 == 2 ) ? 4 : 5;
  cwr2 = ( cycno2 <= 1 ) ? 3 : ( cycno2 == 2 ) ? 4 : 5;

  if ( symbol != 0 )
  {
    symbol = 1;
  }

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

  if ( symbol == pred_MPS ) //MPS happens
  {
    s1 = s2;
    t1 = t2;

    //no updating of interval range and low here or
    //renorm to guarantee s1<8, after renorm, s1 --;
    if ( s1 == 8 ) //renorm
    {
      //left shift 1 bit
      bit_o = ( low >> 9 ) & 1;

      {
        bit_oa = ( low >> 8 ) & 1;

        if ( bit_o )
        {
          put_one_bit_plus_outstanding ( 1 );
        }
        else
        {
          if ( !bit_oa ) //00
          {
            put_one_bit_plus_outstanding ( 0 );
          }
          else  //01
          {
            Ebits_to_follow++;
            bit_oa = 0;
          }
        }

        s1--;
      }

      //restore low
      low = ( ( bit_oa << 8 ) | ( low & 0xff ) ) << 1;
    }
  }
  else
  {
    if ( s_flag == 0 )
    {
      t_rLPS  = ( lg_pmps >> LG_PMPS_SHIFTNO );
    }
    else //s2=s1 + 1
    {
      t_rLPS = t1 + ( lg_pmps >> LG_PMPS_SHIFTNO );
    }

    low_byte[0] = low + ( ( t2 + 256 ) >> s2 ); //first low_byte: 10bits
    low_byte[1] = ( t2 << ( 8 - s2 ) ) & 0xff;

    //restore range
    while ( t_rLPS < QUARTER )
    {
      t_rLPS = t_rLPS << 1;
      s2 ++;
    }

    //left shift s2 bits
    {
      curr_byte = low_byte[0];
      bitstogo = 9;
      bit_oa = ( curr_byte >> bitstogo ) & 0x01;
      byte_no = 0;

      while ( s2 > 0 )
      {
        bit_o = bit_oa;
        bitstogo--;

        if ( bitstogo < 0 )
        {
          curr_byte = low_byte[++byte_no];
          bitstogo = 7;
        }

        bit_oa = ( curr_byte >> bitstogo ) & 0x01;
        //        if (AEC_writting==1)
        //          printf("bo:%d/t boa:%d\n",bit_o,bit_oa);

        if ( bit_o )
        {
          put_one_bit_plus_outstanding ( 1 );
        }
        else
        {
          if ( 1 == bit_oa ) //01
          {
            Ebits_to_follow++;
            bit_oa = 0;
          }
          else //00
          {
            put_one_bit_plus_outstanding ( 0 );
          }
        }
        s2--;
      }

      //restore low
      low = bit_oa;
      s2 = 9;

      while ( s2 > 0 )
      {
        bitstogo--;

        if ( bitstogo < 0 )
        {
          curr_byte = low_byte[++byte_no];
          bitstogo = 7;
        }

        bit_oa = ( curr_byte >> bitstogo ) & 1;
        low = ( low << 1 ) | bit_oa;
        s2--;
      }
    }
    s1 = 0;
    t1 = t_rLPS & 0xff;
  }

  if ( symbol != bit1 )
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

  if ( symbol != bit2 )
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
  Elow = low;
  E_s1 = s1;
  E_t1 = t1;
  eep->C++;

  //update probability estimation
  {
    //bi_ct1
    if ( symbol == bit1 )
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
    if ( symbol == bit2 )
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
}



void biari_encode_symbol_eq_prob ( EncodingEnvironmentPtr eep, unsigned char  symbol )
{
  BiContextType octx;
  BiContextTypePtr ctx = &octx;
#if Encoder_BYPASS_Final
  unsigned char bit_o = 0,bit_oa = 0;
  unsigned int low_byte[3] = {0};
  unsigned int low = Elow;
  t1 = E_t1;
  if(symbol!=0)//LPS
  {
	  //update low
	  low_byte[0] = low + ( ( t1 + 0x100 ) >> 1 ); //first low_byte: 10bits
	  low_byte[1] = ( t1 << 7) & 0xff;
  }

  else //MPS
  {	
	  //LOW is same
	  low_byte[0] = low; //first low_byte: 10bits
	  low_byte[1] = 0;
  }

  bit_o = ( low_byte[0] >> 9 ) & 1;
  bit_oa = ( low_byte[0] >> 8 ) & 1;
  //out bit
  if ( bit_o )
  {
	  put_one_bit_plus_outstanding ( 1 );
  }
  else
  {
	  if ( !bit_oa ) //00
	  {
		  put_one_bit_plus_outstanding ( 0 );
	  }
	  else  //01
	  {
		  Ebits_to_follow++;
		  bit_oa = 0;
	  }
  }
  //update low
  low = ((( ( bit_oa << 8 ) | ( low_byte[0] & 0xff ) ) << 1)|((low_byte[1]>>7)&1));
  Elow = low;
  eep->C++;


#else
 #if  By_PASS_1024
  ctx->LG_PMPS = ( QUARTER << LG_PMPS_SHIFTNO );
#else
  ctx->LG_PMPS = ( QUARTER << LG_PMPS_SHIFTNO )-1;
#endif
  ctx->MPS = 0;
  ctx->cycno = 0;
  biari_encode_symbol ( eep, symbol, ctx );
#endif
}


void biari_encode_symbol_final ( EncodingEnvironmentPtr eep, unsigned char symbol )
{
  BiContextType octx;
  BiContextTypePtr ctx = &octx;
#if Encoder_BYPASS_Final
  unsigned char bit_o = 0,bit_oa = 0,byte_no=0;
  unsigned int low_byte[3] = {0};
  unsigned int  curr_byte = 0;
  short int bitstogo = 0;
  unsigned int low = Elow;
  unsigned int t_rLPS = 0;
  unsigned int s_flag=0;
  t1 = E_t1;
  if ( t1)// get s2,t2
  {
	  s2 = s1;
	  t2 = t1 - 1;
	  s_flag = 0;
  }
  else
  {
	  s2 = s1 + 1;
	  t2 = 255;
	  s_flag = 1;
  }
  if(symbol)//LPS //rLPS=1
  {

	  low_byte[0] = low + ( ( t2 + 256 ) >> s2 ); //first low_byte: 10bits
	  low_byte[1] = ( t2 << ( 8 - s2 ) ) & 0xff;
	  s2+=8;
	  curr_byte=low_byte[0];
	  bitstogo = 9;
	  bit_oa = ( curr_byte >> bitstogo ) & 1;
	  byte_no = 0;
	  while ( s2 > 0 )
	  {
		  bit_o = bit_oa;
		  bitstogo--;
		  if ( bitstogo < 0 )
		  {
			  curr_byte = low_byte[++byte_no];
			  bitstogo = 7;
		  }
		  bit_oa = ( curr_byte >> bitstogo ) & 1;

		  if ( bit_o )
		  {
			  put_one_bit_plus_outstanding ( 1 );
		  }
		  else
		  {
			  if ( !bit_oa ) //00
			  {
				  put_one_bit_plus_outstanding ( 0 );
			  }
			  else  //01
			  {
				  Ebits_to_follow++;
				  bit_oa = 0;
			  }
		  }

		  s2--;
	  }
	  //restore low

	  if(bitstogo>=1) {
		  low=((low_byte[0]<<(9-bitstogo))|(low_byte[1]>>(bitstogo-1)));
		  low=low&((bit_oa<<9)|0x1ff);
	  }
	  else
	  {
		  low=((low_byte[0]<<(9))|(low_byte[1]<<(1)));
		  low=low&((bit_oa<<9)|0x1ff);
	  }
	  t1=0;
	  s1 = 0;
  }
  else  //MPS
  {
	  s1 = s2;
	  t1 = t2; 
	  if( s1)
	  {
		  s1--;
		  t1=255;//(255<<1)|0
		  bit_o = ( low >> 9 ) & 1;
		  bit_oa = ( low >> 8 ) & 1;
		  if ( bit_o )
		  {
			  put_one_bit_plus_outstanding ( 1 );

		  }
		  else
		  {
			  if ( !bit_oa ) //00
			  {
				  put_one_bit_plus_outstanding ( 0 );

			  }
			  else  //01
			  {
				  Ebits_to_follow++;

				  bit_oa = 0;
			  }

		  }
		  //restore low
		  low = ( ( bit_oa << 8 ) | ( low & 0xff ) ) << 1;
	  }
  }
  Elow = low;
  E_t1 = t1;
  E_s1=  s1;
  eep->C++;
#else
  ctx->LG_PMPS = 1 << LG_PMPS_SHIFTNO;
  ctx->MPS = 0;
  ctx->cycno = 0;
  biari_encode_symbol ( eep, symbol, ctx );
#endif
}


/*!
************************************************************************
* \brief
*    Initializes a given context with some pre-defined probability state
************************************************************************
*/


void biari_init_context_logac ( BiContextTypePtr ctx )
{
  ctx->LG_PMPS = ( QUARTER << LG_PMPS_SHIFTNO ) - 1; //10 bits precision
  ctx->MPS   = 0;
  ctx->cycno   = 0;
}


