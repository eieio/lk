LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

INCLUDES += -I$(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/sqapi.cpp \
	$(LOCAL_DIR)/sqbaselib.cpp \
	$(LOCAL_DIR)/sqfuncstate.cpp \
	$(LOCAL_DIR)/sqdebug.cpp \
	$(LOCAL_DIR)/sqlexer.cpp \
	$(LOCAL_DIR)/sqobject.cpp \
	$(LOCAL_DIR)/sqcompiler.cpp \
	$(LOCAL_DIR)/sqstate.cpp \
	$(LOCAL_DIR)/sqtable.cpp \
	$(LOCAL_DIR)/sqmem.cpp \
	$(LOCAL_DIR)/sqvm.cpp \
	$(LOCAL_DIR)/sqclass.cpp \
	$(LOCAL_DIR)/sqstdlib/sqstdblob.cpp \
	$(LOCAL_DIR)/sqstdlib/sqstdaux.cpp \
	$(LOCAL_DIR)/sqstdlib/sqstdmath.cpp \
	$(LOCAL_DIR)/sqstdlib/sqstdstring.cpp \
	$(LOCAL_DIR)/sqstdlib/sqstdrex.cpp \
	$(LOCAL_DIR)/sqstdlib/sqstdstream.cpp \
	$(LOCAL_DIR)/sqstdlib/sqstdio.cpp \

include make/module.mk
