#ifndef GAME_MESSAGE_HPP
#define GAME_MESSAGE_HPP

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

class file_message{
  public:
    static constexpr std::size_t header_length = 4;
    static constexpr std::size_t max_file_length = 2048;

    file_message()
      : file_length_(0)
      {}

    const char* data() const{
      return data_;
    }

    char* data()
    {
      return data_;
    }

    std::size_t length() const
    {
      return header_length + file_length_;
    }

    std::uint32_t payload_length()
    {
      return file_length_;
    }

    const char* file() const
    {
      return data_ + header_length;
    }

    char* file()
    {
      return data_ + header_length;
    }

    std::uint32_t file_length() const
    {
      return file_length_;
    }

    void file_length(std::uint32_t new_length)
    {
      file_length_ = new_length;
      if (file_length_ > max_file_length)
        file_length_ = max_file_length;
    }

    bool decode_header() {

      char header[header_length + 1] = "";
      std::strncat(header, data_, header_length);
      header[header_length] = '\0';

      std::string header_str(header, header_length);
      file_length_ = static_cast<uint32_t>(std::stoi(header_str.substr(0, 4)));

      if (file_length_ > max_file_length) {
          file_length_ = 0;
          return false;
      }

      return true;
    }
    
    void encode_header()
    {

      char header[header_length + 1] = "";
      std::sprintf(header, "%4d", static_cast<int>(file_length_));
      std::memcpy(data_, header, header_length);
    }

  private:
    char data_[header_length + max_file_length];
    uint32_t file_length_;

};

class game_message
{
public:
  static constexpr std::size_t header_length = 3;
  static constexpr std::size_t max_client_id = 4;
  static constexpr std::size_t max_status = 3;
  static constexpr std::size_t max_x = 49;
  static constexpr std::size_t max_y = 49;

  game_message()
    : client_id_(0), status_(0), x_(0), y_(0)
  {
  }

  const char* data() const
  {
    return data_;
  }

  char* data()
  {
    return data_;
  }

  std::size_t length() const
  {
    return header_length;
  }

  std::uint8_t client_id() const
  {
    return client_id_;
  }

  void client_id(std::uint8_t new_client_id)
  {
    client_id_ = new_client_id;
    if (client_id_ > max_client_id)
      client_id_ = max_client_id;
  }

  std::uint8_t status() const{
    return status_;
  }

  void status(std::uint8_t new_status){
    status_ = new_status;
    if(status_ > max_status)
      status_ = max_status;
  }

  std::uint8_t x() const{
    return x_;
  }

  void x(std::uint8_t new_x){
    if(new_x <= max_x && new_x > -1)
      x_ = new_x;
  }

  std::uint8_t y() const{
    return y_;
  }

  void y(std::uint8_t new_y){
    if(new_y <= max_y && new_y > -1)
      y_ = new_y;
  }

  bool decode_header()
  {
      uint8_t header1 = static_cast<uint8_t>(data_[0]);
      client_id_ = (header1 >> 4) & 0x0F;
      status_ = header1 & 0x0F;

      x_ = static_cast<uint8_t>(data_[1]);
      y_ = static_cast<uint8_t>(data_[2]);

      if (client_id_ > max_client_id || status_ > max_status || x_ > max_x || y_ > max_y)
      {
          client_id_ = 0;
          status_ = 0;
          x_ = 0;
          y_ = 0;
          return false;
      }

      return true;
  }

  void encode_header()
  {
      data_[0] = static_cast<char>((client_id_ << 4) | status_);
      data_[1] = static_cast<char>(x_);
      data_[2] = static_cast<char>(y_);
  }

private:
  char data_[header_length];
  uint8_t client_id_;
  uint8_t status_;
  uint8_t x_;
  uint8_t y_;
};

#endif // GAME_MESSAGE_HPP