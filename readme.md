__# Clarius Cast Plugin

This plugin integrates Clarius probes into the [ImFusion Suite](imfusion.com) using the [Clarius Cast API](https://github.com/clariusdev/cast). 

### Dependencies
The dependencies of the Clarius Cast libraries can be installed on ubuntu using apt.

#### Ubuntu 20.04
```bash
sudo apt install libegl1 libc6 libglib2.0-0 libdbus-1-3 libicu66
```

#### Ubuntu 22.04
```bash
sudo apt install libegl1 libc6 libglib2.0-0 libdbus-1-3 libicu70
```


### Build Instructions
```
mkdir build
cd build
cmake ..
make
```
