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

#define DSPL_OUT(ch) putc_dspl_wh(ch)
#else
#define DSPL_OUT(ch) ((void)(ch))
#endif

#endif /* __PROJECT_CONFIG_H */
