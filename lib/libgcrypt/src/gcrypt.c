/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
 
/*
 * I am not gcrypt! I contain a subset of gcrypt symbols implemented using nss
 * in order to allow libotr to work without gcrypt. Ideally libotr would have a
 * pluggable cryptography library, but since this is not the case, a shim (me!)
 * was created to bridge the gap.
 */
 
/*
 * The libraries being reimplemented here.
 */
#include "gcrypt.h"
 
/*
 * The headers used to reimplement the above.
 */
#include <mozilla/mozalloc.h>
#include "gpg-error.h"
#include <assert.h>
#include <pk11pub.h>
//#include "../../../mozilla/security/nss/lib/freebl/mpi/mpi.h"
#include "mpi.h"
// FIXME: priv!?
//#include "../../../mozilla/security/nss/lib/freebl/mpi/mpi-priv.h"
#include "mpi-priv.h"
//#include "../../../mozilla/security/nss/lib/freebl/blapi.h"
#include <blapi.h>

// FIXME: #include "../../../mozilla/security/nss/lib/freebl/secmpi.h"
// Modified to find mpi.h and not `goto`.
#include "secmpi.h"
 
/* Register a custom memory allocation functions. */
void gcry_set_allocation_handler(gcry_handler_alloc_t func_alloc,
                                 gcry_handler_alloc_t func_alloc_secure,
                                 gcry_handler_secure_check_t func_secure_check,
                                 gcry_handler_realloc_t func_realloc,
                                 gcry_handler_free_t func_free) {
  // This is a no-op since we just directly override the functions below.
}
 
/* Libgcrypt uses its own memory allocation.  It is important to use
   gcry_free () to release memory allocated by libgcrypt. */
// TODO Should this use the libotr ones / secure this.
// TODO Add memory reporting.
void *gcry_malloc_secure(size_t n) {
  return malloc(n);
}
void  gcry_free(void *a) {
  return free(a);
}
 
/* Check that the library fulfills the version requirement.  */
const char *gcry_check_version(const char *req_version) {
  // This is a no-op as used in libotr.
  // TODO Initialization stuff.
 
  return GCRYPT_VERSION;
}
 
 
/*
 * Randomize functions.
 */
 
/* Fill BUFFER with LENGTH bytes of random, using random numbers of
   quality LEVEL. */
void gcry_randomize(void *buffer, size_t length, enum gcry_random_level level) {
  assert(level == GCRY_STRONG_RANDOM);
  SECStatus status = SECSuccess;
  status = PK11_GenerateRandom((unsigned char*)buffer, length);
  assert(status == SECSuccess);
}
 
/* Return NBYTES of allocated random using a random numbers of quality
   LEVEL. */
void *gcry_random_bytes(size_t nbytes, enum gcry_random_level level) {
  assert(level == GCRY_STRONG_RANDOM);
  void *data = gcry_malloc_secure(nbytes);
  assert(data != NULL);
  gcry_randomize(data, nbytes, level);
  return data;
}
 
/* Return NBYTES of allocated random using a random numbers of quality
   LEVEL.  The random numbers are created returned in "secure"
   memory. */
void *gcry_random_bytes_secure(size_t nbytes, enum gcry_random_level level) {
  assert(level == GCRY_STRONG_RANDOM);
  return gcry_random_bytes(nbytes, level);
}
 
 
// libgpg-error returns platform specific error codes.
// Since this has nothing to do with bundling two crypto libraries,
// we should just compile and ship it.
//gcry_error_t
//gcry_error
//gcry_error_from_errno
 
 
// Internally, let's just define it like this and then, at the edges, we'll
// parse and serialize appropriately.
#define gcry_mpi mp_int;
 
// mp_size is an unsigned int.
gcry_mpi_t gcry_mpi_new(unsigned int nbits) {
  mp_int *mp = NULL;
  mp_err err = 0;
  err = mp_init_size(mp, MP_HOWMANY(nbits, MP_DIGIT_BIT));
  assert(err == MP_OKAY);
  return (gcry_mpi_t)mp;
};
 
// Verify what's different about the secure alloc?
#define gcry_mpi_snew gcry_mpi_new;
 
// These comparison functions surprisingly use the same conventions.
int gcry_mpi_cmp(const gcry_mpi_t u, const gcry_mpi_t v) {
  return mp_cmp((const mp_int *)u, (const mp_int *)v);
};
 
// mp_digit is an unsigned long. These comparison functions surprisingly use
// the same conventions.
int gcry_mpi_cmp_ui(const gcry_mpi_t u, unsigned long v) {
  return mp_cmp_d((const mp_int *)u, (mp_digit)v);
};
 
gcry_mpi_t gcry_mpi_copy(const gcry_mpi_t a) {
  mp_int *mp = NULL;
  mp_err err = 0;
  err = mp_init_copy(mp, (const mp_int *)a);
  assert(!err);
  return (gcry_mpi_t)mp;
};
 
gcry_mpi_t gcry_mpi_set(gcry_mpi_t w, const gcry_mpi_t u) {
  mp_err err = 0;
  err = mp_copy((const mp_int *)u, (mp_int *)w);
  assert(!err);
  return w;
};
 
// mp_digit is an unsigned long.
gcry_mpi_t gcry_mpi_set_ui(gcry_mpi_t w, unsigned long u) {
  mp_set((mp_int *)w, (mp_digit)u);
  return w;
};
 
// The signatures are a little crossed up here but it's basically the same
// thing.
void gcry_mpi_subm(gcry_mpi_t w, gcry_mpi_t u, gcry_mpi_t v, gcry_mpi_t m) {
  mp_err err = 0;
  err = mp_submod((const mp_int *)u, (const mp_int *)v, (const mp_int *)m,
                  (mp_int *)w);
  assert(!err);
};
 
// mp_digit is an unsigned long. The signatures are a little crossed up here
// but it's basically the same thing.
void gcry_mpi_sub_ui(gcry_mpi_t w, gcry_mpi_t u, unsigned long v) {
  mp_err err = 0;
  err = mp_sub_d((const mp_int *)u, (mp_digit)v, (mp_int *)w);
  assert(!err);
};
 
// The signatures are a little crossed up here but it's basically the same
// thing.
void gcry_mpi_mulm(gcry_mpi_t w, gcry_mpi_t u, gcry_mpi_t v, gcry_mpi_t m) {
  mp_err err = 0;
  err = mp_mulmod((const mp_int *)u, (const mp_int *)v, (const mp_int *)m,
                  (mp_int *)w);
  assert(!err);
};
 
// The return value is ignored in the libotr uses.
int gcry_mpi_invm(gcry_mpi_t x, gcry_mpi_t a, gcry_mpi_t m) {
  mp_err err = 0;
  err = mp_invmod((const mp_int *)a, (const mp_int *)m, (mp_int *)x);
  return (err == MP_UNDEF) ? 0 : 1;
};
 
// The signatures are a little crossed up here but it's basically the same
// thing.
void gcry_mpi_powm(gcry_mpi_t w, const gcry_mpi_t b, const gcry_mpi_t e,
                   const gcry_mpi_t m) {
  mp_err err = 0;
  // The comments say it uses Barrett's algorithm ... does it do
  // Montgomery reduction? It'll probably be fast enough as is.
  err = mp_exptmod((const mp_int *)b, (const mp_int *)e, (const mp_int *)m,
                   (mp_int *)w);
  assert(!err);
};
 
// libotr ignores the return value from this function.
gcry_error_t gcry_mpi_print(enum gcry_mpi_format format, unsigned char *buffer,
                            size_t buflen, size_t *nwritten,
                            const gcry_mpi_t a) {
  assert(format == GCRYMPI_FMT_USG);
  mp_err err = 0;
  // libotr uses this function without a buffer (buflen == 0) to determine
  // how much space to allocate based on nwritten.
  if (buflen > 0) {
    err = mp_to_unsigned_octets((const mp_int *)a, buffer, (mp_size)buflen);
  }
  // `mp_unsigned_octet_size` returns an int.
  if (!err && (nwritten != NULL))
    *nwritten = (size_t)mp_unsigned_octet_size((const mp_int *)a);
  return gpg_error(err ? GPG_ERR_GENERAL : GPG_ERR_NO_ERROR);
};
 
// libotr ignores the return value from this function. This is the same as
// `gcry_mpi_print` but with allocation.
gcry_error_t gcry_mpi_aprint(enum gcry_mpi_format format,
                             unsigned char **buffer, size_t *nwritten,
                             const gcry_mpi_t a) {
  size_t buflen = (size_t)mp_unsigned_octet_size((const mp_int *)a);
  *buffer = malloc(*nwritten);
  return gcry_mpi_print(format, *buffer, buflen, nwritten, a);
};
 
// libotr ignores the return value from this function.
gcry_error_t gcry_mpi_scan(gcry_mpi_t *ret_mpi, enum gcry_mpi_format format,
                           const void *buffer, size_t buflen,
                           size_t *nscanned) {
  mp_err err = 0;
  err = mp_init((mp_int *)*ret_mpi);
  if (err)
    return gpg_error(GPG_ERR_ENOMEM);
  err = mp_read_unsigned_octets((mp_int *)*ret_mpi,
                                (const unsigned char *)buffer, (mp_size)buflen);
  // This should be ok because libotr serializes mpis with no leading zeroes.
  if (!err && nscanned != NULL)
    *nscanned = (size_t)mp_unsigned_octet_size((const mp_int *)*ret_mpi);
  return gpg_error(err ? GPG_ERR_GENERAL : GPG_ERR_NO_ERROR);
};
 
// No-op. Just used in debugging.
void gcry_mpi_dump(const gcry_mpi_t a) {};
 
void gcry_mpi_release(gcry_mpi_t a) {
  return mp_clear((mp_int *)a);
};
 
 
// `gcry_cipher_hd_t` is defined in terms of this.
struct gcry_cipher_handle {
  void *key;
  size_t keylen;
  void *ctr;
  size_t ctrlen;
  AESContext *cx;
};
 
// Always used with the same settings, AES in CTR Mode. Let's assert that
// and then just move forward. No need to generalize anything.
gcry_error_t gcry_cipher_open(gcry_cipher_hd_t *hd, int algo, int mode,
                              unsigned int flags) {
  assert(hd);
  assert(algo == GCRY_CIPHER_AES);
  assert(mode == GCRY_CIPHER_MODE_CTR);
  assert(flags == GCRY_CIPHER_SECURE);
  gcry_cipher_hd_t handle = NULL;
  handle = malloc(sizeof(*handle));
  if (!handle)
    return gpg_error(GPG_ERR_ENOMEM);
  handle->key = NULL;
  handle->keylen = 0;
  handle->ctr = NULL;
  handle->ctrlen = 0;
  handle->cx = AES_AllocateContext();
  *hd = handle;
  return gpg_error(GPG_ERR_NO_ERROR);
};
 
// This always follows a `gcry_cipher_open`.
gcry_error_t gcry_cipher_setkey(gcry_cipher_hd_t hd, const void *key,
                                size_t keylen) {
  hd->key = key;  // memcpy?
  hd->keylen = keylen;
  return gpg_error(GPG_ERR_NO_ERROR);
};
 
// This always follows a `gcry_cipher_open` or `gcry_cipher_reset`.
gpg_error_t gcry_cipher_setctr(gcry_cipher_hd_t hd, const void *ctr,
                               size_t ctrlen) {
  hd->ctr = ctr;  // memcpy?
  hd->ctrlen = ctrlen;
  return gpg_error(GPG_ERR_NO_ERROR);
};
 
gcry_error_t gcry_cipher_encrypt(gcry_cipher_hd_t hd, void *out, size_t outsize,
                                 const void *in, size_t inlen) {
  SECStatus status = SECSuccess;
  unsigned int outputLen = 0;
 
  assert(hd->ctrlen == 16);  // iv is the block size
 
  status = AES_InitContext(
    hd->cx,
    (const unsigned char *)hd->key,
    (unsigned int)hd->keylen,
    (const unsigned char *)hd->ctr,  // iv
    NSS_AES_CTR,
    PR_TRUE,
    16  // Rijndael w/ 128-bit block size for AES
  );
 
  if (status != SECSuccess)
    return gpg_error(GPG_ERR_GENERAL);
 
  status = AES_Encrypt(
    hd->cx,
    (unsigned char *)out,
    &outputLen,  // does this accept NULL? we're throwing it away.
    (unsigned int)outsize,
    (const unsigned char *)in,
    (unsigned int)inlen
  );
 
  return gpg_error((status == SECSuccess) ?
    GPG_ERR_NO_ERROR : GPG_ERR_GENERAL);
};
 
gcry_error_t gcry_cipher_decrypt(gcry_cipher_hd_t hd, void *out, size_t outsize,
                                 const void *in, size_t inlen) {
  SECStatus status = SECSuccess;
  unsigned int outputLen = 0;
 
  assert(hd->ctrlen == 16);  // iv is the block size
 
  status = AES_InitContext(
    hd->cx,
    (const unsigned char *)hd->key,
    (unsigned int)hd->keylen,
    (const unsigned char *)hd->ctr,  // iv
    NSS_AES_CTR,
    PR_FALSE,  // CTR mode seems to override this to PR_TRUE.
    16  // Rijndael w/ 128-bit block size for AES
  );
 
  if (status != SECSuccess)
    return gpg_error(GPG_ERR_GENERAL);
 
  status = AES_Decrypt(
    hd->cx,
    (unsigned char *)out,
    &outputLen,  // does this accept NULL? we're throwing it away.
    (unsigned int)outsize,
    (const unsigned char *)in,
    (unsigned int)inlen
  );
 
  return gpg_error((status == SECSuccess) ?
    GPG_ERR_NO_ERROR : GPG_ERR_GENERAL);
};
 
// This is defined in terms of `gcry_cipher_ctl` in gcrypt.h
// We should declare it differently since we don't otherwise need
// `gcry_cipher_ctl`.
gcry_error_t gcry_cipher_reset(gcry_cipher_hd_t hd) {
  // Leave the key alone.
  hd->ctr = NULL;
  hd->ctrlen = 0;
  if (hd->cx)
    AES_DestroyContext(hd->cx, PR_FALSE);  // But don't free.
  return gpg_error(GPG_ERR_NO_ERROR);
};
 
void gcry_cipher_close(gcry_cipher_hd_t hd) {
  if (!hd)
    return;
  if (hd->cx)
    AES_DestroyContext(hd->cx, PR_TRUE);  // And free it.
  return free(hd);
};
 
 
// We need to comment this out from gcrypt.h and just use our definition.
struct gcry_md_handle {
  HASH_HashType algo;
  HMACContext *cx;
  unsigned char digest[256];  // The larger of SHA1 and SHA256.
};
 
// Always used with pretty much same settings, HMAC with SHA1 or SHA256. Let's
// assert that and then just move forward. No need to generalize anything.
gcry_error_t gcry_md_open(gcry_md_hd_t *hd, int algo, unsigned int flags) {
  assert(flags == GCRY_MD_FLAG_HMAC);
  gcry_md_hd_t handle = NULL;
  handle = malloc(sizeof(*handle));
  if (!handle)
    return gpg_error(GPG_ERR_ENOMEM);
  switch (algo) {
    case GCRY_MD_SHA1:
      handle->algo = HASH_AlgSHA1;
      break;
    case GCRY_MD_SHA256:
      handle->algo = HASH_AlgSHA256;
      break;
    default:
      assert(0);
  }
  handle->cx = NULL;
  *hd = handle;
  return gpg_error(GPG_ERR_NO_ERROR);
};
 
// This always follows a `gcry_md_open`.
gcry_error_t gcry_md_setkey(gcry_md_hd_t hd, const void *key, size_t keylen) {
  SECHashObject *hash_obj = NULL;
  hash_obj = (SECHashObject *)HASH_GetRawHashObject(hd->algo);
  hd->cx = HMAC_Create(
    hash_obj,
    (const unsigned char *)key,
    (unsigned int)keylen,
    PR_FALSE  // Not FIPS Mode, as far as I can tell.
  );
  return gpg_error(GPG_ERR_NO_ERROR);
};
 
void gcry_md_write(gcry_md_hd_t hd, const void *buffer, size_t length) {
  return HMAC_Update(
    hd->cx,
    (const unsigned char *)buffer,
    (unsigned int)length
  );
};
 
unsigned char *gcry_md_read(gcry_md_hd_t hd, int algo) {
  SECStatus status = SECSuccess;
  unsigned int result_len = 0;
  status = HMAC_Finish(
    hd->cx,
    hd->digest,
    &result_len,  // does this accept NULL? we're throwing it away.
    256
  );
  assert(status == SECSuccess);
  return hd->digest;
};
 
void gcry_md_reset(gcry_md_hd_t hd) {
  return HMAC_Begin(hd->cx);
};
 
void gcry_md_close(gcry_md_hd_t hd) {
  if (!hd)
    return;
  if (hd->cx)
    HMAC_Destroy(hd->cx, PR_TRUE);
  return free(hd);
};
 
void gcry_md_hash_buffer(int algo, void *digest, const void *buffer,
                         size_t length) {
  SECStatus status = SECSuccess;
  switch (algo) {
    case GCRY_MD_SHA1:
      status = SHA1_HashBuf(
        (unsigned char *)digest,
        (const unsigned char *)buffer,
        (PRUint32)length
      );
      break;
    case GCRY_MD_SHA256:
    // case SM_HASH_ALGORITHM:  This is defined as GCRY_MD_SHA256.
      status = SHA256_HashBuf(
        (unsigned char *)digest,
        (const unsigned char *)buffer,
        (PRUint32)length
      );
      break;
    default:
      assert(0);
  }
  assert(status == SECSuccess);
};
 
 
// All the s-expression stuff. We should just include libgcrypt's sexp.c and
// be done with it. nss has no facilities for dealing with s-expressions.
//gcry_sexp_t
//gcry_sexp_new
//gcry_sexp_build
//gcry_sexp_release
//gcry_sexp_find_token
//gcry_sexp_nth
//gcry_sexp_nth_mpi
//gcry_sexp_nth_data
//gcry_sexp_length
//gcry_sexp_sprint
 
 
// libotr only ever calls this with the s-exp "(genkey (dsa (nbits 4:1024)))".
// We could validate the tokens, but punting for now.
gcry_error_t gcry_pk_genkey(gcry_sexp_t *r_key, gcry_sexp_t s_parms) {
  gcry_err_code_t rc = GPG_ERR_NO_ERROR;
  SECStatus status = SECSuccess;
  mp_int p, q, g, x, y;
  PQGParams *params = NULL;
  PQGVerify *verify = NULL;
  DSAPrivateKey *privKey = NULL;
  unsigned int L = 1024;  // nbits, "4:" indicates the data length
 
  status = PQG_ParamGenV2(
    L,
    0, 0,  // zeroes here will pick the defaults (smallest secure being 160)
    &params,
    &verify  // no FIPS, so won't use this
  );
 
  if (status != SECSuccess) {
    rc = GPG_ERR_GENERAL;
    goto cleanup;
  }
 
  status = DSA_NewKey(
    params,
    &privKey
  );
 
  if (status != SECSuccess) {
    rc = GPG_ERR_GENERAL;
    goto cleanup;
  }
 
  mp_init(&p);
  mp_init(&q);
  mp_init(&g);
  mp_init(&x);
  mp_init(&y);
 
  SECITEM_TO_MPINT(privKey->params.prime, &p);
  SECITEM_TO_MPINT(privKey->params.subPrime, &q);
  SECITEM_TO_MPINT(privKey->params.base, &g);
  SECITEM_TO_MPINT(privKey->privateValue, &x);
  SECITEM_TO_MPINT(privKey->publicValue, &y);
 
  rc = sexp_build(
    r_key,
    NULL,
    "(key-data"
    " (public-key"
    "  (dsa(p%M)(q%M)(g%M)(y%M)))"
    " (private-key"
    "  (dsa(p%M)(q%M)(g%M)(y%M)(x%M)))"
    ")",
    &p, &q, &g, &y,
    &p, &q, &g, &y, &x
  );
 
cleanup:
  mp_clear(&p);
  mp_clear(&q);
  mp_clear(&g);
  mp_clear(&x);
  mp_clear(&y);
 
  if (params)
    PQG_DestroyParams(params);
 
  if (verify)
    PQG_DestroyVerify(verify);
 
  if (privKey)
    PORT_FreeArena(privKey->params.arena, PR_TRUE);
 
  return gpg_error(rc);
};
 
// From libgcrypt's dsa.c
typedef struct {
  gcry_mpi_t p;
  gcry_mpi_t q;
  gcry_mpi_t g;
  gcry_mpi_t y;
  gcry_mpi_t x;
} DSA_secret_key;
 
// TODO: `gcry_pk_verify` and `gcry_pk_sign` can be dried out.
 
// `data` is: (%m)
// `result` is expected to be: (dsa (r %m) (s %m))
// Assuming a DSA skey. We can assert that but punting for now.
gcry_error_t gcry_pk_sign(gcry_sexp_t *result, gcry_sexp_t data,
                          gcry_sexp_t skey) {
  SECStatus status = SECSuccess;
  gcry_err_code_t rc = GPG_ERR_NO_ERROR;
  gcry_mpi_t data_mpi = NULL;
  gcry_sexp_t list = NULL;
  gcry_sexp_t l2 = NULL;
  DSA_secret_key sk = { NULL, NULL, NULL, NULL, NULL };
  DSAPrivateKey *key = NULL;
  SECItem *digest = NULL;
  SECItem *signature = NULL;
  PLArenaPool *arena = NULL;
  mp_int r, s;
  unsigned int len = 0;
 
  // Alloc an arena
  arena = PORT_NewArena(NSS_FREEBL_DEFAULT_CHUNKSIZE);
  if (!arena) {
    rc = GPG_ERR_ENOMEM;
    goto cleanup;
  }
 
  // Convert data to a SECITEM
  data_mpi = sexp_nth_mpi(data, 0, 0);
  if (!data_mpi) {
    rc = GPG_ERR_INV_OBJ;
    goto cleanup;
  }
  digest = (SECItem *)PORT_ArenaZAlloc(arena, sizeof(SECItem));
  if (!digest) {
    rc = GPG_ERR_ENOMEM;
    goto cleanup;
  }
  MPINT_TO_SECITEM((mp_int *)data_mpi, digest, arena);
 
  // Get the key parameters. See the format above in `gcry_pk_genkey`.
  list = sexp_find_token(skey, "private-key", 0);
  if (!list) {
    rc = GPG_ERR_INV_OBJ;
    goto cleanup;
  }
 
  l2 = sexp_cadr(list);
  sexp_release(list);
  list = l2;
 
  rc = _gcry_sexp_extract_param(list, NULL, "pqgyx",
                                &sk.p, &sk.q, &sk.g, &sk.y, &sk.x, NULL);
  if (rc != GPG_ERR_NO_ERROR)
    goto cleanup;
 
  key = (DSAPrivateKey *)PORT_ArenaZAlloc(arena, sizeof(DSAPrivateKey));
  if (!key) {
    rc = GPG_ERR_ENOMEM;
    goto cleanup;
  }
 
  MPINT_TO_SECITEM((mp_int *)sk.p, &key->params.prime, arena);
  MPINT_TO_SECITEM((mp_int *)sk.q, &key->params.subPrime, arena);
  MPINT_TO_SECITEM((mp_int *)sk.g, &key->params.base, arena);
  MPINT_TO_SECITEM((mp_int *)sk.y, &key->publicValue, arena);
  MPINT_TO_SECITEM((mp_int *)sk.x, &key->privateValue, arena);
 
  // Alloc space for the signature
  signature = (SECItem *)PORT_ArenaZAlloc(arena, sizeof(SECItem));
  if (!signature) {
    rc = GPG_ERR_ENOMEM;
    goto cleanup;
  }
 
  // Sign the digest
  status = DSA_SignDigest(key, signature, digest);
  if (status != SECSuccess) {
    rc = GPG_ERR_GENERAL;
    goto cleanup;
  }
 
  // Convert the signature in SECITEM to sexp
  if (signature->len % 2) {
    rc = GPG_ERR_INV_OBJ;
    goto cleanup;
  }
  len = signature->len / 2;
 
  mp_init(&r);
  mp_init(&s);
 
  OCTETS_TO_MPINT(signature->data[0], &r, len);
  OCTETS_TO_MPINT(signature->data[len], &s, len);
 
  rc = sexp_build(result, NULL, "(sig-val(dsa(r%M)(s%M)))", &r, &s);
 
cleanup:
  mp_clear(&r);
  mp_clear(&s);
  PORT_FreeArena(arena, PR_TRUE);
  gcry_mpi_release(data_mpi);
  gcry_mpi_release(sk.p);
  gcry_mpi_release(sk.q);
  gcry_mpi_release(sk.g);
  gcry_mpi_release(sk.y);
  gcry_mpi_release(sk.x);
  sexp_release(list);
  return gpg_error(rc);
};
 
// `data` is: (%m)
// `sigval` is of the form: (sig-val (dsa (r %m) (s %m)))
// Assuming a DSA pkey. We can assert that but punting for now.
gcry_error_t gcry_pk_verify(gcry_sexp_t sigval, gcry_sexp_t data,
                            gcry_sexp_t pkey) {
  SECStatus status = SECSuccess;
  gcry_err_code_t rc = GPG_ERR_NO_ERROR;
  gcry_mpi_t data_mpi = NULL;
  gcry_mpi_t r = NULL;
  gcry_mpi_t s = NULL;
  gcry_sexp_t list = NULL;
  gcry_sexp_t l2 = NULL;
  DSA_secret_key sk = { NULL, NULL, NULL, NULL, NULL };
  DSAPrivateKey *key = NULL;
  SECItem *digest = NULL;
  SECItem *signature = NULL;
  PLArenaPool *arena = NULL;
  mp_err err = 0;
  unsigned int dsa_subprime_len = 0;
 
  // Alloc an arena
  arena = PORT_NewArena(NSS_FREEBL_DEFAULT_CHUNKSIZE);
  if (!arena) {
    rc = GPG_ERR_ENOMEM;
    goto cleanup;
  }
 
  // Convert data to a SECITEM
  data_mpi = sexp_nth_mpi(data, 0, 0);
  if (!data_mpi) {
    rc = GPG_ERR_INV_OBJ;
    goto cleanup;
  }
  digest = (SECItem *)PORT_ArenaZAlloc(arena, sizeof(SECItem));
  if (!digest) {
    rc = GPG_ERR_ENOMEM;
    goto cleanup;
  }
  MPINT_TO_SECITEM((mp_int *)data_mpi, digest, arena);
 
  // Get the key parameters. See the format above in `gcry_pk_genkey`.
  list = sexp_find_token(pkey, "private-key", 0);
  if (!list) {
    rc = GPG_ERR_INV_OBJ;
    goto cleanup;
  }
 
  l2 = sexp_cadr(list);
  sexp_release(list);
  list = l2;
 
  rc = _gcry_sexp_extract_param(list, NULL, "pqgyx",
                                &sk.p, &sk.q, &sk.g, &sk.y, &sk.x, NULL);
  if (rc != GPG_ERR_NO_ERROR)
    goto cleanup;
 
  key = (DSAPrivateKey *)PORT_ArenaZAlloc(arena, sizeof(DSAPrivateKey));
  if (!key) {
    rc = GPG_ERR_ENOMEM;
    goto cleanup;
  }
 
  MPINT_TO_SECITEM((mp_int *)sk.p, &key->params.prime, arena);
  MPINT_TO_SECITEM((mp_int *)sk.q, &key->params.subPrime, arena);
  MPINT_TO_SECITEM((mp_int *)sk.g, &key->params.base, arena);
  MPINT_TO_SECITEM((mp_int *)sk.y, &key->publicValue, arena);
  MPINT_TO_SECITEM((mp_int *)sk.x, &key->privateValue, arena);
 
  // Convert the signature in sexp to SECITEM.
  // See the format above in `gcry_pk_sign`.
  list = sexp_find_token(sigval, "sig-val", 0);
  if (!list) {
    rc = GPG_ERR_INV_OBJ;
    goto cleanup;
  }
 
  l2 = sexp_cadr(list);
  sexp_release(list);
  list = l2;
 
  rc = _gcry_sexp_extract_param(list, NULL, "rs", &r, &s, NULL);
  if (rc != GPG_ERR_NO_ERROR)
    goto cleanup;
 
  signature = (SECItem *)PORT_ArenaZAlloc(arena, sizeof(SECItem));
 
  dsa_subprime_len = PQG_GetLength(&key->params.subPrime);
 
  err = mp_to_fixlen_octets(r, signature->data, dsa_subprime_len);
  if (err) {
    rc = GPG_ERR_GENERAL;
    goto cleanup;
  }
 
  err = mp_to_fixlen_octets(s, signature->data + dsa_subprime_len,
                            dsa_subprime_len);
  if (err) {
    rc = GPG_ERR_GENERAL;
    goto cleanup;
  }
 
  signature->len = dsa_subprime_len * 2;
 
  // Verify the signature
  status = DSA_VerifyDigest(key, signature, digest);
  if (status != SECSuccess)
    rc = GPG_ERR_GENERAL;
 
cleanup:
  PORT_FreeArena(arena, PR_TRUE);
  gcry_mpi_release(data_mpi);
  gcry_mpi_release(sk.p);
  gcry_mpi_release(sk.q);
  gcry_mpi_release(sk.g);
  gcry_mpi_release(sk.y);
  gcry_mpi_release(sk.x);
  gcry_mpi_release(r);
  gcry_mpi_release(s);
  sexp_release(list);
  return gpg_error(rc);
};
