dummy:
	echo No standard build defined

testMAX: MAX30102.h

testMAX: testMAX.c MAX30102.c
	gcc -Wno-unused -W -Wall -o $@ $^

test: testMAX
	./testMAX

install:
	sudo cp heartbeat.service /etc/systemd/system/heartbeat.service
	sudo systemctl enable heartbeat.service
	sudo systemctl start heartbeat.service

restart: 
	sudo systemctl stop heartbeat.service
	sudo systemctl start heartbeat.service

test:
	sudo systemctl stop heartbeat.service
	sudo ./heartbeat.py
