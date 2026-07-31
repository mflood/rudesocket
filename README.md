# RudeSocket

RudeSocket is a portable C++ TCP client library with TLS, timeouts, proxy
chaining, and a straightforward line-oriented API.

## Why RudeSocket?

- Uses one API across POSIX sockets and WinSock.
- Supports TLS with SNI, system trust stores, and hostname verification.
- Applies caller-controlled timeouts to connect, read, and send operations.
- Supports HTTP proxies, SOCKS4, SOCKS5, tunnels, and proxy chains.
- Can establish TLS at connect time or upgrade an existing connection with
  `startSSL()` for protocols such as SMTP, IMAP, and POP3.
- Builds with or without OpenSSL.

## Quick start

Plain TCP:

```cpp
#include <rude/socket.h>

#include <iostream>

int main()
{
    rude::Socket socket;
    socket.setTimeout(10, 0);

    if (!socket.connect("example.com", 80)) {
        std::cerr << socket.getError() << "\n";
        return 1;
    }

    if (!socket.sends("GET / HTTP/1.0\r\n"
                      "Host: example.com\r\n"
                      "Connection: close\r\n\r\n")) {
        std::cerr << socket.getError() << "\n";
        return 1;
    }
    std::cout << socket.readline() << "\n";
}
```

TLS uses the same interface:

```cpp
rude::Socket socket;
socket.setTimeout(10, 0);

if (!socket.connectSSL("example.com", 443)) {
    std::cerr << socket.getError() << "\n";
    return 1;
}

if (!socket.sends("GET / HTTP/1.0\r\n"
                  "Host: example.com\r\n"
                  "Connection: close\r\n\r\n")) {
    std::cerr << socket.getError() << "\n";
    return 1;
}
std::cout << socket.readline() << "\n";
```

Compile a static installed copy with pkg-config:

```sh
c++ -std=c++17 app.cpp $(pkg-config --cflags --libs --static rudesocket)
```

For a shared build, use `pkg-config --cflags --libs rudesocket` instead.

## Build and install

RudeSocket requires CMake 3.16 or newer and a C++17 compiler. OpenSSL is
optional and enables TLS support. If OpenSSL is unavailable, CMake reports the
condition and produces a TCP-only build; use `-DRUDESOCKET_WITH_SSL=OFF` to
request that configuration explicitly.

```sh
git clone https://github.com/mflood/rudesocket.git
cd rudesocket
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build
cmake --install build --prefix ./install
```

Useful configuration options:

- `-DRUDESOCKET_WITH_SSL=OFF` builds TCP and proxy support without OpenSSL.
- `-DBUILD_SHARED_LIBS=ON` builds a shared library instead of the default
  static library.
- Homebrew OpenSSL can be selected with
  `-DOPENSSL_ROOT_DIR=/opt/homebrew/opt/openssl@3`.

When TLS is disabled, `connectSSL()` and `startSSL()` report the unavailable
capability through `getError()`.

For an install outside the system prefix, point CMake consumers at it with
`-DCMAKE_PREFIX_PATH=/path/to/install`. For pkg-config, add
`/path/to/install/lib/pkgconfig` to `PKG_CONFIG_PATH`.

The complete runnable example is in
[`examples/demo.cpp`](examples/demo.cpp).

## Use from CMake

With an installed copy:

```cmake
find_package(rudesocket REQUIRED)
target_link_libraries(myapp PRIVATE rudesocket::rudesocket)
```

Or include it directly with `FetchContent`:

```cmake
include(FetchContent)
FetchContent_Declare(rudesocket
    GIT_REPOSITORY https://github.com/mflood/rudesocket.git
    GIT_TAG v1.7.1)
FetchContent_MakeAvailable(rudesocket)
target_link_libraries(myapp PRIVATE rudesocket::rudesocket)
```

The CMake target and pkg-config metadata carry the required OpenSSL linkage
and the `RUDESOCKET_WITH_SSL` compile definition to consumers.

## Core API

### Timeouts

`setTimeout(seconds, microseconds)` applies to connect, read, and send
operations, including TLS connections. `setTimeout(0, 0)` selects blocking
operation.

### Reading

- `readline()` reads through CRLF and returns the line without the terminator.
- `reads()` reads until the peer closes the connection.
- `read(buffer, count)` reads an exact byte count.

Failures return a null or negative result as appropriate, and `getError()`
provides transport or TLS details.

### Proxy chaining

Call `insertProxy()`, `insertSocks4()`, `insertSocks5()`, or `insertTunnel()`
before `connect()` or `connectSSL()`. Multiple entries are followed in the
order they were inserted.

### Windows initialization

Windows applications must call `WSAStartup()` before using RudeSocket and
`WSACleanup()` during shutdown. The runnable example includes the complete
Windows setup.

## TLS security

TLS connections verify both the certificate chain and requested hostname by
default. OpenSSL's configured CA locations supply trusted roots, and SNI is
sent for DNS hostnames.

For a controlled development environment using a self-signed certificate,
verification can be disabled explicitly for that socket:

```cpp
rude::Socket socket;
socket.setSSLVerify(false);
socket.connectSSL("test-server.local", 8443);
```

Do not disable verification for production or untrusted networks.

## Documentation and support

- Public API: [`src/socket.h`](src/socket.h)
- Runnable example: [`examples/demo.cpp`](examples/demo.cpp)
- Manual page: `rudesocket(3)`
- Release notes: [`NEWS`](NEWS)
- Bug reports: [GitHub Issues](https://github.com/mflood/rudesocket/issues)

## License

GPL-2.0-or-later. See [`COPYING`](COPYING).
