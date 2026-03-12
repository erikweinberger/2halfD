#ifndef INPUT_TYPES_H
#define INPUT_TYPES_H

#include "TwoHalfD/types/math_types.h"

namespace TwoHalfD {

struct Event {
    enum class Type {
        None,
        KeyPressed,
        KeyReleased,
        MouseMoved,
        MouseButtonPressed,
        MouseButtonReleased,
    };

    Type type{Type::None};

    struct KeyEvent {
        int keyCode;
        int x, y;
    };

    struct MouseMoveEvent {
        int x, y;
        XYVector moveDelta;
    };

    struct MouseButtonEvent {
        int button;
        int x, y;
    };

    union {
        KeyEvent key;
        MouseMoveEvent mouseMove;
        MouseButtonEvent mouseButton;
    };

    Event() : type(Type::None) {}

    static Event KeyPressed(int keyCode, int x = 0, int y = 0) {
        Event e;
        e.type = Type::KeyPressed;
        e.key = {keyCode, x, y};
        return e;
    }

    static Event KeyReleased(int keyCode, int x = 0, int y = 0) {
        Event e;
        e.type = Type::KeyReleased;
        e.key = {keyCode, x, y};
        return e;
    }

    static Event MouseMoved(int x, int y, XYVector moveDelta) {
        Event e;
        e.type = Type::MouseMoved;
        e.mouseMove = {x, y, moveDelta};
        return e;
    }

    static Event MouseButtonPressed(int button, int x, int y) {
        Event e;
        e.type = Type::MouseButtonPressed;
        e.mouseButton = {button, x, y};
        return e;
    }

    static Event MouseButtonReleased(int button, int x, int y) {
        Event e;
        e.type = Type::MouseButtonReleased;
        e.mouseButton = {button, x, y};
        return e;
    }
};

enum keyCodeEnum {
    a = 0,
    b,
    c,
    d,
    e,
    f,
    g,
    h,
    i,
    j,
    k,
    l,
    m,
    n,
    o,
    p,
    q,
    r,
    s,
    t,
    u,
    v,
    w,
    x,
    y,
    z
};

} // namespace TwoHalfD

#endif
