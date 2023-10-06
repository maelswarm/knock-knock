# knock-knock

Nmap wrapper for knocking on ports.

README OOD

## Usage

Scan 1000 addresses, checking for open port 23 (telnet), save those ips to 'ip.txt', and then connect to those ports.
```
./kknock
```
Same as the above command, except the filename is declared as open23.txt.
```
./kknock -o open23.txt
```
Scan 5000 addresses for an open telnet port and save list of ip's as open23.txt.
```
./kknock -d open23.txt -n 5000
```
Connect to each ip's port 21.
```
./kknock -k open23.txt -p 21
```
Version detect port 22 and save as ver_*.txt.
```
./kknock -t open23.txt -p 22
```
You can speecify a wordlist for "auto-knocking" with -a. The default wordlist is klist.txt.
```
./kknock -o open23.txt -a
```

## Notes

Ctrl-z is overridden, it acts as a next button when enumerating through the address list.

```
Trying 181.126.131.230...
Connected to 181.126.131.230.ar.ir.
Escape character is '^]'.
Password:
telnet> ^Z
Trying 173.24.222.213...
Connected to 173.24.222.213.
Escape character is '^]'.

192.168.1.255 login:
telnet> ^Z
Trying 124.140.59.129...
Connected to 124.140.59.129.
```
