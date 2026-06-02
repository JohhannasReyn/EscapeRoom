#!/usr/bin/env bash
set -euo pipefail

echo "Active NetworkManager connections:"
nmcli connection show --active

echo
echo "Ethernet address, used for SSH/internet:"
ip -4 addr show eth0 || true

echo
echo "EscapeRoom hotspot address, used by Picos:"
ip -4 addr show wlan0 || true

echo
echo "Picos associated to the EscapeRoom hotspot:"
sudo iw dev wlan0 station dump || true

echo
echo "Neighbor table for wlan0:"
ip neigh show dev wlan0 || true

echo
echo "NetworkManager DHCP lease files, if present:"
lease_found=0
for lease in /var/lib/NetworkManager/dnsmasq-*.leases /var/lib/misc/dnsmasq.leases; do
    if [ -f "${lease}" ]; then
        lease_found=1
        echo "--- ${lease}"
        sudo cat "${lease}"
    fi
done

if [ "${lease_found}" -eq 0 ]; then
    echo "No dnsmasq lease file found. This can be normal on some NetworkManager versions."
fi
