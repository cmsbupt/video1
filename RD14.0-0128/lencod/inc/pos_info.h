//2013-1-13

#ifndef _POS_INFO_H_
#define _POS_INFO_H_

void ClosePosFile();
void OpenPosFile (char *inFilename);
int roi_parameters_extension(int frame_num,  Bitstream *bitstream);

#endif //_POS_INFO_H_