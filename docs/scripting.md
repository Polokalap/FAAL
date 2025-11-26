# BFS (Basic FAAL Script) basics
- The script only gets gecognised if you put it in the  ~/.faal/scripts folder
- The script must end with the `.bfs` extension

Example script:
```
Name=Cool-user-script
Option=Name=Option 1;Exec=echo "Option 1 ran"
Option=Name=Option 2;Exec=echo "Option 2 ran"
```

- `Name`: This is how you will find the script. for example, if the name is "Screenshot", it will show up as an application in FAAL
- `Option`: This will add an option to the menu.
- `Name (inside of option)`: This is the option's name
`Exec (inside of option)`: This is the code that will run - if you select the option.

You **must** seperate `Name` and `Exec` with a `;`.

## Everything **is** case sensitive