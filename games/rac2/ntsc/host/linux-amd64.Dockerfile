# RAC2 helper image, linux/amd64 (Rosetta on Apple Silicon).
# Plays the role WSL plays on Windows: runs Wrench (64-bit Linux), builds the
# patched GNU EE 2.9-ee-991111b chain with gcc -m32, and runs that chain's
# 32-bit cpp/cc1/as. 32-bit Linux binaries run fine here; 32-bit Wine does not.
FROM --platform=linux/amd64 ubuntu:24.04
RUN apt-get update \
 && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
      python3 python3-venv ca-certificates curl unzip xz-utils file \
      gcc gcc-multilib make flex git patch \
 && rm -rf /var/lib/apt/lists/*
