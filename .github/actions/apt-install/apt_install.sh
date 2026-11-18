#!/usr/bin/env bash
# See action.yml. Reads PACKAGES and ARCHIVES (optional) from the environment.
set -euo pipefail

read -r -a packages <<< "$(echo "$PACKAGES" | tr '\n' ' ')"
apt=(sudo apt-get -o Acquire::Retries=3 -o Acquire::http::Timeout=30 -o Acquire::https::Timeout=30)
ARCHIVES="${ARCHIVES:-}"
ARCHIVES="${ARCHIVES/#\~/$HOME}"
if [ -n "$ARCHIVES" ]; then
  mkdir -p "$ARCHIVES/partial"
  apt+=(-o Dir::Cache::Archives="$ARCHIVES")
fi

drop_azure() {
  [ -f /etc/apt/apt-mirrors.txt ] || return 0
  sudo sed -i '/azure\.archive\.ubuntu\.com/d' /etc/apt/apt-mirrors.txt
  grep -q '^http' /etc/apt/apt-mirrors.txt \
    || echo "http://archive.ubuntu.com/ubuntu/" | sudo tee /etc/apt/apt-mirrors.txt > /dev/null
}

for try in 1 2 3; do
  # A failed index download is only a warning to apt-get update; the download below tells.
  timeout 180 "${apt[@]}" update || true
  if timeout 300 "${apt[@]}" install -y --no-install-recommends --download-only "${packages[@]}"; then
    break
  fi
  if [ "$try" = 3 ]; then
    echo "::error title=apt::the packages did not download after three tries"
    exit 1
  fi
  echo "::warning title=apt::try $try failed: once more without the Azure mirror"
  drop_azure
  sleep $((try * 15))
done
"${apt[@]}" install -y --no-install-recommends "${packages[@]}"
# The cache saves only the packages, readable by the runner (apt's partial folder is _apt's).
if [ -n "$ARCHIVES" ]; then
  sudo find "$ARCHIVES" -maxdepth 1 -name '*.deb' -exec chmod a+r {} +
fi
