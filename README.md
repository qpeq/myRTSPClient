# myRTSPClient: A RTSP client built upon live555

## Prerequisite

cmake 3.16 or later

## Build and Run

1. Checkout the project with submodules:
```shell
$ git clone https://github.com/qpeq/myRTSPClient.git --recursive
```

2. create a build folder under myRTSPClient:
```shell
$ cd myRTSPClient && mkdir build && cd build
```
3. Configure and build with cmake:
```shell
$ cmake .. && cmake --build .
```
4. Run the program with the test url:
```shell
$ cd build
```
```shell
$ ./myRTSPClient rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov
```

To exit the program, just press ctrl-c

## Related Projects

* [live555-cmake](https://github.com/zzu-andrew/live555-cmake)

## License

`myRTSPClient` is released under the MIT License. Use of this source code is governed by
a MIT License that can be found in the LICENSE file.
