#! /bin/sh
# Reference from Mastering Embedded Linux Chapter 10
case "$1" in
    start)
        echo "Starting aesdsocke Daemon"
        /usr/bin/aesdchar_load
        start-stop-daemon -S -n aesdsocket -a /usr/bin/aesdsocket -- -d
        ;;
    stop)
        echo "Stopping aesdsocket Daemon"
        /usr/bin/aesdchar_unload
        start-stop-daemon -K -n aesdsocket
        ;;
    *)
        echo "Usage: $0 {start|stop}"
    exit 1
esac
exit 0
