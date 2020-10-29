#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd $DIR/..

rm ./python/lamppost/*.so
# Collect The Core file
find -name *_pb2_grpc.py -not -path "./python/*" | xargs -i cp -rf {} ./python/lamppost
find -name *_pb2.py  -not -path "./python/*" | xargs -i cp -rf {} ./python/lamppost
find ./build -name *cpython*so  -not -path "./python/*" | xargs -i cp -rf {} ./python/lamppost

sed -i -E 's/^import.*_pb2/from . \0/' ./python/lamppost/*_pb2_grpc.py
sed -i -E 's/^import.*_pb2/from . \0/' ./python/lamppost/*_pb2.py
# Change the link of the cpython so to relative path
# And colelcting it dependency


