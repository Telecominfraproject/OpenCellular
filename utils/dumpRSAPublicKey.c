/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* C port of DumpPublicKey.java from the Android Open source project with
 * support for additional RSA key sizes. (platform/system/core,git/libmincrypt
 * /tools/DumpPublicKey.java). Uses the OpenSSL X509 and BIGNUM library.
 */

#include <inttypes.h>
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <string.h>
#include <unistd.h>

/* Command line tool to extract RSA public keys from X.509 certificates
 * and output a pre-processed version of keys for use by RSA verification
 * routines.
 */

int check(RSA* key) {
  int public_exponent = BN_get_word(key->e);
  int modulus = BN_num_bits(key->n);

  if (public_exponent != 65537) {
    fprintf(stderr, "WARNING: Public exponent should be 65537 (but is %d).\n",
            public_exponent);
  }

  if (modulus != 1024 && modulus != 2048 && modulus != 4096
      && modulus != 8192) {
    fprintf(stderr, "ERROR: Unknown modulus length = %d.\n", modulus);
    return 0;
  }
  return 1;
}

/* Pre-processes and outputs RSA public key to standard out.
 */
void output(RSA* key) {
  int i, nwords;
  BIGNUM *N = key->n;
  BIGNUM *Big1, *Big2, *Big32, *BigMinus1;
  BIGNUM *B;
  BIGNUM *N0inv, *R, *RR, *RRTemp, *NnumBits;
  BIGNUM *n, *rr;
  BN_CTX *bn_ctx = BN_CTX_new();
  uint32_t n0invout;

  N = key->n;
  /* Output size of RSA key in 32-bit words */
  nwords = BN_num_bits(N) / 32;
  write(1, &nwords, sizeof(nwords));

  /* Initialize BIGNUMs */
  Big1 = BN_new();
  Big2 = BN_new();
  Big32 = BN_new();
  BigMinus1 = BN_new();
  N0inv= BN_new();
  R = BN_new();
  RR = BN_new();
  RRTemp = BN_new();
  NnumBits = BN_new();
  n = BN_new();
  rr = BN_new();


  BN_set_word(Big1, 1L);
  BN_set_word(Big2, 2L);
  BN_set_word(Big32, 32L);
  BN_sub(BigMinus1, Big1, Big2);

  B = BN_new();
  BN_exp(B, Big2, Big32, bn_ctx); /* B = 2^32 */

  /* Calculate and output N0inv = -1 / N[0] mod 2^32 */
  BN_mod_inverse(N0inv, N, B, bn_ctx);
  BN_sub(N0inv, B, N0inv);
  n0invout = BN_get_word(N0inv);
  write(1, &n0invout, sizeof(n0invout));

  /* Calculate R = 2^(# of key bits) */
  BN_set_word(NnumBits, BN_num_bits(N));
  BN_exp(R, Big2, NnumBits, bn_ctx);

  /* Calculate RR = R^2 mod N */
  BN_copy(RR, R);
  BN_mul(RRTemp, RR, R, bn_ctx);
  BN_mod(RR, RRTemp, N, bn_ctx);


  /* Write out modulus as little endian array of integers. */
  for (i = 0; i < nwords; ++i) {
    uint32_t nout;

    BN_mod(n, N, B, bn_ctx); /* n = N mod B */
    nout = BN_get_word(n);
    write(1, &nout, sizeof(nout));

    BN_rshift(N, N, 32); /*  N = N/B */
  }

  /* Write R^2 as little endian array of integers. */
  for (i = 0; i < nwords; ++i) {
    uint32_t rrout;

    BN_mod(rr, RR, B, bn_ctx); /* rr = RR mod B */
    rrout = BN_get_word(rr);
    write(1, &rrout, sizeof(rrout));

    BN_rshift(RR, RR, 32); /* RR = RR/B */
  }

  /* Free BIGNUMs. */
  BN_free(Big1);
  BN_free(Big2);
  BN_free(Big32);
  BN_free(BigMinus1);
  BN_free(N0inv);
  BN_free(R);
  BN_free(RRTemp);
  BN_free(NnumBits);
  BN_free(n);
  BN_free(rr);

}

int main(int argc, char* argv[]) {
  FILE* fp;
  X509* cert = NULL;
  RSA* pubkey = NULL;
  EVP_PKEY* key;

  if (argc != 2) {
    fprintf(stderr, "Usage: %s <certfile>\n", argv[0]);
    return -1;
  }

  fp = fopen(argv[1], "r");

  if (!fp) {
    fprintf(stderr, "Couldn't open certificate file!\n");
    return -1;
  }

  /* Read the certificate */
  if (!PEM_read_X509(fp, &cert, NULL, NULL)) {
    fprintf(stderr, "Couldn't read certificate.\n");
    goto fail;
  }

  /* Get the public key from the certificate. */
  key = X509_get_pubkey(cert);

  /* Convert to a RSA_style key. */
  if (!(pubkey = EVP_PKEY_get1_RSA(key))) {
    fprintf(stderr, "Couldn't convert to a RSA style key.\n");
    goto fail;
  }

  if (check(pubkey)) {
    output (pubkey);
  }

fail:
  X509_free(cert);
  RSA_free(pubkey);
  fclose(fp);

  return 0;
}
