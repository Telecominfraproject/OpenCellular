REGISTRY_HOST?=docker.io
#ORG?=opencellular
NAME?=$(shell basename $(CURDIR))
IMAGE?=$(REGISTRY_HOST)/$(ORG)/$(NAME)


HASH?=$(shell git ls-tree HEAD -- ../ | grep \\s\.\/$ | awk '{print $$3}')

hash:
		@echo ${HASH}

docker-build:
	docker build --build-arg USER=$(ORG) -t $(IMAGE):$(HASH) .
	docker tag $(IMAGE):$(HASH) $(IMAGE):latest
