# ftp
### 写在前面
这是一个小型的ftp~有server和client

谢谢徐老师完成了version1.0!

### 关于功能
- get (get [remote_filename]) -- Copy file with the name [remote_filename] from remote directory to local directory.
- put (put [local_filename]) -- Copy file with the name [local_filename] from local directory to remote directory.
- delete (delete [remote_filename]) – Delete the file with the name [remote_filename] from the remote directory.
- ls (ls) -- List the files and subdirectories in the remote directory.
- cd (cd [remote_direcotry_name] or cd ..) – Change to the [remote_direcotry_name] on the remote machine or change to the parent directory of the current directory.
- mkdir (mkdir[remote_direcotry_nam]> ) – Create directory named [remote_direcotry_name] as the sub-directory
of the current working directory on the remote machine.
- pwd (pwd) – Print the current working directory on the remote machine.
- quit (quit) – End the FTP session.


***

## To compile in CLI:
gcc client/client.c -lwsock32 -o client/client.exe
gcc server/server.c -lwsock32 -o server/server.exe
