# Socket_File_Distributor
The client-server system is designed for file transfer and distribution from a single client to
multiple clients. The client sends a file to the server and then redistributes it to all connected
clients. The server keeps track of the file names associated with the files received from clients, along
with added functionalities like compression, decompression, multithreading & concurrency control.

Server Code:
The server code is designed to handle multiple clients concurrently.
It listens for incoming connections from clients and accepts them.
For each connected client, it does the following:
1. Receives the file name, file size, and file data from the client.
2. Saves the received file with the provided file name.
3. Compresses the received file and sends it back to the client.
4. The server also displays information about the received files, such as file name, file size, and
the reception timestamp.

The server stores information about each connected client, including the file name associated
with the files received from them.
The server creates a new thread for each connected client, allowing it to handle multiple clients
simultaneously.


Client Code:
The client code sends a file to the server.
It takes the following steps for each client:
1. Opens a specified file through its location and file name and reads its content.
2. Sends the file's content to the server over a socket connection. This includes the file name,
file size, and the actual file data.
3. Receives a compressed file from the server, decompresses it, and saves it with a new file
name.

