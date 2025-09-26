// SPDX-License-Identifier: GPL-2.0+
/*
 * FB driver for the Raydium RM68140 LCD Controller
 *
 * Miku's Custom Driver - For a 320x480 SPI Display
 * Based on the FBTFT framework and ST7789V driver structure.
 *
 * Copyright (C) 2025 Miku
 */

#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <video/mipi_display.h>

#include "fbtft.h"

#define DRVNAME "fb_rm68140"

/*
 * MIPI DCS Commands and RM68140 Specific Commands
 * These are defined based on the user-provided register table and datasheet.
 */
#define TFT_NOP     0x00
#define TFT_SWRST   0x01
#define TFT_SLPIN   0x10
#define TFT_SLPOUT  0x11
#define TFT_INVOFF  0x20
#define TFT_INVON   0x21
#define TFT_DISPOFF 0x28
#define TFT_DISPON  0x29
#define TFT_CASET   0x2A // Column Address Set
#define TFT_PASET   0x2B // Page Address Set
#define TFT_RAMWR   0x2C // Memory Write
#define TFT_RAMRD   0x2E // Memory Read
#define TFT_MADCTL  0x36 // Memory Access Control
#define TFT_PIXFMT  0x3A // Pixel Format Set

/* Bitmasks for Memory Access Control (MADCTL register 0x36) */
#define TFT_MAD_MY  0x80  // Row Address Order
#define TFT_MAD_MX  0x40  // Column Address Order
#define TFT_MAD_MV  0x20  // Row/Column Exchange
#define TFT_MAD_ML  0x10  // Vertical Refresh Order
#define TFT_MAD_BGR 0x08  // BGR Order (instead of RGB)
#define TFT_MAD_MH  0x04  // Horizontal Refresh Order

/**
 * init_display() - initialize the RM68140 display controller
 * @par: FBTFT parameter object
 *
 * This sequence is translated from the user-provided initialization function.
 * It sends a series of commands and data to the display to configure it
 * for operation.
 *
 * Return: 0 on success, < 0 if error occurred.
 */
static int init_display(struct fbtft_par *par)
{
    /* Reset the display */
    par->fbtftops.reset(par);

    /* Sleep Out */
    write_reg(par, TFT_SLPOUT);
    mdelay(20);

    /* Power Setting */
    write_reg(par, 0xD0, 0x07, 0x42, 0x18);

    /* VCOM Control */
    write_reg(par, 0xD1, 0x00, 0x07, 0x10);

    /* Power Setting for Normal Mode */
    write_reg(par, 0xD2, 0x01, 0x02);

    /* Panel Driving Setting */
    write_reg(par, 0xC0, 0x10, 0x3B, 0x00, 0x02, 0x11);

    /* VCOM Control */
    write_reg(par, 0xC5, 0x03);
    
    /* Gamma Setting */
    write_reg(par, 0xC8, 0x00, 0x32, 0x36, 0x45, 0x06, 0x16, 0x37, 0x75, 0x77, 0x54, 0x0C, 0x00);

    /* Memory Access Control - Set to default portrait mode, BGR color order */
    write_reg(par, TFT_MADCTL, 0x0A); // As per user's init code

    /* Interface Pixel Format - 16 bits/pixel (RGB565) */
    write_reg(par, TFT_PIXFMT, 0x55);

    /* Column Address Set */
    write_reg(par, TFT_CASET, 0x00, 0x00, 0x01, 0x3F); // 0 to 319

    /* Page Address Set */
    write_reg(par, TFT_PASET, 0x00, 0x00, 0x01, 0xDF); // 0 to 479

    /* Wait for display to stabilize */
    mdelay(120);
    
    /* Display On */
    write_reg(par, TFT_DISPON);
    mdelay(25);

    return 0;
}

/**
 * set_var() - apply LCD properties like rotation and BGR mode
 * @par: FBTFT parameter object
 *
 * This function translates the desired rotation into the correct
 * MADCTL register value. The logic is adapted from the user-provided
 * rotation function.
 *
 * Return: 0 on success, < 0 if error occurred.
 */
static int set_var(struct fbtft_par *par)
{
    u8 madctl_val = 0;

    // The user's provided code uses BGR mode. We can make this configurable
    // via a device tree property `bgr`, but for now, we'll follow the provided init.
    if (par->bgr) {
        madctl_val |= TFT_MAD_BGR;
    }

    switch (par->info->var.rotate) {
    case 0:
        // Portrait
        madctl_val |= TFT_MAD_BGR; // Following user's code logic
        break;
    case 90:
        // Landscape
        madctl_val |= TFT_MAD_MV | TFT_MAD_BGR;
        break;
    case 180:
        // Inverted Portrait
        madctl_val |= TFT_MAD_MY | TFT_MAD_MX | TFT_MAD_BGR; // BGR might need adjustment
        break;
    case 270:
        // Inverted Landscape
        madctl_val |= TFT_MAD_MV | TFT_MAD_MY | TFT_MAD_BGR;
        break;
    default:
        return -EINVAL;
    }
    
    write_reg(par, TFT_MADCTL, madctl_val);

    // The user's code also sends a command 0xB6, which is "Display Function Control".
    // This seems to be related to scan direction and might be necessary for correct rotation.
    // We can add this here if needed.
    // Example from user's code:
    // write_reg(par, 0xB6, 0x00, 0x22, 0x3B);

    return 0;
}

/*
 * The `display` struct is the core of the driver. It tells the FBTFT framework
 * about our screen's properties and which functions to call for specific tasks.
 */
static struct fbtft_display display = {
    .regwidth = 8,
    .width = 320,
    .height = 480,
    .fbtftops = {
        .init_display = init_display,
        .set_var = set_var,
        // We can add .set_gamma later if needed
    },
};

/*
 * This macro registers our driver with the kernel.
 * The second argument is the "compatible" string that we will use
 * in our Device Tree (DTS) file.
 */
FBTFT_REGISTER_DRIVER(DRVNAME, "raydium,rm68140", &display);

/*
 * These macros create aliases so the driver can be loaded automatically
 * when a device with the matching compatible string is found in the device tree.
 */
MODULE_ALIAS("spi:" DRVNAME);
MODULE_ALIAS("platform:" DRVNAME);
MODULE_ALIAS("spi:rm68140");
MODULE_ALIAS("platform:rm68140");

MODULE_DESCRIPTION("FB driver for the Raydium RM68140 LCD Controller");
MODULE_AUTHOR("Miku");
MODULE_LICENSE("GPL");