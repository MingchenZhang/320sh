# 320sh
A POSIX complient shell implementation 

Feature: 
* File Redirection: Redirecting file as standard input/output
* Job Control: Launch pipelined processes as job, and allow foreground/background/suspend a job. 
* Proper TTY interaction: Call tcsetpgrp to set foreground pgid just as proper shell does.  
* Filename Globbing: Expand * and ? notation in filenames
* Tab autocompletion: Autocomplete filenames just as bash does
* History: Keep command history in memory/file. Allow search by ctrl-c.
* Variable support: support variable name such as "?"
