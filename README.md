# rudesocket

A small C++ TCP socket client library with SSL/TLS, SOCKS4/5, HTTP proxy,
and tunnel support.

RudeSocket wraps BSD sockets (and WinSock) behind one simple class: connect,
send, read lines, time out cleanly — optionally through a chain of proxies,
and optionally over TLS.

First released in the early 2000s as part of the
[RudeServer](https://github.com/mflood) C++ CGI library family; modernized in
2026 (CMake, C++17, TLS repair: SNI + certificate verification, CI).

## Quick start

Plain TCP:

```cpp
#include <rude/socket.h>
#include <iostream>

int main()
{
    rude::Socket socket;
    socket.setTimeout(10, 0);                 // 10s connect/read/send timeout
    if (!socket.connect("example.com", 80)) {
        std::cerr << socket.getError() << "\n";
        return 1;
    }
    socket.sends("GET / HTTP/1.0\r\nHost: example.com\r\nConnection: close\r\n\r\n");
    std::cout << socket.readline() << "\n";   // "HTTP/1.0 200 OK"
    socket.close();
}
```

SSL/TLS (SNI and certificate verification are automatic):

```cpp
rude::Socket socket;
socket.setTimeout(10, 0);
if (!socket.connectSSL("example.com", 443)) {
    std::cerr << socket.getError() << "\n";   // includes OpenSSL error details
    return 1;
}
socket.sends("GET / HTTP/1.0\r\nHost: example.com\r\nConnection: close\r\n\r\n");
std::cout << socket.readline() << "\n";
socket.close();
```

Compile with:

```sh
c++ -std=c++17 app.cpp $(pkg-config --cflags --libs rudesocket)
```

**Behavior change (1.3.0): `connectSSL()` now verifies the server's
certificate by default.** The certificate chain is checked against the
system's trusted CAs and the certificate must match the requested hostname;
the connection fails otherwise. Code that previously "worked" against
self-signed or mismatched certificates must now opt out explicitly:

```cpp
socket.setSSLVerify(false);   // accept any certificate (e.g. self-signed test servers)
socket.connectSSL("my-test-host", 8443);
```

## Building

Requires CMake ≥ 3.16 and any C++17 compiler. OpenSSL is required only for
SSL support (on by default; the build falls back to plain TCP if OpenSSL is
not found).

```sh
git clone https://github.com/mflood/rudesocket
cd rudesocket
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build            # run the tests
cmake --install build             # add --prefix ~/some/dir for a local install
```

- `-DRUDESOCKET_WITH_SSL=OFF` builds without OpenSSL; `connectSSL()` then
  fails gracefully with a clear `getError()` message.
- On macOS with Homebrew OpenSSL:
  `-DOPENSSL_ROOT_DIR=/opt/homebrew/opt/openssl@3`.

This builds a static library by default; add `-DBUILD_SHARED_LIBS=ON` for a
shared library. An example program lives in [`examples/demo.cpp`](examples/demo.cpp)
(built as `build/examples/demo`) — it performs a plain HTTP request and, when
SSL is enabled, an HTTPS request to example.com with verification on.

### Using from CMake

```cmake
find_package(rudesocket REQUIRED)
target_link_libraries(myapp PRIVATE rudesocket::rudesocket)
```

Or vendor it with `FetchContent`:

```cmake
include(FetchContent)
FetchContent_Declare(rudesocket
    GIT_REPOSITORY https://github.com/mflood/rudesocket
    GIT_TAG master)
FetchContent_MakeAvailable(rudesocket)
target_link_libraries(myapp PRIVATE rudesocket::rudesocket)
```

When the library was built with SSL, the compile definition
`RUDESOCKET_WITH_SSL` is exported to consumers.

## API notes

- The full API is documented in [`src/socket.h`](src/socket.h) and the
  `rudesocket(3)` man page.
- **Timeouts**: `setTimeout(seconds, microseconds)` applies to connect, read,
  and send; `setTimeout(0, 0)` makes the socket blocking. Timeouts are honored
  on SSL connections too.
- **Reading**: `readline()` reads up to CRLF (discarding it), `reads()` reads
  until the peer closes, `read(buf, n)` reads exactly `n` bytes. On failure
  they return null / negative and `getError()` explains why — including
  OpenSSL error details for SSL connections.
- **SSL verification**: on by default (system CA store + hostname check).
  `setSSLVerify(false)` disables it for that socket. IP-literal hosts skip
  SNI per RFC 6066.
- **Proxy chaining**: `insertProxy` / `insertSocks4` / `insertSocks5` /
  `insertTunnel` before `connect()`/`connectSSL()` route the connection
  through the given servers in order.
- **Windows**: call `WSAStartup`/`WSACleanup` yourself (see
  [`examples/demo.cpp`](examples/demo.cpp)).

## History

- **1.3.0** (2026) — CMake build; C++17; TLS repair: SNI support (modern
  servers no longer reject the handshake), certificate verification **on by
  default** with `setSSLVerify(false)` opt-out, `readline()` over SSL no
  longer deadlocks (`SSL_pending`), timeouts honored on SSL reads, SSL errors
  reported through `getError()`; OpenSSL ≥ 1.1 API; ctest suite and CI on
  Linux (x86_64 + ARM), macOS, and Windows. The legacy autotools files are
  still present but no longer maintained.
- **1.2.0** (2008) — last release of the original autotools era.

## License

GPL-2.0-or-later — see [COPYING](COPYING).
