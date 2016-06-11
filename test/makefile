# 此文件由makeobj.sh自动生成
############################################################
# 项目名 : 
# 模块名 : 
# 备  注 : 
############################################################

###### 源文件配置区
#@ c_FILE
c_FILE		=	\
			test.c \
			test2.c \
			test3.c \
			test4.c \
			test5.c \
			press.c \

###### 目标文件、安装目录配置区
include makeinstall
BIN		=	test \
			test2 \
			test3 \
			test4 \
			test5 \
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
test		:	test.o
	$(CC) -o $@ test.o $(LFLAGS)

test2		:	test2.o
	$(CC) -o $@ test2.o $(LFLAGS)

test3		:	test3.o
	$(CC) -o $@ test3.o $(LFLAGS)

test4		:	test4.o
	$(CC) -o $@ test4.o $(LFLAGS)

test5		:	test5.o
	$(CC) -o $@ test5.o $(LFLAGS)

press		:	press.o
	$(CC) -o $@ press.o $(LFLAGS)

