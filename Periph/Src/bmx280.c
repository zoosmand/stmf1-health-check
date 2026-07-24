/**
  ******************************************************************************
  * @file           : bmx280.c
  * @brief          : Bosch BMP280/BME280 sensor implementation.
  ******************************************************************************
  */

#include "bmx280.h"
#include "i2c.h"
#include "main.h"

static int32_t tFine;

static ErrorStatus bmx280_Send(BMxX80_TypeDef*, uint8_t, uint8_t);
static ErrorStatus bmx280_Receive(BMxX80_TypeDef*, uint8_t, uint8_t);
static ErrorStatus bmx280_WaitNvmCopy(BMxX80_TypeDef*);
static ErrorStatus bmx280_WaitMeasurement(BMxX80_TypeDef*);
static int32_t bmx280_CompensateTemperature(BMxX80_TypeDef*);
static uint32_t bmx280_CompensatePressure(BMxX80_TypeDef*);
static uint32_t bmx280_CompensateHumidity(BMxX80_TypeDef*);




// -------------------------------------------------------------
ErrorStatus BMx280_Init(BMxX80_TypeDef* dev) {
  if ((dev == NULL) || (dev->I2Cx == NULL) || (dev->RawBufPtr == NULL)
      || (dev->CalibPtr == NULL) || (dev->Lock != DISABLE)) {
    return (ERROR);
  }
  dev->Lock = ENABLE;
  ErrorStatus status = ERROR;

  if (bmx280_Receive(dev, BMX280_DEV_ID, 1U) != SUCCESS) goto done;
  dev->DevID = dev->RawBufPtr[0];
  if ((dev->DevID != BMP280_ID) && (dev->DevID != BME280_ID)) goto done;

  if (bmx280_Send(dev, BMX280_RESET, BMX280_RESET_VALUE) != SUCCESS) goto done;
  if (bmx280_WaitNvmCopy(dev) != SUCCESS) goto done;

  if (bmx280_Receive(dev, BMX280_CALIB1, 26U) != SUCCESS) goto done;
  BMx280_calib_t* calib = (BMx280_calib_t*)dev->CalibPtr;
  calib->dig_t1 = (uint16_t)((dev->RawBufPtr[1] << 8) | dev->RawBufPtr[0]);
  calib->dig_t2 = (int16_t)((dev->RawBufPtr[3] << 8) | dev->RawBufPtr[2]);
  calib->dig_t3 = (int16_t)((dev->RawBufPtr[5] << 8) | dev->RawBufPtr[4]);
  calib->dig_p1 = (uint16_t)((dev->RawBufPtr[7] << 8) | dev->RawBufPtr[6]);
  calib->dig_p2 = (int16_t)((dev->RawBufPtr[9] << 8) | dev->RawBufPtr[8]);
  calib->dig_p3 = (int16_t)((dev->RawBufPtr[11] << 8) | dev->RawBufPtr[10]);
  calib->dig_p4 = (int16_t)((dev->RawBufPtr[13] << 8) | dev->RawBufPtr[12]);
  calib->dig_p5 = (int16_t)((dev->RawBufPtr[15] << 8) | dev->RawBufPtr[14]);
  calib->dig_p6 = (int16_t)((dev->RawBufPtr[17] << 8) | dev->RawBufPtr[16]);
  calib->dig_p7 = (int16_t)((dev->RawBufPtr[19] << 8) | dev->RawBufPtr[18]);
  calib->dig_p8 = (int16_t)((dev->RawBufPtr[21] << 8) | dev->RawBufPtr[20]);
  calib->dig_p9 = (int16_t)((dev->RawBufPtr[23] << 8) | dev->RawBufPtr[22]);
  if (calib->dig_p1 == 0U) goto done;

  if (dev->DevID == BME280_ID) {
    calib->dig_h1 = dev->RawBufPtr[25];
    if (bmx280_Receive(dev, BMX280_CALIB2, 7U) != SUCCESS) goto done;
    calib->dig_h2 = (int16_t)((dev->RawBufPtr[1] << 8) | dev->RawBufPtr[0]);
    calib->dig_h3 = dev->RawBufPtr[2];
    calib->dig_h4 = (int16_t)(
      ((int16_t)(int8_t)dev->RawBufPtr[3] * 16)
      | (dev->RawBufPtr[4] & 0x0FU)
    );
    calib->dig_h5 = (int16_t)(
      ((int16_t)(int8_t)dev->RawBufPtr[5] * 16)
      | (dev->RawBufPtr[4] >> 4)
    );
    calib->dig_h6 = (int8_t)dev->RawBufPtr[6];
  }

  if (bmx280_Send(
        dev,
        BMX280_SETTINGS,
        BMX280_CONFIG_INACTIVE_250 | BMX280_CONFIG_FILTER_4
      ) != SUCCESS) goto done;
  if ((dev->DevID == BME280_ID)
      && (bmx280_Send(dev, BMX280_CTRL_HUM, BMX280_HUMIDITY_OVS_X4) != SUCCESS)) {
    goto done;
  }
  if (bmx280_Send(
        dev,
        BMX280_CTRL_MEAS,
        BMX280_TEMPERATURE_OVS_X4 | BMX280_PRESSURE_OVS_X4 | BMX280_SLEEP_MODE
      ) != SUCCESS) goto done;

  status = SUCCESS;

done:
  dev->Lock = DISABLE;
  return (status);
}




// -------------------------------------------------------------
ErrorStatus BMx280_Measurement(BMxX80_TypeDef* dev) {
  if ((dev == NULL) || (dev->Lock != DISABLE)) return (ERROR);
  dev->Lock = ENABLE;
  ErrorStatus status = ERROR;

  if (bmx280_Send(
        dev,
        BMX280_CTRL_MEAS,
        BMX280_TEMPERATURE_OVS_X4 | BMX280_PRESSURE_OVS_X4 | BMX280_FORCE_MODE
      ) != SUCCESS) goto done;

  if (bmx280_WaitMeasurement(dev) != SUCCESS) goto done;

  uint8_t dataLength = (dev->DevID == BME280_ID) ? 8U : 6U;
  if (bmx280_Receive(dev, BMX280_DATA, dataLength) != SUCCESS) goto done;
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

  dev->Results.temperature = bmx280_CompensateTemperature(dev);
  dev->Results.pressure = bmx280_CompensatePressure(dev);
  if (dev->DevID == BME280_ID) {
    uint32_t humidityQ22_10 = bmx280_CompensateHumidity(dev);
    dev->Results.humidity = ((humidityQ22_10 * 1000U) + 512U) / 1024U;
  } else {
    dev->Results.humidity = 0U;
  }

  status = SUCCESS;

done:
  dev->Lock = DISABLE;
  return (status);
}




// -------------------------------------------------------------
static ErrorStatus bmx280_Send(BMxX80_TypeDef* dev, uint8_t reg, uint8_t value) {
  uint8_t buffer[2] = {reg, value};
  return I2C_Master_Send(dev->I2Cx, dev->I2C_Address, buffer, sizeof(buffer));
}




// -------------------------------------------------------------
static ErrorStatus bmx280_Receive(BMxX80_TypeDef* dev, uint8_t reg, uint8_t length) {
  return I2C_Master_ReadRegister(
    dev->I2Cx,
    dev->I2C_Address,
    reg,
    dev->RawBufPtr,
    length
  );
}




// -------------------------------------------------------------
static ErrorStatus bmx280_WaitNvmCopy(BMxX80_TypeDef* dev) {
  for (uint8_t attempts = 0U; attempts < 5U; attempts++) {
    _delay_ms(2U);
    if (bmx280_Receive(dev, BMX280_STATUS, 1U) != SUCCESS) return (ERROR);
    if ((dev->RawBufPtr[0] & BMX280_IM_UPDATE) == 0U) return (SUCCESS);
  }
  return (ERROR);
}




// -------------------------------------------------------------
static ErrorStatus bmx280_WaitMeasurement(BMxX80_TypeDef* dev) {
  for (uint16_t attempts = 0U; attempts < 250U; attempts++) {
    _delay_ms(1U);
    if (bmx280_Receive(dev, BMX280_STATUS, 1U) != SUCCESS) return (ERROR);
    if ((dev->RawBufPtr[0] & BMX280_MEASURING) == 0U) return (SUCCESS);
  }
  return (ERROR);
}




// -------------------------------------------------------------
static int32_t bmx280_CompensateTemperature(BMxX80_TypeDef* dev) {
  int32_t adcTemperature = (
      ((int32_t)dev->RawBufPtr[3] << 12)
    | ((int32_t)dev->RawBufPtr[4] << 4)
    | ((int32_t)dev->RawBufPtr[5] >> 4)
  );
  BMx280_calib_t* calib = (BMx280_calib_t*)dev->CalibPtr;
  int32_t var1 = ((((adcTemperature >> 3) - ((int32_t)calib->dig_t1 << 1)))
                  * (int32_t)calib->dig_t2) >> 11;
  int32_t var2 = (((((adcTemperature >> 4) - (int32_t)calib->dig_t1)
                    * ((adcTemperature >> 4) - (int32_t)calib->dig_t1)) >> 12)
                  * (int32_t)calib->dig_t3) >> 14;
  tFine = var1 + var2;
  return (tFine * 5 + 128) >> 8;
}




// -------------------------------------------------------------
static uint32_t bmx280_CompensatePressure(BMxX80_TypeDef* dev) {
  int32_t adcPressure = (
      ((int32_t)dev->RawBufPtr[0] << 12)
    | ((int32_t)dev->RawBufPtr[1] << 4)
    | ((int32_t)dev->RawBufPtr[2] >> 4)
  );
  BMx280_calib_t* calib = (BMx280_calib_t*)dev->CalibPtr;
  int32_t var1 = (tFine >> 1) - 64000;
  int32_t var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * calib->dig_p6;
  var2 += (var1 * calib->dig_p5) << 1;
  var2 = (var2 >> 2) + ((int32_t)calib->dig_p4 << 16);
  var1 = (((calib->dig_p3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3)
          + ((calib->dig_p2 * var1) >> 1)) >> 18;
  var1 = ((32768 + var1) * calib->dig_p1) >> 15;
  if (var1 == 0) return (0U);

  uint32_t pressure = ((uint32_t)(1048576 - adcPressure - (var2 >> 12))) * 3125U;
  pressure = (pressure < 0x80000000U)
    ? (pressure << 1) / (uint32_t)var1
    : (pressure / (uint32_t)var1) * 2U;
  var1 = (calib->dig_p9 * (int32_t)(((pressure >> 3) * (pressure >> 3)) >> 13)) >> 12;
  var2 = ((int32_t)(pressure >> 2) * calib->dig_p8) >> 13;
  return (uint32_t)((int32_t)pressure + ((var1 + var2 + calib->dig_p7) >> 4));
}




// -------------------------------------------------------------
static uint32_t bmx280_CompensateHumidity(BMxX80_TypeDef* dev) {
  int32_t adcHumidity = ((int32_t)dev->RawBufPtr[6] << 8) | dev->RawBufPtr[7];
  BMx280_calib_t* calib = (BMx280_calib_t*)dev->CalibPtr;
  int32_t humidity = tFine - 76800;
  humidity = (((((adcHumidity << 14) - ((int32_t)calib->dig_h4 << 20)
      - (calib->dig_h5 * humidity)) + 16384) >> 15)
    * (((((((humidity * calib->dig_h6) >> 10)
      * (((humidity * calib->dig_h3) >> 11) + 32768)) >> 10)
      + 2097152) * calib->dig_h2 + 8192) >> 14));
  humidity -= (((((humidity >> 15) * (humidity >> 15)) >> 7)
    * calib->dig_h1) >> 4);
  if (humidity < 0) humidity = 0;
  if (humidity > 419430400) humidity = 419430400;
  return (uint32_t)(humidity >> 12);
}
