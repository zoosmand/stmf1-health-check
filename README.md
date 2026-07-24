# Health Check Device

## STM32F103C8

### Features

* Straightforward peripheral initialization
* TCP command service on port 5005

### TCP commands

Commands may be terminated with `CR`, `LF`, or `CRLF`.

| Command | Response |
| --- | --- |
| `get_tmpr` | `OK <latest-temperature> [<latest-temperature>]\r\n` |
| Unknown command | `ERR unknown_command\r\n` |

The `get_tmpr` command returns the most recent periodic measurement without
starting a new conversion. Until the first measurement is available, it returns
`ERR temperature_unavailable\r\n`.

Example using Netcat, with the device at `192.168.1.10`:

```console
$ printf 'get_tmpr\r\n' | nc 192.168.1.10 5005
OK 23.50 24.06
```

---

&copy; 2017-2025, Askug Ltd., Dmitry Slobodchikov
