
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sha.h"

/* update the SHA digest */

void sha_update(SHA_INFO *sha_info, BYTE *buffer, int count)
{
    if ((sha_info->count_lo + ((LONG) count << 3)) < sha_info->count_lo) {
	++sha_info->count_hi;
    }
    sha_info->count_lo += (LONG) count << 3;
    sha_info->count_hi += (LONG) count >> 29;
    while (count >= SHA_BLOCKSIZE) {
	memcpy(sha_info->data, buffer, SHA_BLOCKSIZE);
#ifdef LITTLE_ENDIAN
	byte_reverse(sha_info->data, SHA_BLOCKSIZE);
#endif /* LITTLE_ENDIAN */
	sha_transform(sha_info);
	buffer += SHA_BLOCKSIZE;
	count -= SHA_BLOCKSIZE;
    }
    memcpy(sha_info->data, buffer, count);
}

