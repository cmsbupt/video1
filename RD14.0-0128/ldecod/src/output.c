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
* File name:  output.c
* Function: Output an image and Trance support
*
*************************************************************************************
*/


#include "../../lcommon/inc/contributors.h"

#include "global.h"
#include "../../lcommon/inc/commonVariables.h"
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#include <IO.H>
#endif
#if AVS2_SCENE_CD
/*
*************************************************************************
* Function:Write decoded GB frame to output file
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
#ifdef WIN32
void write_GB_frame (int p_out )         //!< filestream to output file
#else
void write_GB_frame (FILE *p_out)         //!< filestream to output file
#endif
{
	int j;
#ifndef WIN32
  int i;
#endif
	int img_width = ( img->width - img->auto_crop_right );
	int img_height = ( img->height - img->auto_crop_bottom );
	int img_width_cr = ( img_width / 2 );
	int img_height_cr = ( img_height / ( input->chroma_format == 1 ? 2 : 1 ) );
#ifdef WIN32
	unsigned char *buf;
#if EXTEND_BD
	int nSampleSize = (input->output_bit_depth == 8 ? 1 : 2);
	int shift1 = input->sample_bit_depth - input->output_bit_depth; // assuming "sample_bit_depth" is greater or equal to "output_bit_depth"
	int k;
#endif
#if EXTEND_BD
	byte **imgY_out ;
	byte ***imgUV_out ;
#else
	unsigned char **imgY_out ;
	unsigned char ***imgUV_out ;
#endif
#else
#if EXTEND_BD
  unsigned char bufc;
	int shift1 = input->sample_bit_depth - input->output_bit_depth; // assuming "sample_bit_depth" is greater or equal to "output_bit_depth"
	byte **imgY_out;
	byte ***imgUV_out;
#else
	unsigned char **imgY_out ;
	unsigned char ***imgUV_out ;
#endif
#endif

	imgY_out = background_frame[0];
	imgUV_out = &background_frame[1];
	imgUV_out[0] = background_frame[1];
	imgUV_out[1] = background_frame[2];


#ifdef WIN32
#if EXTEND_BD
	buf = malloc ( nSampleSize * img_height * img_width );

	if (!shift1 && input->output_bit_depth == 8) // 8bit encode -> 8bit output
	{
		for (j = 0; j < img_height; j++)
		{
			for (k = 0; k < img_width; k++)
			{
				buf[j * img_width + k] = (unsigned char)imgY_out[j][k];
			}
		}
		_write (p_out, buf, img_height * img_width);
		for (j = 0; j < img_height_cr; j++)
		{
			for (k = 0; k < img_width_cr; k++)
			{
				buf[j * img_width_cr + k] = (unsigned char)imgUV_out[0][j][k];
			}
		}
		_write (p_out, buf, img_height_cr * img_width_cr);
		for (j = 0; j < img_height_cr; j++)
		{
			for (k = 0; k < img_width_cr; k++)
			{
				buf[j * img_width_cr + k] = (unsigned char)imgUV_out[1][j][k];
			}
		}
		_write (p_out, buf, img_height_cr * img_width_cr);
	}
	else if (!shift1 && input->output_bit_depth == 10) // 10bit encode -> 10bit output
	{
		for ( j = 0; j < img_height; j++ )
		{
			memcpy ( buf + j * img_width * nSampleSize, & ( imgY_out[j][0] ), nSampleSize * img_width );
		}

		_write ( p_out, buf, nSampleSize * img_height * img_width );

		for ( j = 0; j < img_height_cr; j++ )
		{
			memcpy ( buf + j * img_width_cr * nSampleSize, & ( imgUV_out[0][j][0] ), nSampleSize * img_width_cr );
		}

		_write ( p_out, buf, nSampleSize * img_height_cr * img_width_cr );

		for ( j = 0; j < img_height_cr; j++ )
		{
			memcpy ( buf + j * img_width_cr * nSampleSize, & ( imgUV_out[1][j][0] ), nSampleSize * img_width_cr );
		}

		_write ( p_out, buf, nSampleSize * img_height_cr * img_width_cr );
	}
	else if (shift1 && input->output_bit_depth == 8) // 10bit encode -> 8bit output
	{
		for (j = 0; j < img_height; j++)
		{
			for (k = 0; k < img_width; k++)
			{
				buf[j * img_width + k] = (unsigned char)Clip1((imgY_out[j][k] + (1 << (shift1 - 1))) >> shift1);
			}
		}
		_write (p_out, buf, img_height * img_width);
		for (j = 0; j < img_height_cr; j++)
		{
			for (k = 0; k < img_width_cr; k++)
			{
				buf[j * img_width_cr + k] = (unsigned char)Clip1((imgUV_out[0][j][k] + (1 << (shift1 - 1))) >> shift1);
			}
		}
		_write (p_out, buf, img_height_cr * img_width_cr);
		for (j = 0; j < img_height_cr; j++)
		{
			for (k = 0; k < img_width_cr; k++)
			{
				buf[j * img_width_cr + k] = (unsigned char)Clip1((imgUV_out[1][j][k] + (1 << (shift1 - 1))) >> shift1);
			}
		}
		_write (p_out, buf, img_height_cr * img_width_cr);
	}

	free ( buf );
#else
	buf = malloc ( img_height * img_width );

	for ( j = 0; j < img_height; j++ )
	{
		memcpy ( buf + j * img_width, & ( imgY_out[j][0] ), img_width );
	}

	_write ( p_out, buf, img_height * img_width );

	for ( j = 0; j < img_height_cr; j++ )
	{
		memcpy ( buf + j * img_width_cr, & ( imgUV_out[0][j][0] ), img_width_cr );
	}

	_write ( p_out, buf, img_height_cr * img_width_cr );

	for ( j = 0; j < img_height_cr; j++ )
	{
		memcpy ( buf + j * img_width_cr, & ( imgUV_out[1][j][0] ), img_width_cr );
	}

	_write ( p_out, buf, img_height_cr * img_width_cr );


	free ( buf );
#endif
#else
#if EXTEND_BD
	if (!shift1 && input->output_bit_depth == 8) // 8bit decoding -> 8bit output
	{
		for ( i=0; i < img_height; i++ )
		{
			for ( j=0; j < img_width; j++ )
			{
				bufc = (unsigned char)(imgY_out[i][j]);
				fputc ( bufc, p_out );
			}
		}
		for ( i=0; i < img_height_cr; i++ )
		{
			for ( j=0; j < img_width_cr; j++ )
			{
				bufc = (unsigned char)(imgUV_out[0][i][j]);
				fputc ( bufc, p_out );
			}
		}
		for ( i=0; i < img_height_cr; i++ )
		{
			for ( j=0; j < img_width_cr; j++ )
			{
				bufc = (unsigned char)(imgUV_out[1][i][j]);
				fputc ( bufc, p_out );
			}
		}
	}
	else if (!shift1 && input->output_bit_depth == 10) // 10bit decoding -> 10bit output
	{
		for ( i=0; i < img_height; i++ )
		{
			for ( j=0; j < img_width; j++ )
			{
				bufc = (unsigned char)(imgY_out[i][j]);
				fputc ( bufc, p_out );
				bufc = (unsigned char)(imgY_out[i][j] >> 8);
				fputc (bufc, p_out);
			}
		}
		for ( i=0; i < img_height_cr; i++ )
		{
			for ( j=0; j < img_width_cr; j++ )
			{
				bufc = (unsigned char)(imgUV_out[0][i][j]);
				fputc ( bufc, p_out );
				bufc = (unsigned char)(imgUV_out[0][i][j] >> 8);
				fputc (bufc, p_out);
			}
		}
		for ( i=0; i < img_height_cr; i++ )
		{
			for ( j=0; j < img_width_cr; j++ )
			{
				bufc = (unsigned char)(imgUV_out[1][i][j]);
				fputc ( bufc, p_out );
				bufc = (unsigned char)(imgUV_out[1][i][j] >> 8);
				fputc (bufc, p_out);
			}
		}
	}
	else if (shift1 && input->output_bit_depth == 8) // 10bit decoding -> 8bit output
	{
		for ( i=0; i < img_height; i++ )
		{
			for ( j=0; j < img_width; j++ )
			{
				bufc = (unsigned char)Clip1((imgY_out[i][j] + (1 << (shift1 - 1))) >> shift1);
				fputc ( bufc, p_out );
			}
		}
		for ( i=0; i < img_height_cr; i++ )
		{
			for ( j=0; j < img_width_cr; j++ )
			{
				bufc = (unsigned char)Clip1((imgUV_out[0][i][j] + (1 << (shift1 - 1))) >> shift1);
				fputc ( bufc, p_out );
			}
		}
		for ( i=0; i < img_height_cr; i++ )
		{
			for ( j=0; j < img_width_cr; j++ )
			{
				bufc = (unsigned char)Clip1((imgUV_out[1][i][j] + (1 << (shift1 - 1))) >> shift1);
				fputc ( bufc, p_out );
			}
		}
	}
	fflush ( p_out );
#else
  for ( i=0; i < img_height; i++ )
  {
    for ( j=0; j < img_width; j++ )
    {
      fputc ( imgY_out[i][j], p_out );
    }
  }
  for ( i=0; i < img_height_cr; i++ )
  {
    for ( j=0; j < img_width_cr; j++ )
    {
      fputc ( imgUV_out[0][i][j], p_out );
    }
  }
  for ( i=0; i < img_height_cr; i++ )
  {
    for ( j=0; j < img_width_cr; j++ )
    {
      fputc ( imgUV_out[1][i][j], p_out );
    }
  }
  fflush ( p_out );
#endif
#endif
}
#else
#ifdef AVS2_S2_BGLONGTERM // not working in 10bit profile!!!
/*
*************************************************************************
* Function:Write decoded GB frame to output file
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
void write_GB_frame(int p_dec)
{
#ifdef WIN32
	int l;
	for(l=0; l<img->height; l++)
	{
		_write(p_dec,imgY[l],img->width);
	}
	for(l=0; l<img->height_cr; l++)
	{
		_write(p_dec,imgUV[0][l],img->width_cr);
	}
	for(l=0; l<img->height_cr; l++)
	{
		_write(p_dec,imgUV[1][l],img->width_cr);
	}

#else
	int i,j;
    for ( i=0; i < img->height; i++ )
    {
      for ( j=0; j < img->width; j++ )
      {
        fputc( imgY[i][j], p_dec );
      }
    }
    for ( i=0; i < img->height_cr; i++ )
    {
      for ( j=0; j < img->width_cr; j++ )
      {
        fputc( imgUV[0][i][j], p_dec );
      }
    }
    for ( i=0; i < img->height_cr; i++ )
    {
      for ( j=0; j < img->width_cr; j++ )
      {
        fputc( imgUV[1][i][j], p_dec );
      }
    }
#endif
}
#endif //AVS2_S2_BGLONGTERM

#endif //AVS2_SCENE_CD
/*
*************************************************************************
* Function:Write decoded frame to output file
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
// Fix by Sunil for RD5.0 test in Linux (2013.11.06)
#ifdef WIN32
void write_frame (int p_out , int pos)         //!< filestream to output file
#else
void write_frame (FILE *p_out , int pos)         //!< filestream to output file
#endif
{
	int j;
#ifndef WIN32
  int i;
#endif
	int img_width = ( img->width - img->auto_crop_right );
	int img_height = ( img->height - img->auto_crop_bottom );
	int img_width_cr = ( img_width / 2 );
	int img_height_cr = ( img_height / ( input->chroma_format == 1 ? 2 : 1 ) );
// Fix by Sunil for RD5.0 test in Linux (2013.11.06)
#ifdef WIN32
	unsigned char *buf;
#if EXTEND_BD
	int nSampleSize = (input->output_bit_depth == 8 ? 1 : 2);
	int shift1 = input->sample_bit_depth - input->output_bit_depth; // assuming "sample_bit_depth" is greater or equal to "output_bit_depth"
	int k;
#endif
#if EXTEND_BD
	byte **imgY_out ;
	byte ***imgUV_out ;
#else
	unsigned char **imgY_out ;
	unsigned char ***imgUV_out ;
#endif
#else
#if EXTEND_BD
  unsigned char bufc;
	int shift1 = input->sample_bit_depth - input->output_bit_depth; // assuming "sample_bit_depth" is greater or equal to "output_bit_depth"
	byte **imgY_out;
	byte ***imgUV_out;
#else
	unsigned char **imgY_out ;
	unsigned char ***imgUV_out ;
#endif
#endif

	for ( j=0; j<REF_MAXBUFFER; j++)
	{
		if (img->imgtr_fwRefDistance[j] == pos)
		{
			imgY_out = ref[j][0];
			imgUV_out= &ref[j][1];

			img->is_output[j] = -1;
			if ( img->refered_by_others[j]==0 || img->imgcoi_ref[j] == -257 )
			{
				img->imgtr_fwRefDistance[j] = -256;
				img->imgcoi_ref[j] = -257;
#if M3480_TEMPORAL_SCALABLE
				img->temporal_id[j] = -1;
#endif
			}
			break;
		}
	}

// Fix by Sunil for RD5.0 test in Linux (2013.11.06)
#ifdef WIN32
#if EXTEND_BD
	buf = malloc ( nSampleSize * img_height * img_width );

	if (!shift1 && input->output_bit_depth == 8) // 8bit encode -> 8bit output
	{
		for (j = 0; j < img_height; j++)
		{
			for (k = 0; k < img_width; k++)
			{
				buf[j * img_width + k] = (unsigned char)imgY_out[j][k];
			}
		}
		_write (p_out, buf, img_height * img_width);
		for (j = 0; j < img_height_cr; j++)
		{
			for (k = 0; k < img_width_cr; k++)
			{
				buf[j * img_width_cr + k] = (unsigned char)imgUV_out[0][j][k];
			}
		}
		_write (p_out, buf, img_height_cr * img_width_cr);
		for (j = 0; j < img_height_cr; j++)
		{
			for (k = 0; k < img_width_cr; k++)
			{
				buf[j * img_width_cr + k] = (unsigned char)imgUV_out[1][j][k];
			}
		}
		_write (p_out, buf, img_height_cr * img_width_cr);
	}
	else if (!shift1 && input->output_bit_depth == 10) // 10bit encode -> 10bit output
	{
		for ( j = 0; j < img_height; j++ )
		{
			memcpy ( buf + j * img_width * nSampleSize, & ( imgY_out[j][0] ), nSampleSize * img_width );
		}

		_write ( p_out, buf, nSampleSize * img_height * img_width );

		for ( j = 0; j < img_height_cr; j++ )
		{
			memcpy ( buf + j * img_width_cr * nSampleSize, & ( imgUV_out[0][j][0] ), nSampleSize * img_width_cr );
		}

		_write ( p_out, buf, nSampleSize * img_height_cr * img_width_cr );

		for ( j = 0; j < img_height_cr; j++ )
		{
			memcpy ( buf + j * img_width_cr * nSampleSize, & ( imgUV_out[1][j][0] ), nSampleSize * img_width_cr );
		}

		_write ( p_out, buf, nSampleSize * img_height_cr * img_width_cr );
	}
	else if (shift1 && input->output_bit_depth == 8) // 10bit encode -> 8bit output
	{
		for (j = 0; j < img_height; j++)
		{
			for (k = 0; k < img_width; k++)
			{
				buf[j * img_width + k] = (unsigned char)Clip1((imgY_out[j][k] + (1 << (shift1 - 1))) >> shift1);
			}
		}
		_write (p_out, buf, img_height * img_width);
		for (j = 0; j < img_height_cr; j++)
		{
			for (k = 0; k < img_width_cr; k++)
			{
				buf[j * img_width_cr + k] = (unsigned char)Clip1((imgUV_out[0][j][k] + (1 << (shift1 - 1))) >> shift1);
			}
		}
		_write (p_out, buf, img_height_cr * img_width_cr);
		for (j = 0; j < img_height_cr; j++)
		{
			for (k = 0; k < img_width_cr; k++)
			{
				buf[j * img_width_cr + k] = (unsigned char)Clip1((imgUV_out[1][j][k] + (1 << (shift1 - 1))) >> shift1);
			}
		}
		_write (p_out, buf, img_height_cr * img_width_cr);
	}

	free ( buf );
#else
	buf = malloc ( img_height * img_width );

	for ( j = 0; j < img_height; j++ )
	{
		memcpy ( buf + j * img_width, & ( imgY_out[j][0] ), img_width );
	}

	_write ( p_out, buf, img_height * img_width );

	for ( j = 0; j < img_height_cr; j++ )
	{
		memcpy ( buf + j * img_width_cr, & ( imgUV_out[0][j][0] ), img_width_cr );
	}

	_write ( p_out, buf, img_height_cr * img_width_cr );

	for ( j = 0; j < img_height_cr; j++ )
	{
		memcpy ( buf + j * img_width_cr, & ( imgUV_out[1][j][0] ), img_width_cr );
	}

	_write ( p_out, buf, img_height_cr * img_width_cr );


	free ( buf );
#endif
#else
#if EXTEND_BD
	if (!shift1 && input->output_bit_depth == 8) // 8bit decoding -> 8bit output
	{
		for ( i=0; i < img_height; i++ )
		{
			for ( j=0; j < img_width; j++ )
			{
				bufc = (unsigned char)(imgY_out[i][j]);
				fputc ( bufc, p_out );
			}
		}
		for ( i=0; i < img_height_cr; i++ )
		{
			for ( j=0; j < img_width_cr; j++ )
			{
				bufc = (unsigned char)(imgUV_out[0][i][j]);
				fputc ( bufc, p_out );
			}
		}
		for ( i=0; i < img_height_cr; i++ )
		{
			for ( j=0; j < img_width_cr; j++ )
			{
				bufc = (unsigned char)(imgUV_out[1][i][j]);
				fputc ( bufc, p_out );
			}
		}
	}
	else if (!shift1 && input->output_bit_depth == 10) // 10bit decoding -> 10bit output
	{
		for ( i=0; i < img_height; i++ )
		{
			for ( j=0; j < img_width; j++ )
			{
				bufc = (unsigned char)(imgY_out[i][j]);
				fputc ( bufc, p_out );
				bufc = (unsigned char)(imgY_out[i][j] >> 8);
				fputc (bufc, p_out);
			}
		}
		for ( i=0; i < img_height_cr; i++ )
		{
			for ( j=0; j < img_width_cr; j++ )
			{
				bufc = (unsigned char)(imgUV_out[0][i][j]);
				fputc ( bufc, p_out );
				bufc = (unsigned char)(imgUV_out[0][i][j] >> 8);
				fputc (bufc, p_out);
			}
		}
		for ( i=0; i < img_height_cr; i++ )
		{
			for ( j=0; j < img_width_cr; j++ )
			{
				bufc = (unsigned char)(imgUV_out[1][i][j]);
				fputc ( bufc, p_out );
				bufc = (unsigned char)(imgUV_out[1][i][j] >> 8);
				fputc (bufc, p_out);
			}
		}
	}
	else if (shift1 && input->output_bit_depth == 8) // 10bit decoding -> 8bit output
	{
		for ( i=0; i < img_height; i++ )
		{
			for ( j=0; j < img_width; j++ )
			{
				bufc = (unsigned char)Clip1((imgY_out[i][j] + (1 << (shift1 - 1))) >> shift1);
				fputc ( bufc, p_out );
			}
		}
		for ( i=0; i < img_height_cr; i++ )
		{
			for ( j=0; j < img_width_cr; j++ )
			{
				bufc = (unsigned char)Clip1((imgUV_out[0][i][j] + (1 << (shift1 - 1))) >> shift1);
				fputc ( bufc, p_out );
			}
		}
		for ( i=0; i < img_height_cr; i++ )
		{
			for ( j=0; j < img_width_cr; j++ )
			{
				bufc = (unsigned char)Clip1((imgUV_out[1][i][j] + (1 << (shift1 - 1))) >> shift1);
				fputc ( bufc, p_out );
			}
		}
	}
	fflush ( p_out );
#else
  for ( i=0; i < img_height; i++ )
  {
    for ( j=0; j < img_width; j++ )
    {
      fputc ( imgY_out[i][j], p_out );
    }
  }
  for ( i=0; i < img_height_cr; i++ )
  {
    for ( j=0; j < img_width_cr; j++ )
    {
      fputc ( imgUV_out[0][i][j], p_out );
    }
  }
  for ( i=0; i < img_height_cr; i++ )
  {
    for ( j=0; j < img_width_cr; j++ )
    {
      fputc ( imgUV_out[1][i][j], p_out );
    }
  }
  fflush ( p_out );
#endif
#endif
}

#if TRACE
static int bitcounter = 0;

/*
*************************************************************************
* Function:Tracing bitpatterns for symbols
A code word has the following format: 0 Xn...0 X2 0 X1 0 X0 1
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

void tracebits (
  const char *trace_str,  //!< tracing information, char array describing the symbol
  int len,                //!< length of syntax element in bits
  int info,               //!< infoword of syntax element
  int value1 )
{

  int i, chars;

  if ( len >= 34 )
  {
    snprintf ( errortext, ET_SIZE, "Length argument to put too long for trace to work" );
    error ( errortext, 600 );
  }

  putc ( '@', p_trace );
  chars = fprintf ( p_trace, "%i", bitcounter );

  while ( chars++ < 6 )
  {
    putc ( ' ', p_trace );
  }

  chars += fprintf ( p_trace, "%s", trace_str );

  while ( chars++ < 55 )
  {
    putc ( ' ', p_trace );
  }

  // Align bitpattern
  if ( len < 15 )
  {
    for ( i = 0 ; i < 15 - len ; i++ )
    {
      fputc ( ' ', p_trace );
    }
  }

  // Print bitpattern
  for ( i = 0 ; i < len / 2 ; i++ )
  {
    fputc ( '0', p_trace );
  }

  // put 1
  fprintf ( p_trace, "1" );

  // Print bitpattern
  for ( i = 0 ; i < len / 2 ; i++ )
  {
    if ( 0x01 & ( info >> ( ( len / 2 - i ) - 1 ) ) )
    {
      fputc ( '1', p_trace );
    }
    else
    {
      fputc ( '0', p_trace );
    }
  }

  fprintf ( p_trace, "  (%3d)\n", value1 );
  bitcounter += len;

  fflush ( p_trace );

}

/*
*************************************************************************
* Function:Tracing bitpatterns
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/


void tracebits2 (
  const char *trace_str,  //!< tracing information, char array describing the symbol
  int len,                //!< length of syntax element in bits
  int info )
{

  int i, chars;

  if ( len >= 45 )
  {
    snprintf ( errortext, ET_SIZE, "Length argument to put too long for trace to work" );
    error ( errortext, 600 );
  }

  putc ( '@', p_trace );
  chars = fprintf ( p_trace, "%i", bitcounter );

  while ( chars++ < 6 )
  {
    putc ( ' ', p_trace );
  }

  chars += fprintf ( p_trace, "%s", trace_str );

  while ( chars++ < 55 )
  {
    putc ( ' ', p_trace );
  }

  // Align bitpattern
  if ( len < 15 )
    for ( i = 0 ; i < 15 - len ; i++ )
    {
      fputc ( ' ', p_trace );
    }

  bitcounter += len;

  while ( len >= 32 )
  {
    for ( i = 0 ; i < 8 ; i++ )
    {
      fputc ( '0', p_trace );
    }

    len -= 8;

  }

  // Print bitpattern
  for ( i = 0 ; i < len ; i++ )
  {
    if ( 0x01 & ( info >> ( len - i - 1 ) ) )
    {
      fputc ( '1', p_trace );
    }
    else
    {
      fputc ( '0', p_trace );
    }
  }

  fprintf ( p_trace, "  (%3d)\n", info );

  fflush ( p_trace );

}

#endif

