#!/usr/bin/env bash
set -euo pipefail

APP_DIR="/home/gaspard/softadastra/cloud"
VIX="/home/gaspard/.local/bin/vix"

cd "$APP_DIR"

echo "==> Pull latest code"
git fetch origin main
git reset --hard origin/main

echo "==> Install dependencies"
"$VIX" install

echo "==> Build Softadastra Cloud"
"$VIX" build --preset release

echo "==> Restart service"
"$VIX" service restart

echo "==> Check service"
"$VIX" service status

echo "==> Waiting for local HTTP health"
healthy=false

for i in {1..20}; do
  if curl -fsS http://127.0.0.1:8080/health >/dev/null; then
    echo "Softadastra Cloud local health is ready"
    healthy=true
    break
  fi

  echo "Waiting for Softadastra Cloud... attempt $i/20"
  sleep 1
done

if [ "$healthy" != "true" ]; then
  echo "Local health check failed"
  "$VIX" logs --errors --lines 100 || true
  exit 1
fi

echo "==> Public HTTPS health"
curl -fsS https://api.softadastra.com/health >/dev/null

echo "==> WebSocket health"
"$VIX" ws check ws://127.0.0.1:9090/ws

echo "==> Softadastra Cloud deployed successfully"
