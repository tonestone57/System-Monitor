## Haiku Generic Makefile ##

# The name of the binary
NAME = SystemMonitor

# The type of binary (APP, SHARED, STATIC, DRIVER)
TYPE = APP

# Application MIME type
APP_MIME_SIG = application/x-vnd.HaikuSysMonTask

# Source files
SRCS = \
	SysMonTaskApp.cpp \
	CPUView.cpp \
	MemView.cpp \
	DiskView.cpp \
	NetworkView.cpp \
	ProcessView.cpp \
	GPUView.cpp \
	SysInfoView.cpp

# Resource definition files
RDEFS = SysMonTask.rdef

# Resource files
# RSRCS =

# Libraries to link against
LIBS = be tracker columnlistview network bnetapi localestub $(STDCPPLIBS)

# Additional object files to link against
EXTRA_OBJS =

# Additional directories with source files
EXTRA_INCLUDES =

# Additional linker flags
LDFLAGS =

# Specify the level of optimization
OPTIMIZE := FULL

# Specify the level of localization
LOCALES =

# Specify any preprocessor symbols to be defined
DEFINES =

# Specify the warning level
WARNINGS = ALL

# Specify whether image symbols will be created
SYMBOLS :=

# Specify debug settings
DEBUGGER := FALSE

# Target-specific flags
TARGET_CFLAGS =-O2 -march=pentium -mtune=generic
TARGET_CPPFLAGS =-O2 -march=pentium -mtune=generic
TARGET_LDFLAGS =-s

## Include the Haiku makefile system rules
include $(BUILDHOME)/etc/makefile-engine
