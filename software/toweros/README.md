
# TowerOS

TowerOS is a customizable, secure OCI container runtime for OpenCellular built with LinuxKit from the [Moby project](https://mobyproject.org) (Apache License 2). The utilities here allow you to create an minimal immutable host boot image that spawns system and user applications in containers.

System containers provide additional namespacing and security and are statically loaded in the immutable boot image which can support dm-verity. User applications are loaded by a system Docker container which writes into a userdata partition. This makes it easy and secure to deploy custom workloads on OC, without affecting the underlying host. A central NOC can also be used to orchestrate and cache images.

# What

## Host Image 
Containers compose everything in this toolkit. An immutable host image is built with LinuxKit using a [specification](https://github.com/linuxkit/linuxkit/blob/master/docs/yaml.md) to create the static runtime. A minimal hardened `kernel` and `init` are assembled into a root filesystem, one-off `onboot` containers are executed with `runc`, and system `services` are spawned with containerd. In this toolkit these containers are defined in the files:

- `boot/oc-minimal.yml`: A minimal kernel + init from Linuxkit using an Alpine Linux base layer. Hardened for security. With patches to the init to enable containerd restart watchdog for services (to be upstreamed). Includes basic system services like SSH, NTP, DHCP. This also includes critical system services like OCWare.
- `boot/docker.yml`: System Docker + Docker compose service with device access. For demo this statically loads a `docker-compose.yml` from the host `/etc/compose` directory, but it can use a remote orchestrator.
- `boot/box.yml`: Anything related to the box hardware. Right now this mounts the userdata disk.
- `boot/vm.yml`: Anything related to running the image with QEMU/VBox.

The images can be protected with dm-verity (WIP) for integration with Verified boot. LinuxKit can easily generate updated images when host images are in need of upgrade and works with iPXE boot. Anyone can create bootable image safely, as LinuxKit uses Docker Content Trust to verify the origin of each container. If a central NOC hosts a mirror to fetch security updates, it only needs to fetch the modified container layers, and can compile new boot images for devices locally.

## User container runtime
Docker is loaded into the host image and started within containerd with a persisted volume mount at `/var/lib/docker`. While several orchestration solutions can be used, for demonstration a [Docker Compose](https://docs.docker.com/compose/) template is statically defined and the boot image at `/etc/compose/docker-compose.yml`. It dynamically pulls and verifies specified user containers from Docker Registry and can enforce security policies from [AppArmor](https://docs.docker.com/engine/security/apparmor/) or [Seccomp](https://docs.docker.com/engine/security/seccomp/). The example includes:

- `boot/compose/osmo-nitb`: The old style NITB from Osmocom with `osmo-nitb`, `osmo-bts`, and a patched `osmo-trx` for OC-SDR connected to the host network. 


## Why use LinuxKit to build a host OS?

Using LinuxKit to build the host OS fits the OpenCellular needs. LinuxKit provides:

-   Batteries included but optional: Only essential services can be packed into the host keep the attack surface small, and host updates infrequent and blazing fast.
-   Secure, immutable: Let Alpine Linux handle secure base layers. Verifiable immutable system images. Write operations from the guest to the file system are not persistent.
-   Easy tooling with easy iteration: Develop, test and sign releases from trusted vendor registries. Only pull updates for modified container layers. Orchestration can enable load balancing and downtime minimization, and online upgrades. 
-   A general purpose toolkit.


# Build

## Requirements
- Make
- Docker CE 17.05+ https://docs.docker.com/install/
- GoLang 1.7+ https://golang.org/dl/
- LinuxKit `go get -u github.com/linuxkit/linuxkit/src/cmd/linuxkit`


## Get your containers

We need to build the individual containers for both the host and the Docker compose runtime. These are under the `pkg/` directory. These will hopefully be upstreamed and maintained by their respective organizations. To build run in your shell from the toweros directory:

`$ make pkg`

### Tag and push user containers
The packages pulled using Docker compose need to be uploaded to a registry so they can be pulled. For demo we are using [Docker Hub](https://hub.docker.com/). Assuming you are signed up there with `$USERNAME`

This will retag the osmocom builds with your username and push to your account until we get them upstream

`$ export USERNAME=<you>`

`$ docker login $USERNAME`

`$ make -C pkg/osmocom all push`

## Assemble Boot image
Small Makefile wrapper around Linuxkit to make bootable images. Right now this is ISO+BIOS but there is support for squashfs as well.
Again we must set the `ORG` for the `/etc/compose/docker-compose.yml` to match what you pushed to Docker Hub earlier. 
Your SSH key will be copied from `~/.ssh/id_rsa.pub` into the base image.

`$ make boot`

# Run
After building you should have a `box.iso` and a `vm.iso` the key difference in being how the persistent disk volume is mounted. 

## VM Image
Mount the `vm.iso` as a CD drive into Virtualbox and the entire disk will be used as persistent storage for Docker under `/var/lib/docker`

## Box Image
Use `dd` to burn the `box.iso` to the first partition of your box's disk `sda`. The second partition `sda2` should created for persistent storage.

`dd if=box.iso bs=1M status=progress of=/dev/sda`
`fdisk /dev/sda` # Create a second partition
`mkfs.ext4 /dev/sda2`

## Sanity
If everything is alright you should be able to SSH into your box and view the containers managed by `containerd`

```
(ns: sshd) linuxkit-102030405030:~# ctr -n services.linuxkit t ls
TASK       PID     STATUS    
sshd       894     RUNNING
compose    2645    RUNNING
dhcpcd     660     RUNNING
docker     714     RUNNING
ntpd       792     RUNNING
rngd       842     RUNNING
```
You can also verify Docker has pulled the containers from the registry
```
(ns: sshd) linuxkit-102030405030:~# ctr -n services.linuxkit t exec --exec-id 2 docker docker ps
CONTAINER ID        IMAGE                       COMMAND                  CREATED             STATUS              PORTS               NAMES
0bf5d5a44428        oramadan/osmo-trx:latest    "osmo-trx"               8 hours ago         Up 8 hours                              compose_osmo-trx_1
acec0b7cc8a1        oramadan/osmo-nitb:latest   "osmo-nitb -c /data/…"   8 hours ago         Up 8 hours                              compose_osmo-nitb_1
1ba1fb815d13        oramadan/osmo-bts:latest    "osmo-bts-trx -c osm…"   8 hours ago         Up 8 hours                              compose_osmo-bts_1
```
