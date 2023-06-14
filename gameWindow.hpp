#ifndef GTKMM_GAMEWINDOW
#define GTKMM_GAMEWINDOW

#include <gtkmm/box.h>
#include <gtkmm/image.h>
#include <gtkmm/window.h>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <fstream>
#include <thread>
#include <boost/asio.hpp>
#include <cstring>

class GameWindow : public Gtk::Window
{
    public:
        GameWindow()
            // :m_box_(Gtk::ORIENTATION_HORIZONTAL, 5)
        {
            set_title("Game Start!");

            set_default_size(600, 600);

            set_border_width(10);

            // put the box into the main window.
            add(m_box1);

            // Now when the button is clicked, we call the "on_button_clicked" function
            // with a pointer to "button 1" as it's argument
            m_button1.signal_clicked().connect(sigc::bind<Glib::ustring>(
                        sigc::mem_fun(*this, &GameWindow::on_button_clicked), "button 1"));

            // instead of gtk_container_add, we pack this button into the invisible
            // box, which has been packed into the window.
            // note that the pack_start default arguments are Gtk::EXPAND | Gtk::FILL, 0
            m_box1.pack_start(m_button1);

            // always remember this step, this tells GTK that our preparation
            // for this button is complete, and it can be displayed now.
            m_button1.show();

            // call the same signal handler with a different argument,
            // passing a pointer to "button 2" instead.
            m_button2.signal_clicked().connect(sigc::bind<-1, Glib::ustring>(
                        sigc::mem_fun(*this, &GameWindow::on_button_clicked), "button 2"));

            m_box1.pack_start(m_button2);

            // Show the widgets.
            // They will not really be shown until this Window is shown.
            m_button2.show();
            m_box1.show();

            // signal_key_press_event().connect(
            //     sigc::mem_fun(*this, &ChoosePlayer4::choice_client_event));
        }

    private:
        void on_button_clicked(Glib::ustring data)
        {
        std::cout << "Hello World - " << data << " was pressed" << std::endl;
        }
        // Child widgets:
        Gtk::Box m_box1;
        Gtk::Button m_button1, m_button2;
};


#endif // GTKMM_GAMEWINDOW
