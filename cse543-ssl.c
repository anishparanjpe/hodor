#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/hmac.h>
#include <sys/types.h>
#include <assert.h>
#include <unistd.h>
#include "cse543-ssl.h"


// OpenSSL example code from:
// https://wiki.openssl.org/index.php/EVP_Authenticated_Encryption_and_Decryption

int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *aad,
	    int aad_len, unsigned char *key, unsigned char *iv,
	    unsigned char *ciphertext, unsigned char *tag)
{
  EVP_CIPHER_CTX *ctx;

  int len;

  int ciphertext_len;


  /* Create and initialise the context */
  if(!(ctx = EVP_CIPHER_CTX_new())) handleErrors();

  /* Initialise the encryption operation. */
  if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL))
    handleErrors();

  /* Set IV length if default 12 bytes (96 bits) is not appropriate */
  if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 16, NULL))
    handleErrors();

  /* Initialise key and IV */
  if(1 != EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv)) handleErrors();

  /* Provide any AAD data. This can be called zero or more times as
   * required
   */
  /*
    if(1 != EVP_EncryptUpdate(ctx, NULL, &len, aad, aad_len))
    handleErrors();
  */

  /* Provide the message to be encrypted, and obtain the encrypted output.
   * EVP_EncryptUpdate can be called multiple times if necessary
   */
  if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
    handleErrors();
  ciphertext_len = len;

  /* Finalise the encryption. Normally ciphertext bytes may be written at
   * this stage, but this does not occur in GCM mode
   */
  if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) handleErrors();
  ciphertext_len += len;

  /* Get the tag */
  if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag))
    handleErrors();

  /* Clean up */
  EVP_CIPHER_CTX_free(ctx);

  return ciphertext_len;
}


// OpenSSL example code from:
// https://wiki.openssl.org/index.php/EVP_Authenticated_Encryption_and_Decryption

int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *aad,
	    int aad_len, unsigned char *tag, unsigned char *key, unsigned char *iv,
	    unsigned char *plaintext)
{
  EVP_CIPHER_CTX *ctx;
  int len;
  int plaintext_len;
  int ret;

  /* Create and initialise the context */
  if(!(ctx = EVP_CIPHER_CTX_new())) handleErrors();

  /* Initialise the decryption operation. */
  if(!EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL))
    handleErrors();

  /* Set IV length. Not necessary if this is 12 bytes (96 bits) */
  if(!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 16, NULL))
    handleErrors();

  /* Initialise key and IV */
  if(!EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv)) handleErrors();

  /* Provide any AAD data. This can be called zero or more times as
   * required
   */
  /*
  if(!EVP_DecryptUpdate(ctx, NULL, &len, aad, aad_len))
    handleErrors();
  */

  /* Provide the message to be decrypted, and obtain the plaintext output.
   * EVP_DecryptUpdate can be called multiple times if necessary
   */
  if(!EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
    handleErrors();
  plaintext_len = len;

  /* Set expected tag value. Works in OpenSSL 1.0.1d and later */
  if(!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag))
    handleErrors();

  /* Finalise the decryption. A positive return value indicates success,
   * anything else is a failure - the plaintext is not trustworthy.
   */
  ret = EVP_DecryptFinal_ex(ctx, plaintext + len, &len);

  /* Clean up */
  EVP_CIPHER_CTX_free(ctx);

  if(ret > 0)
    {
      /* Success */
      plaintext_len += len;
      return plaintext_len;
    }
  else
    {
      /* Verify failed */
      return -1;
    }
}

// OpenSSL example code from:
// https://wiki.openssl.org/index.php/EVP_Message_Digests

void digest_message(const unsigned char *message, size_t message_len, unsigned char **digest, unsigned int *digest_len)
{
  EVP_MD_CTX *mdctx;

  if((mdctx = EVP_MD_CTX_create()) == NULL)
    handleErrors();

  if(1 != EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL))
    handleErrors();

  if(1 != EVP_DigestUpdate(mdctx, message, message_len))
    handleErrors();

  if((*digest = (unsigned char *)OPENSSL_malloc(EVP_MD_size(EVP_sha256()))) == NULL)
    handleErrors();

  if(1 != EVP_DigestFinal_ex(mdctx, *digest, digest_len))
    handleErrors();

  EVP_MD_CTX_destroy(mdctx);
}



// OpenSSL example code derived from:
// https://stackoverflow.com/questions/12545811/using-hmac-vs-evp-functions-in-openssl

int hmac_message(unsigned char* msg, size_t mlen, unsigned char** val, size_t* vlen, unsigned char *key)
{
  HMAC_CTX ctx;
  const EVP_MD* md = NULL;

  OpenSSL_add_all_digests();

  md = EVP_get_digestbyname("SHA256");
  HMAC_CTX_init( &ctx );

  if(!HMAC_Init_ex(&ctx, key, sizeof(key), md, NULL))
    handleErrors();

  if(!HMAC_Update(&ctx, msg, mlen))
    handleErrors();

  if(!HMAC_Final(&ctx, *val, (unsigned int *)vlen))
    handleErrors();

  HMAC_CTX_cleanup(&ctx);

#if 0
  unsigned int i;

  printf("HMAC is: ");
  for(i = 0; i < *vlen; i++)
    printf("%02x", (*val)[i]);
  printf("\n");
#endif

  return 0;
}


void handleErrors(void)
{
  ERR_print_errors_fp(stderr);
  abort();
}
