#ifndef GTKMM_GAMEWINDOW
#define GTKMM_GAMEWINDOW

#include <gtkmm/box.h>
#include <gtkmm/image.h>
#include <gtkmm/window.h>
#include <gtkmm.h>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <fstream>
#include <thread>
#include <boost/asio.hpp>
#include <cstring>

#define MAX_CLIENT_NUM 2
#define WIDTH 50
#define HIGHT 30

class GameWindow : public Gtk::Window
{
    public:
        GameWindow() :
            client_id_(-1),
            count_(0),
            isStart_(false)
        {
            printf("gameWindow initialized\n");
            set_title("Avoid the zombes!");
            set_default_size(520, 320);

            add(m_grid_);
            m_grid_.set_row_homogeneous(true);
            m_grid_.set_column_homogeneous(true);


            // border / place holders / background
            Gtk::Image *image;
            for (int i=0; i<WIDTH+3; i++) {
                image = Gtk::manage(new Gtk::Image("./brick.png"));
                m_grid_.attach(*image, i, 0, 1, 1);
                image = Gtk::manage(new Gtk::Image("./brick.png"));
                m_grid_.attach(*image, i, HIGHT+2, 1, 1);

                if( i!= 0 && i<HIGHT+3) {
                image = Gtk::manage(new Gtk::Image("./brick.png"));
                m_grid_.attach(*image, 0, i, 1, 1);
                image = Gtk::manage(new Gtk::Image("./brick.png"));
                m_grid_.attach(*image, WIDTH+2, i, 1, 1);
                }
            }

            start_msg_.set_text("waiting for other users...");
            m_grid_.attach(start_msg_, int(WIDTH/2) - 4, 3, 8, 1);

            player_msg_.set_text("You");

            // show grid
            m_grid_.show_all_children();
            m_grid_.show();
        }

        ~GameWindow() { printf("~\n");}

        void input_id(int id) {
            client_id_ = id;
        }

        void input_images(std::string imagePath) {
            Gtk::Image *image = Gtk::manage(new Gtk::Image(imagePath));
            images_.push_back(image);
        }

        void update_status(int id, int status, int x, int y) {
            if(id == 0 && status == 0) end_game();
            else if(status == 4 && !isStart_) print_start();

            m_grid_.remove(*images_[id]);
            
            if(id==0 || status == 0) {
                m_grid_.attach(*images_[id], x+1,y+1, 2, 2);
                if(count_ && id==0) count_++;
            } else if(status == 1) {    //zombe client
                m_grid_.remove(*images_[id+MAX_CLIENT_NUM]);
                m_grid_.attach(*images_[id+MAX_CLIENT_NUM], x+1,y+1, 2,2 );
            }

            if(id==client_id_) {
                m_grid_.remove(player_msg_);
                if(y<HIGHT/2) m_grid_.attach(player_msg_, x+1, y+3, 1, 1);
                else m_grid_.attach(player_msg_, x+1, y-1, 1, 1);
            }

            if(count_>4) {
                m_grid_.remove(start_msg_);
                count_ = 0;
            }

            show_all();
            // queue_draw();
        }
    private:
        
        void print_start() {
            count_++;
            isStart_ = true;

            printf("print start\n");
            m_grid_.remove(start_msg_);

            start_msg_.set_text("Game Start!");
            m_grid_.attach(start_msg_, int(WIDTH/2) - 2, 2, 4, 1);
        }

        void end_game() {
            start_msg_.set_text("Game Ended!");
            m_grid_.attach(start_msg_, int(WIDTH/2) - 2, 2, 4, 1);
            sleep(2);
            hide();
        }

        // Child widget
        Gtk::Grid m_grid_;
        std::vector<Gtk::Image*> images_;

        Gtk::Label start_msg_;
        Gtk::Label player_msg_;
        int client_id_;
        int count_;

        bool isStart_;
};


#endif // GTKMM_GAMEWINDOW
