# 此文件由makeobj.sh自动生成
############################################################
# 项目名 : 
# 模块名 : 
# 备  注 : 
############################################################

###### 源文件配置区
#@ c_FILE
c_FILE		=	\
			test_demo.c \
			test_success.c \
			test_failure.c \
			test_client_block.c \
			test_client_nonblock.c \
			test_client_nonblock_slow.c \
			test_server_block.c \
			test_server_nonblock.c \
			test_server_nonblock_slow.c \
			test_client_gzip.c \
			test_server_gzip.c \
			press.c \

###### 目标文件、安装目录配置区
include makeinstall
BIN		=	test_demo \
			test_success \
			test_failure \
			test_client_block \
			test_client_nonblock \
			test_client_nonblock_slow \
			test_server_block \
			test_server_nonblock \
			test_server_nonblock_slow \
			test_client_gzip \
			test_server_gzip \
			press \

###### 编译选项
CFLAGS		=	$(_CFLAGS) \

###### 链接选项
LFLAGS		=	$(_LFLAGS) \
			-lfasterhttp \

###### 额外宏定义区
CLEAN_ADDITION	=

###### 加载mktpl模板库
#@ make_all
#@ make_clean
#@ make_install
#@ make_install_BININST
#@ make_install_DFTHDERINST
#@ make_uninstall
#@ make_uninstall_BININST
#@ make_uninstall_DFTHDERINST
include $(MKTPLDIR)/makeobj_$(MKTPLOS).inc

###### 目标文件依赖关系
test_demo		:	test_demo.o
	$(CC) -o $@ test_demo.o $(LFLAGS)

test_success		:	test_success.o
	$(CC) -o $@ test_success.o $(LFLAGS)

test_failure		:	test_failure.o
	$(CC) -o $@ test_failure.o $(LFLAGS)

test_client_block	:	test_client_block.o
	$(CC) -o $@ test_client_block.o $(LFLAGS)

test_client_nonblock	:	test_client_nonblock.o
	$(CC) -o $@ test_client_nonblock.o $(LFLAGS)

test_client_nonblock_slow:	test_client_nonblock_slow.o
	$(CC) -o $@ test_client_nonblock_slow.o $(LFLAGS)

test_server_block	:	test_server_block.o
	$(CC) -o $@ test_server_block.o $(LFLAGS)

test_server_nonblock	:	test_server_nonblock.o
	$(CC) -o $@ test_server_nonblock.o $(LFLAGS)

test_server_nonblock_slow:	test_server_nonblock_slow.o
	$(CC) -o $@ test_server_nonblock_slow.o $(LFLAGS)

test_client_gzip	:	test_client_gzip.o
	$(CC) -o $@ test_client_gzip.o $(LFLAGS)

test_server_gzip	:	test_server_gzip.o
	$(CC) -o $@ test_server_gzip.o $(LFLAGS)

press			:	press.o
	$(CC) -o $@ press.o $(LFLAGS)

