FROM ubuntu:16.04

RUN apt-get update && apt-get upgrade -y
RUN apt-get install -y \
        apt-utils \
        git \
        sudo \
        bc
RUN useradd -m docker && echo "docker:docker" | chpasswd && adduser docker sudo

COPY . /app
WORKDIR /app/proxygen
RUN sed -i 's/\(LIBS="$LIBS $BOOST.*\)"/\1 -ldl -levent_core -lssl"/' proxygen/configure.ac && \
    sed -i 's/\(LIBS="$LIBS -ldouble.*\)"/\1 -lboost_context -lboost_regex -lboost_filesystem -lsodium"/' proxygen/configure.ac
WORKDIR /app/proxygen/proxygen
RUN ./deps.sh -j $(printf %.0f $(echo "$(nproc) * 1.5" | bc -l))


ENV GLADIUSBASE=/gladius
RUN mkdir -p ${GLADIUSBASE}/wallet
RUN mkdir -p ${GLADIUSBASE}/keys
RUN touch ${GLADIUSBASE}/gladius-masternode.toml


