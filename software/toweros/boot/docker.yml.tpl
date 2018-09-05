services:
  - name: docker
    image: docker:18.06.0-ce-dind
    capabilities:
     - all
    net: host
    mounts:
     - type: cgroup
       options: ["rw","nosuid","noexec","nodev","relatime"]
    resources:
        devices:
                - allow: true
                  access: "rwm"
    binds:
     - /etc/resolv.conf:/etc/resolv.conf
     - /var/lib/docker:/var/lib/docker
     - /lib/modules:/lib/modules
     - /etc/docker/daemon.json:/etc/docker/daemon.json
     - /dev/bus:/dev/bus
     - /var/run:/var/run
     - /data:/data
    command: ["/usr/local/bin/docker-init", "/usr/local/bin/dockerd"]
  - name: compose
    image: ${COMPOSE_BUILD}
    binds:
     - /var/run:/var/run
     - /etc/compose:/compose
files:
  - path: var/lib/docker
    directory: true
    mode: "0777"
  - path: etc/docker/daemon.json
    contents: '{"debug": true}'
    mode: "0660"
