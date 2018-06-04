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

#include "2sysincludes.h"

#include "2common.h"
#include "2rsa.h"
#include "2sha.h"
#include "host_common.h"
#include "host_signature2.h"
#include "vb2_common.h"

/* Invoke [external_signer] command with [pem_file] as an argument, contents of
 * [inbuf] passed redirected to stdin, and the stdout of the command is put
 * back into [outbuf].  Returns -1 on error, 0 on success.
 */
static int sign_external(uint32_t size,
			 const uint8_t *inbuf,
			 uint8_t *outbuf,
			 uint32_t outbufsize,
			 const char *pem_file,
			 const char *external_signer)
{
	int rv = 0, n;
	int p_to_c[2], c_to_p[2];  /* pipe descriptors */
	pid_t pid;

	VB2_DEBUG("Will invoke \"%s %s\" to perform signing.\n"
		 "Input to the signer will be provided on standard in.\n"
		 "Output of the signer will be read from standard out.\n",
		  external_signer, pem_file);

	/* Need two pipes since we want to invoke the external_signer as
	 * a co-process writing to its stdin and reading from its stdout. */
	if (pipe(p_to_c) < 0 || pipe(c_to_p) < 0) {
		VB2_DEBUG("pipe() error\n");
		return -1;
	}
	if ((pid = fork()) < 0) {
		VB2_DEBUG("fork() error\n");
		return -1;
	} else if (pid > 0) {  /* Parent. */
		close(p_to_c[STDIN_FILENO]);
		close(c_to_p[STDOUT_FILENO]);

		/* We provide input to the child process (external signer). */
		if (write(p_to_c[STDOUT_FILENO], inbuf, size) != size) {
			VB2_DEBUG("write() error\n");
			rv = -1;
		} else {
			/* Send EOF to child (signer process). */
			close(p_to_c[STDOUT_FILENO]);

			do {
				n = read(c_to_p[STDIN_FILENO], outbuf,
					 outbufsize);
				outbuf += n;
				outbufsize -= n;
			} while (n > 0 && outbufsize);

			if (n < 0) {
				VB2_DEBUG("read() error\n");
				rv = -1;
			}
		}
		if (waitpid(pid, NULL, 0) < 0) {
			VB2_DEBUG("waitpid() error\n");
			rv = -1;
		}
	} else {  /* Child. */
		close (p_to_c[STDOUT_FILENO]);
		close (c_to_p[STDIN_FILENO]);
		/* Map the stdin to the first pipe (this pipe gets input
		 * from the parent) */
		if (STDIN_FILENO != p_to_c[STDIN_FILENO]) {
			if (dup2(p_to_c[STDIN_FILENO], STDIN_FILENO) !=
			    STDIN_FILENO) {
				VB2_DEBUG("stdin dup2() failed\n");
				close(p_to_c[0]);
				return -1;
			}
		}
		/* Map the stdout to the second pipe (this pipe sends back
		 * signer output to the parent) */
		if (STDOUT_FILENO != c_to_p[STDOUT_FILENO]) {
			if (dup2(c_to_p[STDOUT_FILENO], STDOUT_FILENO) !=
			    STDOUT_FILENO) {
				VB2_DEBUG("stdout dup2() failed\n");
				close(c_to_p[STDOUT_FILENO]);
				return -1;
			}
		}
		/* External signer is invoked here. */
		if (execl(external_signer, external_signer, pem_file,
			  (char *) 0) < 0) {
			VB2_DEBUG("execl() of external signer failed\n");
		}
	}
	return rv;
}

struct vb2_signature *vb2_external_signature(const uint8_t *data,
					     uint32_t size,
					     const char *key_file,
					     uint32_t key_algorithm,
					     const char *external_signer)
{
	int vb2_alg = vb2_crypto_to_hash(key_algorithm);
	uint8_t digest[VB2_MAX_DIGEST_SIZE];
	int digest_size = vb2_digest_size(vb2_alg);

	uint32_t digest_info_size = 0;
	const uint8_t *digest_info = NULL;
	if (VB2_SUCCESS != vb2_digest_info(vb2_alg,
					   &digest_info, &digest_info_size))
		return NULL;


	uint8_t *signature_digest;
	uint64_t signature_digest_len = digest_size + digest_info_size;

	int rv;

	/* Calculate the digest */
	if (VB2_SUCCESS != vb2_digest_buffer(data, size, vb2_alg,
					     digest, sizeof(digest)))
		return NULL;

	/* Prepend the digest info to the digest */
	signature_digest = calloc(signature_digest_len, 1);
	if (!signature_digest)
		return NULL;

	memcpy(signature_digest, digest_info, digest_info_size);
	memcpy(signature_digest + digest_info_size, digest, digest_size);

	/* Allocate output signature */
	uint32_t sig_size =
		vb2_rsa_sig_size(vb2_crypto_to_signature(key_algorithm));
	struct vb2_signature *sig = vb2_alloc_signature(sig_size, size);
	if (!sig) {
		free(signature_digest);
		return NULL;
	}

	/* Sign the signature_digest into our output buffer */
	rv = sign_external(signature_digest_len,    /* Input length */
			   signature_digest,        /* Input data */
			   vb2_signature_data(sig), /* Output sig */
			   sig_size,                /* Max Output sig size */
			   key_file,                /* Key file to use */
			   external_signer);        /* External cmd to invoke */
	free(signature_digest);

	if (-1 == rv) {
		VB2_DEBUG("RSA_private_encrypt() failed.\n");
		free(sig);
		return NULL;
	}

	/* Return the signature */
	return sig;
}
