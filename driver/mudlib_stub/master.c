// Master object for driver bring-up / smoke testing.
// connect() is invoked by Server::onNewConnection() for every new TCP
// client, via the same applyMaster() mechanism used elsewhere. It must
// return an object -- the driver binds that object to the Connection.

void create() {
    write("Master object create() reached.\n");

    call_other(clone_object("/obj/simple_login"), "greet");
    clone_object("/obj/simple_login")->greet_again();
}

object connect() {
    return clone_object("/obj/simple_login");
}
