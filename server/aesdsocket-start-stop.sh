#! /bin/sh
# Reference from Mastering Embedded Linux Chapter 10
case "$1" in
    start)
        echo "Starting aesdsocket daemon"
        start-stop-daemon -S -n aesdsocket -a /usr/bin/aesdsocket
        ;;
    stop)
        echo "Stopping aesdsocket daemon"
        start-stop-daemon -K -n aesdsocket
        ;;
    *)
        echo "Usage: $0 {start|stop}"
    exit 1
esac
exit 0