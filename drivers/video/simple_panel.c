// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2016 Google, Inc
 * Written by Simon Glass <sjg@chromium.org>
 */

#define LOG_DEBUG

#include <common.h>
#include <backlight.h>
#include <dm.h>
#include <log.h>
#include <panel.h>
#include <asm/gpio.h>
#include <power/regulator.h>

struct simple_panel_priv {
	struct udevice *reg;
	struct udevice *backlight;
	struct gpio_desc enable;
};

static const struct display_timing tm070jdhg30_timing = {
       .pixelclock.typ         = 71700000,
       .hactive.typ            = 1280,
       .hfront_porch.typ       = 5,
       .hback_porch.typ        = 151,
       .hsync_len.typ          = 4,
       .vactive.typ            = 800,
       .vfront_porch.typ       = 2,
       .vback_porch.typ        = 28,
       .vsync_len.typ          = 1,
};

static int simple_panel_enable_backlight(struct udevice *dev)
{
	struct simple_panel_priv *priv = dev_get_priv(dev);
	int ret;

	dm_gpio_set_value(&priv->enable, 1);
	if (priv->backlight) {
		debug("%s: start, backlight = '%s'\n", __func__, priv->backlight->name);
		ret = backlight_enable(priv->backlight);
		debug("%s: done, ret = %d\n", __func__, ret);
		if (ret)
			return ret;
	}

	return 0;
}

static int simple_panel_set_backlight(struct udevice *dev, int percent)
{
	struct simple_panel_priv *priv = dev_get_priv(dev);
	int ret;

	debug("%s: start, backlight = '%s'\n", __func__, priv->backlight->name);
	dm_gpio_set_value(&priv->enable, 1);
	if (priv->backlight) {
		ret = backlight_set_brightness(priv->backlight, percent);
		debug("%s: done, ret = %d\n", __func__, ret);
		if (ret)
			return ret;
	}

	return 0;
}

static int simple_panel_of_to_plat(struct udevice *dev)
{
	struct simple_panel_priv *priv = dev_get_priv(dev);
	int ret;

	if (IS_ENABLED(CONFIG_DM_REGULATOR)) {
		ret = uclass_get_device_by_phandle(UCLASS_REGULATOR, dev,
						   "power-supply", &priv->reg);
		if (ret) {
			debug("%s: Warning: cannot get power supply: ret=%d\n",
			      __func__, ret);
			if (ret != -ENOENT)
				return ret;
		}
	}

	ret = uclass_get_device_by_phandle(UCLASS_PANEL_BACKLIGHT, dev,
						   "backlight", &priv->backlight);
	if (ret) {
		debug("%s: Cannot get backlight: ret=%d\n", __func__, ret);
		if (ret != -ENOENT)
			return log_ret(ret);

		priv->backlight = NULL;
	}

	ret = gpio_request_by_name(dev, "enable-gpios", 0, &priv->enable,
				   GPIOD_IS_OUT);
	if (ret) {
		debug("%s: Warning: cannot get enable GPIO: ret=%d\n",
		      __func__, ret);
		if (ret != -ENOENT)
			return log_ret(ret);
	}

	return 0;
}

static int simple_panel_remove(struct udevice *dev)
{
	struct simple_panel_priv *priv = dev_get_priv(dev);
	int ret;

	if (priv->backlight) {
		ret = backlight_set_brightness(priv->backlight, BACKLIGHT_OFF);
		if (ret)
			return ret;
	}

	if (IS_ENABLED(CONFIG_DM_REGULATOR) && priv->reg) {
		debug("%s: Enable regulator '%s'\n", __func__, priv->reg->name);
		ret = regulator_set_enable(priv->reg, false);
		if (ret)
			return ret;
	}

	dm_gpio_set_value(&priv->enable, 0);

	return 0;
}

static int simple_panel_probe(struct udevice *dev)
{
	struct simple_panel_priv *priv = dev_get_priv(dev);
	int ret;

	printf("simple_panel_probe\n");

	if (IS_ENABLED(CONFIG_DM_REGULATOR) && priv->reg) {
		debug("%s: Enable regulator '%s'\n", __func__, priv->reg->name);
		ret = regulator_set_enable(priv->reg, true);
		if (ret)
			return ret;
	}

	return 0;
}

static int simple_panel_get_display_timing(struct udevice *dev,
                                           struct display_timing *timings)
{
	printf("simple_panel_get_display_timing\n");
       memcpy(timings, &tm070jdhg30_timing, sizeof(*timings));

       return 0;
}

static const struct panel_ops simple_panel_ops = {
	.enable_backlight	= simple_panel_enable_backlight,
	.set_backlight		= simple_panel_set_backlight,
	.get_display_timing = simple_panel_get_display_timing,
};

static const struct udevice_id simple_panel_ids[] = {
	{ .compatible = "simple-panel" },
	{ .compatible = "auo,b133xtn01" },
	{ .compatible = "auo,b116xw03" },
	{ .compatible = "auo,b133htn01" },
	{ .compatible = "boe,nv140fhmn49" },
	{ .compatible = "lg,lb070wv8" },
	{ .compatible = "sharp,lq123p1jx31" },
	{ .compatible = "boe,nv101wxmn51" },
	{ .compatible = "tianma,tm070jdhg30" },
	{ }
};

U_BOOT_DRIVER(simple_panel) = {
	.name	= "simple_panel",
	.id	= UCLASS_PANEL,
	.of_match = simple_panel_ids,
	.ops	= &simple_panel_ops,
	.of_to_plat	= simple_panel_of_to_plat,
	.probe		= simple_panel_probe,
	.priv_auto	= sizeof(struct simple_panel_priv),
	.remove		= simple_panel_remove,
};
