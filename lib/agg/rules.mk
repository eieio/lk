LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULES += \
	lib/libm \

GLOBAL_INCLUDES += $(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/agg_embedded_raster_fonts.cpp \
	$(LOCAL_DIR)/agg_vcgen_stroke.cpp \
	$(LOCAL_DIR)/agg_rounded_rect.cpp \
	$(LOCAL_DIR)/agg_arc.cpp \

include make/module.mk
