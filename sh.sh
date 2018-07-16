#!/bin/bash
set -eux

if [[ "$(dirname "$0")" = "/workspace" ]]; then
  echo "executed on google container builder"
else
  echo "executed on local"
fi

echo ok
