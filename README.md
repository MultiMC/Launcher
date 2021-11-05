# **This is a "cracked" version of a popular Minecraft launcher that lets you play the game without a Mojang account.**

# This software is not related to MultiMC developers and provided without any warranty. Please don't bomb MultiMC developers if something gets wrong using this launcher.

## Pre-built binaries:
- Windows / Linux / MacOS: https://nightly.link/AfoninZ/MultiMC5-Cracked/workflows/main/develop

## How to install and use
1. Download pre-built binaries for your system.
2. Unpack them in your desired directory.
3. Launch `DevLauncher`.
4. Go to account settings, and you will be requested to use email and password.
5. As your email use your username, and then type a random password or use ElyBy account.
6. Save it.
7. Now enjoy the Launcher.

In case if you are using MacOS/Linux then additional step is to make a script `DevLauncher` executable by using the command `chmod +x DevLauncher` in the terminal

To remove the "localhost:nnnn" or "ElyBy" in the minecraft main menu and F3, add `-Dauthlibinjector.noShowServerName` to the java arguments.

# Details about the ORIGINAL launcher below:

MultiMC 5
=========

MultiMC is a custom launcher for Minecraft that focuses on predictability, long term stability and simplicity.

## Development
If you want to contribute, talk to us on [Discord](https://discord.gg/multimc) (this is the official MultiMC discord, do NOT talk about the cracked version here) first.

While blindly submitting PRs is definitely possible, they're not necessarily going to get accepted.

We aren't looking for flashy features, but expanding upon the existing feature set without distruption or endangering future viability of the project is OK.

### Building
If you want to build MultiMC yourself, check [BUILD.md](BUILD.md) for build instructions.

### Code formatting
Just follow the existing formatting.

In general, in order of importance:
* Make sure your IDE is not messing up line endings or whitespace and avoid using linters.
* Prefer readability over dogma.
* Keep to the existing formatting.
* Indent with 4 space unless it's in a submodule.
* Keep lists (of arguments, parameters, initializers...) as lists, not paragraphs. It should either read from top to bottom, or left to right. Not both.


## Translations
Translations can be done [on crowdin](https://translate.multimc.org) (this link is for the original MultiMC launcher). Please avoid making direct pull requests to the translations repository.

## Forking/Redistributing/Custom builds policy
We keep MultiMC open source because we think it's important to be able to see the source code for a project like this, and we do so using the Apache license.

Part of the reason for using the Apache license is that we don't want people using the "MultiMC" name when redistributing the project. This means people must take the time to go through the source code and remove all references to "MultiMC", including but not limited to the project icon and the title of windows, (no *MultiMC-fork* in the title).

Apache covers reasonable use for the name - a mention of the project's origins in the About dialog and the license is acceptable. However, it should be abundantly clear that the project is a fork *without* implying that you have our blessing.


## License
Copyright &copy; 2013-2021 MultiMC Contributors

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this program except in compliance with the License. You may obtain a copy of the License at [http://www.apache.org/licenses/LICENSE-2.0](http://www.apache.org/licenses/LICENSE-2.0).

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
