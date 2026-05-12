# Linux Networking System

Using the system developed in my [Fridge project](https://github.com/bigbyrus/fridge-project), I designed another Web Server to display images taken by the camera module to the client. This implementation uses the C
programming language to communicate with the Linux kernel on a RaspberryPi. My application uses the serial port to communicate with, and provide power to, the camera module. It also utilizes sockets to accept incoming
TCP connections from clients. The web server manages concurrency issues by generating threads to handle each incoming TCP connection individually as they arrive, and employing a `mutex` to protect the shared camera 
resource. 
