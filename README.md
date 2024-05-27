# Proxy Server

Proxy is a simple concurrent HTTP proxy server that caches recently-requested web objects.

It's an application for learning about **Socket Programming* and *Concurrent Programming**.


## Overview

A web proxy is a program that acts as a middleman between a Web browser and an end server.
- Instead of contacting the end server directly to get a Web page, the browser contacts the proxy, which forwards the request on to the end server. When the end server replies to the proxy, the proxy sends the reply on to the browser.
- It can deal with multiple concurrent connections using multi-threading.
- It can also cache web objects by storing local copies of objects from servers then responding to future requests by reading them out of its cache rather than by communicating again with remote servers.



## How does proxy deal with concurrent requests?

## Main Thread

- The main thread is responsible for repeatedly accepting incoming connection requests from clients.
- Upon accepting a connection, the main thread creates a new worker thread.
- The worker thread is created in detached mode, allowing it to operate independently and freeing the main thread to continue accepting new connections without waiting for the worker thread to complete.

## Worker Threads

- Each worker thread is tasked with handling the communication with a specific client.
- The worker thread receives the incoming request from the client.
- It then forwards this request to the intended destination server.
- The response from the destination server is received by the worker thread and sent back to the client.
- Operating in detached mode ensures that resources are automatically cleaned up once the thread finishes its task.


## How does proxy synchronize accesses to cache?

- Accesses to the cache must be thread-safe, and free of race conditions, So as a matter of fact we have these special requirements:
    1. Multiple threads must be able to simultaneously read from the cache.
    2. No thread can write to specific object while another thread is reading it.
    3. Only one thread should be permitted to write to the object at a time, but that restriction mustn't exist for readers.



- A *writer thread* locks the write mutex each time it writes to the cache line associtaed with it, and unlocks it each time it finishes writing. This guarantees that there is at most one writer in that cache line at any point of time.

- On the other hand, only the first *reader thread* to read a cache line locks write mutex for that cache line, and only the last reader to finish reading unlocks it. The write mutex is ignored by readers who enter and leave while other readers are present.


