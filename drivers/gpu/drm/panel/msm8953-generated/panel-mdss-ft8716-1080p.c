// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2024 FIXME
// Generated with linux-mdss-dsi-panel-driver-generator from vendor device tree:
//   Copyright (c) 2013, The Linux Foundation. All rights reserved. (FIXME)

#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <drm/drm_probe_helper.h>

struct ft8716 {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct regulator_bulk_data supplies[2];
	struct gpio_desc *reset_gpio;
};

static inline struct ft8716 *to_ft8716(struct drm_panel *panel)
{
	return container_of(panel, struct ft8716, panel);
}

static void ft8716_reset(struct ft8716 *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(5000, 6000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(10000, 11000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(10000, 11000);
}

static int ft8716_on(struct ft8716 *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret;

	mipi_dsi_generic_write_seq(dsi, 0x00, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0xff, 0x87, 0x16, 0x01);
	mipi_dsi_generic_write_seq(dsi, 0x00, 0x80);
	mipi_dsi_generic_write_seq(dsi, 0xff, 0x87, 0x16);
	mipi_dsi_generic_write_seq(dsi, 0x00, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0xff, 0x00, 0x00, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0x00, 0x80);
	mipi_dsi_generic_write_seq(dsi, 0xff, 0x00, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0x35, 0x00);

	ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to exit sleep mode: %d\n", ret);
		return ret;
	}
	msleep(120);

	ret = mipi_dsi_dcs_set_display_on(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to set display on: %d\n", ret);
		return ret;
	}
	msleep(20);

	mipi_dsi_generic_write_seq(dsi, 0x00, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0x53, 0x24);

	return 0;
}

static int ft8716_off(struct ft8716 *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret;

	mipi_dsi_generic_write_seq(dsi, 0x00, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0x99, 0x95, 0x27);

	ret = mipi_dsi_dcs_set_display_off(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to set display off: %d\n", ret);
		return ret;
	}
	msleep(20);

	ret = mipi_dsi_dcs_enter_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to enter sleep mode: %d\n", ret);
		return ret;
	}
	msleep(120);

	mipi_dsi_generic_write_seq(dsi, 0x00, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0x99, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0x00, 0x00);
	mipi_dsi_generic_write_seq(dsi, 0xf7, 0x5a, 0xa5, 0x87, 0x16);

	return 0;
}

static int ft8716_prepare(struct drm_panel *panel)
{
	struct ft8716 *ctx = to_ft8716(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	ret = regulator_bulk_enable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
	if (ret < 0) {
		dev_err(dev, "Failed to enable regulators: %d\n", ret);
		return ret;
	}

	ft8716_reset(ctx);

	ret = ft8716_on(ctx);
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
		return ret;
	}

	return 0;
}

static int ft8716_unprepare(struct drm_panel *panel)
{
	struct ft8716 *ctx = to_ft8716(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	ret = ft8716_off(ctx);
	if (ret < 0)
		dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);

	return 0;
}

static const struct drm_display_mode ft8716_mode = {
	.clock = (1080 + 32 + 4 + 32) * (1920 + 16 + 2 + 16) * 60 / 1000,
	.hdisplay = 1080,
	.hsync_start = 1080 + 32,
	.hsync_end = 1080 + 32 + 4,
	.htotal = 1080 + 32 + 4 + 32,
	.vdisplay = 1920,
	.vsync_start = 1920 + 16,
	.vsync_end = 1920 + 16 + 2,
	.vtotal = 1920 + 16 + 2 + 16,
	.width_mm = 65,
	.height_mm = 115,
	.type = DRM_MODE_TYPE_DRIVER,
};

static int ft8716_get_modes(struct drm_panel *panel,
			    struct drm_connector *connector)
{
	return drm_connector_helper_get_modes_fixed(connector, &ft8716_mode);
}

static const struct drm_panel_funcs ft8716_panel_funcs = {
	.prepare = ft8716_prepare,
	.unprepare = ft8716_unprepare,
	.get_modes = ft8716_get_modes,
};

static int ft8716_bl_update_status(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	u16 brightness = backlight_get_brightness(bl);
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_set_display_brightness(dsi, brightness);
	if (ret < 0)
		return ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	return 0;
}

// TODO: Check if /sys/class/backlight/.../actual_brightness actually returns
// correct values. If not, remove this function.
static int ft8716_bl_get_brightness(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	u16 brightness;
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_get_display_brightness(dsi, &brightness);
	if (ret < 0)
		return ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	return brightness & 0xff;
}

static const struct backlight_ops ft8716_bl_ops = {
	.update_status = ft8716_bl_update_status,
	.get_brightness = ft8716_bl_get_brightness,
};

static struct backlight_device *
ft8716_create_backlight(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	const struct backlight_properties props = {
		.type = BACKLIGHT_RAW,
		.brightness = 255,
		.max_brightness = 255,
	};

	return devm_backlight_device_register(dev, dev_name(dev), dev, dsi,
					      &ft8716_bl_ops, &props);
}

static int ft8716_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct ft8716 *ctx;
	int ret;

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->supplies[0].supply = "vsn";
	ctx->supplies[1].supply = "vsp";
	ret = devm_regulator_bulk_get(dev, ARRAY_SIZE(ctx->supplies),
				      ctx->supplies);
	if (ret < 0)
		return dev_err_probe(dev, ret, "Failed to get regulators\n");

	ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio))
		return dev_err_probe(dev, PTR_ERR(ctx->reset_gpio),
				     "Failed to get reset-gpios\n");

	ctx->dsi = dsi;
	mipi_dsi_set_drvdata(dsi, ctx);

	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST |
			  MIPI_DSI_MODE_VIDEO_HSE |
			  MIPI_DSI_CLOCK_NON_CONTINUOUS | MIPI_DSI_MODE_LPM;

	drm_panel_init(&ctx->panel, dev, &ft8716_panel_funcs,
		       DRM_MODE_CONNECTOR_DSI);

	ctx->panel.backlight = ft8716_create_backlight(dsi);
	if (IS_ERR(ctx->panel.backlight))
		return dev_err_probe(dev, PTR_ERR(ctx->panel.backlight),
				     "Failed to create backlight\n");

	drm_panel_add(&ctx->panel);

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		drm_panel_remove(&ctx->panel);
		return dev_err_probe(dev, ret, "Failed to attach to DSI host\n");
	}

	return 0;
}

static void ft8716_remove(struct mipi_dsi_device *dsi)
{
	struct ft8716 *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id ft8716_of_match[] = {
	{ .compatible = "mdss,ft8716-1080p" }, // FIXME
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ft8716_of_match);

static struct mipi_dsi_driver ft8716_driver = {
	.probe = ft8716_probe,
	.remove = ft8716_remove,
	.driver = {
		.name = "panel-ft8716",
		.of_match_table = ft8716_of_match,
	},
};
module_mipi_dsi_driver(ft8716_driver);

MODULE_AUTHOR("linux-mdss-dsi-panel-driver-generator <fix@me>"); // FIXME
MODULE_DESCRIPTION("DRM driver for ft8716 1080p video mode dsi panel");
MODULE_LICENSE("GPL");
