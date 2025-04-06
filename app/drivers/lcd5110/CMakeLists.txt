# SPDX-License-Identifier: Apache-2.0

zephyr_library_named(display__lcd5110)
# zephyr_library_sources_ifdef(CONFIG_LCD5110 display_lcd5110.c)
zephyr_library_sources(display_lcd5110.c)


add_dependencies(display__lcd5110 offsets_h)
