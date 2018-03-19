# ESP8266 iot Multi-sensor and RF / IR transmitter.

## Fetures
- Connect to WiFi & MQTT.
- Subscribe to MQTT topics (LED / RF / IR)
- Publish sensor readings on change (Button / Motion / Temp / Light)

## MQTT topics:

All topics are prefixed by the value of `mqtt_topic_root` e.g. `mqtt_topic_root = "foxhat/iotmulti";`

### Subscriptions:
- /led `on` `off`
- /ir `NEC,32,16769056`
- /rf `24,1397077`

#### Publish:
- /button `0` `1`
- /motion `0` `1`
- /temp `0-999` WIP
- /light `0-999` WIP
