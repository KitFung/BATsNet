#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd $DIR/..

# Collect The Core file
find -name *_pb2_grpc.py | xargs -i cp -rf {} ./python/lamppost
find -name *_pb2.py | xargs -i cp -rf {} ./python/lamppost
find ./build -name *cpython*so | xargs -i cp -rf {} ./python/lamppost

# Change the link of the cpython so to relative path
# And colelcting it dependency


