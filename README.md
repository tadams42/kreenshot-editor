kreenshot-editor
================

kreenshot-editor is incubating.

Build
-----
$ mkdir build; cd build; cmake ..
$ make

Run
---
in directory build:
$ ./src/kreenshot-editor

Component 1: main-application
-----------------------------
- command line options to give image and cursor (optional)
- show editor-component

Component 2: image-editor
-------------------------
- display image
- show toolbars for editing
    - define toolbars by creating "prototype items" with default settings
    - make it possible for each toolbar entry to save the current configuration
        - dropdown menu (show basic properties as string like line width, color etc)
- set (including an optional cursor) and get image

TODO
----
- write filename in title and full path in status bar (<path> [change...])
- save image to file
- save settings to file
- enable more item properties (see item.h/cpp)
- enable hover visual
- makes sure line has a bigger catch area when trying to move
- ToolManager: Draw item tool: text, highlight, obfuscate
- key movement of items

- crop etc. tool
- rename Item to KreenItem
- add context menu to select tools
- add handles for rect resizing and line repositioning (as extra class)
    - Ctrl for propertional
- Draw rect tool
- draw arrow
- click on empty space chooses Select tool
- The QGraphicsProxyWidget class provides a proxy layer for embedding a QWidget in a QGraphicsScene

- where to store data files like default background image?
- nullable QPoint for function parameter?
- is QImage reference counted?

TODO2
-----
- statt QScrollView kann man auch bei QGraphicsScene.setSceneRect verwenden. Was ist besser?

DEV
---
Debug with: $ kreenshot-editor ~/Pictures/screenshots/2014-05-04_Screenshot-1.png

TODO
----
- Select a Item
  - Handles for resizing appear
  - Controls for common properties appear
     - most recently used or pinned colors
     - slider for line width
     - slider for font size
     - slider for arrow tip typ?

Related tools
-------------
These Linux tools have similar image editor capabilities:
- greenshot (.NET) - the inspirational source for kreenshot-editor
- shutter (Perl)
- hotshots (QT)

DONE
----
- Item
- DrawingArea
- BackgroundImage
- On the DrawingArea there are Items on the BackgroundImage.
- Hover over an Item => highlighing and ability to move the Item
