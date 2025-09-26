// SPDX-License-Identifier: GPL-2.0+
/*
 * FB driver for the ST7796 LCD Controller
 *
 * Based on the user-provided initialization sequence and the fb_st7789v.c driver.
 * This driver is intended for use with the Linux fbtft framework.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <video/mipi_display.h>

#include "fbtft.h"

#define DRVNAME "fb_st7796"

/*
 * Default gamma curve based on the provided init sequence.
 * It consists of 2 curves (Positive and Negative), each with 14 values.
 */
#define DEFAULT_GAMMA \
    "F0 09 0B 06 04 15 2F 54 42 3C 17 14 18 1B\n" \
    "E0 09 0B 06 04 03 2B 43 42 3B 16 14 17 1B"

/**
 * enum st7796_command - ST7796 display controller specific commands
 */
enum st7796_command {
    INVCTR    = 0xB4, /* Display Inversion Control */
    DFUNCTR   = 0xB6, /* Display Function Control */
    PWCTR2    = 0xC1, /* Power Control 2 */
    /* 0xC2 is also a power control command used in the init sequence */
    VMCTR1    = 0xC5, /* VCOM Control */
    DOOCTRL   = 0xE8, /* Display Output Ctrl Adjust */
    GMCTRP1   = 0xE0, /* Positive Gamma Correction */
    GMCTRN1   = 0xE1, /* Negative Gamma Correction */
    CMDSETCTL = 0xF0, /* Command Set Control */
};

/* Bitmasks for Memory Data Access Control (MADCTL, 0x36) command */
#define MADCTL_MY  BIT(7)  /* Row Address Order */
#define MADCTL_MX  BIT(6)  /* Column Address Order */
#define MADCTL_MV  BIT(5)  /* Row/Column Exchange */
#define MADCTL_ML  BIT(4)  /* Vertical Refresh Order */
#define MADCTL_BGR BIT(3)  /* BGR order */

/**
 * init_display() - initialize the display controller
 */
static int init_display(struct fbtft_par *par)
{
    par->fbtftops.reset(par);
    mdelay(120);

    write_reg(par, MIPI_DCS_SOFT_RESET);
    mdelay(120);

    write_reg(par, MIPI_DCS_EXIT_SLEEP_MODE);
    mdelay(120);

    /* Command Set control: Enable extension command 2 part I & II */
    write_reg(par, CMDSETCTL, 0xC3);
    write_reg(par, CMDSETCTL, 0x96);

    /* Interface Pixel Format: 16-bit/pixel (RGB-565) */
    write_reg(par, MIPI_DCS_SET_PIXEL_FORMAT, 0x55);

    /* Column inversion: 1-dot inversion */
    write_reg(par, INVCTR, 0x01);

    /* Display Function Control */
    write_reg(par, DFUNCTR, 0x80, 0x02, 0x3B);

    /* Display Output Ctrl Adjust */
    write_reg(par, DOOCTRL, 0x40, 0x8A, 0x00, 0x00, 0x29, 0x19, 0xA5, 0x33);

    /* Power control 2 */
    write_reg(par, PWCTR2, 0x06);

    /* Power control 3 */
    write_reg(par, 0xC2, 0xA7);

    /* VCOM Control */
    write_reg(par, VMCTR1, 0x18);
    mdelay(120);

    /* Gamma settings are handled by the set_gamma function */

    /* Command Set control: Disable extension command 2 part I & II */
    write_reg(par, CMDSETCTL, 0x3C);
    write_reg(par, CMDSETCTL, 0x69);
    mdelay(120);

    /* Display on */
    write_reg(par, MIPI_DCS_SET_DISPLAY_ON);

    return 0;
}

/**
 * set_var() - apply LCD properties like rotation and BGR mode
 */
static int set_var(struct fbtft_par *par)
{
    u8 madctl_par = 0;

    if (par->bgr)
        madctl_par |= MADCTL_BGR;

    /* Rotation logic from the user-provided 'rotate' function */
    switch (par->info->var.rotate) {
    case 0:
        madctl_par |= MADCTL_MX;
        break;
    case 90:
        madctl_par |= MADCTL_MV;
        break;
    case 180:
        madctl_par |= MADCTL_MY;
        break;
    case 270:
        madctl_par |= (MADCTL_MX | MADCTL_MY | MADCTL_MV);
        break;
    default:
        return -EINVAL;
    }

    write_reg(par, MIPI_DCS_SET_ADDRESS_MODE, madctl_par);

    return 0;
}

/**
 * set_gamma() - set gamma curves
 */
static int set_gamma(struct fbtft_par *par, u32 *curves)
{
    int i;
    int c;

    if (par->gamma.num_values != 14) {
         dev_err(par->info->device, "Unsupported gamma length: %d\n",
             par->gamma.num_values);
         return -EINVAL;
    }

    for (i = 0; i < par->gamma.num_curves; i++) {
        c = i * par->gamma.num_values;
        /* GMCTRP1 = 0xE0, GMCTRN1 = 0xE1 */
        write_reg(par, GMCTRP1 + i,
              curves[c + 0],  curves[c + 1],  curves[c + 2],
              curves[c + 3],  curves[c + 4],  curves[c + 5],
              curves[c + 6],  curves[c + 7],  curves[c + 8],
              curves[c + 9],  curves[c + 10], curves[c + 11],
              curves[c + 12], curves[c + 13]);
    }
    return 0;
}

/**
 * blank() - blank/unblank the display
 */
static int blank(struct fbtft_par *par, bool on)
{
    if (on)
        write_reg(par, MIPI_DCS_SET_DISPLAY_OFF);
    else
        write_reg(par, MIPI_DCS_SET_DISPLAY_ON);
    return 0;
}

static struct fbtft_display display = {
    .regwidth = 8,
    .width = 320,
    .height = 480,
    .gamma_num = 2,
    .gamma_len = 14,
    .gamma = DEFAULT_GAMMA,
    .fbtftops = {
        .init_display = init_display,
        .set_var = set_var,
        .set_gamma = set_gamma,
        .blank = blank,
    },
};

FBTFT_REGISTER_DRIVER(DRVNAME, "sitronix,st7796", &display);

MODULE_ALIAS("spi:" DRVNAME);
MODULE_ALIAS("platform:" DRVNAME);
MODULE_ALIAS("spi:st7796");
MODULE_ALIAS("platform:st7796");

MODULE_DESCRIPTION("FB driver for the ST7796 LCD Controller");
MODULE_AUTHOR("Gemini (based on user-provided code and ST7789 driver)");
MODULE_LICENSE("GPL");