## liblsvc
A lightweight message bus and modulized services

# Features
- Auto-voted broker within the same network
- Network accessibility via commandline
- Runtime update of endpoint services
- Designed to be slim and flexible

# Build and run
```
git clone git@github.com:emonyr/lsvc.git
cd ./lsvc/
make clean
make
source common.env
./lsvc &
./lsvc log -l 4
```