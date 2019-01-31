# This Dockerfile downloads, builds, and installs ProxyGen and all 
# necessary dependencies. It could be used to base other containers off of
# that will run the Gladius Masternode application.

FROM ubuntu:16.04 as proxygen-env

RUN apt-get update && \
        apt-get upgrade -y && \
        apt-get install -y \
                apt-utils \
                git \
                sudo \
                bc \
                libdouble-conversion1v5

RUN useradd -m docker && echo "docker:docker" | chpasswd && adduser docker sudo

# Clone the ProxyGen library
RUN git clone https://github.com/facebook/proxygen.git && \
    cd proxygen && \
    git checkout 5f95b45182018f71b5c43af4035b236eaf88cb89

WORKDIR /proxygen

# Tweak the build libraries to get it passing
RUN sed -i 's/\(LIBS="$LIBS $BOOST.*\)"/\1 -ldl -levent_core -lssl"/' proxygen/configure.ac && \
    sed -i 's/\(LIBS="$LIBS -ldouble.*\)"/\1 -lboost_context -lboost_regex -lboost_filesystem -lsodium"/' proxygen/configure.ac

WORKDIR /proxygen/proxygen

# Build and install ProxyGen
RUN ./deps.sh -j $(printf %.0f $(echo "$(nproc) * 1.5" | bc -l))

# Remove gigantic source code tree
RUN rm -rf /proxygen

# Tell the linker where to find ProxyGen and friends
ENV LD_LIBRARY_PATH /usr/local/lib

# ###################################################
# Use a separate stage to build the masternode binary  
# ###################################################
FROM proxygen-env as masternode-builder

# Install google test libs
RUN apt-get install -y libgtest-dev gdb

RUN cd /usr/src/gtest && \
        cmake CMakeLists.txt && \
        make && \
        cp *.a $LD_LIBRARY_PATH

# Install html parsing library
RUN git clone https://github.com/lexborisov/myhtml && \
    cd myhtml && \
    git checkout 18d5d82b21f854575754c41f186b1a3f8778bb99 && \
    make && \
    make test && \
    sudo make install

# Clean up APT when done
RUN apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

WORKDIR /app

# Move src and build template files over from host
COPY src/ .

# Invoke autotools toolchain to create configure file and Makefiles
RUN aclocal && autoconf && automake --add-missing

# Make a separate directory for build artifacts
WORKDIR /app/build

# Generates actual Makefile
RUN ../configure

# Build the masternode
RUN make -j $(printf %.0f $(echo "$(nproc) * 1.5" | bc -l)) \
        && mkdir -p /tmp/dist \
        && cp `ldd masternode | grep -v nux-vdso | awk '{print $3}'` /tmp/dist/

# ###################################################
# # Use a separate stage to deliver the binary
# ###################################################
FROM ubuntu:16.04 as production

WORKDIR /app

COPY --from=masternode-builder /tmp/dist/* /usr/lib/x86_64-linux-gnu/
COPY --from=masternode-builder /app/build/masternode .

ENV UPGRADE_INSECURE false

ENTRYPOINT ./masternode --origin_host=$ORIGIN_HOST \
        --origin_port=$ORIGIN_PORT \
        --protected_host=$PROTECTED_HOST \
        --cert_path=$CERT_PATH \
        --key_path=$KEY_PATH \
        --cache_dir=$CACHE_DIR \
        --gateway_address=$GATEWAY_ADDRESS \
        --sw_path=$SW_PATH \
        --upgrade_insecure=$UPGRADE_INSECURE
