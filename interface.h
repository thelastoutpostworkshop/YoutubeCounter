// Class to manage the interface activated with the Rotary Encoder
//
enum Interface_Mode
{
    NORMAL,
    VIEWCOUNT,
    VOLUME,
    Interface_Mode_COUNT
};

class InterfaceMode
{
    Interface_Mode mode;
    unsigned long lastChangeMillis;

public:
    InterfaceMode() : mode(NORMAL), lastChangeMillis(millis()) {}

    void nextMode()
    {
        mode = static_cast<Interface_Mode>((mode + 1) % Interface_Mode_COUNT);
        lastChangeMillis = millis();
    }

    void prevMode()
    {
        if (mode == 0)
        {
            mode = static_cast<Interface_Mode>(Interface_Mode_COUNT - 1);
        }
        else
        {
            mode = static_cast<Interface_Mode>(mode - 1);
        }
        lastChangeMillis = millis();
    }

    Interface_Mode getMode()
    {
        return mode;
    }

    void resetTime()
    {
        lastChangeMillis = millis();
    }

    bool checkReset()
    {
        unsigned long currentMillis = millis();
        unsigned long delayMillis = 10000; // 10 seconds

        if (mode != NORMAL && currentMillis - lastChangeMillis >= delayMillis)
        {
            mode = NORMAL;
            return true;
        }
        return false;
    }
};
