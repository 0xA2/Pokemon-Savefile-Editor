# Pokemon-Savefile-Editor
Command line savefile editor for the 4th generation NDS Pokemon games

Tested only on Debian 10 using [DeSmuME  - DS Emulator](http://desmume.com/)

---------------

### Current Features

- Editing player name
- Editing species of lead pokemon
- Editing ability of lead pokemon
- Editing moves of lead pokemon
- Make lead pokemon shiny

---------------

### Compile
```bash
Compile with: $ g++ saveditor.ccp -o saveditor
```

---------------

### Usage
```bash
$ ./saveditor
Usage: ./saveditor [SavefileName] [VersionName]
Versions available: 'diamond', 'pearl', 'platinum', 'heartgold', 'soulsilver'
```

---------------
### Main Menu

```bash
-------------------------------
    Pokemon Savefile Editor
-------------------------------
1) Edit player
2) Edit Pokemon
3) Exit
> 
```
