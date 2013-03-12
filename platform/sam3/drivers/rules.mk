LOCAL_DIR := $(GET_LOCAL_DIR)

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)

DEFINES += \
	PASS=0 \
	FAIL=1 \

MODULE_SRCS += \
	$(LOCAL_DIR)/pio/pio.c \
	$(LOCAL_DIR)/pmc/pmc.c \
	$(LOCAL_DIR)/tc/tc.c \
	$(LOCAL_DIR)/uart/uart.c \
	$(LOCAL_DIR)/wdt/wdt.c \
	$(LOCAL_DIR)/spi/spi.c \
	$(LOCAL_DIR)/twi/twi.c \

