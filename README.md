# text_editor
My attempt to make a text editor in C
This program only runs in Linux, I'm currently using WSL with an Ubuntu distro to control it and edit it in VSCode with CMake for the project build.
Not all features work exacty as they would in Linux, like the Ctrl. + V isn't truly disabled even after setting its flag off.

As of April 20th, 2025, the project is finished. Possible extra features are the following:
1. Improve Ctrl. + F so that id doesn't delete its current line when pressing enter.
2. Add "select" feature when pressing Shift + Arrow.
3. Include highlight formatting for other file types such as .m, .py or .json

It is also fair to mention that this project was made possible by using the contents of the following booklet as a guide: https://viewsourcecode.org/snaptoken/kilo/
