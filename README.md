## Steps 1 2 3

From https://stackoverflow.com/questions/79440040/how-do-i-call-async-write-multiple-times-and-send-data-to-the-server-boostas/79440804?noredirect=1#comment140106566_79440804
 
  - 1 orig.cpp
  - 2 minimal-change.cpp
  - 3 implement.cpp
 
 1. The simplest GUI application starting point
 
 2. Using wsclient-skeleton.cpp as a minimal stub to implement the
    wsclient.h interface
 
 3. Uses the Asio implementation from my answer to implement the exact
    same interface.
 
    I "complicated" things considerably by showing Pimpl Idiom to keep
    Boost completely out of the wsclient.h interface.
    That's more complicated because of the interaction with
    `std::enable_shared_from_this` (see the `Impl::owner_` member).
 
    However, it drives home the point of Separation of Concerns: if you
    do not **allow** complexity to leak into the interface, you can keep
    each part of the application simple. At the same time, you can
    replace the Asio implementation with a Qt5 networking implementation
    without breaking everything in the future.
 
 Note that I did extend `WSClient::Session` to have a `setHandler` method
 so you can implement `process_message` in the user code.

 [Screencast from 2025-02-17 04-34-35.webm](https://github.com/user-attachments/assets/3a8c3162-07ab-46a5-b53c-18cd7ade1161)

