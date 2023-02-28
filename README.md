# g-Shell

## Description

Unix shell program with same functionalities of the conventional terminal containing the basic set of commands introduced in terminal.

## Specification
Implemented using c language along with unistd ans sys libraries


The following table shows the current set of commands supported by the shell:
| Command | Arguments | Description |
|------|----------|--------|
| cd | '', ~, /, .., - | Changes the current working directory |
| pwd | None | Writes to standard output the full path name of your current directory (from the root directory) |
| ls | ~, /, .., -, -l, -sl, -R, -a | Displays the contents of a directory |
| mkdir | Directory name | Creates a new directory in the Linux/Unix operating system |
| rm | Directory name, -r | Deletes file or directory |
| ps | aux, eaf | views information related with the processes on a system |
| kill | -, -s, -p | Sends the specified signal to the specified processes or process groups |
| export | Expression | Marks variables and functions to be passed to child processes |
| find | -H, -L Path | locates files based on some user-specified criteria |
| echo | $Variable | Displays a line of string that is passed as the arguments or value of an environment variable |
| Process name | & | Runs the process as a backgroud process |
