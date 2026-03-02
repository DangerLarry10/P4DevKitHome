# Main component CMakeLists.txt
# ---
# In ESP-IDF, each folder under main/ or components/ is a "component."
# This file tells the build system which .cpp files to compile and where
# to find header files.
#
# SRCS = source files to compile (every .cpp we write goes here)
# INCLUDE_DIRS = folders where #include looks for headers

idf_component_register(
    SRCS
        "main.cpp"
        "home/home_screen.cpp"
        "home/app_manager.cpp"
        "home/status_bar.cpp"
        "home/app_list.cpp"
        "apps/calculator/calculator_app.cpp"
        "apps/drawing/drawing_app.cpp"
    INCLUDE_DIRS
        "."
        "home"
        "apps"
        "apps/calculator"
        "apps/drawing"
)

# Suppress warnings about missing field initializers in designated initializers.
# This is common in ESP-IDF code where you only set a few fields of a config struct.
target_compile_options(${COMPONENT_LIB} PUBLIC -Wno-missing-field-initializers)
