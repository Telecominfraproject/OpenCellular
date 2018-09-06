REGISTRY_HOST?=docker.io
ORG?=opencellular
NAME?=$(shell basename $(CURDIR))
IMAGE?=$(REGISTRY_HOST)/$(ORG)/$(NAME)

docker-build:
	        docker build --build-arg USER=$(ORG) -t $(IMAGE):latest .

