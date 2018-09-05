kernel:
  image: linuxkit/kernel:4.14.67
  cmdline: "console=tty0 console=ttyS0 console=ttyAMA0 console=ttysclp0 root=/dev/sda1" # page_poison=1
init:
  - ${INIT_BUILD}
  - linuxkit/runc:v0.6
  - ${CONTAINERD_BUILD}
  - linuxkit/ca-certificates:v0.6
  - linuxkit/getty:v0.6
onboot:
  - name: sysctl
    image: linuxkit/sysctl:v0.6
  - name: sysfs
    image: linuxkit/sysfs:v0.6
  - name: rngd1
    image: linuxkit/rngd:v0.6
    command: ["/sbin/rngd", "-1"]
services:
  - name: ntpd
    image: linuxkit/openntpd:v0.6
  - name: rngd
    image: linuxkit/rngd:v0.6
  - name: sshd
    image: linuxkit/sshd:v0.6
  - name: dhcpcd
    image: linuxkit/dhcpcd:v0.6
files:
  - path: root/.ssh/authorized_keys
    source: ~/.ssh/id_rsa.pub
    mode: "0600"
trust:
  org:
    - linuxkit
