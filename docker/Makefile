VERSION=0.3.0-dev
SHARE=share

all:	build-share run clean
	

build-share:
	mkdir -p ${SHARE}

run:
	sudo docker run -ti --name=aprof-container -v `pwd`/share:/home/ubuntu/share -h ubuntu aprof-${VERSION}
	echo ""

clean:
	sudo docker rm aprof-container

build:
	-sudo docker rmi aprof-${VERSION}
	sudo docker build -t aprof-${VERSION} -f Dockerfile ../
