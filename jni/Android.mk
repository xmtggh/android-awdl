LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := libc++:./libc++.so
include $(BUILD_MULTI_PREBUILT)
#java 现用版本
include $(CLEAR_VARS)
LOCAL_MODULE := p2pex
LOCAL_ARM_MODE := arm
nl_sources := $(sort $(wildcard $(LOCAL_PATH)/libnl/lib/*.c))
nl_sources := $(nl_sources:$(LOCAL_PATH)/libnl/lib/%=%)
link_sources := $(sort $(wildcard $(LOCAL_PATH)/libnl/lib/route/link/*.c))
link_sources := $(link_sources:$(LOCAL_PATH)/libnl/lib/route/link/%=%)
p2p_sources := $(sort $(wildcard $(LOCAL_PATH)/src/*.c))
p2p_sources := $(p2p_sources:$(LOCAL_PATH)/src/%=%)
LOCAL_SRC_FILES := \
				  $(nl_sources:%=libnl/lib/%) \
				  $(link_sources:%=libnl/lib/route/link/%) \
				  $(p2p_sources:%=src/%) \
				  libev/ev.c \
                  libs/libpcap/missing/strlcpy.c \
                  libs/libpcap/bpf_image.c \
                  libs/libpcap/bpf_dump.c \
                  libs/libpcap/bpf_filter.c \
                   libs/libpcap/etherent.c \
                   libs/libpcap/gencode.c \
                   libs/libpcap/nametoaddr.c \
                   libs/libpcap/optimize.c \
                   libs/libpcap/pcap.c \
                   libs/libpcap/pcap-linux.c \
                   libs/libpcap/savefile.c \
                   libs/libpcap/sf-pcap.c \
                   libs/libpcap/pcap-common.c \
                   libs/libpcap/pcap-usb-linux.c \
                   libs/libpcap/pcap-netfilter-linux-android.c \
                   libs/libpcap/fad-getad.c \
                   libs/libpcap/fmtutils.c \
                   libs/libpcap/sf-pcapng.c \
                   libs/libpcap/scanner.c \
                   libs/libpcap/grammar.c \
                   libs/libnl/lib/fib_lookup/lookup.c \
                    libs/libnl/lib/fib_lookup/request.c \
                    libs/libnl/lib/genl/ctrl.c \
                    libs/libnl/lib/genl/family.c \
                    libs/libnl/lib/genl/genl.c \
                    libs/libnl/lib/genl/mngt.c \
                    libs/libnl/lib/netfilter/nfnl.c \
                    libs/libnl/lib/route/nexthop_encap.c \
                    libs/libnl/lib/route/nh_encap_mpls.c \
                    libs/libnl/lib/route/route_utils.c \
                    libs/libnl/lib/route/route.c \
                    libs/libnl/lib/route/route_obj.c \
                    libs/libnl/lib/route/nexthop.c \
                    libs/libnl/lib/route/rtnl.c \
                    libs/libnl/lib/route/link.c \
                    libs/libnl/lib/route/neigh.c



LOCAL_CFLAGS := -O3 -Wall -DBUILD_STANDALONE -DCPU_ARM -finline-functions -fPIC
LOCAL_CFLAGS += -Wno-sequence-point -Wno-extra
LOCAL_CFLAGS += "-Wno-\#warnings" -Wno-constant-logical-operand -Wno-self-assign  -DSYSCONFDIR=\"/etc\" -D_ANDROID_ -DANDROID -DHAVE_CONFIG_H -D_BSD_SOURCE
LOCAL_CFLAGS +="-D_BSD_SOURCE" \
        "-Wall" \
        "-Wno-unused-parameter" \
        "-Wno-sign-compare" \
        "-Wno-missing-field-initializers" \
        "-Wno-tautological-compare" \
        "-Wno-pointer-arith" \
        "-UNDEBUG" \
        "-D_GNU_SOURCE"
LOCAL_JNI_SHARED_LIBRARIES :=libsmartphotojni
LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog

LOCAL_C_INCLUDES:= ${LOCAL_PATH}/include \
 				   ${LOCAL_PATH}/libev \
                   ${LOCAL_PATH}/libpcap \
                   ${LOCAL_PATH}/libnl/include

include $(BUILD_EXECUTABLE)





