#!/usr/bin/env bash
# One-command demo: install deps, build, test, and regenerate portfolio figures.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

echo "==> Installing Python dependencies"
python3 -m pip install -q -r requirements.txt

echo "==> Building solver and running unit tests"
make test

echo "==> Generating figures and animation"
make figures

echo
echo "Demo complete. Artifacts:"
for f in \
  docs/assets/advection_profiles.png \
  docs/assets/convergence.png \
  docs/assets/scheme_comparison.png \
  docs/assets/advection_animation.gif
do
  if [[ -f "$f" ]]; then
    size="$(wc -c < "$f" | tr -d ' ')"
    echo "  - $f  (${size} bytes)"
  else
    echo "  - $f  (missing)"
  fi
done
