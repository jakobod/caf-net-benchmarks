# CAF-net Benchmarks

This repository contains some benchmark suites for the caf-net library, which is currently being implemented.

# What is included?

Contained in this repo are:
- pingpong benchmark
- streaming benchmark (using CAF streams)
- simple_streaming benchmark (streaming without CAF streams)

# How to build
Since caf-net is developed in another repository, you will have to specify both build directories to build it.

### Build CAF
```bash
git clone https://github.com/actor-framework/actor-framework.git
cd actor-framework
./configure --build-type=Release --no-examples --no-openssl
make -C build -j$(nproc)
cd ..
```

### Build the incubator
```bash
git clone https://github.com/actor-framework/incubator.git
cd incubator
./configure --build-type=Release --with-caf=<CAF_BUILD_DIR>
make -C build -j$(nproc)
cd ..
```

### Build this repo
```bash
git clone https://github.com/jakobod/caf-net-benchmarks.git
cd caf-net-benchmarks
./configure --with-caf=<CAF_BUILD_DIR> --with-caf-net=<CAF_NET_BUILD_DIR>
make -C build -j$(nproc)
cd ..
```

