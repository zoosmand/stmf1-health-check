# Health Check Device

## STM32F103C8

### Features

* Straightforward peripheral initialization
* TCP command service on port 5005

### TCP commands

Commands may be terminated with `CR`, `LF`, or `CRLF`.
The server sends one response and then closes the connection. Sensor commands
return cached results from the periodic measurement service; they do not start
a new conversion.

| Command | Response |
| --- | --- |
| `get_sensors` | Number of registered sensors by measurement type |
| `get_model_X` | Model of physical sensor `X` |
| `health_X` | Cached availability and health of physical sensor `X` |
| `get_all_X` | All recent measurements from physical sensor `X` |
| `get_t_X` | Recent temperature from temperature sensor `X` |
| `get_p_X` | Recent pressure from pressure sensor `X` |
| `get_h_X` | Recent relative humidity from humidity sensor `X` |
| `get_tmpr` | Recent temperatures from all temperature sensors |
| Unknown command | `ERR unknown_command\r\n` |

Sensor numbers are one-based. `get_model_X`, `health_X`, and `get_all_X` use the
physical sensor list. The temperature, pressure, and humidity commands each use
their own capability-specific list. For example, `get_p_1` selects the first
sensor that provides pressure, even if that device is not physical sensor 1.
Physical indexes remain assigned to the same registered sensor. DS18B20 devices
are identified by their unique ROM addresses, so disconnecting one does not
renumber the others.

`get_sensors` returns counts in the following format:

```text
OK t:4,h:2,p:2,all:4
```

Here, `all` is the number of physical sensors. A sensor that provides multiple
measurement types is counted once in `all` and once in each applicable type.
Gas measurements are reserved for a future protocol extension.

Example responses:

```text
get_model_1 -> OK DS18B20
get_all_1   -> OK model:DS18B20,t:28.06
get_all_3   -> OK model:BME280,t:26.98,p:100099,h:46.419
get_t_1     -> OK 28.06
get_p_1     -> OK 100099
get_h_1     -> OK 46.419
```

`health_X` reads service metadata and does not communicate with the sensor. A
healthy response includes the model, time since the last successful measurement
in milliseconds, consecutive failure count, and last error:

```text
health_1 -> OK model:DS18B20,state:healthy,age_ms:2150,failures:0,error:none
```

The possible health states are:

| State | Meaning |
| --- | --- |
| `initializing` | Registered, but no successful measurement is available yet |
| `healthy` | The latest periodic measurement succeeded |
| `degraded` | One or two consecutive measurements failed |
| `failed` | Three or more consecutive measurements failed |
| `stale` | The last successful measurement is older than three service periods |
| `missing` | A previously registered DS18B20 is absent from the latest discovery |

Diagnostic errors currently include `none`, `not_ready`, `timeout`, `crc`,
`bus`, `missing`, and `conversion`. When a sensor has never produced a valid
measurement, the response contains `age_ms:unavailable`.

Temperature is expressed in degrees Celsius, pressure in pascals, and relative
humidity as a percentage. `get_tmpr` remains available for compatibility and
returns all recent temperatures in one response.

Possible error responses include:

| Response | Meaning |
| --- | --- |
| `ERR invalid_sensor_number` | The index is missing, zero, malformed, or too large |
| `ERR sensor_not_found` | The requested physical or capability-specific index does not exist |
| `ERR measurement_unavailable` | The sensor exists, but no valid recent measurement is available |
| `ERR temperature_unavailable` | No complete result is available for the legacy `get_tmpr` command |
| `ERR command_too_long` | The command exceeds the receive buffer |
| `ERR unknown_command` | The command name is not supported |

Example using Netcat, with the device at `192.168.1.10`:

```console
$ printf 'get_sensors\r\n' | nc 192.168.1.10 5005
OK t:4,h:2,p:2,all:4
$ printf 'get_all_3\r\n' | nc 192.168.1.10 5005
OK model:BME280,t:26.98,p:100099,h:46.419
```

---

&copy; 2017-2025, Askug Ltd., Dmitry Slobodchikov
