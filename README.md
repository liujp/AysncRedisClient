### Compile
`g++ -O2 -o test test.cc -lhiredis -levent -levent_pthreads`

### Constructor
 use libevent in one thread to listen to the socket.nad handle the callbacks with the response data from the socket


