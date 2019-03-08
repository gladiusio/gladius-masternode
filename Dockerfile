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
                libdouble-conversion1v5 \
                hardening-wrapper

ENV DEB_BUILD_HARDENING=1

RUN useradd -m docker && echo "docker:docker" | chpasswd && adduser docker sudo

# Clone the ProxyGen library
RUN git clone https://github.com/facebook/proxygen.git && \
    cd proxygen && \
    git checkout 66e6f6846c7035cd7f7fd191f3afccc2b190f400

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

# Copies only the libraries necessary to run the masternode from the
# builder stage.
COPY --from=masternode-builder /tmp/dist/* /usr/lib/x86_64-linux-gnu/

# Copies the masternode binary to this image
COPY --from=masternode-builder /app/build/masternode .

# Command to run the masternode. The command line flags passed to the
# masternode executable can be set by providing environment variables
# to the docker container individually or with an env file.
# See: https://docs.docker.com/engine/reference/commandline/run/#set-environment-variables--e---env---env-file
ENTRYPOINT ./masternode --logtostderr=1 --tryfromenv=ip,port,ssl_port,origin_host,origin_port,protected_domain,cert_path,key_path,cache_dir,gateway_address,gateway_port,sw_path,upgrade_insecure,pool_domain,cdn_subdomain
