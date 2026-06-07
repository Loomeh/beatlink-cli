# beatlink-cli
A project for accessing [gridLeak](https://github.com/grid-leak) servers in Mirror's Edge Catalyst on Linux.

Made for the [Beat Revival](https://beatrevival.me) project.
Most functionality is directly ported from [BeatLink](https://github.com/synthic/BeatLink).

## Usage
Just run the application directly. Either from your desktop environment, or from the command line.

In the event that the beatlink-cli isn't working properly, you can use the `-debug` command line argument to get lots of additional debug output. This output will also be very useful if asking for support in the [Beat Revival Discord](https://discord.gg/FGftmuRrrG):
```
./beatlink-cli -debug
```
>[!WARNING]
>Do NOT post your authentication token publicly when pasting the debug output!

## Building
```
git clone https://github.com/Loomeh/beatlink-cli.git
cd beatlink-cli
cmake -B build -S .
cmake --build build --config Release
```
beatlink-cli pulls the sources for OpenSSL and crypto++ on building so that they can easily be statically linked with beatlink-cli. However, this means that your first time building will probably be pretty slow. You can speed up the build process by asking CMake to use all of your CPU's cores when building:
```
cmake --build build --config Release -j$(nproc)
```
