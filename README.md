## Synopsis

EconetClock2 is the ATtiny45-20 firmware source code for the EconetClock2 project.

## Motivation

This project creates an Acorn Econet clock suitable for use with Acorn computers from the 1980s and 90s.  The Acorn Econet network is an EIA-422-B based network that is driven by a differential clock signal.

There are several designs for Econet clocks available on the Internet and even a few commercial providers of clocks, however most designs are based-on (or derived from) Acorn’s original design and require a complex, high-component count circuit in order to operate.

The clock created by this project consists of only 3 primary components (an ATtiny45-20 microcontroller, a capacitor and a resistor) and is therefore much easier to build.

## Installation

Note: This is an AVR Studio 7 project that can be loaded and compiled by the IDE

Please see http://www.waitingforfriday.com/?p=19 for detailed documentation about EconetClock2

## Author

EconetClock2 is written and maintained by Simon Inns.

## License

    EconetClock2 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    EconetClock2 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with EconetClock2. If not, see <http://www.gnu.org/licenses/>.