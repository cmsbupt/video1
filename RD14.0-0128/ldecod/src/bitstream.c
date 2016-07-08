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
* File name: bitstream.c
* Function: decode bitstream
*
*************************************************************************************
*/

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "global.h"
#include "../../lcommon/inc/commonVariables.h"
#include "annexb.h"
#include "../../lcommon/inc/memalloc.h"
#include "biaridecod.h"

#define SVA_STREAM_BUF_SIZE 1024

extern StatBits *StatBitsPtr; //rm52k_r2
FILE *bitsfile = NULL;    //!< the bit stream file

typedef struct
{
  FILE *f;
  unsigned char buf[SVA_STREAM_BUF_SIZE];
  unsigned int  uClearBits;
  unsigned int  uPre3Bytes;
  int iBytePosition;
  int iBufBytesNum;
  int iClearBitsNum;
  int iStuffBitsNum;
  int iBitsCount;
} InputStream;

InputStream IRABS;
InputStream *pIRABS = &IRABS;

void OpenIRABS ( InputStream *p, char *fname )
{
  p->f = fopen ( fname, "rb" );

  if ( p->f == NULL )
  {
    printf ( "\nCan't open file %s", fname );
    exit ( -1 );
  }

  p->uClearBits     = 0xffffffff;
  p->iBytePosition    = 0;
  p->iBufBytesNum     = 0;
  p->iClearBitsNum    = 0;
  p->iStuffBitsNum    = 0;
  p->iBitsCount     = 0;
  p->uPre3Bytes     = 0;
}

void CloseIRABS ( InputStream *p )
{
  fclose ( p->f );
}

/*
*************************************************************************
* Function: Check start code's type
* Input:
* Output:
* Return:
* Attention: If the start code is video_sequence_start_code,user_data_start_code
or extension_start_code, the demulation mechanism is forsymded.
* Author: X ZHENG, 20080515
*************************************************************************
*/
void CheckType ( int startcode )
{
  startcode = startcode & 0x000000ff;

  switch ( startcode )
  {
  case 0xb0:
  case 0xb2:
  case 0xb5:
    demulate_enable = 0;
    break;
  default:
    demulate_enable = 1;
    break;
  }
}
// move iBytePosition to the next byte of start code prefix
//return
//    0 : OK
//   -1 : arrive at stream end and start code is not found
//   -2 : p->iBytePosition error
//-------------------------------------------------------------------------
int NextStartCode ( InputStream *p )
{
  int i, m;
  unsigned char a, b; // a b 0 1 2 3 4 ... M-3 M-2 M-1
  m = 0; // cjw 20060323 for linux envi

  while ( 1 )
  {
    if ( p->iBytePosition >= p->iBufBytesNum - 2 ) //if all bytes in buffer has been searched
    {
      m = p->iBufBytesNum - p->iBytePosition;

      if ( m < 0 )
      {
        return -2;  // p->iBytePosition error
      }

      if ( m == 1 )
      {
        b = p->buf[p->iBytePosition + 1];
      }

      if ( m == 2 )
      {
        b = p->buf[p->iBytePosition + 1];
        a = p->buf[p->iBytePosition];
      }

      p->iBufBytesNum = fread ( p->buf, 1, SVA_STREAM_BUF_SIZE, p->f );
      p->iBytePosition = 0;
    }

    if ( p->iBufBytesNum + m < 3 )
    {
      return -1;  //arrive at stream end and start code is not found
    }

    if ( m == 1 && b == 0 && p->buf[0] == 0 && p->buf[1] == 1 )
    {
      p->iBytePosition  = 2;
      p->iClearBitsNum  = 0;
      p->iStuffBitsNum  = 0;
      p->iBitsCount   += 24;
      p->uPre3Bytes   = 1;
      return 0;
    }

    if ( m == 2 && b == 0 && a == 0 && p->buf[0] == 1 )
    {
      p->iBytePosition  = 1;
      p->iClearBitsNum  = 0;
      p->iStuffBitsNum  = 0;
      p->iBitsCount   += 24;
      p->uPre3Bytes   = 1;
      return 0;
    }

    if ( m == 2 && b == 0 && p->buf[0] == 0 && p->buf[1] == 1 )
    {
      p->iBytePosition  = 2;
      p->iClearBitsNum  = 0;
      p->iStuffBitsNum  = 0;
      p->iBitsCount   += 24;
      p->uPre3Bytes   = 1;
      return 0;
    }

    for ( i = p->iBytePosition; i < p->iBufBytesNum - 2; i++ )
    {
      if ( p->buf[i] == 0 && p->buf[i + 1] == 0 && p->buf[i + 2] == 1 )
      {
        p->iBytePosition  = i + 3;
        p->iClearBitsNum  = 0;
        p->iStuffBitsNum  = 0;
        p->iBitsCount   += 24;
        p->uPre3Bytes   = 1;
        return 0;
      }

      p->iBitsCount += 8;
    }

    p->iBytePosition = i;
  }
}

/*
*************************************************************************
* Function:
* Input:
* Output:
* Return:  0 : OK
-1 : arrive at stream end
-2 : meet another start code
* Attention:
*************************************************************************
*/
int ClearNextByte ( InputStream *p )
{
  int i, k, j;
  unsigned char temp[3];
  i = p->iBytePosition;
  k = p->iBufBytesNum - i;

  if ( k < 3 )
  {
    for ( j = 0; j < k; j++ )
    {
      temp[j] = p->buf[i + j];
    }

    p->iBufBytesNum = fread ( p->buf + k, 1, SVA_STREAM_BUF_SIZE - k, p->f );

    if ( p->iBufBytesNum == 0 )
    {
      if ( k > 0 )
      {
        p->uPre3Bytes = ( ( p->uPre3Bytes << 8 ) | p->buf[i] ) & 0x00ffffff;

        if ( p->uPre3Bytes < 4 && demulate_enable )
        {
          p->uClearBits = ( p->uClearBits << 6 ) | ( p->buf[i] >> 2 );
          p->iClearBitsNum += 6;
          StatBitsPtr->emulate_bits += 2; //rm52k_r2
        }
        else
        {
          p->uClearBits = ( p->uClearBits << 8 ) | p->buf[i];
          p->iClearBitsNum += 8;
        }

        p->iBytePosition++;
        return 0;
      }
      else
      {
        return -1;//arrive at stream end
      }
    }
    else
    {
      for ( j = 0; j < k; j++ )
      {
        p->buf[j] = temp[j];
      }

      p->iBufBytesNum += k;
      i = p->iBytePosition = 0;
    }
  }

  if ( p->buf[i] == 0 && p->buf[i + 1] == 0 && p->buf[i + 2] == 1 )
  {
    return -2;  // meet another start code
  }

  p->uPre3Bytes = ( ( p->uPre3Bytes << 8 ) | p->buf[i] ) & 0x00ffffff;

  if ( p->uPre3Bytes < 4 && demulate_enable )
  {
    p->uClearBits = ( p->uClearBits << 6 ) | ( p->buf[i] >> 2 );
    p->iClearBitsNum += 6;
    StatBitsPtr->emulate_bits += 2; //rm52k_r2
  }
  else
  {
    p->uClearBits = ( p->uClearBits << 8 ) | p->buf[i];
    p->iClearBitsNum += 8;
  }

  p->iBytePosition++;
  return 0;
}


/*
*************************************************************************
* Function:
* Input:
* Output:
* Return:  0 : OK
-1 : arrive at stream end
-2 : meet another start code
* Attention:
*************************************************************************
*/
int read_n_bit ( InputStream *p, int n, int *v )
{
  int r;
  unsigned int t;

  while ( n > p->iClearBitsNum )
  {
    r = ClearNextByte ( p );

    if ( r )
    {
      if ( r == -1 )
      {
        if ( p->iBufBytesNum - p->iBytePosition > 0 )
        {
          break;
        }
      }

      return r;
    }
  }

  t = p->uClearBits;
  r = 32 - p->iClearBitsNum;
  *v = ( t << r ) >> ( 32 - n );
  printf("t=0x%08X r=%4d n=%4d byte=0x%02X\n", t, r, n, (t << r) >> (32 - n));
  p->iClearBitsNum -= n;
  return 0;
}
//==================================================================================

void OpenBitstreamFile ( char *fn )
{
  OpenIRABS ( pIRABS, fn );
}

void CloseBitstreamFile()
{
  CloseIRABS ( pIRABS );
}


/*
*************************************************************************
* Function:For supporting multiple sequences in a stream
* Input:
* Output:
* Return:
* Attention: Modified by X ZHENG, 20080721
* Author: Carmen, 2008/01/22
*************************************************************************
*/
int IsEndOfBitstream ()
{
  int ret, seof, m;
  ret = feof ( pIRABS->f );
  m = pIRABS->iBufBytesNum - pIRABS->iBytePosition;
  seof = ( ( ret ) && ( !pIRABS->iBufBytesNum ) ) || ( ( ret ) && ( m == 1 ) );
  return ( ( !seof ) ? ( 0 ) : ( 1 ) );
}

/*
*************************************************************************
* Function:check slice start code
* Input:
* Output:
* Return:
* Attention: jlzheng  6.30
*************************************************************************
*/
int checkstartcode()   //check slice start code    jlzheng  6.30
{
  int temp_i, temp_val;

  //multiple slice
  DecodingEnvironmentPtr dep_dp;


  dep_dp = &img->currentSlice->partArr[0].de_AEC;
#if !AEC_LCU_STUFFINGBIT_FIX
#if Decoder_BYPASS_Final
  biari_decode_read( dep_dp);
#endif
#endif
  currStream->frame_bitoffset = arideco_bits_read ( dep_dp ); //multiple slice, ??

  //multiple slice

  if ( currStream->bitstream_length * 8 - currStream->frame_bitoffset == 0 )
  {
    return 1;
  }

  if ( img->current_mb_nr == 0 )
  {
    //--- added by Yulj 2004.07.15
    if ( currStream->bitstream_length * 8 - currStream->frame_bitoffset <= 8
         && currStream->bitstream_length * 8 - currStream->frame_bitoffset > 0 )
    {
      temp_i = currStream->bitstream_length * 8 - currStream->frame_bitoffset;
      assert ( temp_i > 0 );
      temp_val = get_uv ( temp_i, "filling data" ) ;
    }
  }

  if ( img->current_mb_nr == 0 )
  {
    if ( StartCodePosition > 4 && StartCodePosition < 0x000fffff )
    {
      return 1;
    }
    else
    {
      currStream->frame_bitoffset = currentbitoffset;
      return 0;
    }
  }

  if ( img->current_mb_nr != 0 )
  {
    //--- added by Yulj 2004.07.15
    if ( currStream->bitstream_length * 8 - currStream->frame_bitoffset <= 8
         && currStream->bitstream_length * 8 - currStream->frame_bitoffset > 0 )
    {
      temp_i = currStream->bitstream_length * 8 - currStream->frame_bitoffset;
      assert ( temp_i > 0 );
      temp_val = get_uv ( temp_i, "stuffing pattern" ) ; //modified at rm52k

      if ( temp_val == ( 1 << ( temp_i - 1 ) ) )
      {
        return 1;  // last MB in current slice
      }
    }

    return 0;       // not last MB in current slice
    //---end
  }

  return 1;
}



//------------------------------------------------------------
// buf          : buffer
// startcodepos :
// length       :
int GetOneUnit ( char *buf, int *startcodepos, int *length )
{
  int i, j, k;
#if EXTRACT
  unsigned char b;
#endif
  i = NextStartCode ( pIRABS );

  if ( i != 0 )
  {
    if ( i == -1 )
    {
      printf ( "\narrive at stream end and start code is not found!" );
    }

    if ( i == -2 )
    {
      printf ( "\np->iBytePosition error!" );
    }

    exit ( i );
  }

  buf[0] = 0;
  buf[1] = 0;
  buf[2] = 1;
  *startcodepos = 3;
  i = read_n_bit ( pIRABS, 8, &j );
  buf[3] = ( char ) j;

#if EXTRACT
  	    if(buf[3]>=SLICE_START_CODE_MIN && buf[3]<=SLICE_START_CODE_MAX)
		{
			//added by chenh
			sliceNum++;
			sprintf(filename,"./%s/aec/slice_%04d_%04d.txt",infile,img->coding_order,sliceNum);
      if (p_aec != NULL)
      {
        fclose(p_aec);
        p_aec = NULL;
      }
			if( ( p_aec=fopen(filename,"w+"))==0 )
			{
				 snprintf ( errortext, ET_SIZE, "Error open aec/slice_%04d_%04d.txt",infile,img->coding_order,sliceNum);
				 error ( errortext, 500 );
			}

            
			sprintf(filename,"./%s/bitstream/slice_%04d_%04d.txt",infile,img->coding_order,sliceNum);
      if (p_slice_bit != NULL)
      {
        fclose(p_slice_bit);
        p_slice_bit = NULL;
      }
			if( ( p_slice_bit=fopen(filename,"w+"))==0 )
			{
				 snprintf ( errortext, ET_SIZE, "Error open Bitstream/slice_%04d.txt!",sliceNum);
				 error ( errortext, 500 );
			}

#if EXTRACT_SliceByteAscii_08X  //按照2个字节后1个空格 %02x 2016-3-14      
			//写入slice start code
			fprintf(p_slice_bit,"%02x%02x ",buf[0],buf[1]);//00 00
			fprintf(p_slice_bit,"%02x%02x",buf[2],buf[3]);//01 start_code
#else
			//写入slice start code
			fwrite(&buf[0],1,1,p_slice_bit);//00
			fwrite(&buf[1],1,1,p_slice_bit);//00
			fwrite(&buf[2],1,1,p_slice_bit);//01
			fwrite(&buf[3],1,1,p_slice_bit);//start code
#endif

			//
		}
#endif  
  CheckType ( buf[3] ); //X ZHENG, 20080515, for demulation

  if ( buf[3] == SEQUENCE_END_CODE )
  {
    *length = 4;
    return -1;
  }

  k = 4;

#if EXTRACT_SliceByteAscii_08X
  unsigned int write_byte=4;
#endif

  while ( 1 )
  {
    i = read_n_bit ( pIRABS, 8, &j );
#if EXTRACT
	
#if EXTRACT_SliceByteAscii_08X  //按照2个字节后1个空格 %02x 2016-3-14      
	  //写入slice start code
	  b=(unsigned char)j;
    if(buf[3]>=SLICE_START_CODE_MIN && buf[3]<=SLICE_START_CODE_MAX && i==0)
    {
      if((write_byte%2)==0)//偶数个字节后打印空格
      	fprintf(p_slice_bit," ");//打印1个空格
      fprintf(p_slice_bit,"%02x",b);//打印1个空格
      write_byte++;
    }
#else

    b=(unsigned char)j;
    if(buf[3]>=SLICE_START_CODE_MIN && buf[3]<=SLICE_START_CODE_MAX && i==0)
    {
      fwrite(&b,1,1,p_slice_bit);
    }
#endif

#endif

    if ( i < 0 )
    {
      break;
    }

    buf[k++] = ( char ) j;
  }
#if EXTRACT
  if((write_byte&1)==1)//如果打印奇数个字节后打印ff
	  fprintf(p_slice_bit,"%02x",0xff);//打印1个ff

  if(buf[3]>=SLICE_START_CODE_MIN && buf[3]<=SLICE_START_CODE_MAX)
  {
    if (p_slice_bit != NULL)
    {
      fclose(p_slice_bit);
      p_slice_bit = NULL;
    }
  }
#endif

  if ( pIRABS->iClearBitsNum > 0 )
  {
    int shift;
    shift = 8 - pIRABS->iClearBitsNum;
    i = read_n_bit ( pIRABS, pIRABS->iClearBitsNum, &j );

    if ( j != 0 ) //qhg 20060327 for de-emulation.
    {
      buf[k++] = ( char ) ( j << shift );
    }

    StatBitsPtr->last_unit_bits += shift;  //rm52k_r2
  }

  *length = k;
  return k;
}






