// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 StarFive Technology Co., Ltd.
 */
#ifndef __VENC_STARFIVE_H__
#define __VENC_STARFIVE_H__

#include <linux/io.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <linux/reset-controller.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/clk-provider.h>

#define VENC_ID_NUM 5

struct starfive_venc_data {
	struct device *dev;
	struct clk *clk_venc_axi;
	struct clk *clk_vencbrg_main;
	struct clk *clk_venc_bclk;
	struct clk *clk_venc_cclk;
	struct clk *clk_venc_apb;

    struct reset_control *rst_vencbrg_main;
    struct reset_control *rst_venc_axi;
    struct reset_control *rst_venc_bclk;
    struct reset_control *rst_venc_cclk;
    struct reset_control *rst_venc_apb;

	struct clk *clk_venc_id[VENC_ID_NUM];
	struct reset_control *rst_venc_id[VENC_ID_NUM];
};

static struct starfive_venc_data *sf_venc = NULL;

const char venc_data_id[VENC_ID_NUM][15] = {
	"venc_axi",
	"vencbrg_main",
	"venc_bclk",
	"venc_cclk",
	"venc_apb",
};

void starfive_venc_rst_status(void)
{
	int i;
	int rst_state;
	for (i = 0; i < VENC_ID_NUM; i++) {		
		rst_state = reset_control_status(sf_venc->rst_venc_id[i]);
		printk(">>>>>status_rst%d:%d", i, rst_state);
	}
	return;
}

void starfive_venc_clk_status(void)
{
	int i;
	int clk_state;
	for (i = 0; i < VENC_ID_NUM; i++) {		
		clk_state = __clk_is_enabled(sf_venc->clk_venc_id[i]);
		printk(">>>>>status_clk%d:%d", i, clk_state);
	}
	return;
}

void starfive_venc_rst_exit(void)
{
	int i;
	int ret;
	for (i = 0; i < VENC_ID_NUM; i++) {
		/* Assert the reset of "vencbrg_main" could crash*/
		if (i != 1 ) {
			ret = reset_control_assert(sf_venc->rst_venc_id[i]);
			if (ret) {
				dev_err(sf_venc->dev, "reset assert failed\n");
			}
		}					
	}
	return;
}

void starfive_venc_clk_exit(void)
{
	int i;
	for (i = 0; i < VENC_ID_NUM; i++) {
		clk_disable_unprepare(sf_venc->clk_venc_id[i]);
	}
	
	return;
}

static int starfive_venc_clk_init(void)
{
	int ret = 0;
	int i;
	for (i = 0; i < VENC_ID_NUM; i++) {
		ret = clk_prepare_enable(sf_venc->clk_venc_id[i]);
		if (ret) {
			dev_err(sf_venc->dev, venc_data_id[i]);
			goto init_clk_failed;
		}
	}

	return 0;

init_clk_failed:
	for(; i > 0 ; i--) {
		clk_disable_unprepare(sf_venc->clk_venc_id[i-1]);
	}

	return ret;
}

static int starfive_venc_get_clk(void)
{
	int ret = 0;
	int i;
	for ( i = 0; i < VENC_ID_NUM ; i++) {
		sf_venc->clk_venc_id[i] = devm_clk_get(sf_venc->dev, venc_data_id[i]);
		if (IS_ERR(sf_venc->clk_venc_id[i])) {
			dev_err(sf_venc->dev,  venc_data_id[i]);
			ret = PTR_ERR(sf_venc->clk_venc_id[i]);
			goto get_clk_failed;
		}
	}

	return 0;

get_clk_failed:
	for( ; i > 0 ; i--) {
		devm_clk_put(sf_venc->dev, sf_venc->clk_venc_id[i-1]);
	}

	return ret;
}

static int starfive_venc_reset_init(void)
{
    int ret = 0;
	int i;
	for (i = 0; i < VENC_ID_NUM ; i++) {
		ret = reset_control_deassert(sf_venc->rst_venc_id[i]);
    	if (ret) {
			dev_err(sf_venc->dev, venc_data_id[i]);
       	 	goto init_reset_failed;
		}
	}

	return 0;

init_reset_failed:
	for (; i > 0 ; i--) {
		reset_control_assert(sf_venc->rst_venc_id[i-1]);
	}

	return ret;
}

static int starfive_venc_get_resets(void)
{
	
    int ret = 0;
	int i;
	for (i = 0; i < VENC_ID_NUM ; i++) {
		sf_venc->rst_venc_id[i] = devm_reset_control_get_exclusive(sf_venc->dev, venc_data_id[i]);
		if (IS_ERR(sf_venc->rst_venc_id[i])) {
			dev_err(sf_venc->dev,  venc_data_id[i]);
			ret = PTR_ERR(sf_venc->rst_venc_id[i]);
			goto get_resets_failed;
		}
	}

	return 0;

get_resets_failed:
	for (; i > 0; i--) {
		reset_control_put(sf_venc->rst_venc_id[i-1]);
	}

	return ret;
}

int starfive_venc_data_init(struct device *dev)
{
	int ret = 0;

	if (sf_venc == NULL){
		sf_venc = devm_kzalloc(dev, sizeof(*sf_venc), GFP_KERNEL);
		if (!sf_venc)
			return -ENOMEM;
		sf_venc->dev = dev;

		ret = starfive_venc_get_clk();
		if (ret) {
			dev_err(sf_venc->dev, "failed to get venc clock\n");
			return ret;
		}

		ret = starfive_venc_get_resets();
		if (ret) {
			dev_err(sf_venc->dev, "failed to get venc resets\n");
			return ret;
		}		
	}

	return 0;
}

int starfive_venc_clk_enable(struct device *dev)
{
    int ret = 0;

	ret = starfive_venc_data_init(dev);
	if (ret)
		return ret;

	ret = starfive_venc_clk_init();
	if (ret) {
		dev_err(sf_venc->dev, "failed to enable venc clock\n");
		return ret;
	}

	return 0;
}

int starfive_venc_clk_disable(struct device *dev)
{
    int ret = 0;

	ret = starfive_venc_data_init(dev);
	if (ret)
		return ret;
    
	starfive_venc_clk_exit();

	return 0;
	
}

int starfive_venc_rst_deassert(struct device *dev)
{
    int ret = 0;

	ret = starfive_venc_data_init(dev);
	if (ret)
		return ret;

	ret = starfive_venc_reset_init();
	if (ret) {
		dev_err(sf_venc->dev, "failed to deassert venc resets\n");
		return ret;
	}

	return 0;
}

int starfive_venc_rst_assert(struct device *dev)
{
    int ret = 0;
	ret = starfive_venc_data_init(dev);
	if (ret)
		return ret;
    
	starfive_venc_rst_exit();

	return 0;
	
}

int starfive_venc_clk_rst_init(struct platform_device *pdev)
{
    int ret = 0;

	ret = starfive_venc_data_init(&pdev->dev);
	if (ret)
		return ret;

	ret = starfive_venc_clk_init();
	if (ret) {
		dev_err(sf_venc->dev, "failed to enable venc clock\n");
		return ret;
	} 

    //resets
	starfive_venc_rst_exit();
	mdelay(1);

	ret = starfive_venc_reset_init();
	if (ret) {
		dev_err(sf_venc->dev, "failed to set venc reset\n");
		goto init_failed;
	}

	printk("starfive venc clock & reset init success.");
    return 0;

init_failed:
    starfive_venc_clk_exit();
    return ret;
}

#endif
