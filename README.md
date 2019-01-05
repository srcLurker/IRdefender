# IR Defender

Use an IR LED to defend an area from laser tag players. It can attach to
the local WiFi and provide a web page for control. It
uses the laser tag firing pattern to attack players within range of the
IR transmitter.

The project uses a WiFi IR blaster and is compiled using the Arduino SDK.

## Getting Started

There are three directories:

* IRdefender - Code which defends an area and provides a web page for control.
* IRrecvDemo - Code to determine the protocol of the laser tag.
* IRsendDemo - Code to test out the firing code found in IRrecvDemo.

### Prerequisites

This project used a
[WiFi IR Blaster from Sparkfun](https://www.sparkfun.com/products/15031)
and the
[Arduino SDK](https://www.arduino.cc/en/Main/Software). Sparkfun has a
[tutorial](https://learn.sparkfun.com/tutorials/sparkfun-wifi-ir-blaster-hookup-guide?_ga=2.67321752.863144658.1545789012-1566644384.1545558054)
which can be used for the initial installation.


## Deployment

Use the Arduino IDE and SDK with the ESP8266 library addon for deployment.

## Contributing

Please send pull requests for updates or changes.

## Authors

* **Charles Spirakis** - *Initial work*

## License

This project is licensed under the LGPL License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

* Thanks to all the people who made Arduino and the Arduino IDE available.
* Thanks to SparkFun for making it easy to get started with embedded computing.
