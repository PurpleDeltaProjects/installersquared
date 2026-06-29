# InstallerSquared

InstallerSquared is an application that helps you to find, download, and install applications in a simple and fast way.

## Features
- Choose from a large selection of applications with a graphical user interface.
- Search for, sort, and filter to find applications quickly.
- Install all chosen applications with one click.
- Create installers that install all chosen apps when run, and can be reused and shared with others. (Coming Soon)

## Overview
This repository contains:

- `website/` - The site which contains the download for the application (Coming Soon), as well as hosts the data the client uses to install apps.
- `client/` - The code for the client itself, along with a few python scripts that helped with the development of it.

## Requirements

### Runtime
- Windows 10 version 1809 or newer or Windows 11

### Developing/Compiling
- Python 3 (for the build script)
- MinGW (g++) installed and in PATH
- [webview/webview](https://github.com/webview/webview)
- [Json11](https://github.com/dropbox/json11) with json11.cpp placed in the dependencies folder
