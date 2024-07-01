```
  ___                      _            ______            _   _    _ _   
 / _ \                    (_)           | ___ \          | | | |  (_) |  
/ /_\ \_ __ ___   __ _ _____ _ __   __ _| |_/ /___   ___ | |_| | ___| |_ 
|  _  | '_ ` _ \ / _` |_  / | '_ \ / _` |    // _ \ / _ \| __| |/ / | __|
| | | | | | | | | (_| |/ /| | | | | (_| | |\ \ (_) | (_) | |_|   <| | |_ 
\_| |_/_| |_| |_|\__,_/___|_|_| |_|\__, \_| \_\___/ \___/ \__|_|\_\_|\__|
                                    __/ |                                
                                   |___/                                 
v1.0.0
```

## amazing_rootkit

### Building
```
cd amazing_rootkit
make
```
then we should have the `amazing_kernel_module.ko` file.

PS:
before bulding the project, fot the current version of the code you might need to replace the hardcoed function pointers we hook. For example:
```
sudo cat /proc/kallsyms |grep -e tcp4_seq_show -e tcp6_seq_show -e udp4_seq_show -e udp6_seq_show -e inet_ioctl -e sys_getdents64
```

### Running
```
sudo insmod amazing_kernel_module.ko

# can be foe validating the module is loaded
sudo dmesg -w | grep AmazingRootkit
```

## usersapce_dummy
For the POC purposes we created that dummy program that will "act" as out usemode agent.

### Building
```
gcc userspace_dummy.c -o dummy
```
then we should have the `dummy` file.

### Running
```
./dummy <target_pid_to_set_root> <target_port> <target_filename> <target_pid_to_hide>
```



*Academic project , Developed for Ubuntu 22.04 LTS. #Linux #Rootkit #Cybersecurity*
