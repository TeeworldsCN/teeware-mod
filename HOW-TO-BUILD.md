# Dependencies:

* gcc/g++
* bam: `0.4`

# Build:
## Linux:
__build with bam:__
* Release:`bam server_release`
* Debug:`bam server_debug`

__build docker image:__
`docker build . -t teeware-mod-img`

# run:
## run directly:

	./TeeWare

## run with docker container:

	docker run --name=teeware-srv-1 --net=host -v /YOUR_PATH_TO_CFG_FILES/autoexec_server.cfg:/autoexec_server.cfg  -d teeware-mod-img
