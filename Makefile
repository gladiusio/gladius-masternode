##
## Makefile to test and build the gladius masternode
##

##
# GLOBAL VARIABLES
##

# if we are running on a windows machine
# we need to append a .exe to the
# compiled binary
BINARY_SUFFIX=
ifeq ($(OS),Windows_NT)
	BINARY_SUFFIX=.exe
endif

ifeq ($(GOOS),windows)
	BINARY_SUFFIX=.exe
endif

# code source and build directories
SRC_DIR=./cmd
DST_DIR=./build

MN_SRC=$(SRC_DIR)
MN_DEST=$(DST_DIR)/gladius-masternode$(BINARY_SUFFIX)

# commands for go
GOBUILD=go build
GOTEST=go test
##
# MAKE TARGETS
##

# general make targets
all: masternode

clean:
	rm -rf ./build/*
	go clean

# dependency management
dependencies:
	# install go packages
	dep ensure

test: $(MN_SRC)
	$(GOTEST) ./...

lint:
	gometalinter ./...

masternode: test lint
	$(GOBUILD) -o $(MN_DEST) $(MN_SRC)
