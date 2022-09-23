sailfish-browser
================
Sailfish Browser is web browser for Sailfish OS and is shipping with Sailfish OS devices. Sailfish Browser uses Sailfish Silica Qt components for the browser chrome and gecko engine with embedlite Qt5 binding.
More information about the architecture can be found from https://web.archive.org/web/20180830103541/http://blog.idempotent.info/posts/whats-behind-sailfish-browser.html . Feel free to ask questions on Forum from Maintainers.

Maintainers
-----------
- Raine Mäkeläinen (rainemak)
- David Llewellyn-Jones (llewelld)

Engine and adaptation
---------------------
- Sailfish WebView - https://github.com/sailfishos/sailfish-components-webview
- QtMozEmbed - Qt bindings - https://github.com/sailfishos/qtmozembed
- Embedlite components - https://github.com/sailfishos/embedlite-components
- Gecko browser engine with embedlite API - https://github.com/sailfishos/gecko-dev

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
