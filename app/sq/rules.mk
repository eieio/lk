LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

#INCLUDES += -I$(LOCAL_DIR)/include

MODULES += \
	lib/squirrel \
	lib/agg \

MODULE_SRCS += \
	$(LOCAL_DIR)/main.c \
	$(LOCAL_DIR)/gfx.cpp \

include make/module.mk

