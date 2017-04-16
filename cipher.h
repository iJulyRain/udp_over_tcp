/*
 * =====================================================================================
 *
 *       Filename:  cipher.h
 *
 *    Description:  cipher header
 *
 *        Version:  1.0
 *        Created:  11/06/2016 01:14:47 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  lizhixian (group3), lizhixian@integritytech.com.cn
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef __CIPHER_H__
#define __CIPHER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define rs_decrypt rs_encrypt

void rs_encrypt(const unsigned char *in, unsigned char *out, size_t len, const char *key);
void rs_md5(uint8_t *initial_msg, size_t initial_len, uint32_t *hash);

#endif
