/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host functions for signature generation.
 */

/* TODO: change all 'return 0', 'return 1' into meaningful return codes */

#include <openssl/rsa.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "cryptolib.h"
#include "file_keys.h"
#include "host_common.h"
#include "vboot_common.h"


VbSignature* SignatureAlloc(uint64_t sig_size, uint64_t data_size) {
  VbSignature* sig = (VbSignature*)malloc(sizeof(VbSignature) + sig_size);
  if (!sig)
    return NULL;

  sig->sig_offset = sizeof(VbSignature);
  sig->sig_size = sig_size;
  sig->data_size = data_size;
  return sig;
}


void SignatureInit(VbSignature* sig, uint8_t* sig_data,
                   uint64_t sig_size, uint64_t data_size) {
  sig->sig_offset = OffsetOf(sig, sig_data);
  sig->sig_size = sig_size;
  sig->data_size = data_size;
}


int SignatureCopy(VbSignature* dest, const VbSignature* src) {
  if (dest->sig_size < src->sig_size)
    return 1;
  dest->sig_size = src->sig_size;
  dest->data_size = src->data_size;
  Memcpy(GetSignatureData(dest), GetSignatureDataC(src), src->sig_size);
  return 0;
}


VbSignature* CalculateChecksum(const uint8_t* data, uint64_t size) {

  uint8_t* header_checksum;
  VbSignature* sig;

  header_checksum = DigestBuf(data, size, SHA512_DIGEST_ALGORITHM);
  if (!header_checksum)
    return NULL;

  sig = SignatureAlloc(SHA512_DIGEST_SIZE, 0);
  if (!sig) {
    VbExFree(header_checksum);
    return NULL;
  }
  sig->sig_offset = sizeof(VbSignature);
  sig->sig_size = SHA512_DIGEST_SIZE;
  sig->data_size = size;

  /* Signature data immediately follows the header */
  Memcpy(GetSignatureData(sig), header_checksum, SHA512_DIGEST_SIZE);
  VbExFree(header_checksum);
  return sig;
}

VbSignature* CalculateHash(const uint8_t* data, uint64_t size,
                           const VbPrivateKey* key) {
  uint8_t* digest = NULL;
  int digest_size = hash_size_map[key->algorithm];
  VbSignature* sig = NULL;

  /* Calculate the digest */
  digest = DigestBuf(data, size, key->algorithm);
  if (!digest)
    return NULL;

  /* Allocate output signature */
  sig = SignatureAlloc(digest_size, size);
  if (!sig) {
    free(digest);
    return NULL;
  }

  /* The digest itself is the signature data */
  Memcpy(GetSignatureData(sig), digest, digest_size);
  free(digest);

  /* Return the signature */
  return sig;
}

VbSignature* CalculateSignature(const uint8_t* data, uint64_t size,
                                const VbPrivateKey* key) {

  uint8_t* digest;
  int digest_size = hash_size_map[key->algorithm];

  const uint8_t* digestinfo = hash_digestinfo_map[key->algorithm];
  int digestinfo_size = digestinfo_size_map[key->algorithm];

  uint8_t* signature_digest;
  int signature_digest_len = digest_size + digestinfo_size;

  VbSignature* sig;
  int rv;

  /* Calculate the digest */
  /* TODO: rename param 3 of DigestBuf to hash_type */
  digest = DigestBuf(data, size, hash_type_map[key->algorithm]);
  if (!digest)
    return NULL;

  /* Prepend the digest info to the digest */
  signature_digest = malloc(signature_digest_len);
  if (!signature_digest) {
    VbExFree(digest);
    return NULL;
  }
  Memcpy(signature_digest, digestinfo, digestinfo_size);
  Memcpy(signature_digest + digestinfo_size, digest, digest_size);
  VbExFree(digest);

  /* Allocate output signature */
  sig = SignatureAlloc(siglen_map[key->algorithm], size);
  if (!sig) {
    free(signature_digest);
    return NULL;
  }

  /* Sign the signature_digest into our output buffer */
  rv = RSA_private_encrypt(signature_digest_len,   /* Input length */
                           signature_digest,       /* Input data */
                           GetSignatureData(sig),  /* Output sig */
                           key->rsa_private_key,   /* Key to use */
                           RSA_PKCS1_PADDING);     /* Padding to use */
  free(signature_digest);

  if (-1 == rv) {
    VBDEBUG(("SignatureBuf(): RSA_private_encrypt() failed.\n"));
    free(sig);
    return NULL;
  }

  /* Return the signature */
  return sig;
}

/* Invoke [external_signer] command with [pem_file] as
 * an argument, contents of [inbuf] passed redirected to stdin,
 * and the stdout of the command is put back into [outbuf].
 * Returns -1 on error, 0 on success.
 */
int InvokeExternalSigner(uint64_t size,
                         const uint8_t* inbuf,
                         uint8_t* outbuf,
                         uint64_t outbufsize,
                         const char* pem_file,
                         const char* external_signer) {

  int rv = 0, n;
  int p_to_c[2], c_to_p[2];  /* pipe descriptors */
  pid_t pid;

  VBDEBUG(("Will invoke \"%s %s\" to perform signing.\n"
           "Input to the signer will be provided on standard in.\n"
           "Output of the signer will be read from standard out.\n",
           external_signer, pem_file));

  /* Need two pipes since we want to invoke the external_signer as
   * a co-process writing to its stdin and reading from its stdout. */
  if (pipe(p_to_c) < 0 || pipe(c_to_p) < 0) {
    VBDEBUG(("pipe() error\n"));
    return -1;
  }
  if ((pid = fork()) < 0) {
    VBDEBUG(("fork() error"));
    return -1;
  }
  else if (pid > 0) {  /* Parent. */
    close(p_to_c[STDIN_FILENO]);
    close(c_to_p[STDOUT_FILENO]);

    /* We provide input to the child process (external signer). */
    if (write(p_to_c[STDOUT_FILENO], inbuf, size) != size) {
      VBDEBUG(("write() error while providing input to external signer\n"));
      rv = -1;
    } else {
      close(p_to_c[STDOUT_FILENO]);  /* Send EOF to child (signer process). */
      do {
        n = read(c_to_p[STDIN_FILENO], outbuf, outbufsize);
        outbuf += n;
        outbufsize -= n;
      } while (n > 0 && outbufsize);

      if (n < 0) {
        VBDEBUG(("read() error while reading output from external signer\n"));
        rv = -1;
      }
    }
    if (waitpid(pid, NULL, 0) < 0) {
      VBDEBUG(("waitpid() error\n"));
      rv = -1;
    }
  } else {  /* Child. */
    close (p_to_c[STDOUT_FILENO]);
    close (c_to_p[STDIN_FILENO]);
    /* Map the stdin to the first pipe (this pipe gets input
     * from the parent) */
    if (STDIN_FILENO != p_to_c[STDIN_FILENO]) {
      if (dup2(p_to_c[STDIN_FILENO], STDIN_FILENO) != STDIN_FILENO) {
        VBDEBUG(("stdin dup2() failed (external signer)\n"));
        close(p_to_c[0]);
        return -1;
      }
    }
    /* Map the stdout to the second pipe (this pipe sends back
     * signer output to the parent) */
    if (STDOUT_FILENO != c_to_p[STDOUT_FILENO]) {
      if (dup2(c_to_p[STDOUT_FILENO], STDOUT_FILENO) != STDOUT_FILENO) {
        VBDEBUG(("stdout dup2() failed (external signer)\n"));
        close(c_to_p[STDOUT_FILENO]);
        return -1;
      }
    }
    /* External signer is invoked here. */
    if (execl(external_signer, external_signer, pem_file, (char *) 0) < 0) {
      VBDEBUG(("execl() of external signer failed\n"));
    }
  }
  return rv;
}

/* TODO(gauravsh): This could easily be integrated into CalculateSignature()
 * since the code is almost a mirror - I have kept it as such to avoid changing
 * the existing interface. */
VbSignature* CalculateSignature_external(const uint8_t* data, uint64_t size,
                                         const char* key_file,
                                         uint64_t key_algorithm,
                                         const char* external_signer) {
  uint8_t* digest;
  uint64_t digest_size = hash_size_map[key_algorithm];

  const uint8_t* digestinfo = hash_digestinfo_map[key_algorithm];
  uint64_t digestinfo_size = digestinfo_size_map[key_algorithm];

  uint8_t* signature_digest;
  uint64_t signature_digest_len = digest_size + digestinfo_size;

  VbSignature* sig;
  int rv;

  /* Calculate the digest */
  /* TODO: rename param 3 of DigestBuf to hash_type */
  digest = DigestBuf(data, size, hash_type_map[key_algorithm]);
  if (!digest)
    return NULL;

  /* Prepend the digest info to the digest */
  signature_digest = malloc(signature_digest_len);
  if (!signature_digest) {
    free(digest);
    return NULL;
  }
  Memcpy(signature_digest, digestinfo, digestinfo_size);
  Memcpy(signature_digest + digestinfo_size, digest, digest_size);
  free(digest);

  /* Allocate output signature */
  sig = SignatureAlloc(siglen_map[key_algorithm], size);
  if (!sig) {
    free(signature_digest);
    return NULL;
  }

  /* Sign the signature_digest into our output buffer */
  rv = InvokeExternalSigner(signature_digest_len, /* Input length */
                            signature_digest,     /* Input data */
                            GetSignatureData(sig), /* Output sig */
                            siglen_map[key_algorithm], /* Max Output sig size */
                            key_file,             /* Key file to use */
                            external_signer);     /* External cmd to invoke */
  free(signature_digest);

  if (-1 == rv) {
    VBDEBUG(("SignatureBuf(): RSA_private_encrypt() failed.\n"));
    free(sig);
    return NULL;
  }

  /* Return the signature */
  return sig;
}
