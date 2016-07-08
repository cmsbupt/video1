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
* File name:
* Function:
*
*************************************************************************************
*/
#define INCLUDED_BY_CONFIGFILE_C

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <memory.h>

#ifdef WIN32
#include <IO.H>
#include <fcntl.h>
#endif

#include "global.h"
#include "../../lcommon/inc/commonVariables.h"
#include "configfile.h"
#if TDRDO
#include "tdrdo.h"
#endif

#define MAX_ITEMS_TO_PARSE  10000
#define _S_IREAD      0000400         /* read permission, owner */
#define _S_IWRITE     0000200         /* write permission, owner */

static char *GetConfigFileContent ( char *Filename );
static void ParseContent ( char *buf, int bufsize );
static int ParameterNameToMapIndex ( char *s );
static void PatchInp ();

static int ParseRefContent(char **buf);

/*
*************************************************************************
* Function:Parse the command line parameters and read the config files.
* Input: ac
number of command line parameters
av
command line parameters
* Output:
* Return:
* Attention:
*************************************************************************
*/

void Configure ( int ac, char *av[] )
{
  char *content;
  int CLcount, ContentLen, NumberParams;

  memset ( &configinput, 0, sizeof ( InputParameters ) );

  // Process default config file
  // Parse the command line

  CLcount = 1;

  gop_size = -1;

  while ( CLcount < ac )
  {
    if ( 0 == strncmp ( av[CLcount], "-f", 2 ) ) // A file parameter?
    {
      content = GetConfigFileContent ( av[CLcount + 1] );
      printf ( "Parsing Configfile %s", av[CLcount + 1] );
      ParseContent ( content, strlen ( content ) );
      printf ( "\n" );
      free ( content );
      CLcount += 2;
    }
    else
    {
      if ( 0 == strncmp ( av[CLcount], "-p", 2 ) ) // A config change?
      {
        // Collect all data until next parameter (starting with -<x> (x is any character)),
        // put it into content, and parse content.
        CLcount++;
        ContentLen = 0;
        NumberParams = CLcount;

        // determine the necessary size for content
        while ( NumberParams < ac && av[NumberParams][0] != '-' )
        {
          ContentLen += strlen ( av[NumberParams++] );  // Space for all the strings
        }

        ContentLen += 1000;                     // Additional 1000 bytes for spaces and \0s

        if ( ( content = malloc ( ContentLen ) ) == NULL )
        {
          no_mem_exit ( "Configure: content" );
        }

        content[0] = '\0';

        // concatenate all parameters itendified before
        while ( CLcount < NumberParams )
        {
          char *source = &av[CLcount][0];
          char *destin = &content[strlen ( content ) ];

          while ( *source != '\0' )
          {
            if ( *source == '=' ) // The Parser expects whitespace before and after '='
            {
              *destin++ = ' ';
              *destin++ = '=';
              *destin++ = ' '; // Hence make sure we add it
            }
            else
            {
              *destin++ = *source;
            }

            source++;
          }

          *destin++ = ' ';      // add a space to support multiple config items
          *destin = '\0';
          CLcount++;
        }

        printf ( "Parsing command line string '%s'", content );
        ParseContent ( content, strlen ( content ) );
        free ( content );
        printf ( "\n" );
      }
      else
      {
        snprintf ( errortext, ET_SIZE, "Error in command line, ac %d, around string '%s', missing -f or -p parameters?", CLcount, av[CLcount] );
        error ( errortext, 300 );
      }
    }
  }

#if D20141230_BUG_FIX
  input->b_dmh_enabled = 1;
#endif 

  PatchInp();  //rm52k
  input->successive_Bframe = input->successive_Bframe_all;
  printf ( "\n" );
}

/*
*************************************************************************
* Function: Alocate memory buf, opens file Filename in f, reads contents into
buf and returns buf
* Input:name of config file
* Output:
* Return:
* Attention:
*************************************************************************
*/
char *GetConfigFileContent ( char *Filename )
{
  unsigned FileSize;
  FILE *f;
  char *buf;

  if ( NULL == ( f = fopen ( Filename, "r" ) ) )
  {
    snprintf ( errortext, ET_SIZE, "Cannot open configuration file %s.\n", Filename );
    error ( errortext, 300 );
  }

  if ( 0 != fseek ( f, 0, SEEK_END ) )
  {
    snprintf ( errortext, ET_SIZE, "Cannot fseek in configuration file %s.\n", Filename );
    error ( errortext, 300 );
  }

  FileSize = ftell ( f );

  if ( FileSize < 0 || FileSize > 60000 )
  {
    snprintf ( errortext, ET_SIZE, "Unreasonable Filesize %d reported by ftell for configuration file %s.\n", FileSize, Filename );
    error ( errortext, 300 );
  }

  if ( 0 != fseek ( f, 0, SEEK_SET ) )
  {
    snprintf ( errortext, ET_SIZE, "Cannot fseek in configuration file %s.\n", Filename );
    error ( errortext, 300 );
  }

  if ( ( buf = malloc ( FileSize + 1 ) ) == NULL )
  {
    no_mem_exit ( "GetConfigFileContent: buf" );
  }

  // Note that ftell() gives us the file size as the file system sees it.  The actual file size,
  // as reported by fread() below will be often smaller due to CR/LF to CR conversion and/or
  // control characters after the dos EOF marker in the file.

  FileSize = fread ( buf, 1, FileSize, f );
  buf[FileSize] = '\0';

  fclose ( f );

  return buf;
}

void print_ref_man(int v)
{
	int i,j;
	ref_man *p;
	if(v)
		printf("\n DOI POI type qpof rfby nref reflist      nrem remlist");
	else
		printf("\n POI DOI type qpof rfby nref reflist      nrem remlist");

	for(j=0;j<gop_size_all;j++)
	{
		p = cfg_ref_all + j;
		printf("\n %2d %3d %4d %4d %4d",j+1,p->poc,p->type,p->qp_offset,p->referd_by_others);
		//if(p->predict > 0)
		//	printf("%2d",p->deltaRPS);
		//else
			printf("  ");
		printf(" %4d ",p->num_of_ref);
		for(i=0;i<p->num_of_ref;i++)
			printf(" %2d",p->ref_pic[i]);
		for(i=0;i<MAXREF - p->num_of_ref;i++)
			printf("   ");
		printf(" %4d ",p->num_to_remove);
		for(i=0;i<p->num_to_remove;i++)
			printf(" %2d",p->remove_pic[i]);
		for(i=0;i<MAXREF - p->num_to_remove;i++)
			printf("   ");
		if( configinput.TemporalScalableFlag == 1 )
			printf(" %2d",p->temporal_id);
	}
	printf("\n");
}
void TranslateRPS()
{
	ref_man tmp_cfg_ref[MAXGOP];
	ref_man *p;
	int i,j;
	int poi,doi;
	for(i=0;i<gop_size_all;i++)
	{
		tmp_cfg_ref[i] = cfg_ref_all[i];
	}
	//按照DOI的顺序重新排列RPS
	for(i=0;i<gop_size_all;i++)
	{
		j = tmp_cfg_ref[i].poc - 1;//重排前.poc里放的实际上是DOI（从1开始），而POI则是重排前的顺序索引，即i
		cfg_ref_all[j] = tmp_cfg_ref[i];
		cfg_ref_all[j].poc = i+1;//poi也是从1开始
	}
	//下面把用POI表示的参考帧和删除帧翻译成用deltaDOI表示
	for(i=0;i<gop_size_all;i++)
	{
		p = cfg_ref_all + i;//i+1是当前图像的doi
		for(j=0;j<p->num_of_ref;j++)
		{
			int n;
			poi = p->ref_pic[j];//取第j个参考图像的poi，从1开始
			n = 0;
			while(poi<1)//把poi调整到1到gop_size_all之间
			{
				poi += gop_size_all;
				n += gop_size_all;
			}
			doi = tmp_cfg_ref[poi - 1].poc;//doi从1开始
			p->ref_pic[j] = i+1 - doi + n;
		}
		for(j=0;j<p->num_to_remove;j++)
		{
			int n;
			poi = p->remove_pic[j];//取第j个参考图像的poi，从1开始
			n = 0;
			while(poi<1)//把poi调整到1到gop_size_all之间
			{
				poi += gop_size_all;
				n += gop_size_all;
			}
			doi = tmp_cfg_ref[poi - 1].poc;//doi从1开始
			p->remove_pic[j] = i+1 - doi + n;
		}
	}
#ifdef ZHAOHAIWU20150420
	if(gop_size_all==24)//GOP24
		{
		if (cfg_ref_all[0].poc==8)
		for(j=8;j<24;j++)//GOP24结构是GOP8+GOP16
		{
			cfg_ref_all[j].poc-=8;
		}
		else if (cfg_ref_all[0].poc==16)
		for(j=16;j<24;j++)//GOP24结构为GOP16+GOP8
		{
			cfg_ref_all[j].poc-=16;
		}
		}
#else
	if(cfg_ref_all[0].poc < gop_size_all)//there are sub gops
	{
		int sub_gop_size, sub_gop_start, sub_gop_num=1;
		sub_gop_size  = cfg_ref_all[0].poc;
		sub_gop_start = sub_gop_size;
		while(sub_gop_start < gop_size_all)
		{
			for(j=sub_gop_start; j<gop_size_all; j++)
			{
				cfg_ref_all[j].poc -= sub_gop_size;//减去上一个subgop的图像数
			}
			sub_gop_size  = cfg_ref_all[sub_gop_start].poc;//取下一个subgop的图像数
			sub_gop_start += sub_gop_size;//计算再下一个subgop的起点
			sub_gop_num++;//subgop计数，可以用来做合法性检查。
			//顺便多说一句，sub_gop_size可以用来检查与配置参数NumberBFrames的一致性
		}
	}
#endif
}
int ParseRefContent(char **buf)
{
	ref_man *tmp;
	int flag = 0;
	int i = 1;
	int j = 0;
	char* token;
	char **p = buf;
	char* tmp_str=":";
	char str[3];
	char headstr[10]={'F','r','a','m','e','\0','\0','\0','\0','\0'};

// Fix by Sunil for RD5.0 test in Linux (2013.11.06)
#ifdef WIN32
	_itoa(i,str,10);//ZHAOHAIWU
#else
	sprintf(str,"%d",i);
#endif
	strcat(headstr,str);
	strcat(headstr,tmp_str);

	memset(cfg_ref_all,-1,sizeof(struct reference_management)*MAXGOP);

	while (0 == strcmp(headstr, *p++))
	{
		tmp = cfg_ref_all+i-1;
		token = *p++;
		tmp->poc = atoi(token);
		token = *p++;
		tmp->qp_offset = atoi(token);
		token = *p++;
		tmp->num_of_ref = atoi(token);
		token = *p++;
		tmp->referd_by_others = atoi(token);
		for (j=0; j<tmp->num_of_ref; j++)
		{
			token =  *p++;
			tmp->ref_pic[j] = atoi(token);
		}

#if REF_BUG_FIX
        //check the reference configuration
        for (j = 0; j < tmp->num_of_ref - 1; j++)
        {
            if (tmp->ref_pic[j] < tmp->ref_pic[j + 1]){
                printf("wrong reference configuration");
                exit(-1);
            }
        }
#endif

		token = *p++;
		/*tmp->predict = atoi(token);
		if (0 != tmp->predict)
		{
			token = *p++;
			tmp->deltaRPS = atoi(token);
		}
		token =  *p++;*/
		tmp->num_to_remove = atoi(token);
		for (j=0;j<tmp->num_to_remove;j++)
		{
			token =  *p++;
			tmp->remove_pic[j] = atoi(token);
		}

#if M3480_TEMPORAL_SCALABLE
		if( configinput.TemporalScalableFlag == 1 )
		{
			token =  *p++;
			tmp->temporal_id = atoi(token);
		}
#endif

		i++;
		headstr[5] = headstr[6] = headstr[7] = headstr[8] = headstr[9] = '\0';
// Fix by Sunil for RD5.0 test in Linux (2013.11.06)
#ifdef WIN32
		_itoa(i,str,10);//ZHAOHAIWU
#else
    sprintf(str,"%d",i);
#endif
		strcat(headstr,str);
		strcat(headstr,tmp_str);
	}

	gop_size_all = i-1;
#if TDRDO
	// This is a copy of QP offset from cfg files.
	for(j=0; j<gop_size_all; j++)
		QpOffset[j] = cfg_ref_all[j].qp_offset;
	GroupSize = gop_size_all;
#endif
	//print_ref_man(0);
	TranslateRPS();
	//print_ref_man(1);
	return (p-buf-1);
}
/*
*************************************************************************
* Function: Parses the character array buf and writes global variable input, which is defined in
configfile.h.  This hack will continue to be necessary to facilitate the addition of
new parameters through the Map[] mechanism (Need compiler-generated addresses in map[]).
* Input:  buf
buffer to be parsed
bufsize
buffer size of buffer
* Output:
* Return:
* Attention:
*************************************************************************
*/

void ParseContent ( char *buf, int bufsize )
{
  char *items[MAX_ITEMS_TO_PARSE];
  int MapIdx;
  int item     = 0;
  int InString = 0;
  int InItem   = 0;
  char *p      = buf;
  char *bufend = &buf[bufsize];
  int IntContent;
  int i;

  char headstr[10] = {'F','r','a','m','e', '1', ':', '\0'};

  // Stage one: Generate an argc/argv-type list in items[], without comments and whitespace.
  // This is context insensitive and could be done most easily with lex(1).

  while ( p < bufend )
  {
    switch ( *p )
    {
    case 13:
      p++;
      break;
    case '#':                 // Found comment
      *p = '\0';              // Replace '#' with '\0' in case of comment immediately following integer or string

      while ( *p != '\n' && p < bufend ) // Skip till EOL or EOF, whichever comes first
      {
        p++;
      }

      InString = 0;
      InItem = 0;
      break;
    case '\n':
      InItem = 0;
      InString = 0;
      *p++ = '\0';
      break;
    case ' ':
    case '\t':              // Skip whitespace, leave state unchanged

      if ( InString )
      {
        p++;
      }
      else
      {
        // Terminate non-strings once whitespace is found
        *p++ = '\0';
        InItem = 0;
      }

      break;
    case '"':               // Begin/End of String
      *p++ = '\0';

      if ( !InString )
      {
        items[item++] = p;
        InItem = ~InItem;
      }
      else
      {
        InItem = 0;
      }

      InString = ~InString; // Toggle
      break;
    default:

      if ( !InItem )
      {
        items[item++] = p;
        InItem = ~InItem;
      }

      p++;
    }
  }

  item--;

  for ( i = 0; i < item; i += 3 )
  {
	  if (0 == strcmp(items[i], headstr))
	  {
		  i += ParseRefContent(&items[i]);
	  }

    if ( 0 > ( MapIdx = ParameterNameToMapIndex ( items[i] ) ) )
    {
      snprintf ( errortext, ET_SIZE, " Parsing error in config file: Parameter Name '%s' not recognized.", items[i] );
      error ( errortext, 300 );
    }

    if ( strcmp ( "=", items[i + 1] ) )
    {
      snprintf ( errortext, ET_SIZE, " Parsing error in config file: '=' expected as the second token in each line." );
      error ( errortext, 300 );
    }

    // Now interprete the Value, context sensitive...
    switch ( Map[MapIdx].Type )
    {
    case 0:           // Numerical

      if ( 1 != sscanf ( items[i + 2], "%d", &IntContent ) )
      {
        snprintf ( errortext, ET_SIZE, " Parsing error: Expected numerical value for Parameter of %s, found '%s'.", items[i], items[i + 2] );
        error ( errortext, 300 );
      }

      * ( int * ) ( Map[MapIdx].Place ) = IntContent;
      printf ( "." );
      break;
    case 1:
      strcpy ( ( char * ) Map[MapIdx].Place, items [i + 2] );
      printf ( "." );
      break;
    default:
      assert ( "Unknown value type in the map definition of configfile.h" );
    }
  }

  memcpy ( input, &configinput, sizeof ( InputParameters ) );
#if M3480_TEMPORAL_SCALABLE
  if( input->TemporalScalableFlag == 1 )
	  temporal_id_exist_flag = 1; 
  else
	  temporal_id_exist_flag = 0;
#endif
}

/*
*************************************************************************
* Function:Return the index number from Map[] for a given parameter name.
* Input:parameter name string
* Output:
* Return: the index number if the string is a valid parameter name,         \n
-1 for error
* Attention:
*************************************************************************
*/

static int ParameterNameToMapIndex ( char *s )
{
  int i = 0;

  while ( Map[i].TokenName != NULL )
  {
    if ( 0 == strcmp ( Map[i].TokenName, s ) )
    {
      return i;
    }
    else
    {
      i++;
    }
  }

  return -1;
};

/*
*************************************************************************
* Function:Checks the input parameters for consistency.
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

static void PatchInp ()
{
#if M3198_CU8
  unsigned int ui_MaxSize = 8;
#else
  unsigned int ui_MaxSize = 16;
#endif

#if INTERLACE_CODING
  input->org_img_height = input->img_height;
  input->org_img_width = input->img_width;
  input->org_no_frames = input->no_frames;
  if (input->InterlaceCodingOption == 3)
  {
    img->is_field_sequence = 1;
    input->img_height = input->img_height/2;
    input->no_frames = input->no_frames*2;
    input->intra_period = input->intra_period*2;
  }
  else
  {
    img->is_field_sequence = 0;
  }
#endif

  // consistency check of QPs
#if MB_DQP  
  if (input->useDQP)
  {
	  input->fixed_picture_qp = 0;/*lgp*/ 
  } 
  else
  {
	  input->fixed_picture_qp = 1;
  }
#else
  input->fixed_picture_qp = 1;
#endif

  if (input->profile_id == 0x12 && input->intra_period != 1)
  {
    snprintf ( errortext, ET_SIZE, "Baseline picture file only supports intra picture coding!" );
    error ( errortext, 400 );
  }

  if (input->profile_id == 0x20 && (input->sample_bit_depth > 8 || input->input_sample_bit_depth > 8))
  {
    snprintf ( errortext, ET_SIZE, "Baseline file only supports 8-bit coding!" );
    error ( errortext, 400 );
  }

  if ( input->qpI > MAX_QP + (input->sample_bit_depth - 8)*8 || input->qpI < MIN_QP )
  {
    snprintf ( errortext, ET_SIZE, "Error input parameter quant_I,check configuration file" );
    error ( errortext, 400 );
  }

  if ( input->qpP > MAX_QP + (input->sample_bit_depth - 8)*8 || input->qpP < MIN_QP )
  {
    snprintf ( errortext, ET_SIZE, "Error input parameter quant_P,check configuration file" );
    error ( errortext, 400 );
  }

  if ( input->qpB > MAX_QP + (input->sample_bit_depth - 8)*8 || input->qpB < MIN_QP )
  {
    snprintf ( errortext, ET_SIZE, "Error input parameter quant_B,check configuration file" );
    error ( errortext, 400 );
  }

  if ( !input->useNSQT && input->useSDIP )
  {
    snprintf ( errortext, ET_SIZE, "Error SDIP shouldn't be tunned on when NSQT is off!" );
    error ( errortext, 400 );
  }


#if M3198_CU8
  if ( input->g_uiMaxSizeInBit > 6 || input->g_uiMaxSizeInBit < 3 )
#else
  if ( input->g_uiMaxSizeInBit > 6 || input->g_uiMaxSizeInBit < 4 )
#endif
  {
    snprintf ( errortext, ET_SIZE, "Error input parameter MaxSizeInBit,check configuration file" );
    error ( errortext, 400 );
  }

  // consistency check no_multpred
  if ( input->no_multpred < 1 )
  {
    input->no_multpred = 1;
  }

#if !INTERLACE_CODING
  //20080721
  if ( input->progressive_sequence == 1 && input->progressive_frame == 0 )
  {
    snprintf ( errortext, ET_SIZE, "\nprogressive_frame should be set to 1 in the case of progressive sequence input!" );
    error ( errortext, 400 );
  }

  if ( input->progressive_frame == 1 && input->InterlaceCodingOption != 0 )
  {
    snprintf ( errortext, ET_SIZE, "\nProgressive frame cann't use interlace coding!" );
    error ( errortext, 400 );
  }
#endif
#if INTERLACE_CODING_FIX
  if ( input->InterlaceCodingOption == 1 )
  {
    snprintf ( errortext, ET_SIZE, "\nfield coding in a frame is forbidden in AVS2-baseline!" );
    error ( errortext, 400 );
  }
  if ( input->InterlaceCodingOption == 2 )
  {
    snprintf ( errortext, ET_SIZE, "\npicture adaptive frame/field coding (PAFF) is forbidden in AVS2-baseline!" );
    error ( errortext, 400 );
  }
  if ( input->InterlaceCodingOption == 3 && input->progressive_sequence == 1 )
  {
    snprintf ( errortext, ET_SIZE, "\nfield picture coding  is forbidden when progressive_sequence is '1'" );
    error ( errortext, 400 );
  }
  if ( input->progressive_sequence == 1 && input->progressive_frame == 0 )
  {
    snprintf ( errortext, ET_SIZE, "\nprogressive_frame should be set to 1 when progressive_sequence is '1'!" );
    error ( errortext, 400 );
  }
#endif

  //20080721
  //qyu 0817 padding

  if ( input->img_width % ui_MaxSize != 0 )
  {
    img->auto_crop_right = ui_MaxSize - ( input->img_width % ui_MaxSize );
  }
  else
  {
    img->auto_crop_right = 0;
  }
#if INTERLACE_CODING
  if ( !input->progressive_sequence && !img->is_field_sequence )   //only used for frame picture's field coding
#else
  if ( !input->progressive_sequence ) //20080721
#endif
  {
    if ( input->img_height % ( ui_MaxSize << 1 ) != 0 )
    {
      img->auto_crop_bottom = ( ui_MaxSize << 1 ) - ( input->img_height % ( ui_MaxSize << 1 ) );
    }
    else
    {
      img->auto_crop_bottom = 0;
    }
  }
  else
  {
    if ( input->img_height % ui_MaxSize != 0 )
    {
      img->auto_crop_bottom = ui_MaxSize - ( input->img_height % ui_MaxSize );
    }
    else
    {
      img->auto_crop_bottom = 0;
    }
  }
  if ( input->slice_row_nr != 0 && input->slice_row_nr > ( ( input->img_height / ui_MaxSize )*(input->img_width / ui_MaxSize ) ) )
  {
    snprintf ( errortext, ET_SIZE, "\nNumber of LCUs in slice cannot exceed total LCU in picture!" );
    error ( errortext, 400 );
  }
#if M3198_CU8
#else
  input->slice_row_nr = input->slice_row_nr * ( 1 << ( input->g_uiMaxSizeInBit - 4 ) );
#endif

  // check range of filter offsets
  if ( input->alpha_c_offset > 8 || input->alpha_c_offset < -8 ) //20080721
  {
    snprintf ( errortext, ET_SIZE, "Error input parameter LFAlphaC0Offset, check configuration file" );
    error ( errortext, 400 );
  }

  if ( input->beta_offset > 8 || input->beta_offset < -8 ) //20080721
  {
    snprintf ( errortext, ET_SIZE, "Error input parameter LFBetaOffset, check configuration file" );
    error ( errortext, 400 );
  }

  // Set block sizes

  // B picture consistency check
  if ( input->successive_Bframe > input->jumpd )
  {
    snprintf ( errortext, ET_SIZE, "Number of B-frames %d can not exceed the number of frames skipped", input->successive_Bframe );
    error ( errortext, 400 );
  }
  if ( input->successive_Bframe_all == 0 )
  {
    input->low_delay = 1;
  }
  else
  {
    input->low_delay = 0;
  }

  // Open Files
#ifdef WIN32

  if ( strlen ( input->infile ) > 0 && ( p_in = open ( input->infile, O_RDONLY | O_BINARY ) ) == -1 )
#else
  if ( strlen ( input->infile ) > 0 && ( p_in = fopen ( input->infile, "rb" ) ) == NULL )
#endif
  {
    snprintf ( errortext, ET_SIZE, "Input file %s does not exist", input->infile );
  }

#ifdef WIN32

  if ( input->output_enc_pic ) //output_enc_pic
    if ( strlen ( input->ReconFile ) > 0 && ( p_dec = open ( input->ReconFile, _O_WRONLY | _O_CREAT | _O_BINARY | _O_TRUNC, _S_IREAD | _S_IWRITE ) ) == -1 )
#else
  if ( input->output_enc_pic ) //output_enc_pic
    if ( strlen ( input->ReconFile ) > 0 && ( p_dec = fopen ( input->ReconFile, "wb" ) ) == NULL )
#endif
    {
      snprintf ( errortext, ET_SIZE, "Error open file %s", input->ReconFile );
    }
#ifdef AVS2_S2_BGLONGTERM
#ifdef WIN32
#if AVS2_SCENE_CD
  if ( input->output_enc_pic && input->bg_enable) //output_enc_pic
#else
  if ( input->output_enc_pic && input->profile_id == 0x50 && input->bg_enable) //output_enc_pic
#endif
    if ( strlen ( input->ReconFile ) > 0 && ( p_dec_background = open ( "background_ref.yuv", _O_WRONLY | _O_CREAT | _O_BINARY | _O_TRUNC, _S_IREAD | _S_IWRITE ) ) == -1 )
#else
#if AVS2_SCENE_CD
  if ( input->output_enc_pic && input->bg_enable) //output_enc_pic
#else
  if ( input->output_enc_pic && input->profile_id == 0x50 && input->bg_enable) //output_enc_pic
#endif
	if ( strlen ( input->ReconFile ) > 0 && ( p_dec_background = fopen ( "background_ref.yuv", "wb" ) ) == NULL )
#endif
	{
		snprintf ( errortext, ET_SIZE, "Error open file background_ref.yuv" );
	}
#endif
#if TRACE
  if ( strlen ( input->TraceFile ) > 0 && ( p_trace = fopen ( input->TraceFile, "w" ) ) == NULL )
  {
    snprintf ( errortext, ET_SIZE, "Error open file %s", input->TraceFile );
  }
#endif
  // frame/field consistency check

  // input intra period and Seqheader check Add cjw, 20070327
  if ( input->seqheader_period != 0 && input->intra_period == 0 )
  {
    if ( input->intra_period == 0 )
    {
      snprintf ( errortext, ET_SIZE, "\nintra_period  should not equal %d when seqheader_period equal %d", input->intra_period, input->seqheader_period );
    }

    error ( errortext, 400 );
  }

  // input intra period and Seqheader check Add cjw, 20070327
  if ( input->seqheader_period == 0 && input->vec_period != 0 )
  {
    snprintf ( errortext, ET_SIZE, "\nvec_period  should not equal %d when seqheader_period equal %d", input->intra_period, input->seqheader_period );
    error ( errortext, 400 );
  }

  if (input->profile_id == BASELINE_PROFILE && (input->sample_bit_depth > 8 || input->input_sample_bit_depth > 8))
  {
    snprintf ( errortext, ET_SIZE, "\nThe value of sample_bit_depth or input_sample_bit_depth is invalid!" );
    error ( errortext, 400 );
  }

  // Added by LiShao, Tsinghua, 20070327
  //ProfileCheck();
  //LevelCheck();
}

/*
******************************************************************************
*  Function: Determine the MVD's value (1/4 pixel) is legal or not.
*  Input:
*  Output:
*  Return: 0: out of the legal mv range; 1: in the legal mv range
*  Attention:
*  Author: xiaozhen zheng, 20071009
******************************************************************************
*/
void DecideMvRange()
{
#ifdef AVS2_S2_BGLONGTERM
#if EXTEND_BD
  if ( input->profile_id == BASELINE10_PROFILE || input->profile_id == BASELINE_PROFILE )
#else
	if ( input->profile_id == 0x20 || input->profile_id == 0x40 || input->profile_id == 0x50 )
#endif
#else
#if EXTEND_BD
  if ( input->profile_id == 0x20 || input->profile_id == 0x40 || input->profile_id == 0x52 )
#else
	if ( input->profile_id == 0x20 || input->profile_id == 0x40 )
#endif
#endif
  {
    switch ( input->level_id )
    {
    case 0x10:
      Min_V_MV = -512;
      Max_V_MV =  511;
      Min_H_MV = -8192;
      Max_H_MV =  8191;
      break;
    case 0x20:
      Min_V_MV = -1024;
      Max_V_MV =  1023;
      Min_H_MV = -8192;
      Max_H_MV =  8191;
      break;
    case 0x22:
      Min_V_MV = -1024;
      Max_V_MV =  1023;
      Min_H_MV = -8192;
      Max_H_MV =  8191;
      break;
    case 0x40:     
      Min_V_MV = -2048;
      Max_V_MV =  2047;
      Min_H_MV = -8192;
      Max_H_MV =  8191;
      break;
    case 0x42:
      Min_V_MV = -2048;
      Max_V_MV =  2047;
      Min_H_MV = -8192;
      Max_H_MV =  8191;
      break;
    }
  }
}

/*
*************************************************************************
* Function: Parses the character array buf and writes global variable sliceregion,
which contains the information of sliceset's structure.
* Input: buf
buffer to be parsed
bufsize
buffer size of buffer
* Output:
* Return:
* Attention:
* Author: XZHENG
* Date: 2008.04
*************************************************************************
*/
void ParseSliceSet ( char *buf, int bufsize )
{
  char *items[MAX_ITEMS_TO_PARSE];
  int item     = 0;
  int InString = 0;
  int InItem   = 0;
  char *p      = buf;
  char *bufend = &buf[bufsize];
  int IntContent;
  int frmidx1 = -1;
  int frmidx2 = -1;
  int slice_flag;
  int regionnum = 0;
  int i;

  // Stage one: Generate an argc/argv-type list in items[], without comments and whitespace.
  // This is context insensitive and could be done most easily with lex(1).

  while ( p < bufend )
  {
    switch ( *p )
    {
    case 13:
      p++;
      break;
    case '#':                 // Found comment
      *p = '\0';              // Replace '#' with '\0' in case of comment immediately following integer or string

      while ( *p != '\n' && p < bufend ) // Skip till EOL or EOF, whichever comes first
      {
        p++;
      }

      InString = 0;
      InItem = 0;
      break;
    case '\n':
      InItem = 0;
      InString = 0;
      *p++ = '\0';
      break;
    case ' ':
    case '\t':              // Skip whitespace, leave state unchanged

      if ( InString )
      {
        p++;
      }
      else
      {
        // Terminate non-strings once whitespace is found
        *p++ = '\0';
        InItem = 0;
      }

      break;
    case '"':               // Begin/End of String
      *p++ = '\0';

      if ( !InString )
      {
        items[item++] = p;
        InItem = ~InItem;
      }
      else
      {
        InItem = 0;
      }

      InString = ~InString; // Toggle
      break;
    default:

      if ( !InItem )
      {
        items[item++] = p;
        InItem = ~InItem;
      }

      p++;
    }
  }

  item--;

  for ( i = 0; i < item; i += 3 )
  {
    if ( !strcmp ( "FRAMESTART", items[i] ) )
    {
      sscanf ( items[i + 2], "%d", &IntContent );
      frmidx1 = IntContent;
      i += 3;
      sscanf ( items[i + 2], "%d", &IntContent );
      frmidx2 = IntContent;
      i += 3;
    }

    if ( frmidx1 == -1 || frmidx2 == -1 )
    {
      snprintf ( errortext, ET_SIZE, " The frame range for the slice set should be specified!" );
      error ( errortext, 300 );
    }

    if ( frmidx1 > frmidx2 )
    {
      snprintf ( errortext, ET_SIZE, " The begining frame_num of slice set should not be larger than the end frame_num!" );
      error ( errortext, 300 );
    }

    if ( ( img->tr >= frmidx1 && img->tr <= frmidx2 ) || ( frmidx1 == 0 && frmidx2 == 0 ) )
    {
      slice_flag = 1;
    }
    else
    {
      slice_flag = 0;
    }

    sscanf ( items[i + 2], "%d", &IntContent );

    if ( !strcmp ( "SLICESETINDEX", items[i] ) && slice_flag )
    {
      sliceregion[regionnum].slice_set_index = IntContent;
    }
    else if ( !strcmp ( "SLICEDeltaQP", items[i] )  && slice_flag )
    {
      sliceregion[regionnum].sliceqp = img->type == B_IMG ? input->qpB + IntContent : ( img->type == INTRA_IMG ? input->qpI + IntContent : input->qpP + IntContent );
      sliceregion[regionnum].sliceqp = sliceregion[regionnum].sliceqp < 0 ? 0 : sliceregion[regionnum].sliceqp > 63 ? 63 : sliceregion[regionnum].sliceqp;
    }
    else if ( !strcmp ( "REGIONSTARTX", items[i] ) && slice_flag )
    {
      sliceregion[regionnum].pos[0] = IntContent;
    }
    else if ( !strcmp ( "REGIONENDX", items[i] ) && slice_flag )
    {
      sliceregion[regionnum].pos[1] = IntContent;
    }
    else if ( !strcmp ( "REGIONSTARTY", items[i] ) && slice_flag )
    {
      sliceregion[regionnum].pos[2] = IntContent;
    }
    else if ( !strcmp ( "REGIONENDY", items[i] ) && slice_flag )
    {
      sliceregion[regionnum++].pos[3] = IntContent;
    }
  }
}

/*
*************************************************************************
* Function: Parse the slice set config file
* Input: Null
* Output:
* Return:
* Attention:
* Author: XZHENG
* Date: 2008.04
*************************************************************************
*/
void SliceSetConfigure()
{
  char *content;

  content = GetConfigFileContent ( input->slice_set_config );
  ParseSliceSet ( content, strlen ( content ) );
  free ( content );
}
