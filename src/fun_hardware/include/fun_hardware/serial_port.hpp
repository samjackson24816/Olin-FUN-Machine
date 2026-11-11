#pragma once

#include <string>
#include <vector>

namespace fun_hardware
{

// A minimal serial port for talking to an Arduino with text lines.
// Fixed at 115200 baud. Never blocks: reads return only what has already arrived.
class SerialPort
{
public:
  SerialPort() = default;
  ~SerialPort();

  // One object owns one open port, so it cannot be copied.
  SerialPort(const SerialPort &) = delete;
  SerialPort & operator=(const SerialPort &) = delete;

  bool open(const std::string & path);  // e.g. "/dev/ttyACM0"; false if it fails
  void close();
  bool is_open() const;

  bool write_line(const std::string & line);  // sends line + "\n"

  // Returns every complete line received so far (without the line ending).
  // Closes the port if the device was unplugged.
  std::vector<std::string> read_lines();

private:
  int fd_ = -1;          // file descriptor, -1 = not open
  std::string pending_;  // received text that does not end in "\n" yet
};

}  // namespace fun_hardware
