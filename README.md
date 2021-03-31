# gRPC performance test application

the project supports Windows, Linux and Linux RT for both the client and server.
Always build release when running benchmarks.  There is a large difference in performance between debug and release.

## Building on Windows

### Prerequisites
To prepare for cmake + Microsoft Visual C++ compiler build
- Install Visual Studio 2015, 2017, or 2019 (Visual C++ compiler will be used).
- Install [Git](https://git-scm.com/).
- Install gRPC for C++
- Install [CMake](https://cmake.org/download/).


### Building
- Launch "x64 Native Tools Command Prompt for Visual Studio"

Download the repo and update submodules, this will pull the gRPC components and all dependencies

```
> git clone https://github.com/ni/labview-grpc-query-server.git labview-grpc-query-server
> cd grpc-perf
> git submodule update --init --recursive
```

Build Debug
```
> mkdir build
> cd build
> cmake ..
> cmake --build .
```

Build Release
```
> mkdir build
> cd build
> cmake ..
> cmake --build . --config Release
```

## Building on Linux

Download the repo and update submodules, this will pull the gRPC components and all dependencies

```
> git clone https://github.com/ni/labview-grpc-query-server.git labview-grpc-query-server
> cd grpc-perf
> git submodule update --init --recursive
```

Build

```
> cmake .
> make
```

Build Release

```
> cmake -DCMAKE_BUILD_TYPE=Release .
> make
```

## Building on Linux RT

Install required packages not installed by default

```
> opkg update
> opkg install git
> opkg install git-perltools
> opkg install cmake
> opkg install g++
> opkg install g++-symlinks
```

Download the repo and update submodules, this will pull the gRPC components and all dependencies

```
> git clone https://github.com/ni/labview-grpc-query-server.git labview-grpc-query-server
> cd labview-grpc-query-server
> git submodule update --init --recursive
```

Build

```
> cmake .
> make
```

## Running Tests

Start the server, perftest_server, on the server machine.
Run the client, perftest_client.
If you want to run the client on a different machine pass in the server to conect to: perftest_client --target={server name or ip}

## SSL/TLS Support

You can enable SSL/TLS support on the server by passing in a path to a certificate and private key for the server.

You can generate the certificate with openssl using the following script.

```
mypass="password123"

echo Generate server key:
openssl genrsa -passout pass:$mypass -des3 -out server.key 4096

echo Generate server signing request:
openssl req -passin pass:$mypass -new -key server.key -out server.csr -subj  "/C=US/ST=TX/L=Austin/O=NI/OU=labview/CN=localhost"

echo Self-sign server certificate:
openssl x509 -req -passin pass:$mypass -days 365 -in server.csr -signkey server.key -set_serial 01 -out server.crt

echo Remove passphrase from server key:
openssl rsa -passin pass:$mypass -in server.key -out server.key

rm server.csr
```

Clients then must connect using the server certificate that was generated (server.cer) otherwise the connection will fail.

If you do not passing in a certificate then the server will use insecure gRPC.
