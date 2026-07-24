/**
  ******************************************************************************
  * @file           : bmx680.c
  * @brief          : Bosch BME680 sensor implementation.
  ******************************************************************************
  */

#include "bmx680.h"
#include "i2c.h"

static int32_t tFine;

static ErrorStatus bmx680_Send(BMxX80_TypeDef*, uint8_t, uint8_t);
static ErrorStatus bmx680_Receive(BMxX80_TypeDef*, uint8_t, uint8_t);
static void bmx680_CompensateTemperature(BMxX80_TypeDef*);




// -------------------------------------------------------------
ErrorStatus BMx680_Init(BMxX80_TypeDef* dev) {
  if ((dev == NULL) || (dev->I2Cx == NULL) || (dev->RawBufPtr == NULL)
      || (dev->CalibPtr == NULL) || (dev->Lock != DISABLE)) {
    return (ERROR);
  }
  dev->Lock = ENABLE;

  if (bmx680_Receive(dev, BMX680_DEV_ID, 1U) != SUCCESS) return (ERROR);
  dev->DevID = dev->RawBufPtr[0];
  if (dev->DevID != BME680_ID) return (ERROR);

  BMx680_calib_t* calib = (BMx680_calib_t*)dev->CalibPtr;
  if (bmx680_Receive(dev, BMX680_CALIB1, 24U) != SUCCESS) return (ERROR);
  calib->par_t2 = (int16_t)((dev->RawBufPtr[1] << 8) | dev->RawBufPtr[0]);
  calib->par_t3 = (int16_t)dev->RawBufPtr[2];
  calib->par_p1 = (int16_t)((dev->RawBufPtr[5] << 8) | dev->RawBufPtr[4]);
  calib->par_p2 = (int16_t)((dev->RawBufPtr[7] << 8) | dev->RawBufPtr[6]);
  calib->par_p3 = (int16_t)dev->RawBufPtr[8];
  calib->par_p4 = (int16_t)((dev->RawBufPtr[11] << 8) | dev->RawBufPtr[10]);
  calib->par_p5 = (int16_t)((dev->RawBufPtr[13] << 8) | dev->RawBufPtr[12]);
  calib->par_p6 = (int16_t)dev->RawBufPtr[15];
  calib->par_p7 = (int16_t)dev->RawBufPtr[14];
  calib->par_p8 = (int16_t)((dev->RawBufPtr[17] << 8) | dev->RawBufPtr[16]);
  calib->par_p9 = (int16_t)((dev->RawBufPtr[19] << 8) | dev->RawBufPtr[18]);
  calib->par_p10 = (int16_t)dev->RawBufPtr[20];

  if (bmx680_Receive(dev, BMX680_CALIB2, 16U) != SUCCESS) return (ERROR);
  calib->par_h1 = (int16_t)((dev->RawBufPtr[2] << 8) | (dev->RawBufPtr[1] & 0x0FU));
  calib->par_h2 = (int16_t)((dev->RawBufPtr[0] << 8) | ((dev->RawBufPtr[1] & 0xF0U) >> 4));
  calib->par_h3 = (int16_t)dev->RawBufPtr[3];
  calib->par_h4 = (int16_t)dev->RawBufPtr[4];
  calib->par_h5 = (int16_t)dev->RawBufPtr[5];
  calib->par_h6 = (int16_t)dev->RawBufPtr[6];
  calib->par_h7 = (int16_t)dev->RawBufPtr[7];
  calib->par_t1 = (int16_t)((dev->RawBufPtr[9] << 8) | dev->RawBufPtr[8]);
  calib->par_g1 = (int16_t)dev->RawBufPtr[12];
  calib->par_g2 = (int16_t)((dev->RawBufPtr[11] << 8) | dev->RawBufPtr[10]);
  calib->par_g3 = (int16_t)dev->RawBufPtr[13];

  dev->Lock = DISABLE;
  return (SUCCESS);
}




// -------------------------------------------------------------
ErrorStatus BMx680_Measurement(BMxX80_TypeDef* dev) {
  if ((dev == NULL) || (dev->Lock != DISABLE)) return (ERROR);
  dev->Lock = ENABLE;

  if (bmx680_Send(dev, BMX680_CTRL_HUM, BMX680_HUMIDITY_OVS_0) != SUCCESS) {
    return (ERROR);
  }
  if (bmx680_Send(
        dev,
        BMX680_CTRL_MEAS,
        BMX680_TEMPERATURE_OVS_8 | BMX680_PRESSURE_OVS_0 | BMX680_FORCED_MODE
      ) != SUCCESS) return (ERROR);

  uint32_t timeout = 100000U;
  do {
    if (bmx680_Receive(dev, BMX680_STATUS, 1U) != SUCCESS) return (ERROR);
    if ((dev->RawBufPtr[0] & BMX680_MEASURING) == 0U) break;
  } while (--timeout != 0U);
  if (timeout == 0U) return (ERROR);

  if (bmx680_Receive(dev, BMX680_TEMP_MSB, 3U) != SUCCESS) return (ERROR);
  bmx680_CompensateTemperature(dev);

  dev->Lock = DISABLE;
  return (SUCCESS);
}




// -------------------------------------------------------------
static ErrorStatus bmx680_Send(BMxX80_TypeDef* dev, uint8_t reg, uint8_t value) {
  uint8_t buffer[2] = {reg, value};
  return I2C_Master_Send(dev->I2Cx, dev->I2C_Address, buffer, sizeof(buffer));
}




// -------------------------------------------------------------
static ErrorStatus bmx680_Receive(BMxX80_TypeDef* dev, uint8_t reg, uint8_t length) {
  return I2C_Master_ReadRegister(
    dev->I2Cx,
    dev->I2C_Address,
    reg,
    dev->RawBufPtr,
    length
  );
}




// -------------------------------------------------------------
static void bmx680_CompensateTemperature(BMxX80_TypeDef* dev) {
  uint32_t adcTemperature = (
      ((uint32_t)dev->RawBufPtr[0] << 12)
    | ((uint32_t)dev->RawBufPtr[1] << 4)
    | ((uint32_t)dev->RawBufPtr[2] >> 4)
  );
  BMx680_calib_t* calib = (BMx680_calib_t*)dev->CalibPtr;
  int32_t var1 = ((int32_t)(adcTemperature >> 3) - ((int32_t)calib->par_t1 << 1));
  int32_t var2 = (var1 * calib->par_t2) >> 11;
  int32_t var3 = ((((var1 >> 1) * (var1 >> 1)) >> 12)
                  * ((int32_t)calib->par_t3 << 4)) >> 14;
  tFine = var2 + var3;
  dev->Results.temperature = (tFine * 5 + 128) >> 8;
}
