#!/bin/bash

echo export LD_PRELOAD="$(basename ${0})/crystalhd_wrapper.so"
