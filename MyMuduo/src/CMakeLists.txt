# 定义了SRC_LIST变量，包含当前目录下所有的源文件
aux_source_directory(. SRC_LIST)

# 编译动态库
add_library(mymuduo SHARED ${SRC_LIST})