#!/bin/bash

if [ $# -ne 1 ]; then
  echo "Usage: $0 <private_key_pem_file>"
  echo "Reads data to sign from stdin, encrypted data is output to stdout"
  exit 1
fi

openssl rsautl -sign -inkey $1
