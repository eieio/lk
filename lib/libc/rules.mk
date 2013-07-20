LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/atoi.c \
	$(LOCAL_DIR)/ctype.c \
	$(LOCAL_DIR)/printf.c \
	$(LOCAL_DIR)/malloc.c \
	$(LOCAL_DIR)/rand.c \
	$(LOCAL_DIR)/stdio.c \
	$(LOCAL_DIR)/eabi.c

ifeq ($(WITH_CPP_SUPPORT),true)
MODULE_SRCS += \
	$(LOCAL_DIR)/new.cpp \
	$(LOCAL_DIR)/atexit.c \
	$(LOCAL_DIR)/pure_virtual.cpp
endif

include $(LOCAL_DIR)/string/rules.mk

include make/module.mk
