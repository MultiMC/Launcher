Launcher
=========

Launcher is a custom launcher for Minecraft that allows you to easily manage multiple installations of Minecraft at once. It also allows you to easily install and remove mods by simply dragging and dropping.


## Development
The project uses C++ and Qt5 as the language and base framework. This might seem odd in the Minecraft community, but allows using 25MB of RAM, where other tools use an excessive amount of resources for no reason.

We can do more, with less, on worse hardware and leave more resources for the game while keeping the launcher running and providing extra features.

### Building
If you want to build Launcher yourself, check [BUILD.md](BUILD.md) for build instructions.

### Code formatting
Just follow the existing formatting.

In general:
* Indent with 4 space unless it's in a submodule
* Keep lists (of arguments, parameters, initializators...) as lists, not paragraphs.
* Prefer readability over dogma.


## Forking/Redistributing
We keep Launcher open source because we think it's important to be able to see the source code for a project like this, and we do so using the Apache license.

Part of the reason for using the Apache license is we don't want people using the "MultiMC" (the original project) name when redistributing the project. This means people must take the time to go through the source code and remove all references to "MultiMC", including but not limited to the project icon and the title of windows, (no *MultiMC-fork* in the title).

Apache covers reasonable use for the name - a mention of the project's origins in the About dialog and the license is acceptable. However, it should be abundantly clear that the project is a fork *without* implying that you have our blessing.


## License
Copyright &copy; (year goes here) Launcher Contributors

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this program except in compliance with the License. You may obtain a copy of the License at [http://www.apache.org/licenses/LICENSE-2.0](http://www.apache.org/licenses/LICENSE-2.0).

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
