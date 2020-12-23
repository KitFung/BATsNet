# The data collector

`data_upload_mission_app.cc`: A implementation of uploading local saved data to server with using the schedule. Since the scheduler is DEPRACATED, this is also DEPRACATED

`data_io.cc` is the implementation about how to store data in compressed form to disk and how to read.

`data_collector.cc` is the class that provide the data collect interface. It the mode is realtime upload, it will send data to server immediate after receive data, otherwise, it store data to disk.
