LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := cheat
LOCAL_SRC_FILES := cheat.cpp \
                   imgui/imgui.cpp \
                   imgui/imgui_draw.cpp \
                   imgui/imgui_widgets.cpp \
                   imgui/imgui_tables.cpp \
                   imgui/imgui_impl_android.cpp \
                   imgui/imgui_impl_opengl3.cpp
LOCAL_CFLAGS    := -fexceptions -frtti -std=c++11
LOCAL_CPPFLAGS  := -fexceptions -frtti -std=c++11
LOCAL_LDLIBS    := -llog -landroid -lGLESv2 -lEGL -pthread
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/imgui
LOCAL_SHARED_LIBRARIES := EGL GLESv2
include $(BUILD_SHARED_LIBRARY)
