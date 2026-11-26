// Arduino 2.x tears down the TLS context before WiFiClientSecure::lastError()
// can report the certificate flags. Observe the verifier before that happens.
// This wrapper NEVER clears flags, changes the profile, or accepts a failed
// certificate. Enable it with --wrap=mbedtls_x509_crt_verify_restartable.
#ifdef PANICMINER_UI
#include <Arduino.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/error.h>
#include <string.h>
#include <time.h>
#include "chainFeed.h"

extern "C" int __real_mbedtls_pk_verify_ext(mbedtls_pk_type_t, const void*,
    mbedtls_pk_context*, mbedtls_md_type_t, const unsigned char*, size_t,
    const unsigned char*, size_t);
extern "C" int __wrap_mbedtls_pk_verify_ext(mbedtls_pk_type_t type, const void* options,
    mbedtls_pk_context* pk, mbedtls_md_type_t md, const unsigned char* hash, size_t hash_len,
    const unsigned char* sig, size_t sig_len)
{
    int ret = __real_mbedtls_pk_verify_ext(type, options, pk, md, hash, hash_len, sig, sig_len);
    if (ret && chainFeedIsCurrentTask()) {
        char error[128];
        mbedtls_strerror(ret, error, sizeof(error));
        Serial.printf("[CHAIN TLS] signature verify %d: %s; heap %u, largest %u\n", ret, error,
            (unsigned)ESP.getFreeHeap(), (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    }
    return ret;
}

typedef int (*verify_cb_t)(void*, mbedtls_x509_crt*, int, uint32_t*);

struct verify_context_t {
    verify_cb_t callback;
    void* context;
};

static int report_certificate(void* arg, mbedtls_x509_crt* crt, int depth, uint32_t* flags)
{
    verify_context_t* ctx = static_cast<verify_context_t*>(arg);
    // Preserve an existing callback and its return value exactly.
    int ret = ctx->callback ? ctx->callback(ctx->context, crt, depth, flags) : 0;
    if (*flags) {
        char subject[192], info[192];
        mbedtls_x509_dn_gets(subject, sizeof(subject), &crt->subject);
        mbedtls_x509_crt_verify_info(info, sizeof(info), "", *flags);
        // Keep every diagnostic on one serial line.
        for (char* p = info; *p; ++p) if (*p == '\n' || *p == '\r') *p = ';';
        Serial.printf("[CHAIN TLS] depth %d flags 0x%08lx: %s; %s\n", depth,
                      (unsigned long)*flags, subject, info);
    }
    return ret;
}

extern "C" int __real_mbedtls_x509_crt_verify_restartable(
    mbedtls_x509_crt*, mbedtls_x509_crt*, mbedtls_x509_crl*,
    const mbedtls_x509_crt_profile*, const char*, uint32_t*, verify_cb_t, void*,
    mbedtls_x509_crt_restart_ctx*);

extern "C" int __wrap_mbedtls_x509_crt_verify_restartable(
    mbedtls_x509_crt* crt, mbedtls_x509_crt* trust_ca, mbedtls_x509_crl* crl,
    const mbedtls_x509_crt_profile* profile, const char* hostname, uint32_t* flags,
    verify_cb_t callback, void* context, mbedtls_x509_crt_restart_ctx* restart)
{
    if (!hostname || strcmp(hostname, "mempool.space") != 0)
        return __real_mbedtls_x509_crt_verify_restartable(crt, trust_ca, crl, profile,
                                                        hostname, flags, callback, context, restart);
    verify_context_t ctx = {callback, context};
    int ret = __real_mbedtls_x509_crt_verify_restartable(crt, trust_ca, crl, profile,
                                                       hostname, flags, report_certificate, &ctx, restart);
    if (ret) Serial.printf("[CHAIN TLS] verify %d flags 0x%08lx; system epoch %lu\n",
                           ret, (unsigned long)*flags, (unsigned long)time(NULL));
    return ret;
}
#endif
