# udp_over_tcp
----
> a simple udp over tcp tunnel

## how to build

**using make**

    ➜  udp_over_tcp git:(master) ✗ make
    gcc -o uot package.c client.c vlog.c ezbuf.c main.c hash.c server.c ev/ev.c -lm -I . -I ./ev/ -DHAVE_CONFIG_H -g
    ➜  udp_over_tcp git:(master) ✗ ls
    client.c        Dockerfile  ezbuf.c  hash.c  khash.h  Makefile   README.md     server.c  udp_over_tcp.h  vlog.c
    CMakeLists.txt  ev          ezbuf.h  hash.h  main.c   package.c  repositories  Shanghai  uot

**using cmake**

    ➜  udp_over_tcp git:(master) ✗ mkdir build &&cd build && cmake .. && make
    -- The C compiler identification is GNU 5.4.0
    -- The CXX compiler identification is GNU 5.4.0
    -- Check for working C compiler: /usr/bin/cc
    -- Check for working C compiler: /usr/bin/cc -- works
    -- Detecting C compiler ABI info
    -- Detecting C compiler ABI info - done
    -- Detecting C compile features
    -- Detecting C compile features - done
    -- Check for working CXX compiler: /usr/bin/c++
    -- Check for working CXX compiler: /usr/bin/c++ -- works
    -- Detecting CXX compiler ABI info
    -- Detecting CXX compiler ABI info - done
    -- Detecting CXX compile features
    -- Detecting CXX compile features - done
    -- Configuring done
    -- Generating done
    -- Build files have been written to: /home/julyrain/work/gitlab/udp_over_tcp/build
    Scanning dependencies of target ev
    [ 10%] Building C object ev/CMakeFiles/ev.dir/ev.c.o
    [ 20%] Linking C static library libev.a
    [ 20%] Built target ev
    Scanning dependencies of target uot
    [ 30%] Building C object CMakeFiles/uot.dir/server.c.o
    [ 40%] Building C object CMakeFiles/uot.dir/client.c.o
    [ 50%] Building C object CMakeFiles/uot.dir/main.c.o
    [ 60%] Building C object CMakeFiles/uot.dir/ezbuf.c.o
    [ 70%] Building C object CMakeFiles/uot.dir/package.c.o
    [ 80%] Building C object CMakeFiles/uot.dir/vlog.c.o
    [ 90%] Building C object CMakeFiles/uot.dir/hash.c.o
    [100%] Linking C executable uot
    [100%] Built target uot
    ➜  build git:(master) ✗ ls
    CMakeCache.txt  CMakeFiles  cmake_install.cmake  ev  Makefile  uot

## usage
    
    [udp client]<---->[uot client(u2t)]<--tunnel-->[uot server(t2u)]<---->[udp server]

    ➜  build git:(master) ✗ ./uot
    usage:
        uot -v -t u2t -b 0.0.0.0 -l 4500 -s 192.168.1.111 -p 4500
        * u2t udp to tcp
        * t2u tcp to udp
