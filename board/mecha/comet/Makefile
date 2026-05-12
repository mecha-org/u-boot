#
# Copyright 2019 NXP
#
# SPDX-License-Identifier:      GPL-2.0+
#

obj-y += comet.o
obj-spl-y += comet.o

ifdef CONFIG_XPL_BUILD	
obj-y += spl.o
obj-$(CONFIG_IMX8M_LPDDR4_2GB) += lpddr4_timing_2gb.o
obj-$(CONFIG_IMX8M_LPDDR4_4GB) += lpddr4_timing_4gb.o
obj-$(CONFIG_IMX8M_LPDDR4_8GB) += lpddr4_timing_8gb.o
endif
