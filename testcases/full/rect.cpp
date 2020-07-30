struct Rect {
    enum Color { RED, GREEN, BLUE }; 

    int area() { return length * height; }
    int perimeter() { return 2 * (length + height); }
    int getColor(int i) { return i < 0 || i > 3 ? RED : colors[i]; }

    int length;
    int height;
    Color colors[4]; 
};

// int main() {
//     Rect rect;
//     rect.length = 10;
//     rect.height = 5;

//     //return rect.area() + rect.perimeter();
//     return 0;
// }