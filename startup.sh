#!/bin/bash

WORK_PATH=$(pwd)

WHOAMI=`whoami`
PID=`ps -u $WHOAMI | grep kftp | awk '{print $1}'`

if (test "$#" = 0); then
	echo "Usage: $0 [ stop | start | status ]"
	exit 0
fi

if (test "$1" = "start"); then
	if (test "$PID" = ""); then
          cd ./bin
          ./kftp
	else
		echo "kftp server is running"
	fi
	exit 0
fi

if (test "$1" = "stop"); then
	if (test "$PID" != ""); then
		kill -s 2 $PID
	fi
	exit 0
fi

if (test "$1" = "status"); then
	if (test "$PID" = ""); then
		echo "kftp server is not run"
	else
		echo "kftp server is running"
	fi
	exit 0
fi

echo "Usage: $0 [stop] [start] [status]"

