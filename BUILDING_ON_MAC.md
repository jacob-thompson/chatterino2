# Building on macOS

Chatterino2 is built in CI on Intel on macOS 13.
Local dev machines for testing are available on Apple Silicon on macOS 13.

## Installing dependencies

1. Install Xcode and Xcode Command Line Utilities
1. Start Xcode, go into Settings -> Locations, and activate your Command Line Tools
1. Install [Homebrew](https://brew.sh/#install)  
   We use this for dependency management on macOS
1. Install all dependencies:  
   `brew install boost openssl@3 rapidjson cmake qt@6`

## Building

### Building from terminal

1. Open a terminal
1. Go to the project directory where you cloned Chatterino2 & its submodules
1. Create a build directory and go into it:  
   `mkdir build && cd build`
1. Run CMake. To enable Lua plugins in your build add `-DCHATTERINO_PLUGINS=ON` to this command.  
   `cmake -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt@6 ..`
1. Build:  
   `make`

Your binary can now be found under bin/chatterino.app/Contents/MacOS/chatterino directory

## macOS Features

### Native Notifications

Chatterino2 on macOS uses the native UNUserNotificationCenter framework for notifications when a channel goes live. This provides a better user experience with:

- Native macOS notification appearance and behavior  
- System-wide notification settings integration
- Support for notification actions (clicking to open the channel)
- Automatic permission handling

The application will request notification permission on first launch when notifications are enabled in settings. You can manage notification permissions through:
- System Preferences → Notifications & Focus → Chatterino
- Or by right-clicking on the notification and selecting "Turn off notifications for Chatterino"

No additional configuration or entitlements are required - the UserNotifications framework is available to all applications on macOS 10.14+.

### Other building methods

You can achieve similar results by using an IDE like Qt Creator, although this is undocumented but if you know the IDE you should have no problems applying the terminal instructions to your IDE.
