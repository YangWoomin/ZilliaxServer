
NAME.PROJ				= mq_test_producer
DIR.INPUT.PROJ			= .
DIR.INPUT.BUILD			= ../build
DIR.OUTPUT.PROJ.BIN		= ../output/bin
DIR.OUTPUT.PROJ.LIB		= ../output/lib

default: build_debug

-include $(DIR.INPUT.BUILD)/build.mk

.PHONY	+= all configure reconfigure \
			build_debug clean_debug rebuild_debug \
			build_release clean_release rebuild_release \
			distclean default

configure: .configure


distclean: .distclean


reconfigure: .reconfigure


build_debug: .build_debug


clean_debug: .clean_debug


rebuild_debug: .rebuild_debug


build_release: .build_release


clean_release: .clean_release


rebuild_release: .rebuild_release




