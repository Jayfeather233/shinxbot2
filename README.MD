# shinxBot2

A (multi) bot management framework for Tencent QQ, based on [Lagrange.Onebot](https://github.com/LagrangeDev/Lagrange.Core), restructured from [shinxBot](https://github.com/Jayfeather233/shinxBot).

[Documents](./README.md)/[文档](./README_ZH.md)

### File Structure

```plaintext
./src/functions        # Code for all features
./src/inc              # Header files for functions
./src/interfaces       # Interfaces, all functions inherit this class
./src/utils            # Various utilities
./lib/functions/       # Dynamic library directory for bot functions
./lib/events/         # Dynamic library directory for bot events
```

### Running the Bot

1. Download and extract from Release, then run `shinxbot`.
2. Clone the repository and execute `docker compose up -d` in the root directory for automatic configuration and compilation into the executable `./build/shinxbot`.

   Use `docker exec -it shinx-bot bash` to enter the container terminal and run inside the container.
3. After compiling on bare metal, run `./build/shinxbot`, see the next section for details.

Before running, ensure the Onebot 11 standard backend is running, and input the reporting port and API port for the Onebot 11 backend after running `shinxbot`.

You can use `start.sh` to run shinxbot in the background.

### Compiling from Source

Since it includes the `base64` submodule, please enable `--recursive` when cloning the repository.

```sh
git clone --recursive git@github.com:Jayfeather233/shinxbot2.git
```

This project depends on the following packages:

- libcurl
- openssl-dev / libssl-dev
- libjsoncpp
- libzip
- ImageMagick++ (version 7+)
- libmagick++-dev

Refer to `entrypoint.sh` for specific installation methods.

- **Linux**: Execute the automatic compilation script `build.sh`
- **Windows**: You need to change TCP connection headers and handle OS-level differences such as sleep, file:///, etc. (not verified), recommended to run in a container.

### Adding a Bot

You can refer to `mybot` in `./src/bots`, which is a class already connected to Onebot 11.

- All bots must inherit from the `bot` class in `./src/interfaces/bot.h`.
- Write the sending/receiving ports in `./config/port.txt`.
- Add `add_new_bot(new yourbot(receive_port[id], send_port[id]));` in `main()` of `./src/main.cpp`.

### Adding Features

Using `class mybot` as an example, the function parameters only relate to messages, and the events accept JSON data. Call methods from functions when receiving chat messages and use methods from events for other prompt messages.

1. Create a new directory in `./src/functions`, place your code there, write a class that inherits from the class in `./src/interfaces/processable.h`, implement the virtual methods, and provide a `create` method to obtain a pointer of type `processable*` (see `functions/getJapaneseImage/Anime_Img.cpp` for reference).
2. If the check() method in the inherited virtual method succeeds, the process() method will be executed.
3. A CMake template is provided in `generate_cmake.py` to compile your functionality, automatically generating the dynamic library in `./lib/functions/`.
4. Start the bot. If the bot is running, you can use the admin account (from `op_list.json`) to send `bot.load [function|event] name` to load it.

Before running the bot, ensure that the dynamic libraries for functions are placed in `./lib/functions | ./lib/events`, or generate them using `generate_cmake.py & make_all.sh`.

### Features Supported

- Stinky Number Argument Validator, from: [itorr](https://github.com/itorr/homo)
- Random Anime Images. Source: [dmoe](https://www.dmoe.cc)
- What's this short for?, from: [itorr](https://github.com/itorr/nbnhhsh)
- Generate fake forwarding messages in group or private chats
- Random Color Palette
- Cyber Cat Raising (co-created with GPT-3.5)
- Cat image for HTTP code. Source: [httpcats](https://httpcats.com/)
- Image OCR (embedded with QQ)
- Image Processing: Symmetry, Rotation, Kaleidoscope
- OpenAI's `gpt-3.5-turbo` API support
- Google's `gemini-pro[-vision]` API support, [url](https://ai.google.dev/docs)
- StableDiffusion's `SDXL-Turbo` API support, [url](https://sdxlturbo.ai/)
- Memes storage
- Most Active User Reminders
- Group member change notifications
- Echo
- Forward messages between different group chats and private chats
- Graylist Management
- Manage multiple bots, with future capability to broadcast messages

### Tips for Development

1. If using clang, the `.clang-format` file in the root directory provides automatic formatting settings.
2. `sender.sh` in `dev_tools` can be used to send messages to shinxbot, while `inspector` captures data sent from shinxbot to go-cqhttp. `inspector` cannot run simultaneously with go-cqhttp. Both serve as equivalents to go-cqhttp, providing convenience for developers unable to run it. (You may need to modify paths in `inspector.cpp` and `sender.sh` to point to the `config` file.)

For example:

```bash
bash sender.sh "bot.help"
```