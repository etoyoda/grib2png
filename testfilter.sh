#!/bin/bash
set -Ceuo pipefail

test -f sample.gsm || bash prepsample.sh

