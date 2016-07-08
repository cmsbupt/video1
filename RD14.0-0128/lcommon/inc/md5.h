/*
*****************************************************************************
* COPYRIGHT AND WARRANTY INFORMATION
* Copyright 2015-2015
* Contributors: Yimin ZHOU, yiminzhou@uestc.edu.cn
* Institution:  University of Electronic Science and Technology of China
******************************************************************************
*/
#ifndef _MD5_H_
#define _MD5_H_

#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

#define RL(x, y) (((x) << (y)) | ((x) >> (32 - (y))))

#define PP(x) (x<<24)|((x<<8)&0xff0000)|((x>>8)&0xff00)|(x>>24)

#define FF(a, b, c, d, x, s, ac) a = b + (RL((a + F(b,c,d) + x + ac),s))
#define GG(a, b, c, d, x, s, ac) a = b + (RL((a + G(b,c,d) + x + ac),s))
#define HH(a, b, c, d, x, s, ac) a = b + (RL((a + H(b,c,d) + x + ac),s))
#define II(a, b, c, d, x, s, ac) a = b + (RL((a + I(b,c,d) + x + ac),s))

int BufferMD5(unsigned char *buffer, int len, unsigned int md5value[4]);

long long FileMD5(char *filename, unsigned int md5value[4]);

#endif
