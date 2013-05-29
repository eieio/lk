LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

INCLUDES += -I$(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/main.c \
	$(LOCAL_DIR)/ui.cpp \ 

MODULES += \
	lib/gfxconsole \
	lib/libm \
	lib/agg \

WITH_CPP_SUPPORT := true

include make/module.mk
