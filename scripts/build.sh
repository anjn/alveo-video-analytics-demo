# Build program
mkdir -p /tmp/build
pushd /tmp/build
cmake /workspace/demo
make -j$(nproc) $*
