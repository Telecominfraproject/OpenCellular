FROM docker/compose:1.22.0
# because compose requires all sorts of dynamic libs, including glibc, it is much easier to
#   add docker client to compose than the reverse

ENV DOCKER_BUCKET download.docker.com
ENV DOCKER_VERSION 18.06.0-ce
ENV DOCKER_SHA256 1c2fa625496465c68b856db0ba850eaad7a16221ca153661ca718de4a2217705

# we need docker compose and docker load
# also need curl to test availability of docker API
RUN apk add --update curl

# we only need the client
RUN set -x \
	&& curl -fSL "https://${DOCKER_BUCKET}/linux/static/stable/x86_64/docker-${DOCKER_VERSION}.tgz" -o docker.tgz \
	&& echo "${DOCKER_SHA256} *docker.tgz" | sha256sum -c - \
	&& tar -xzvf docker.tgz \
	&& mv docker/docker /usr/local/bin/ \
	&& rm -rf docker docker.tgz \
	&& docker -v


RUN mkdir -p /compose /app
WORKDIR /app
COPY . /app
ENTRYPOINT ["/app/waitfordocker.sh"]
CMD ["/app/load-images-and-compose.sh"]
