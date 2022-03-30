As per https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32/api-guides/lwip.html:

"The lwIP Raw API is designed for single threaded devices and is not supported in ESP-IDF.

The Netconn API is used to implement the BSD Sockets API inside lwIP, and it can also be called directly from ESP-IDF apps. This API has lower resource usage than the BSD Sockets API, in particular it can send and receive data without needing to first copy it into internal lwIP buffers."

and

"Espressif does not test the Netconn API in ESP-IDF. As such, this functionality is enabled but not supported. Some functionality may only work correctly when used from the BSD Sockets API."