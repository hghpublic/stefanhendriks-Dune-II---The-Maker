//
// Created by shendriks on 9/3/2020.
//

#ifndef D2TM_CRECTANGLE_H
#define D2TM_CRECTANGLE_H


class cRectangle {
public:
    cRectangle(int x, int y, int width, int height) : x(x), y(y), height(height), width(width) {
    }

    bool isWithin(int pointX, int pointY) {
        return (pointX >= x && pointX <= (x + width)) &&
               (pointY >= y && pointY <= (y + height));
    }

    static bool isWithin(int pointX, int pointY, int x, int y, int width, int height) {
        return (pointX >= x && pointX <= (x + width)) &&
               (pointY >= y && pointY <= (y + height));
    }


private:
    int x;
    int y;
    int width;
    int height;

};


#endif //D2TM_CRECTANGLE_H
