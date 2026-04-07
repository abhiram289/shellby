# Shellby - Custom Unix Shell in C

Shellby is a lightweight Unix-like command-line shell built in C that replicates core functionalities of traditional shells. It supports command execution, process management, input/output redirection, and piping, providing a hands-on understanding of how modern shells operate internally.


##  Features

- Execute built-in and external commands  
- Foreground and background process execution (`&`)  
- Input (`<`) and output (`>`, `>>`) redirection  
- Piping (`|`) between multiple commands  
- Signal handling (`Ctrl+C`, `Ctrl+Z`)  
- Custom shell prompt  
- Command parsing and tokenization  


##  Tech Stack

- **Language:** C  
- **System Calls:** `fork()`, `execvp()`, `wait()`, `pipe()`, `dup2()`  
- **Platform:** Linux / Unix-based systems  


## Author

abhiram289
