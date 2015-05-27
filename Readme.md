NAME
----
strats - the stratego server.

SYNOPSIS
--------
`strats *portnr*`

DESCRIPTION
-----------
Opens a port that accepts two TCP connections.
Since it uses plain ASCII to comunicate, you can talk to it through telnet.

EXAMPLES
--------
`strats 3720` starts up the server. To talk to it, start telnet in another
terminal: `telnet 127.0.0.1 3720` (alternatively use netcat: `nc 127.0.0.1 3720`).

We now have an interactive telnet/netcat prompt where we can send and receive messages to and from strats. Since stratego is a two-player game, startup a second terminal in the same way. Do the next two steps for both terminals.

First we tell it which color we want to play: `RED` or `BLUE`.
The server will respond with the color we got. (again either `RED` or `BLUE`). 

Now we know what color we are, we can send it our piece setup. The character for a piece is just its rank (`1` being the spy and `9` for the General). The exceptions are the bomb `B`, the flag `F`, and the Marshal `M`. You can add spaces everywhere you want, as long as the message doesn't exceed 109 characters. An example of a valid setup is: `M653262226 4288924BB5 727165B452 73334BFBB3`. If your setup is valid, the server will tell you `OK`, else it will tell you `INVALID`.

The server will now wait until both players have entered their information. When both have done so, The red player gets the message `START`, indicating that the red player may begin. The Blue player gets the signal `WAIT` indicating that it's the red players turn.

Setup is now done, the game has started! If it's not your turn, the server will reply to your commands by shouting `WAIT` at you. If it *is* your turn, the server will show you the board, and expects you to make your move. You can do this by using the `MOVE` command or by `SURRENDER`-ing.

An example of a map is:
```
??????????
??????????
??????????
??????????
..  ..  ..
..  ..  ..
M653262226
4288924BB5
727165B452
73334BFB3B
```

The `MOVE` command needs two arguments: the coordinates of the piece you want to move, and the coordinates of the place you want to move it to. Coordinates are very simple: 1-10 vertically and A-J horisontaly. `A0` is left bottom corner, and `J10` is the upper right corner. An example for a move command is: `MOVE B4 B5`

If you attacked a unit the server will respond with `ATTACK` followed by the identity of the attacking piece, the identity of the defening piece, and the identity of the winning piece. If two pieces were the same rank, the identity of the winning piece will be `0`. Examples: `ATTACK 3 B 3` (a miner attacked a bomb and diffused it), `ATTACK 4 6 6` (a sergeant attacked a captain and lost).

If the unit landed on an empty square the server will respond with `EMPTY`.

SEE ALSO
--------
[The protocol](https://docs.google.com/document/d/1Yh6MONmk1tg2I4mimA9eq-g7S-NOwr49JpOw5YCdX80/edit?usp=sharing)
