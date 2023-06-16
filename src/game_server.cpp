#include <cstdlib>
#include <deque>
#include <iostream>
#include <fstream>
#include <list>
#include <memory>
#include <thread>
#include <chrono>
#include <set>
#include <utility>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include "game_message.hpp"

using boost::asio::ip::tcp;

#define MAX_CLIENT_NUM 2
#define FILE_PATH "./img/"
#define EXTENSION ".png"
#define EXTENSION_LENGTH 4

int check_client_list[MAX_CLIENT_NUM + 1] = {0};

game_message client_list[MAX_CLIENT_NUM + 1];
int count = 0;

boost::mutex mutex;

int client_count = 0;
class game_server;
std::list<class game_server> servers;

typedef std::deque<game_message> game_message_queue;

class game_session;
class game_server; // Forward declaration

//----------------------------------------------------------------------

class game_participant
{
public:
  virtual ~game_participant() {}
  virtual void deliver(const game_message &msg) = 0;
};

typedef std::shared_ptr<game_participant> game_participant_ptr;

class game_room
{
public:
  void join(game_participant_ptr participant)
  {
    participants_.insert(participant);
  }

  void leave(game_participant_ptr participant)
  {
    participants_.erase(participant);
  }

  void deliver(const game_message &msg)
  {
    for (auto participant : participants_)
      participant->deliver(msg);
  }

private:
  std::set<game_participant_ptr> participants_;
  game_message_queue recent_msgs_;
};

class game_server
{
public:
  game_server(boost::asio::io_context &io_context, const tcp::endpoint &endpoint)
      : acceptor_(io_context, endpoint), io_context_(io_context)
  {
    do_accept();
  }

  game_room &get_room()
  {
    return room_;
  }

private:
  void do_accept();
  tcp::acceptor acceptor_;
  boost::asio::io_context &io_context_;
  game_room room_;
};

class game_session : public game_participant,
                     public std::enable_shared_from_this<game_session>
{
public:
  game_session(tcp::socket socket, game_room &room)
      : socket_(std::move(socket)),
        room_(room)
  {
  }

  void start()
  {
    room_.join(shared_from_this());
    // client 및 zombe 이미지 파일 전송
    char file_num = MAX_CLIENT_NUM * 2 + 1;
    boost::asio::write(socket_, boost::asio::buffer(&file_num, 1));

    char client = 0;
    int ret;

    ret = img_send("zombie");
    if (ret != 0)
      socket_.close();

    client++;

    // client 이미지 전송
    while (client != file_num / 2 + 1)
    {
      std::string client_file = "client_" + std::to_string(client);
      ret = img_send(client_file);
      if (ret != 0)
        socket_.close();

      client++;
    }

    while (client != file_num)
    {
      std::string client_file = "client_" + std::to_string(client - MAX_CLIENT_NUM) + "_zombe";
      ret = img_send(client_file);
      if (ret != 0)
        socket_.close();

      client++;
    }

    check_client();
  }

  void deliver(const game_message &msg)
  {
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);
    if (!write_in_progress)
    {
      do_write();
    }
  }

private:
  void check_client()
  {
    auto self(shared_from_this());
    boost::asio::async_read(socket_, boost::asio::buffer(&recv_client_id_, 1),
                            [this, self](std::error_code ec, std::size_t /*length*/)
                            {
                              printf("recv\n");
                              if (!ec)
                              {
                                printf("recv client id %d\n", recv_client_id_);
                                boost::mutex::scoped_lock lock(mutex);
                                if (check_client_list[recv_client_id_] == 0)
                                {
                                  check_client_list[recv_client_id_] = 1;
                                  std::cout << "send ack 0" << std::endl;
                                  ack_ = 0;
                                  client_count++;
                                  send_ack();
                                }
                                else
                                {
                                  std::cout << "send ack 1" << std::endl;
                                  ack_ = 1;
                                  send_ack();
                                }
                              }
                            });
  }

  void send_ack()
  {
    boost::asio::write(socket_, boost::asio::buffer(&ack_, 1));

    if (ack_ == 1)
      check_client();
    else
    {
      client_list[0].client_id(0);
      client_list[0].status(1);
      client_list[0].x(game_message::max_x / 2);
      client_list[0].y(game_message::max_y / 2);
      client_list[0].encode_header();
      deliver(client_list[0]);

      for (int i = 1; i <= MAX_CLIENT_NUM; i++)
      {
        client_list[i].client_id(i);
        client_list[i].status(0);

        if (i % 2 == 0)
          client_list[i].x(0);
        else
          client_list[i].x(game_message::max_x);

        if (i < 2)
          client_list[i].y(game_message::max_y);
        else
          client_list[i].y();

        client_list[i].encode_header();
        deliver(client_list[i]);
      }

      recv_client_move();
    }
  }

  void check_event()
  {
    bool stauts_change = true;
    printf("id %d\n", recv_client_id_);
    mutex.lock();
    if(count == 0){
      mutex.unlock();
      return;
    }
    
    for (int i = 0; i <= MAX_CLIENT_NUM; i++)
    {

      printf("client[%d] %d / %d / %d / %d \n", i, client_list[i].client_id(), client_list[i].status(), client_list[i].x(), client_list[i].y());
      if (i != recv_client_id_)
      {
        switch (move_)
        {
        case 'U':
          if (client_list[i].x() == client_list[recv_client_id_].x() && client_list[i].y() == client_list[recv_client_id_].y() - 1 && client_list[i].status() == 1)
          {
            client_list[recv_client_id_].status(1);
            stauts_change = false;
          }
          break;
        case 'D':
          if (client_list[i].x() == client_list[recv_client_id_].x() && client_list[i].y() == client_list[recv_client_id_].y() + 1 && client_list[i].status() == 1)
          {
            client_list[recv_client_id_].status(1);
            stauts_change = false;
          }
          break;
        case 'L':
          if (client_list[i].x() == client_list[recv_client_id_].x() - 1 && client_list[i].y() == client_list[recv_client_id_].y() && client_list[i].status() == 1)
          {
            client_list[recv_client_id_].status(1);
            stauts_change = false;
          }
          break;
        case 'R':
          if (client_list[i].x() == client_list[recv_client_id_].x() + 1 && client_list[i].y() == client_list[recv_client_id_].y() && client_list[i].status() == 1)
          {
            client_list[recv_client_id_].status(1);
            stauts_change = false;
          }
          break;
        }

        if (!stauts_change)
        {
          std::cout << "client " << recv_client_id_ << " from " << i << std::endl;
          break;
        }
      }
    }

    if (stauts_change)
    {
      switch (move_)
      {
      case 'U':
        client_list[recv_client_id_].y(client_list[recv_client_id_].y() - 1);
        break;
      case 'D':
        client_list[recv_client_id_].y(client_list[recv_client_id_].y() + 1);
        break;
      case 'L':
        client_list[recv_client_id_].x(client_list[recv_client_id_].x() - 1);
        break;
      case 'R':
        client_list[recv_client_id_].x(client_list[recv_client_id_].x() + 1);
        break;
      }
    }

    client_list[recv_client_id_].encode_header();
    for (auto &server : servers)
    {
      server.get_room().deliver(client_list[recv_client_id_]);
    }

    mutex.unlock();
  }

  void recv_client_move()
  {
    auto self(shared_from_this());
    boost::asio::async_read(socket_, boost::asio::buffer(&move_, 1),
                            [this, self](std::error_code ec, std::size_t /*length*/)
                            {
                              if (!ec)
                              {
                                printf("move %c\n", move_);
                                check_event();
                              }
                              else{
                                room_.leave(shared_from_this());
                                client_list[recv_client_id_].status(2);
                                client_list[recv_client_id_].encode_header();

                                for (auto &server : servers)
                                {
                                  server.get_room().deliver(client_list[recv_client_id_]);
                                }

                              }

                              recv_client_move();
                            });
  }

  int img_send(std::string filename)
  {
    std::string file_path = FILE_PATH + filename + EXTENSION;
    std::ifstream file(file_path, std::ios::binary);
    if (!file)
    {
      std::cout << "파일을 열 수 없습니다." << std::endl;
      return 1;
    }

    std::string filename_with_extension = filename + EXTENSION;

    char filename_length = filename_with_extension.length();
    boost::asio::write(socket_, boost::asio::buffer(&filename_length, 1));

    boost::asio::write(socket_, boost::asio::buffer(filename_with_extension.c_str(), filename_with_extension.length()));

    file.seekg(0, std::ios::end);
    std::streamsize file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    while (file_size > 0)
    {
      std::streamsize read_size = std::min(file_size, static_cast<std::streamsize>(file_message::max_file_length));
      message_.file_length(static_cast<uint32_t>(read_size));
      message_.encode_header();
      file.read(message_.file(), read_size);

      boost::asio::write(socket_, boost::asio::buffer(message_.data(), message_.length()));

      file_size -= read_size;
    }

    message_.file_length(0);
    message_.encode_header();
    boost::asio::write(socket_, boost::asio::buffer(message_.data(), message_.length()));

    return 0;
  }

  void do_write()
  {
    auto self(shared_from_this());
    boost::asio::async_write(socket_,
                             boost::asio::buffer(write_msgs_.front().data(),
                                                 game_message::header_length),
                             [this, self](boost::system::error_code ec, std::size_t /*length*/)
                             {
                               if (!ec)
                               {
                                 write_msgs_.pop_front();
                                 if (!write_msgs_.empty())
                                 {
                                   do_write();
                                 }
                               }
                               else
                               {
                                 room_.leave(shared_from_this());
                                 client_list[recv_client_id_].status(2);
                                  client_list[recv_client_id_].encode_header();

                                  for (auto &server : servers)
                                  {
                                    server.get_room().deliver(client_list[recv_client_id_]);
                                  }
                               }
                             });
  }

  tcp::socket socket_;
  game_room &room_;
  file_message message_;
  game_message_queue write_msgs_;
  char recv_client_id_;
  char ack_;
  char move_;
};

void game_server::do_accept()
{
  acceptor_.async_accept(
      [this](boost::system::error_code ec, tcp::socket socket)
      {
        if (!ec)
        {
          std::make_shared<game_session>(std::move(socket), room_)->start();
        }

        do_accept();
      });
}

void runIO(boost::asio::io_context &io_context)
{
  io_context.run();
}

int main(int argc, char *argv[])
{

  try
  {
    if (argc < 2)
    {
      std::cerr << "Usage: chat_server <port>\n";
      return 1;
    }

    boost::asio::io_context io_context;

    for (int i = 1; i < argc; ++i)
    {
      tcp::endpoint endpoint(tcp::v4(), std::atoi(argv[i]));
      servers.emplace_back(io_context, endpoint);
    }

    std::thread thread([&io_context]()
                       { runIO(io_context); });

    while (client_count != MAX_CLIENT_NUM)
    {
    };

    int randomID = 0, randomXY = 0, alive = 0, timer = 1000;

    sleep(1);
    client_list[0].status(4);
    client_list[0].encode_header();
    for (auto &server : servers)
    {
      server.get_room().deliver(client_list[0]);
    }

    client_list[0].status(1);

    while (true)
    {
      alive = 0;
      srand(static_cast<unsigned int>(time(nullptr)));
      mutex.lock();

      for (int i = 1; i <= MAX_CLIENT_NUM; i++)
      {
        if (client_list[i].status() == 0)
        {
          alive++;
        }
      }

      if (alive == 1){
        client_list[0].status(0);
        client_list[0].encode_header();
        for (auto &server : servers)
        {
          server.get_room().deliver(client_list[0]);
        }
        mutex.unlock();
        break;
      }
       

      do
      {
        if(count % 3 == 0)
          randomID = (rand() % MAX_CLIENT_NUM) + 1;

        randomXY = rand() % 2;
      } while (client_list[randomID].status() != 0);

      if (randomXY == 0)
      {
        if (client_list[randomID].x() > client_list[0].x() && client_list[0].x() < game_message::max_x + 1)
          client_list[0].x(client_list[0].x() + 1);
        else if (client_list[randomID].x() < client_list[0].x() && client_list[0].x() > -1)
          client_list[0].x(client_list[0].x() - 1);
      }
      else
      {
        if (client_list[randomID].y() > client_list[0].y() && client_list[0].y() < game_message::max_y + 1)
          client_list[0].y(client_list[0].y() + 1);
        else if (client_list[randomID].y() < client_list[0].y() && client_list[0].y() > -1)
          client_list[0].y(client_list[0].y() - 1);
      }

      client_list[0].encode_header();

      printf("send zombe\n");
      printf("client[%d] %d / %d / %d / %d \n", 0, client_list[0].client_id(), client_list[0].status(), client_list[0].x(), client_list[0].y());

      // 모든 서버에 메시지 전송
      for (auto &server : servers)
      {
        server.get_room().deliver(client_list[0]);
      }

      for (int i = 1; i <= MAX_CLIENT_NUM; i++)
      {
        if (client_list[i].x() == client_list[0].x() && client_list[i].y() == client_list[0].y() && client_list[i].status() == 0)
        {
          std::cout << "client " << i << " die\n";
          client_list[i].status(1);
          client_list[i].encode_header();
          for (auto &server : servers)
          {
            server.get_room().deliver(client_list[i]);
          }
        }
      }

      mutex.unlock();

      if ((count % 10 == 0) && (count / 10 > 1) && timer > 300)
      {
        timer -= 50;
      }

      // 1초 대기
      std::this_thread::sleep_for(std::chrono::milliseconds(timer));
      count++;
    }

    thread.join();
  }
  catch (std::exception &e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
