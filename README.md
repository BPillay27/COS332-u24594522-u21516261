These are assessments from the University of Pretoria for Networking.

## Prac 1
This was a web server that was running on Apache 2 that dished out a html page from console output from a c++11 server that handled the dynamic game logic.

## Prac 2
This is a c++11 appointment database server that deals with telnet and runs it on localhost it is suppose to disable local echoing so the server echoing the user's input back to them, and the server runs concurrency so that multiple user's can connect to the server at the same time, the server also watches it's own terminal for the input 'stop' so that it can cleanly shutdown. 

## Prac 3
A c++11 World clock server, that dishes out html files and receives commands using the http protocol mainly use the get.

## Prac 4
A c++11 implementation of the Database-Server-Client architechture. The database from Prac2 (which now supports images) is queried by a server upon HTTP request from the client. The server then updates the clients page after adding, deleting or searching for appointments.

## Prac 5
A C++ client with a LDAP server that acts like a database that is holding assets. The assets in this one is planes.

## Prac 6
An event reminder, that sends an email to the localhost (linux), you need the port 25 open and it uses SMNTP and ours works off postfix and only runs locally wit hno internet connection. 
To run the project:
make run
and fill in the .env with the namae of your system if you don't know what it is use
whoami on linux
Also postfix must be running you can use this command to start it 
sudo service postfix start
and to stop it
sudo service postfix stop
