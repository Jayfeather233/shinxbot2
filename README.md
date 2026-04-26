# shinxBot2

A bot framework based on [Lagrange.Onebot](https://github.com/LagrangeDev/Lagrange.Core), restructured from [shinxBot](https://github.com/Jayfeather233/shinxBot).

This respositry only contains framework. For functions(plugins), please see [shinxbot2-plugins](https://github.com/Jayfeather233/shinxbot2-plugins)

[Documents](./README.md)/[文档](./README_ZH.md)

### File Structure

```plaintext
./src/inc              # Header files for functions
./src/interfaces       # Interfaces, all functions inherit this class
./src/utils            # Various utilities
./lib/functions/       # Dynamic library directory for bot functions
./lib/events/          # Dynamic library directory for bot events
```

### Running the Bot

**Note: this section only compile and run the framework, without any functions**

This project depends on the following packages:

- openssl-dev / libssl-dev
- libjsoncpp
- libzip
- ImageMagick++ (version 7+)
- libmagick++-dev
- libfmt-dev (version 8+)

For methods other than running with Docker, please install dependencies first. For specific installation methods, refer to `Dockerfile`.

No matter what method you use, before running, ensure the Onebot 11 standard backend is running. After running `shinxbot`, input the reporting port and API port for the Onebot 11 backend, or write the sending/receiving ports in `./config/port.txt`.(And add mapping port in `docker-compose`)

Here are three ways to run the bot (direct execution is not recommended due to potential version inconsistencies of dependency libraries):

1. Clone the repository `git clone --recursive git@github.com:Jayfeather233/shinxbot2.git` and execute `docker compose up -d` in the root directory for automatic configuration and compilation into the executable `./shinxbot`.

   Use `docker exec -it shinx-bot bash` to enter the container terminal and run inside the container.

   This method does not compile functions; refer to the **Compiling from Source** section to compile features.

   config the port in `./config/port.txt`.

   Then run `./start.sh` to start running.

2. After compiling on bare metal, run `./shinxbot`. See **Compiling from Source**.

3. Download and extract from Release, then run `shinxbot`. (Out dated release)

You can use `start.sh` to run shinxbot in the background.

### Compiling from Source

Since it includes submodule(s), please enable `--recursive` when cloning the repository.

```sh
git clone --recursive git@github.com:Jayfeather233/shinxbot2.git
```

- **Linux**: Execute the automatic compilation script `build.sh` (`build.sh main` builds everything by default; `build.sh simple` skips rebuilding `libutils`).

- **Windows**: You need to change TCP connection headers and handle OS-level differences such as sleep, file:///, etc. (not verified); running in a container is recommended.

Note: This only compiles the bot framework. For plugins see: [shinxbot2-plugins](https://github.com/Jayfeather233/shinxbot2-plugins)

### Connecting Other Bot Backends

It is not supported to add multiple bots in one program; please clone another copy.

You can refer to `shinxbot` in `./src/bots`, which is a class already connected to Onebot 11.

- The bot must inherit from the `bot` class in `./src/interfaces/bot.h`.
- Write the new bot's sending/receiving ports in `./config/port.txt`.

### Adding Features

See: [shinxbot2\-plugins](https://github.com/Jayfeather233/shinxbot2-plugins).

### Tips for Development

1. If using clang, the `.clang-format` file in the root directory provides automatic formatting settings.

2. `tools/dev_tools/sender.sh` can be used to send messages to shinxbot, while `inspector` captures data sent from shinxbot to Onebot 11. `inspector` cannot run simultaneously with Onebot 11. Both serve as equivalents to Onebot 11, providing convenience for developers unable to run it. (You may need to modify paths in `inspector.cpp` and `sender.sh` to point to the `config` file.)

   For example:

```bash
bash tools/dev_tools/sender.sh "bot.help"
```

Backward compatibility: `src/dev_tools/sender.sh` remains as a wrapper.

## License

This project is licensed under the [LGPLv3](./LICENSE). If you modify the code of this project, you should also use the LGPL license.

### Add-on Policy

For functionalities that simply write a function/event without modifying the code of this project, any license may be used.