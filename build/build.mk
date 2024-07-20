
###########################################################
# check predefined variables

ifndef NAME.PROJ
$(error 'NAME.PROJ should be defined')
endif

ifndef DIR.INPUT.PROJ
$(error 'DIR.INPUT.PROJ should be defined')
endif

ifndef DIR.INPUT.BUILD
$(error 'DIR.INPUT.BUILD should be defined')
endif

ifndef DIR.OUTPUT.PROJ.BIN
$(error 'DIR.OUTPUT.PROJ.BIN should be defined')
endif

ifndef DIR.OUTPUT.PROJ.LIB
$(error 'DIR.OUTPUT.PROJ.LIB should be defined')
endif


###########################################################
# declare common variables

# platform
ifeq ($(OS), Windows_NT)
	PLATFORM := windows
else
	UNAME := $(shell uname -s)
	ifeq ($(UNAME),Linux)
		PLATFORM := linux
	else
$(error '$(UNAME) is not supported')
	endif
endif

# config
CONFIG.DEBUG = debug
CONFIG.RELEASE = release
ifneq ($(findstring $(CONFIG.DEBUG),$(MAKECMDGOALS)),)
	CONFIG = $(CONFIG.DEBUG)
else
	CONFIG = $(CONFIG.RELEASE)
endif

# make & cmake files
FILE.MAKEFILE = Makefile
FILE.CMAKEFILE = CMakeLists.txt
FILE.INPUT.MAKEFILES = $(MAKEFILE_LIST)
FILE.INPUT.CMAKEFILES = $(wildcard $(DIR.INPUT.PROJ)/$(FILE.CMAKEFILE)) $(wildcard $(DIR.INPUT.PROJ)/src/$(FILE.CMAKEFILE)) $(wildcard $(DIR.INPUT.BUILD)/*.cmake)
DIR.CMAKEFILES = CMakeFiles
FILE.CMAKECACHE = CMakeCache.txt

# generated directories & files
ifeq ($(PLATFORM),windows)
	DIR.GEN.PROJ.BUILD = $(DIR.INPUT.PROJ)/.msvc
else
	DIR.GEN.PROJ.BUILD = $(DIR.INPUT.PROJ)/.make
	DIR.GEN.PROJ.BUILD.DEBUG = $(DIR.GEN.PROJ.BUILD)/$(CONFIG.DEBUG)
	DIR.GEN.PROJ.BUILD.RELEASE = $(DIR.GEN.PROJ.BUILD)/$(CONFIG.RELEASE)
endif

# output files
DIRS.OUTPUT.PROJ = $(DIR.OUTPUT.PROJ.BIN) $(DIR.OUTPUT.PROJ.LIB)
ifeq ($(PLATFORM),windows)
	FILE.OUTPUT.PREFIXES = 
	FILE.OUTPUT.EXTENSIONS = dll lib exp exe pdb
else
	FILE.OUTPUT.PREFIXES = lib
	FILE.OUTPUT.EXTENSIONS = so a out
endif
FILE.DEBUG_POSTFIX = d
FILES.OUTPUT.PROJ.DEBUG = $(foreach dir, $(DIRS.OUTPUT.PROJ), \
							$(foreach ext, $(FILE.OUTPUT.EXTENSIONS), \
								$(dir)/$(NAME.PROJ)$(FILE.DEBUG_POSTFIX).$(ext) \
								$(foreach prefix, $(FILE.OUTPUT.PREFIXES), \
									$(dir)/$(prefix)$(NAME.PROJ)$(FILE.DEBUG_POSTFIX).$(ext) \
								) \
							) \
						)
FILES.OUTPUT.PROJ.RELEASE = $(foreach dir, $(DIRS.OUTPUT.PROJ), \
								$(foreach ext, $(FILE.OUTPUT.EXTENSIONS), \
									$(dir)/$(NAME.PROJ).$(ext) \
									$(foreach prefix, $(FILE.OUTPUT.PREFIXES), \
										$(dir)/$(prefix)$(NAME.PROJ).$(ext) \
									) \
								) \
							)


###########################################################
# define commands

ifeq ($(PLATFORM),windows)

RF		:= @rm.exe -f
MD		:= @mkdir.exe -p
RD		:= @rm.exe -rf
CP		:= @cp.exe -rfv
ECHO	:= @echo
MAKE	:= make.exe
CMAKE	:= cmake.exe

else

RF		:= rm -f
MD		:= mkdir -p
RD		:= rm -rf
CP		:= cp -rfv
ECHO	:= @echo
MAKE	:= make
CMAKE	:= cmake

endif


###########################################################
# targets

.PHONY: .configure .reconfigure \
		.build_debug .clean_debug .rebuild_debug \
		.build_release .clean_release .rebuild_release \
		.distclean .test

ifeq ($(PLATFORM),windows)

.configure: $(DIR.GEN.PROJ.BUILD)/$(FILE.CMAKECACHE)


$(DIR.GEN.PROJ.BUILD)/$(FILE.CMAKECACHE): $(FILE.INPUT.CMAKEFILES)
	$(CMAKE) -S $(DIR.INPUT.PROJ) -B $(DIR.GEN.PROJ.BUILD) -G "Visual Studio 17 2022" -A x64 -DCMAKE_CONFIGURATION_TYPES:STRING='debug;release'

.build_debug: $(DIR.GEN.PROJ.BUILD)/$(NAME.PROJ).sln


.clean_debug:
	$(RD) $(DIR.GEN.PROJ.BUILD)/$(DIR.CMAKEFILES)
	$(RF) $(DIR.GEN.PROJ.BUILD)/$(FILE.CMAKECACHE)
	$(RF) $(FILES.OUTPUT.PROJ.DEBUG)

.build_release: $(DIR.GEN.PROJ.BUILD)/$(NAME.PROJ).sln


.clean_release:
	$(RD) $(DIR.GEN.PROJ.BUILD)/$(DIR.CMAKEFILES)
	$(RF) $(DIR.GEN.PROJ.BUILD)/$(FILE.CMAKECACHE)
	$(RF) $(FILES.OUTPUT.PROJ.RELEASE)

$(DIR.GEN.PROJ.BUILD)/$(NAME.PROJ).sln: configure
	msbuild.exe $@ -p:Configuration=$(CONFIG)

else # linux

FILES.GEN.MAKEFILE.DEBUG = $(DIR.GEN.PROJ.BUILD.DEBUG)/$(FILE.MAKEFILE)
FILES.GEN.MAKEFILE.RELEASE = $(DIR.GEN.PROJ.BUILD.RELEASE)/$(FILE.MAKEFILE)

.configure: .configure_debug .configure_release


.configure_debug: $(DIR.GEN.PROJ.BUILD.DEBUG) $(FILES.GEN.MAKEFILE.DEBUG)


$(DIR.GEN.PROJ.BUILD.DEBUG):
	$(MD) $@

$(FILES.GEN.MAKEFILE.DEBUG): $(FILE.INPUT.MAKEFILES) $(FILE.INPUT.CMAKEFILES)
	$(CMAKE) -S $(DIR.INPUT.PROJ) -B $(DIR.GEN.PROJ.BUILD.DEBUG) -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=debug

.configure_release: $(DIR.GEN.PROJ.BUILD.RELEASE) $(FILES.GEN.MAKEFILE.RELEASE)


$(DIR.GEN.PROJ.BUILD.RELEASE):
	$(MD) $@

$(FILES.GEN.MAKEFILE.RELEASE): $(FILE.INPUT.MAKEFILES) $(FILE.INPUT.CMAKEFILES)
	$(CMAKE) -S $(DIR.INPUT.PROJ) -B $(DIR.GEN.PROJ.BUILD.RELEASE) -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=release

.build_debug: .configure_debug
	$(MAKE) -C $(DIR.GEN.PROJ.BUILD.DEBUG) all

.clean_debug:
#	$(MAKE) -C $(DIR.GEN.PROJ.BUILD.DEBUG) clean
	$(RD) $(DIR.GEN.PROJ.BUILD.DEBUG)/$(DIR.CMAKEFILES)
	$(RF) $(DIR.GEN.PROJ.BUILD.DEBUG)/$(FILE.CMAKECACHE)
	$(RF) $(FILES.OUTPUT.PROJ.DEBUG)

.build_release: .configure_release
	$(MAKE) -C $(DIR.GEN.PROJ.BUILD.RELEASE) all

.clean_release:
#	$(MAKE) -C $(DIR.GEN.PROJ.BUILD.RELEASE) clean
	$(RD) $(DIR.GEN.PROJ.BUILD.RELEASE)/$(DIR.CMAKEFILES)
	$(RF) $(DIR.GEN.PROJ.BUILD.RELEASE)/$(FILE.CMAKECACHE)
	$(RF) $(FILES.OUTPUT.PROJ.RELEASE)

endif

.reconfigure: .distclean .configure


.distclean:
	$(RD) $(DIR.GEN.PROJ.BUILD)
	$(RF) $(FILES.OUTPUT.PROJ.DEBUG)
	$(RF) $(FILES.OUTPUT.PROJ.RELEASE)

.rebuild_debug: .clean_debug .build_debug


.rebuild_release: .clean_release .build_release


$(DIR.GEN.PROJ.BUILD):
	$(MD) $@


