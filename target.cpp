#include <iostream>

int main() {
    int x = 0;
    int y = 1;

    auto a = [&] { std::cout << x << y; };      // захоплення за посиланням
    auto b = [x, y] { std::cout << x << y; };   // тільки за значенням
    auto c = [&x, y] { std::cout << x << y; };  // теж є посилання

    a();
    b();
    c();
}
