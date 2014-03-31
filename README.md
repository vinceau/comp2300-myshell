MYSHELL
=======

Author
------

Vincent Au - u5388374

Features
--------

### Full program execution
You have control to all your typical commands that you would usually use. Flags are also fully supported.
> ```$ ls -la```

### Background execution
If you want to run a program in the background simply use the & argument at the end of your command.
> ```$ eog picture.jpg &```

### IO Redirection
* > redirect stdout
* < redirect stdin
* >> append stdout

Testing
-------

In order to test the execute program in the background feature, I needed to run programs that wouldn't immediately return control to the user. I chose to use the *eog* program which would open up an image and only return when the image window was closed.


Copyright
---------

Copywrite (c) 2014 Vincent Au
