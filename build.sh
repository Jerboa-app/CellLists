#!/bin/sh
for file in CMakeFiles cmake_install.cmake CMakeCache.txt Makefile Jerboa
do
  if [ -d $file ];
  then
    rm -rf $file
  fi
  if [ -f $file ];
  then
    rm $file
  fi
done

if [ -z "${SFML_DIR}" ];
then
  export SFML_DIR=include/SFML-2.5.1/build
fi
cmake . -D SFML_DIR=$SFML_DIR && make
