/**
  ******************************************************************************
  * @file           : project_config.h
  * @brief          : Build-time feature selection and implementation mapping.
  ******************************************************************************
  */

#ifndef __PROJECT_CONFIG_H
#define __PROJECT_CONFIG_H

#if defined(USE_WH_DISPLAY)
#ifndef WH_DSPL_MODEL
#define WH_DSPL_MODEL 1602
#endif

/*
 * Temporary 3.3 V contrast workaround.
 * Use 2 for the normal WH1602/WH2004 controller configuration.
 */
#ifndef WH_DSPL_LINE_MODE
#define WH_DSPL_LINE_MODE 1
#endif

#define DSPL_OUT(ch) putc_dspl_wh(ch)
#elif defined(USE_SSD_DISPLAY)
#ifndef SSD_DSPL_MODEL
#define SSD_DSPL_MODEL SSD1315_MODEL
#endif

#ifndef SSD_DSPL_FONT
#define SSD_DSPL_FONT SSD13XX_FONT_5X7
#endif

#define DSPL_OUT(ch) putc_dspl_ssd(ch)
#else
#define DSPL_OUT(ch) ((void)(ch))
#endif

#endif /* __PROJECT_CONFIG_H */
