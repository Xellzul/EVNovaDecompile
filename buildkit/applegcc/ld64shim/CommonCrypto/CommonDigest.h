/* The LC_UUID MD5 through OpenSSL. */
#ifndef SHIM_COMMONDIGEST_H
#define SHIM_COMMONDIGEST_H
#include <openssl/md5.h>
#define CC_MD5_CTX MD5_CTX
#define CC_MD5_Init MD5_Init
#define CC_MD5_Update MD5_Update
#define CC_MD5_Final MD5_Final
#define CC_MD5_DIGEST_LENGTH MD5_DIGEST_LENGTH
#endif
