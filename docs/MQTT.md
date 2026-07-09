# MQTT

InfoTerm can display up to twelve configurable MQTT values.

Each MQTT value supports:

- Enabled/disabled state
- Display label
- MQTT topic
- Unit
- Last received value

## Example Values

| Label | Topic | Unit |
| --- | --- | --- |
| Temperature | `home/weather/temperature` | `°C` |
| Humidity | `home/weather/humidity` | `%` |
| PV Power | `home/energy/pv_power` | `W` |
| Battery | `home/energy/battery_soc` | `%` |
| Grid | `home/energy/grid_power` | `W` |
| Water | `home/tank/water_level` | `%` |

## Weather Values from MQTT

Weather-related values can be provided through MQTT. If MQTT weather values are unavailable, the firmware can fall back to the configured standard weather provider.

## Notes

- Use retained messages on the broker if values should appear immediately after reboot.
- Keep payloads simple where possible, for example `23.4` or `850`.
- Units are configured in InfoTerm and do not have to be part of the MQTT payload.
