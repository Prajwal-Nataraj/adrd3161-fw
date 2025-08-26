# Copyright (c) 2025 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_add_dependencies(offsets_h)
zephyr_library_sources(tmc9660_spi.c)
