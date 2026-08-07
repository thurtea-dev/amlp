// Login/player object. create() fires once per clone (including the one
// bound to each new network connection) and sends a welcome message.
// receive_message(string msg) is called by Server::handleConnection()
// for every line of input the client sends. This slice adds control
// flow: typing "quit" takes a different branch than any other input,
// demonstrating if/else and the == comparison operator working on
// strings at runtime.

void create() {
    write("Welcome to lpcdriver! You are connected.\n");
}

void receive_message(string msg) {
    if (msg == "quit") {
        write("Goodbye.\n");
    } else {
        write("You said: ");
        write(msg);
        write("\n");
    }
}

void greet() {
    write("simple_login.c: greet() called via call_other()!\n");
}

void greet_again() {
    write("simple_login.c: greet_again() called via -> operator!\n");
}
