#! /bin/sh

if [ -z $1 ]
then
   echo "Usage: ./copy_back_from_boost.sh BOOST_DIR"
   exit 1
fi

cp -v $1/boost/archive/eos/* .
cp -v $1/libs/serialization/src/portable_archive_dll.cpp .
