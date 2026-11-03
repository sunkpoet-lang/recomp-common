// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of recomp-common: https://github.com/tmcleroy/recomp-common
//
// The proxy Windows is set to use, as curl takes it (src/proxy.h).
#include "proxy.h"

#include <cstdio>

static int failures = 0;
#define CHECK(x)                                                         \
    do {                                                                 \
        if (!(x)) {                                                      \
            std::fprintf(stderr, "%s:%d: FAILED: %s\n", __FILE__, __LINE__, #x); \
            failures++;                                                  \
        }                                                                \
    } while (0)

int main() {
    using recomp::ProxyFromSetting;
    CHECK(ProxyFromSetting("") == "");
    CHECK(ProxyFromSetting("127.0.0.1:7890") == "http://127.0.0.1:7890");  // (Clash's)
    CHECK(ProxyFromSetting("http://127.0.0.1:10809") == "http://127.0.0.1:10809");
    CHECK(ProxyFromSetting("http=127.0.0.1:1080;https=127.0.0.1:1081") == "http://127.0.0.1:1081");
    CHECK(ProxyFromSetting("http=proxy:8080;ftp=proxy:21") == "http://proxy:8080");
    CHECK(ProxyFromSetting("socks=127.0.0.1:1080") == "socks4a://127.0.0.1:1080");
    CHECK(ProxyFromSetting("ftp=proxy:21") == "");
    CHECK(ProxyFromSetting(" 127.0.0.1:7890 ;") == "http://127.0.0.1:7890");
    recomp::UseSystemProxy();  // (does nothing but on Windows)
    if (failures == 0) {
        std::printf("proxy_test: all as it should be\n");
    }
    return failures == 0 ? 0 : 1;
}
