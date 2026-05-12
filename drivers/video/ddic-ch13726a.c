#include <backlight.h>
#include <dm.h>
#include <mipi_dsi.h>
#include <panel.h>
#include <asm/gpio.h>
#include <dm/device_compat.h>
#include <linux/delay.h>
#include <power/regulator.h>

struct ch13726a_panel_priv {
        struct udevice *reg;
        struct udevice *backlight;
        struct gpio_desc reset;
};

static const struct display_timing default_timing = {
        .pixelclock.typ         = 59832000, // 59.832 MHz,
        .hactive.typ            = 1080,
        .hfront_porch.typ       = 80,
        .hback_porch.typ        = 80,
        .hsync_len.typ          = 80,
        .vactive.typ            = 1240,
        .vfront_porch.typ       = 90,
        .vback_porch.typ        = 90,
        .vsync_len.typ          = 90,
};

#if 0
static void ch13726a_dcs_write_cmd(struct udevice *dev, u8 cmd, u8 value)
{
        struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);
        struct mipi_dsi_device *device = plat->device;
        int err;

        err = mipi_dsi_dcs_write(device, cmd, &value, 1);
        if (err < 0)
                dev_err(dev, "MIPI DSI DCS write failed: %d\n", err);
}
#endif

static int ch13726a_dcs_write_buf(struct udevice *dev, const void *data,
                                  size_t len)
{
        struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);
        struct mipi_dsi_device *device = plat->device;
        int err;

        err = mipi_dsi_dcs_write_buffer(device, data, len);
        if (err < 0)
                dev_err(dev, "MIPI DSI DCS write buffer failed: %d\n", err);

	return err;
}

#define dcs_write_seq(dev, cmd, seq...)				\
({								\
	const u8 d[] = { cmd, seq };			\
								\
	ch13726a_dcs_write_buf(dev, d, ARRAY_SIZE(d));		\
})


static int ch13726a_init_sequence(struct udevice *dev)
{	
	dcs_write_seq(dev, MIPI_DCS_WRITE_CONTROL_DISPLAY, MIPI_DCS_NOP);
	dcs_write_seq(dev, MIPI_DSI_V_SYNC_END, MIPI_DCS_NOP);
	mdelay(120);
	dcs_write_seq(dev, MIPI_DSI_GENERIC_LONG_WRITE, MIPI_DCS_NOP);
	return 0;
}

static void ch13726a_reset(struct udevice *dev)
{
	struct ch13726a_panel_priv *priv = dev_get_priv(dev);
	dm_gpio_set_value(&priv->reset, true);
	mdelay(20);
	dm_gpio_set_value(&priv->reset, false);
	mdelay(20);
	dm_gpio_set_value(&priv->reset, true);
	mdelay(20);
}

static int ch13726a_panel_enable_backlight(struct udevice *dev)
{
	struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);
	struct mipi_dsi_device *device = plat->device;
	int ret;

	if (!device) {
        dev_err(dev, "mipi dsi device not attached\n");
        return -ENODEV;
    }
	device->lanes = plat->lanes;
	device->format = plat->format;
	device->mode_flags = plat->mode_flags;

	ret = mipi_dsi_attach(device);
	if (ret < 0)
		return ret;
	
	ch13726a_reset(dev);
	
	device->mode_flags |= MIPI_DSI_MODE_LPM;

	u8 id[3];
	ret = mipi_dsi_dcs_read(device, 0x04, id, 3);

	if (ret < 0) {
		printf("Panel not responding skipping init\n");
		return 1;
	}

	ret = ch13726a_init_sequence(dev);
	if (ret)
		return -ENODEV;
	
	ret = mipi_dsi_dcs_exit_sleep_mode(device);
	if (ret)
        	return -EIO;
	
	mdelay(128);

	ret = mipi_dsi_dcs_set_display_on(device);
	if (ret)
        	return -EIO;
	mdelay(20);

	ret = mipi_dsi_dcs_set_display_brightness(device,255);
	if(ret < 0) {
		printf("Error setting Display Brightness\n");
		return -EIO;
	}
	
	return 0;
}

static int ch13726a_panel_get_display_timing(struct udevice *dev,
                                            struct display_timing *timings)
{
	memcpy(timings, &default_timing, sizeof(*timings));
	
	return 0;
}

#if 0
static int ch13726a_panel_of_to_plat(struct udevice *dev)
{
	struct ch13726a_panel_priv *priv = dev_get_priv(dev);
	int ret;

	ret = gpio_request_by_name(dev, "reset-gpio", 0, &priv->reset,
                           GPIOD_IS_OUT);
	if (ret) {
		dev_err(dev, "Warning: cannot get reset GPIO\n");
		if (ret != -ENOENT)
                	return ret;
	}

	return 0;
}
#endif

static int ch13726a_panel_probe(struct udevice *dev)
{
	struct ch13726a_panel_priv *priv = dev_get_priv(dev);
	struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);
	int ret;

	plat->lanes = 4;
	plat->format = MIPI_DSI_FMT_RGB888;
	plat->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST;

	ret = gpio_request_by_name(dev, "reset-gpio", 0, &priv->reset,
                    GPIOD_IS_OUT);
 	if (ret) {
		dev_err(dev, "Warning: cannot get reset GPIO\n");
		if (ret != -ENOENT)
			return ret;
 	}

	return 0;
}

static const struct panel_ops ch13726a_panel_ops = {
        .enable_backlight = ch13726a_panel_enable_backlight,
        .get_display_timing = ch13726a_panel_get_display_timing,
};

static const struct udevice_id ch13726a_panel_ids[] = {
        { .compatible = "ch13726a,rp5" },
        { }
};

U_BOOT_DRIVER(ch13726a_panel) = {
	.name 		= "ch13726a_panel",
	.id 		= UCLASS_PANEL,
	.of_match	= ch13726a_panel_ids,
	.ops 		= &ch13726a_panel_ops,
	.probe 		= ch13726a_panel_probe,
	.plat_auto 	= sizeof(struct mipi_dsi_panel_plat),
	.priv_auto 	= sizeof(struct ch13726a_panel_priv),
};