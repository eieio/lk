LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

#INCLUDES += -I$(LOCAL_DIR)/include

MODULES += \
	lib/squirrel \

MODULE_SRCS += \
	$(LOCAL_DIR)/main.c 

include make/module.mk
