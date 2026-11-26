# usermod/letu/micropython.cmake —— 编进固件的唯一配置
# 注意：不要自己 add_library(usermod)，MicroPython 的 py/usermod.cmake 已经建好了，
# 这里只建“我们的库”并挂到 usermod 上。
add_library(usermod_letu INTERFACE)
target_sources(usermod_letu INTERFACE ${CMAKE_CURRENT_LIST_DIR}/mod_letu.c)
target_link_libraries(usermod INTERFACE usermod_letu)
