#pragma once

// GETs are idempotent. Retry one transport failure on a fresh TLS connection;
// leave HTTP errors to the caller and preserve its certificate configuration.
template<class Http, class Tls, class Body, class Error, class Wait>
bool chainGetBody(Http& http, Tls& tls, const char* url, Body& body,
                  Error error, Wait wait)
{
    body = "";
    for (unsigned attempt = 1; attempt <= 2; ++attempt) {
        if (!http.begin(tls, url)) {
            http.end();
            tls.stop();
            return false;
        }
        int code = http.GET();
        if (code == 200) {
            body = http.getString();
            http.end();  // permit reuse for the next endpoint in this cycle
            return body.length() != 0;
        }
        error(code, attempt);
        http.end();
        tls.stop();       // never reuse a failed or unread response
        if (code >= 0 || attempt == 2) return false;
        wait();
    }
    return false;
}
