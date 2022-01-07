# LED Wireless Controller for NodeMCU

Control 2 LED Strips (WS2812B) with a NodeMCU over WIFI with dynamic IP discovery.

## Config

Create a header file `deviceconfig.h` in the root directory.

If those defines are not present you will get a compiler error.

```cpp
#define DEVICE_NAME "LED Controller living room"
#define DEVICE_TYPE "ledv1"

#define STASSID "MYWIFI"
#define STAPSK "myWifiPassword"

#define NUM_LEDS 46
#define NUM_LEDS2 37
#define DATA_PIN D1  
#define DATA_PIN2 D2 
```

## UDP Discovery

Send `"getIoTs"` as ASCII bytes to the broadcast address via UDP on port 4301.

```dart
udpSocket.send(ascii.encode("getIoTs"), IPAdress("192.168.178.255"), 4301);
```

The device will answer with:

```
"I","O","T,"
"I","P",192,168,178,69,
"N","A","M","E",<name len>,"L","E","D"," ","C","o","n","t","r","o","l","l","e","r", ...
"T","Y","P",<type len>,"l","e","d","v","1"
```
... in binary.

## TCP Commands

Connect to the device via TCP at Port 4300.
```dart
tcpSocket = await Socket.connect(IPAdress("192.168.178.69"), 4300);
```

```
<getstate>
        returns the current state as JSON
<fullrgb,r,g,b>
<fullhsv,h,s,v>
<fullrainbow>
<circlerainbow,left|right,speed>
<stripes,r1,g1,b1,l1,r2,g2,b2,l2>
<settemp,temp>
        0: uncorrected
        1: tungsten 40W
        2: tungsten 100W
        3: halogen
        4: carbon arc
        5: high noon sun
        6: direct sun
        7: overcast
        8: clear blue sky
<setcc,cc>
        0: uncorrected
        1: typical LED strip
        2: typical pixel string
<setbright,brightness>
        0-255
```

### State retuned by `<getstate>`

```json
{
    "iotstate": {
            "c1": [0, 255, 226],
            "c2": [0, 0, 0],
            "mode": 3,
            "direction": 1,
            "speed": 1,
            "cc": 2,
            "temp": 1,
            "brightness": 255
    }
}
```