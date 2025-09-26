// SPDX-License-Identifier: GPL-2.0+
/*
 * FB driver for the Ilitek ILI9488 LCD Controller
 *
 * Miku's Custom Driver - For a 320x480 4-Wire SPI Display
 * Based on the FBTFT framework and user-provided initialization sequence.
 *
 * Copyright (C) 2025 Miku
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <video/mipi_display.h>

#include "fbtft.h"

#define DRVNAME     "fb_ili9488"
#define WIDTH       320
#define HEIGHT      480

/*
 * This initialization sequence is a direct translation of the user-provided
 * ILI9488_init.h file into the FBTFT array format.
 * Format: -1 for command, -2 for delay, -3 for end.
 */
static const s16 default_init_sequence[] = {
    // Positive Gamma Control
    -1, 0xE0, 0x00, 0x03, 0x09, 0x08, 0x16, 0x0A, 0x3F, 0x78, 0x4C, 0x09, 0x0A, 0x08, 0x16, 0x1A, 0x0F,
    // Negative Gamma Control
    -1, 0XE1, 0x00, 0x16, 0x19, 0x03, 0x0F, 0x05, 0x32, 0x45, 0x46, 0x04, 0x0E, 0x0D, 0x35, 0x37, 0x0F,
    // Power Control 1
    -1, 0xC0, 0x17, 0x15,
    // Power Control 2
    -1, 0xC1, 0x41,
    // VCOM Control
    -1, 0xC5, 0x00, 0x12, 0x80,
    // Interface Mode Control
    -1, 0xB0, 0x00,
    // Frame Rate Control
    -1, 0xB1, 0xA0,
    // Display Inversion Control
    -1, 0xB4, 0x02,
    // Display Function Control
    -1, 0xB6, 0x02, 0x02, 0x3B,
    // Entry Mode Set
    -1, 0xB7, 0xC6,
    // Adjust Control 3
    -1, 0xF7, 0xA9, 0x51, 0x2C, 0x82,
    // Pixel Interface Format (Set to 16-bit/pixel for SPI)
    -1, MIPI_DCS_SET_PIXEL_FORMAT, 0x55,
    // Exit Sleep Mode
    -1, MIPI_DCS_EXIT_SLEEP_MODE,
    -2, 120, // Delay 120ms
    // Display On
    -1, MIPI_DCS_SET_DISPLAY_ON,
    -2, 25,  // Delay 25ms
    // End marker
    -3
};

/*
 * set_addr_win is a standard function for TFT displays that sets
 * the drawing window (the area of memory to write pixel data to).
 */
static void set_addr_win(struct fbtft_par *par, int xs, int ys, int xe, int ye)
{
    write_reg(par, MIPI_DCS_SET_COLUMN_ADDRESS,
          xs >> 8, xs & 0xFF, xe >> 8, xe & 0xFF);

    write_reg(par, MIPI_DCS_SET_PAGE_ADDRESS,
          ys >> 8, ys & 0xFF, ye >> 8, ye & 0xFF);

    write_reg(par, MIPI_DCS_WRITE_MEMORY_START);
}

/*
 * set_var applies runtime properties like screen rotation.
 * This logic is translated from the user-provided ILI9488_rotate.h.
 */
static int set_var(struct fbtft_par *par)
{
    u8 madctl_val = 0;

    // The user-provided rotation logic implies BGR color order.
    // The `par->bgr` property from the device tree can override this if needed.
    if (par->bgr) {
        madctl_val |= (1 << 3); // TFT_MAD_BGR
    }

    switch (par->info->var.rotate) {
    case 0:
        madctl_val |= (1 << 6); // TFT_MAD_MX
        break;
    case 90:
        madctl_val |= (1 << 5); // TFT_MAD_MV
        break;
    case 180:
        madctl_val |= (1 << 7); // TFT_MAD_MY
        break;
    case 270:
        madctl_val |= (1 << 7) | (1 << 6) | (1 << 5); // TFT_MAD_MX | TFT_MAD_MY | TFT_MAD_MV
        break;
    default:
        return -EINVAL;
    }

    write_reg(par, MIPI_DCS_SET_ADDRESS_MODE, madctl_val);

    return 0;
}

/*
 * The `display` struct is the core of the driver. It tells the FBTFT framework
 * about our screen's properties and which functions to call for specific tasks.
 */
static struct fbtft_display display = {
    .regwidth = 8,
    .width = WIDTH,
    .height = HEIGHT,
    .init_sequence = default_init_sequence,
    .fbtftops = {
        .set_addr_win = set_addr_win,
        .set_var = set_var,
    },
};

/*
 * This macro registers our driver with the kernel.
 * The "compatible" string should match the one in your Device Tree (DTS) file.
 */
FBTFT_REGISTER_DRIVER(DRVNAME, "ilitek,ili9488", &display);

/*
 * These macros create aliases so the driver can be loaded automatically.
 */
MODULE_ALIAS("spi:" DRVNAME);
MODULE_ALIAS("platform:" DRVNAME);
MODULE_ALIAS("spi:ili9488");
MODULE_ALIAS("platform:ili9488");

MODULE_DESCRIPTION("FB driver for the Ilitek ILI9488 LCD Controller");
MODULE_AUTHOR("Miku");
MODULE_LICENSE("GPL");