FROM ubuntu:16.04

RUN apt-get update && apt-get upgrade -y
RUN apt-get install -y \
        apt-utils \
        git \
        sudo \
        bc
RUN useradd -m docker && echo "docker:docker" | chpasswd && adduser docker sudo

RUN git clone https://github.com/facebook/proxygen.git && \
    cd proxygen && \
    git checkout 5f95b45182018f71b5c43af4035b236eaf88cb89

WORKDIR /proxygen
RUN sed -i 's/\(LIBS="$LIBS $BOOST.*\)"/\1 -ldl -levent_core -lssl"/' proxygen/configure.ac && \
    sed -i 's/\(LIBS="$LIBS -ldouble.*\)"/\1 -lboost_context -lboost_regex -lboost_filesystem -lsodium"/' proxygen/configure.ac
WORKDIR /proxygen/proxygen
RUN ./deps.sh -j $(printf %.0f $(echo "$(nproc) * 1.5" | bc -l))
