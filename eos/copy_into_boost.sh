#! /bin/sh

if [ -z $1 ]
then
   echo "Usage: ./copy_back_from_boost.sh BOOST_DIR"
   exit 1
fi

mkdir -v $1/boost/archive/eos/
cp -v *.hpp $1/boost/archive/eos/
cp -v *.cpp $1/libs/serialization/src/portable_archive_dll.cpp
