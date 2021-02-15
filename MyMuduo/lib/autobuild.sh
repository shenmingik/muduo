#!/bin/bash

set -e

cd ..

# 如果没有build目录，创建该目录
if [ ! -d `pwd`/build ]; then
    mkdir `pwd`/build
fi

rm -rf `pwd`/build/*

cd `pwd`/build &&
    cmake .. &&
    make

# 回到项目根目录
cd ..

# 把头文件拷贝到 /usr/include/mymuduo  so库拷贝到 /usr/lib    PATH
if [ ! -d /usr/include/mymuduo ]; then 
    mkdir /usr/include/mymuduo
fi

cd `pwd`/include

# 拷贝hpp文件
for header in `ls *.hpp`
do
    cp $header /usr/include/mymuduo
done

cd ..
cp `pwd`/lib/libmymuduo.so /usr/lib

ldconfig