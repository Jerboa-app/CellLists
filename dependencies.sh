#!/bin/sh
cd include
for DEP in SFML-2.5.1 TGUI-0.9.1
do
  if [ -d $DEP ];
  then
    echo "Directory $DEP exists, overwrite?"
    read -p "[y/n]: " I
    if [ "$I" = "y" ];
    then
      rm -rf $DEP
      tar -xvf $DEP.tar.gz
    fi
  else
    tar -xvf $DEP.tar.gz
  fi

  if [ -d $DEP ];
  then
    echo "Building"
    cd $DEP
    if [ -d "build" ];
    then
      echo "Build dir already exists, overwrite?"
      read -p "[y/n]: " I
      if [ "$I" != "y" ];
      then
        cd ../
        pwd
        continue
      else
        rm -rf build
      fi
    fi
    mkdir build && cd build
    export SFML_DIR="../../SFML-2.5.1/build"
    cmake .. && make
    cd ../../
  fi
done
