MYSHELL
=======

Author
------
Vincent Au - u5388374

Features
--------
### Full program execution
All your typical commands that you would usually use are fully supported. You
can use flags too!  
> ```$ ls -la```

### Change directory
Change your current working directory using the ```cd path``` command. Last
directory is also supported using ```cd -```.

### Home Environmental Variable
Access your home directory from anywhere using the tilde ```~``` key.  
> ```$ cd ~/path```  
> ```$ ls -la ~/path```

### Background execution
Run a program in the background simply by adding the & argument to the end of
your command.  
> ```$ eog picture.jpg &```

### Unlimited Pipe Functionality
Pass your output to other functions as many times as you'd like!  
> ```$ ls | grep .txt | xargs cat | wc -l```

### IO Redirection
Pass a file to a function and output the result to a file! Supported
redirections include:

* ```<``` redirect from ```stdin```
* ```>``` redirect from ```stdout```
* ```>>``` append from ```stdout```

> ```$ ./foo < input.txt | ./bar -bar | ./foobar > output.txt```

### Line-editing and Command History (from readline)
Made a mistake in your command? Use the up arrow key to see your previously
inputted commands and then edit them with your arrow keys.

### Noob-friendly
Not sure what you're doing? That's okay! Myshell is very forgiving to the user!

> ```$    ./foo     -foo       | |    |  |    ./bar      -bar```

Will be interpreted as: ```$ ./foo -foo | ./bar -bar```

Inner Workings
--------------
Myshell reads in a line using ```readline()``` and calling ```strip()```,
removes any unnecessary spaces before replacing any tilde ```~``` characters
with the user's home path. It will then check if it's a shell specific command
such as ```cd``` and if so, will handle the command before returning for
another input, otherwise, it will parse the new input string to
```check_io()```.

```check_io()``` then checks the string for any IO redirecting commands such as
```<``` or ```>``` and if it finds any it will find the next argument that
follows it and attempt to open that file. If no errors occur, it will modify
the input string, removing the IO command and the following argument and then
return a file descriptor of the input and output files. If no IO redirecting
commands are found, it will default to ```stdin``` and ```stdout```.

```run_pipe()``` will then look through the input string and split it into an
array of strings ```cmd_array``` by the pipe ```|``` character. It will then
iterate over each of the strings in ```cmd_array``` and turn the strings into
an argument vector. Input file descriptors, ```pipe_in```, are created and the
input is set to the result of ```check_io()```. A pipe is created and the input
is replaced with the input of ```pipe_in```. If it's on the final iteration,
that is the last command in ```cmd_array```, it will also set the output to the
output of ```check_io()```. ```execvp()``` is then called on the argument
vector and its output is saved into ```pipe_in``` for the next command.

When all the commands in ```cmd_array``` are run, it will return and wait for
the next input until the ```exit``` command is received.

Testing
-------
As I primarily use my Mac for development, code that worked on my machine might
not always work on the lab machines. ```git``` and ```ssh``` was used to easily
track changes and to ensure that the codes on both machines. The program was
also compiled using the ```-g``` flag to allow for easy debugging of the
program using the ```gdb``` program.

Lots of my functions, such as ```fix_home()``` and ```run_pipe()```, revolves
around string manipulation. When testing these functions, it was necessary to
regularly print the strings to ensure they were working correctly.

In order to test the execution of a program in the background, I needed to run
programs that wouldn't immediately return control to the user. I used the
```eog``` program which would open up an image and only return when the image
window was closed to test this run-in-background functionality was working
correctly.

Limitations
-----------
Quoted text isn't supported. Because of this, functions that take a string
literal misbehave. For example:  
> ```$ git commit -m "Commit message."```  
> ```$ echo "hello world"```

The second command should have two arguments, the first being ```echo``` and the
other being ```hello world```. So when it prints, it should simply show
```hello world``` on screen. However, as quotes are not supported, and the
commands are separated by space, this command gets interpreted as three
arguments: ```echo```, ```"hello``` and ```world"```. When this gets executed,
it prints ```"hello world"```.

If more time was available, I would like to successfully implement recognition
of both single quotes and double quotes, such that they could be nested within
each other.

Copyright
---------
Copyright (c) 2014 Vincent Au
