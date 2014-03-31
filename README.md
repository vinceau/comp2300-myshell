MYSHELL
=======

Author
------
Vincent Au - u5388374

Features
--------
### Full program execution
All your typical commands that you would usually use are fully supported. You can use flags too!
> ```$ ls -la```

### Change directory
Change your current working directory using the ```cd path``` command. Last directory is also supported using ```cd -```.

### Home Environmental Variable
Access your home directory from anywhere using the tilde key ~.
> ```$ cd ~/path```  
> ```$ ls -la ~/path```

### Background execution
Run a program in the background simply by adding the & argument to the end of your command.
> ```$ eog picture.jpg &```

### Unlimited Pipe Functionality
Pass your output to other functions as many times as you'd like!
> ```$ ls | grep \.txt | xargs cat | wc -l```

### IO Redirection
Pass a file to a function and output the result to a file! Supported redirections include:

* < redirect from stdin
* > redirect from stdout
* >> append from stdout

> ```$ ./foo < input.txt | ./bar -bar | ./foobar > output.txt```

### Line-editing and Command History (from readline)
Made a mistake in your command? Use the up arrow key to see your previously inputted commands and then edit them with your arrow keys.

### Noob-friendly
Not sure what you're doing? That's okay! Myshell is very forgiving to the user!
> ```$      ./foo          -foo       | |    |  |    ./bar      -bar```

Will be interpreted as: ```$ ./foo -foo | ./bar -bar```

Inner Workings
--------------
Myshell reads in a line using ```readline()``` and calling ```strip()```, removes any unnecessary spaces. It will first check if it's a shell specific command such as ```cd``` and if so handle the command, otherwise parse it to the ```check_io()```. This will check the string for any IO redirecting commands such as ```<``` or ```>``` and if it finds any it will find the next argument that follows it and attempt to open that file. If no errors occur, it will modify the input string, removing the IO command and the following argument and then return a file descriptor of the input and output files. If no IO redirecting commands are found, it will default to stdin and stdout.

```run_pipe()``` will then look through the input string and split the string into an array of strings by the pipe ```|``` character.

Testing
-------
In order to test the execute program in the background feature, I needed to run programs that wouldn't immediately return control to the user. I chose to use the *eog* program which would open up an image and only return when the image window was closed.


Copyright
---------
Copyright (c) 2014 Vincent Au
