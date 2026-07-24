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
static uint32_t bmx680_CompensatePressure(BMxX80_TypeDef*);
static uint32_t bmx680_CompensateHumidity(BMxX80_TypeDef*);




// -------------------------------------------------------------
ErrorStatus BMx680_Init(BMxX80_TypeDef* dev) {
  if ((dev == NULL) || (dev->I2Cx == NULL) || (dev->RawBufPtr == NULL)
      || (dev->CalibPtr == NULL) || (dev->Lock != DISABLE)) {
    return (ERROR);
  }
  dev->Lock = ENABLE;
  ErrorStatus status = ERROR;

  if (bmx680_Receive(dev, BMX680_DEV_ID, 1U) != SUCCESS) goto done;
  dev->DevID = dev->RawBufPtr[0];
  if (dev->DevID != BME680_ID) goto done;

  BMx680_calib_t* calib = (BMx680_calib_t*)dev->CalibPtr;
  if (bmx680_Receive(dev, BMX680_CALIB1, 24U) != SUCCESS) goto done;
  calib->par_t2 = (int16_t)((dev->RawBufPtr[1] << 8) | dev->RawBufPtr[0]);
  calib->par_t3 = (int8_t)dev->RawBufPtr[2];
  calib->par_p1 = (uint16_t)((dev->RawBufPtr[5] << 8) | dev->RawBufPtr[4]);
  calib->par_p2 = (int16_t)((dev->RawBufPtr[7] << 8) | dev->RawBufPtr[6]);
  calib->par_p3 = (int8_t)dev->RawBufPtr[8];
  calib->par_p4 = (int16_t)((dev->RawBufPtr[11] << 8) | dev->RawBufPtr[10]);
  calib->par_p5 = (int16_t)((dev->RawBufPtr[13] << 8) | dev->RawBufPtr[12]);
  calib->par_p6 = (int8_t)dev->RawBufPtr[15];
  calib->par_p7 = (int8_t)dev->RawBufPtr[14];
  calib->par_p8 = (int16_t)((dev->RawBufPtr[17] << 8) | dev->RawBufPtr[16]);
  calib->par_p9 = (int16_t)((dev->RawBufPtr[19] << 8) | dev->RawBufPtr[18]);
  calib->par_p10 = dev->RawBufPtr[20];
  if (calib->par_p1 == 0U) goto done;

  if (bmx680_Receive(dev, BMX680_CALIB2, 16U) != SUCCESS) goto done;
  calib->par_h1 = (uint16_t)((dev->RawBufPtr[2] << 4) | (dev->RawBufPtr[1] & 0x0FU));
  calib->par_h2 = (uint16_t)((dev->RawBufPtr[0] << 4) | (dev->RawBufPtr[1] >> 4));
  calib->par_h3 = (int8_t)dev->RawBufPtr[3];
  calib->par_h4 = (int8_t)dev->RawBufPtr[4];
  calib->par_h5 = (int8_t)dev->RawBufPtr[5];
  calib->par_h6 = dev->RawBufPtr[6];
  calib->par_h7 = (int8_t)dev->RawBufPtr[7];
  calib->par_t1 = (uint16_t)((dev->RawBufPtr[9] << 8) | dev->RawBufPtr[8]);
  calib->par_g1 = (int8_t)dev->RawBufPtr[12];
  calib->par_g2 = (int16_t)((dev->RawBufPtr[11] << 8) | dev->RawBufPtr[10]);
  calib->par_g3 = (int8_t)dev->RawBufPtr[13];

  status = SUCCESS;

done:
  dev->Lock = DISABLE;
  return (status);
}




// -------------------------------------------------------------
ErrorStatus BMx680_Measurement(BMxX80_TypeDef* dev) {
  if ((dev == NULL) || (dev->Lock != DISABLE)) return (ERROR);
  dev->Lock = ENABLE;
  ErrorStatus status = ERROR;

  if (bmx680_Send(dev, BMX680_CTRL_HUM, BMX680_HUMIDITY_OVS_4) != SUCCESS) {
    goto done;
  }
  if (bmx680_Send(
        dev,
        BMX680_CTRL_MEAS,
        BMX680_TEMPERATURE_OVS_8 | BMX680_PRESSURE_OVS_4 | BMX680_FORCED_MODE
      ) != SUCCESS) goto done;

  uint16_t timeout = 250U;
  do {
    _delay_ms(1U);
    if (bmx680_Receive(dev, BMX680_STATUS, 1U) != SUCCESS) goto done;
    if ((dev->RawBufPtr[0] & BMX680_MEASURING) == 0U) break;
  } while (--timeout != 0U);
  if (timeout == 0U) goto done;

  if (bmx680_Receive(dev, BMX680_DATA, 8U) != SUCCESS) goto done;
  uint32_t rawPressure = (
      ((uint32_t)dev->RawBufPtr[0] << 12)
    | ((uint32_t)dev->RawBufPtr[1] << 4)
    | ((uint32_t)dev->RawBufPtr[2] >> 4)
  );
  uint32_t rawTemperature = (
      ((uint32_t)dev->RawBufPtr[3] << 12)
    | ((uint32_t)dev->RawBufPtr[4] << 4)
    | ((uint32_t)dev->RawBufPtr[5] >> 4)
  );
  if ((rawPressure == 0x80000U) || (rawTemperature == 0x80000U)) goto done;

  bmx680_CompensateTemperature(dev);
  dev->Results.pressure = bmx680_CompensatePressure(dev);
  dev->Results.humidity = bmx680_CompensateHumidity(dev);

  status = SUCCESS;

done:
  dev->Lock = DISABLE;
  return (status);
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
      ((uint32_t)dev->RawBufPtr[3] << 12)
    | ((uint32_t)dev->RawBufPtr[4] << 4)
    | ((uint32_t)dev->RawBufPtr[5] >> 4)
  );
  BMx680_calib_t* calib = (BMx680_calib_t*)dev->CalibPtr;
  int32_t var1 = ((int32_t)(adcTemperature >> 3) - ((int32_t)calib->par_t1 << 1));
  int32_t var2 = (var1 * calib->par_t2) >> 11;
  int32_t var3 = ((((var1 >> 1) * (var1 >> 1)) >> 12)
                  * ((int32_t)calib->par_t3 << 4)) >> 14;
  tFine = var2 + var3;
  dev->Results.temperature = (tFine * 5 + 128) >> 8;
}




// -------------------------------------------------------------
static uint32_t bmx680_CompensatePressure(BMxX80_TypeDef* dev) {
  uint32_t adcPressure = (
      ((uint32_t)dev->RawBufPtr[0] << 12)
    | ((uint32_t)dev->RawBufPtr[1] << 4)
    | ((uint32_t)dev->RawBufPtr[2] >> 4)
  );
  BMx680_calib_t* calib = (BMx680_calib_t*)dev->CalibPtr;
  int32_t var1 = (tFine >> 1) - 64000;
  int32_t var2 = (((((var1 >> 2) * (var1 >> 2)) >> 11) * calib->par_p6) >> 2);
  var2 += (var1 * calib->par_p5) << 1;
  var2 = (var2 >> 2) + ((int32_t)calib->par_p4 << 16);
  var1 = (((((var1 >> 2) * (var1 >> 2)) >> 13)
           * ((int32_t)calib->par_p3 << 5)) >> 3)
         + ((calib->par_p2 * var1) >> 1);
  var1 >>= 18;
  var1 = ((32768 + var1) * calib->par_p1) >> 15;
  if (var1 == 0) return (0U);

  int32_t pressure = (int32_t)(1048576U - adcPressure);
  pressure = (int32_t)((pressure - (var2 >> 12)) * 3125U);
  pressure = (pressure >= 0x40000000)
    ? (pressure / var1) << 1
    : (pressure << 1) / var1;

  var1 = (calib->par_p9
          * (int32_t)(((pressure >> 3) * (pressure >> 3)) >> 13)) >> 12;
  var2 = ((pressure >> 2) * calib->par_p8) >> 13;
  int32_t var3 = (
      (pressure >> 8) * (pressure >> 8) * (pressure >> 8) * calib->par_p10
    ) >> 17;
  pressure += (
      var1 + var2 + var3 + ((int32_t)calib->par_p7 << 7)
    ) >> 4;
  return (pressure > 0) ? (uint32_t)pressure : 0U;
}




// -------------------------------------------------------------
static uint32_t bmx680_CompensateHumidity(BMxX80_TypeDef* dev) {
  uint16_t adcHumidity = (uint16_t)(
    ((uint16_t)dev->RawBufPtr[6] << 8) | dev->RawBufPtr[7]
  );
  BMx680_calib_t* calib = (BMx680_calib_t*)dev->CalibPtr;
  int32_t temperature = ((tFine * 5) + 128) >> 8;
  int32_t var1 = (int32_t)adcHumidity - ((int32_t)calib->par_h1 * 16)
    - (((temperature * calib->par_h3) / 100) >> 1);
  int32_t var2 = (calib->par_h2
    * (((temperature * calib->par_h4) / 100)
      + ((((temperature * ((temperature * calib->par_h5) / 100)) >> 6) / 100))
      + (1 << 14))) >> 10;
  int32_t var3 = var1 * var2;
  int32_t var4 = (((int32_t)calib->par_h6 << 7)
    + ((temperature * calib->par_h7) / 100)) >> 4;
  int32_t var5 = ((var3 >> 14) * (var3 >> 14)) >> 10;
  int32_t var6 = (var4 * var5) >> 1;
  int32_t humidity = (((var3 + var6) >> 10) * 1000) >> 12;

  if (humidity < 0) return (0U);
  if (humidity > 100000) return (100000U);
  return (uint32_t)humidity;
}
