version: '2.3'
services:
  osmo-trx:
    image: $ORG/osmo-trx:latest
    privileged: true
    device_cgroup_rules:
      - 'a *:* rwm'
    cap_add:
      - ALL
    restart: always
    network_mode: "host"
  osmo-bts:
    image: $ORG/osmo-bts:latest
    volumes:
      - /data:/data
    restart: always
    network_mode: "host"
  osmo-nitb:
    image: $ORG/osmo-nitb:latest
    volumes:
      - /data:/data
      - state:/var/state
    restart: always
    network_mode: "host"
volumes:
  state:
    driver: "local"
