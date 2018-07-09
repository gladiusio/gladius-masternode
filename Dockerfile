FROM golang:1.10 AS builder

ADD https://github.com/golang/dep/releases/download/v0.4.1/dep-linux-amd64 /usr/bin/dep
RUN chmod +x /usr/bin/dep

WORKDIR $GOPATH/src/github.com/gladiusio/gladius-masternode
COPY . ./
COPY ./html /html
RUN make dependencies
RUN make docker

ENV GLADIUSBASE=/gladius
RUN mkdir -p ${GLADIUSBASE}/wallet
RUN mkdir -p ${GLADIUSBASE}/keys
RUN touch ${GLADIUSBASE}/gladius-masternode.toml
RUN echo 'ControldHostname = "gladius-controld-masternode"' > ${GLADIUSBASE}/gladius-masternode.toml

########################################

FROM ubuntu
ENV GLADIUSBASE=/gladius
COPY --from=builder ${GLADIUSBASE}/wallet ${GLADIUSBASE}/wallet
COPY --from=builder ${GLADIUSBASE}/keys ${GLADIUSBASE}/keys
COPY --from=builder ${GLADIUSBASE}/gladius-masternode.toml ${GLADIUSBASE}/gladius-masternode.toml
COPY --from=builder ./html /html

COPY --from=builder /gladius-masternode ./
ENTRYPOINT ["./gladius-masternode"]
