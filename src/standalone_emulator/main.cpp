#include "application.hpp"

int main()
{
    Application app;
    while (app.isRunning())
    {
        app.update();
        app.draw();
    }
}