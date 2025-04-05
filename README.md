# unix-distributed-file-system
This project involves implementing a Distributed File System using socket programming. It requires creating four servers (S1, S2, S3, S4) and a client program (w25clients) to manage file storage and retrieval.

## How to Run the Code
1. Compile all programs:
`
gcc S1.c -o S1 -lpthread
`
`
gcc S2.c -o S2
`
`
gcc S3.c -o S3
`
`
gcc S4.c -o S4
`
`
gcc w25clients.c -o w25clients
`

2. Open four terminal windows and run each server in a separate terminal:

    - `# Terminal 1 - Main server`
    `./S1`

    - `# Terminal 2 - PDF server`
    `./S2`

    - `# Terminal 3 - TXT server`
    `./S3`

    - `# Terminal 4 - ZIP server`
    `./S4`

3. Run the client program in another terminal:

    - `./w25clients`

4. Use the client menu to interact with the distributed file system:

    - uploadf to upload files (they will be automatically routed to the appropriate server)

    - dispfnames to list files on the main server

    - exit to quit the client

