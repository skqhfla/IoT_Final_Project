#include <cstdlib>
#include <deque>
#include <iostream>
#include <fstream>
#include <thread>
#include <boost/asio.hpp>
#include <cstring>
#include <gtkmm.h>
#include "game_message.hpp"
#include "gameWindow.hpp"

using boost::asio::ip::tcp;

typedef std::deque<char> move_queue;
char check = -1;
char client_num;

std::vector<Gtk::Image *> images;
std::vector<std::string> imagePaths;

Gtk::Image *createImage(const std::string &imagePath);
void hideImage(Gtk::Image *imageToHide);
void startGameWindow(Gtk::Window* window);

class game_client
{
public:
  game_client(boost::asio::io_context &io_context,
              const tcp::resolver::results_type &endpoints,
              Gtk::Window *window1, GameWindow *window2)
      : io_context_(io_context),
        socket_(io_context),
        window1_(window1),
        window2_(window2),
        client_id_(0)
  {
    do_connect(endpoints);
  }

  void write(const char move)
  {
    boost::asio::post(io_context_,
                      [this, move]()
                      {
                        bool write_in_progress = !write_msgs_.empty();
                        write_msgs_.push_back(move);

                        if (!write_in_progress)
                        {
                          do_write();
                        }
                      });
  }

  void choice_client(char client_id)
  {
    boost::asio::write(socket_, boost::asio::buffer(&client_id, 1));

    char ACK;
    boost::asio::read(socket_, boost::asio::buffer(&ACK, 1));

    printf("recv ack %d\n", ACK);
    if (ACK == 0)
    {
      check = 1;
      client_id_ = client_id;

      window1_->hide();

      printf("startGameWindow\n");
      window2_->input_id(client_id_);
      window2_->show();
      printf("showed\n");
    }
    else
    {
      hideImage(images[client_id - 1]);
      window1_->queue_draw();
    }
  }

  bool choice_client_event(GdkEventKey *event)
  {
    switch (event->keyval)
    {
    case GDK_KEY_1:
    case GDK_KEY_KP_1:
      choice_client(1);
      break;
    case GDK_KEY_2:
    case GDK_KEY_KP_2:
      choice_client(2);
      break;
    case GDK_KEY_3:
    case GDK_KEY_KP_3:
      choice_client(3);
      break;
    case GDK_KEY_4:
    case GDK_KEY_KP_4:
      choice_client(4);
      break;
    default:
      break;
    }

    return false;
  }

  bool game_event(GdkEventKey *event)
  {
    switch (event->keyval)
    {
    case 119:
      write('U');
      break;
    case 97:
      write('L');
      break;
    case 100:
      write('R');
      break;
    case 115:
      write('D');
      break;
    default:
      break;
    }

    return false;
  }

private:
  void do_connect(const tcp::resolver::results_type &endpoints)
  {
    boost::asio::async_connect(socket_, endpoints,
                               [this](boost::system::error_code ec, tcp::endpoint)
                               {
                                 if (!ec)
                                 {
                                   recv_file_num();
                                 }
                               });
  }

  void do_read_header()
  {
    boost::asio::async_read(socket_,
                            boost::asio::buffer(game_msg_.data(), game_message::header_length),
                            [this](boost::system::error_code ec, std::size_t /*length*/)
                            {
                              if (!ec && game_msg_.decode_header())
                              {
                                do_read_body();
                              }
                              else
                              {
                                socket_.close();
                              }
                            });
  }

  void do_read_body()
  {
    printf("data %hhu / %d / %d / %d \n", game_msg_.client_id(), game_msg_.status(), game_msg_.x(), game_msg_.y());
    window2_->update_status(game_msg_.client_id(), game_msg_.status(), game_msg_.x(), game_msg_.y());
    do_read_header();
  }

  void recv_file_num()
  {

    boost::system::error_code ec;
    boost::asio::read(socket_, boost::asio::buffer(&file_num_, 1), ec);

    printf("recv file num %d\n", file_num_);

    client_num = file_num_;

    if (!ec)
    {
      read_file_name();
    }
    else
    {
      std::cerr << "파일 개수 읽기 에러: " << ec.message() << std::endl;
    }
  }

  void read_file_name()
  {
    if (file_num_ == 0)
    {
      std::cout << "모든 파일을 전송받았습니다." << std::endl;
      check = 0;

      Gtk::Box imageBox(Gtk::ORIENTATION_HORIZONTAL, 10);

      for (int i = 1; i < imagePaths.size() / 2 + 1; i++)
      {
        std::cout << imagePaths[i] << std::endl;

        images.push_back(createImage(imagePaths[i]));

        // 이미지를 감싸는 Gtk::Box 생성
        Gtk::Box *box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 10));
        
        box->set_halign(Gtk::ALIGN_CENTER);
        box->set_hexpand(true);
        box->pack_start(*images.back(), Gtk::PACK_SHRINK);

        // box 안에 들어갈 Gtk::Label 생성
        Gtk::Label* text = Gtk::manage(new Gtk::Label());
        text->set_text("Player " + std::to_string(i));
        box->pack_start(*text);

        // 이미지를 감싸는 Box를 이미지 Box에 추가
        imageBox.add(*box);
      }

      // 이미지 Box 내에서 일정 간격을 유지하기 위해 Gtk::Box의 간격 설정
      imageBox.set_spacing(50);

      // 이미지 Box를 윈도우에 추가
      window1_->add(imageBox);

      // 윈도우를 보여주기
      window1_->show_all();

      while (check != 1) {
      };

      do_read_header();
    }

    else
    {
      char filename_length;
      boost::system::error_code ec;
      std::size_t length = boost::asio::read(socket_,
                                             boost::asio::buffer(&filename_length, 1), ec);

      open_file(filename_length);
      write_file();
    }
  }

  void
  open_file(char filename_length)
  {
    boost::system::error_code ec;
    char filename_char[255];
    std::size_t length = boost::asio::read(socket_,
                                           boost::asio::buffer(filename_char, filename_length), ec);

    std::string filename(filename_char, filename_char + filename_length);

    imagePaths.push_back(filename);
    window2_->input_images(imagePaths.back());    //gameWindow에 이미지 경로 전달

    file_.open(filename, std::ios::binary | std::ios::out);
    if (!file_)
    {
      std::cerr << "Failed to create file: " << filename << std::endl;
      return;
    }
  }

  void write_file()
  {
    boost::system::error_code ec;
    while (true)
    {
      std::size_t length = boost::asio::read(socket_,
                                             boost::asio::buffer(file_msg_.data(), file_message::header_length), ec);
      if (!ec && file_msg_.decode_header())
      {
        if (file_msg_.file_length() != 0)
        {
          std::vector<char> buffer(2048);
          size_t size = boost::asio::read(socket_, boost::asio::buffer(buffer, file_msg_.file_length()));

          file_.write(buffer.data(), size);
        }
        else
          break;
      }
      else
      {
        std::cerr << "파일 데이터 읽기 에러: " << ec.message() << std::endl;
      }
    }

    close_file();
    read_file_name();
  }

  void close_file()
  {
    file_.close();
    file_num_--;
  }

  void do_write()
  {
    boost::asio::async_write(socket_,
                             boost::asio::buffer(&write_msgs_.front(),
                                                 1),
                             [this](boost::system::error_code ec, std::size_t /*length*/)
                             {
                               if (!ec)
                               {
                                 write_msgs_.pop_front();
                                 if (!write_msgs_.empty())
                                 {
                                   do_write();
                                 }
                               }
                             });
  }

  boost::asio::io_context &io_context_;
  tcp::socket socket_;
  unsigned char file_num_;
  char client_id_;
  file_message file_msg_;
  game_message game_msg_;
  move_queue write_msgs_;
  std::ofstream file_;
  Gtk::Window *window1_;
  GameWindow *window2_;
};

Gtk::Image *createImage(const std::string &imagePath)
{
  Gtk::Image *image = Gtk::manage(new Gtk::Image(imagePath));
  return image;
}

void hideImage(Gtk::Image *imageToHide)
{
  Gtk::Box *imageBox = dynamic_cast<Gtk::Box *>(imageToHide->get_parent());
  if (imageBox)
  {
    // 이미지를 제거합니다.
    imageBox->remove(*imageToHide);
  }

  const std::vector<Gtk::Widget*>& boxChildren = imageBox->get_children();
  for(Gtk::Widget* child : boxChildren) {
    Gtk::Label *text = dynamic_cast<Gtk::Label*>(child);
    if(text) {
      // 텍스트를 변경합니다.
      text->set_text("Already Taken!");
    }
  }
}

// void showUnderline(Gtk::Image *image) {
//   Gtk::Box *imageBox = dynamic_cast<Gtk::Box *>(image->get_parent());
//   if (imageBox)
//   {
//     const std::vector<Gtk::Widget*>& boxChildren = imageBox->get_children();
//     for(Gtk::Widget* child : boxChildren) {
//       Gtk::TextView *text = dynamic_cast<Gtk::TextView*>(child);
//
//       if(text) {
//         // 텍스트를 변경합니다.
//         Glib::RefPtr<Gtk::TextBuffer> tbuff = text->get_buffer();
//
//         // 밑줄을 위한 TextTag 생성
//         Glib::RefPtr<Gtk::TextTag> underlineTag = Gtk::TextTag::create();
//         underlineTag->property_underline() = Pango::Underline::UNDERLINE_SINGLE;
//
//         // TextTag를 TextBuffer에 추가
//         tbuff->get_tag_table()->add(underlineTag);
//
//         // 텍스트에 밑줄 적용
//         Gtk::TextIter start, end;
//         tbuff->get_bounds(start, end);
//         tbuff->apply_tag(underlineTag, start, end);
//
//       }
//     }
//   }
// }


void startGameWindow(Gtk::Window* window)   // removeAllWidgets from window to start the game
{
  const auto& children = window->get_children();
  for (const auto& child : children) {
    Gtk::Box* box = dynamic_cast<Gtk::Box*>(child);

    if (box) {
      // box 안의 모든 위젯(image들) 제거하기
      const std::vector<Gtk::Widget*> allWidgets = box->get_children();
      for (Gtk::Widget* widget : allWidgets) {
          box->remove(*widget);
      }
      return;
    }
  }
}

class KeyEventHandler
{
public:
  KeyEventHandler(game_client *client) : g_client(client) {}

  bool on_key_press(GdkEventKey *event)
  {
    if (check == 0)
      return g_client->choice_client_event(event);
    else if (check == 1)
      return g_client->game_event(event);
    else
      return false;

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

private:
  game_client *g_client;
};

int main(int argc, char *argv[])
{
  Gtk::Main kit(argc, argv);
  Glib::RefPtr<Gtk::Application> app = Gtk::Application::create(argc, argv, "com.kaze.test");
  app->set_flags(Gio::APPLICATION_HANDLES_OPEN);

  Gtk::Window window1;
  GameWindow window2;
  window1.set_title("Select Character");
  window1.set_default_size(500, 200);
  window1.set_border_width(30);

  try
  {
    if (argc != 3)
    {
      std::cerr << "Usage: chat_client <host> <port>\n";
      return 1;
    }

    boost::asio::io_context io_context;

    tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(argv[1], argv[2]);

    game_client c(io_context, endpoints, &window1, &window2);

    KeyEventHandler key_event_handler(&c);
    window1.signal_key_press_event().connect(sigc::mem_fun(key_event_handler, &KeyEventHandler::on_key_press));
    window2.signal_key_press_event().connect(sigc::mem_fun(key_event_handler, &KeyEventHandler::on_key_press));

    std::thread t([&io_context]()
                  { io_context.run(); });
    
    Gtk::Main::run(window1);
    // app->run(window1);
    printf("end run1\n");
    Gtk::Main::run(window2);
    // app->run(window2);
    printf("end run2\n");

    t.join();
    
  }
  catch (std::exception &e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
