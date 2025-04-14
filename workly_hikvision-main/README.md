Config file (config.json)
===========
`{`  
`	"localIP": "192.168.0.50",`  
`	"port": 6000,`  
`	"ehomeKey": "qwerty123", `
`	"images_directory": "",`  
`	"records_directory": "",`  
`	"logs_directory": "",`  
`	"pghostaddr": "127.0.0.1",`  
`	"pgport": "5432",`  
`	"pgdatabase": "workly",`  
`	"pguser": "postgres",`  
`	"pgpassword": "1234",`  
`	"pgsslmode": "allow"`  
`}`  


Commands
========

User management
---------------

**Adds/Updates simple user with id 1**  
`DATA USER PIN=1	Name=John Appl	Pri=0	Passwd=	Card=[00000000]	TZ=000000000	Grp=0`   

**Adds/Updates admin with id 1**  
`DATA USER PIN=1	Name=John Appl	Pri=1	Passwd=	Card=[00000000]	TZ=000000000	Grp=0`  

**Adds admin with id 1**  
`ADD ADMIN PIN=1`  

**Deletes user with id 1**  
`DATA DEL_USER PIN=1`  

Card management
---------------

**Adds new card for user with id 1. Fails if card already exists**  
`ADD CARD PIN=1	CardData=12345`  

**Deletes card for a user with id 1**  
`DATA DEL_CARD PIN=1`  

**Uploads card info of user with id 1 to a database**  
`SYNC CARD PIN=1	EMPLOYEE_ID=100`  

Face data management
--------------------

**Add face model for a user with id 1. FACE_ID is row ID of face model in w_faces table**  
`DATA FACE PIN=1	FACE_ID=100`  

**Deletes face data for a user with id 1**  
`DATA DEL_FACE PIN=1`  

**Uploads face model of user with id 1 to a database (w_faces table)**  
`SYNC FACE PIN=1	EMPLOYEE_ID=100`  


Record/Event management
-----------------------

**Get all records or between specified data range and save as CSV in listeners folder**  
`GET RECORDS`  
`GET RECORDS START=2021-09-01`  
`GET RECORDS END=2021-10-01`  
`GET RECRODS START=2021-09-01	END=2021-10-01`  


System config
--------------

**Set device time zone in minutes**  
`SET DEVICE_TIME_ZONE OFFSET=300`  

**Sync device time with server's time**  
`SET DEVICE_TIME`  

**Set IP config**  
`SET NETWORK DHCP_ENABLED=0	IP=192.168.0.10	SUBNET_MASK=255.255.255.0	DEFAULT_GATEWAY=192.168.0.1`  

**Set ISUP server**  
`SET AUTOREGISTER ENABLED=1	DEVICE_ID=15	SERVER_IP=192.168.0.50	SERVER_PORT=6000	EHOME_KEY=qwerty123`  

**Clear all employees data**  
`CLEAR DATA`  

**Reboot device**  
`REBOOT`  


# Dependencies
`sudo apt install libjpeg-dev`
`sudo apt install libpq-dev`
`sudo apt install libssl-dev`