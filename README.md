sailfish-browser
================
Sailfish Browser is web browser for Jolla's Sailfish OS and is shipping with Jolla device. Sailfish Browser uses Sailfish Silica Qt components for the browser chrome and gecko engine with embedlite Qt5 binding.
More information about the architecture can be found from http://blog.idempotent.info/posts/whats-behind-sailfish-browser.html

Maintainers
-----------
- Dmitry Rozhkov (rojkov)
- Raine Mäkeläinen (rainemak)
- Vesa-Matti Hartikainen (veskuh)

Engine and adaptation
---------------------
- QtMozEmbed - Qt bindings - https://github.com/tmeshkova/qtmozembed
embedlite components - 
- Embedding Compontents - https://github.com/tmeshkova/embedlite-components
- Gecko with embedlite - browser engine with embedding API - https://github.com/tmeshkova/gecko-dev/tree/embedlite/embedding/embedlite  

Tools
-----
All tools are located in source tree under [tools](https://github.com/sailfishos/sailfish-browser/tree/master/tools).

#### [memory-dump-reader](https://github.com/sailfishos/sailfish-browser/tree/master/tools/memory-dump-reader)

Memory dump reader is a simple desktop utility for dumping and collecting memory information of the Sailfish Browser.
Current version of the memory-dump-reader is a work-in-progress version.

##### Compilation

- Change directory to the tools/memory-dump-reader
- \<qmake-bin-path\>/qmake
- make

##### Reading and collecting

Once memory-dump-reader is compiled, run it like: ```dumpMemoryInfo /tmp/fileName.gz ip-of-the-device```.
The script dumps remotely memory information of the browser and copies the dump to the desktop.
The ```dumpMemoryInfo``` script works best when you have added your public ssh key as an authorized key of the device.

License
-------
The browser is open source and licensed under Mozilla Public License v2.0 (http://www.mozilla.org/MPL/2.0/).

Wiki
----
For more information see wiki: https://github.com/sailfishos/sailfish-browser/wiki/Sailfish-Browser-wiki
