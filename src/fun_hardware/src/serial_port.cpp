#include "fun_hardware/serial_port.hpp"

#include <cerrno>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

namespace fun_hardware
{

SerialPort::~SerialPort()
{
  close();
}

bool SerialPort::open(const std::string & path)
{
  close();

  fd_ = ::open(path.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (fd_ < 0) {
    return false;
  }

  // Raw mode: bytes pass through unchanged, no echo, no line editing.
  termios settings{};
  if (tcgetattr(fd_, &settings) != 0) {
    close();
    return false;
  }
  cfmakeraw(&settings);
  cfsetispeed(&settings, B115200);
  cfsetospeed(&settings, B115200);
  settings.c_cflag |= CLOCAL | CREAD;
  if (tcsetattr(fd_, TCSANOW, &settings) != 0) {
    close();
    return false;
  }

  tcflush(fd_, TCIOFLUSH);  // throw away anything left over from before
  pending_.clear();
  return true;
}

void SerialPort::close()
{
  if (fd_ >= 0) {
    ::close(fd_);
    fd_ = -1;
  }
}

bool SerialPort::is_open() const
{
  return fd_ >= 0;
}

bool SerialPort::write_line(const std::string & line)
{
  if (fd_ < 0) {
    return false;
  }
  const std::string data = line + "\n";
  const ssize_t written = ::write(fd_, data.data(), data.size());
  if (written != static_cast<ssize_t>(data.size())) {
    close();
    return false;
  }
  return true;
}

std::vector<std::string> SerialPort::read_lines()
{
  std::vector<std::string> lines;
  if (fd_ < 0) {
    return lines;
  }

  // Read everything that has arrived.
  char chunk[256];
  while (true) {
    const ssize_t n = ::read(fd_, chunk, sizeof(chunk));
    if (n > 0) {
      pending_.append(chunk, static_cast<size_t>(n));
    } else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
      break;  // nothing more for now
    } else {
      close();  // n == 0 or a real error: the device is gone
      break;
    }
  }

  // Cut the text into lines.
  size_t end;
  while ((end = pending_.find('\n')) != std::string::npos) {
    std::string line = pending_.substr(0, end);
    pending_.erase(0, end + 1);
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();  // Arduino println() ends lines with "\r\n"
    }
    if (!line.empty()) {
      lines.push_back(line);
    }
  }
  return lines;
}

}  // namespace fun_hardware
