#pragma once
#include <openssl/evp.h>

// Replace only the platform SHA boundary, so host tests exercise the actual
// firmware's coinbase assembly, merkle tree and header byte ordering.
struct mbedtls_sha256_context { EVP_MD_CTX* value; };
inline void mbedtls_sha256_init(mbedtls_sha256_context* ctx) { ctx->value = EVP_MD_CTX_new(); }
inline int mbedtls_sha256_starts_ret(mbedtls_sha256_context* ctx, int) {
    return EVP_DigestInit_ex(ctx->value, EVP_sha256(), nullptr) == 1 ? 0 : -1;
}
inline int mbedtls_sha256_update_ret(mbedtls_sha256_context* ctx, const unsigned char* data, size_t size) {
    return EVP_DigestUpdate(ctx->value, data, size) == 1 ? 0 : -1;
}
inline int mbedtls_sha256_finish_ret(mbedtls_sha256_context* ctx, unsigned char* out) {
    return EVP_DigestFinal_ex(ctx->value, out, nullptr) == 1 ? 0 : -1;
}
inline void mbedtls_sha256_free(mbedtls_sha256_context* ctx) { EVP_MD_CTX_free(ctx->value); }
